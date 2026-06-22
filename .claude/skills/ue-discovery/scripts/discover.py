#!/usr/bin/env python3
"""ue-discovery 主入口：读 seeds.yaml → 轮询种子源 → 去重 → 评分 → 写 queue.json

设计 v2：视频爬取剔除，改为人工维护 video-links.md
当前可用渠道：Bilibili 搜索（bili-cli + 代理）
"""

import argparse
import json
import re
import subprocess
import sys
from datetime import datetime, timezone, timedelta
from pathlib import Path
from urllib.parse import urlparse, urlunparse, parse_qsl, urlencode

try:
    import yaml
except ImportError:
    sys.exit("需要 pyyaml: pip install pyyaml")


def load_seeds(path: Path) -> dict:
    with path.open(encoding="utf-8") as f:
        return yaml.safe_load(f)


def normalize_url(url: str) -> str:
    """标准化 URL 去除追踪参数用于精确去重"""
    p = urlparse(url)
    tracking_prefixes = {"utm_", "fbclid", "gclid", "ref", "source"}
    clean_qs = [(k, v) for k, v in parse_qsl(p.query) if not any(k.startswith(pre) for pre in tracking_prefixes)]
    return urlunparse((p.scheme, p.netloc, p.path.rstrip("/"), "", urlencode(clean_qs), ""))


def title_jaccard(a: str, b: str) -> float:
    """标题 Jaccard 相似度（小写 token 化）"""
    ta = set(re.findall(r"\w+", a.lower()))
    tb = set(re.findall(r"\w+", b.lower()))
    if not ta or not tb:
        return 0.0
    return len(ta & tb) / len(ta | tb)


def slugify(text: str, maxlen: int = 60) -> str:
    """ASCII 安全 slug 生成"""
    text = text.lower().strip()
    text = re.sub(r"[^a-z0-9\s-]", "", text)
    text = re.sub(r"[\s_-]+", "-", text).strip("-")
    return text[:maxlen]


def score_entry(entry: dict, seeds: dict) -> tuple[int, dict, list[str]]:
    """返回 (总分, 分项, 命中关键词)"""
    keywords = []
    for bucket in ("core", "systems", "animation", "combat", "cpp", "pipeline"):
        keywords.extend(seeds.get("keywords", {}).get(bucket, []))

    text = f"{entry.get('title', '')} {entry.get('description', '')}".lower()
    matched = [kw for kw in keywords if kw.lower() in text]
    kw_score = min(len(matched) * 5, 40)

    channel_score = 35 if entry.get("trusted") else 0

    published = entry.get("published_at")
    recency_score = 0
    if published:
        try:
            pub_dt = datetime.fromisoformat(published.replace("Z", "+00:00"))
            days_ago = (datetime.now(timezone.utc) - pub_dt).days
            if days_ago <= 7:
                recency_score = 25
            elif days_ago <= 30:
                recency_score = 15
            elif days_ago <= 90:
                recency_score = 5
        except (ValueError, TypeError):
            pass

    total = kw_score + channel_score + recency_score
    return total, {"keywords": kw_score, "channel": channel_score, "recency": recency_score}, matched


def is_video_source(source_type: str, platform: str) -> bool:
    return platform in ("youtube", "bilibili") and source_type == "video"


def poll_bilibili_search(query_config: dict) -> list[dict]:
    """用 bili search 搜索 Bilibili 视频"""
    query = query_config["query"]
    search_type = query_config.get("type", "video")
    limit = query_config.get("limit", 10)

    try:
        result = subprocess.run(
            ["bili", "search", query, "--type", search_type, "-n", str(limit)],
            capture_output=True, text=True, timeout=60, check=False
        )
        items = []
        # 解析 bili search 输出（YAML 格式）
        in_data = False
        for line in result.stdout.splitlines():
            if line.startswith("data:"):
                in_data = True
                continue
            if in_data and line.startswith("- id:"):
                # 开始新条目
                pass
            if in_data and line.startswith("  title:"):
                title = line.split("title:", 1)[1].strip()
                items.append({
                    "title": title,
                    "url": "",  # bili search 不直接输出 URL，需要拼接
                    "author": query_config["name"],
                    "author_url": "",
                    "platform": "bilibili",
                    "source_type": "video",
                    "published_at": None,
                    "duration_sec": None,
                    "trusted": query_config.get("trusted", False),
                    "description": f"Bilibili 搜索结果: {query}",
                })
        return items
    except (subprocess.TimeoutExpired, FileNotFoundError) as e:
        print(f"[WARN] Bilibili 搜索失败 '{query}': {e}", file=sys.stderr)
        return []


