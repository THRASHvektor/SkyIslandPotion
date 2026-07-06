#include "World/Encounter/SIPEncounterPCGComponent.h"

#include "Character/SIPEnemyCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "SIPLogCategory.h"
#include "World/Elemental/SIPElementReactiveZoneBase.h"
#include "World/Encounter/SIPEncounterExitGate.h"
#include "World/Encounter/SIPEncounterPatternData.h"

USIPEncounterPCGComponent::USIPEncounterPCGComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USIPEncounterPCGComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateEncounter(DefaultSeed);
	}
}

void USIPEncounterPCGComponent::GenerateEncounter(int32 Seed)
{
	ClearEncounter();

	const int32 ResolvedSeed = Seed == 0 ? DefaultSeed : Seed;
	FRandomStream RandomStream(ResolvedSeed);

	USIPEncounterPatternData* Pattern = SelectPattern(RandomStream);
	if (!Pattern)
	{
		UE_LOG(LogSIP, Warning, TEXT("Encounter PCG failed: no pattern found for LevelIndex=%d."), LevelIndex);
		return;
	}

	FSIPEncounterCandidate BestCandidate;
	float BestScore = -FLT_MAX;
	FString LastFailureReason;

	const int32 SafeCandidateCount = FMath::Max(1, CandidateCount);
	for (int32 CandidateIndex = 0; CandidateIndex < SafeCandidateCount; ++CandidateIndex)
	{
		FSIPEncounterCandidate Candidate;
		if (!BuildCandidate(*Pattern, RandomStream, Candidate))
		{
			LastFailureReason = TEXT("Failed to build candidate.");
			continue;
		}

		const FSIPEncounterValidationResult ValidationResult = ValidateCandidate(*Pattern, Candidate);
		if (!ValidationResult.bPassed)
		{
			LastFailureReason = ValidationResult.FailureReason;
			continue;
		}

		Candidate.Score = ScoreCandidate(*Pattern, Candidate, ValidationResult);
		if (Candidate.Score > BestScore)
		{
			BestScore = Candidate.Score;
			BestCandidate = Candidate;
		}
	}

	if (BestScore <= -FLT_MAX)
	{
		UE_LOG(LogSIP, Warning, TEXT("Encounter PCG failed: no valid candidate. Last reason: %s"), *LastFailureReason);
		return;
	}

	SpawnCandidate(BestCandidate, *Pattern);
	StartObjectiveTracking(Pattern);

	if (bDrawDebug)
	{
		DrawDebugCandidate(BestCandidate, *Pattern);
	}

	UE_LOG(LogSIP, Log, TEXT("Encounter PCG generated pattern [%s] with score %.2f and seed %d."),
		*Pattern->PatternTag.ToString(), BestCandidate.Score, ResolvedSeed);
}

void USIPEncounterPCGComponent::ClearEncounter()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ObjectiveCheckTimerHandle);
	}

	bEncounterCompleted = false;
	ObjectiveEnemies.Reset();
	SpawnedExitGate = nullptr;

	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}

	SpawnedActors.Reset();
}

USIPEncounterPatternData* USIPEncounterPCGComponent::SelectPattern(FRandomStream& RandomStream) const
{
	TArray<USIPEncounterPatternData*> MatchingPatterns;
	for (USIPEncounterPatternData* Pattern : Patterns)
	{
		if (Pattern && Pattern->LevelIndex == LevelIndex)
		{
			MatchingPatterns.Add(Pattern);
		}
	}

	if (MatchingPatterns.IsEmpty())
	{
		for (USIPEncounterPatternData* Pattern : Patterns)
		{
			if (Pattern)
			{
				MatchingPatterns.Add(Pattern);
			}
		}
	}

	if (MatchingPatterns.IsEmpty())
	{
		return nullptr;
	}

	const int32 PatternIndex = RandomStream.RandRange(0, MatchingPatterns.Num() - 1);
	return MatchingPatterns[PatternIndex];
}

bool USIPEncounterPCGComponent::BuildCandidate(const USIPEncounterPatternData& Pattern, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const
{
	if (Pattern.TerrainRules.IsEmpty())
	{
		return false;
	}

	switch (Pattern.PatternMode)
	{
	case ESIPEncounterPatternMode::MultipleCluster:
		GenerateThreeElementClusterLayout(Pattern, RandomStream, OutCandidate);
		break;
	case ESIPEncounterPatternMode::SingleCluster:
	default:
		GenerateSingleClusterLayout(Pattern, RandomStream, OutCandidate);
		break;
	}

	FillNormalGroundPatches(Pattern, OutCandidate);

	return !OutCandidate.TerrainPatches.IsEmpty();
}

void USIPEncounterPCGComponent::GenerateSingleClusterLayout(const USIPEncounterPatternData& Pattern, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const
{
	const float ArenaHalfExtent = GetArenaHalfExtent(Pattern, 300.0f);
	ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(-ArenaHalfExtent * 0.55f, 0.0f), ArenaHalfExtent, FVector2D::ZeroVector), OutCandidate.PlayerStartLocation);
	ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(ArenaHalfExtent * 0.65f, 0.0f), ArenaHalfExtent, FVector2D::ZeroVector), OutCandidate.ExitLocation);
	ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(ArenaHalfExtent * 0.25f, ArenaHalfExtent * 0.35f), ArenaHalfExtent, FVector2D::ZeroVector), OutCandidate.RewardLocation);

	for (const FSIPTerrainClusterRule& Rule : Pattern.TerrainRules)
	{
		if (!Rule.ZoneClass)
		{
			continue;
		}

		const int32 MinCount = FMath::Max(0, Rule.MinCount);
		const int32 MaxCount = FMath::Max(MinCount, Rule.MaxCount);
		const int32 Count = RandomStream.RandRange(MinCount, MaxCount);
		const FVector2D ClusterCenter(RandomStream.FRandRange(-ArenaHalfExtent * 0.1f, ArenaHalfExtent * 0.25f), 0.0f);

		for (int32 Index = 0; Index < Count; ++Index)
		{
			TryAddTerrainPatch(Rule, ClusterCenter, ArenaHalfExtent, OutCandidate.TerrainPatches.IsEmpty(), RandomStream, OutCandidate);
		}
	}

	for (const FSIPEnemySpawnRule& Rule : Pattern.EnemyRules)
	{
		if (!Rule.EnemyClass)
		{
			continue;
		}

		const int32 MinCount = FMath::Max(0, Rule.MinCount);
		const int32 MaxCount = FMath::Max(MinCount, Rule.MaxCount);
		const int32 Count = RandomStream.RandRange(MinCount, MaxCount);
		OutCandidate.RequestedEnemySpawnCount += Count;

		for (int32 Index = 0; Index < Count; ++Index)
		{
			TryBuildEnemySpawn(Pattern, Rule, ArenaHalfExtent, RandomStream, OutCandidate);
		}
	}

	if (Pattern.DefaultCoverClass)
	{
		for (int32 CoverIndex = 0; CoverIndex < 2; ++CoverIndex)
		{
			const float Side = CoverIndex == 0 ? -1.0f : 1.0f;
			FVector CoverLocation;
			ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(-ArenaHalfExtent * 0.2f, Side * ArenaHalfExtent * 0.25f), ArenaHalfExtent, FVector2D::ZeroVector), CoverLocation);

			FSIPResolvedCoverPoint CoverPoint;
			CoverPoint.CoverClass = Pattern.DefaultCoverClass;
			CoverPoint.Transform = FTransform(FRotator::ZeroRotator, CoverLocation);
			OutCandidate.CoverPoints.Add(CoverPoint);
		}
	}
}

