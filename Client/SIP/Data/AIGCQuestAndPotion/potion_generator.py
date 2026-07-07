# potion_generator.py

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


def get_existing_potions(items):
    return [
        item for item in items
        if item.get("type") == "Potion"
    ]


# =========================
# Local Validation
# =========================

def validate_materials(material_items, item_map):
    errors = []

    for material in material_items:
        item_id = material.get("item_id")
        count = material.get("count", 1)

        if item_id not in item_map:
            errors.append(f"Material item does not exist: {item_id}")
            continue

        item_data = item_map[item_id]

        if item_data.get("type") != "Material":
            errors.append(f"Item is not a material: {item_id}")

        if not isinstance(count, int) or count <= 0:
            errors.append(f"Invalid count for material {item_id}: {count}")

    return errors


def validate_selected_potion(generated_result, item_map):
    errors = []

    potion_id = generated_result.get("potion_id")

    if not isinstance(potion_id, str) or not potion_id.strip():
        errors.append("potion_id is missing or empty.")
        return errors

    potion_id = potion_id.strip()

    if potion_id not in item_map:
        errors.append(f"Selected potion does not exist in items.json: {potion_id}")
        return errors

    potion_data = item_map[potion_id]

    if potion_data.get("type") != "Potion":
        errors.append(f"Selected item is not a Potion: {potion_id}")

    return errors


# =========================
# Prompt Builder
# =========================

def build_potion_selection_prompt(
    material_items,
    items,
    world_state,
    story_memory
):
    item_map = build_item_map(items)

    selected_materials = []

    for material in material_items:
        item_id = material["item_id"]
        count = material.get("count", 1)
        item_data = item_map[item_id]

        selected_materials.append({
            "id": item_data["id"],
            "name": item_data["name"],
            "type": item_data["type"],
            "description": item_data["description"],
            "count": count
        })

    existing_potions = get_existing_potions(items)

    prompt_data = {
        "selected_materials": selected_materials,
        "available_potions": existing_potions,
        "world_state": world_state,
        "story_memory": story_memory,
        "required_output_format": {
            "potion_id": "Must be one id from available_potions.",
            "reason": "Explain why this potion can be made from the selected materials."
        }
    }

    return f"""
You are an RPG alchemy crafting judge.

You receive:
- selected crafting materials
- available potion items from the item database
- current world state
- story memory

Your job:
Choose exactly one potion from available_potions that can reasonably be crafted
from the selected materials.

Important rules:
- Return valid JSON only.
- Do not return Markdown.
- Do not wrap JSON in code fences.
- Do not invent a new potion.
- potion_id must be one existing id from available_potions.
- If multiple potions are possible, choose the best fit based on material descriptions.
- If no potion fits perfectly, choose the closest reasonable potion from available_potions.
- The result must only include: potion_id and reason.

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
                "content": "You are an RPG alchemy crafting judge. Return valid JSON only."
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
# Main Generator Function
# =========================

def generate_potion(material_items):
    """
    Select one existing potion from items.json based on selected materials.

    Parameters:
        material_items: list[dict]
            Example:
            [
                {"item_id": "moon_grass", "count": 2},
                {"item_id": "clear_spring_water", "count": 1},
                {"item_id": "mana_crystal_shard", "count": 1}
            ]

    Returns:
        dict | None

        Example:
        {
            "id": "small_mana_potion",
            "name": "Small Mana Potion",
            "type": "Potion",
            "description": "A basic potion that restores a small amount of mana.",
            "craft_reason": "Moon Grass stabilizes magic, Clear Spring Water forms the base, and Mana Crystal Shard provides mana."
        }
    """

    print("========== LLM Potion Selector ==========\n")

    items = load_json(config.ITEMS_PATH)
    world_state = load_json(config.WORLD_STATE_PATH)
    story_memory = load_json(config.STORY_MEMORY_PATH)

    item_map = build_item_map(items)

    print("Selected Materials:")
    print(json.dumps(material_items, ensure_ascii=False, indent=2))

    material_errors = validate_materials(material_items, item_map)

    if material_errors:
        print("\n[MATERIALS INVALID]")
        for error in material_errors:
            print("-", error)
        return None

    print("\n[MATERIALS OK]")

    existing_potions = get_existing_potions(items)

    if not existing_potions:
        print("\n[FAILED] No potion exists in items.json.")
        return None

    prompt = build_potion_selection_prompt(
        material_items=material_items,
        items=items,
        world_state=world_state,
        story_memory=story_memory
    )

    print("\nCalling LLM...\n")

    raw_response = call_llm(prompt)

    print("=== Raw LLM Response ===")
    print(raw_response)

    generated_result = parse_llm_json(raw_response)

    potion_errors = validate_selected_potion(generated_result, item_map)

    if potion_errors:
        print("\n[SELECTED POTION INVALID]")
        for error in potion_errors:
            print("-", error)
        return None

    potion_id = generated_result["potion_id"].strip()
    potion_data = item_map[potion_id]

    final_potion = {
        "id": potion_data["id"],
        "name": potion_data["name"],
        "type": potion_data["type"],
        "description": potion_data["description"],
    }

    print("\n=== Final Selected Potion ===")
    print(json.dumps(final_potion, ensure_ascii=False, indent=2))

    return final_potion


# =========================
# Test Data
# =========================

if __name__ == "__main__":
    material_items = [
        {
            "item_id": "moon_grass",
            "count": 2
        },
        {
            "item_id": "clear_spring_water",
            "count": 1
        },
        {
            "item_id": "mana_crystal_shard",
            "count": 1
        }
    ]

    generate_potion(material_items)