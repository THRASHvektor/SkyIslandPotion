// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Components/SIPHeroAnimationBridgeComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Character/SIPCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SIPGameplayTags.h"
#include "TimerManager.h"

/**
 * Z 说明：
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

// Z 说明：BeginPlay 时先缓存 Owner 角色和 ASC，减少后续重复查找
void USIPHeroAnimationBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerReferences();
}

/**
 * Z 说明：TickComponent
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
}

/**
 * Z 说明：RequestAttackAnimation
 * 进入一次攻击表现流程，并安排命中窗口开始/结束事件
 */
bool USIPHeroAnimationBridgeComponent::RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay)
{
	CancelAttackAnimation();

	LastRequestedActionTag = SIPGameplayTags::Event_Animation_Attack_Request;
	ResetAnimationEventDispatchState(LastRequestedActionTag);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	AddAnimationStateTag(SIPGameplayTags::State_Combat);
	AddAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);

	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start, HitWindowStartDelay);
	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_End, HitWindowEndDelay);
	DispatchAnimationEvent(LastRequestedActionTag);
	return true;
}

/**
 * Z 说明：RequestThrowAnimation
 * 进入一次投掷表现流程，并安排释放事件
 */
bool USIPHeroAnimationBridgeComponent::RequestThrowAnimation(float ReleaseDelay)
{
	CancelThrowAnimation();

	LastRequestedActionTag = SIPGameplayTags::Event_Animation_Throw_Request;
	ResetAnimationEventDispatchState(LastRequestedActionTag);
	ResetAnimationEventDispatchState(SIPGameplayTags::Event_Animation_Throw_Release);
	AddAnimationStateTag(SIPGameplayTags::State_Combat);
	AddAnimationStateTag(SIPGameplayTags::State_Combat_Throwing);

	ScheduleAnimationEvent(SIPGameplayTags::Event_Animation_Throw_Release, ReleaseDelay);
	DispatchAnimationEvent(LastRequestedActionTag);
	return true;
}

// Z 说明：取消攻击表现相关的事件与状态标签
void USIPHeroAnimationBridgeComponent::CancelAttackAnimation()
{
	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_Request);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_HitWindow_Start);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Attack_HitWindow_End);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);
	ClearCombatStateIfIdle();
}

// Z 说明：取消投掷表现相关的事件与状态标签
void USIPHeroAnimationBridgeComponent::CancelThrowAnimation()
{
	CancelAnimationEvent(SIPGameplayTags::Event_Animation_Throw_Release);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Throw_Request);
	DispatchedAnimationEvents.Add(SIPGameplayTags::Event_Animation_Throw_Release);
	RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Throwing);
	ClearCombatStateIfIdle();
}

// Z 说明：动画 Notify 可以直接通过该入口把事件送入桥接组件
void USIPHeroAnimationBridgeComponent::NotifyAnimationEvent(FGameplayTag EventTag)
{
	DispatchAnimationEvent(EventTag);
}

// Z 说明：判断当前是否持有指定表现状态标签
bool USIPHeroAnimationBridgeComponent::HasAnimationStateTag(FGameplayTag Tag) const
{
	return Tag.IsValid() && ActiveAnimationStateTags.HasTagExact(Tag);
}

// Z 说明：只要仍处于 Combat 状态标签下，就认为在战斗表现阶段
bool USIPHeroAnimationBridgeComponent::IsInCombatPresentation() const
{
	return HasAnimationStateTag(SIPGameplayTags::State_Combat);
}

/**
 * Z 说明：AddAnimationStateTag
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
 * Z 说明：RemoveAnimationStateTag
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

// Z 说明：攻击和投掷都结束后，移除总战斗状态标签
void USIPHeroAnimationBridgeComponent::ClearCombatStateIfIdle()
{
	const bool bHasAttackState = HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking);
	const bool bHasThrowState = HasAnimationStateTag(SIPGameplayTags::State_Combat_Throwing);

	if (!bHasAttackState && !bHasThrowState)
	{
		RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
		RemoveAnimationStateTag(SIPGameplayTags::State_Combat);
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
 * Z 说明：ScheduleAnimationEvent
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

// Z 说明：取消某个已注册的动画事件定时器
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
 * Z 说明：DispatchAnimationEvent
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

// Z 说明：根据事件类型同步命中窗口等表现状态标签
void USIPHeroAnimationBridgeComponent::ApplyAnimationEventState(FGameplayTag EventTag)
{
	if (EventTag == SIPGameplayTags::Event_Animation_Attack_HitWindow_Start)
	{
		if (HasAnimationStateTag(SIPGameplayTags::State_Combat_Attacking))
		{
			AddAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
		}
	}
	else if (EventTag == SIPGameplayTags::Event_Animation_Attack_HitWindow_End)
	{
		RemoveAnimationStateTag(SIPGameplayTags::State_Combat_Attack_HitWindow);
	}
}

// Z 说明：缓存 OwnerCharacter 与 ASC，供后续事件分发和标签同步使用
void USIPHeroAnimationBridgeComponent::CacheOwnerReferences()
{
	OwnerCharacter = Cast<ASIPCharacter>(GetOwner());
	OwnerAbilitySystemComponent = OwnerCharacter.IsValid() ? OwnerCharacter->GetSIPAbilitySystemComponent() : nullptr;
}