def main():
    ap = argparse.ArgumentParser(description="UE 社区种子源发现")
    ap.add_argument("--seeds", required=True, help="seeds.yaml 路径")
    ap.add_argument("--out", default="queue.json", help="输出 queue.json 路径")
    ap.add_argument("--min-score", type=int, default=30, help="及格线（默认 30）")
    ap.add_argument("--since", type=int, default=90, help="只看 N 天内（默认 90）")
    ap.add_argument("--limit", type=int, default=50, help="最多产出 N 条（默认 50）")
    ap.add_argument("--dry-run", action="store_true", help="只打印不写文件")
    args = ap.parse_args()

    seeds = load_seeds(Path(args.seeds))

    candidates = []

    # Bilibili 搜索（当前唯一可用的自动轮询渠道）
    for query_config in seeds.get("bilibili_search", []):
        candidates.extend(poll_bilibili_search(query_config))

    # 占位日志（已剔除的渠道）
    for sub in seeds.get("reddit_subreddits", []):
        print(f"[INFO] Reddit 轮询需登录态，暂未启用: {sub['name']}", file=sys.stderr)
    for feed in seeds.get("rss_feeds", []):
        print(f"[INFO] RSS 轮询被 CAPTCHA 拦截，暂未启用: {feed['name']}", file=sys.stderr)
    for repo in seeds.get("github_repos", []):
        print(f"[INFO] GitHub 轮询 API 超时，暂未启用: {repo['name']}", file=sys.stderr)

    # 去重
    seen_urls = set()
    seen_titles = []
    deduped = []
    for c in candidates:
        if not c.get("url"):
            # Bilibili 搜索结果可能没有直接 URL，用标题去重
            if any(title_jaccard(c.get("title", ""), t) > 0.8 for t in seen_titles):
                continue
            seen_titles.append(c.get("title", ""))
            deduped.append(c)
            continue
        norm = normalize_url(c["url"])
        if norm in seen_urls:
            continue
        if any(title_jaccard(c.get("title", ""), t) > 0.8 for t in seen_titles):
            continue
        seen_urls.add(norm)
        seen_titles.append(c.get("title", ""))
        deduped.append(c)

    # 评分
    scored = []
    for c in deduped:
        total, breakdown, matched = score_entry(c, seeds)
        if total < args.min_score:
            continue
        c["relevance_score"] = total
        c["score_breakdown"] = breakdown
        c["matched_keywords"] = matched
        c["slug"] = slugify(f"{c['author']}-{c['title']}-{c.get('published_at', 'unknown')[:10] if c.get('published_at') else 'nodate'}")
        c["download_required"] = is_video_source(c["source_type"], c["platform"])
        c["raw_dir"] = f"raw/{c['slug']}"
        scored.append(c)

    scored.sort(key=lambda x: x["relevance_score"], reverse=True)
    scored = scored[:args.limit]

    queue = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "seed_version": str(Path(args.seeds).resolve()),
        "total": len(scored),
        "status": "pending_review",
        "items": scored,
    }

    if args.dry_run:
        print(json.dumps(queue, indent=2, ensure_ascii=False))
        return

    Path(args.out).write_text(json.dumps(queue, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"[OK] queue.json 写入 {args.out}：{len(scored)} 条")
    print(f"     download_required (人工下载): {sum(1 for x in scored if x['download_required'])} 条")
    print(f"     Agent 直读 (帖子/文章): {sum(1 for x in scored if not x['download_required'])} 条")
    print(f"     Top 5:")
    for i, x in enumerate(scored[:5], 1):
        print(f"       {i}. [{x['relevance_score']}] {x['title']} — {x['author']}")


if __name__ == "__main__":
    main()
