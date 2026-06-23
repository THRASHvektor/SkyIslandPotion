// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/SIPHeroAnimInstance.h"

#include "Character/Components/SIPHeroAnimationBridgeComponent.h"
#include "Character/SIPHeroCharacter.h"
#include "Data/SIPCombatSemanticProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

/**
 * SIPHeroAnimInstance.cpp 实现主角动画实例基础类。
 *
 * 主要功能：
 * 1. 在动画实例初始化时定位主角与桥接组件。
 * 2. 每帧从桥接组件同步移动、战斗和事件状态。
 * 3. 当桥接组件尚未就绪时，提供一份基础移动状态回退。
 */
void USIPHeroAnimInstance::NativeInitializeAnimation()
{
	// 在 BlueprintInitializeAnimation 触发前先准备好权威状态，
	// 避免子AnimBP 需要自己补写移动/战斗缓存。
	CacheAnimationReferences();
	SyncFromAnimationBridge();

	Super::NativeInitializeAnimation();
}

/**
 * 每帧刷新当前 AnimInstance 所需的表现层数据
 */
void USIPHeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	// 回收动画运行态所有权：先由父类同步最新缓存，
	// 再让 BlueprintUpdateAnimation 消费这些结果。
	if (!OwningHeroCharacter || !AnimationBridgeComponent)
	{
		CacheAnimationReferences();
	}

	SyncFromAnimationBridge();

	Super::NativeUpdateAnimation(DeltaSeconds);
}

/**
 * 线程安全的武器模块查询辅助函数。
 */
bool USIPHeroAnimInstance::HasWeaponModuleTag(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentWeaponModuleTag.MatchesTagExact(Tag);
}

/**
 * 线程安全的施法阶段查询辅助函数。
 */
bool USIPHeroAnimInstance::HasCastPhaseTag(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentCastPhaseTag.MatchesTagExact(Tag);
}

/**
 * 线程安全的语义动作家族查询辅助函数。
 */
bool USIPHeroAnimInstance::HasCombatActionFamilyTag(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentCombatActionFamilyTag.MatchesTagExact(Tag);
}

/**
 * 线程安全的语义身体状态查询辅助函数。
 */
bool USIPHeroAnimInstance::HasCombatBodyStateTag(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentCombatBodyStateTag.MatchesTagExact(Tag);
}

/**
 * Flask-rig 施法期间会让瞄准偏移和移动表现更像明确的施法姿态。
 */
bool USIPHeroAnimInstance::IsFlaskRigCasting() const
{
	return
		HasWeaponModuleTag(SIPGameplayTags::State_Combat_WeaponModule_FlaskRig) &&
		(
			HasCastPhaseTag(SIPGameplayTags::State_Combat_Cast_PreCast) ||
			HasCastPhaseTag(SIPGameplayTags::State_Combat_Cast_Release)
		);
}

/**
 * 当前 AnimBP 用于识别第一段冰面攻击起手状态的便捷判断。
 */
bool USIPHeroAnimInstance::IsIceRuneDaggerSlideAttack() const
{
	return
		HasWeaponModuleTag(SIPGameplayTags::State_Combat_WeaponModule_RuneDagger) &&
		HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_SlideEntry);
}

/**
 * 当前 AnimBP 用于识别冰面恢复状态的便捷判断。
 */
bool USIPHeroAnimInstance::IsIceRuneDaggerSlipRecovery() const
{
	return
		HasWeaponModuleTag(SIPGameplayTags::State_Combat_WeaponModule_RuneDagger) &&
		HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_SlipRecovery);
}

/**
 * 对那些攻击片段本身已经带有很强方向性身体语言的语义状态，
 * 关闭默认 combat aim offset，避免表现互相打架。
 */
