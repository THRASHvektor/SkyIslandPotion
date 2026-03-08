#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SIPGameplayAbility_Attack.generated.h"

class ASIPCharacter;
class UAnimMontage;

UCLASS()
class SIP_API USIPGameplayAbility_Attack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USIPGameplayAbility_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	TArray<ASIPCharacter*> CollectTargets(ASIPCharacter* SourceCharacter) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float DamageAmount = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRange = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;
};