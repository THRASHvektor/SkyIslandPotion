// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace SIPGameplayTags
{
	SIP_API	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	// Input Tags
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Walk);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Sprint);

	// State Tags
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Jumping);

	// System Tags
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

	// Health Tags
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health_Changed);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Health_MaxChanged);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeathStarted);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(DeathStopped);

	// Vitality Tags (for buffs)
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_Healing);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_Burning);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Vitality_SpeedBoost);
};
