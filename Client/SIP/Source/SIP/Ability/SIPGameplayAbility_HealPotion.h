#pragma once

#include "CoreMinimal.h"
#include "SIPGameplayAbility.h"
#include "SIPGameplayAbility_HealPotion.generated.h"

class ASIPCharacter;

/**
 * 立即生效的治疗药水能力。
 * 其中 GAS 仍然负责激活、阻塞和生命周期管理，而实际治疗会在 Commit 成功后直接作用到角色身上。
 */
UCLASS()
class SIP_API USIPGameplayAbility_HealPotion : public USIPGameplayAbility
{
	GENERATED_BODY()

public:
	// 将治疗药水配置为一次性实例化能力。
	USIPGameplayAbility_HealPotion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// 只有角色存在、存活且当前血量未满时才允许使用治疗药水。
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	// 在 Commit 成功后立刻回复生命值，然后结束这次一次性能力。
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 成功喝下药水后恢复的生命值数额。
	UPROPERTY(EditDefaultsOnly, Category = "Potion")
	float HealAmount = 35.0f;
};
