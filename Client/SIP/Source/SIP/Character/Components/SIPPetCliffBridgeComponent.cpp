// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Components/SIPPetCliffBridgeComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"

USIPPetCliffBridgeComponent::USIPPetCliffBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		FloatingStepMesh = CubeMeshAsset.Object;
	}
}

void USIPPetCliffBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	ScanTimer = FMath::FRandRange(0.0f, FMath::Max(0.1f, AutoScanInterval));
}

void USIPPetCliffBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearGeneratedBridge();
	Super::EndPlay(EndPlayReason);
}

void USIPPetCliffBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CurrentBridgeCooldown = FMath::Max(0.0f, CurrentBridgeCooldown - DeltaTime);
	PetEnergy = FMath::Clamp(PetEnergy + EnergyRegenPerSecond * DeltaTime, 0.0f, FMath::Max(MaxPetEnergy, 0.0f));

	if (!bAutoScan || (bBuildOnlyOnce && bHasBuiltBridge))
	{
		return;
	}

	ScanTimer -= DeltaTime;
	if (ScanTimer > 0.0f)
	{
		return;
	}

	ScanTimer = FMath::Max(0.1f, AutoScanInterval);
	ScanAndBuildBridge();
}

bool USIPPetCliffBridgeComponent::ScanAndBuildBridge()
{
	FSIPPetBridgeCandidate Candidate;
	const bool bFound = FindBridgeCandidate(Candidate);
	OnScanFinished.Broadcast(bFound);

	if (!bFound)
	{
		return false;
	}

	CurrentBridgeUtilityScore = CalculateBridgeUtilityScore(Candidate);
	if (bUseUtilityDecision && CurrentBridgeUtilityScore < MinBridgeUtilityScore)
	{
		LastDecisionReason = FString::Printf(TEXT("Utility score too low: %.2f < %.2f"), CurrentBridgeUtilityScore, MinBridgeUtilityScore);
		return false;
	}

	return BuildBridge(Candidate);
}

bool USIPPetCliffBridgeComponent::ScanAndBuildBridgeInDirection(FVector WorldDirection)
{
	FSIPPetBridgeCandidate Candidate;
	const bool bFound = FindBridgeCandidateInDirection(WorldDirection, Candidate);
	OnScanFinished.Broadcast(bFound);

	if (!bFound)
	{
		return false;
	}

	CurrentBridgeUtilityScore = CalculateBridgeUtilityScore(Candidate);
	if (bUseUtilityDecision && CurrentBridgeUtilityScore < MinBridgeUtilityScore)
	{
		LastDecisionReason = FString::Printf(TEXT("Utility score too low: %.2f < %.2f"), CurrentBridgeUtilityScore, MinBridgeUtilityScore);
		return false;
	}

	return BuildBridge(Candidate);
}

void USIPPetCliffBridgeComponent::ClearGeneratedBridge()
{
	for (const TObjectPtr<AStaticMeshActor>& StepActorPtr : GeneratedStepActors)
	{
		AStaticMeshActor* StepActor = StepActorPtr.Get();
		if (IsValid(StepActor))
		{
			StepActor->Destroy();
		}
	}

	GeneratedStepActors.Empty();
	bHasBuiltBridge = false;
}

bool USIPPetCliffBridgeComponent::FindBridgeCandidate(FSIPPetBridgeCandidate& OutCandidate)
{
	if (ScanMode == ESIPPetBridgeScanMode::OwnerForward)
	{
		const AActor* Owner = GetOwner();
		return Owner && FindBridgeCandidateInDirection(Owner->GetActorForwardVector(), OutCandidate);
	}

	const int32 DirectionCount = FMath::Max(RadialScanDirections, 1);
	float BestDistance = TNumericLimits<float>::Max();
	FSIPPetBridgeCandidate BestCandidate;

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const float BaseYaw = Owner->GetActorRotation().Yaw;
	for (int32 Index = 0; Index < DirectionCount; ++Index)
	{
		const float Yaw = BaseYaw + (360.0f / DirectionCount) * Index;
		const FVector Direction = FRotator(0.0f, Yaw, 0.0f).Vector();

		FSIPPetBridgeCandidate Candidate;
		if (!FindBridgeCandidateInDirection(Direction, Candidate))
		{
			continue;
		}

		if (Candidate.GapDistance < BestDistance)
		{
			BestDistance = Candidate.GapDistance;
			BestCandidate = Candidate;
		}
	}

	OutCandidate = BestCandidate;
	return OutCandidate.bFound;
}

