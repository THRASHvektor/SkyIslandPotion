#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SIPGameplayAbility_Dash.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
class USceneComponent;

UCLASS()
class SIP_API USIPGameplayAbility_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USIPGameplayAbility_Dash(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

private:
	FVector CalculateDashDirection() const;
	bool PerformDash(const FVector& DashDirection);

public:
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashCooldown = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	TSubclassOf<UGameplayEffect> DashCooldownEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|VFX")
	UNiagaraSystem* DashTrailEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|VFX")
	UNiagaraSystem* DashLandedEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	bool bCheckCollision = true;
};
