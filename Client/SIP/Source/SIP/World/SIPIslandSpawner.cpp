// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPIslandSpawner.h"

#include "Components/BoxComponent.h"
#include "SIPLogCategory.h"
#include "World/SIPElementalZoneActor.h"
#include "World/SIPIslandActor.h"

// Island spawning is done entirely on demand, so no per-frame Tick is required.
ASIPIslandSpawner::ASIPIslandSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Auto-spawn the island layout when the level starts running.
void ASIPIslandSpawner::BeginPlay()
{
	Super::BeginPlay();
	SpawnAllIslands();
}

// Clear any old layout, then place each island using repeated candidate sampling and scoring.
void ASIPIslandSpawner::SpawnAllIslands()
{
	if (!IslandClass)
	{
		UE_LOG(LogSIP, Error,
			TEXT("IslandSpawner[%s]: IslandClass is not set! Please assign BP_IslandActor (or a biome subclass)."),
			*GetName());
		return;
	}

	if (BiomeWeights.IsEmpty())
	{
		UE_LOG(LogSIP, Warning,
			TEXT("IslandSpawner[%s]: BiomeWeights is empty. All islands will use the default graph."),
			*GetName());
	}

	ClearAllIslands();

	FRandomStream RandStream(WorldSeed);
	const FVector Origin = GetActorLocation();
	int32 SpawnedCount = 0;
	const int32 MaxPlacementPassesPerIsland = 4;
	int32 FailedPlacements = 0;

	for (int32 IslandIndex = 0; IslandIndex < IslandCount; ++IslandIndex)
	{
		const FGameplayTag ChosenBiome = PickRandomBiome(RandStream);
		const float CandidateRadius = GetCandidatePlacementRadius(ChosenBiome);
		const float CandidateHalfHeight = GetCandidatePlacementHalfHeight(ChosenBiome);
		const int32 CandidateBatchSize = FMath::Max(LayoutCandidateCount, 1);

		bool bFoundCandidate = false;
		float BestScore = -BIG_NUMBER;
		FVector BestLocation = FVector::ZeroVector;
		TArray<ASIPIslandActor*> BestNearbyIslands;

		for (int32 PassIndex = 0; PassIndex < MaxPlacementPassesPerIsland; ++PassIndex)
		{
			for (int32 CandidateIndex = 0; CandidateIndex < CandidateBatchSize; ++CandidateIndex)
			{
				const FVector CandidateLoc = SampleCandidateLocation(RandStream, Origin, ChosenBiome);

				TArray<ASIPIslandActor*> NearbyIslands;
				if (!IsLocationValid(CandidateLoc, CandidateRadius, CandidateHalfHeight, NearbyIslands))
				{
					continue;
				}

				const float CandidateScore = ScoreCandidateLocation(
					CandidateLoc,
					ChosenBiome,
					CandidateRadius,
					CandidateHalfHeight,
					NearbyIslands.Num());

				if (!bFoundCandidate || CandidateScore > BestScore)
				{
					bFoundCandidate = true;
					BestScore = CandidateScore;
					BestLocation = CandidateLoc;
					BestNearbyIslands = NearbyIslands;
				}
			}
		}

		if (!bFoundCandidate)
		{
			++FailedPlacements;
			UE_LOG(LogSIP, Warning,
				TEXT("IslandSpawner: Failed to place island %d / %d for biome [%s]."),
				IslandIndex + 1, IslandCount, *ChosenBiome.ToString());
			continue;
		}

		const float YawRot = RandStream.FRandRange(0.f, 360.f);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ASIPIslandActor* Island = GetWorld()->SpawnActor<ASIPIslandActor>(
			IslandClass,
			BestLocation,
			FRotator(0.f, YawRot, 0.f),
			Params);

		if (!Island)
		{
			UE_LOG(LogSIP, Warning, TEXT("IslandSpawner: SpawnActor failed for island index %d."), IslandIndex);
			continue;
		}

		const int32 IslandSeed = WorldSeed + SpawnedCount * 1000;
		Island->InitializeIsland(ChosenBiome, SpawnedCount, WorldSeed);

		for (ASIPIslandActor* NearbyIsland : BestNearbyIslands)
		{
			if (!IsValid(NearbyIsland))
			{
				continue;
			}

			Island->RegisterNearbyIsland(NearbyIsland);
			NearbyIsland->RegisterNearbyIsland(Island);
		}

		SpawnZonesOnIsland(Island, ChosenBiome, IslandSeed);

		SpawnedIslands.Add(Island);
		++SpawnedCount;
	}

	if (SpawnedCount < IslandCount)
	{
		UE_LOG(LogSIP, Warning,
			TEXT("IslandSpawner: Only spawned %d / %d islands. Failed placements=%d. "
			     "Consider increasing SpawnRadius, lowering footprint padding, or reducing island count."),
			SpawnedCount, IslandCount, FailedPlacements);
	}
	else
	{
		UE_LOG(LogSIP, Log,
			TEXT("IslandSpawner: Successfully spawned %d islands. WorldSeed=%d"),
			SpawnedCount, WorldSeed);
	}
}

