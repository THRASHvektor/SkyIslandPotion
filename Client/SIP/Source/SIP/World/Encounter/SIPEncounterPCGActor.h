#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPEncounterPCGActor.generated.h"

class USceneComponent;
class USIPEncounterPCGComponent;

UCLASS(Blueprintable)
class SIP_API ASIPEncounterPCGActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPEncounterPCGActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Encounter PCG")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Encounter PCG")
	TObjectPtr<USIPEncounterPCGComponent> EncounterPCGComponent;
};
