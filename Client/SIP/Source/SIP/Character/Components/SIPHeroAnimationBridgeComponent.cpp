// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Components/SIPHeroAnimationBridgeComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Character/SIPCharacter.h"
#include "Character/SIPHeroCharacter.h"
#include "Character/Components/SIPSandboxLocomotionComponent.h"
#include "Combat/SIPCombatSemanticResolver.h"
#include "Data/SIPCombatSemanticProfile.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"
#include "TimerManager.h"

namespace
{
	void EmitCombatBodyStateDebug(UObject* ContextObject, const FString& Message, const bool bOnScreen)
	{
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("%s"), *Message);

		if (!bOnScreen || !GEngine)
		{
			return;
		}

		const uint64 MessageKey = uint64(uintptr_t(ContextObject)) + 2001ull;
		GEngine->AddOnScreenDebugMessage(MessageKey, 1.5f, FColor::Yellow, Message);
	}
}

/**
 * SIPHeroAnimationBridgeComponent.cpp 实现玩法逻辑到动画表现层的桥接。
 *
 * 主要功能：
 * 1. 同步角色移动状态给动画蓝图使用。
 * 2. 维护战斗表现相关的 Gameplay Tags。
 * 3. 统一派发攻击/投掷等动画事件到 GAS。
 * 4. 在没有完整动画 Notify 的情况下，用定时器模拟事件时序。
 */
USIPHeroAnimationBridgeComponent::USIPHeroAnimationBridgeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USIPHeroAnimationBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerReferences();
}

/**
 * 每帧同步移动表现层最常用的几个状态：
 * 1. 世界速度
 * 2. 地面速度
 * 3. 是否下落
 * 4. 是否处于跳跃上升阶段
 */
void USIPHeroAnimationBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ASIPCharacter* Character = OwnerCharacter.Get();
	if (!Character)
	{
		CacheOwnerReferences();
		Character = OwnerCharacter.Get();
	}

	if (!Character)
	{
		CachedVelocity = FVector::ZeroVector;
		GroundSpeed = 0.0f;
		bIsFalling = false;
		bIsJumping = false;
		return;
	}

	CachedVelocity = Character->GetVelocity();
	GroundSpeed = FVector(CachedVelocity.X, CachedVelocity.Y, 0.0f).Size();

	if (const UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
	{
		bIsFalling = MovementComponent->IsFalling();
		bIsJumping = bIsFalling && CachedVelocity.Z > 0.0f;
	}
	else
	{
		bIsFalling = false;
		bIsJumping = false;
	}

	// 累计 Falling 时长，用于在 ShouldSuppressMotionMatching 中过滤冰面微型落差。
	if (bIsFalling)
	{
		FallingDuration += DeltaTime;
	}
	else
	{
		FallingDuration = 0.0f;
	}

	UpdateCombatActionDescriptor();
	ClearCombatStateIfIdle();
}

/**
 * 进入一次攻击表现流程，并安排命中窗口开始/结束事件
 */
bool USIPHeroAnimationBridgeComponent::RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay)
{
	return RequestAttackAnimation(HitWindowStartDelay, HitWindowEndDelay, FGameplayTag(), SIPGameplayTags::State_Combat_Cast_PreCast);
}

bool USIPHeroAnimationBridgeComponent::RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay, const FGameplayTag WeaponModuleTag, const FGameplayTag InitialCastPhaseTag)
{
	CancelAttackAnimation();

	LastRequestedActionTag = SIPGameplayTags::Event_Animation_Attack_Request;
	ResetAnimationEventDispatchState(LastRequestedActionTag);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	AddAnimationStateTag(SIPGameplayTags::State_Combat);
	AddAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);
	SetWeaponModuleTag(WeaponModuleTag);
	bQueuedBufferedFollowUpAfterAttack = false;
	bGoldenPathWasActiveDuringAttack = false;
	ClearPostAttackMMSuppressionGrace();

	if (const ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		const FSIPCombatFeatureVector InitialVector = SIPCombatSemantic::BuildHeroCombatFeatureVector(
			HeroCharacter,
			WeaponModuleTag,
			InitialCastPhaseTag,
			GetIceRuneDaggerSignedTurnAngleDegrees(HeroCharacter),
			CombatSemanticProfile);
		const FSIPCombatResolutionContext InitialContext;
		const FSIPCombatActionDescriptor InitialDescriptor = SIPCombatSemantic::ResolveIceRuneDaggerGoldenPath(InitialVector, InitialContext);
		bCombatBodyStateLocked = InitialDescriptor.HasResolvedAction();
		bLockedIceRuneDaggerSemantic = InitialDescriptor.HasResolvedAction();
	}
	else
	{
		ResetCombatBodyStateLock();
	}

	SetCastPhaseTag(InitialCastPhaseTag);

	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start, HitWindowStartDelay);
	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_End, HitWindowEndDelay);
	DispatchAnimationEvent(LastRequestedActionTag);
	return true;
}