// Destroy all spawned island actors so the world can be regenerated from scratch.
void ASIPIslandSpawner::ClearAllIslands()
{
	for (ASIPIslandActor* Island : SpawnedIslands)
	{
		if (IsValid(Island))
		{
			Island->Destroy();
		}
	}

	SpawnedIslands.Empty();
	UE_LOG(LogSIP, Log, TEXT("IslandSpawner: All islands cleared."));
}

// Weighted random biome pick used by SpawnAllIslands for each island slot.
FGameplayTag ASIPIslandSpawner::PickRandomBiome(FRandomStream& RandStream) const
{
	if (BiomeWeights.IsEmpty())
	{
		return FGameplayTag::EmptyTag;
	}

	float TotalWeight = 0.f;
	for (const auto& Pair : BiomeWeights)
	{
		TotalWeight += FMath::Max(0.f, Pair.Value);
	}

	if (TotalWeight <= 0.f)
	{
		return FGameplayTag::EmptyTag;
	}

	const float Roll = RandStream.FRandRange(0.f, TotalWeight);
	float Cumulative = 0.f;

	for (const auto& Pair : BiomeWeights)
	{
		Cumulative += FMath::Max(0.f, Pair.Value);
		if (Roll <= Cumulative)
		{
			return Pair.Key;
		}
	}

	return BiomeWeights.CreateConstIterator()->Key;
}

// Reject candidates that overlap an existing island footprint within the same vertical band.
bool ASIPIslandSpawner::IsLocationValid(
	const FVector& Candidate,
	float CandidateRadius,
	float CandidateHalfHeight,
	TArray<ASIPIslandActor*>& OutNearbyIslands) const
{
	OutNearbyIslands.Reset();

	for (const ASIPIslandActor* Island : SpawnedIslands)
	{
		if (!IsValid(Island))
		{
			continue;
		}

		const FVector ExistingLocation = Island->GetActorLocation();
		const float HorizontalDistance = FVector::Dist2D(ExistingLocation, Candidate);
		const float VerticalDistance = FMath::Abs(ExistingLocation.Z - Candidate.Z);
		const float RequiredVerticalGap = Island->GetPlacementHalfHeight() + CandidateHalfHeight + IslandVerticalPadding;

		// Give vertically separated islands permission to stack instead of failing the XY layout check.
		if (VerticalDistance >= RequiredVerticalGap)
		{
			continue;
		}

		const float RequiredHorizontalGap = FMath::Max(
			MinIslandSpacing,
			Island->GetPlacementRadius2D() + CandidateRadius + IslandFootprintPadding);

		if (HorizontalDistance < RequiredHorizontalGap)
		{
			return false;
		}

		if (SeamBandWidth > 0.f && HorizontalDistance <= RequiredHorizontalGap + SeamBandWidth)
		{
			OutNearbyIslands.Add(const_cast<ASIPIslandActor*>(Island));
		}
	}

	return true;
}

// Estimate the 2D placement radius using the island CDO bounds plus any biome-specific scale.
float ASIPIslandSpawner::GetCandidatePlacementRadius(const FGameplayTag& Biome) const
{
	const ASIPIslandActor* IslandCDO = IslandClass ? IslandClass->GetDefaultObject<ASIPIslandActor>() : nullptr;
	const FVector BoundsExtent = (IslandCDO && IslandCDO->IslandBounds)
		? IslandCDO->IslandBounds->GetScaledBoxExtent()
		: FVector(1000.f, 1000.f, 400.f);

	float Scale = 1.f;
	if (const float* ScalePtr = BiomeFootprintScale.Find(Biome))
	{
		Scale = FMath::Max(*ScalePtr, 0.1f);
	}

	return FMath::Max(BoundsExtent.X, BoundsExtent.Y) * Scale;
}

