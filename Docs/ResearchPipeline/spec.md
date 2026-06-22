# UE 社区调研管线 — 设计规格

**状态**：已拍板，待转实施计划
**日期**：2026-06-22
**服务对象**：SkyIslandPotion UE 项目
**拍板人**：用户

---

## 1. 目标

为 SkyIslandPotion 项目调研 Unreal Engine 开发社区的实践现状，产出**工程情报**（可借鉴实现、踩坑规避、插件/素材清单、技术选型依据），不是学术综述。

调研对象具体化：

| 类别 | 平台 | 典型内容 |
|------|------|---------|
| 官方 | Unreal 官网 / forums / DevCon / GDC talks | release notes、官方教程、roadmap |
| 英文社区 | Reddit r/UnrealEngine、Discord、X 上的 UE 开发者、GitHub EpicGames/UnrealEngine + 第三方 plugin 仓库 | 问题讨论、插件、最佳实践 |
| 中文社区 | Bilibili UE 教程区、知乎 UE 专栏、UE 中国官方号、CSDN/掘金 | 中文教程、本地化经验 |
| 视频 | YouTube（Unreal Sensei / Ryan Laley / William Faichild 等）、Bilibili | 教程、case study、技术演示 |

## 2. 拍板结论

| 拍板点 | 选择 | 理由 |
|--------|------|------|
| 0a SkillSpector | 纳入工具栈 | 装任何新 skill 前必跑，64 类漏洞扫描 |
| 0b Agent-Reach | 替换 web-access 的"多平台读取+搜索"职责；web-access 保留 CDP 交互 | Agent-Reach 的专用 CLI 路由更稳、更轻；B 站 yt-dlp 已被风控封死，必须切 bili-cli |
| 1 调研重心 | **R1 视频为主** | ai-notes 视频管线基建已就绪；UE 视频教程是最高密度知识载体；`Docs/AgentLoop` 已跑通全链路 |
| 2 产出形态 | **B3 双轨** | Obsidian 做工作台（实体/概念网络+检索），LaTeX 出最终调研笔记 |
| 3 缺口补全 | **C2 两个新 skill** | `ue-discovery` + `ue-ingest`，边界沿"原料入库 / 知识消化"自然分界 |
| 4 skill 边界 | discovery 管"发现+队列"，ingest 管"结构化+归档" | 沿原料/知识分界，符合 superpowers 的小而专原则 |

后续 R2/R3（社区帖、官方资源）作为 P4 扩展项，不在 MVP 范围。

## 3. 工具栈定位

### 3.1 抓取层（复用 + 替换）

**人机分工原则**：视频下载交给人做（高摩擦低智力密度：反爬/cookie/验证码/m4s 合并），Agent 接管下载后的所有环节（高智力密度：转写/结构化/归档/检索/产出）。

| 职责 | 谁做 | 工具 | 状态 |
|------|------|------|------|
| YouTube 视频下载 | **人工** | yt-dlp / youtube-dl / 浏览器，人随意选 | 不纳入 Agent 管线 |
| B 站视频下载 | **人工** | BilibiliDown GUI / yt-dlp / 浏览器，人随意选 | 不纳入 Agent 管线（BilibiliDown 是 Java GUI，Agent 调不了；2026-06 yt-dlp 被 B 站风控 412 封死，人用 GUI 反而最稳） |
| 多平台读取（元数据/字幕/评论/搜索，不含下载） | Agent | **Agent-Reach** | 新装入 |
| CDP 浏览器交互（登录态操作、动态页面、点击导航、视频截帧） | Agent | **web-access** | 已装，保留 |
| 视频转写 | Agent | ai-notes 的 `transcribe_faster_whisper.py`（faster-whisper large-v3 + CUDA） | 已装，保留 |
| LaTeX/PDF 产出 | Agent | ai-notes 模板 + `render_pdf_qa.py` | 已装，保留 |

**下载环节的契约**：人下载完，把视频文件放进 `raw/<slug>/video.mp4`（或 `raw/<slug>/video.mkv` 等），Agent 从这里接手。`<slug>` 由 `ue-discovery` 在 `queue.json` 里预先分配，人按队列下载。

### 3.2 安检层

| 职责 | 工具 | 使用时机 |
|------|------|---------|
| skill 安全扫描 | **SkillSpector**（NVIDIA，64 漏洞模式，16 类） | 装任何新 skill 前必跑；已装 skill 一次性回扫 |

### 3.3 待建（两个新 skill）

| skill | 职责 | 输入 | 输出 |
|-------|------|------|------|
| `ue-discovery` | 监控 UE 种子源、去重、评分、产出待抓取队列 | `seeds.yaml`（种子源配置） | `queue.json`（去重+评分后的队列） |
| `ue-ingest` | 对抓取回来的原料做结构化提取、写入 Obsidian wiki、维护索引与检索 | `raw/<slug>/`（已抓取原料） | `.obsidian/wiki/{entities,concepts,sources}/` + `log.md` |

