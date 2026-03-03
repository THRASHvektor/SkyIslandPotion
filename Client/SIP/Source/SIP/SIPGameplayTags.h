// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace SIPGameplayTags
{
	SIP_API	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	// Declare all of the custom native tags that Lyra will use
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	//SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Stick);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Walk);
	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Sprint);

	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_Jumping);

	SIP_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

};
