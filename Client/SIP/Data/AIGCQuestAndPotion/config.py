from pathlib import Path


# =========================
# Paths
# =========================

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"

ITEMS_PATH = DATA_DIR / "items.json"
NPCS_PATH = DATA_DIR / "npcs.json"
WORLD_STATE_PATH = DATA_DIR / "world_state.json"
STORY_MEMORY_PATH = DATA_DIR / "story_memory.json"
PLAYER_STATE_PATH = DATA_DIR / "player_state.json"


# =========================
# LLM Config
# =========================

LLM_BASE_URL = "https://api.deepseek.com"
LLM_API_KEY = "sk-f1a00400134947218b3666d776e8789e"