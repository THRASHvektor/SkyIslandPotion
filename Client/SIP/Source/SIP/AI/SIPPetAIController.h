#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SIPPetAIController.generated.h"

UCLASS()
class SIP_API ASIPPetAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASIPPetAIController();

protected:
	// 当 AI 附身到宠物身上时触发
	virtual void OnPossess(APawn* InPawn) override;
};