/**
 * 使用 GA 已预解析的语义描述符进入攻击表现状态，
 * 跳过桥接层自身的初始解析，消除两层解析之间的时序差异。
 */
bool USIPHeroAnimationBridgeComponent::RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay, const FGameplayTag WeaponModuleTag, const FGameplayTag InitialCastPhaseTag, const FSIPCombatActionDescriptor& PreResolvedDescriptor)
{
	CancelAttackAnimation();

	LastRequestedActionTag = SIPGameplayTags::Event_Animation_Attack_Request;
	ResetAnimationEventDispatchState(LastRequestedActionTag);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	AddAnimationStateTag(SIPGameplayTags::State_Combat);
	AddAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);
	SetWeaponModuleTag(WeaponModuleTag);
	bQueuedBufferedFollowUpAfterAttack = false;
	bGoldenPathWasActiveDuringAttack = false;
	ClearPostAttackMMSuppressionGrace();

	const bool bHasPreResolvedDescriptor = PreResolvedDescriptor.HasResolvedAction();
	bCombatBodyStateLocked = bHasPreResolvedDescriptor;
	bLockedIceRuneDaggerSemantic = bHasPreResolvedDescriptor;

	SetCastPhaseTag(InitialCastPhaseTag);

	// 直接应用 GA 的预解析结果，覆盖 SetCastPhaseTag 触发的中间解析
	if (bHasPreResolvedDescriptor)
	{
		SetCombatActionDescriptor(PreResolvedDescriptor);
	}

	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start, HitWindowStartDelay);
	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_End, HitWindowEndDelay);
	DispatchAnimationEvent(LastRequestedActionTag);
	return true;
}

/**
 * 进入一次投掷表现流程，并安排释放事件
 */
bool USIPHeroAnimationBridgeComponent::RequestThrowAnimation(float ReleaseDelay)
{
	return RequestThrowAnimation(ReleaseDelay, FGameplayTag(), SIPGameplayTags::State_Combat_Cast_PreCast);
}

bool USIPHeroAnimationBridgeComponent::RequestThrowAnimation(float ReleaseDelay, const FGameplayTag WeaponModuleTag, const FGameplayTag InitialCastPhaseTag)
{
	CancelThrowAnimation();

	LastRequestedActionTag = SIPGameplayTags::Event_Animation_Throw_Request;
	ResetAnimationEventDispatchState(LastRequestedActionTag);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Throw_Release);
	AddAnimationStateTag(SIPGameplayTags::State_Combat);
	AddAnimationStateTag(SIPGameplayTags::State_Combat_Throwing);
	SetWeaponModuleTag(WeaponModuleTag);
	SetCastPhaseTag(InitialCastPhaseTag);

	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Throw_Release, ReleaseDelay);
	DispatchAnimationEvent(LastRequestedActionTag);
	return true;
}

