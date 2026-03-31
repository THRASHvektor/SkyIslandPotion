// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * ASIPIslandSpawner 是整个岛屿世界的全局调度器
 * 关卡中只放置一个，负责在运行时批量生成所有悬浮岛
 *
 * 生成策略：
 * 1. 以 Spawner 自身位置为圆心，在 SpawnRadius 范围内随机极坐标散布
 * 2. 中心区域（<0.25x半径）留空，形成玩家出生点的开阔感
 * 3. 间距检查：候选位置与已生成岛的距离 < MinIslandSpacing 时放弃并重试
 * 4. 最大重试次数 = IslandCount * 10，防止死循环（半径太小导致放不下）
 * 5. Biome 按 BiomeWeights 加权随机选取
 *
 * WorldSeed 机制：
 * - 同一 WorldSeed 每次生成完全相同的世界布局（可用于存档/分享）
 * - 每个岛的子 Seed = WorldSeed + IslandIndex * 1000
 * - 只需改变 WorldSeed 即可得到全新世界
 *
 * Blueprint 工作流：
 * 1. 创建 BP_IslandSpawner 子类
 * 2. 设置 IslandClass = BP_IslandActor（或具体 Biome 子类）
 * 3. 配置 BiomeWeights（如 Biome.Forest=3, Biome.Fire=1, Biome.Ice=1）
 * 4. 调整 IslandCount、SpawnRadius、WorldSeed
 * 5. 放入关卡，PIE 即可看到岛屿散布效果
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Math/RandomStream.h"
#include "SIPIslandSpawner.generated.h"

class ASIPIslandActor;
class ASIPElementalZoneActor;

USTRUCT(BlueprintType)
struct FSIPBiomeSpawnBand
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Island|Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinRadiusRatio = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Island|Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxRadiusRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Island|Biome")
	float HeightOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Island|Biome", meta = (ClampMin = "0.0"))
	float HeightScale = 1.0f;
};

UCLASS(Blueprintable)
class SIP_API ASIPIslandSpawner : public AActor
{
	GENERATED_BODY()

public:
	ASIPIslandSpawner();

protected:
	virtual void BeginPlay() override;

public:
	// ===== 生成控制 =====

	/**
	 * 触发所有岛屿的批量生成
	 * BeginPlay 时自动调用；也可在关卡蓝图中手动调用（如需延迟生成）
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Island")
	void SpawnAllIslands();

	/**
	 * 销毁所有已生成的岛屿实例并清空列表
	 * 可用于世界重置或切换 WorldSeed
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Island")
	void ClearAllIslands();

	// ===== 配置：岛屿类 =====

	/**
	 * 要生成的岛屿 Actor 类
	 * 在 BP_IslandSpawner 子类中设置为 BP_IslandActor（或具体 Biome 子类）
	 * 若未设置，BeginPlay 会输出 Error 日志并跳过生成
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Island|Setup")
	TSubclassOf<ASIPIslandActor> IslandClass;

	// ===== 配置：生成参数 =====

	/** 生成岛屿总数 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "1", ClampMax = "200"))
	int32 IslandCount = 12;

	/**
	 * 水平散布半径（cm）
	 * 岛屿在以 Spawner 为圆心、此半径的圆形区域内随机分布
	 * 推荐：IslandCount * MinIslandSpacing * 0.8 左右，保证放得下
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout")
	float SpawnRadius = 20000.f;

	/**
	 * Z 轴高度随机范围（cm）
	 * 岛屿在 Spawner 位置上下此范围内浮动
	 * 例：3000 → 岛屿 Z 在 [-1500, +1500] 范围内随机
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout")
	float HeightVariance = 3000.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "1", ClampMax = "32"))
	int32 LayoutCandidateCount = 8;

	/**
	 * 岛屿间最小间距（cm）
	 * 防止岛屿网格重叠
	 * 建议 >= 岛屿 BoxExtent.X * 2（默认 2000）
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout")
	float MinIslandSpacing = 2500.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float IslandFootprintPadding = 250.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float IslandVerticalPadding = 200.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float SeamBandWidth = 600.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float SeamPlacementBias = 150.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float SameBiomeAttractionRadius = 6500.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float SameBiomeAttractionScore = 900.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float DifferentBiomeAvoidanceRadius = 5000.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Layout", meta = (ClampMin = "0.0"))
	float DifferentBiomeAvoidancePenalty = 1100.f;

	// ===== 配置：随机种子 =====

	/**
	 * 全局 WorldSeed
	 * 决定整个世界的布局（岛屿位置、Biome 分配、PCG 植被）
	 * 同一 Seed 每次运行生成完全相同的世界
	 * 修改此值即可得到全新世界布局
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Seed")
	int32 WorldSeed = 42;

	// ===== 配置：Biome 权重 =====

	/**
	 * Biome 类型权重表
	 * Key:   Biome Tag（SIPGameplayTags::Biome_Forest 等）
	 * Value: 出现权重（相对值，总和不必为1）
	 *
	 * 示例配置：
	 *   Biome.Forest = 3  → 60%
	 *   Biome.Fire   = 1  → 20%
	 *   Biome.Ice    = 1  → 20%
	 *
	 * 若为空，所有岛屿 BiomeType 为空（PCG 使用默认 Graph）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Island|Biome")
	TMap<FGameplayTag, float> BiomeWeights;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Island|Biome")
	TMap<FGameplayTag, float> BiomeFootprintScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Island|Biome")
	TMap<FGameplayTag, FSIPBiomeSpawnBand> BiomeSpawnBands;

	// ===== 配置：Zone 生成 =====

	/**
	 * 每种 Biome 对应的 ZoneActor 子类
	 * 在 BP_IslandSpawner 中配置：Forest → BP_Zone_Forest，Fire → BP_Zone_Fire 等
	 * 若某 Biome 未配置则该 Biome 的岛不生成 Zone
	 */
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Island|Zones")
	TMap<FGameplayTag, TSubclassOf<ASIPElementalZoneActor>> ZoneClassPerBiome;

