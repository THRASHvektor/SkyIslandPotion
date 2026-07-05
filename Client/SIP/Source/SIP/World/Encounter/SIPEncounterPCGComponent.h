#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "World/Encounter/SIPEncounterPCGTypes.h"
#include "SIPEncounterPCGComponent.generated.h"

class ASIPElementReactiveZoneBase;
class USIPEncounterPatternData;
class ASIPEnemyCharacter;
class ASIPEncounterExitGate;

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPEncounterPCGComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPEncounterPCGComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "SIP|Encounter PCG")
	void GenerateEncounter(int32 Seed = 0);

	UFUNCTION(BlueprintCallable, Category = "SIP|Encounter PCG")
	void ClearEncounter();

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Encounter PCG", DisplayName = "On Encounter Completed")
	void K2_OnEncounterCompleted();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG")
	bool bGenerateOnBeginPlay = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG", meta = (ClampMin = "1"))
	int32 LevelIndex = 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG")
	int32 DefaultSeed = 42;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG", meta = (ClampMin = "1", ClampMax = "64"))
	int32 CandidateCount = 12;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG")
	TArray<TObjectPtr<USIPEncounterPatternData>> Patterns;

	// Actor 级的下一张地图覆盖项，优先级高于 Pattern.NextMap。留空时使用 Pattern 自带的配置。
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Encounter PCG|Exit", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> NextMapOverride;

	// 运行时设置 Actor 级地图覆盖（可在生成前由上一层流程传入）。
	UFUNCTION(BlueprintCallable, Category = "SIP|Encounter PCG|Exit")
	void SetNextMapOverride(const TSoftObjectPtr<UWorld>& InNextMap) { NextMapOverride = InNextMap; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG|Projection", meta = (ClampMin = "0.0"))
	float ProjectionTraceHalfHeight = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG|Projection")
	TEnumAsByte<ECollisionChannel> ProjectionTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG|Terrain", meta = (ClampMin = "0.0"))
	float TerrainPatchSeparationPadding = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG|Terrain", meta = (ClampMin = "1", ClampMax = "128"))
	int32 TerrainPatchPlacementAttempts = 24;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG|Enemy", meta = (ClampMin = "1", ClampMax = "128"))
	int32 EnemySpawnPlacementAttempts = 32;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter PCG|Debug", meta = (ClampMin = "0.0", EditCondition = "bDrawDebug", EditConditionHides))
	float DebugDrawDuration = 10.0f;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASIPEnemyCharacter>> ObjectiveEnemies;

	UPROPERTY(Transient)
	TObjectPtr<ASIPEncounterExitGate> SpawnedExitGate;

	FTimerHandle ObjectiveCheckTimerHandle;
	bool bEncounterCompleted = false;

	USIPEncounterPatternData* SelectPattern(FRandomStream& RandomStream) const;
	bool BuildCandidate(const USIPEncounterPatternData& Pattern, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const;
	void GenerateSingleClusterLayout(const USIPEncounterPatternData& Pattern, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const;
	void GenerateThreeElementClusterLayout(const USIPEncounterPatternData& Pattern, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const;

	bool ProjectLocalPointToWorldSurface(const FVector2D& LocalPoint, FVector& OutWorldLocation) const;
	FVector GetEncounterOrigin() const;
	float GetArenaHalfExtent(const USIPEncounterPatternData& Pattern, float MinHalfExtent) const;
	FVector2D ClampLocalPointToSquare(const FVector2D& LocalPoint, float HalfExtent, const FVector2D& Padding) const;
	FVector ResolveTerrainZoneExtent(const FSIPTerrainClusterRule& Rule, FRandomStream& RandomStream) const;
	bool TryBuildConnectedTerrainPatch(const FSIPTerrainClusterRule& Rule, float ArenaHalfExtent, const FVector& ZoneExtent, bool bKeyTerrain, FRandomStream& RandomStream, const FSIPEncounterCandidate& Candidate, FSIPResolvedTerrainPatch& OutTerrainPatch) const;
	bool TryAddTerrainPatch(const FSIPTerrainClusterRule& Rule, const FVector2D& ClusterCenter, float ArenaHalfExtent, bool bKeyTerrain, FRandomStream& RandomStream, FSIPEncounterCandidate& OutCandidate) const;
	bool CanPlaceTerrainPatch(const FSIPResolvedTerrainPatch& NewPatch, const FSIPEncounterCandidate& Candidate) const;
	bool DoTerrainPatchesOverlap2D(const FSIPResolvedTerrainPatch& FirstPatch, const FSIPResolvedTerrainPatch& SecondPatch, float Padding) const;
	void FillNormalGroundPatches(const USIPEncounterPatternData& Pattern, FSIPEncounterCandidate& OutCandidate) const;
	void AddMergedGroundPatch(const USIPEncounterPatternData& Pattern, const TArray<float>& XBreaks, const TArray<float>& YBreaks, int32 StartXIndex, int32 StartYIndex, int32 EndXIndex, int32 EndYIndex, FSIPEncounterCandidate& OutCandidate) const;
	bool IsPointInsideGroundPatch2D(const FVector& Point, const FSIPResolvedGroundPatch& GroundPatch) const;
	bool AreGroundPatchesConnected2D(const FSIPResolvedGroundPatch& FirstPatch, const FSIPResolvedGroundPatch& SecondPatch) const;
	bool HasGroundPathFromStartToExit(const FSIPEncounterCandidate& Candidate) const;
	bool TryBuildEnemySpawn(const USIPEncounterPatternData& Pattern, const FSIPEnemySpawnRule& Rule, float ArenaHalfExtent, FRandomStream& RandomStream, FSIPEncounterCandidate& Candidate) const;
	bool IsEnemySpawnLocationValid(const USIPEncounterPatternData& Pattern, const FVector& SpawnLocation, const FSIPEncounterCandidate& Candidate) const;
	FVector SamplePointInSquare(FRandomStream& RandomStream, const FVector& Center, float HalfExtent) const;
	FVector SamplePointNearTerrain(FRandomStream& RandomStream, const FSIPResolvedTerrainPatch& TerrainPatch, float Distance, float ArenaHalfExtent) const;
	int32 FindRandomTerrainIndexByElement(const FSIPEncounterCandidate& Candidate, const FGameplayTag& ElementTag, FRandomStream& RandomStream) const;
	int32 CountEnemiesAffectedByCoreReaction(const USIPEncounterPatternData& Pattern, const FSIPEncounterCandidate& Candidate) const;

	FSIPEncounterValidationResult ValidateCandidate(const USIPEncounterPatternData& Pattern, const FSIPEncounterCandidate& Candidate) const;
	float ScoreCandidate(const USIPEncounterPatternData& Pattern, const FSIPEncounterCandidate& Candidate, const FSIPEncounterValidationResult& ValidationResult) const;
	void SpawnCandidate(const FSIPEncounterCandidate& Candidate, const USIPEncounterPatternData& Pattern);
	void SpawnMarker(TSubclassOf<AActor> MarkerClass, const FVector& Location, const FName& FallbackName);
	void DrawDebugCandidate(const FSIPEncounterCandidate& Candidate, const USIPEncounterPatternData& Pattern) const;
	void StartObjectiveTracking(USIPEncounterPatternData* Pattern);
	void CheckObjectiveState();
	void CompleteEncounter();

	// 按 Actor Override -> Pattern 默认的优先级解析出 ExitGate 应前往的下一张地图。
	TSoftObjectPtr<UWorld> ResolveNextMap(const USIPEncounterPatternData& Pattern) const;
};
