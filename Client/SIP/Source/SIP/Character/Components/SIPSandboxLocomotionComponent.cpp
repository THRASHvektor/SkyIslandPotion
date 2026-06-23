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
	if (bStartOnIceForDebug)
	{
		bIceSurfaceActive = true;
		SetLooseStateTag(SIPGameplayTags::State_Surface_Ice, true);
	}
	RefreshSurfaceMovementProfile();
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

void USIPSandboxLocomotionComponent::SetIceSurfaceActive(const bool bEnabled)
{
	if (bIceSurfaceActive == bEnabled)
	{
		return;
	}

	bIceSurfaceActive = bEnabled;
	SetLooseStateTag(SIPGameplayTags::State_Surface_Ice, bIceSurfaceActive);
	RefreshSurfaceMovementProfile();
	RefreshRotationMode();
	RefreshWalkSpeedOverride();
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

void USIPSandboxLocomotionComponent::HandleExternalSemanticStateChanged()
{
	RefreshRotationMode();
	RefreshWalkSpeedOverride();
	SyncOwnerAnimationState();
}

// 只有瞄准、横移和 Traversal 这几种状态需要使用控制器朝向。
/**
 * 判断当前是否应该使用控制器朝向。
 *
 * 不只是显式的瞄准 / 横移模式会走这里，
 * 那些需要更明确战斗转向感的语义攻击状态也会走这里，
 * 否则它们会被普通移动转向规则稀释掉。
 */
bool USIPSandboxLocomotionComponent::ShouldUseControllerDesiredRotation() const
{
	return bAimIntent || bStrafeIntent || bTraversalActive || IsFlaskRigCasting() || IsIceRuneDaggerCombatSteeringActive();
}

// 步行优先级高于冲刺，因为它代表更保守的速度请求。
ESIPSandboxDesiredGait USIPSandboxLocomotionComponent::GetDesiredGait() const
{
	if (bWalkIntent)
	{
		return ESIPSandboxDesiredGait::Walk;
	}

	if (IsFlaskRigCasting())
	{
		return ESIPSandboxDesiredGait::Walk;
	}

	if (bSprintIntent)
	{
		return ESIPSandboxDesiredGait::Sprint;
	}

	// 没有明确意图时，用实际速度决定步态。
	// 默认返回 Run 会导致 Chooser 在角色静止时仍选择跑动数据库，
	// Motion Matching 选不到匹配的 idle 动画 → "原地走路"。
	if (const ASIPCharacter* Character = OwnerCharacter.Get())
	{
		const float GroundSpeed2D = Character->GetVelocity().Size2D();
		if (GroundSpeed2D < 10.0f)
		{
			return ESIPSandboxDesiredGait::Walk;
		}
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
		if (!bHasCachedBaseMovementProfile)
		{
			CachedBaseMaxAcceleration = MovementComponent->MaxAcceleration;
			CachedBaseBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
			CachedBaseGroundFriction = MovementComponent->GroundFriction;
			CachedBaseBrakingFrictionFactor = MovementComponent->BrakingFrictionFactor;
			bHasCachedBaseMovementProfile = true;
		}
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

	// While a full-body attack montage is active, freeze external rotation so the
	// montage keeps ownership of facing and root presentation. This prevents
	// controller / movement driven spinning from dragging the montage off course.
	if (IsAttackMontageActive())
	{
		Character->bUseControllerRotationYaw = false;
		MovementComponent->bUseControllerDesiredRotation = false;
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->RotationRate = FRotator::ZeroRotator;
		return;
	}

	const bool bUseControllerDesiredRotation = ShouldUseControllerDesiredRotation();
	const bool bUseIceCombatRotationRate = IsIceRuneDaggerCombatSteeringActive();
	const float RotationRateMultiplier = (bIceSurfaceActive && !bUseIceCombatRotationRate) ? IceRotationRateMultiplier : 1.0f;
	const FRotator BaseRotationRate =
		bUseIceCombatRotationRate
			? IceCombatRotationRate
			: (bUseControllerDesiredRotation ? StrafeRotationRate : MovementRotationRate);
	Character->bUseControllerRotationYaw = false;
	MovementComponent->bUseControllerDesiredRotation = bUseControllerDesiredRotation;
	MovementComponent->bOrientRotationToMovement = !bUseControllerDesiredRotation;
	MovementComponent->RotationRate = BaseRotationRate * RotationRateMultiplier;
}

void USIPSandboxLocomotionComponent::RefreshSurfaceMovementProfile()
{
	UCharacterMovementComponent* MovementComponent = OwnerMovementComponent.Get();
	if (!MovementComponent)
	{
		CacheOwnerReferences();
		MovementComponent = OwnerMovementComponent.Get();
	}

	if (!MovementComponent || !bHasCachedBaseMovementProfile)
	{
		return;
	}

	if (bIceSurfaceActive)
	{
		MovementComponent->MaxAcceleration = CachedBaseMaxAcceleration * IceMaxAccelerationMultiplier;
		MovementComponent->BrakingDecelerationWalking = CachedBaseBrakingDecelerationWalking * IceBrakingDecelerationMultiplier;
		MovementComponent->GroundFriction = CachedBaseGroundFriction * IceGroundFrictionMultiplier;
		MovementComponent->BrakingFrictionFactor = CachedBaseBrakingFrictionFactor * IceBrakingFrictionFactorMultiplier;
		return;
	}

	MovementComponent->MaxAcceleration = CachedBaseMaxAcceleration;
	MovementComponent->BrakingDecelerationWalking = CachedBaseBrakingDecelerationWalking;
	MovementComponent->GroundFriction = CachedBaseGroundFriction;
	MovementComponent->BrakingFrictionFactor = CachedBaseBrakingFrictionFactor;
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

	const bool bShouldOverrideMoveSpeed = bWalkIntent || bSprintIntent || IsFlaskRigCasting() || bIceSurfaceActive;
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

		if (IsFlaskRigCasting())
		{
			DesiredMoveSpeed = FMath::Min(DesiredMoveSpeed, FlaskRigCastSpeedCap);
		}

		// 冰面物理参数让实际轨迹大幅偏离 Dense 动画的烘焙轨迹，
		// 速度越高分歧越大，PoseSearch 在高速区找不到稳定候选导致抽搐。
		// 限速到 trajectory 分歧可接受的范围。
		if (bIceSurfaceActive)
		{
			DesiredMoveSpeed = FMath::Min(DesiredMoveSpeed, IceSprintSpeedCap);
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

bool USIPSandboxLocomotionComponent::IsFlaskRigCasting() const
{
	const USIPAbilitySystemComponent* AbilitySystemComponent = OwnerAbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		return false;
	}

	const bool bHasFlaskRigTag =
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_WeaponModule_FlaskRig);

	const bool bHasActiveCastPhase =
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_Cast_PreCast) ||
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_Cast_Release);

	return bHasFlaskRigTag && bHasActiveCastPhase;
}

