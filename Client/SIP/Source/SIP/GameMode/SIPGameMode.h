// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SIPGameMode.generated.h"

/**
 * Z 说明：
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

	/**
	 * Z 说明：项目级默认 KillZ（cm）。
	 * 关卡加载时会写入到该 World 的 WorldSettings->KillZ，
	 * 使得任何位置低于此 Z 的 Actor 触发 FellOutOfWorld。
	 * ASIPCharacter 已把 FellOutOfWorld 接入 GAS GE 死亡流程。
	 * 若关卡自身的 WorldSettings 已改过 KillZ 且不希望被覆盖，请把
	 * bOverrideWorldSettingsKillZ 关闭，或在具体关卡里再手动覆盖。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|World")
	float DefaultKillZ = -1000.0f;

	/**
	 * Z 说明：是否用 DefaultKillZ 覆盖当前关卡 WorldSettings->KillZ。
	 * 默认为 true，保证所有使用本 GameMode 的关卡都有一致的坠落死亡阈值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|World")
	bool bOverrideWorldSettingsKillZ = true;

protected:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};




