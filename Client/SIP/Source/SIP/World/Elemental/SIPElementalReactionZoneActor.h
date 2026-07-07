#pragma once

#include "CoreMinimal.h"
#include "World/Elemental/SIPElementReactiveZoneBase.h"
#include "SIPElementalReactionZoneActor.generated.h"

class UPCGComponent;

UCLASS(Blueprintable)
class SIP_API ASIPElementalReactionZoneActor : public ASIPElementReactiveZoneBase
{
	GENERATED_BODY()

public:
	ASIPElementalReactionZoneActor();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Zone")
	TArray<TObjectPtr<AActor>> LinkedDefaultActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Zone|PCG")
	TObjectPtr<UPCGComponent> PCGComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Zone|PCG")
	bool bAutoGenerateVegetation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Zone|PCG")
	int32 VegetationSeed = 42;

	UFUNCTION(BlueprintCallable, Category = "SIP|Zone|PCG")
	void GenerateVegetation(int32 Seed = 0);

protected:
	virtual void BeginPlay() override;
	virtual void OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation) override;

	void ApplyReaction(const FGameplayTag& ReactionTag, const FVector& ImpactLocation);
	void HideDefaultActors();
	void ClearPCGContent();
};