bool USIPHeroAnimInstance::ShouldEnableCombatAimOffset() const
{
	const bool bHasIceRuneDaggerSemanticOverride =
		HasWeaponModuleTag(SIPGameplayTags::State_Combat_WeaponModule_RuneDagger) &&
		(
			HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_SlideEntry) ||
			HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DriftSlash) ||
			HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DriftTurn) ||
			HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_SlipRecovery) ||
			HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DelayedRestart)
		);

	return !(IsFlaskRigCasting() || bHasIceRuneDaggerSemanticOverride);
}

/**
 * 告诉下游动画逻辑：什么时候应该由战斗转向压过普通移动转向。
 */
bool USIPHeroAnimInstance::ShouldPreferCombatSteering() const
{
	return
		IsFlaskRigCasting() ||
		(
			HasWeaponModuleTag(SIPGameplayTags::State_Combat_WeaponModule_RuneDagger) &&
			(
				HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_SlideEntry) ||
				HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DriftSlash) ||
				HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DriftTurn) ||
				HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_SlipRecovery) ||
				HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DelayedRestart)
			)
		);
}

/**
 * 提供给当前 AnimBP 侧倾逻辑使用的小型缩放值，
 * 用来根据不同语义状态放大或减弱身体动势。
 */
float USIPHeroAnimInstance::GetCombatSemanticLeanScale() const
{
	if (IsFlaskRigCasting())
	{
		return 0.35f;
	}

	if (IsIceRuneDaggerSlideAttack())
	{
		return 1.35f;
	}

	if (HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DriftSlash))
	{
		return 1.30f;
	}

	if (HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DriftTurn))
	{
		return 1.45f;
	}

	if (IsIceRuneDaggerSlipRecovery())
	{
		return 1.15f;
	}

	if (HasCombatBodyStateTag(SIPGameplayTags::State_Combat_BodyState_DelayedRestart))
	{
		return 1.10f;
	}

	return 1.0f;
}

bool USIPHeroAnimInstance::ShouldSuppressMotionMatching() const
{
	return bShouldSuppressMotionMatching;
}

ESIPSemanticLocomotionMode USIPHeroAnimInstance::GetSemanticLocomotionMode() const
{
	return SemanticLocomotionMode;
}

FGameplayTag USIPHeroAnimInstance::GetDesiredPoseSearchDatabaseTag() const
{
	return DesiredPoseSearchDatabaseTag;
}

bool USIPHeroAnimInstance::HasSemanticPoseSearchDatabaseTagOverride() const
{
	return
		DesiredPoseSearchDatabaseTag.IsValid() &&
		!DesiredPoseSearchDatabaseTag.MatchesTagExact(SIPGameplayTags::PoseSearch_Database_Default);
}

EPoseSearchInterruptMode USIPHeroAnimInstance::GetMMInterruptMode() const
{
	// 抑制期间：不中断当前动画，MM 继续播放存量结果，不做新搜索
	if (bShouldSuppressMotionMatching)
	{
		return EPoseSearchInterruptMode::DoNotInterrupt;
	}
	// 抑制刚解除的第一帧：强制重新搜索，
	// 避免 MM 继续播放蒙太奇期间累积的陈旧动画帧。
	if (bMMForceInterruptPending)
	{
		bMMForceInterruptPending = false;
		return EPoseSearchInterruptMode::ForceInterrupt;
	}
	// 正常情况：Chooser 切库时中断旧动画重新搜索
	return EPoseSearchInterruptMode::InterruptOnDatabaseChange;
}

bool USIPHeroAnimInstance::ShouldReleaseOffsetRootBone() const
{
	return bShouldReleaseOffsetRootBone;
}

/**
 * 重新查找主角角色与动画桥接组件引用
 */
void USIPHeroAnimInstance::CacheAnimationReferences()
{
	ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(TryGetPawnOwner());
	if (!HeroCharacter)
	{
		HeroCharacter = Cast<ASIPHeroCharacter>(GetOwningActor());
	}

	OwningHeroCharacter = HeroCharacter;
	AnimationBridgeComponent = HeroCharacter ? HeroCharacter->GetHeroAnimationBridgeComponent() : nullptr;
}

