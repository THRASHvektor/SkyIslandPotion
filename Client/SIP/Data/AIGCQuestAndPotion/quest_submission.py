# quest.py

import json
from pathlib import Path
from openai import OpenAI

import config


# =========================
# JSON Helpers
# =========================

def load_json(path: Path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def build_item_map(items):
    return {item["id"]: item for item in items}


# =========================
# Local Validation
# =========================

def validate_submitted_items(submitted_items, item_map):
    errors = []

    for item in submitted_items:
        item_id = item.get("item_id")
        count = item.get("count")

        if item_id not in item_map:
            errors.append(f"Submitted item does not exist: {item_id}")
            continue

        if not isinstance(count, int) or count <= 0:
            errors.append(f"Invalid submitted count for {item_id}: {count}")

    return errors


def validate_rewards(reward_items, item_map):
    valid_rewards = []
    removed_rewards = []

    for reward in reward_items:
        item_id = reward.get("item_id")
        count = reward.get("count")

        if item_id not in item_map:
            removed_rewards.append({
                "reward": reward,
                "reason": "item_id does not exist in items.json"
            })
            continue

        if not isinstance(count, int) or count <= 0:
            removed_rewards.append({
                "reward": reward,
                "reason": "count must be a positive integer"
            })
            continue

        valid_rewards.append({
            "item_id": item_id,
            "item_name": item_map[item_id].get("name", item_id),
            "count": count,
            "reason": reward.get("reason", "")
        })

    return valid_rewards, removed_rewards


# =========================
# Prompt Builder
# =========================

def build_prompt(
    quest_title,
    quest_content,
    submitted_items,
    items,
    npcs,
    world_state,
    story_memory,
    player_state
):
    prompt_data = {
        "quest_title": quest_title,
        "quest_content": quest_content,
        "submitted_items": submitted_items,
        "items": items,
        "npcs": npcs,
        "world_state": world_state,
        "story_memory": story_memory,
        "player_state": player_state,
        "required_output_format": {
            "judgement": {
                "status": "success | partial_success | failure",
                "quality": "excellent | good | acceptable | poor",
                "reason": "Explain how well the player completed the task."
            },
            "reply_letter": "A letter-style reply to the player.",
            "reward_items": [
                {
                    "item_id": "Must be an id from the provided items list.",
                    "count": "Positive integer.",
                    "reason": "Why this reward fits the task, history, NPC, and world state."
                }
            ],
            "world_update": {
                "public_summary": "What people in the world may notice after this quest.",
                "hidden_summary": "Author-only consequence that may influence later quests.",
                "possible_future_quests": [
                    "Possible future quest idea influenced by this result."
                ]
            },
            "memory_summary": "Short summary that can be appended to story memory."
        }
    }

    return f"""
You are the world judge and narrative engine for an RPG quest system.

You receive:
- quest title
- quest content
- submitted items
- item database
- NPC database
- current world state
- story memory
- player state

You must:
1. Judge whether the submitted items complete the quest.
2. Write a letter-style reply from the most suitable NPC or quest giver.
3. Generate appropriate rewards.
4. Suggest world-state changes.
5. Summarize long-term story memory.

Important rules:
- Return valid JSON only.
- Do not return Markdown.
- Do not wrap JSON in code fences.
- reward_items must only use item_id values from the provided items list.
- Do not invent item_id values.
- You may freely judge task success, reward value, world consequences, and story connections.
- The reply letter should fit the NPC, quest situation, world state, and story history.

Input:
{json.dumps(prompt_data, ensure_ascii=False, indent=2)}
""".strip()


# =========================
# LLM Call
# =========================

def call_llm(prompt):
    if not config.LLM_API_KEY:
        raise RuntimeError(
            "LLM_API_KEY is missing. Please check config.py or your environment variable."
        )

    client = OpenAI(
        api_key=config.LLM_API_KEY,
        base_url=config.LLM_BASE_URL
    )

    response = client.chat.completions.create(
        model="deepseek-v4-flash",
        messages=[
            {
                "role": "system",
                "content": "You are an RPG world judge. Return valid JSON only."
            },
            {
                "role": "user",
                "content": prompt
            }
        ],
        stream=False,
        reasoning_effort="high",
        extra_body={
            "thinking": {
                "type": "disabled"
            }
        }
    )

    return response.choices[0].message.content


def parse_llm_json(raw_text):
    try:
        return json.loads(raw_text)
    except json.JSONDecodeError:
        print("\n=== Raw LLM Output ===")
        print(raw_text)
        raise


# =========================
# Main Function
# =========================

def run_quest(quest_title, quest_content, submitted_items):
    """
    Run one quest judgement.

    Parameters:
        quest_title: str
        quest_content: str
        submitted_items: list[dict]
            Example:
            [
                {"item_id": "medium_mana_potion", "count": 10}
            ]

    Returns:
        dict: final validated LLM result
    """

    print("========== LLM Quest Runner ==========\n")

    items = load_json(config.ITEMS_PATH)
    npcs = load_json(config.NPCS_PATH)
    world_state = load_json(config.WORLD_STATE_PATH)
    story_memory = load_json(config.STORY_MEMORY_PATH)
    player_state = load_json(config.PLAYER_STATE_PATH)

    item_map = build_item_map(items)

    print("Quest Title:")
    print(quest_title)

    print("\nQuest Content:")
    print(quest_content)

    print("\nSubmitted Items:")
    print(json.dumps(submitted_items, ensure_ascii=False, indent=2))

    submit_errors = validate_submitted_items(submitted_items, item_map)

    if submit_errors:
        print("\n[SUBMISSION INVALID]")
        for error in submit_errors:
            print("-", error)
        return None

    print("\n[SUBMISSION OK]")

    prompt = build_prompt(
        quest_title=quest_title,
        quest_content=quest_content,
        submitted_items=submitted_items,
        items=items,
        npcs=npcs,
        world_state=world_state,
        story_memory=story_memory,
        player_state=player_state
    )

    print("\nCalling LLM...\n")

    raw_response = call_llm(prompt)

    print("=== Raw LLM Response ===")
    print(raw_response)

    llm_result = parse_llm_json(raw_response)

    reward_items = llm_result.get("reward_items", [])
    valid_rewards, removed_rewards = validate_rewards(reward_items, item_map)

    llm_result["reward_items"] = valid_rewards

    print("\n=== Final Quest Result ===")
    print(json.dumps(llm_result, ensure_ascii=False, indent=2))

    if removed_rewards:
        print("\n=== Removed Invalid Rewards ===")
        print(json.dumps(removed_rewards, ensure_ascii=False, indent=2))

    return llm_result


# =========================
# Test Data
# =========================

if __name__ == "__main__":
    quest_title = "Mana Potions for the Northern Army"

    quest_content = """
General Arden sent a formal request asking the player to deliver mana potions
for an important northern military operation. He did not explain the full purpose
of the operation, but the request sounded urgent and well-funded.

The player delivered the requested mana potions on time.
""".strip()

    submitted_items = [
        {
            "item_id": "medium_mana_potion",
            "count": 10
        }
    ]

    run_quest(
        quest_title=quest_title,
        quest_content=quest_content,
        submitted_items=submitted_items
    )