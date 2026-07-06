#pragma once

#include "CoreMinimal.h"
#include "World/Elemental/SIPElementReactiveZoneBase.h"
#include "SIPIceElementalZoneActor.generated.h"

class UNiagaraSystem;

UCLASS(Blueprintable)
class SIP_API ASIPIceElementalZoneActor : public ASIPElementReactiveZoneBase
{
	GENERATED_BODY()

public:
	ASIPIceElementalZoneActor();

protected:
	virtual void OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Ice")
	TObjectPtr<UNiagaraSystem> MeltVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Ice")
	bool bHideVisualActorOnMelt = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Ice")
	bool bDisableZoneCollisionOnMelt = true;

	void OnMeltReaction(const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation);
};
