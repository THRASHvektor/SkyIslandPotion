// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * USIPIslandGeneratorComponent 是岛屿 PCG 生成的管理组件
 * 挂在岛屿根 Actor 上（或关卡中的 IslandManager Actor）
 *
 * 功能：
 * 1. 根据 BiomeType 生成对应岛屿的植被、矿物、敌人生成点
 * 2. 每种 Biome 对应一个独立配置的 PCG Graph（在 Editor 中指定）
 * 3. 提供 GenerateIsland() / ClearIsland() 供关卡蓝图/传送系统调用
 *
 * 与 SIPElementalZoneActor 的区别：
 * - 本组件负责整个岛屿的大尺度初始生成（一次性）
 * - ZoneActor 负责局部区域的运行时元素反应（动态）
 *
 * PCG 插件使用前提：
 * - 编辑器中启用 PCG 插件（Edit → Plugins → Procedural Content Generation）
 * - 为每种 Biome 制作独立 PCG Graph Asset 并在此处指定
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SIPIslandGeneratorComponent.generated.h"

class UPCGComponent;
class UPCGGraphInterface;

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPIslandGeneratorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPIslandGeneratorComponent();

protected:
	virtual void BeginPlay() override;

public:
	/**
	 * 触发 PCG 生成
	 * 关卡蓝图或传送门系统在玩家进入岛屿时调用
	 * @param Seed  随机种子（0 = 使用 DefaultSeed）
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Island")
	void GenerateIsland(int32 Seed = 0);

	/**
	 * 清除 PCG 生成的所有内容
	 * 玩家离开岛屿时调用以释放内存
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Island")
	void ClearIsland();

	/** 是否在 BeginPlay 时自动生成 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Island")
	bool bGenerateOnBeginPlay = false;

	/**
	 * 当前岛��的 Biome 类型
	 * 用于日志和调试，实际生成逻辑由 PCGGraphPerBiome 决定
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SIP|Island")
	FGameplayTag BiomeType;

	/**
	 * 每种 Biome 对应的 PCG Graph
	 * 在编辑器中配置：Key = Biome Tag, Value = PCG Graph Asset
	 * PCG Graph 内部控制该岛屿的植被密度、矿物分布、敌人生成点等
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Island")
	TMap<FGameplayTag, TObjectPtr<UPCGGraphInterface>> PCGGraphPerBiome;

	/** 默认随机种子 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SIP|Island")
	int32 DefaultSeed = 42;

private:
	/** 运行时获取或创建 Owner 上的 PCGComponent */
	UPCGComponent* GetOrCreatePCGComponent();
};
