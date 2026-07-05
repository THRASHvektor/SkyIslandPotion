#include "World/Encounter/SIPEncounterPCGActor.h"

#include "Components/SceneComponent.h"
#include "World/Encounter/SIPEncounterPCGComponent.h"

ASIPEncounterPCGActor::ASIPEncounterPCGActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EncounterPCGComponent = CreateDefaultSubobject<USIPEncounterPCGComponent>(TEXT("EncounterPCGComponent"));
}
