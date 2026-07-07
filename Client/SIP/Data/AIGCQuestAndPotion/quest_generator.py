# quest_generator.py

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


# =========================
# Prompt Builder
# =========================

def build_quest_generation_prompt(
    items,
    npcs,
    world_state,
    story_memory,
    player_state
):
    prompt_data = {
        "items": items,
        "npcs": npcs,
        "world_state": world_state,
        "story_memory": story_memory,
        "player_state": player_state,
        "required_output_format": {
            "quest_title": "Short quest title.",
            "quest_content": "A letter-style quest request written by a suitable NPC."
        }
    }

    return f"""
You are the quest generator for an RPG world.

You receive:
- item database
- NPC database
- current world state
- story memory
- player state

Your job:
Generate exactly one new quest.

Important rules:
- Return valid JSON only.
- Do not return Markdown.
- Do not wrap JSON in code fences.
- Only generate quest_title and quest_content.
- Do not generate rewards.
- Do not generate required items.
- The quest should naturally follow from the current world state and story memory.
- The quest content should be written as a letter-style request from a suitable NPC.
- The NPC should come from the provided NPC data.
- The quest may imply what the player should help with, but do not output a separate item list.

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
        model="deepseek-v4-pro",
        messages=[
            {
                "role": "system",
                "content": "You are an RPG quest generator. Return valid JSON only."
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
                "type": "enabled"
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
# Validation
# =========================

def validate_generated_quest(generated_quest):
    errors = []

    quest_title = generated_quest.get("quest_title")
    quest_content = generated_quest.get("quest_content")

    if not isinstance(quest_title, str) or not quest_title.strip():
        errors.append("quest_title is missing or empty.")

    if not isinstance(quest_content, str) or not quest_content.strip():
        errors.append("quest_content is missing or empty.")

    return errors


# =========================
# Main Generator Function
# =========================

def generate_quest():
    """
    Generate one new quest from current data files.

    Returns:
        {
            "quest_title": str,
            "quest_content": str
        }
    """

    print("========== LLM Quest Generator ==========\n")

    items = load_json(config.ITEMS_PATH)
    npcs = load_json(config.NPCS_PATH)
    world_state = load_json(config.WORLD_STATE_PATH)
    story_memory = load_json(config.STORY_MEMORY_PATH)
    player_state = load_json(config.PLAYER_STATE_PATH)

    prompt = build_quest_generation_prompt(
        items=items,
        npcs=npcs,
        world_state=world_state,
        story_memory=story_memory,
        player_state=player_state
    )

    print("Calling LLM to generate quest...\n")

    raw_response = call_llm(prompt)

    print("=== Raw LLM Response ===")
    print(raw_response)

    generated_quest = parse_llm_json(raw_response)

    errors = validate_generated_quest(generated_quest)

    if errors:
        print("\n[GENERATED QUEST INVALID]")
        for error in errors:
            print("-", error)
        return None

    final_quest = {
        "quest_title": generated_quest["quest_title"].strip(),
        "quest_content": generated_quest["quest_content"].strip()
    }

    print("\n=== Final Generated Quest ===")
    print(json.dumps(final_quest, ensure_ascii=False, indent=2))

    return final_quest


if __name__ == "__main__":
    generate_quest()