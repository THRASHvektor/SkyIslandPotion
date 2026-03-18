#pragma once

#include "CoreMinimal.h"
#include "Character/SIPCharacter.h" 
#include "SIPPetCharacter.generated.h"

UENUM(BlueprintType)
enum class EPetState : uint8
{
	Wild		UMETA(DisplayName = "Wild"),
	Companion	UMETA(DisplayName = "Companion"),
	Worker		UMETA(DisplayName = "Worker")
};

UCLASS()
class SIP_API ASIPPetCharacter : public ASIPCharacter
{
	GENERATED_BODY()
	
public:
	ASIPPetCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pet AI")
	EPetState CurrentPetState = EPetState::Wild; 
};