void USIPHeroAnimationBridgeComponent::CancelAttackAnimation()
{
	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_Request);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);
	bQueuedBufferedFollowUpAfterAttack = false;
	bGoldenPathWasActiveDuringAttack = false;
	ClearPostAttackMMSuppressionGrace();
	SetCastPhaseTag(FGameplayTag());
	ResetCombatBodyStateLock();
	SetCombatActionDescriptor(FSIPCombatActionDescriptor());
	ClearCombatStateIfIdle();

	// 安全网：确保 RotationRate 在攻击取消后恢复。
	// SetCastPhaseTag(empty) 通常会触发 RefreshRotationMode，
	// 但如果 CastPhaseTag 已经是空的则会提前返回，漏掉旋转恢复。
	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		if (USIPSandboxLocomotionComponent* LocomotionComponent = HeroCharacter->GetSandboxLocomotionComponent())
		{
			LocomotionComponent->HandleExternalSemanticStateChanged();
		}
	}
}

void USIPHeroAnimationBridgeComponent::FinishAttackAnimation(const bool bQueuedBufferedFollowUp)
{
	bGoldenPathWasActiveDuringAttack = false;

	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_Request);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);
	bQueuedBufferedFollowUpAfterAttack = bQueuedBufferedFollowUp;
	SetCastPhaseTag(FGameplayTag());
	ResetCombatBodyStateLock();
	UpdateCombatActionDescriptor();

	// 先清理战斗状态：有语义尾巴则启动 TTL，无尾巴则移除 State_Combat。
	// 必须在 grace 之前执行——否则无尾巴分支的 ClearPostAttackMMSuppressionGrace()
	// 会在同一帧内把刚设好的 grace 立刻清掉。
	ClearCombatStateIfIdle();

	// 连招衔接时保持短暂 MM 抑制，避免 State_Combat_Attacking 被移除后、
	// 下一招设置前的间隙让 MM 闪烁搜索一帧产生"小碎步"。
	// 非连招结束时 **不设 grace**——让 MM 在 DefaultSlot BlendOut 期间立即重新搜索，
	// 配合 bAlwaysUpdateSourcePose=true，DefaultSlot 可以平滑地从蒙太奇混合到
	// MM 的当前最优匹配，而不是等 BlendOut 结束后才放行产生硬跳。
	if (bQueuedBufferedFollowUp)
	{
		bPostAttackMMSuppressionGraceActive = true;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PostAttackMMSuppressionGraceHandle);
			FTimerDelegate Delegate;
			Delegate.BindUObject(this, &USIPHeroAnimationBridgeComponent::OnPostAttackMMSuppressionGraceExpired);
			// 使用配置值（默认 2.0s），覆盖 DynamicBlendOut 上限 0.65s，
			// 避免 BlendOut 期间 MM 恢复造成单帧骨骼跳动。
			World->GetTimerManager().SetTimer(PostAttackMMSuppressionGraceHandle, Delegate, PostAttackMMSuppressionGraceSeconds, false);
		}
	}

	// 安全网：确保 RotationRate 在攻击结束后恢复。
	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		if (USIPSandboxLocomotionComponent* LocomotionComponent = HeroCharacter->GetSandboxLocomotionComponent())
		{
			LocomotionComponent->HandleExternalSemanticStateChanged();
		}
	}
}

void USIPHeroAnimationBridgeComponent::CancelThrowAnimation()
{
	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Throw_Release);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Throw_Request);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Throw_Release);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Throwing);
	bQueuedBufferedFollowUpAfterAttack = false;
	SetCastPhaseTag(FGameplayTag());
	ResetCombatBodyStateLock();
	SetCombatActionDescriptor(FSIPCombatActionDescriptor());
	ClearCombatStateIfIdle();
}

void USIPHeroAnimationBridgeComponent::NotifyAnimationEvent(FGameplayTag EventTag)
{
	DispatchAnimationEvent(EventTag);
}

bool USIPHeroAnimationBridgeComponent::HasAnimationStateTag(FGameplayTag Tag) const
{
	return Tag.IsValid() && ActiveAnimationStateTags.HasTagExact(Tag);
}

/**
 * 供下游动画代码查询当前是否处于全局战斗表现状态。
 */
bool USIPHeroAnimationBridgeComponent::IsInCombatPresentation() const
{
	return HasAnimationStateTag(SIPGameplayTags::State_Combat);
}

/**
 * 当前武器模块标签查询辅助函数。
 */
bool USIPHeroAnimationBridgeComponent::HasCurrentWeaponModuleTag(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentWeaponModuleTag.MatchesTagExact(Tag);
}

