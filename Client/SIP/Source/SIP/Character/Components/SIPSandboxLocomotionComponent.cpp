#include "Character/Components/SIPSandboxLocomotionComponent.h"

#include "Ability/SIPAbilitySystemComponent.h"
#include "Character/SIPCharacter.h"
#include "Character/SIPHeroCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SIPGameplayTags.h"

// 这是一个事件驱动的移动组件，只在意图变化时才做同步。
USIPSandboxLocomotionComponent::USIPSandboxLocomotionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

// 先准备缓存引用，确保移动和动画从同一份初始状态起步。
void USIPSandboxLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerReferences();
	RefreshRotationMode();
	RefreshWalkSpeedOverride();
	SyncOwnerAnimationState();
}

// 切换步行意图后，立刻刷新速度和动画状态。
void USIPSandboxLocomotionComponent::SetWalkIntent(const bool bEnabled)
{
	if (bWalkIntent == bEnabled)
	{
		return;
	}

	bWalkIntent = bEnabled;
	RefreshWalkSpeedOverride();
	SyncOwnerAnimationState();
}

// 切换冲刺意图后，立刻刷新速度和动画状态。
void USIPSandboxLocomotionComponent::SetSprintIntent(const bool bEnabled)
{
	if (bSprintIntent == bEnabled)
	{
		return;
	}

	bSprintIntent = bEnabled;
	RefreshWalkSpeedOverride();
	SyncOwnerAnimationState();
}

// 瞄准会更新 Loose Gameplay Tags，并强制切到控制器朝向模式。
void USIPSandboxLocomotionComponent::SetAimIntent(const bool bEnabled)
{
	if (bAimIntent == bEnabled)
	{
		return;
	}

	bAimIntent = bEnabled;
	SetLooseStateTag(SIPGameplayTags::State_Movement_Aiming, bAimIntent);
	RefreshRotationMode();
	SyncOwnerAnimationState();
}

// 从朝向和表现层角度看，横移与瞄准采用类似处理。
void USIPSandboxLocomotionComponent::SetStrafeIntent(const bool bEnabled)
{
	if (bStrafeIntent == bEnabled)
	{
		return;
	}

	bStrafeIntent = bEnabled;
	SetLooseStateTag(SIPGameplayTags::State_Movement_Strafing, bStrafeIntent);
	RefreshRotationMode();
	SyncOwnerAnimationState();
}

// Traversal 期间暂时把移动控制权交给 Traversal/Root Motion 逻辑。
void USIPSandboxLocomotionComponent::SetTraversalActive(const bool bEnabled)
{
	if (bTraversalActive == bEnabled)
	{
		return;
	}

	bTraversalActive = bEnabled;
	SetLooseStateTag(SIPGameplayTags::State_Movement_Traversing, bTraversalActive);
	RefreshRotationMode();
	SyncOwnerAnimationState();
}

// 下蹲动作仍由 Character 自己处理，但这里负责保持表现标签同步。
void USIPSandboxLocomotionComponent::HandleCrouchPressed()
{
	ASIPCharacter* Character = OwnerCharacter.Get();
	if (!Character || Character->bIsCrouched)
	{
		return;
	}

	Character->Crouch();
	SetLooseStateTag(SIPGameplayTags::State_Movement_Crouching, Character->bIsCrouched);
	SyncOwnerAnimationState();
}

// 取消下蹲后，把最终姿态同步回 ASC 和动画快照。
void USIPSandboxLocomotionComponent::HandleCrouchReleased()
{
	ASIPCharacter* Character = OwnerCharacter.Get();
	if (!Character || !Character->bIsCrouched)
	{
		return;
	}

	Character->UnCrouch();
	SetLooseStateTag(SIPGameplayTags::State_Movement_Crouching, Character->bIsCrouched);
	SyncOwnerAnimationState();
}

// 只有瞄准、横移和 Traversal 这几种状态需要使用控制器朝向。
bool USIPSandboxLocomotionComponent::ShouldUseControllerDesiredRotation() const
{
	return bAimIntent || bStrafeIntent || bTraversalActive;
}

// 步行优先级高于冲刺，因为它代表更保守的速度请求。
ESIPSandboxDesiredGait USIPSandboxLocomotionComponent::GetDesiredGait() const
{
	if (bWalkIntent)
	{
		return ESIPSandboxDesiredGait::Walk;
	}

	if (bSprintIntent)
	{
		return ESIPSandboxDesiredGait::Sprint;
	}

	return ESIPSandboxDesiredGait::Run;
}