	/** 每个岛屿生成的 ZoneActor 数量（0 = 不生成）*/
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Zones", meta = (ClampMin = "0", ClampMax = "10"))
	int32 ZonesPerIsland = 1;

	/**
	 * LineTrace 发射起点距岛屿中心的高度偏移（cm）
	 * 需大于岛屿 Mesh 的实际高度，默认 3000（约 30m）
	 */
	UPROPERTY(EditInstanceOnly, Category = "SIP|Island|Zones")
	float ZoneTraceHeight = 3000.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Zones", meta = (ClampMin = "1", ClampMax = "32"))
	int32 ZonePlacementAttemptsPerZone = 8;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island|Zones", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ZoneMinSpacingRatio = 0.3f;

	// ===== 只读：运行时状态 =====

	/** 当前已生成的岛屿数量（只读，供蓝图和调试查看） */
	UFUNCTION(BlueprintPure, Category = "SIP|Island")
	int32 GetSpawnedIslandCount() const { return SpawnedIslands.Num(); }

private:
	/** 已生成的岛屿实例列表（Transient，不序列化）*/
	UPROPERTY(Transient)
	TArray<TObjectPtr<ASIPIslandActor>> SpawnedIslands;

	/**
	 * 按 BiomeWeights 加权随机选取一个 Biome Tag
	 * @param RandStream 外部传入的随机流（保证整体 Seed 确定性）
	 */
	FGameplayTag PickRandomBiome(FRandomStream& RandStream) const;

	/**
	 * 检查候选位置是否满足最小间距要求
	 * @param Candidate  候选世界坐标
	 * @param MinSpacing 最小间距阈值
	 */
	bool IsLocationValid(
		const FVector& Candidate,
		float CandidateRadius,
		float CandidateHalfHeight,
		TArray<ASIPIslandActor*>& OutNearbyIslands) const;

	float GetCandidatePlacementRadius(const FGameplayTag& Biome) const;
	float GetCandidatePlacementHalfHeight(const FGameplayTag& Biome) const;
	FVector SampleCandidateLocation(FRandomStream& RandStream, const FVector& Origin, const FGameplayTag& Biome) const;
	float ScoreCandidateLocation(
		const FVector& Candidate,
		const FGameplayTag& CandidateBiome,
		float CandidateRadius,
		float CandidateHalfHeight,
		int32 NearbyIslandCount) const;

	/**
	 * 在已生成的岛屿 Mesh 表面用 LineTrace 找到落点，Spawn ZoneActor
	 * @param Island      目标岛屿 Actor
	 * @param Biome       该岛的 Biome Tag（用于查 ZoneClassPerBiome）
	 * @param IslandSeed  该岛的子 Seed（WorldSeed + Index*1000），保证 Zone 位置可复现
	 */
	void SpawnZonesOnIsland(ASIPIslandActor* Island, const FGameplayTag& Biome, int32 IslandSeed);
};
