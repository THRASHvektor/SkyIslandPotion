// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/Components/SIPPetCliffBridgeComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "NavigationSystem.h"
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
	ScanSuppressionTimer = FMath::Max(0.0f, ScanSuppressionTimer - DeltaTime);
	PetEnergy = FMath::Clamp(PetEnergy + EnergyRegenPerSecond * DeltaTime, 0.0f, FMath::Max(MaxPetEnergy, 0.0f));

	if (!bAutoScan || ScanSuppressionTimer > 0.0f || (bBuildOnlyOnce && bHasBuiltBridge))
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
	LastScanDebug.Reset();

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
	LastScanDebug.Reset();

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

bool USIPPetCliffBridgeComponent::ForceBuildBridgeInDirection(FVector WorldDirection)
{
	LastScanDebug.Reset();

	const bool bPreviousUseUtilityDecision = bUseUtilityDecision;
	const bool bPreviousBuildOnlyOnce = bBuildOnlyOnce;
	const float PreviousCooldown = CurrentBridgeCooldown;
	const float PreviousPetEnergy = PetEnergy;
	const float PreviousMaxLandingDistance = MaxLandingDistance;
	const float PreviousTraceUpHeight = TraceUpHeight;
	const float PreviousTraceDownDepth = TraceDownDepth;
	const float PreviousMaxLandingRise = MaxLandingRise;
	const float PreviousMaxLandingDrop = MaxLandingDrop;
	const FVector PreviousNavMeshProjectionExtent = NavMeshProjectionExtent;
	const float PreviousMaxNavMeshProjection2DDistance = MaxNavMeshProjection2DDistance;
	const float PreviousMaxNavMeshProjectionZDistance = MaxNavMeshProjectionZDistance;

	bUseUtilityDecision = false;
	bBuildOnlyOnce = false;
	CurrentBridgeCooldown = 0.0f;
	PetEnergy = FMath::Max(PetEnergy, BridgeEnergyCost);
	MaxLandingDistance = FMath::Max(MaxLandingDistance, 10000.0f);
	TraceUpHeight = FMath::Max(TraceUpHeight, 5000.0f);
	TraceDownDepth = FMath::Max(TraceDownDepth, 10000.0f);
	MaxLandingRise = FMath::Max(MaxLandingRise, 3500.0f);
	MaxLandingDrop = FMath::Max(MaxLandingDrop, 1800.0f);
	NavMeshProjectionExtent.X = FMath::Max(NavMeshProjectionExtent.X, 700.0f);
	NavMeshProjectionExtent.Y = FMath::Max(NavMeshProjectionExtent.Y, 700.0f);
	NavMeshProjectionExtent.Z = FMath::Max(NavMeshProjectionExtent.Z, 3000.0f);
	MaxNavMeshProjection2DDistance = FMath::Max(MaxNavMeshProjection2DDistance, 700.0f);
	MaxNavMeshProjectionZDistance = FMath::Max(MaxNavMeshProjectionZDistance, 3000.0f);
	LastScanDebug += FString::Printf(
		TEXT("Component=%s owner=%s runtimeMax=%.0f prevMax=%.0f navRequired=%s mesh=%s\n"),
		*GetName(),
		*GetNameSafe(GetOwner()),
		MaxLandingDistance,
		PreviousMaxLandingDistance,
		bRequireNavMeshLanding ? TEXT("Y") : TEXT("N"),
		*GetNameSafe(FloatingStepMesh.Get()));

	FSIPPetBridgeCandidate Candidate;
	const bool bFound = FindBridgeCandidateInDirection(WorldDirection, Candidate);
	OnScanFinished.Broadcast(bFound);

	bool bBuilt = false;
	if (bFound)
	{
		CurrentBridgeUtilityScore = 1.0f;
		bBuilt = BuildBridge(Candidate);
	}
	else
	{
		LastDecisionReason = bRequireNavMeshLanding
			? TEXT("Force build failed: no valid navmesh landing found.")
			: TEXT("Force build failed: no valid landing found.");
	}

	bUseUtilityDecision = bPreviousUseUtilityDecision;
	bBuildOnlyOnce = bPreviousBuildOnlyOnce;
	MaxLandingDistance = PreviousMaxLandingDistance;
	TraceUpHeight = PreviousTraceUpHeight;
	TraceDownDepth = PreviousTraceDownDepth;
	MaxLandingRise = PreviousMaxLandingRise;
	MaxLandingDrop = PreviousMaxLandingDrop;
	NavMeshProjectionExtent = PreviousNavMeshProjectionExtent;
	MaxNavMeshProjection2DDistance = PreviousMaxNavMeshProjection2DDistance;
	MaxNavMeshProjectionZDistance = PreviousMaxNavMeshProjectionZDistance;
	if (!bBuilt)
	{
		CurrentBridgeCooldown = PreviousCooldown;
		PetEnergy = PreviousPetEnergy;
	}

	return bBuilt;
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

void USIPPetCliffBridgeComponent::SuppressBridgeScanForSeconds(float Duration)
{
	ScanSuppressionTimer = FMath::Max(ScanSuppressionTimer, Duration);
	ScanTimer = FMath::Max(ScanTimer, Duration);
}

bool USIPPetCliffBridgeComponent::IsBridgeActive() const
{
	for (const TObjectPtr<AStaticMeshActor>& StepActorPtr : GeneratedStepActors)
	{
		if (IsValid(StepActorPtr.Get()))
		{
			return true;
		}
	}

	return bHasBuiltBridge;
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
		LastScanDebug += TEXT("Fail: no owner or world.\n");
		return false;
	}

	WorldDirection.Z = 0.0f;
	if (!WorldDirection.Normalize())
	{
		LastDecisionReason = TEXT("Invalid scan direction.");
		LastScanDebug += TEXT("Fail: invalid scan direction.\n");
		return false;
	}

	const FVector FeetLocation = GetOwnerFeetLocation();
	LastScanDebug += FString::Printf(TEXT("Scan dir=%s feet=%s\n"), *WorldDirection.ToCompactString(), *FeetLocation.ToCompactString());

	FHitResult CurrentGroundHit;
	if (!TraceGroundAt(FeetLocation, CurrentGroundHit) || !IsGroundHitWalkable(CurrentGroundHit))
	{
		LastDecisionReason = TEXT("No walkable ground under pet.");
		LastScanDebug += TEXT("Fail: no walkable ground under pet.\n");
		return false;
	}
	LastScanDebug += FString::Printf(TEXT("Start ground=%s normal=%s\n"), *CurrentGroundHit.ImpactPoint.ToCompactString(), *CurrentGroundHit.ImpactNormal.ToCompactString());

	bool bSawGap = false;
	float GapStartDistance = EdgeProbeDistance;
	int32 SampleCount = 0;
	float FirstGapDistance = -1.0f;
	const float SearchStep = FMath::Max(10.0f, LandingSearchStep);
	for (float Distance = EdgeProbeDistance; Distance <= MaxLandingDistance; Distance += SearchStep)
	{
		++SampleCount;
		const FVector ProbeLocation = FeetLocation + WorldDirection * Distance;
		FHitResult LandingHit;
		const bool bHasGround = TraceGroundAt(ProbeLocation, LandingHit);
		const bool bHasWalkableGround = bHasGround && IsGroundHitWalkable(LandingHit);
		const bool bLandingHeightAllowed = bHasWalkableGround && IsLandingHeightAllowed(CurrentGroundHit, LandingHit);
		FVector NavLandingLocation = FVector::ZeroVector;
		const bool bLandingOnNavMesh = bLandingHeightAllowed && IsLandingOnNavMesh(LandingHit, NavLandingLocation);

		if (bDrawDebug)
		{
			const FColor DebugColor = bLandingOnNavMesh ? FColor::Green : (bHasGround ? FColor::Orange : FColor::Red);
			DrawDebugPoint(World, ProbeLocation, 12.0f, DebugColor, false, AutoScanInterval);
			if (bLandingOnNavMesh)
			{
				DrawDebugSphere(World, NavLandingLocation, 32.0f, 8, FColor::Blue, false, AutoScanInterval);
			}
		}

		if (!bSawGap && !bLandingOnNavMesh)
		{
			bSawGap = true;
			GapStartDistance = Distance;
			FirstGapDistance = Distance;
			LastScanDebug += FString::Printf(
				TEXT("Gap starts at %.0f (hasGround=%s walkable=%s heightOk=%s navOk=%s)\n"),
				Distance,
				bHasGround ? TEXT("Y") : TEXT("N"),
				bHasWalkableGround ? TEXT("Y") : TEXT("N"),
				bLandingHeightAllowed ? TEXT("Y") : TEXT("N"),
				bLandingOnNavMesh ? TEXT("Y") : TEXT("N"));
		}

		if (!bSawGap || !bLandingOnNavMesh)
		{
			continue;
		}

		OutCandidate.bFound = true;
		OutCandidate.Direction = WorldDirection;
		OutCandidate.StartLocation = CurrentGroundHit.ImpactPoint + WorldDirection * FMath::Max(EdgeProbeDistance * 0.65f, GapStartDistance - SearchStep);
		OutCandidate.LandingLocation = LandingHit.ImpactPoint;
		OutCandidate.GapDistance = FVector::Dist2D(OutCandidate.StartLocation, OutCandidate.LandingLocation);
		LastScanDebug += FString::Printf(
			TEXT("Candidate at %.0f landing=%s nav=%s samples=%d gapStart=%.0f gapDist=%.0f\n"),
			Distance,
			*LandingHit.ImpactPoint.ToCompactString(),
			*NavLandingLocation.ToCompactString(),
			SampleCount,
			FirstGapDistance,
			OutCandidate.GapDistance);

		if (bDrawDebug)
		{
			DrawDebugLine(World, OutCandidate.StartLocation, OutCandidate.LandingLocation, FColor::Cyan, false, AutoScanInterval, 0, 5.0f);
			DrawDebugSphere(World, OutCandidate.LandingLocation, 55.0f, 12, FColor::Green, false, AutoScanInterval);
		}
		LastDecisionReason = TEXT("Valid bridge candidate found.");
		return true;
	}

	LastDecisionReason = TEXT("No landing found.");
	LastScanDebug += FString::Printf(TEXT("Fail: no landing found. samples=%d sawGap=%s firstGap=%.0f maxDistance=%.0f\n"), SampleCount, bSawGap ? TEXT("Y") : TEXT("N"), FirstGapDistance, MaxLandingDistance);
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

bool USIPPetCliffBridgeComponent::IsGroundHitWalkable(const FHitResult& GroundHit) const
{
	if (!bTreatSteepSlopeAsGap)
	{
		return GroundHit.bBlockingHit;
	}

	if (!GroundHit.bBlockingHit)
	{
		return false;
	}

	const FVector Normal = GroundHit.ImpactNormal.GetSafeNormal();
	const float WalkableDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(MaxWalkableSlopeAngle, 0.0f, 89.0f)));
	return FVector::DotProduct(Normal, FVector::UpVector) >= WalkableDot;
}

