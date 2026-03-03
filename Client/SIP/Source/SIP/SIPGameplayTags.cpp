// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPGameplayTags.h"

#include "Engine/EngineTypes.h"
#include "GameplayTagsManager.h"
#include "SIPLogCategory.h"

namespace SIPGameplayTags
{
	// Input Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "Move input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse", "Look (mouse) input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "Jump input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Sprint, "InputTag.Sprint", "Sprint input.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Walk, "InputTag.Walk", "Walk input.");

	// State Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_Jumping, "State.Movement.Jumping", "Jumping state.");

	// System Tags
	UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

	// Health Tags
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health_Changed, "Health.Changed", "Health value changed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Health_MaxChanged, "Health.MaxChanged", "Max health value changed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage, "Damage", "Damage taken.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "Death", "Character death.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(DeathStarted, "Death.Started", "Death has started.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(DeathStopped, "Death.Stopped", "Death has stopped (revive).");

	// Vitality Tags (for buffs/debuffs)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Healing, "Vitality.Healing", "Healing over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_Burning, "Vitality.Burning", "Burning damage over time.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Vitality_SpeedBoost, "Vitality.SpeedBoost", "Movement speed boost.");

	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString)
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

		if (!Tag.IsValid() && bMatchPartialString)
		{
			FGameplayTagContainer AllTags;
			Manager.RequestAllGameplayTags(AllTags, true);

			for (const FGameplayTag& TestTag : AllTags)
			{
				if (TestTag.ToString().Contains(TagString))
				{
					UE_LOG(LogSIP, Display, TEXT("Could not find exact match for tag [%s] but found partial match on tag [%s]."), *TagString, *TestTag.ToString());
					Tag = TestTag;
					break;
				}
			}
		}

		return Tag;
	}
}
