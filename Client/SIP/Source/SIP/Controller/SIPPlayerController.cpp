#include "SIPPlayerController.h"
#include "Character/SIPCharacter.h"
#include "Ability/SIPAbilitySystemComponent.h"

ASIPPlayerController::ASIPPlayerController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ASIPPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
    Super::PostProcessInput(DeltaTime, bGamePaused);

    if (ASIPCharacter* character = Cast<ASIPCharacter>(GetPawn()))
    {
        if (USIPAbilitySystemComponent* ASC = character->GetSIPAbilitySystemComponent())
        {
            ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
        }
    }
}
