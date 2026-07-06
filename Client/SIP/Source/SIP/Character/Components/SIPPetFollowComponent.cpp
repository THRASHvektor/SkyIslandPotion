// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Components/SIPPetFollowComponent.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

USIPPetFollowComponent::USIPPetFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.03f;
}

void USIPPetFollowComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoFindPlayer && !FollowTarget)
	{
		FollowTarget = UGameplayStatics::GetPlayerPawn(this, 0);
	}
}

void USIPPetFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (bAutoFindPlayer && !FollowTarget)
	{
		FollowTarget = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	if (!FollowTarget || !bUseDirectMovement)
	{
		return;
	}

	const FVector GoalLocation = ResolveGoalLocation();
	const FVector CurrentLocation = Owner->GetActorLocation();
	const float Distance = FVector::Dist2D(CurrentLocation, GoalLocation);

	if (Distance > TeleportDistance)
	{
		Owner->SetActorLocation(GoalLocation, false);
		return;
	}

	if (Distance <= StopDistance)
	{
		return;
	}

	FVector Direction = GoalLocation - CurrentLocation;
	Direction.Z = 0.0f;
	if (!Direction.Normalize())
	{
		return;
	}

	const FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
	Owner->SetActorLocation(NewLocation, true);
	Owner->SetActorRotation(Direction.Rotation());
}

void USIPPetFollowComponent::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;
}

FVector USIPPetFollowComponent::ResolveGoalLocation() const
{
	if (!FollowTarget)
	{
		return FVector::ZeroVector;
	}

	return FollowTarget->GetActorLocation()
		+ FollowTarget->GetActorForwardVector() * FollowOffset.X
		+ FollowTarget->GetActorRightVector() * FollowOffset.Y
		+ FVector(0.0f, 0.0f, FollowOffset.Z);
}
