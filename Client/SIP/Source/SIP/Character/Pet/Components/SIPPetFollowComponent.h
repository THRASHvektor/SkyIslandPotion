// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SIPPetFollowComponent.generated.h"

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPPetFollowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPPetFollowComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Follow")
	void SetFollowTarget(AActor* NewTarget);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Follow")
	bool bAutoFindPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Follow")
	bool bUseDirectMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Follow", meta = (ClampMin = "0.0"))
	float DesiredDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Follow", meta = (ClampMin = "0.0"))
	float StopDistance = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Follow", meta = (ClampMin = "0.0"))
	float MoveSpeed = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Follow", meta = (ClampMin = "0.0"))
	float TeleportDistance = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Follow")
	FVector FollowOffset = FVector(-140.0f, 90.0f, 0.0f);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Follow")
	TObjectPtr<AActor> FollowTarget;

private:
	FVector ResolveGoalLocation() const;
};
