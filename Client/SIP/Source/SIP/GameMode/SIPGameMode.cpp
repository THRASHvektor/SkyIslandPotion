// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPGameMode.h"
#include "Character/SIPCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASIPGameMode::ASIPGameMode()
{
	// 写死默认Pawn为指定路径蓝图
	/*static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}*/
	
}