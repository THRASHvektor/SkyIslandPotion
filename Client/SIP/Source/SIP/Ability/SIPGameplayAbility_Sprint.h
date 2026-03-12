#pragma once

#include "CoreMinimal.h"
#include "SIPGameplayAbility.h"
#include "SIPGameplayAbility_Sprint.generated.h"

/**
 * USIPGameplayAbility_Sprint
 *
 * 冲刺技能：按下时应用加速效果，松开时移除
 */
UCLASS()
class SIP_API USIPGameplayAbility_Sprint : public USIPGameplayAbility
{
	GENERATED_BODY()

public:
	USIPGameplayAbility_Sprint(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// 激活技能时调用（按键按下）
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// 取消技能时调用（按键松开）
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 检查是否可以激活
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	// 输入松开时调用，用于结束持续型技能
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	// 冲刺时应用的GameplayEffect（加速效果）
	UPROPERTY(EditDefaultsOnly, Category = "Sprint")
	TSubclassOf<UGameplayEffect> SprintEffectClass;

	// 激活时创建的EffectHandle，用于后续移除
	FActiveGameplayEffectHandle SprintEffectHandle;
};
