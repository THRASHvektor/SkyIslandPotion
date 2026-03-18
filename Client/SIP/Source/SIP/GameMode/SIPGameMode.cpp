// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPGameMode.h"
#include "Character/SIPCharacter.h"
#include "UObject/ConstructorHelpers.h"

// 原生 GameMode 目前刻意保持轻量，默认 Pawn 选择仍然交给蓝图层处理。
ASIPGameMode::ASIPGameMode()
{
	// 写死默认Pawn为指定路径蓝图
	/*static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}*/
	
}
