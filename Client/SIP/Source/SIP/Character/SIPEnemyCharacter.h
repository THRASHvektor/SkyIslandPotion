// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * ASIPEnemyCharacter 是所有敌人角色的 C++ 基类
 * 继承自 ASIPCharacter，共享 GAS/血量/死亡流程
 *
 * 设计目的：
 * - 将敌人行为逻辑从 BP_Enemy 中上移到 C++，便于扩展和维护
 * - 提供 AI 控制和自动 GAS 初始化支持
 * - 敌人 ASC 挂在 Character 自身（不需要 PlayerState，单机/简单多人适用）
 *
 * 继承层次：
 * AActor → APawn → ACharacter → ASIPCharacter → ASIPEnemyCharacter
 */

#pragma once

#include "CoreMinimal.h"
#include "Character/SIPCharacter.h"
#include "SIPEnemyCharacter.generated.h"

/**
 * ASIPEnemyCharacter
 *
 * 所有 AI 敌人的基类。
 * BP_Enemy 应继承此类而非直接继承 ASIPCharacter。
 */
UCLASS()
class SIP_API ASIPEnemyCharacter : public ASIPCharacter
{
	GENERATED_BODY()

public:
	ASIPEnemyCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 死亡时的处理：停止 AI、播放死亡动画、延迟销毁
	virtual void OnDeath() override;

protected:
	virtual void BeginPlay() override;

	/** 死亡后多少秒销毁 Actor，<=0 表示不自动销毁 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Enemy")
	float DestroyDelay = 3.0f;

private:
	void DestroyEnemy();

	FTimerHandle DestroyTimerHandle;
};