/**
 * 优先从桥接组件同步数据，没有桥接组件时回退到角色基础移动状态
 */
void USIPHeroAnimInstance::SyncFromAnimationBridge()
{
	USIPHeroAnimationBridgeComponent* AnimationBridge = AnimationBridgeComponent.Get();
	ASIPHeroCharacter* HeroCharacter = OwningHeroCharacter.Get();

	// 只要桥接组件存在，它就是权威状态源，
	// 因为它已经把移动、战斗、时序和语义描述符合并好了。
	if (AnimationBridge)
	{
		bHasAnimationBridge = true;
		GroundSpeed = AnimationBridge->GetGroundSpeed();
		Velocity = AnimationBridge->GetVelocity();
		bIsMoving = AnimationBridge->IsMoving();
		bIsFalling = AnimationBridge->IsFalling();
		bIsJumping = AnimationBridge->IsJumping();
		bIsInCombatPresentation = AnimationBridge->IsInCombatPresentation();
		ActiveAnimationStateTags = AnimationBridge->GetAnimationStateTags();
		LastRequestedActionTag = AnimationBridge->GetLastRequestedActionTag();
		CurrentWeaponModuleTag = AnimationBridge->GetCurrentWeaponModuleTag();
		CurrentCastPhaseTag = AnimationBridge->GetCurrentCastPhaseTag();
		CurrentCombatActionFamilyTag = AnimationBridge->GetCurrentCombatActionFamilyTag();
		CurrentCombatBodyStateTag = AnimationBridge->GetCurrentCombatBodyStateTag();
		const FSIPCombatActionDescriptor CombatDescriptor = AnimationBridge->GetCurrentCombatActionDescriptor();
		CurrentCombatDesiredVariant = CombatDescriptor.DesiredVariant;
		bShouldUseMomentumWarpForCombatAction = CombatDescriptor.bUseMomentumWarp;
		CurrentCombatRecoveryBias = CombatDescriptor.RecoveryBias;
		CurrentCombatChainWindowPolicy = CombatDescriptor.ChainWindowPolicy;
		const bool bPreviousSuppressMM = bShouldSuppressMotionMatching;
		bShouldSuppressMotionMatching = AnimationBridge->ShouldSuppressMotionMatching();
		// 当 MM 抑制刚刚解除时（战斗→移动过渡），置一个一次性标记，
		// 让 GetMMInterruptMode 在下一次求值中返回 ForceInterrupt，
		// 确保 PoseSearch 立即重新搜索最优匹配而不是继续播放陈旧动画。
		if (bPreviousSuppressMM && !bShouldSuppressMotionMatching)
		{
			bMMForceInterruptPending = true;
		}
		// MM 抑制刚解除时，强制 OffsetRootBone 释放一帧，
		// 把战斗期间累积的根骨骼位移归零，避免角色瞬移回原点。
		// 其余帧保持 Interpolate/Accumulate，让 halflife 自然消化位差。
		bShouldReleaseOffsetRootBone = (bPreviousSuppressMM && !bShouldSuppressMotionMatching);
		SemanticLocomotionMode = AnimationBridge->GetSemanticLocomotionMode();
		if (const USIPCombatSemanticProfile* Profile = AnimationBridge->GetCombatSemanticProfile())
		{
			DesiredPoseSearchDatabaseTag = Profile->GetPoseSearchDatabaseTagForMode(SemanticLocomotionMode);
		}
		else
		{
			DesiredPoseSearchDatabaseTag = FGameplayTag();
		}

		if (!DesiredPoseSearchDatabaseTag.MatchesTagExact(LastLoggedDesiredPoseSearchDatabaseTag))
		{
			UE_LOG(
				LogSIP,
				Log,
				TEXT("[PoseSearchSemantic] DesiredDBTag=%s Mode=%d SuppressMM=%s Weapon=%s BodyState=%s"),
				DesiredPoseSearchDatabaseTag.IsValid() ? *DesiredPoseSearchDatabaseTag.ToString() : TEXT("None"),
				static_cast<int32>(SemanticLocomotionMode),
				bShouldSuppressMotionMatching ? TEXT("Y") : TEXT("N"),
				CurrentWeaponModuleTag.IsValid() ? *CurrentWeaponModuleTag.ToString() : TEXT("None"),
				CurrentCombatBodyStateTag.IsValid() ? *CurrentCombatBodyStateTag.ToString() : TEXT("None"));
			LastLoggedDesiredPoseSearchDatabaseTag = DesiredPoseSearchDatabaseTag;
		}

		UpdateCombatSemanticCache();
		return;
	}

	// 当桥接组件不可用时，退回角色基础移动状态，
	// 保证 AnimBP 至少还能读到一份干净的基础数据，而不是脏值。
	ResetAnimationState();

	if (!HeroCharacter)
	{
		return;
	}

	Velocity = HeroCharacter->GetVelocity();
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.0f).Size();
	bIsMoving = GroundSpeed > KINDA_SMALL_NUMBER;

	if (const UCharacterMovementComponent* MovementComponent = HeroCharacter->GetCharacterMovement())
	{
		bIsFalling = MovementComponent->IsFalling();
		bIsJumping = bIsFalling && Velocity.Z > 0.0f;
	}

	UpdateCombatSemanticCache();
}