### 3.4 管线全景

```
┌─────────────────────────────────────────────────────────────┐
│  ue-discovery (新)                                           │
│  种子源：YouTube 频道列表 / Bilibili UP 主列表 /              │
│         Reddit r/UnrealEngine / X 上的 UE list / RSS         │
│  去重 + UE 主题过滤 + 相关度评分                              │
│  产出 queue.json（含 <slug> 预分配）                          │
└──────────────────┬──────────────────────────────────────────┘
                   ↓ queue.json
┌─────────────────────────────────────────────────────────────┐
│  人工下载（人做，Agent 不参与）                               │
│  - YouTube: yt-dlp / youtube-dl / 浏览器，人随意选            │
│  - B站: BilibiliDown GUI / 浏览器，人随意选                   │
│  - 放进 raw/<slug>/video.mp4                                 │
│  - 帖子/文章类原料：Agent-Reach 读取（无需人工）              │
└──────────────────┬──────────────────────────────────────────┘
                   ↓ raw/<slug>/{video.mp4, html, metadata}
┌─────────────────────────────────────────────────────────────┐
│  Agent 接手（自动）                                          │
│  - Agent-Reach: 帖子/文章读取 + Exa 搜索（非视频原料）        │
│  - web-access: CDP 交互兜底（登录态操作、动态页面）           │
│  - 转写: ai-notes/transcribe_faster_whisper.py（视频原料）    │
│  - 字幕: yt-dlp --write-sub（若视频原料已有字幕轨）           │
└──────────────────┬──────────────────────────────────────────┘
                   ↓ raw/<slug>/{transcript, frames, html, metadata}
┌─────────────────────────────────────────────────────────────┐
│  ue-ingest (新)                                              │
│  - 实体识别（作者/频道/插件/资产/项目）                       │
│  - 概念抽取（技术点/模式/最佳实践）                           │
│  - 引文时间戳（视频时间码 + 帖子 URL）                        │
│  - wiki 链接 + 矛盾检测（WARN flag）                          │
│  - 写入 .obsidian/wiki/{entities,concepts,sources}/         │
│  - 更新 index.md + log.md                                    │
└──────────────────┬──────────────────────────────────────────┘
                   ↓ wiki/
┌─────────────────────────────────────────────────────────────┐
│  产出层（复用）                                              │
│  - ai-notes: 从 wiki/ 选材 → LaTeX 调研笔记 → PDF            │
│  - 沿用 Docs/AgentLoop/ 的产出风格                           │
│  - 视觉 QA: render_pdf_qa.py                                 │
└─────────────────────────────────────────────────────────────┘
```

## 4. 两个新 skill 的职责边界

### 4.1 `ue-discovery`

- **输入**：`seeds.yaml`（种子源配置）
- **输出**：`queue.json`（去重+评分后的待抓取队列）
- **内部职责**：
  - 周期性轮询种子源
  - 跨源去重（URL 去重 + 标题相似度去重）
  - UE 主题相关度评分（关键词白名单 + 频道白名单加权）
  - 人工 review gate（产出队列后等用户确认再抓取）
- **不做**：抓取、结构化、归档、最终产出

### 4.2 `ue-ingest`

- **输入**：`raw/<slug>/`（已抓取的原料：视频/帧/转写/HTML/元数据）
- **输出**：`.obsidian/wiki/` 下的实体/概念/来源页 + `log.md` 条目
- **内部职责**：
  - 实体识别（作者、频道、插件、资产、项目名）
  - 概念抽取（技术点、模式、最佳实践、踩坑）
  - 引文时间戳（视频时间码 + 帖子 URL）
  - wiki 链接（`[[实体名]]` / `[[概念名]]`）
  - 矛盾检测（同实体多来源冲突时打 WARN flag）
  - 索引维护（`index.md` + `log.md`）
- **不做**：发现、抓取、最终综述

## 5. Obsidian wiki schema（P0 定）

### 5.1 目录结构

```
.obsidian/wiki/
├── entities/         # 实体页：作者、频道、插件、资产、项目
│   ├── _index.md
│   ├── unreal-sensei.md
│   ├── ryan-laley.md
│   ├── niagara-system.md       # 插件/系统名
│   └── ...
├── concepts/         # 概念页：技术点、模式、最佳实践
│   ├── _index.md
│   ├── gas-ability-setup.md
│   ├── lumen-pitfalls.md
│   └── ...
├── sources/          # 来源页：每个被调研的视频/帖子一个页
│   ├── _index.md
│   ├── 2026-06-22-unreal-sensei-niagara-tutorial.md
│   └── ...
├── index.md          # 总索引
└── log.md            # 摄入日志（append-only）
```

### 5.2 实体页字段

