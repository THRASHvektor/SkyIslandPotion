#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SIPEncounterPCGTypes.generated.h"

class AActor;
class ASIPElementReactiveZoneBase;
class ASIPEnemyCharacter;

UENUM(BlueprintType)
enum class ESIPEncounterPatternMode : uint8
{
	SingleCluster,
	MultipleCluster
};

UENUM(BlueprintType)
enum class ESIPEnemyTerrainRelation : uint8
{
	OnTerrain,
	NearTerrain,
	EdgeOfTerrain,
	OnSafeGround,
	OnHighGround
};

USTRUCT(BlueprintType)
struct FSIPTerrainClusterRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain")
	FGameplayTag ElementTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain")
	TSubclassOf<ASIPElementReactiveZoneBase> ZoneClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (ClampMin = "0"))
	int32 MinCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (ClampMin = "0"))
	int32 MaxCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (ClampMin = "0.0"))
	float ClusterHalfExtent = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (EditCondition = "!bRandomizeZoneExtent", EditConditionHides))
	FVector ZoneExtent = FVector(300.0f, 300.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain")
	bool bRandomizeZoneExtent = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (EditCondition = "bRandomizeZoneExtent", EditConditionHides))
	FVector MinRandomZoneExtent = FVector(200.0f, 200.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (EditCondition = "bRandomizeZoneExtent", EditConditionHides))
	FVector MaxRandomZoneExtent = FVector(500.0f, 500.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain")
	bool bAllowConnectedZones = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bAllowConnectedZones", EditConditionHides))
	float ConnectedZonePlacementChance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct FSIPEnemySpawnRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Enemy")
	TSubclassOf<ASIPEnemyCharacter> EnemyClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Enemy")
	FGameplayTag PreferredTerrainElement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Enemy")
	ESIPEnemyTerrainRelation TerrainRelation = ESIPEnemyTerrainRelation::NearTerrain;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Enemy", meta = (ClampMin = "0"))
	int32 MinCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Enemy", meta = (ClampMin = "0"))
	int32 MaxCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Enemy", meta = (ClampMin = "0.0"))
	float SpawnSpread = 300.0f;
};

USTRUCT(BlueprintType)
struct FSIPEncounterValidationRule
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Validation", meta = (ClampMin = "0.0", EditCondition = "bRequirePlayerCanHitKeyTerrain", EditConditionHides))
	float PlayerAttackRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Validation", meta = (ClampMin = "0.0"))
	float MinPlayerToEnemyDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Validation", meta = (ClampMin = "0.0"))
	float MinEnemyToEnemyDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Validation")
	bool bRequireEnemyOnPreferredTerrain = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Validation")
	bool bRequirePlayerCanHitKeyTerrain = true;
};

USTRUCT()
struct FSIPResolvedTerrainPatch
{
	GENERATED_BODY()

	FGameplayTag ElementTag;
	TSubclassOf<ASIPElementReactiveZoneBase> ZoneClass;
	FTransform Transform = FTransform::Identity;
	FVector ZoneExtent = FVector(300.0f, 300.0f, 120.0f);
	float SeparationPadding = 50.0f;
	bool bKeyTerrain = false;
};

USTRUCT()
struct FSIPResolvedEnemySpawn
{
	GENERATED_BODY()

	TSubclassOf<ASIPEnemyCharacter> EnemyClass;
	FGameplayTag PreferredTerrainElement;
	ESIPEnemyTerrainRelation TerrainRelation = ESIPEnemyTerrainRelation::NearTerrain;
	FTransform Transform = FTransform::Identity;
	int32 LinkedTerrainIndex = INDEX_NONE;
};

USTRUCT()
struct FSIPResolvedCoverPoint
{
	GENERATED_BODY()

	TSubclassOf<AActor> CoverClass;
	FTransform Transform = FTransform::Identity;
};

USTRUCT()
struct FSIPResolvedGroundPatch
{
	GENERATED_BODY()

	TSubclassOf<AActor> GroundClass;
	FTransform Transform = FTransform::Identity;
	FVector GroundExtent = FVector(300.0f, 300.0f, 40.0f);
};

USTRUCT()
struct FSIPEncounterCandidate
{
	GENERATED_BODY()

	FVector PlayerStartLocation = FVector::ZeroVector;
	FVector ExitLocation = FVector::ZeroVector;
	FVector RewardLocation = FVector::ZeroVector;

	TArray<FSIPResolvedTerrainPatch> TerrainPatches;
	TArray<FSIPResolvedGroundPatch> GroundPatches;
	TArray<FSIPResolvedEnemySpawn> EnemySpawns;
	TArray<FSIPResolvedCoverPoint> CoverPoints;

	int32 RequestedEnemySpawnCount = 0;
	float Score = 0.0f;
};

USTRUCT()
struct FSIPEncounterValidationResult
{
	GENERATED_BODY()

	bool bPassed = false;
	float ScoreBonus = 0.0f;
	FString FailureReason;
};