bool USIPPetCliffBridgeComponent::FindBridgeCandidateInDirection(FVector WorldDirection, FSIPPetBridgeCandidate& OutCandidate)
{
	OutCandidate = FSIPPetBridgeCandidate();

	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		LastDecisionReason = TEXT("No owner or world.");
		return false;
	}

	WorldDirection.Z = 0.0f;
	if (!WorldDirection.Normalize())
	{
		LastDecisionReason = TEXT("Invalid scan direction.");
		return false;
	}

	const FVector FeetLocation = GetOwnerFeetLocation();

	FHitResult CurrentGroundHit;
	if (!TraceGroundAt(FeetLocation, CurrentGroundHit))
	{
		LastDecisionReason = TEXT("No ground under pet.");
		return false;
	}

	FHitResult EdgeProbeHit;
	const FVector EdgeProbeLocation = FeetLocation + WorldDirection * EdgeProbeDistance;
	if (TraceGroundAt(EdgeProbeLocation, EdgeProbeHit))
	{
		if (bDrawDebug)
		{
			DrawDebugLine(World, FeetLocation, EdgeProbeLocation, FColor::Yellow, false, AutoScanInterval, 0, 2.0f);
		}
		LastDecisionReason = TEXT("Edge probe still has ground.");
		return false;
	}

	bool bSawGap = true;
	for (float Distance = MinLandingDistance; Distance <= MaxLandingDistance; Distance += FMath::Max(10.0f, LandingSearchStep))
	{
		const FVector ProbeLocation = FeetLocation + WorldDirection * Distance;
		FHitResult LandingHit;
		const bool bHasGround = TraceGroundAt(ProbeLocation, LandingHit);

		if (bDrawDebug)
		{
			const FColor DebugColor = bHasGround ? FColor::Green : FColor::Red;
			DrawDebugPoint(World, ProbeLocation, 12.0f, DebugColor, false, AutoScanInterval);
		}

		if (!bSawGap || !bHasGround)
		{
			continue;
		}

		OutCandidate.bFound = true;
		OutCandidate.Direction = WorldDirection;
		OutCandidate.StartLocation = CurrentGroundHit.ImpactPoint + WorldDirection * EdgeProbeDistance * 0.65f;
		OutCandidate.LandingLocation = LandingHit.ImpactPoint;
		OutCandidate.GapDistance = FVector::Dist2D(OutCandidate.StartLocation, OutCandidate.LandingLocation);

		if (bDrawDebug)
		{
			DrawDebugLine(World, OutCandidate.StartLocation, OutCandidate.LandingLocation, FColor::Cyan, false, AutoScanInterval, 0, 5.0f);
			DrawDebugSphere(World, OutCandidate.LandingLocation, 55.0f, 12, FColor::Green, false, AutoScanInterval);
		}
		LastDecisionReason = TEXT("Valid bridge candidate found.");
		return true;
	}

	LastDecisionReason = TEXT("No landing found.");
	return false;
}

bool USIPPetCliffBridgeComponent::BuildBridge(const FSIPPetBridgeCandidate& Candidate)
{
	if (!Candidate.bFound || (bBuildOnlyOnce && bHasBuiltBridge))
	{
		LastDecisionReason = TEXT("Build blocked: no candidate or bridge already built.");
		return false;
	}

	if (CurrentBridgeCooldown > 0.0f)
	{
		LastDecisionReason = FString::Printf(TEXT("Build blocked: cooldown %.1fs."), CurrentBridgeCooldown);
		return false;
	}

	if (PetEnergy < BridgeEnergyCost)
	{
		LastDecisionReason = FString::Printf(TEXT("Build blocked: energy %.0f / %.0f."), PetEnergy, BridgeEnergyCost);
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || !FloatingStepMesh)
	{
		LastDecisionReason = TEXT("Build blocked: missing world or step mesh.");
		return false;
	}

	if (bClearPreviousBridgeBeforeBuild)
	{
		ClearGeneratedBridge();
	}

	const float BridgeDistance = FVector::Dist(Candidate.StartLocation, Candidate.LandingLocation);
	const float ResolvedStepSpacing = CalculateStepSpacing();
	const int32 StepCount = FMath::Clamp(
		FMath::CeilToInt(BridgeDistance / FMath::Max(50.0f, ResolvedStepSpacing)) + 1,
		2,
		FMath::Max(2, MaxStepCount));
	const FRotator StepRotation = Candidate.Direction.Rotation();

	for (int32 Index = 0; Index < StepCount; ++Index)
	{
		const float Alpha = StepCount > 1 ? static_cast<float>(Index) / static_cast<float>(StepCount - 1) : 0.0f;
		FVector StepLocation = FMath::Lerp(Candidate.StartLocation, Candidate.LandingLocation, Alpha);
		StepLocation.Z += StepZOffset + FMath::Sin(Alpha * PI) * StepArcHeight;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = GetOwner();

		AStaticMeshActor* StepActor = World->SpawnActor<AStaticMeshActor>(StepLocation, StepRotation, SpawnParams);
		if (!StepActor)
		{
			continue;
		}

		UStaticMeshComponent* MeshComponent = StepActor->GetStaticMeshComponent();
		if (MeshComponent)
		{
			MeshComponent->SetMobility(EComponentMobility::Movable);
			MeshComponent->SetStaticMesh(FloatingStepMesh);
			MeshComponent->SetWorldScale3D(StepScale);
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

			if (FloatingStepMaterial)
			{
				MeshComponent->SetMaterial(0, FloatingStepMaterial);
			}
		}

		StepActor->Tags.Add(TEXT("SIPPetGeneratedBridge"));
		GeneratedStepActors.Add(StepActor);

		if (bDrawDebug)
		{
			DrawDebugSphere(World, StepLocation, 28.0f, 8, FColor::Cyan, false, 3.0f);
		}
	}

	bHasBuiltBridge = GeneratedStepActors.Num() > 0;
	if (bHasBuiltBridge)
	{
		PetEnergy = FMath::Max(0.0f, PetEnergy - BridgeEnergyCost);
		CurrentBridgeCooldown = BridgeCooldown;
		LastDecisionReason = FString::Printf(TEXT("Bridge built. Score %.2f, Energy %.0f."), CurrentBridgeUtilityScore, PetEnergy);
		OnBridgeBuilt.Broadcast(Candidate);
	}

	return bHasBuiltBridge;
}

