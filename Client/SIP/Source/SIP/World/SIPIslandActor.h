// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * ASIPIslandActor 是单个悬浮岛屿的根 Actor
 *
 * 职责：
 * 1. 承载岛屿视觉网格（IslandMesh）——初期用 Prototype，后期替换风格化美术资产
 * 2. 提供 PCG 采样边界盒（IslandBounds）——IslandGeneratorComponent 以此框定地表采样范围
 * 3. 通过 SIPIslandGeneratorComponent 驱动表面植被/矿物/生成点的程序化生成
 * 4. 持有 BiomeType，决定该岛的元素风格（Forest / Fire / Ice / Plains）
 *
 * 生命周期：
 * - 由 ASIPIslandSpawner 在运行时批量生成，不需要手动放置
 * - Spawner 调用 InitializeIsland() 注入 Biome 和 Seed，然后触发 PCG 生成
 *
 * Blueprint 工作流：
 * - 为每种 Biome 创建 BP 子类（BP_IslandActor_Forest、BP_IslandActor_Fire 等）
 * - 在 BP 子类中：
 *   1. 替换 IslandMesh 为对应 Biome 的风格化石块资产
 *   2. 在 IslandGenerator 的 PCGGraphPerBiome 中指定对应 Biome 的 PCG Graph
 *   3. 调整 IslandBounds 的 BoxExtent 匹配网格实际尺寸
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SIPIslandActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USIPIslandGeneratorComponent;
class UStaticMesh;

/** TMap 嵌套 TArray 时 UHT 不支持，用此 Wrapper 绕开 */
USTRUCT(BlueprintType)
struct FSIPMeshVariantList
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<TObjectPtr<UStaticMesh>> Meshes;
};

UCLASS(Blueprintable)
class SIP_API ASIPIslandActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPIslandActor();

protected:
	virtual void BeginPlay() override;

public:
	// ===== 组件 =====

	/**
	 * PCG 采样边界盒（根组件）
	 * IslandGeneratorComponent 的 PCGComponent 以此框定地表采样范围
	 * BoxExtent 默认 1000x1000x400，在 BP 子类中按网格实际尺寸调整
	 * 【重要】Z 轴下沿需覆盖到网格顶面以下，否则 PCG Surface Sampler 采不到点
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Island")
	TObjectPtr<UBoxComponent> IslandBounds;

	/**
	 * 岛屿视觉网格（悬浮石块主体）
	 * CollisionProfile = BlockAll，供玩家站立
	 * 初期用 LevelPrototyping 的 SM_Cube 占位，后期替换风格化手工美术资产
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Island")
	TObjectPtr<UStaticMeshComponent> IslandMesh;

	/**
	 * PCG 植被/矿物/生成点组件（复用已有 SIPIslandGeneratorComponent）
	 * bGenerateOnBeginPlay = false：不自动生成，由 InitializeIsland() 手动触发
	 * 配置方式：在 BP 子类中，选中 IslandGenerator，在 PCGGraphPerBiome 中指定各 Biome 的 Graph 资产
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Island")
	TObjectPtr<USIPIslandGeneratorComponent> IslandGenerator;

	/**
	 * 每种 Biome 对应的岛屿网格变体列表
	 * InitializeIsland 时根据 IslandSeed 随机选取其中一个，同 Seed 永远选同一个
	 * 在 BP 子类中配置：例如 Forest → [SM_Rock_Flat, SM_Rock_Peak, SM_Rock_Basin]
	 * 若为空则保持 BP 子类默认网格不变
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Island|Appearance")
	TMap<FGameplayTag, FSIPMeshVariantList> MeshVariantsPerBiome;

	// ===== 运行时数据 =====

	/**
	 * 岛屿 Biome 类型（由 ASIPIslandSpawner 在 Spawn 时注入）
	 * 决定 PCG Graph 选择和未来的视觉材质参数
	 * 对应 SIPGameplayTags 中的 Biome_Forest / Biome_Fire / Biome_Ice / Biome_Plains
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Island")
	FGameplayTag BiomeType;

	/**
	 * 岛屿索引（由 Spawner 注入，从 0 开始）
	 * 用于派生确定性子 Seed：IslandSeed = WorldSeed + IslandIndex * 1000
	 * 同一 WorldSeed 下同一 Index 永远生成相同岛屿
	 */
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Island")
	int32 IslandIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Island")
	bool bIsSeamCandidate = false;

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Island")
	int32 NearbyIslandCount = 0;

	// ===== 接口 =====

	/**
	 * 由 ASIPIslandSpawner 调用，完成 Biome 和 Seed 注入后触发 PCG 生成
	 * 必须在 BeginPlay 之后调用（PCGWorldActor 需要已存在于关卡中）
	 *
	 * @param InBiome    Biome 类型 Tag（Biome.Forest 等）
	 * @param InIndex    岛屿索引，用于 Seed 派生
	 * @param WorldSeed  全局 WorldSeed（由 Spawner 传入，保证整个世界可复现）
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Island")
	void InitializeIsland(const FGameplayTag& InBiome, int32 InIndex, int32 WorldSeed);

	UFUNCTION(BlueprintPure, Category = "SIP|Island")
	float GetPlacementRadius2D() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Island")
	float GetPlacementHalfHeight() const;

	void RegisterNearbyIsland(ASIPIslandActor* OtherIsland);

	/**
	 * 蓝图事件：Biome 注入完成后通知 BP 子类
	 * BP 可在此切换材质、调整后处理颜色等视觉表现
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Island", DisplayName = "On Biome Initialized")
	void K2_OnBiomeInitialized(const FGameplayTag& InBiome);
};
