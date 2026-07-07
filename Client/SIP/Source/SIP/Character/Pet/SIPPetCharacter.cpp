#include "SIPPetCharacter.h"

#include "Character/Pet/AI/SIPPetAIController.h"
#include "Character/Pet/Components/SIPPetMoodComponent.h"

ASIPPetCharacter::ASIPPetCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ASIPPetAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	MoodComponent = CreateDefaultSubobject<USIPPetMoodComponent>(TEXT("PetMoodComponent"));
}
