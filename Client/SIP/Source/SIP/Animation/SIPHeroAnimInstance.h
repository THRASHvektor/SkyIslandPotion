// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "SIPHeroAnimInstance.generated.h"

class ASIPHeroCharacter;
class USIPHeroAnimationBridgeComponent;

/**
 * Z 说明：
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
	// Z 说明：初始化动画实例时缓存主角与桥接组件引用
	virtual void NativeInitializeAnimation() override;

	// Z 说明：每帧同步桥接组件中的动画表现数据
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Z 说明：返回当前拥有该动画实例的主角角色
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	ASIPHeroCharacter* GetOwningHeroCharacter() const { return OwningHeroCharacter.Get(); }

	// Z 说明：返回当前动画实例关联的桥接组件
	UFUNCTION(BlueprintPure, Category = "SIP|Animation")
	USIPHeroAnimationBridgeComponent* GetAnimationBridgeComponent() const { return AnimationBridgeComponent.Get(); }

protected:
	// Z 说明：重新缓存拥有者角色与桥接组件引用
	void CacheAnimationReferences();

	// Z 说明：将桥接组件状态同步到当前动画实例属性
	void SyncFromAnimationBridge();

	// Z 说明：清空动画实例缓存的表现层状态
	void ResetAnimationState();

protected:
	// Z 说明：当前拥有该动画实例的主角角色
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	TObjectPtr<ASIPHeroCharacter> OwningHeroCharacter = nullptr;

	// Z 说明：当前关联的动画桥接组件
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	TObjectPtr<USIPHeroAnimationBridgeComponent> AnimationBridgeComponent = nullptr;

	// Z 说明：角色当前平面地面速度
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	float GroundSpeed = 0.0f;

	// Z 说明：角色当前世界速度
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	FVector Velocity = FVector::ZeroVector;

	// Z 说明：角色当前是否正在移动
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	bool bIsMoving = false;

	// Z 说明：角色当前是否处于下落状态
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	bool bIsFalling = false;

	// Z 说明：角色当前是否处于跳跃上升阶段
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	bool bIsJumping = false;

	// Z 说明：角色当前是否处于战斗表现阶段
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	bool bIsInCombatPresentation = false;

	// Z 说明：当前动画实例是否成功连接到桥接组件
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	bool bHasAnimationBridge = false;

	// Z 说明：桥接组件当前维护的表现状态标签集合
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	FGameplayTagContainer ActiveAnimationStateTags;

	// Z 说明：最近一次请求的动作标签，例如攻击请求或投掷请求
	UPROPERTY(Transient, BlueprintReadOnly, Category = "SIP|Animation")
	FGameplayTag LastRequestedActionTag;
};
