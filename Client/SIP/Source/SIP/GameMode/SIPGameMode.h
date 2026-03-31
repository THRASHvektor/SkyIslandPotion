// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SIPGameMode.generated.h"

/**
 * 项目的最小化原生 GameMode。
 * 当前大部分玩家和关卡配置仍放在 Blueprint 中，
 * 这个类主要作为后续扩展游戏规则时的稳定原生基类。
 */
UCLASS(minimalapi)
class ASIPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 保留显式构造函数，后续新增原生默认值时不需要反复改蓝图。
	ASIPGameMode();

	//virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};