// Estimate the vertical half-height used when checking stackability between islands.
float ASIPIslandSpawner::GetCandidatePlacementHalfHeight(const FGameplayTag& Biome) const
{
	const ASIPIslandActor* IslandCDO = IslandClass ? IslandClass->GetDefaultObject<ASIPIslandActor>() : nullptr;
	const FVector BoundsExtent = (IslandCDO && IslandCDO->IslandBounds)
		? IslandCDO->IslandBounds->GetScaledBoxExtent()
		: FVector(1000.f, 1000.f, 400.f);

	float Scale = 1.f;
	if (const float* ScalePtr = BiomeFootprintScale.Find(Biome))
	{
		Scale = FMath::Max(*ScalePtr, 0.1f);
	}

	return BoundsExtent.Z * Scale;
}

// Sample a raw island candidate within the biome's radial band and height range.
FVector ASIPIslandSpawner::SampleCandidateLocation(FRandomStream& RandStream, const FVector& Origin, const FGameplayTag& Biome) const
{
	float MinRadiusRatio = 0.25f;
	float MaxRadiusRatio = 1.0f;
	float HeightOffset = 0.f;
	float HeightScale = 1.0f;

	if (const FSIPBiomeSpawnBand* SpawnBand = BiomeSpawnBands.Find(Biome))
	{
		MinRadiusRatio = FMath::Clamp(SpawnBand->MinRadiusRatio, 0.f, 1.f);
		MaxRadiusRatio = FMath::Clamp(SpawnBand->MaxRadiusRatio, MinRadiusRatio, 1.f);
		HeightOffset = SpawnBand->HeightOffset;
		HeightScale = FMath::Max(0.f, SpawnBand->HeightScale);
	}

	const float Angle = RandStream.FRandRange(0.f, 360.f);
	const float Radius = RandStream.FRandRange(SpawnRadius * MinRadiusRatio, SpawnRadius * MaxRadiusRatio);
	const float HeightRange = HeightVariance * 0.5f * HeightScale;
	const float Height = HeightOffset + RandStream.FRandRange(-HeightRange, HeightRange);

	return Origin + FVector(
		FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
		FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
		Height);
}

// Score valid candidates so seam opportunities and biome clustering win over arbitrary placement.
float ASIPIslandSpawner::ScoreCandidateLocation(
	const FVector& Candidate,
	const FGameplayTag& CandidateBiome,
	float CandidateRadius,
	float CandidateHalfHeight,
	int32 NearbyIslandCount) const
{
	float MinHorizontalClearance = SpawnRadius;
	float BiomeAffinityScore = 0.f;

	for (const ASIPIslandActor* Island : SpawnedIslands)
	{
		if (!IsValid(Island))
		{
			continue;
		}

		const FVector ExistingLocation = Island->GetActorLocation();
		const float VerticalDistance = FMath::Abs(ExistingLocation.Z - Candidate.Z);
		const float RequiredVerticalGap = Island->GetPlacementHalfHeight() + CandidateHalfHeight + IslandVerticalPadding;

		if (VerticalDistance >= RequiredVerticalGap)
		{
			continue;
		}

		const float RequiredHorizontalGap = FMath::Max(
			MinIslandSpacing,
			Island->GetPlacementRadius2D() + CandidateRadius + IslandFootprintPadding);
		const float HorizontalDistance = FVector::Dist2D(ExistingLocation, Candidate);
		const float HorizontalClearance = HorizontalDistance - RequiredHorizontalGap;
		MinHorizontalClearance = FMath::Min(MinHorizontalClearance, HorizontalClearance);

		if (Island->BiomeType == CandidateBiome)
		{
			if (SameBiomeAttractionRadius > RequiredHorizontalGap && HorizontalDistance <= SameBiomeAttractionRadius)
			{
				const float AttractionAlpha = 1.f - ((HorizontalDistance - RequiredHorizontalGap) /
					FMath::Max(1.f, SameBiomeAttractionRadius - RequiredHorizontalGap));
				BiomeAffinityScore += FMath::Max(0.f, AttractionAlpha) * SameBiomeAttractionScore;
			}
		}
		else if (DifferentBiomeAvoidanceRadius > RequiredHorizontalGap && HorizontalDistance <= DifferentBiomeAvoidanceRadius)
		{
			const float AvoidanceAlpha = 1.f - ((HorizontalDistance - RequiredHorizontalGap) /
				FMath::Max(1.f, DifferentBiomeAvoidanceRadius - RequiredHorizontalGap));
			BiomeAffinityScore -= FMath::Max(0.f, AvoidanceAlpha) * DifferentBiomeAvoidancePenalty;
		}
	}

	return MinHorizontalClearance + NearbyIslandCount * SeamPlacementBias + BiomeAffinityScore;
}