/**
 * 当前施法阶段标签查询辅助函数。
 */
bool USIPHeroAnimationBridgeComponent::IsInCastPhase(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentCastPhaseTag.MatchesTagExact(Tag);
}

/**
 * 当前动作家族标签查询辅助函数，供 AnimBP 和玩法代码共用。
 */
bool USIPHeroAnimationBridgeComponent::HasCurrentCombatActionFamilyTag(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentCombatActionFamilyTag.MatchesTagExact(Tag);
}

/**
 * 当前身体状态标签查询辅助函数，供 AnimBP 和玩法代码共用。
 */
bool USIPHeroAnimationBridgeComponent::HasCurrentCombatBodyStateTag(const FGameplayTag Tag) const
{
	return Tag.IsValid() && CurrentCombatBodyStateTag.MatchesTagExact(Tag);
}

/**
 * 计算 Ice Rune Dagger 解析器使用的“面朝 vs 速度方向”有符号测量值。
 */
float USIPHeroAnimationBridgeComponent::GetIceRuneDaggerSignedTurnAngleDegrees(const ASIPCharacter* Character) const
{
	// Use a meaningful speed threshold to avoid jittery angle output at near-zero velocity
	// on ice (friction is very low → tiny residual velocity produces random facing angles).
	static constexpr float MinSpeedSquaredForAngle = 10.0f * 10.0f; // 10 cm/s
	if (!Character || CachedVelocity.SizeSquared2D() <= MinSpeedSquaredForAngle)
	{
		return 0.0f;
	}

	return SIPCombatSemantic::GetSignedTurnAngleDegrees(Character->GetActorForwardVector(), CachedVelocity);
}

/**
 * 切换当前武器模块标签，并触发下游语义刷新。
 */
void USIPHeroAnimationBridgeComponent::SetWeaponModuleTag(const FGameplayTag& Tag)
{
	if (CurrentWeaponModuleTag == Tag)
	{
		return;
	}

	if (CurrentWeaponModuleTag.IsValid())
	{
		RemoveAnimationStateTag(CurrentWeaponModuleTag);
	}

	CurrentWeaponModuleTag = Tag;

	if (CurrentWeaponModuleTag.IsValid())
	{
		AddAnimationStateTag(CurrentWeaponModuleTag);
	}

	UpdateCombatActionDescriptor();

	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		if (USIPSandboxLocomotionComponent* LocomotionComponent = HeroCharacter->GetSandboxLocomotionComponent())
		{
			LocomotionComponent->HandleExternalSemanticStateChanged();
		}
	}
}

/**
 * 切换当前施法阶段标签，并触发下游语义刷新。
 */
void USIPHeroAnimationBridgeComponent::SetCastPhaseTag(const FGameplayTag& Tag)
{
	if (CurrentCastPhaseTag == Tag)
	{
		return;
	}

	if (CurrentCastPhaseTag.IsValid())
	{
		RemoveAnimationStateTag(CurrentCastPhaseTag);
	}

	CurrentCastPhaseTag = Tag;

	if (CurrentCastPhaseTag.IsValid())
	{
		AddAnimationStateTag(CurrentCastPhaseTag);
	}

	UpdateCombatActionDescriptor();

	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		if (USIPSandboxLocomotionComponent* LocomotionComponent = HeroCharacter->GetSandboxLocomotionComponent())
		{
			LocomotionComponent->HandleExternalSemanticStateChanged();
		}
	}
}

/**
 * 用主角当前状态和桥接层持有的攻击尾态上下文，重新解析当前语义描述符。
 */