float USIPPetCliffBridgeComponent::CalculateBridgeUtilityScore(const FSIPPetBridgeCandidate& Candidate) const
{
	if (!Candidate.bFound)
	{
		return 0.0f;
	}

	const float GapAlpha = FMath::Clamp(
		(Candidate.GapDistance - MinLandingDistance) / FMath::Max(1.0f, MaxLandingDistance - MinLandingDistance),
		0.0f,
		1.0f);
	const float EnergyAlpha = BridgeEnergyCost > 0.0f ? FMath::Clamp(PetEnergy / BridgeEnergyCost, 0.0f, 1.0f) : 1.0f;
	const float CooldownAlpha = BridgeCooldown > 0.0f ? 1.0f - FMath::Clamp(CurrentBridgeCooldown / BridgeCooldown, 0.0f, 1.0f) : 1.0f;

	const float Score =
		PersonalityBridgeBias * 0.42f +
		PersonalityCuriosity * 0.20f +
		PersonalityProtectiveness * 0.16f +
		GapAlpha * 0.12f +
		EnergyAlpha * 0.07f +
		CooldownAlpha * 0.03f;

	return FMath::Clamp(Score, 0.0f, 1.0f);
}

float USIPPetCliffBridgeComponent::CalculateStepSpacing() const
{
	if (!bAutoFitStepSpacing || !FloatingStepMesh)
	{
		return StepSpacing;
	}

	const FBoxSphereBounds MeshBounds = FloatingStepMesh->GetBounds();
	const float MeshLengthX = MeshBounds.BoxExtent.X * 2.0f * FMath::Max(0.01f, StepScale.X);
	if (MeshLengthX <= KINDA_SMALL_NUMBER)
	{
		return StepSpacing;
	}

	const float OverlapMultiplier = 1.0f - FMath::Clamp(StepOverlapRatio, 0.0f, 0.8f);
	return FMath::Max(50.0f, MeshLengthX * OverlapMultiplier);
}

bool USIPPetCliffBridgeComponent::TraceGroundAt(const FVector& WorldLocation, FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World)
	{
		return false;
	}

	const FVector TraceStart = WorldLocation + FVector(0.0f, 0.0f, TraceUpHeight);
	const FVector TraceEnd = WorldLocation - FVector(0.0f, 0.0f, TraceDownDepth);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SIPPetCliffBridgeGroundTrace), false);
	QueryParams.AddIgnoredActor(Owner);

	const bool bHit = World->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, GroundTraceChannel, QueryParams);
	if (bDrawDebug)
	{
		DrawDebugLine(World, TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, AutoScanInterval, 0, 1.5f);
	}

	return bHit;
}

FVector USIPPetCliffBridgeComponent::GetOwnerFeetLocation() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return FVector::ZeroVector;
	}

	const FVector Origin = Owner->GetActorLocation();
	const FVector Extent = Owner->GetComponentsBoundingBox(true).GetExtent();
	return Origin - FVector(0.0f, 0.0f, Extent.Z);
}
