// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "SIPLogCategory.h"

// 敌人角色当前的专属初始化很少，主要复用父类的通用角色逻辑。
ASIPEnemyCharacter::ASIPEnemyCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// PCG 运行时 SpawnActor 出的敌人不算 "Placed in World"，
	// 若沿用 APawn 默认的 EAutoPossessAI::PlacedInWorld，AIController 不会被自动创建，
	// 结果敌人 Controller 一直为 None，CharacterMovement 也不会跑物理 tick。
	// 改为 PlacedInWorldOrSpawned 后，无论手摆还是运行时 spawn 都会自动 possess。
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 保底：即便某帧 Controller 尚未就位（例如 spawn 到 possess 之间的过渡帧，
	// 或 AIController 因外部原因未成功创建），也让 CharacterMovement 继续跑物理，
	// 避免脚下地形被销毁时敌人卡在空中不下落。
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bRunPhysicsWithNoController = true;
	}
}

// 保留显式 BeginPlay，方便后续补充敌人专属启动逻辑。
void ASIPEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// 进入敌人死亡流程后，根据配置决定是否延迟做最终销毁清理。
void ASIPEnemyCharacter::OnDeath()
{

	// 关闭碰撞和移动，基类也会做一次，这里显式走父类链路保证敌人版本一致。
	Super::OnDeath();

	// 如果配置了延迟时间，则延后销毁 Actor。
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

// 延迟销毁定时器最终落到这里，完成敌人 Actor 的清理。
void ASIPEnemyCharacter::DestroyEnemy()
{
	UE_LOG(LogSIPCharacter, Log, TEXT("%s enemy destroyed."), *GetName());
	Destroy();
}
