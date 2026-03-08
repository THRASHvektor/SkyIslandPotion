#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SIPGameplayAbility_HealPotion.generated.h"

class ASIPCharacter;

UCLASS()
class SIP_API USIPGameplayAbility_HealPotion : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USIPGameplayAbility_HealPotion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Potion")
	float HealAmount = 35.0f;
};