void USIPHeroAnimationBridgeComponent::UpdateCombatActionDescriptor()
{
	const ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get());
	if (!HeroCharacter)
	{
		SetCombatActionDescriptor(FSIPCombatActionDescriptor());
		return;
	}

	const FSIPCombatFeatureVector FeatureVector = SIPCombatSemantic::BuildHeroCombatFeatureVector(
		HeroCharacter,
		CurrentWeaponModuleTag,
		CurrentCastPhaseTag,
		GetIceRuneDaggerSignedTurnAngleDegrees(HeroCharacter),
		CombatSemanticProfile);

	FSIPCombatResolutionContext ResolutionContext;
	ResolutionContext.PreviousBodyStateTag = CurrentCombatBodyStateTag;
	ResolutionContext.bHasBufferedFollowUp = bQueuedBufferedFollowUpAfterAttack;
	ResolutionContext.bLockGoldenPath = bCombatBodyStateLocked && bLockedIceRuneDaggerSemantic;
	ResolutionContext.DelayedRestartMinSpeed = CombatSemanticProfile
		? CombatSemanticProfile->IceRuneDaggerDelayedRestartMinSpeed
		: IceRuneDaggerDelayedRestartMinSpeed;
	ResolutionContext.GlideExitMinSpeed = CombatSemanticProfile
		? CombatSemanticProfile->IceRuneDaggerGlideExitMinSpeed
		: IceRuneDaggerGlideExitMinSpeed;
	SetCombatActionDescriptor(SIPCombatSemantic::ResolveIceRuneDaggerGoldenPath(FeatureVector, ResolutionContext));

	// 延迟锁定：如果 GA 激活时速度瞬时为 0（traversal/落地瞬间），
	// 初始解析未能构建语义描述符导致 lock 未设置。
	// 下一帧速度恢复后解析出黄金链，此时补上 lock，
	// 避免语义状态随速度波动在后续帧被冲掉。
	if (HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking) &&
		CurrentCombatActionDescriptor.bGoldenPathActive &&
		!bCombatBodyStateLocked)
	{
		bCombatBodyStateLocked = true;
		bLockedIceRuneDaggerSemantic = true;
	}
}

/**
 * 发布一份新解析出来的语义描述符。
 *
 * 这里会同时更新：
 * 1. loose gameplay tags。
 * 2. 本地缓存的动作/身体状态字段。
 * 3. 调试输出。
 * 4. 主角线程安全快照，供动画层消费。
 */
void USIPHeroAnimationBridgeComponent::SetCombatActionDescriptor(const FSIPCombatActionDescriptor& Descriptor)
{
	const bool bDescriptorUnchanged =
		CurrentCombatActionFamilyTag == Descriptor.ActionFamilyTag &&
		CurrentCombatBodyStateTag == Descriptor.BodyStateTag &&
		CurrentCombatActionDescriptor.DesiredVariant == Descriptor.DesiredVariant &&
		CurrentCombatActionDescriptor.bUseMomentumWarp == Descriptor.bUseMomentumWarp &&
		CurrentCombatActionDescriptor.RecoveryBias == Descriptor.RecoveryBias &&
		CurrentCombatActionDescriptor.ChainWindowPolicy == Descriptor.ChainWindowPolicy &&
		CurrentCombatActionDescriptor.bGoldenPathActive == Descriptor.bGoldenPathActive;
	if (bDescriptorUnchanged)
	{
		return;
	}

	const FGameplayTag PreviousCombatActionFamilyTag = CurrentCombatActionFamilyTag;
	const FGameplayTag PreviousCombatBodyStateTag = CurrentCombatBodyStateTag;
	const FName PreviousDesiredVariant = CurrentCombatActionDescriptor.DesiredVariant;

	if (CurrentCombatActionFamilyTag.IsValid() && CurrentCombatActionFamilyTag != Descriptor.ActionFamilyTag)
	{
		RemoveAnimationStateTag(CurrentCombatActionFamilyTag);
	}

	if (CurrentCombatBodyStateTag.IsValid() && CurrentCombatBodyStateTag != Descriptor.BodyStateTag)
	{
		RemoveAnimationStateTag(CurrentCombatBodyStateTag);
	}

	CurrentCombatActionDescriptor = Descriptor;
	CurrentCombatActionFamilyTag = Descriptor.ActionFamilyTag;
	CurrentCombatBodyStateTag = Descriptor.BodyStateTag;

	// 跟踪本次攻击期间黄金链是否曾经被激活过，
	// 即使后续帧因速度下降塌回 None，FinishAttackAnimation 仍然知道有过语义状态。
	if (Descriptor.bGoldenPathActive && HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking))
	{
		bGoldenPathWasActiveDuringAttack = true;
	}

	if (CurrentCombatActionFamilyTag.IsValid() && CurrentCombatActionFamilyTag != PreviousCombatActionFamilyTag)
	{
		AddAnimationStateTag(CurrentCombatActionFamilyTag);
	}

	if (CurrentCombatBodyStateTag.IsValid() && CurrentCombatBodyStateTag != PreviousCombatBodyStateTag)
	{
		AddAnimationStateTag(CurrentCombatBodyStateTag);
	}

	if (bDebugCombatBodyState)
	{
		const float ResolvedSpeed = OwnerCharacter.IsValid() ? OwnerCharacter->GetVelocity().Size2D() : CachedVelocity.Size2D();
		EmitCombatBodyStateDebug(
			this,
			FString::Printf(
				TEXT("[CombatSemantic] %s/%s -> %s/%s | Variant=%s->%s | Speed=%.1f | Cast=%s | Weapon=%s | BufferedFollowUp=%s"),
				*PreviousCombatActionFamilyTag.ToString(),
				*PreviousCombatBodyStateTag.ToString(),
				*CurrentCombatActionFamilyTag.ToString(),
				*CurrentCombatBodyStateTag.ToString(),
				*PreviousDesiredVariant.ToString(),
				*CurrentCombatActionDescriptor.DesiredVariant.ToString(),
				ResolvedSpeed,
				*CurrentCastPhaseTag.ToString(),
				*CurrentWeaponModuleTag.ToString(),
				bQueuedBufferedFollowUpAfterAttack ? TEXT("Y") : TEXT("N")),
			bDebugCombatBodyStateOnScreen);
	}

	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		HeroCharacter->RefreshSandboxThreadSafeState();
	}
}

