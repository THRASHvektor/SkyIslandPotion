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

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow")
	bool bAutoFollowPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow", meta = (ClampMin = "0.0"))
	float FollowStartDistance = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow", meta = (ClampMin = "0.0"))
	float FollowStopDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow", meta = (ClampMin = "0.05"))
	float RepathInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow")
	bool bTeleportWhenStranded = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow", meta = (ClampMin = "0.0"))
	float StrandedTeleportDistance = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow", meta = (ClampMin = "0.0"))
	float StrandedTeleportDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet AI|Follow")
	FVector TeleportOffsetFromPlayer = FVector(-180.0f, 100.0f, 80.0f);

protected:
	// 当 AI 附身到宠物身上时触发
	virtual void OnPossess(APawn* InPawn) override;

private:
	bool TryTeleportNearPlayer(APawn* ControlledPet) const;

	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedPlayerPawn;

	float RepathTimer = 0.0f;
	float StrandedTimer = 0.0f;
};
