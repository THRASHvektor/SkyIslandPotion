#include "SIPPetAIController.h"

ASIPPetAIController::ASIPPetAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASIPPetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

}
