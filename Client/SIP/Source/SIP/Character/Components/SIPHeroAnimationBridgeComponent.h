// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/Components/SIPComponent.h"
#include "SIPHeroAnimationBridgeComponent.generated.h"

class ASIPCharacter;
class USIPAbilitySystemComponent;

/**
 * Z 说明：
 * HeroAnimationBridgeComponent 是“玩法逻辑”和“动画表现层”之间的桥接组件。
 *
 * 核心职责：
 * 1. 向动画蓝图暴露移动与战斗表现状态。
 * 2. 接收动画 Notify 事件，并转发为 GAS 可消费的 Gameplay Event。
 * 3. 在没有完整动画资源接入时，提供一套基于定时器的事件回退机制。
 */
UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPHeroAnimationBridgeComponent : public USIPComponent
{
	GENERATED_BODY()

public:
	// Z 说明：构造函数，启用 Tick 以便持续同步移动状态
	USIPHeroAnimationBridgeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Z 说明：开始游戏时缓存 Owner 与 ASC 引用
	virtual void BeginPlay() override;
	
	// Z 说明：每帧同步角色速度、落地状态和跳跃状态，供动画层读取
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Z 说明：请求一次攻击表现，并安排命中窗口事件
	bool RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay);
	
	// Z 说明：请求一次投掷表现，并安排释放事件
	bool RequestThrowAnimation(float ReleaseDelay);

	// Z 说明：取消当前攻击表现相关的定时事件与状态标签
	void CancelAttackAnimation();
	
	// Z 说明：取消当前投掷表现相关的定时事件与状态标签
	void CancelThrowAnimation();

	// Z 说明：供动画 Notify 主动调用，将某个事件转发到 Gameplay 层
	UFUNCTION(BlueprintCallable, Category = "SIP|Animation")
	void NotifyAnimationEvent(FGameplayTag EventTag);

	// Z 说明：查询当前桥接组件是否持有某个表现状态标签
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool HasAnimationStateTag(FGameplayTag Tag) const;

	// Z 说明：返回当前激活的表现状态标签集合
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTagContainer GetAnimationStateTags() const { return ActiveAnimationStateTags; }

	// Z 说明：返回最近一次请求的动作事件标签，方便动画蓝图分支判断
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTag GetLastRequestedActionTag() const { return LastRequestedActionTag; }

	// Z 说明：返回地面平面速度
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	float GetGroundSpeed() const { return GroundSpeed; }

	// Z 说明：返回角色当前世界速度
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FVector GetVelocity() const { return CachedVelocity; }

	// Z 说明：简单判断角色当前是否在移动
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsMoving() const { return GroundSpeed > KINDA_SMALL_NUMBER; }

	// Z 说明：返回角色当前是否处于下落状态
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsFalling() const { return bIsFalling; }

	// Z 说明：返回角色当前是否处于起跳上升阶段
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsJumping() const { return bIsJumping; }

	// Z 说明：判断当前是否处于战斗表现阶段
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsInCombatPresentation() const;

private:
	// Z 说明：增加一个表现状态标签，并同步到 ASC Loose Tag
	void AddAnimationStateTag(const FGameplayTag& Tag);
	
	// Z 说明：移除一个表现状态标签，并同步到 ASC Loose Tag
	void RemoveAnimationStateTag(const FGameplayTag& Tag);

	void ResetAnimationEventDispatchState(const FGameplayTag& EventTag);
	
	// Z 说明：如果当前没有攻击/投掷表现，则清理总战斗状态
	void ClearCombatStateIfIdle();
	
	// Z 说明：注册一个延时动画事件，用于模拟或补足动画时序
	void ScheduleAnimationEvent(FGameplayTag EventTag, float DelaySeconds);
	
	// Z 说明：取消某个延时动画事件
	void CancelAnimationEvent(FGameplayTag EventTag);
	
	// Z 说明：真正派发动画事件到 Owner Actor
	void DispatchAnimationEvent(FGameplayTag EventTag);
	
	// Z 说明：根据事件更新桥接组件维护的表现状态标签
	void ApplyAnimationEventState(FGameplayTag EventTag);
	
	// Z 说明：缓存 Owner Character 与其 AbilitySystemComponent
	void CacheOwnerReferences();

	// Z 说明：拥有该桥接组件的角色
	TWeakObjectPtr<ASIPCharacter> OwnerCharacter;
	
	// Z 说明：角色持有的 ASC，用于同步 Loose Gameplay Tags
	TWeakObjectPtr<USIPAbilitySystemComponent> OwnerAbilitySystemComponent;

	// Z 说明：当前激活的表现状态标签集合
	UPROPERTY(Transient)
	FGameplayTagContainer ActiveAnimationStateTags;

	// Z 说明：最近一次请求的动作标签，例如攻击请求、投掷请求
	UPROPERTY(Transient)
	FGameplayTag LastRequestedActionTag;

	// Z 说明：标签引用计数，避免同一标签被多处流程重复增删时出错
	TMap<FGameplayTag, int32> AnimationStateTagCounts;
	
	// Z 说明：当前已注册的延时动画事件定时器
	TMap<FGameplayTag, FTimerHandle> ScheduledAnimationEvents;

	TSet<FGameplayTag> TrackedAnimationEvents;

	TSet<FGameplayTag> DispatchedAnimationEvents;

	// Z 说明：地面平面速度缓存
	float GroundSpeed = 0.0f;
	
	// Z 说明：角色世界速度缓存
	FVector CachedVelocity = FVector::ZeroVector;
	
	// Z 说明：角色是否处于下落状态
	bool bIsFalling = false;
	
	// Z 说明：角色是否处于跳跃上升阶段
	bool bIsJumping = false;
};