void USIPEncounterPCGComponent::GenerateThreeElementClusterLayout(const USIPEncounterPatternData& Pattern, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const
{
	const float ArenaHalfExtent = GetArenaHalfExtent(Pattern, 600.0f);
	ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(0.0f, -ArenaHalfExtent * 0.55f), ArenaHalfExtent, FVector2D::ZeroVector), OutCandidate.PlayerStartLocation);
	ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(0.0f, ArenaHalfExtent * 0.65f), ArenaHalfExtent, FVector2D::ZeroVector), OutCandidate.ExitLocation);
	ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(ArenaHalfExtent * 0.35f, ArenaHalfExtent * 0.2f), ArenaHalfExtent, FVector2D::ZeroVector), OutCandidate.RewardLocation);

	const TArray<FVector2D> ClusterCenters = {
		FVector2D(0.0f, ArenaHalfExtent * 0.35f),
		FVector2D(-ArenaHalfExtent * 0.45f, -ArenaHalfExtent * 0.1f),
		FVector2D(ArenaHalfExtent * 0.45f, -ArenaHalfExtent * 0.1f)
	};

	int32 RuleIndex = 0;
	for (const FSIPTerrainClusterRule& Rule : Pattern.TerrainRules)
	{
		if (!Rule.ZoneClass)
		{
			++RuleIndex;
			continue;
		}

		const FVector2D ClusterCenter = ClusterCenters[RuleIndex % ClusterCenters.Num()];
		const int32 MinCount = FMath::Max(0, Rule.MinCount);
		const int32 MaxCount = FMath::Max(MinCount, Rule.MaxCount);
		const int32 Count = RandomStream.RandRange(MinCount, MaxCount);

		for (int32 Index = 0; Index < Count; ++Index)
		{
			TryAddTerrainPatch(Rule, ClusterCenter, ArenaHalfExtent, Index == 0, RandomStream, OutCandidate);
		}

		++RuleIndex;
	}

	for (const FSIPEnemySpawnRule& Rule : Pattern.EnemyRules)
	{
		if (!Rule.EnemyClass)
		{
			continue;
		}

		const int32 MinCount = FMath::Max(0, Rule.MinCount);
		const int32 MaxCount = FMath::Max(MinCount, Rule.MaxCount);
		const int32 Count = RandomStream.RandRange(MinCount, MaxCount);
		OutCandidate.RequestedEnemySpawnCount += Count;

		for (int32 Index = 0; Index < Count; ++Index)
		{
			TryBuildEnemySpawn(Pattern, Rule, ArenaHalfExtent, RandomStream, OutCandidate);
		}
	}

	if (Pattern.DefaultCoverClass)
	{
		FVector CoverLocation;
		ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(FVector2D(0.0f, -ArenaHalfExtent * 0.15f), ArenaHalfExtent, FVector2D::ZeroVector), CoverLocation);

		FSIPResolvedCoverPoint CoverPoint;
		CoverPoint.CoverClass = Pattern.DefaultCoverClass;
		CoverPoint.Transform = FTransform(FRotator::ZeroRotator, CoverLocation);
		OutCandidate.CoverPoints.Add(CoverPoint);
	}
}

bool USIPEncounterPCGComponent::ProjectLocalPointToWorldSurface(const FVector2D& LocalPoint, FVector& OutWorldLocation) const
{
	const FVector Origin = GetEncounterOrigin();
	const FVector TraceXY = Origin + FVector(LocalPoint.X, LocalPoint.Y, 0.0f);
	const FVector TraceStart = TraceXY + FVector(0.0f, 0.0f, ProjectionTraceHalfHeight);
	const FVector TraceEnd = TraceXY - FVector(0.0f, 0.0f, ProjectionTraceHalfHeight);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SIPEncounterPCGProject), false, GetOwner());
	const bool bHit = GetWorld() && GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ProjectionTraceChannel, QueryParams);
	if (bHit)
	{
		OutWorldLocation = HitResult.Location;
		return true;
	}

	OutWorldLocation = TraceXY;
	return true;
}