bool USIPPetCliffBridgeComponent::IsLandingHeightAllowed(const FHitResult& StartGroundHit, const FHitResult& LandingHit) const
{
	if (!bLimitLandingHeightDifference)
	{
		return LandingHit.bBlockingHit;
	}

	if (!StartGroundHit.bBlockingHit || !LandingHit.bBlockingHit)
	{
		return false;
	}

	const float HeightDelta = LandingHit.ImpactPoint.Z - StartGroundHit.ImpactPoint.Z;
	return HeightDelta >= -FMath::Max(0.0f, MaxLandingDrop)
		&& HeightDelta <= FMath::Max(0.0f, MaxLandingRise);
}

bool USIPPetCliffBridgeComponent::IsLandingOnNavMesh(const FHitResult& LandingHit, FVector& OutNavLocation) const
{
	OutNavLocation = LandingHit.ImpactPoint;
	if (!bRequireNavMeshLanding)
	{
		return LandingHit.bBlockingHit;
	}

	UWorld* World = GetWorld();
	if (!World || !LandingHit.bBlockingHit)
	{
		return false;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSystem)
	{
		return false;
	}

	FNavLocation ProjectedLocation;
	if (!NavSystem->ProjectPointToNavigation(LandingHit.ImpactPoint, ProjectedLocation, NavMeshProjectionExtent))
	{
		return false;
	}

	OutNavLocation = ProjectedLocation.Location;
	const float Distance2D = FVector::Dist2D(ProjectedLocation.Location, LandingHit.ImpactPoint);
	const float DistanceZ = FMath::Abs(ProjectedLocation.Location.Z - LandingHit.ImpactPoint.Z);
	return Distance2D <= FMath::Max(0.0f, MaxNavMeshProjection2DDistance)
		&& DistanceZ <= FMath::Max(0.0f, MaxNavMeshProjectionZDistance);
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
