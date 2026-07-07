# check_config.py

import json
from pathlib import Path
from openai import OpenAI

import config


# =========================
# 1. Check Data Paths Exist
# =========================

def check_data_paths_exist():
    print("=== 1. Checking Data Paths Exist ===")

    paths = {
        "ITEMS_PATH": config.ITEMS_PATH,
        "NPCS_PATH": config.NPCS_PATH,
        "WORLD_STATE_PATH": config.WORLD_STATE_PATH,
        "STORY_MEMORY_PATH": config.STORY_MEMORY_PATH,
        "PLAYER_STATE_PATH": config.PLAYER_STATE_PATH,
    }

    all_ok = True

    for name, path in paths.items():
        path = Path(path)

        if path.exists() and path.is_file():
            print(f"[OK] {name}: {path}")
        else:
            print(f"[FAILED] {name}: {path} does not exist or is not a file")
            all_ok = False

    return all_ok


# =========================
# 2. Check Data Content Valid
# =========================

def check_data_content_valid():
    print("\n=== 2. Checking JSON Content Valid ===")

    paths = {
        "ITEMS_PATH": config.ITEMS_PATH,
        "NPCS_PATH": config.NPCS_PATH,
        "WORLD_STATE_PATH": config.WORLD_STATE_PATH,
        "STORY_MEMORY_PATH": config.STORY_MEMORY_PATH,
        "PLAYER_STATE_PATH": config.PLAYER_STATE_PATH,
    }

    all_ok = True

    for name, path in paths.items():
        path = Path(path)

        try:
            with open(path, "r", encoding="utf-8") as f:
                json.load(f)

            print(f"[OK] {name}: valid JSON")

        except FileNotFoundError:
            print(f"[FAILED] {name}: file not found")
            all_ok = False

        except json.JSONDecodeError as e:
            print(f"[FAILED] {name}: invalid JSON")
            print(f"         line {e.lineno}, column {e.colno}: {e.msg}")
            all_ok = False

        except Exception as e:
            print(f"[FAILED] {name}: cannot read file")
            print(f"         {e}")
            all_ok = False

    return all_ok


# =========================
# 3. Check LLM Config Exists
# =========================

def check_llm_config_exists():
    print("\n=== 3. Checking LLM Config Exists ===")

    all_ok = True

    llm_base_url = getattr(config, "LLM_BASE_URL", None)
    llm_api_key = getattr(config, "LLM_API_KEY", None)

    if llm_base_url:
        print(f"[OK] LLM_BASE_URL: {llm_base_url}")
    else:
        print("[FAILED] LLM_BASE_URL is missing or empty")
        all_ok = False

    if llm_api_key:
        print("[OK] LLM_API_KEY: exists")
    else:
        print("[FAILED] LLM_API_KEY is missing or empty")
        all_ok = False

    return all_ok


# =========================
# 4. Check LLM Connection
# =========================

def check_llm_connection():
    print("\n=== 4. Checking LLM Connection ===")

    try:
        client = OpenAI(
            api_key=config.LLM_API_KEY,
            base_url=config.LLM_BASE_URL
        )

        response = client.chat.completions.create(
            model="deepseek-v4-pro",
            messages=[
                {
                    "role": "system",
                    "content": "You are a connection test assistant."
                },
                {
                    "role": "user",
                    "content": "Reply with only: OK"
                }
            ],
            stream=False,
            reasoning_effort="high",
            extra_body={
                "thinking": {
                    "type": "enabled"
                }
            }
        )

        content = response.choices[0].message.content

        print("[OK] LLM connected successfully")
        print("LLM response:", content)

        return True

    except Exception as e:
        print("[FAILED] LLM connection failed")
        print(e)
        return False


# =========================
# Main
# =========================

def main():
    print("========== LLM Quest Harness Config Check ==========\n")

    path_exists_ok = check_data_paths_exist()
    content_valid_ok = check_data_content_valid()
    llm_config_ok = check_llm_config_exists()

    if llm_config_ok:
        llm_connection_ok = check_llm_connection()
    else:
        print("\n=== 4. Checking LLM Connection ===")
        print("[SKIPPED] LLM config is invalid, skip connection test")
        llm_connection_ok = False

    print("\n========== Final Result ==========")

    print("Path exists check:       ", "OK" if path_exists_ok else "FAILED")
    print("JSON content check:      ", "OK" if content_valid_ok else "FAILED")
    print("LLM config check:        ", "OK" if llm_config_ok else "FAILED")
    print("LLM connection check:    ", "OK" if llm_connection_ok else "FAILED")

    if path_exists_ok and content_valid_ok and llm_config_ok and llm_connection_ok:
        print("\n[SUCCESS] All checks passed.")
    else:
        print("\n[FAILED] Some checks failed.")


if __name__ == "__main__":
    main()