FVector USIPEncounterPCGComponent::GetEncounterOrigin() const
{
	return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

float USIPEncounterPCGComponent::GetArenaHalfExtent(const USIPEncounterPatternData& Pattern, float MinHalfExtent) const
{
	return FMath::Max(MinHalfExtent, Pattern.ArenaHalfExtent);
}

FVector2D USIPEncounterPCGComponent::ClampLocalPointToSquare(const FVector2D& LocalPoint, float HalfExtent, const FVector2D& Padding) const
{
	const float MaxX = FMath::Max(0.0f, HalfExtent - FMath::Max(0.0f, Padding.X));
	const float MaxY = FMath::Max(0.0f, HalfExtent - FMath::Max(0.0f, Padding.Y));
	return FVector2D(
		FMath::Clamp(LocalPoint.X, -MaxX, MaxX),
		FMath::Clamp(LocalPoint.Y, -MaxY, MaxY));
}

FVector USIPEncounterPCGComponent::ResolveTerrainZoneExtent(const FSIPTerrainClusterRule& Rule, FRandomStream& RandomStream) const
{
		if (!Rule.bRandomizeZoneExtent)
		{
				const FVector FixedExtent = Rule.ZoneExtent.GetAbs();
				return FVector(
						FMath::Max(FixedExtent.X, 1.0f),
						FMath::Max(FixedExtent.Y, 1.0f),
						FMath::Max(FixedExtent.Z, 1.0f));
		}

		const FVector MinExtent = Rule.MinRandomZoneExtent.GetAbs();
		const FVector MaxExtent = Rule.MaxRandomZoneExtent.GetAbs();
		return FVector(
				FMath::Max(1.0f, RandomStream.FRandRange(FMath::Min(MinExtent.X, MaxExtent.X), FMath::Max(MinExtent.X, MaxExtent.X))),
				FMath::Max(1.0f, RandomStream.FRandRange(FMath::Min(MinExtent.Y, MaxExtent.Y), FMath::Max(MinExtent.Y, MaxExtent.Y))),
				FMath::Max(1.0f, RandomStream.FRandRange(FMath::Min(MinExtent.Z, MaxExtent.Z), FMath::Max(MinExtent.Z, MaxExtent.Z))));
}

bool USIPEncounterPCGComponent::TryBuildConnectedTerrainPatch(const FSIPTerrainClusterRule& Rule, float ArenaHalfExtent, const FVector& ZoneExtent, bool bKeyTerrain, FRandomStream& RandomStream, const FSIPEncounterCandidate& Candidate, FSIPResolvedTerrainPatch& OutTerrainPatch) const
{
	if (!Rule.bAllowConnectedZones || Candidate.TerrainPatches.IsEmpty())
	{
		return false;
	}

	const FSIPResolvedTerrainPatch& AnchorPatch = Candidate.TerrainPatches[RandomStream.RandRange(0, Candidate.TerrainPatches.Num() - 1)];
	const FVector AnchorLocation = AnchorPatch.Transform.GetLocation();
	const FVector AnchorExtent = AnchorPatch.ZoneExtent.GetAbs();
	const int32 SideIndex = RandomStream.RandRange(0, 3);
	FVector2D LocalPoint(AnchorLocation.X - GetEncounterOrigin().X, AnchorLocation.Y - GetEncounterOrigin().Y);

	switch (SideIndex)
	{
	case 0:
		LocalPoint.X += AnchorExtent.X + ZoneExtent.X;
		LocalPoint.Y += RandomStream.FRandRange(-FMath::Min(AnchorExtent.Y, ZoneExtent.Y), FMath::Min(AnchorExtent.Y, ZoneExtent.Y));
		break;
	case 1:
		LocalPoint.X -= AnchorExtent.X + ZoneExtent.X;
		LocalPoint.Y += RandomStream.FRandRange(-FMath::Min(AnchorExtent.Y, ZoneExtent.Y), FMath::Min(AnchorExtent.Y, ZoneExtent.Y));
		break;
	case 2:
		LocalPoint.X += RandomStream.FRandRange(-FMath::Min(AnchorExtent.X, ZoneExtent.X), FMath::Min(AnchorExtent.X, ZoneExtent.X));
		LocalPoint.Y += AnchorExtent.Y + ZoneExtent.Y;
		break;
	default:
		LocalPoint.X += RandomStream.FRandRange(-FMath::Min(AnchorExtent.X, ZoneExtent.X), FMath::Min(AnchorExtent.X, ZoneExtent.X));
		LocalPoint.Y -= AnchorExtent.Y + ZoneExtent.Y;
		break;
	}

	LocalPoint = ClampLocalPointToSquare(LocalPoint, ArenaHalfExtent, FVector2D(ZoneExtent.X, ZoneExtent.Y));

	FVector WorldLocation;
	if (!ProjectLocalPointToWorldSurface(LocalPoint, WorldLocation))
	{
		return false;
	}

	OutTerrainPatch.ElementTag = Rule.ElementTag;
	OutTerrainPatch.ZoneClass = Rule.ZoneClass;
	OutTerrainPatch.Transform = FTransform(FRotator::ZeroRotator, WorldLocation);
	OutTerrainPatch.ZoneExtent = ZoneExtent;
	OutTerrainPatch.SeparationPadding = 0.0f;
	OutTerrainPatch.bKeyTerrain = bKeyTerrain;
	return true;
}

bool USIPEncounterPCGComponent::TryAddTerrainPatch(const FSIPTerrainClusterRule& Rule, const FVector2D& ClusterCenter, float ArenaHalfExtent, bool bKeyTerrain, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const
{
	const int32 SafeAttemptCount = FMath::Max(1, TerrainPatchPlacementAttempts);
	for (int32 AttemptIndex = 0; AttemptIndex < SafeAttemptCount; ++AttemptIndex)
	{
		const FVector ZoneExtent = ResolveTerrainZoneExtent(Rule, RandomStream);
		const bool bTryConnectedPlacement = Rule.bAllowConnectedZones && RandomStream.FRand() <= Rule.ConnectedZonePlacementChance;

		FSIPResolvedTerrainPatch TerrainPatch;
		if (bTryConnectedPlacement && TryBuildConnectedTerrainPatch(Rule, ArenaHalfExtent, ZoneExtent, bKeyTerrain, RandomStream, OutCandidate, TerrainPatch))
		{
			// Connected placement already built the patch.
		}
		else
		{
			const float SeparationPadding = Rule.bAllowConnectedZones ? 0.0f : TerrainPatchSeparationPadding;
			const FVector2D LocalPoint = ClampLocalPointToSquare(
				ClusterCenter + FVector2D(
					RandomStream.FRandRange(-Rule.ClusterHalfExtent, Rule.ClusterHalfExtent),
					RandomStream.FRandRange(-Rule.ClusterHalfExtent, Rule.ClusterHalfExtent)),
				ArenaHalfExtent,
				FVector2D(ZoneExtent.X + SeparationPadding, ZoneExtent.Y + SeparationPadding));

			FVector WorldLocation;
			if (!ProjectLocalPointToWorldSurface(LocalPoint, WorldLocation))
			{
				continue;
			}

			TerrainPatch.ElementTag = Rule.ElementTag;
			TerrainPatch.ZoneClass = Rule.ZoneClass;
			TerrainPatch.Transform = FTransform(FRotator::ZeroRotator, WorldLocation);
			TerrainPatch.ZoneExtent = ZoneExtent;
			TerrainPatch.SeparationPadding = SeparationPadding;
			TerrainPatch.bKeyTerrain = bKeyTerrain;
		}

		if (!CanPlaceTerrainPatch(TerrainPatch, OutCandidate))
		{
			continue;
		}

		OutCandidate.TerrainPatches.Add(TerrainPatch);
		return true;
	}

	return false;
}

bool USIPEncounterPCGComponent::CanPlaceTerrainPatch(const FSIPResolvedTerrainPatch& NewPatch, const FSIPEncounterCandidate& Candidate) const
{
	for (const FSIPResolvedTerrainPatch& ExistingPatch : Candidate.TerrainPatches)
	{
		const float PairPadding = FMath::Min(NewPatch.SeparationPadding, ExistingPatch.SeparationPadding);
		if (DoTerrainPatchesOverlap2D(NewPatch, ExistingPatch, PairPadding))
		{
			return false;
		}
	}

	return true;
}

bool USIPEncounterPCGComponent::DoTerrainPatchesOverlap2D(const FSIPResolvedTerrainPatch& FirstPatch, const FSIPResolvedTerrainPatch& SecondPatch, float Padding) const
{
	const FVector FirstLocation = FirstPatch.Transform.GetLocation();
	const FVector SecondLocation = SecondPatch.Transform.GetLocation();
	const FVector FirstExtent = FirstPatch.ZoneExtent.GetAbs();
	const FVector SecondExtent = SecondPatch.ZoneExtent.GetAbs();
	const float SafePadding = FMath::Max(0.0f, Padding);

	const bool bOverlapX = FMath::Abs(FirstLocation.X - SecondLocation.X) < FirstExtent.X + SecondExtent.X + SafePadding;
	const bool bOverlapY = FMath::Abs(FirstLocation.Y - SecondLocation.Y) < FirstExtent.Y + SecondExtent.Y + SafePadding;
	return bOverlapX && bOverlapY;
}

void USIPEncounterPCGComponent::FillNormalGroundPatches(const USIPEncounterPatternData& Pattern, FSIPEncounterCandidate& OutCandidate) const
{
	OutCandidate.GroundPatches.Reset();

	if (!Pattern.NormalGroundClass)
	{
		return;
	}

	const float ArenaHalfExtent = GetArenaHalfExtent(Pattern, 300.0f);
	const FVector Origin = GetEncounterOrigin();
	TArray<float> XBreaks;
	TArray<float> YBreaks;
	XBreaks.Reserve(OutCandidate.TerrainPatches.Num() * 2 + 2);
	YBreaks.Reserve(OutCandidate.TerrainPatches.Num() * 2 + 2);
	XBreaks.Add(-ArenaHalfExtent);
	XBreaks.Add(ArenaHalfExtent);
	YBreaks.Add(-ArenaHalfExtent);
	YBreaks.Add(ArenaHalfExtent);

	for (const FSIPResolvedTerrainPatch& TerrainPatch : OutCandidate.TerrainPatches)
	{
		const FVector TerrainLocation = TerrainPatch.Transform.GetLocation();
		const FVector TerrainExtent = TerrainPatch.ZoneExtent.GetAbs();
		const FVector2D TerrainLocalLocation(TerrainLocation.X - Origin.X, TerrainLocation.Y - Origin.Y);
		const float MinX = FMath::Clamp(TerrainLocalLocation.X - TerrainExtent.X, -ArenaHalfExtent, ArenaHalfExtent);
		const float MaxX = FMath::Clamp(TerrainLocalLocation.X + TerrainExtent.X, -ArenaHalfExtent, ArenaHalfExtent);
		const float MinY = FMath::Clamp(TerrainLocalLocation.Y - TerrainExtent.Y, -ArenaHalfExtent, ArenaHalfExtent);
		const float MaxY = FMath::Clamp(TerrainLocalLocation.Y + TerrainExtent.Y, -ArenaHalfExtent, ArenaHalfExtent);

		if (MaxX - MinX > KINDA_SMALL_NUMBER)
		{
			XBreaks.Add(MinX);
			XBreaks.Add(MaxX);
		}

		if (MaxY - MinY > KINDA_SMALL_NUMBER)
		{
			YBreaks.Add(MinY);
			YBreaks.Add(MaxY);
		}
	}

	XBreaks.Sort();
	YBreaks.Sort();

	for (int32 Index = XBreaks.Num() - 1; Index > 0; --Index)
	{
		if (FMath::IsNearlyEqual(XBreaks[Index], XBreaks[Index - 1], 1.0f))
		{
			XBreaks.RemoveAt(Index);
		}
	}

	for (int32 Index = YBreaks.Num() - 1; Index > 0; --Index)
	{
		if (FMath::IsNearlyEqual(YBreaks[Index], YBreaks[Index - 1], 1.0f))
		{
			YBreaks.RemoveAt(Index);
		}
	}

	const int32 XCellCount = XBreaks.Num() - 1;
	const int32 YCellCount = YBreaks.Num() - 1;
	if (XCellCount <= 0 || YCellCount <= 0)
	{
		return;
	}

	TArray<bool> bGroundCells;
	bGroundCells.Init(false, XCellCount * YCellCount);
	for (int32 XIndex = 0; XIndex < XCellCount; ++XIndex)
	{
		const float MinX = XBreaks[XIndex];
		const float MaxX = XBreaks[XIndex + 1];
		if (MaxX - MinX <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		for (int32 YIndex = 0; YIndex < YCellCount; ++YIndex)
		{
			const float MinY = YBreaks[YIndex];
			const float MaxY = YBreaks[YIndex + 1];
			if (MaxY - MinY <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const FVector2D LocalCenter((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
			bool bCoveredByTerrain = false;
			for (const FSIPResolvedTerrainPatch& TerrainPatch : OutCandidate.TerrainPatches)
			{
				const FVector TerrainLocation = TerrainPatch.Transform.GetLocation();
				const FVector TerrainExtent = TerrainPatch.ZoneExtent.GetAbs();
				const FVector2D TerrainLocalLocation(TerrainLocation.X - Origin.X, TerrainLocation.Y - Origin.Y);
				if (FMath::Abs(LocalCenter.X - TerrainLocalLocation.X) < TerrainExtent.X && FMath::Abs(LocalCenter.Y - TerrainLocalLocation.Y) < TerrainExtent.Y)
				{
					bCoveredByTerrain = true;
					break;
				}
			}

			bGroundCells[XIndex * YCellCount + YIndex] = !bCoveredByTerrain;
		}
	}

	TArray<bool> bConsumedCells;
	bConsumedCells.Init(false, XCellCount * YCellCount);
	for (int32 XIndex = 0; XIndex < XCellCount; ++XIndex)
	{
		for (int32 YIndex = 0; YIndex < YCellCount; ++YIndex)
		{
			const int32 CellIndex = XIndex * YCellCount + YIndex;
			if (!bGroundCells[CellIndex] || bConsumedCells[CellIndex])
			{
				continue;
			}

			int32 EndXIndex = XIndex + 1;
			while (EndXIndex < XCellCount && bGroundCells[EndXIndex * YCellCount + YIndex] && !bConsumedCells[EndXIndex * YCellCount + YIndex])
			{
				++EndXIndex;
			}

			int32 EndYIndex = YIndex + 1;
			bool bCanExtendY = true;
			while (EndYIndex < YCellCount && bCanExtendY)
			{
				for (int32 TestXIndex = XIndex; TestXIndex < EndXIndex; ++TestXIndex)
				{
					const int32 TestCellIndex = TestXIndex * YCellCount + EndYIndex;
					if (!bGroundCells[TestCellIndex] || bConsumedCells[TestCellIndex])
					{
						bCanExtendY = false;
						break;
					}
				}

				if (bCanExtendY)
				{
					++EndYIndex;
				}
			}

			for (int32 ConsumeXIndex = XIndex; ConsumeXIndex < EndXIndex; ++ConsumeXIndex)
			{
				for (int32 ConsumeYIndex = YIndex; ConsumeYIndex < EndYIndex; ++ConsumeYIndex)
				{
					bConsumedCells[ConsumeXIndex * YCellCount + ConsumeYIndex] = true;
				}
			}

			AddMergedGroundPatch(Pattern, XBreaks, YBreaks, XIndex, YIndex, EndXIndex, EndYIndex, OutCandidate);
		}
	}
}

void USIPEncounterPCGComponent::AddMergedGroundPatch(const USIPEncounterPatternData& Pattern, const TArray<float>& XBreaks, const TArray<float>& YBreaks, int32 StartXIndex, int32 StartYIndex, int32 EndXIndex, int32 EndYIndex, FSIPEncounterCandidate& OutCandidate) const
{
	if (!XBreaks.IsValidIndex(StartXIndex) || !XBreaks.IsValidIndex(EndXIndex) || !YBreaks.IsValidIndex(StartYIndex) || !YBreaks.IsValidIndex(EndYIndex))
	{
		return;
	}

	const float MinX = XBreaks[StartXIndex];
	const float MaxX = XBreaks[EndXIndex];
	const float MinY = YBreaks[StartYIndex];
	const float MaxY = YBreaks[EndYIndex];
	const float HalfX = (MaxX - MinX) * 0.5f;
	const float HalfY = (MaxY - MinY) * 0.5f;
	if (HalfX <= KINDA_SMALL_NUMBER || HalfY <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector2D LocalCenter((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
	FVector WorldLocation;
	if (!ProjectLocalPointToWorldSurface(LocalCenter, WorldLocation))
	{
		return;
	}

	FSIPResolvedGroundPatch GroundPatch;
	GroundPatch.GroundClass = Pattern.NormalGroundClass;
	GroundPatch.Transform = FTransform(FRotator::ZeroRotator, WorldLocation);
	GroundPatch.GroundExtent = FVector(HalfX, HalfY, FMath::Max(1.0f, Pattern.GroundPatchHalfHeight));
	OutCandidate.GroundPatches.Add(GroundPatch);
}

bool USIPEncounterPCGComponent::IsPointInsideGroundPatch2D(const FVector& Point, const FSIPResolvedGroundPatch& GroundPatch) const
{
	const FVector PatchLocation = GroundPatch.Transform.GetLocation();
	const FVector PatchExtent = GroundPatch.GroundExtent.GetAbs();
	return FMath::Abs(Point.X - PatchLocation.X) <= PatchExtent.X && FMath::Abs(Point.Y - PatchLocation.Y) <= PatchExtent.Y;
}

bool USIPEncounterPCGComponent::AreGroundPatchesConnected2D(const FSIPResolvedGroundPatch& FirstPatch, const FSIPResolvedGroundPatch& SecondPatch) const
{
	const FVector FirstLocation = FirstPatch.Transform.GetLocation();
	const FVector SecondLocation = SecondPatch.Transform.GetLocation();
	const FVector FirstExtent = FirstPatch.GroundExtent.GetAbs();
	const FVector SecondExtent = SecondPatch.GroundExtent.GetAbs();
	const float FirstMinX = FirstLocation.X - FirstExtent.X;
	const float FirstMaxX = FirstLocation.X + FirstExtent.X;
	const float FirstMinY = FirstLocation.Y - FirstExtent.Y;
	const float FirstMaxY = FirstLocation.Y + FirstExtent.Y;
	const float SecondMinX = SecondLocation.X - SecondExtent.X;
	const float SecondMaxX = SecondLocation.X + SecondExtent.X;
	const float SecondMinY = SecondLocation.Y - SecondExtent.Y;
	const float SecondMaxY = SecondLocation.Y + SecondExtent.Y;
	const float Tolerance = 1.0f;

	const bool bShareVerticalEdge = (FMath::IsNearlyEqual(FirstMaxX, SecondMinX, Tolerance) || FMath::IsNearlyEqual(SecondMaxX, FirstMinX, Tolerance))
		&& FMath::Min(FirstMaxY, SecondMaxY) - FMath::Max(FirstMinY, SecondMinY) > Tolerance;
	const bool bShareHorizontalEdge = (FMath::IsNearlyEqual(FirstMaxY, SecondMinY, Tolerance) || FMath::IsNearlyEqual(SecondMaxY, FirstMinY, Tolerance))
		&& FMath::Min(FirstMaxX, SecondMaxX) - FMath::Max(FirstMinX, SecondMinX) > Tolerance;
	return bShareVerticalEdge || bShareHorizontalEdge;
}

bool USIPEncounterPCGComponent::HasGroundPathFromStartToExit(const FSIPEncounterCandidate& Candidate) const
{
	const int32 StartPatchIndex = Candidate.GroundPatches.IndexOfByPredicate([this, &Candidate](const FSIPResolvedGroundPatch& GroundPatch)
	{
		return IsPointInsideGroundPatch2D(Candidate.PlayerStartLocation, GroundPatch);
	});

	if (StartPatchIndex == INDEX_NONE)
	{
		return false;
	}

	const int32 ExitPatchIndex = Candidate.GroundPatches.IndexOfByPredicate([this, &Candidate](const FSIPResolvedGroundPatch& GroundPatch)
	{
		return IsPointInsideGroundPatch2D(Candidate.ExitLocation, GroundPatch);
	});

	if (ExitPatchIndex == INDEX_NONE)
	{
		return false;
	}

	if (StartPatchIndex == ExitPatchIndex)
	{
		return true;
	}

	TArray<bool> bVisited;
	bVisited.Init(false, Candidate.GroundPatches.Num());
	TArray<int32> OpenSet;
	OpenSet.Add(StartPatchIndex);
	bVisited[StartPatchIndex] = true;

	for (int32 OpenSetIndex = 0; OpenSetIndex < OpenSet.Num(); ++OpenSetIndex)
	{
		const int32 CurrentPatchIndex = OpenSet[OpenSetIndex];
		for (int32 NextPatchIndex = 0; NextPatchIndex < Candidate.GroundPatches.Num(); ++NextPatchIndex)
		{
			if (bVisited[NextPatchIndex] || !AreGroundPatchesConnected2D(Candidate.GroundPatches[CurrentPatchIndex], Candidate.GroundPatches[NextPatchIndex]))
			{
				continue;
			}

			if (NextPatchIndex == ExitPatchIndex)
			{
				return true;
			}

			bVisited[NextPatchIndex] = true;
			OpenSet.Add(NextPatchIndex);
		}
	}

	return false;
}

bool USIPEncounterPCGComponent::TryBuildEnemySpawn(const USIPEncounterPatternData& Pattern, const FSIPEnemySpawnRule& Rule, float ArenaHalfExtent, FRandomStream& RandomStream, FSIPEncounterCandidate& Candidate) const
{
	const int32 SafeAttemptCount = FMath::Max(1, EnemySpawnPlacementAttempts);
	const float SafeSpawnSpread = FMath::Max(100.0f, Rule.SpawnSpread);

	for (int32 AttemptIndex = 0; AttemptIndex < SafeAttemptCount; ++AttemptIndex)
	{
		const int32 TerrainIndex = FindRandomTerrainIndexByElement(Candidate, Rule.PreferredTerrainElement, RandomStream);
		FVector SpawnLocation = GetEncounterOrigin();

		if (Candidate.TerrainPatches.IsValidIndex(TerrainIndex))
		{
			const FSIPResolvedTerrainPatch& LinkedTerrain = Candidate.TerrainPatches[TerrainIndex];
			if (Rule.TerrainRelation == ESIPEnemyTerrainRelation::OnTerrain)
			{
				SpawnLocation = LinkedTerrain.Transform.GetLocation();
				SpawnLocation.X += RandomStream.FRandRange(-LinkedTerrain.ZoneExtent.X * 0.5f, LinkedTerrain.ZoneExtent.X * 0.5f);
				SpawnLocation.Y += RandomStream.FRandRange(-LinkedTerrain.ZoneExtent.Y * 0.5f, LinkedTerrain.ZoneExtent.Y * 0.5f);
			}
			else
			{
				const float DistanceScale = RandomStream.FRandRange(1.0f, 1.75f);
				SpawnLocation = SamplePointNearTerrain(RandomStream, LinkedTerrain, SafeSpawnSpread * DistanceScale, ArenaHalfExtent);
			}
		}
		else
		{
			SpawnLocation = SamplePointInSquare(RandomStream, GetEncounterOrigin(), ArenaHalfExtent * 0.5f);
		}

		if (!IsEnemySpawnLocationValid(Pattern, SpawnLocation, Candidate))
		{
			continue;
		}

		FSIPResolvedEnemySpawn EnemySpawn;
		EnemySpawn.EnemyClass = Rule.EnemyClass;
		EnemySpawn.PreferredTerrainElement = Rule.PreferredTerrainElement;
		EnemySpawn.TerrainRelation = Rule.TerrainRelation;
		EnemySpawn.Transform = FTransform(FRotator::ZeroRotator, SpawnLocation + FVector(0.0f, 0.0f, 90.0f));
		EnemySpawn.LinkedTerrainIndex = TerrainIndex;
		Candidate.EnemySpawns.Add(EnemySpawn);
		return true;
	}

	return false;
}

bool USIPEncounterPCGComponent::IsEnemySpawnLocationValid(const USIPEncounterPatternData& Pattern, const FVector& SpawnLocation, const FSIPEncounterCandidate& Candidate) const
{
	const FSIPEncounterValidationRule& ValidationRule = Pattern.ValidationRule;
	if (FVector::Dist2D(Candidate.PlayerStartLocation, SpawnLocation) < ValidationRule.MinPlayerToEnemyDistance)
	{
		return false;
	}

	for (const FSIPResolvedEnemySpawn& ExistingEnemySpawn : Candidate.EnemySpawns)
	{
		if (FVector::Dist2D(ExistingEnemySpawn.Transform.GetLocation(), SpawnLocation) < ValidationRule.MinEnemyToEnemyDistance)
		{
			return false;
		}
	}

	return true;
}

FVector USIPEncounterPCGComponent::SamplePointInSquare(FRandomStream& RandomStream, const FVector& Center, float HalfExtent) const
{
	const float SafeHalfExtent = FMath::Max(0.0f, HalfExtent);
	return Center + FVector(
		RandomStream.FRandRange(-SafeHalfExtent, SafeHalfExtent),
		RandomStream.FRandRange(-SafeHalfExtent, SafeHalfExtent),
		0.0f);
}

FVector USIPEncounterPCGComponent::SamplePointNearTerrain(FRandomStream& RandomStream, const FSIPResolvedTerrainPatch& TerrainPatch, float Distance, float ArenaHalfExtent) const
{
	const float Angle = RandomStream.FRandRange(0.0f, UE_TWO_PI);
	const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
	const FVector TargetLocation = TerrainPatch.Transform.GetLocation() + Direction * Distance;

	FVector ProjectedLocation;
	const FVector2D LocalPoint(TargetLocation.X - GetEncounterOrigin().X, TargetLocation.Y - GetEncounterOrigin().Y);
	if (ProjectLocalPointToWorldSurface(ClampLocalPointToSquare(LocalPoint, ArenaHalfExtent, FVector2D::ZeroVector), ProjectedLocation))
	{
		return ProjectedLocation;
	}

	return TargetLocation;
}


int32 USIPEncounterPCGComponent::FindRandomTerrainIndexByElement(const FSIPEncounterCandidate& Candidate, const FGameplayTag& ElementTag, FRandomStream& RandomStream) const
{
	TArray<int32> MatchingTerrainIndices;
	for (int32 Index = 0; Index < Candidate.TerrainPatches.Num(); ++Index)
	{
		if (!ElementTag.IsValid() || Candidate.TerrainPatches[Index].ElementTag.MatchesTagExact(ElementTag))
		{
			MatchingTerrainIndices.Add(Index);
		}
	}

	if (!MatchingTerrainIndices.IsEmpty())
	{
		return MatchingTerrainIndices[RandomStream.RandRange(0, MatchingTerrainIndices.Num() - 1)];
	}

	return Candidate.TerrainPatches.IsEmpty() ? INDEX_NONE : RandomStream.RandRange(0, Candidate.TerrainPatches.Num() - 1);
}

int32 USIPEncounterPCGComponent::CountEnemiesAffectedByCoreReaction(const USIPEncounterPatternData& Pattern, const FSIPEncounterCandidate& Candidate) const
{
	int32 AffectedEnemyCount = 0;

	for (const FSIPResolvedEnemySpawn& EnemySpawn : Candidate.EnemySpawns)
	{
		if (!Candidate.TerrainPatches.IsValidIndex(EnemySpawn.LinkedTerrainIndex))
		{
			continue;
		}

		const FSIPResolvedTerrainPatch& TerrainPatch = Candidate.TerrainPatches[EnemySpawn.LinkedTerrainIndex];
		if (Pattern.RequiredTerrainElement.IsValid() && !TerrainPatch.ElementTag.MatchesTagExact(Pattern.RequiredTerrainElement))
		{
			continue;
		}

		const FVector EnemyLocation = EnemySpawn.Transform.GetLocation();
		const FVector TerrainLocation = TerrainPatch.Transform.GetLocation();
		const float DeltaX = FMath::Abs(EnemyLocation.X - TerrainLocation.X);
		const float DeltaY = FMath::Abs(EnemyLocation.Y - TerrainLocation.Y);
		if (DeltaX <= TerrainPatch.ZoneExtent.X && DeltaY <= TerrainPatch.ZoneExtent.Y)
		{
			++AffectedEnemyCount;
		}
	}

	return AffectedEnemyCount;
}

FSIPEncounterValidationResult USIPEncounterPCGComponent::ValidateCandidate(const USIPEncounterPatternData& Pattern, const FSIPEncounterCandidate& Candidate) const
{
	FSIPEncounterValidationResult Result;

	if (Candidate.TerrainPatches.IsEmpty())
	{
		Result.FailureReason = TEXT("Candidate has no terrain patches.");
		return Result;
	}

	if (Pattern.EnemyRules.Num() > 0 && Candidate.EnemySpawns.IsEmpty())
	{
		Result.FailureReason = TEXT("Candidate has enemy rules but no enemy spawns.");
		return Result;
	}

	if (Candidate.EnemySpawns.Num() < Candidate.RequestedEnemySpawnCount)
	{
		Result.FailureReason = TEXT("Candidate could not place all requested enemies with the current distance constraints.");
		return Result;
	}

	for (int32 FirstIndex = 0; FirstIndex < Candidate.TerrainPatches.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < Candidate.TerrainPatches.Num(); ++SecondIndex)
		{
			const float PairPadding = FMath::Min(Candidate.TerrainPatches[FirstIndex].SeparationPadding, Candidate.TerrainPatches[SecondIndex].SeparationPadding);
			if (DoTerrainPatchesOverlap2D(Candidate.TerrainPatches[FirstIndex], Candidate.TerrainPatches[SecondIndex], PairPadding))
			{
				Result.FailureReason = TEXT("Terrain patches overlap.");
				return Result;
			}
		}
	}

	if (!Pattern.NormalGroundClass)
	{
		Result.FailureReason = TEXT("Pattern has no normal ground class.");
		return Result;
	}

	if (Candidate.GroundPatches.IsEmpty())
	{
		Result.FailureReason = TEXT("Candidate has no normal ground patches.");
		return Result;
	}

	if (Pattern.bRequireGroundPathFromStartToExit && !HasGroundPathFromStartToExit(Candidate))
	{
		Result.FailureReason = TEXT("Normal ground has no path from player start to exit.");
		return Result;
	}

	const FSIPEncounterValidationRule& ValidationRule = Pattern.ValidationRule;
	if (ValidationRule.bRequirePlayerCanHitKeyTerrain)
	{
		bool bCanHitKeyTerrain = false;
		for (const FSIPResolvedTerrainPatch& TerrainPatch : Candidate.TerrainPatches)
		{
			if (!TerrainPatch.bKeyTerrain)
			{
				continue;
			}

			const float Distance = FVector::Dist2D(Candidate.PlayerStartLocation, TerrainPatch.Transform.GetLocation());
			if (Distance <= ValidationRule.PlayerAttackRange)
			{
				bCanHitKeyTerrain = true;
				Result.ScoreBonus += 30.0f;
				break;
			}
		}

		if (!bCanHitKeyTerrain)
		{
			Result.FailureReason = TEXT("Player cannot hit any key terrain patch.");
			return Result;
		}
	}

	if (ValidationRule.bRequireEnemyOnPreferredTerrain)
	{
		for (const FSIPResolvedEnemySpawn& EnemySpawn : Candidate.EnemySpawns)
		{
			if (!Candidate.TerrainPatches.IsValidIndex(EnemySpawn.LinkedTerrainIndex))
			{
				Result.FailureReason = TEXT("Enemy has no linked preferred terrain.");
				return Result;
			}
		}
	}

	if (Pattern.RequiredTerrainElement.IsValid())
	{
		bool bHasRequiredTerrain = false;
		for (const FSIPResolvedTerrainPatch& TerrainPatch : Candidate.TerrainPatches)
		{
			if (TerrainPatch.ElementTag.MatchesTagExact(Pattern.RequiredTerrainElement))
			{
				bHasRequiredTerrain = true;
				break;
			}
		}

		if (!bHasRequiredTerrain)
		{
			Result.FailureReason = TEXT("Candidate has no required terrain element.");
			return Result;
		}
	}

	if (Pattern.MinEnemiesAffectedByReaction > 0)
	{
		const int32 AffectedEnemyCount = CountEnemiesAffectedByCoreReaction(Pattern, Candidate);
		if (AffectedEnemyCount < Pattern.MinEnemiesAffectedByReaction)
		{
			Result.FailureReason = TEXT("Core reaction cannot affect enough enemies.");
			return Result;
		}

		Result.ScoreBonus += AffectedEnemyCount * 40.0f;
	}

	for (const FSIPResolvedEnemySpawn& EnemySpawn : Candidate.EnemySpawns)
	{
		const float Distance = FVector::Dist2D(Candidate.PlayerStartLocation, EnemySpawn.Transform.GetLocation());
		if (Distance < ValidationRule.MinPlayerToEnemyDistance)
		{
			Result.FailureReason = TEXT("Enemy spawned too close to player start.");
			return Result;
		}
	}

	for (int32 FirstIndex = 0; FirstIndex < Candidate.EnemySpawns.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < Candidate.EnemySpawns.Num(); ++SecondIndex)
		{
			const float Distance = FVector::Dist2D(Candidate.EnemySpawns[FirstIndex].Transform.GetLocation(), Candidate.EnemySpawns[SecondIndex].Transform.GetLocation());
			if (Distance < ValidationRule.MinEnemyToEnemyDistance)
			{
				Result.FailureReason = TEXT("Enemies spawned too close to each other.");
				return Result;
			}
		}
	}

	Result.bPassed = true;
	return Result;
}

float USIPEncounterPCGComponent::ScoreCandidate(const USIPEncounterPatternData& Pattern, const FSIPEncounterCandidate& Candidate, const FSIPEncounterValidationResult& ValidationResult) const
{
	float Score = 100.0f + ValidationResult.ScoreBonus;

	Score += Candidate.TerrainPatches.Num() * 15.0f;
	Score += Candidate.EnemySpawns.Num() * 20.0f;
	Score += Candidate.CoverPoints.Num() * 10.0f;
	Score += CountEnemiesAffectedByCoreReaction(Pattern, Candidate) * 40.0f;

	if (Pattern.ExitGateClass)
	{
		Score += 20.0f;
	}

	if (Pattern.RequiredPlayerElement.IsValid() && Pattern.ExpectedReaction.IsValid())
	{
		Score += 15.0f;
	}

	for (const FSIPResolvedEnemySpawn& EnemySpawn : Candidate.EnemySpawns)
	{
		if (Candidate.TerrainPatches.IsValidIndex(EnemySpawn.LinkedTerrainIndex))
		{
			const FSIPResolvedTerrainPatch& TerrainPatch = Candidate.TerrainPatches[EnemySpawn.LinkedTerrainIndex];
			const float Distance = FVector::Dist2D(EnemySpawn.Transform.GetLocation(), TerrainPatch.Transform.GetLocation());
			Score += FMath::Clamp(600.0f - Distance, 0.0f, 600.0f) * 0.05f;
		}
	}

	const float PlayerExitDistance = FVector::Dist2D(Candidate.PlayerStartLocation, Candidate.ExitLocation);
	const float PreferredDistance = GetArenaHalfExtent(Pattern, 600.0f);
	Score -= FMath::Abs(PlayerExitDistance - PreferredDistance) * 0.01f;

	return Score;
}

void USIPEncounterPCGComponent::SpawnCandidate(const FSIPEncounterCandidate& Candidate, const USIPEncounterPatternData& Pattern)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (const FSIPResolvedGroundPatch& GroundPatch : Candidate.GroundPatches)
	{
		if (!GroundPatch.GroundClass)
		{
			continue;
		}

		FActorSpawnParameters GroundSpawnParams = SpawnParams;
		GroundSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* GroundActor = World->SpawnActor<AActor>(GroundPatch.GroundClass, GroundPatch.Transform, GroundSpawnParams);
		if (!GroundActor)
		{
			continue;
		}

		if (ASIPElementReactiveZoneBase* GroundZone = Cast<ASIPElementReactiveZoneBase>(GroundActor))
		{
			GroundZone->ApplyGeneratedZoneExtent(GroundPatch.GroundExtent);
		}
		else
		{
			const FVector SafeBaseExtent(
				FMath::Max(Pattern.NormalGroundBaseExtent.X, KINDA_SMALL_NUMBER),
				FMath::Max(Pattern.NormalGroundBaseExtent.Y, KINDA_SMALL_NUMBER),
				FMath::Max(Pattern.NormalGroundBaseExtent.Z, KINDA_SMALL_NUMBER));
			GroundActor->SetActorScale3D(FVector(
				GroundPatch.GroundExtent.X / SafeBaseExtent.X,
				GroundPatch.GroundExtent.Y / SafeBaseExtent.Y,
				GroundPatch.GroundExtent.Z / SafeBaseExtent.Z));
		}

		SpawnedActors.Add(GroundActor);
	}

	for (const FSIPResolvedTerrainPatch& TerrainPatch : Candidate.TerrainPatches)
	{
		if (!TerrainPatch.ZoneClass)
		{
			continue;
		}

		ASIPElementReactiveZoneBase* Zone = World->SpawnActor<ASIPElementReactiveZoneBase>(TerrainPatch.ZoneClass, TerrainPatch.Transform, SpawnParams);
		if (!Zone)
		{
			continue;
		}

		Zone->ZoneElementTag = TerrainPatch.ElementTag;
		Zone->ApplyGeneratedZoneExtent(TerrainPatch.ZoneExtent);

		SpawnedActors.Add(Zone);
	}

	for (const FSIPResolvedCoverPoint& CoverPoint : Candidate.CoverPoints)
	{
		if (!CoverPoint.CoverClass)
		{
			continue;
		}

		AActor* CoverActor = World->SpawnActor<AActor>(CoverPoint.CoverClass, CoverPoint.Transform, SpawnParams);
		if (CoverActor)
		{
			SpawnedActors.Add(CoverActor);
		}
	}

	for (const FSIPResolvedEnemySpawn& EnemySpawn : Candidate.EnemySpawns)
	{
		if (!EnemySpawn.EnemyClass)
		{
			continue;
		}

		ASIPEnemyCharacter* Enemy = World->SpawnActor<ASIPEnemyCharacter>(EnemySpawn.EnemyClass, EnemySpawn.Transform, SpawnParams);
		if (Enemy)
		{
			SpawnedActors.Add(Enemy);
			ObjectiveEnemies.Add(Enemy);
		}
	}

	if (Pattern.ExitGateClass)
	{
		SpawnedExitGate = World->SpawnActor<ASIPEncounterExitGate>(Pattern.ExitGateClass, FTransform(FRotator::ZeroRotator, Candidate.ExitLocation), SpawnParams);
		if (SpawnedExitGate)
		{
			SpawnedActors.Add(SpawnedExitGate);
			SpawnedExitGate->SetGateUnlocked(!Pattern.bUnlockExitWhenAllEnemiesDefeated || ObjectiveEnemies.IsEmpty());
			// 将 Actor 覆盖优先、Pattern 兜底解析出的下一张地图写入到 Gate，交由玩家解锁交互时触发跳图。
			SpawnedExitGate->SetDestinationMap(ResolveNextMap(Pattern));
		}
	}

	SpawnMarker(Pattern.PlayerStartMarkerClass, Candidate.PlayerStartLocation, TEXT("EncounterPlayerStartMarker"));
	SpawnMarker(Pattern.ExitMarkerClass, Candidate.ExitLocation, TEXT("EncounterExitMarker"));
	SpawnMarker(Pattern.RewardMarkerClass, Candidate.RewardLocation, TEXT("EncounterRewardMarker"));
}

void USIPEncounterPCGComponent::SpawnMarker(TSubclassOf<AActor> MarkerClass, const FVector& Location, const FName& FallbackName)
{
	if (!MarkerClass || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Name = MakeUniqueObjectName(GetWorld(), MarkerClass, FallbackName);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* Marker = GetWorld()->SpawnActor<AActor>(MarkerClass, FTransform(FRotator::ZeroRotator, Location), SpawnParams);
	if (Marker)
	{
		SpawnedActors.Add(Marker);
	}
}

void USIPEncounterPCGComponent::DrawDebugCandidate(const FSIPEncounterCandidate& Candidate, const USIPEncounterPatternData& Pattern) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ArenaHalfExtent = GetArenaHalfExtent(Pattern, 300.0f);
	DrawDebugBox(World, GetEncounterOrigin(), FVector(ArenaHalfExtent, ArenaHalfExtent, 10.0f), FColor::Cyan, false, DebugDrawDuration, 0, 2.0f);
	DrawDebugSphere(World, Candidate.PlayerStartLocation + FVector(0.0f, 0.0f, 60.0f), 80.0f, 16, FColor::Green, false, DebugDrawDuration, 0, 4.0f);
	DrawDebugSphere(World, Candidate.ExitLocation + FVector(0.0f, 0.0f, 60.0f), 80.0f, 16, FColor::Blue, false, DebugDrawDuration, 0, 4.0f);
	DrawDebugLine(World, Candidate.PlayerStartLocation, Candidate.ExitLocation, FColor::Cyan, false, DebugDrawDuration, 0, 2.0f);

	for (const FSIPResolvedTerrainPatch& TerrainPatch : Candidate.TerrainPatches)
	{
		const FColor TerrainColor = TerrainPatch.bKeyTerrain ? FColor::Yellow : FColor::White;
		DrawDebugBox(World, TerrainPatch.Transform.GetLocation(), TerrainPatch.ZoneExtent, TerrainColor, false, DebugDrawDuration, 0, 3.0f);
	}

	for (const FSIPResolvedGroundPatch& GroundPatch : Candidate.GroundPatches)
	{
		DrawDebugBox(World, GroundPatch.Transform.GetLocation(), GroundPatch.GroundExtent, FColor::Green, false, DebugDrawDuration, 0, 1.0f);
	}

	for (const FSIPResolvedEnemySpawn& EnemySpawn : Candidate.EnemySpawns)
	{
		DrawDebugSphere(World, EnemySpawn.Transform.GetLocation(), 60.0f, 12, FColor::Red, false, DebugDrawDuration, 0, 3.0f);
		if (Candidate.TerrainPatches.IsValidIndex(EnemySpawn.LinkedTerrainIndex))
		{
			DrawDebugLine(World, EnemySpawn.Transform.GetLocation(), Candidate.TerrainPatches[EnemySpawn.LinkedTerrainIndex].Transform.GetLocation(), FColor::Orange, false, DebugDrawDuration, 0, 2.0f);
		}
	}
}

void USIPEncounterPCGComponent::StartObjectiveTracking(USIPEncounterPatternData* Pattern)
{
	if (!Pattern)
	{
		return;
	}

	if (!Pattern->bUnlockExitWhenAllEnemiesDefeated || ObjectiveEnemies.IsEmpty())
	{
		CompleteEncounter();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ObjectiveCheckTimerHandle,
			this,
			&USIPEncounterPCGComponent::CheckObjectiveState,
			FMath::Max(0.05f, Pattern->ObjectiveCheckInterval),
			true);
	}
}

void USIPEncounterPCGComponent::CheckObjectiveState()
{
	if (bEncounterCompleted)
	{
		return;
	}

	ObjectiveEnemies.RemoveAll([](const TObjectPtr<ASIPEnemyCharacter>& Enemy)
	{
		return !IsValid(Enemy) || Enemy->IsDeadOrDying();
	});

	if (ObjectiveEnemies.IsEmpty())
	{
		CompleteEncounter();
	}
}

void USIPEncounterPCGComponent::CompleteEncounter()
{
	if (bEncounterCompleted)
	{
		return;
	}

	bEncounterCompleted = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ObjectiveCheckTimerHandle);
	}

	if (SpawnedExitGate)
	{
		SpawnedExitGate->SetGateUnlocked(true);
	}

	K2_OnEncounterCompleted();

	UE_LOG(LogSIP, Log, TEXT("Encounter PCG objective completed."));
}

TSoftObjectPtr<UWorld> USIPEncounterPCGComponent::ResolveNextMap(const USIPEncounterPatternData& Pattern) const
{
	// Actor 级 override 优先；仅当 override 为空时才回落到 Pattern 中配置的默认地图。
	if (!NextMapOverride.IsNull())
	{
		return NextMapOverride;
	}

	return Pattern.NextMap;
}
