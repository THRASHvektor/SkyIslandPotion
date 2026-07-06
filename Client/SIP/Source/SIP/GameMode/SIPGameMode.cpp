// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPGameMode.h"
#include "Character/SIPCharacter.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "SIPLogCategory.h"
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

// 关卡加载时把项目默认 KillZ 写入当前 World 的 WorldSettings，
// 这样所有使用本 GameMode 的关卡都会共享统一的坠落死亡阈值。
void ASIPGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (!bOverrideWorldSettingsKillZ)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			const float PreviousKillZ = WorldSettings->KillZ;
			WorldSettings->KillZ = DefaultKillZ;
			UE_LOG(LogSIP, Log, TEXT("SIPGameMode: KillZ overridden on map '%s' (%.1f -> %.1f)."),
				*MapName, PreviousKillZ, DefaultKillZ);
		}
	}
}

