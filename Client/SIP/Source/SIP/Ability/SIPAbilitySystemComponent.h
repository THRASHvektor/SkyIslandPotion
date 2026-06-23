// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * USIPAbilitySystemComponent 是项目的核心技能系统组件
 * 它继承自 UAbilitySystemComponent，是 GAS（Gameplay Ability System）的核心类
 * 负责管理所有技能（Ability）的注册、激活、输入处理和生命周期
 */

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "SIPAbilitySystemComponent.generated.h"

/**
 * ASC（AbilitySystemComponent）是 GAS 的核心组件
 * 每个需要使用技能系统的角色都需要挂载一个 ASC 实例
 * 它负责：
 * 1. 存储和管理所有已注册的 Ability
 * 2. 处理输入与 Ability 的绑定
 * 3. 应用 GameplayEffect 到角色
 * 4. 管理 AttributeSet（属性集）
 */

UCLASS()
class SIP_API USIPAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:

    USIPAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /**
     * @param InputTag - 输入标签，用于匹配对应的 Ability
     * 
     * 工作原理：
     * 1. 遍历所有已注册的 Ability
     * 2. 找到 DynamicAbilityTags 中包含 InputTag 的 Ability
     * 3. 将该 Ability 的 Handle 加入待激活列表
     */
    /* 使用DynamicAbilityTag进行技能触发 */
    void AbilityInputTagPressed(const FGameplayTag& InputTag);

    /**
     * @param InputTag - 输入标签，用于匹配对应的 Ability
     */
    /* 使用DynamicAbilityTag进行技能触发 */
    void AbilityInputTagReleased(const FGameplayTag& InputTag);

    /**
     * 这是 Lyra/ShooterGame 风格的核心设计
     * 
     * 为什么要这样做？
     * 1. 避免在输入事件回调中直接激活 Ability（可能有延迟或竞态问题）
     * 2. 支持复杂输入场景：长按蓄力、预输入、多键组合
     * 3. 将输入处理集中在 Tick 中，可以批量处理输入
     * 
     * @param DeltaTime - 帧间隔时间
     * @param bGamePaused - 游戏是否暂停
     */
    // 在tick中处理输入，触发技能
    void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

protected:
    /**
     * 用于通知 Ability 输入事件（内部会触发 WaitInputPress 等节点）
     */
    virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
    
    /**
     * 用于通知 Ability 输入释放事件（内部会触发 WaitInputRelease 等节点）
     */
    virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
    
protected:
    /**
     * 然后在外部 Tick 集中处理这些输入
     * 
     * 为什么要这样设计？
     * 1. 处理复杂输入：长按蓄力、预输入、多键组合等
     * 2. 统一管理输入状态，避免输入事件回调中直接操作 Ability
     * 3. 可以在 Tick 中对输入进行预处理（方向向量归一化等）
     */

    // Handles to abilities that had their input pressed this frame.
    TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

    // Handles to abilities that had their input released this frame.
    TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

    // Handles to abilities that have their input held.
    TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};
