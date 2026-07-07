#include "SIPPetAIController.h"

#include "Character/Pet/Components/SIPPetCliffBridgeComponent.h"
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

		if (ShouldDelayTeleportForActiveBridge(ControlledPet))
		{
			StrandedTimer = 0.0f;
			return;
		}

		if (bTeleportWhenStranded && Distance >= StrandedTeleportDistance)
		{
			StrandedTimer += RepathInterval;
			if (StrandedTimer >= StrandedTeleportDelay && TryTeleportNearPlayer(ControlledPet))
			{
				StrandedTimer = 0.0f;
				StopMovement();
				SuppressBridgeScanAfterTeleport(ControlledPet);
			}
		}
	}
	else if (Distance <= FollowStopDistance)
	{
		StrandedTimer = 0.0f;
		BridgeActiveGraceTimer = 0.0f;
		StopMovement();
	}
	else
	{
		StrandedTimer = 0.0f;
		BridgeActiveGraceTimer = 0.0f;
	}
}

void ASIPPetAIController::ApplyPersonalityTuning(const FSIPPetBehaviourTuning& NewTuning)
{
	ActivePersonalityTuning = NewTuning;
	FollowStartDistance = NewTuning.FollowStartDistance;
	FollowStopDistance = NewTuning.FollowStopDistance;
	StrandedTeleportDistance = NewTuning.StrandedTeleportDistance;
	StrandedTeleportDelay = NewTuning.StrandedTeleportDelay;
	RepathInterval = NewTuning.MoveRepathInterval;
	RepathTimer = 0.0f;
	StrandedTimer = 0.0f;
	BridgeActiveGraceTimer = 0.0f;
}

bool ASIPPetAIController::ShouldDelayTeleportForActiveBridge(APawn* ControlledPet)
{
	if (!ControlledPet)
	{
		return false;
	}

	const USIPPetCliffBridgeComponent* BridgeComponent = ControlledPet->FindComponentByClass<USIPPetCliffBridgeComponent>();
	if (!BridgeComponent || !BridgeComponent->IsBridgeActive())
	{
		BridgeActiveGraceTimer = 0.0f;
		return false;
	}

	BridgeActiveGraceTimer += RepathInterval;
	return BridgeActiveGraceTimer < BridgeActiveTeleportGrace;
}

void ASIPPetAIController::SuppressBridgeScanAfterTeleport(APawn* ControlledPet) const
{
	if (!ControlledPet || BridgeScanSuppressAfterTeleport <= 0.0f)
	{
		return;
	}

	if (USIPPetCliffBridgeComponent* BridgeComponent = ControlledPet->FindComponentByClass<USIPPetCliffBridgeComponent>())
	{
		BridgeComponent->SuppressBridgeScanForSeconds(BridgeScanSuppressAfterTeleport);
	}
}

void ASIPPetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	RepathTimer = 0.0f;
	StrandedTimer = 0.0f;

	if (USIPPetPersonalityComponent* PersonalityComponent = InPawn ? InPawn->FindComponentByClass<USIPPetPersonalityComponent>() : nullptr)
	{
		ApplyPersonalityTuning(PersonalityComponent->BehaviourTuning);
	}
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
