#pragma once

#include "CoreMinimal.h"
#include "World/Elemental/SIPElementReactiveZoneBase.h"
#include "SIPWaterElementalZoneActor.generated.h"

class ASIPCharacter;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS(Blueprintable)
class SIP_API ASIPWaterElementalZoneActor : public ASIPElementReactiveZoneBase
{
	GENERATED_BODY()

public:
	ASIPWaterElementalZoneActor();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Electrify|Visual")
	TObjectPtr<UNiagaraSystem> ElectrifyStartVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Electrify|Visual")
	TObjectPtr<UNiagaraSystem> ElectrifyLoopVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Electrify|Visual")
	FVector ElectrifyLoopVFXLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Electrify|Damage", meta = (ClampMin = "0.0"))
	float ElectrifyDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Electrify|Damage", meta = (ClampMin = "0.05"))
	float ElectrifyDamageInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Electrify|Damage", meta = (ClampMin = "0.0"))
	float ElectrifyCharacterDamage = 12.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Water|Electrify")
	bool bIsElectrified = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Freeze|Visual")
	TObjectPtr<UNiagaraSystem> FreezeStartVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Freeze|Visual")
	TObjectPtr<UNiagaraSystem> FreezeLoopVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Freeze|Visual")
	FVector FreezeLoopVFXLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Freeze|Slow", meta = (ClampMin = "0.0"))
	float FreezeDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Water|Freeze|Slow")
	TSubclassOf<UGameplayEffect> FreezeSlowGameplayEffectClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Water|Freeze")
	bool bIsFrozen = false;

	void OnElectrifyReaction(const FSIPElementImpactContext& ImpactContext);
	void OnFreezeReaction(const FSIPElementImpactContext& ImpactContext);
	void StartElectrify(const FSIPElementImpactContext& ImpactContext);
	void ApplyElectrifyDamageTick();
	void FinishElectrify();
	void StartFreeze(const FSIPElementImpactContext& ImpactContext);
	int32 ApplyFreezeSlowEffectToOverlappingCharacters(AActor* EffectInstigator);
	void FinishFreeze();
	void StopLoopingVFX(TObjectPtr<UNiagaraComponent>& VFXComponent);

	FTimerHandle ElectrifyDamageTimerHandle;
	FTimerHandle ElectrifyFinishTimerHandle;
	FTimerHandle FreezeFinishTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveElectrifyLoopVFX;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveFreezeLoopVFX;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ElectrifyDamageInstigator;
};
