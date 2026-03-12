/**
 * Z 说明：
 * SIPGameplayAbility_Dash.cpp 实现了闪现技能的具体逻辑
 * 
 * 技能执行流程：
 * 1. CanActivateAbility: 检查前置条件
 * 2. ActivateAbility: 激活技能
 * 3. CalculateDashDirection: 计算位移方向
 * 4. PerformDash: 执行位移
 * 5. EndAbility: 结束技能
 */

#include "SIPGameplayAbility_Dash.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "SIPLogCategory.h"
#include "SIPGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "GameplayEffect.h"
#include "NiagaraFunctionLibrary.h"

/**
 * Z 说明：构造函数
 * 初始化技能属性
 * 
 * InstancingPolicy = InstancedPerExecution:
 * - 每次激活都创建新的实例
 * - 适合有内部状态的技能
 */
USIPGameplayAbility_Dash::USIPGameplayAbility_Dash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Z 说明：实例化策略
	// InstancedPerExecution: 每次执行创建新实例
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	
	// Z 说明：添加技能标签
	// 匹配 InputTag.Dash，用于 ASC 识别
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Dash")));
	
	// Z 说明：激活时屏蔽的标签
	// 防止技能重复激活
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Dash")));
}

/**
 * Z 说明：CanActivateAbility
 * 检查技能是否可以被激活
 * 在技能激活前调用
 */
bool USIPGameplayAbility_Dash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	// Z 说明：先检查基类的条件
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// Z 说明：检查角色有效性
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	// Z 说明：检查是否在空中
	// 闪现不允许在空中使用
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (MovementComp && MovementComp->IsFalling())
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("DashAbility: Cannot dash while falling"));
		return false;
	}

	return true;
}

/**
 * Z 说明：ActivateAbility
 * 激活技能时的主要逻辑
 * 
 * 流程：
 * 1. 计算位移方向
 * 2. 执行位移
 * 3. 应用冷却效果
 * 4. 结束技能
 */
void USIPGameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Z 说明：调用基类激活
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Z 说明：第一步 - 计算位移方向
	FVector DashDirection = CalculateDashDirection();
	
	// Z 说明：第二步 - 执行位移
	if (PerformDash(DashDirection))
	{
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash completed successfully"));

		// Z 说明：第三步 - 应用冷却效果
		// 使用 GameplayEffect 实现冷却机制
		if (DashCooldownEffect)
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
			if (ASC)
			{
				// Z 说明：创建效果上下文
				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
				
				// Z 说明：应用冷却效果到自身
				ASC->ApplyGameplayEffectToSelf(
					DashCooldownEffect->GetDefaultObject<UGameplayEffect>(),
					1.0f,
					EffectContext
				);
				
				UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash cooldown applied"));
			}
		}
	}
	else
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("Dash failed"));
	}

	// Z 说明：第四步 - 结束技能
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

/**
 * Z 说明：EndAbility
 * 技能结束时调用
 * 清理工作
 */
void USIPGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/**
 * Z 说明：CalculateDashDirection
 * 计算位移方向
 * 
 * 优先级：
 * 1. 如果有移动输入，按输入方向（相对于摄像机）
 * 2. 如果没有输入，按摄像机朝向
 */
FVector USIPGameplayAbility_Dash::CalculateDashDirection() const
{
	// Z 说明：获取角色
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		// Z 说明：默认向前
		return FVector::ForwardVector;
	}

	FVector DashDirection = FVector::ForwardVector;

	// Z 说明：检查是否有玩家控制器
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (PC)
	{
		// Z 说明：获取控制器旋转（忽略俯仰和翻滚）
		FRotator ControlRotation = PC->GetControlRotation();
		ControlRotation.Pitch = 0.0f;
		ControlRotation.Roll = 0.0f;
		
		// Z 说明：获取前方向量（注意：UE 的 Forward 是 Y 轴）
		FVector ForwardDir = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
		FVector RightDir = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
		
		// // Z 说明：获取移动输入值
		// // 注意：这里使用旧版输入系统获取输入值
		// float MoveForward = Character->GetInputAxisValue(TEXT("MoveForward"));
		// float MoveRight = Character->GetInputAxisValue(TEXT("MoveRight"));

		// 更新输入为增强输入系统
		DashDirection = Character->GetLastMovementInputVector();
		if (DashDirection.IsNearlyZero())
		{
			// 如果玩家没按方向键，默认向角色前方闪现
			DashDirection = Character->GetActorForwardVector();
		}
		
		// // Z 说明：有移动输入时，按输入方向
		// if (FMath::Abs(MoveForward) > 0.1f || FMath::Abs(MoveRight) > 0.1f)
		// {
		// 	DashDirection = (ForwardDir * MoveForward + RightDir * MoveRight).GetSafeNormal();
		// }
		// else
		// {
		// 	// Z 说明：无输入时，按摄像机朝向
		// 	DashDirection = ForwardDir;
		// }
	}

	return DashDirection.GetSafeNormal();
}

/**
 * Z 说明：PerformDash
 * 执行位移
 * 
 * 步骤：
 * 1. 计算起点和终点
 * 2. 碰撞检测
 * 3. 播放拖尾特效
 * 4. 移动角色
 * 5. 播放落地特效
 */
bool USIPGameplayAbility_Dash::PerformDash(const FVector& DashDirection)
{
	// Z 说明：获取角色
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	// Z 说明：计算位移终点
	FVector StartLocation = Character->GetActorLocation();
	FVector DashTarget = StartLocation + DashDirection * DashDistance;

	// Z 说明：碰撞检测（可选）
	if (bCheckCollision)
	{
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(Character);

		FHitResult HitResult;
		
		// Z 说明：直线射线检测
		bool bHit = UKismetSystemLibrary::LineTraceSingle(
			Character,
			StartLocation,
			DashTarget,
			UEngineTypes::ConvertToTraceType(ECC_WorldStatic),
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			HitResult,
			true
		);

		// Z 说明：如果检测到碰撞，调整终点位置
		if (bHit)
		{
			// Z 说明：终点设为碰撞点向前50单位，避免贴在墙上
			DashTarget = HitResult.Location - DashDirection * 50.0f;
			UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash blocked by collision, adjusting target"));
		}
	}

	// Z 说明：计算特效中间位置
	FVector MidPoint = (StartLocation + DashTarget) * 0.5f;
	MidPoint.Z = FMath::Max(StartLocation.Z, DashTarget.Z) + 20.0f;

	// Z 说明：播放拖尾特效
	if (DashTrailEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			DashTrailEffect,
			MidPoint,
			Character->GetActorRotation()
		);
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash trail effect spawned"));
	}

	// Z 说明：执行位移
	// bSweep = true: 启用碰撞检测
	Character->SetActorLocation(DashTarget, true);
	
	// Z 说明：保持一定速度
	// 防止移动组件停止角色
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->Velocity = DashDirection * 100.0f;
	}

	// Z 说明：播放落地特效
	if (DashLandedEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			DashLandedEffect,
			DashTarget,
			Character->GetActorRotation()
		);
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash landed effect spawned"));
	}

	return true;
}
