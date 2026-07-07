from quest_generator import generate_quest
from potion_generator import generate_potion

def test_generate_quest():
    """
    Test the generate_quest function.
    """
    quest = generate_quest()

    if quest:
        print("Quest Title:", quest["quest_title"])
        print("Quest Content:", quest["quest_content"])
    else:
        print("No quest generated.")
        
def test_generate_potion():
    potion = generate_potion([
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
    ])

    if potion:
        print("Potion ID:", potion["id"])
        print("Potion Name:", potion["name"])
        print("Potion Description:", potion["description"])

if __name__ == "__main__":
    # test_generate_quest()
    test_generate_potion()