// Spawn elemental zones by tracing down onto the final island surface with per-zone spacing checks.
void ASIPIslandSpawner::SpawnZonesOnIsland(ASIPIslandActor* Island, const FGameplayTag& Biome, int32 IslandSeed)
{
	if (ZonesPerIsland <= 0 || !IsValid(Island))
	{
		return;
	}

	const TSubclassOf<ASIPElementalZoneActor>* ZoneClassPtr = ZoneClassPerBiome.Find(Biome);
	if (!ZoneClassPtr || !(*ZoneClassPtr))
	{
		return;
	}

	const FVector IslandLoc = Island->GetActorLocation();
	const FVector Extent = Island->IslandBounds->GetScaledBoxExtent();
	const float SpreadX = Extent.X * 0.65f;
	const float SpreadY = Extent.Y * 0.65f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Island);

	TArray<FVector> AcceptedZoneLocations;
	const float ZoneMinSpacing = FMath::Max(100.f, FMath::Min(SpreadX, SpreadY) * ZoneMinSpacingRatio);

	for (int32 i = 0; i < ZonesPerIsland; ++i)
	{
		FRandomStream ZoneRand(IslandSeed + i + 1);
		bool bSpawnedZone = false;

		for (int32 Attempt = 0; Attempt < ZonePlacementAttemptsPerZone; ++Attempt)
		{
			const float AngleRadians = ZoneRand.FRandRange(0.f, UE_TWO_PI);
			const float RadiusAlpha = FMath::Sqrt(ZoneRand.FRandRange(0.f, 1.f));
			const float OffsetX = FMath::Cos(AngleRadians) * SpreadX * RadiusAlpha;
			const float OffsetY = FMath::Sin(AngleRadians) * SpreadY * RadiusAlpha;

			const FVector TraceStart = IslandLoc + FVector(OffsetX, OffsetY, ZoneTraceHeight);
			const FVector TraceEnd = IslandLoc + FVector(OffsetX, OffsetY, -ZoneTraceHeight);

			FHitResult Hit;
			const bool bHit = GetWorld()->LineTraceSingleByChannel(
				Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);

			if (!bHit)
			{
				continue;
			}

			const bool bTooCloseToOtherZone = AcceptedZoneLocations.ContainsByPredicate(
				[&Hit, ZoneMinSpacing](const FVector& ExistingLocation)
				{
					return FVector::Dist2D(ExistingLocation, Hit.Location) < ZoneMinSpacing;
				});

			if (bTooCloseToOtherZone)
			{
				continue;
			}

			FActorSpawnParameters ZoneParams;
			ZoneParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			const float ZoneYaw = ZoneRand.FRandRange(0.f, 360.f);
			ASIPElementalZoneActor* Zone = GetWorld()->SpawnActor<ASIPElementalZoneActor>(
				*ZoneClassPtr,
				Hit.Location + FVector(0.f, 0.f, 10.f),
				FRotator(0.f, ZoneYaw, 0.f),
				ZoneParams);

			if (!Zone)
			{
				continue;
			}

			AcceptedZoneLocations.Add(Hit.Location);
			bSpawnedZone = true;
			UE_LOG(LogSIP, Log,
				TEXT("IslandSpawner: Zone[%d] spawned on Island[%d] Biome=[%s] at %s"),
				i, Island->IslandIndex, *Biome.ToString(), *Hit.Location.ToString());
			break;
		}

		if (!bSpawnedZone)
		{
			UE_LOG(LogSIP, Warning,
				TEXT("IslandSpawner: Failed to place Zone[%d] on Island[%d] after %d attempts. "
				     "Try increasing island size, lowering ZoneMinSpacingRatio, or raising ZoneTraceHeight."),
				i, Island->IslandIndex, ZonePlacementAttemptsPerZone);
		}
	}
}