/**
 * 刷新那些会被 AnimBP 在一帧内多次读取的派生缓存。
 */
void USIPHeroAnimInstance::UpdateCombatSemanticCache()
{
	bIsFlaskRigCasting = IsFlaskRigCasting();
	bIsIceRuneDaggerSlideAttack = IsIceRuneDaggerSlideAttack();
	bIsIceRuneDaggerSlipRecovery = IsIceRuneDaggerSlipRecovery();
	bShouldEnableCombatAimOffset = ShouldEnableCombatAimOffset();
	bShouldPreferCombatSteering = ShouldPreferCombatSteering() || bShouldUseMomentumWarpForCombatAction;
	CombatSemanticLeanScale = GetCombatSemanticLeanScale();
}

/**
 * 清空所有缓存值，避免在引用失效或初始化空档期泄漏旧战斗状态。
 */
void USIPHeroAnimInstance::ResetAnimationState()
{
	GroundSpeed = 0.0f;
	Velocity = FVector::ZeroVector;
	bIsMoving = false;
	bIsFalling = false;
	bIsJumping = false;
	bHasAnimationBridge = false;
	bIsInCombatPresentation = false;
	ActiveAnimationStateTags.Reset();
	LastRequestedActionTag = FGameplayTag();
	CurrentWeaponModuleTag = FGameplayTag();
	CurrentCastPhaseTag = FGameplayTag();
	CurrentCombatActionFamilyTag = FGameplayTag();
	CurrentCombatBodyStateTag = FGameplayTag();
	CurrentCombatDesiredVariant = NAME_None;
	bShouldUseMomentumWarpForCombatAction = false;
	CurrentCombatRecoveryBias = ESIPRecoveryBias::Fast;
	CurrentCombatChainWindowPolicy = ESIPChainWindowPolicy::Normal;
	bIsFlaskRigCasting = false;
	bIsIceRuneDaggerSlideAttack = false;
	bIsIceRuneDaggerSlipRecovery = false;
	bShouldEnableCombatAimOffset = true;
	bShouldPreferCombatSteering = false;
	CombatSemanticLeanScale = 1.0f;
	bShouldSuppressMotionMatching = false;
	bShouldReleaseOffsetRootBone = false;
	bMMForceInterruptPending = false;
	SemanticLocomotionMode = ESIPSemanticLocomotionMode::Default;
	DesiredPoseSearchDatabaseTag = FGameplayTag();
	LastLoggedDesiredPoseSearchDatabaseTag = FGameplayTag();
}
