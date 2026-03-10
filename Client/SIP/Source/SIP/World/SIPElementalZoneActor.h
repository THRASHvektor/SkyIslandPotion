// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * ASIPElementalZoneActor 是放置在关卡中的元素区域标记 Actor
 *
 * 工作原理：
 * 1. 设计师在关卡中放置此 Actor，设置 ZoneElementTag（该区域的元素属性）
 * 2. SIPPotionProjectile 落地时进行球形检测，找到范围内的所有 ZoneActor
 * 3. ZoneActor.ReceiveElementHit(IncomingElement, ImpactLocation) 被调用
 * 4. 内部查询 SIPElementReactionSubsystem，得到 ReactionTag
 * 5. 根据 ReactionTag 执行可见变化：
 *    - 清理 PCGComponent 生成的内容（移除树木/矿物等）
 *    - 显示/隐藏关联的 LinkedActors（手动放置的网格体）
 *    - 生成对应的 VFX
 *    - 召唤 ReactionSpawnActors（如冰平台、焦土特效 Actor）
 * 6. 记录 ZoneStateTag，防止重复触发相同反应
 *
 * Designer 工作流：
 * - 直接放此 Actor 到关卡，右键 → Add Component → PCGComponent（可选）
 * - 填写 ZoneElementTag = Element.Plant（森林区）
 * - 将周围的树木 Actor 拖进 LinkedDefaultActors
 * - 为每种反应指定 ReactionVFXMap 和 ReactionSpawnClassMap
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SIPElementalZoneActor.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class UPCGComponent;

UCLASS(Blueprintable)
class SIP_API ASIPElementalZoneActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPElementalZoneActor();

protected:
	virtual void BeginPlay() override;

public:
	/**
	 * 被 SIPPotionProjectile 命中时调用
	 * @param IncomingElement 药水弹丸携带的元素
	 * @param ImpactLocation  命中位置
	 */
	void ReceiveElementHit(const FGameplayTag& IncomingElement, const FVector& ImpactLocation);

	/** 当前区域元素类型（如 Element.Plant、Element.Ice）*/
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "SIP|Zone")
	FGameplayTag ZoneElementTag;

	/**
	 * 关联的"默认状态" Actor 列表
	 * 发生反应时这些 Actor 会被隐藏（树木、冰柱等）
	 * 在关卡中直接将对应 Actor 拖入此列表
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Zone")
	TArray<TObjectPtr<AActor>> LinkedDefaultActors;

	/**
	 * 每种反应类型对应要生成的 Actor 类（如焦土模型、冰平台 BP）
	 * Key: ReactionTag（Reaction.Burn 等）
	 * Value: Actor 子类
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Zone")
	TMap<FGameplayTag, TSubclassOf<AActor>> ReactionSpawnClassMap;

	/**
	 * 每种反应类型对应的 Niagara VFX
	 * Key: ReactionTag
	 * Value: Niagara System Asset
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Zone")
	TMap<FGameplayTag, TObjectPtr<UNiagaraSystem>> ReactionVFXMap;

	/**
	 * 区域碰撞盒——Projectile 通过球形检测找到此 ZoneActor
	 * 尺寸在编辑器里调整即可
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Zone")
	TObjectPtr<UBoxComponent> ZoneBounds;

	// ===== PCG 植被 =====
	/**
	 * PCG 组件——负责区域内植被的程序化生成与清除
	 *
	 * 【唯一配置点】在 BP_ElementalZone 的 Components 面板中选中 PCGComponent，
	 * 然后在 Details 面板的 Graph 属性里指定 PCG Graph 资产（如 PCG_ForestVegetation）。
	 * C++ 侧不重复暴露 Graph 属性，避免两处配置导致混乱。
	 *
	 * Generation Trigger 建议设为 GenerateOnDemand，由 BeginPlay/Bloom 反应手动驱动。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Zone|PCG")
	TObjectPtr<UPCGComponent> PCGComponent;

	/** 是否在 BeginPlay 时自动生成植被（仅 ZoneElementTag == Element.Plant 时生效） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Zone|PCG")
	bool bAutoGenerateVegetation = true;

	/** 植被生成种子：同种子同结果，方便关卡复现 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SIP|Zone|PCG")
	int32 VegetationSeed = 42;

	/**
	 * 手动触发植被生成
	 * Seed=0 时使用 VegetationSeed；Bloom 反应会以随机新种子调用此函数
	 * 前提：PCGComponent 上已配置好 Graph 资产，否则此调用无效并输出 Warning
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Zone|PCG")
	void GenerateVegetation(int32 Seed = 0);

protected:
	/** 当前状态 Tag，已有状态则不重复触发 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Zone", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ZoneStateTag;

	/** 执行反应逻辑：PCG 清理、Actor 显隐、VFX 生成 */
	void ApplyReaction(const FGameplayTag& ReactionTag, const FVector& ImpactLocation);

	/** 隐藏所有 LinkedDefaultActors */
	void HideDefaultActors();

	/** 生成反应 Actor（如冰平台） */
	void SpawnReactionActor(const FGameplayTag& ReactionTag, const FVector& Location);

	/** 播放反应 VFX */
	void PlayReactionVFX(const FGameplayTag& ReactionTag, const FVector& Location);

	/** 清理 PCGComponent 生成的内容（如 PCG 种出的树木） */
	void ClearPCGContent();

	/**
	 * Blueprint 可重写的反应扩展接口
	 * C++ 执行完核心逻辑后调用此函数，供 BP 添加动画/声效等
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Zone", DisplayName = "On Reaction Applied")
	void K2_OnReactionApplied(FGameplayTag ReactionTag, FVector Location);
};
