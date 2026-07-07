#pragma once

#include "CoreMinimal.h"
#include "World/Elemental/SIPElementReactiveZoneBase.h"
#include "SIPPlantElementalZoneActor.generated.h"

class UMaterialInterface;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS(Blueprintable)
class SIP_API ASIPPlantElementalZoneActor : public ASIPElementReactiveZoneBase
{
	GENERATED_BODY()

public:
	ASIPPlantElementalZoneActor();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Visual")
	TObjectPtr<UMaterialInterface> BurningMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Visual")
	TObjectPtr<UMaterialInterface> BurntMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Visual")
	TObjectPtr<UNiagaraSystem> BurnStartVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Visual")
	TObjectPtr<UNiagaraSystem> BurningLoopVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Visual")
	FVector BurningLoopVFXLocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Damage", meta = (ClampMin = "0.0"))
	float BurnDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Damage", meta = (ClampMin = "0.05"))
	float BurnDamageInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Burn|Damage", meta = (ClampMin = "0.0"))
	float BurnCharacterDamage = 20.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Plant|Burn")
	bool bIsBurning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Plant|Bloom")
	TObjectPtr<UNiagaraSystem> BloomVFX;

	void OnBurnReaction(const FSIPElementImpactContext& ImpactContext);
	void OnBloomReaction(const FVector& ReactionLocation);
	void StartBurning(const FSIPElementImpactContext& ImpactContext);
	void ApplyBurnDamageTick();
	void FinishBurning();
	void StopBurningVFX();

	FTimerHandle BurnDamageTimerHandle;
	FTimerHandle BurnFinishTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveBurningLoopVFX;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> BurnDamageInstigator;
};
