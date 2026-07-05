#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/Encounter/SIPEncounterPCGTypes.h"
#include "SIPEncounterPatternData.generated.h"

class ASIPEncounterExitGate;

UCLASS(BlueprintType)
class SIP_API USIPEncounterPatternData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter")
	FGameplayTag PatternTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter", meta = (ClampMin = "1"))
	int32 LevelIndex = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter", meta = (ClampMin = "0.0"))
	float ArenaHalfExtent = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter")
	ESIPEncounterPatternMode PatternMode = ESIPEncounterPatternMode::SingleCluster;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Ground")
	TSubclassOf<AActor> NormalGroundClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Ground")
	FVector NormalGroundBaseExtent = FVector(300.0f, 300.0f, 40.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Ground", meta = (ClampMin = "1.0"))
	float GroundPatchHalfHeight = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Ground")
	bool bRequireGroundPathFromStartToExit = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Terrain")
	TArray<FSIPTerrainClusterRule> TerrainRules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Enemy")
	TArray<FSIPEnemySpawnRule> EnemyRules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Validation")
	FSIPEncounterValidationRule ValidationRule;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Objective")
	FGameplayTag RequiredPlayerElement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Objective")
	FGameplayTag RequiredTerrainElement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Objective")
	FGameplayTag ExpectedReaction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Objective", meta = (ClampMin = "0"))
	int32 MinEnemiesAffectedByReaction = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Objective")
	bool bUnlockExitWhenAllEnemiesDefeated = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Objective", meta = (ClampMin = "0.05", EditCondition = "bUnlockExitWhenAllEnemiesDefeated", EditConditionHides))
	float ObjectiveCheckInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Debug")
	TSubclassOf<AActor> PlayerStartMarkerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Debug")
	TSubclassOf<AActor> ExitMarkerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Exit")
	TSubclassOf<ASIPEncounterExitGate> ExitGateClass;

	// 玩家使用出口时默认要传送到的下一张地图（Pattern 级默认，可被 EncounterPCGActor 覆盖）。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Exit", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> NextMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Debug")
	TSubclassOf<AActor> RewardMarkerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Encounter|Cover")
	TSubclassOf<AActor> DefaultCoverClass;
};
