// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/SIPCombatSemanticResolver.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/Components/SIPComponent.h"
#include "SIPHeroAnimationBridgeComponent.generated.h"

class ASIPCharacter;
class ASIPHeroCharacter;
class USIPAbilitySystemComponent;
class USIPCombatSemanticProfile;

/**
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
	USIPHeroAnimationBridgeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 组件进入世界后缓存拥有者相关引用。
	 */
	virtual void BeginPlay() override;
	
	/**
	 * 每帧刷新移动缓存和战斗语义输出。
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 不需要显式武器模块标签时使用的攻击请求便捷重载。
	 */
	bool RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay);

	/**
	 * 进入攻击表现状态，并挂上攻击能力会监听的时序事件。
	 */
	bool RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay, FGameplayTag WeaponModuleTag, FGameplayTag InitialCastPhaseTag);

	/**
	 * 使用 GA 已预解析的语义描述符进入攻击表现状态。
	 * 跳过桥接层自身的初始解析，消除 GA 与 Bridge 之间的时序不一致。
	 */
	bool RequestAttackAnimation(float HitWindowStartDelay, float HitWindowEndDelay, FGameplayTag WeaponModuleTag, FGameplayTag InitialCastPhaseTag, const FSIPCombatActionDescriptor& PreResolvedDescriptor);
	
	/**
	 * 不需要显式武器模块标签时使用的投掷请求便捷重载。
	 */
	bool RequestThrowAnimation(float ReleaseDelay);

	/**
	 * 进入投掷表现状态，并挂上释放事件。
	 */
	bool RequestThrowAnimation(float ReleaseDelay, FGameplayTag WeaponModuleTag, FGameplayTag InitialCastPhaseTag);

	/**
	 * 硬取消攻击表现，并清掉语义尾态。
	 */
	void CancelAttackAnimation();

	/**
	 * 软结束攻击表现，让攻击后的语义尾态还能保留下来，
	 * 例如 glide exit 或 delayed restart。
	 */
	void FinishAttackAnimation(bool bQueuedBufferedFollowUp = false);
	
	/**
	 * 硬取消投掷表现。
	 */
	void CancelThrowAnimation();

	UFUNCTION(BlueprintCallable, Category = "SIP|Animation")
	void NotifyAnimationEvent(FGameplayTag EventTag);

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool HasAnimationStateTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTagContainer GetAnimationStateTags() const { return ActiveAnimationStateTags; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTag GetLastRequestedActionTag() const { return LastRequestedActionTag; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTag GetCurrentWeaponModuleTag() const { return CurrentWeaponModuleTag; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTag GetCurrentCastPhaseTag() const { return CurrentCastPhaseTag; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTag GetCurrentCombatActionFamilyTag() const { return CurrentCombatActionFamilyTag; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FGameplayTag GetCurrentCombatBodyStateTag() const { return CurrentCombatBodyStateTag; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FSIPCombatActionDescriptor GetCurrentCombatActionDescriptor() const { return CurrentCombatActionDescriptor; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool HasCurrentWeaponModuleTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsInCastPhase(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool HasCurrentCombatActionFamilyTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool HasCurrentCombatBodyStateTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	float GetGroundSpeed() const { return GroundSpeed; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	FVector GetVelocity() const { return CachedVelocity; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsMoving() const { return GroundSpeed > KINDA_SMALL_NUMBER; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsFalling() const { return bIsFalling; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsJumping() const { return bIsJumping; }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool IsInCombatPresentation() const;

	/**
	 * 当语义战斗蒙太奇正在播放时返回 true，
	 * ABP 应用此信号暂停 Motion Matching 搜索。
	 */
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	bool ShouldSuppressMotionMatching() const;

	/**
	 * 返回当前语义系统建议的 locomotion 模式，
	 * 让 ABP 决定应该搜哪组 PoseSearchDatabase。
	 */
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	ESIPSemanticLocomotionMode GetSemanticLocomotionMode() const;

	/**
	 * 获取当前绑定的语义调参 Profile（可为 null）。
	 */
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	USIPCombatSemanticProfile* GetCombatSemanticProfile() const { return CombatSemanticProfile; }

private:
	/**
	 * 计算 Ice Rune Dagger 语义链使用的“面朝 vs 速度方向”有符号夹角。
	 */
	float GetIceRuneDaggerSignedTurnAngleDegrees(const ASIPCharacter* Character) const;

	/**
	 * 切换当前对外发布的武器模块标签。
	 */
	void SetWeaponModuleTag(const FGameplayTag& Tag);

	/**
	 * 切换当前对外发布的施法阶段标签。
	 */
	void SetCastPhaseTag(const FGameplayTag& Tag);

	/**
	 * 用当前运行时状态重新计算共享语义描述符。
	 */
	void UpdateCombatActionDescriptor();

	/**
	 * 把新解析出的语义描述符应用到标签和缓存字段上。
	 */
	void SetCombatActionDescriptor(const FSIPCombatActionDescriptor& Descriptor);

	/**
	 * 通过引用计数添加一个 loose gameplay tag，
	 * 让多个系统可以安全地共同发布同一个标签。
	 */
	void AddAnimationStateTag(const FGameplayTag& Tag);
	
	/**
	 * 移除一个 loose gameplay tag 引用，
	 * 只有没有任何持有者时才真正清掉。
	 */
	void RemoveAnimationStateTag(const FGameplayTag& Tag);

	/**
	 * 重置某个已跟踪动画事件的去重状态。
	 */
	void ResetAnimationEventDispatchState(const FGameplayTag& EventTag);
	
	/**
	 * 只有攻击和投掷两条表现状态都完全空闲后，才清理战斗表现层状态。
	 */
	void ClearCombatStateIfIdle();
	
	/**
	 * 为桥接层管理的动画事件安排一个计时器回退。
	 */
	void ScheduleAnimationEvent(FGameplayTag EventTag, float DelaySeconds);
	
	/**
	 * 取消一个通过计时器安排的动画事件。
	 */
	void CancelAnimationEvent(FGameplayTag EventTag);
	
	/**
	 * 把一个动画事件送进拥有者 Actor 的 GAS 事件通道。
	 */
	void DispatchAnimationEvent(FGameplayTag EventTag);
	
	/**
	 * 当动画事件触发时，更新桥接层自己应该同步推进的状态，
	 * 例如进入 release 或 recover。
	 */
	void ApplyAnimationEventState(FGameplayTag EventTag);
	
	/**
	 * 缓存桥接层每帧都会用到的 Owner 和 ASC 引用。
	 */
	void CacheOwnerReferences();

	/**
	 * 当当前攻击不再持有黄金链语义锁时，清掉这层临时锁定。
	 */
	void ResetCombatBodyStateLock();

	/**
	 * 启动语义尾态自动衰减计时器。
	 */
	void StartSemanticTailStateTTL();

	/**
	 * 尾态 TTL 到期后清理残留语义状态。
	 */
	void OnSemanticTailStateTTLExpired();

	/**
	 * 清除攻击后 MM 抑制宽限期。
	 */
	void ClearPostAttackMMSuppressionGrace();

	/**
	 * 攻击后 MM 抑制宽限期到期时恢复 MM。
	 */
	void OnPostAttackMMSuppressionGraceExpired();

	TWeakObjectPtr<ASIPCharacter> OwnerCharacter;
	
	TWeakObjectPtr<USIPAbilitySystemComponent> OwnerAbilitySystemComponent;

	UPROPERTY(Transient)
	FGameplayTagContainer ActiveAnimationStateTags;

	UPROPERTY(Transient)
	FGameplayTag LastRequestedActionTag;

	UPROPERTY(Transient)
	FGameplayTag CurrentWeaponModuleTag;

	UPROPERTY(Transient)
	FGameplayTag CurrentCastPhaseTag;

	UPROPERTY(Transient)
	FGameplayTag CurrentCombatActionFamilyTag;

	UPROPERTY(Transient)
	FGameplayTag CurrentCombatBodyStateTag;

	UPROPERTY(Transient)
	FSIPCombatActionDescriptor CurrentCombatActionDescriptor;

	/**
	 * 可选的语义调参 Profile 数据资产。
	 * 设置后，桥接层和解析器会读取 Profile 中的阈值替代内部默认值。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Combat")
	TObjectPtr<USIPCombatSemanticProfile> CombatSemanticProfile;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Combat", meta = (ClampMin = "0.0"))
	float IceRuneDaggerMinMomentumForSlideState = 260.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Combat", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float IceRuneDaggerDriftTurnMinAngleDegrees = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Combat", meta = (ClampMin = "0.0"))
	float IceRuneDaggerDelayedRestartMinSpeed = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Combat", meta = (ClampMin = "0.0"))
	float IceRuneDaggerGlideExitMinSpeed = 120.0f;

	/**
	 * 语义尾态（GlideExit / DelayedRestart 等）在没有后续输入时最多保持多久。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Combat", meta = (ClampMin = "0.1"))
	float SemanticTailStateTTLSeconds = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Debug")
	bool bDebugCombatBodyState = true;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Debug")
	bool bDebugCombatBodyStateOnScreen = true;

	TMap<FGameplayTag, int32> AnimationStateTagCounts;
	
	TMap<FGameplayTag, FTimerHandle> ScheduledAnimationEvents;

	TSet<FGameplayTag> TrackedAnimationEvents;

	TSet<FGameplayTag> DispatchedAnimationEvents;

	bool bCombatBodyStateLocked = false;
	bool bLockedIceRuneDaggerSemantic = false;
	bool bQueuedBufferedFollowUpAfterAttack = false;
	bool bGoldenPathWasActiveDuringAttack = false;

	FTimerHandle SemanticTailStateTTLHandle;

	/**
	 * 攻击蒙太奇结束后保持 MM 抑制的宽限期（秒）。
	 * 须 >= 动态 BlendOut 上限（0.50s），让 DefaultSlot 完全混回 MM 输出后才放行搜索，
	 * 防止 OffsetRootBone 在过渡期中看到 > 30 单位的位置跳变导致角色瞬移。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Animation|Combat", meta = (ClampMin = "0.0"))
	float PostAttackMMSuppressionGraceSeconds = 0.60f;

	FTimerHandle PostAttackMMSuppressionGraceHandle;
	bool bPostAttackMMSuppressionGraceActive = false;

	float GroundSpeed = 0.0f;
	
	FVector CachedVelocity = FVector::ZeroVector;
	
	bool bIsFalling = false;
	
	bool bIsJumping = false;
};