/**
 * 通过引用计数管理状态标签，避免标签被重复移除
 */
void USIPHeroAnimationBridgeComponent::AddAnimationStateTag(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}

	int32& TagCount = AnimationStateTagCounts.FindOrAdd(Tag);
	++TagCount;

	if (TagCount == 1)
	{
		ActiveAnimationStateTags.AddTag(Tag);

		if (USIPAbilitySystemComponent* AbilitySystemComponent = OwnerAbilitySystemComponent.Get())
		{
			AbilitySystemComponent->AddLooseGameplayTag(Tag);
		}
	}
}

/**
 * 当引用计数归零时，才真正移除标签并同步到 ASC
 */
void USIPHeroAnimationBridgeComponent::RemoveAnimationStateTag(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return;
	}

	int32* ExistingCount = AnimationStateTagCounts.Find(Tag);
	if (!ExistingCount)
	{
		return;
	}

	--(*ExistingCount);
	if (*ExistingCount > 0)
	{
		return;
	}

	AnimationStateTagCounts.Remove(Tag);
	ActiveAnimationStateTags.RemoveTag(Tag);

	if (USIPAbilitySystemComponent* AbilitySystemComponent = OwnerAbilitySystemComponent.Get())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	}
}

void USIPHeroAnimationBridgeComponent::ClearCombatStateIfIdle()
{
	const bool bHasAttackState = HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);
	const bool bHasThrowState = HasAnimationStateTag(SIPGameplayTags::State_Combat_Throwing);

	if (!bHasAttackState && !bHasThrowState)
	{
		RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
		if (CurrentCastPhaseTag.IsValid())
		{
			SetCastPhaseTag(FGameplayTag());
		}

		const bool bHasSemanticTail = CurrentCombatActionDescriptor.HasResolvedAction();
		if (!bHasSemanticTail)
		{
			bQueuedBufferedFollowUpAfterAttack = false;
			SetCombatActionDescriptor(FSIPCombatActionDescriptor());
			SetWeaponModuleTag(FGameplayTag());
			ResetCombatBodyStateLock();
			RemoveAnimationStateTag(SIPGameplayTags::State_Combat);
			// 无语义尾巴时，攻击蒙太奇已经完全退场，不需要继续保留 MM grace。
			ClearPostAttackMMSuppressionGrace();
		}
		else
		{
			StartSemanticTailStateTTL();
		}
	}
}

