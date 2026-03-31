/**
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
 * 初始化技能属性
 * 
 * InstancingPolicy = InstancedPerExecution:
 * - 每次激活都创建新的实例
 * - 适合有内部状态的技能
 */
USIPGameplayAbility_Dash::USIPGameplayAbility_Dash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// InstancedPerExecution: 每次执行创建新实例
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	
	// 匹配 InputTag.Dash，用于 ASC 识别
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Dash")));
	
	// 防止技能重复激活
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Dash")));
}

/**
 * 检查技能是否可以被激活
 * 在技能激活前调用
 */
bool USIPGameplayAbility_Dash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

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
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	FVector DashDirection = CalculateDashDirection();
	
	if (PerformDash(DashDirection))
	{
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash completed successfully"));

		// 使用 GameplayEffect 实现冷却机制
		if (DashCooldownEffect)
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
			if (ASC)
			{
				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
				
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

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

/**
 * 技能结束时调用
 * 清理工作
 */
void USIPGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/**
 * 计算位移方向
 * 
 * 优先级：
 * 1. 如果有移动输入，按输入方向（相对于摄像机）
 * 2. 如果没有输入，按摄像机朝向
 */
FVector USIPGameplayAbility_Dash::CalculateDashDirection() const
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	FVector DashDirection = FVector::ForwardVector;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (PC)
	{
		FRotator ControlRotation = PC->GetControlRotation();
		ControlRotation.Pitch = 0.0f;
		ControlRotation.Roll = 0.0f;
		
		FVector ForwardDir = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
		FVector RightDir = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
		

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
		

		// if (FMath::Abs(MoveForward) > 0.1f || FMath::Abs(MoveRight) > 0.1f)
		// {
		// 	DashDirection = (ForwardDir * MoveForward + RightDir * MoveRight).GetSafeNormal();
		// }
		// else
		// {

		// 	DashDirection = ForwardDir;
		// }
	}

	return DashDirection.GetSafeNormal();
}

/**
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
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	FVector StartLocation = Character->GetActorLocation();
	FVector DashTarget = StartLocation + DashDirection * DashDistance;

	if (bCheckCollision)
	{
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(Character);

		FHitResult HitResult;
		
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

		if (bHit)
		{
			DashTarget = HitResult.Location - DashDirection * 50.0f;
			UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash blocked by collision, adjusting target"));
		}
	}

	FVector MidPoint = (StartLocation + DashTarget) * 0.5f;
	MidPoint.Z = FMath::Max(StartLocation.Z, DashTarget.Z) + 20.0f;

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

	// bSweep = true: 启用碰撞检测
	Character->SetActorLocation(DashTarget, true);
	
	// 防止移动组件停止角色
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->Velocity = DashDirection * 100.0f;
	}

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