// 把语义化的步态请求换算成具体的移动速度目标。
float USIPSandboxLocomotionComponent::GetDesiredMaxWalkSpeed() const
{
	switch (GetDesiredGait())
	{
	case ESIPSandboxDesiredGait::Walk:
		return FMath::Min(CachedBaseMoveSpeed, WalkSpeedCap);
	case ESIPSandboxDesiredGait::Sprint:
		return FMath::Max(CachedBaseMoveSpeed, SprintSpeedFloor);
	default:
		return CachedBaseMoveSpeed;
	}
}

// 一次性缓存常用的拥有者侧引用，降低后续更新成本。
void USIPSandboxLocomotionComponent::CacheOwnerReferences()
{
	OwnerCharacter = Cast<ASIPCharacter>(GetOwner());
	OwnerAbilitySystemComponent = OwnerCharacter.IsValid() ? OwnerCharacter->GetSIPAbilitySystemComponent() : nullptr;
	OwnerMovementComponent = OwnerCharacter.IsValid() ? OwnerCharacter->GetCharacterMovement() : nullptr;

	if (const UCharacterMovementComponent* MovementComponent = OwnerMovementComponent.Get())
	{
		CachedBaseMoveSpeed = MovementComponent->MaxWalkSpeed;
	}
}

// 重新配置 CharacterMovement，让朝向规则匹配当前移动模式。
void USIPSandboxLocomotionComponent::RefreshRotationMode()
{
	ASIPCharacter* Character = OwnerCharacter.Get();
	UCharacterMovementComponent* MovementComponent = OwnerMovementComponent.Get();
	if (!Character || !MovementComponent)
	{
		CacheOwnerReferences();
		Character = OwnerCharacter.Get();
		MovementComponent = OwnerMovementComponent.Get();
	}

	if (!Character || !MovementComponent)
	{
		return;
	}

	const bool bUseControllerDesiredRotation = ShouldUseControllerDesiredRotation();
	Character->bUseControllerRotationYaw = false;
	MovementComponent->bUseControllerDesiredRotation = bUseControllerDesiredRotation;
	MovementComponent->bOrientRotationToMovement = !bUseControllerDesiredRotation;
	MovementComponent->RotationRate = bUseControllerDesiredRotation ? StrafeRotationRate : MovementRotationRate;
}

// 把请求到的步态写成 MaxWalkSpeed 覆盖值，同时保留原始速度方便恢复。
void USIPSandboxLocomotionComponent::RefreshWalkSpeedOverride()
{
	UCharacterMovementComponent* MovementComponent = OwnerMovementComponent.Get();
	if (!MovementComponent)
	{
		CacheOwnerReferences();
		MovementComponent = OwnerMovementComponent.Get();
	}

	if (!MovementComponent || !bDriveWalkSpeedOverride)
	{
		return;
	}

	const bool bShouldOverrideMoveSpeed = bWalkIntent || bSprintIntent;
	if (bShouldOverrideMoveSpeed)
	{
		if (!bWalkSpeedOverridden)
		{
			CachedBaseMoveSpeed = MovementComponent->MaxWalkSpeed;
		}

		float DesiredMoveSpeed = CachedBaseMoveSpeed;
		if (bWalkIntent)
		{
			DesiredMoveSpeed = FMath::Min(CachedBaseMoveSpeed, WalkSpeedCap);
		}
		else if (bSprintIntent)
		{
			DesiredMoveSpeed = FMath::Max(CachedBaseMoveSpeed, SprintSpeedFloor);
		}

		MovementComponent->MaxWalkSpeed = DesiredMoveSpeed;
		bWalkSpeedOverridden = true;
		return;
	}

	if (bWalkSpeedOverridden)
	{
		MovementComponent->MaxWalkSpeed = CachedBaseMoveSpeed;
		bWalkSpeedOverridden = false;
	}
}

// 用于把移动表现标签镜像同步到拥有者 ASC 的辅助函数。
void USIPSandboxLocomotionComponent::SetLooseStateTag(const FGameplayTag& Tag, const bool bEnabled)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (USIPAbilitySystemComponent* AbilitySystemComponent = OwnerAbilitySystemComponent.Get())
	{
		if (bEnabled)
		{
			AbilitySystemComponent->AddLooseGameplayTag(Tag);
		}
		else
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
		}
	}
}

// 保持主角线程安全动画快照与最新移动状态一致。
void USIPSandboxLocomotionComponent::SyncOwnerAnimationState()
{
	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(GetOwner()))
	{
		HeroCharacter->RefreshSandboxThreadSafeState();
	}
}