void USIPHeroAnimationBridgeComponent::ResetAnimationEventDispatchState(const FGameplayTag& EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	TrackedAnimationEvents.Add(EventTag);
	DispatchedAnimationEvents.Remove(EventTag);
}

/**
 * 通过定时器延迟派发某个动画事件，用于没有 Notify 时的回退时序
 */
void USIPHeroAnimationBridgeComponent::ScheduleAnimationEvent(FGameplayTag EventTag, float DelaySeconds)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	CancelAnimationEvent(EventTag);
	ResetAnimationEventDispatchState(EventTag);

	if (DelaySeconds <= 0.0f)
	{
		DispatchAnimationEvent(EventTag);
		return;
	}

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &USIPHeroAnimationBridgeComponent::DispatchAnimationEvent, EventTag);

	FTimerHandle& TimerHandle = ScheduledAnimationEvents.FindOrAdd(EventTag);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, DelaySeconds, false);
	}
}

void USIPHeroAnimationBridgeComponent::CancelAnimationEvent(FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (FTimerHandle* TimerHandle = ScheduledAnimationEvents.Find(EventTag))
		{
			World->GetTimerManager().ClearTimer(*TimerHandle);
		}
	}

	ScheduledAnimationEvents.Remove(EventTag);
}

/**
 * 真正将动画事件转发给 Owner Actor 的 GAS 事件系统
 */
void USIPHeroAnimationBridgeComponent::DispatchAnimationEvent(FGameplayTag EventTag)
{
	if (!EventTag.IsValid())
	{
		return;
	}

	const bool bIsTrackedEvent = TrackedAnimationEvents.Contains(EventTag);
	if (bIsTrackedEvent && DispatchedAnimationEvents.Contains(EventTag))
	{
		return;
	}

	CancelAnimationEvent(EventTag);
	if (bIsTrackedEvent)
	{
		DispatchedAnimationEvents.Add(EventTag);
	}
	ApplyAnimationEventState(EventTag);

	if (AActor* OwnerActor = GetOwner())
	{
		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = OwnerActor;
		Payload.Target = OwnerActor;
		Payload.OptionalObject = this;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
	}
}

void USIPHeroAnimationBridgeComponent::ApplyAnimationEventState(FGameplayTag EventTag)
{
	if (EventTag == SIPGameplayTags::Event_Animation_Attack_HitWindow_Start)
	{
		if (HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking))
		{
			AddAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
			SetCastPhaseTag(SIPGameplayTags::State_Combat_Cast_Release);
		}
	}
	else if (EventTag == SIPGameplayTags::Event_Animation_Attack_HitWindow_End)
	{
		RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
		if (HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking))
		{
			SetCastPhaseTag(SIPGameplayTags::State_Combat_Cast_Recover);
		}
	}
	else if (EventTag == SIPGameplayTags::Event_Animation_Throw_Release)
	{
		if (HasAnimationStateTag(SIPGameplayTags::State_Combat_Throwing))
		{
			SetCastPhaseTag(SIPGameplayTags::State_Combat_Cast_Release);
		}
	}
}

void USIPHeroAnimationBridgeComponent::CacheOwnerReferences()
{
	OwnerCharacter = Cast<ASIPCharacter>(GetOwner());
	OwnerAbilitySystemComponent = OwnerCharacter.IsValid() ? OwnerCharacter->GetSIPAbilitySystemComponent() : nullptr;
}

void USIPHeroAnimationBridgeComponent::ResetCombatBodyStateLock()
{
	bCombatBodyStateLocked = false;
	bLockedIceRuneDaggerSemantic = false;
}

