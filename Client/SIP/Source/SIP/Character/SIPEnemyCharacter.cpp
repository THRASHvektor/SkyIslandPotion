// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPEnemyCharacter.h"
#include "TimerManager.h"
#include "SIPLogCategory.h"

ASIPEnemyCharacter::ASIPEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ASIPEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASIPEnemyCharacter::OnDeath()
{

	// 关闭碰撞和移动（基类 OnDeathStarted 也会做，这里确保 AI 版本也执行）
	Super::OnDeath();

	// 延迟销毁 Actor
	if (DestroyDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			DestroyTimerHandle,
			this,
			&ASIPEnemyCharacter::DestroyEnemy,
			DestroyDelay,
			false
		);
	}
	else
	{
		DestroyEnemy();
	}
}

void ASIPEnemyCharacter::DestroyEnemy()
{
	UE_LOG(LogSIPCharacter, Log, TEXT("%s enemy destroyed."), *GetName());
	Destroy();
}