```yaml
---
type: entity
entity_type: author | channel | plugin | asset | project
name: Unreal Sensei
aliases: [unreal-sensei]
platforms: [youtube]
urls: [https://youtube.com/@UnrealSensei]
first_seen: 2026-06-22
last_updated: 2026-06-22
---
```

### 5.3 概念页字段

```yaml
---
type: concept
name: GAS Ability Setup
aliases: [GameplayAbilitySystem setup, GAS 配置]
related_entities: [[gameplay-ability-system]]
related_concepts: [[gas-ability-task], [[gas-effect]]
first_seen: 2026-06-22
last_updated: 2026-06-22
---
```

### 5.4 来源页字段

```yaml
---
type: source
source_type: video | post | article | doc
title: ...
url: ...
author: [[unreal-sensei]]
date: 2026-05-14
duration: 1820
ingested: 2026-06-22
status: ingested | reviewed | cited
key_timestamps:
  - t: "02:15"  desc: "..."
  - t: "08:42"  desc: "..."
entities: [[niagara-system], [[lumen]]
concepts: [[niagara-emitter-setup], [[lumen-pitfalls]]
---
```

## 6. 分阶段路线图

| 阶段 | 时长 | 产出 | 验收标准 |
|------|------|------|---------|
| **P0 设计冻结** | 1 天 | 本 spec 落盘 + commit | 用户 review 通过 |
| **P1 安检+替换** | 2 天 | SkillSpector 装好并回扫现有 skill；Agent-Reach 装好并扫过；Agent-Reach 的读取能力（非下载）验证通；faster-whisper CUDA 路径验证 | `agent-reach doctor` 全绿；SkillSpector 对 Agent-Reach 出报告且风险分可接受；`ctranslate2.get_cuda_device_count()` 返回 ≥1 |
| **P2 discovery MVP** | 4 天 | `ue-discovery` skill + 1 个种子源（YouTube UE 频道列表）跑通 | `queue.json` 产出 10 条以上有效条目 |
| **P3 ingest MVP** | 5 天 | `ue-ingest` skill + 对 P2 产出跑通 | `wiki/` 下生成 5+ 实体页、3+ 概念页 |
| **P4 端到端跑批** | 3 天 | 用 1 个真实主题（如"UE5 Niagara 入门")跑 20 条原料 | 一份小型调研 PDF 产出（Docs/ResearchPipeline/ 下） |
| **P5 扩源** | 5 天 | 加 Bilibili UP 主 / Reddit / X 种子；加检索索引 | 跨源去重 + 全文检索可用 |
| **P6 出综述** | 5 天 | 按选定主题出第一份正式调研 PDF | 用户 review + 视觉 QA 通过 |

MVP 边界：P0-P4（约 2 周）。P5/P6 视 MVP 结果再决定是否启动。

## 7. 风险与已知缺口

1. **web-access 的 CDP 模式在 Windows 上未验证**——P1 需先跑 `check-deps.mjs` 确认。Agent-Reach 装好后，web-access 的多平台读取职责被接管，CDP 仅作兜底，风险降低。
2. **faster-whisper CUDA 路径**——ai-notes 的 AGENTS.md 提到"本机可能 CPU-only Torch 而 CTranslate2 能用 CUDA"。P1 需验证 `ctranslate2.get_cuda_device_count()`，多小时视频 CPU 转写不可接受。
3. **人工下载是管线瓶颈**——视频下载交给人做，意味着队列规模要控制。ue-discovery 的 queue.json 应做优先级排序，让人能挑高价值的先下。单批建议 ≤20 条视频。
4. **Obsidian 检索对中文分词**——P5 检索层可能需要引入 embed 索引，Obsidian 自带搜索对中文一般。
5. **superpowers 的 HARD-GATE**——brainstorming skill 要求 spec 写完→用户 review→才转 writing-plans。本 spec 遵守此流程。
6. **Agent-Reach 自称"纯 vibe coding"**——作者原话。P1 用 SkillSpector 扫它。

## 8. 安装前规范

装任何新 skill / CLI 工具前：

1. 先用 SkillSpector 扫源码：`skillspector scan <path-or-url>`
2. 风险分 ≤ 20（LOW）才装
3. 21-50（MEDIUM）需用户确认
4. 51+ 不装

## 9. 不做的事（YAGNI）

- 不自建视频下载器（下载交给人做）
- 不把视频下载纳入 Agent 管线（GUI 工具如 BilibiliDown Agent 调不了；CLI 下载器受平台风控波动大，维护成本高）
- 不自建转写模型（用 faster-whisper）
- 不自建搜索引擎（Exa + Obsidian 自带）
- 不做实时监控（周期性轮询足够）
- 不做 R2/R3（社区帖、官方资源）在 MVP 内
- 不做学术综述
- 不做多人协作

## 10. 后续动作

本 spec 经用户 review 通过后，进 superpowers 的 `writing-plans` skill 出详细实施计划，按 P0→P6 分阶段执行。
