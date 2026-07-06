#include "SIPPetAIController.h"

#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

ASIPPetAIController::ASIPPetAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASIPPetAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bAutoFollowPlayer)
	{
		return;
	}

	APawn* ControlledPet = GetPawn();
	if (!ControlledPet)
	{
		return;
	}

	if (!CachedPlayerPawn)
	{
		CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	if (!CachedPlayerPawn || CachedPlayerPawn == ControlledPet)
	{
		return;
	}

	RepathTimer -= DeltaSeconds;
	if (RepathTimer > 0.0f)
	{
		return;
	}
	RepathTimer = RepathInterval;

	const float Distance = FVector::Dist2D(ControlledPet->GetActorLocation(), CachedPlayerPawn->GetActorLocation());
	if (Distance > FollowStartDistance)
	{
		MoveToActor(CachedPlayerPawn, FollowStopDistance, true, true, true, nullptr, true);

		if (bTeleportWhenStranded && Distance >= StrandedTeleportDistance)
		{
			StrandedTimer += RepathInterval;
			if (StrandedTimer >= StrandedTeleportDelay && TryTeleportNearPlayer(ControlledPet))
			{
				StrandedTimer = 0.0f;
				StopMovement();
			}
		}
	}
	else if (Distance <= FollowStopDistance)
	{
		StrandedTimer = 0.0f;
		StopMovement();
	}
	else
	{
		StrandedTimer = 0.0f;
	}
}

void ASIPPetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	RepathTimer = 0.0f;
	StrandedTimer = 0.0f;
}

bool ASIPPetAIController::TryTeleportNearPlayer(APawn* ControlledPet) const
{
	if (!ControlledPet || !CachedPlayerPawn)
	{
		return false;
	}

	const FVector DesiredLocation = CachedPlayerPawn->GetActorLocation()
		+ CachedPlayerPawn->GetActorForwardVector() * TeleportOffsetFromPlayer.X
		+ CachedPlayerPawn->GetActorRightVector() * TeleportOffsetFromPlayer.Y
		+ FVector(0.0f, 0.0f, TeleportOffsetFromPlayer.Z);

	FVector TeleportLocation = DesiredLocation;
	if (const UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(350.0f, 350.0f, 500.0f)))
		{
			TeleportLocation = ProjectedLocation.Location + FVector(0.0f, 0.0f, 60.0f);
		}
	}

	return ControlledPet->SetActorLocation(TeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);
}