bool USIPHeroAnimationBridgeComponent::ShouldSuppressMotionMatching() const
{
	if (bCombatLandingRecoveryActive)
	{
		return false;
	}

	// 任何活跃的全身攻击蒙太奇都必须压制 MM，避免 PoseSearch 与 DefaultSlot 争抢全身骨骼。
	// 唯一例外是角色持续处于 Falling 状态超过阈值，此时放行 MM，
	// 让 GASP 的 InAir / Landing 数据库接管着陆过渡。
	// 低于阈值的微型落差（冰面不平地形导致的 1-2 帧 Falling 抖动）不放行，
	// 防止攻击蒙太奇期间 MM 闪烁激活产生抽搐。
	static constexpr float MinFallingDurationForMMRelease = 0.15f;
	if (HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking))
	{
		return !(bIsFalling && FallingDuration >= MinFallingDurationForMMRelease);
	}

	// 攻击蒙太奇结束后的短暂宽限期，
	// 让角色动量自然衬减而不是被 MM 弹回去。
	return bPostAttackMMSuppressionGraceActive;
}

ESIPSemanticLocomotionMode USIPHeroAnimationBridgeComponent::GetSemanticLocomotionMode() const
{
	const bool bOnIce = [this]()
	{
		const ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get());
		if (!HeroCharacter) return false;
		const USIPSandboxLocomotionComponent* Loco = HeroCharacter->GetSandboxLocomotionComponent();
		return Loco && Loco->IsIceSurfaceActive();
	}();

	if (ShouldSuppressMotionMatching())
	{
		return ESIPSemanticLocomotionMode::SemanticCombatOverride;
	}

	if (bOnIce && IsInCombatPresentation())
	{
		return ESIPSemanticLocomotionMode::IceCombat;
	}

	if (bOnIce)
	{
		return ESIPSemanticLocomotionMode::IceLocomotion;
	}

	return ESIPSemanticLocomotionMode::Default;
}

void USIPHeroAnimationBridgeComponent::StartSemanticTailStateTTL()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(SemanticTailStateTTLHandle);

	const float TTLSeconds = CombatSemanticProfile
		? CombatSemanticProfile->SemanticTailStateTTLSeconds
		: SemanticTailStateTTLSeconds;

	FTimerDelegate Delegate;
	Delegate.BindUObject(this, &USIPHeroAnimationBridgeComponent::OnSemanticTailStateTTLExpired);
	World->GetTimerManager().SetTimer(SemanticTailStateTTLHandle, Delegate, TTLSeconds, false);
}

void USIPHeroAnimationBridgeComponent::OnSemanticTailStateTTLExpired()
{
	if (HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking) ||
		HasAnimationStateTag(SIPGameplayTags::State_Combat_Throwing))
	{
		return;
	}

	bQueuedBufferedFollowUpAfterAttack = false;
	ClearPostAttackMMSuppressionGrace();
	SetCombatActionDescriptor(FSIPCombatActionDescriptor());
	SetWeaponModuleTag(FGameplayTag());
	ResetCombatBodyStateLock();
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat);
}

void USIPHeroAnimationBridgeComponent::ClearPostAttackMMSuppressionGrace()
{
	bPostAttackMMSuppressionGraceActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostAttackMMSuppressionGraceHandle);
	}
}

void USIPHeroAnimationBridgeComponent::OnPostAttackMMSuppressionGraceExpired()
{
	bPostAttackMMSuppressionGraceActive = false;

	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		HeroCharacter->RefreshSandboxThreadSafeState();
	}
}

void USIPHeroAnimationBridgeComponent::NotifyMontageFullyEnded()
{
	if (!bPostAttackMMSuppressionGraceActive)
	{
		return;
	}

	ClearPostAttackMMSuppressionGrace();

	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
	{
		HeroCharacter->RefreshSandboxThreadSafeState();
	}
}

void USIPHeroAnimationBridgeComponent::NotifyLanded()
{
	const bool bInCombatAnimation =
		HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking) ||
		bPostAttackMMSuppressionGraceActive;

	if (!bInCombatAnimation)
	{
		return;
	}

	bCombatLandingRecoveryActive = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CombatLandingRecoveryHandle);
		World->GetTimerManager().SetTimer(
			CombatLandingRecoveryHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				bCombatLandingRecoveryActive = false;
				if (ASIPHeroCharacter* HC = Cast<ASIPHeroCharacter>(OwnerCharacter.Get()))
				{
					HC->RefreshSandboxThreadSafeState();
				}
			}),
			0.30f,
			false);
	}
}
