// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Combat/SIPCombatSemanticResolver.h"
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "SIPHeroAnimInstance.generated.h"

class ASIPHeroCharacter;
class USIPHeroAnimationBridgeComponent;

/**
 * SIPHeroAnimInstance 是主角动画蓝图推荐继承的基础 AnimInstance。
 *
 * 核心职责：
 * 1. 缓存主角与动画桥接组件引用。
 * 2. 每帧把桥接组件中的移动/战斗表现数据同步到动画层。
 * 3. 为后续自定义 AnimBP 提供统一的 Blueprint 读取入口。
 */
UCLASS(Blueprintable, BlueprintType)
class SIP_API USIPHeroAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/**
	 * 在 Blueprint 初始化开始消费线程安全状态之前，
	 * 先缓存主角和桥接组件引用。
	 */
	virtual void NativeInitializeAnimation() override;

	/**
	 * 每次更新时刷新面向动画层的状态快照。
	 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation", meta = (BlueprintThreadSafe))
	ASIPHeroCharacter* GetOwningHeroCharacter() const { return OwningHeroCharacter.Get(); }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation", meta = (BlueprintThreadSafe))
	USIPHeroAnimationBridgeComponent* GetAnimationBridgeComponent() const { return AnimationBridgeComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool HasWeaponModuleTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool HasCastPhaseTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool HasCombatActionFamilyTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool IsFlaskRigCasting() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool HasCombatBodyStateTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool IsIceRuneDaggerSlideAttack() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool IsIceRuneDaggerSlipRecovery() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool ShouldEnableCombatAimOffset() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool ShouldPreferCombatSteering() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	float GetCombatSemanticLeanScale() const;

	/**
	 * Motion Matching 应在返回 true 时被 ABP 暂停，
	 * 让战斗蒙太奇完全接管角色。
	 */
	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	bool ShouldSuppressMotionMatching() const;

	/**
	 * 返回语义系统建议的 locomotion 模式，
	 * ABP 可以用它来选择正确的 PoseSearchDatabase。
	 */
	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	ESIPSemanticLocomotionMode GetSemanticLocomotionMode() const;

	/**
	 * 返回当前语义状态对应的 PoseSearch 数据库标签。
	 * 当 Profile 中配置了对应 Mode 的标签时返回有效值，
	 * 否则返回空标签（交给 ABP 自行按默认逻辑选库）。
	 */
	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	FGameplayTag GetDesiredPoseSearchDatabaseTag() const;

	/**
	 * ABP 的 Update_MotionMatching 中 Get MMInterrupt Mode 节点调用此函数。
	 * 抑制期间返回 DoNotInterrupt，让 MM 继续播放当前动画而不重新搜索；
	 * 非抑制期间返回 InterruptOnDatabaseChange，允许 Chooser 切库时中断。
	 */
	UFUNCTION(BlueprintPure, Category = "SIP|Animation|Combat", meta = (BlueprintThreadSafe))
	EPoseSearchInterruptMode GetMMInterruptMode() const;

protected:
	/**
	 * 当引用失效时，重新解析拥有者主角和桥接组件。
	 */
	void CacheAnimationReferences();

	/**
	 * 把桥接组件持有的运行时状态复制进 AnimInstance 字段。
	 */
	void SyncFromAnimationBridge();

	/**
	 * 从当前语义状态派生出动画层会频繁读取的布尔量和标量缓存。
	 */
	void UpdateCombatSemanticCache();

	/**
	 * 当桥接数据不可用时，重置 AnimInstance 内部状态。
	 */
	void ResetAnimationState();

protected:
	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	TObjectPtr<ASIPHeroCharacter> OwningHeroCharacter = nullptr;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	TObjectPtr<USIPHeroAnimationBridgeComponent> AnimationBridgeComponent = nullptr;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	float GroundSpeed = 0.0f;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	bool bIsMoving = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	bool bIsFalling = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	bool bIsJumping = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	bool bIsInCombatPresentation = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	bool bHasAnimationBridge = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	FGameplayTagContainer ActiveAnimationStateTags;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	FGameplayTag LastRequestedActionTag;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	FGameplayTag CurrentWeaponModuleTag;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	FGameplayTag CurrentCastPhaseTag;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	FGameplayTag CurrentCombatActionFamilyTag;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation")
	FGameplayTag CurrentCombatBodyStateTag;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	FName CurrentCombatDesiredVariant = NAME_None;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	bool bShouldUseMomentumWarpForCombatAction = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	ESIPRecoveryBias CurrentCombatRecoveryBias = ESIPRecoveryBias::Fast;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	ESIPChainWindowPolicy CurrentCombatChainWindowPolicy = ESIPChainWindowPolicy::Normal;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	bool bIsFlaskRigCasting = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	bool bIsIceRuneDaggerSlideAttack = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	bool bIsIceRuneDaggerSlipRecovery = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	bool bShouldEnableCombatAimOffset = true;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	bool bShouldPreferCombatSteering = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	float CombatSemanticLeanScale = 1.0f;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	bool bShouldSuppressMotionMatching = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	ESIPSemanticLocomotionMode SemanticLocomotionMode = ESIPSemanticLocomotionMode::Default;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "SIP|Animation|Combat")
	FGameplayTag DesiredPoseSearchDatabaseTag;
};