/**
 * 判断当前是否处于“应该按战斗逻辑转向”的 Ice Rune Dagger 语义状态。
 *
 * 这里故意直接读 ASC 上的 loose gameplay tags，
 * 因为桥接层已经把语义答案同步到了那里，
 * 而 locomotion 这里只需要一份廉价的布尔判断。
 */
bool USIPSandboxLocomotionComponent::IsIceRuneDaggerCombatSteeringActive() const
{
	const USIPAbilitySystemComponent* AbilitySystemComponent = OwnerAbilitySystemComponent.Get();
	if (!AbilitySystemComponent || !bIceSurfaceActive)
	{
		return false;
	}

	const bool bHasRuneDaggerTag =
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_WeaponModule_RuneDagger);
	if (!bHasRuneDaggerTag)
	{
		return false;
	}

	return
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_ActionFamily_SlideEntry) ||
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_ActionFamily_DriftSlash) ||
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_ActionFamily_DriftTurnSlash) ||
		AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_ActionFamily_DelayedRestart);
}

bool USIPSandboxLocomotionComponent::IsAttackMontageActive() const
{
	const USIPAbilitySystemComponent* AbilitySystemComponent = OwnerAbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		return false;
	}

	return AbilitySystemComponent->HasMatchingGameplayTag(SIPGameplayTags::State_Combat_Attacking);
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
