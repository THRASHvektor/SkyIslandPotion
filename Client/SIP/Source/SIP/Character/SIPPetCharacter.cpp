#include "SIPPetCharacter.h"

#include "AI/SIPPetAIController.h"

ASIPPetCharacter::ASIPPetCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AIControllerClass = ASIPPetAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}
