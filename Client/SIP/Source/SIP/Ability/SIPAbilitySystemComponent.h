// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "SIPAbilitySystemComponent.generated.h"

UCLASS()
class SIP_API USIPAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:

    USIPAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /* 使用DynamicAbilityTag进行技能触发 */
    void AbilityInputTagPressed(const FGameplayTag& InputTag);

    /* 使用DynamicAbilityTag进行技能触发 */
    void AbilityInputTagReleased(const FGameplayTag& InputTag);

    // 在tick中处理输入，触发技能
    void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

protected:
    virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
    virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
    
protected:
    /* 参考lyra做法，将每帧的输入转化成abilityspec进行缓存，然后再在外部tick集中处理这些输入，用于处理复杂输入，包括长按蓄力、预输入等 */

    // Handles to abilities that had their input pressed this frame.
    TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

    // Handles to abilities that had their input released this frame.
    TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

    // Handles to abilities that have their input held.
    TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};