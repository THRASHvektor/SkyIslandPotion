// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * USIPElementReactionSubsystem 是元素反应的计算核心
 * 继承自 UWorldSubsystem，随 World 生命周期存在
 *
 * 职责：
 * 1. 维护反应表（ZoneElement + IncomingElement → ReactionTag）
 * 2. 提供 QueryReaction() 接口供 ZoneActor 查询
 * 3. 通过 OnElementalReaction 委托广播反应事件（UI、音效可监听）
 *
 * 反应矩阵（POC Demo 5种）：
 *   Element.Plant  + Element.Fire    → Reaction.Burn      (植被清除，道路开放)
 *   Element.Ice    + Element.Fire    → Reaction.Melt      (冰结构融化，隐藏区域显现)
 *   Element.Water  + Element.Ice     → Reaction.Freeze    (水面冰封，形成可行走平台)
 *   Element.Water  + Element.Thunder → Reaction.Electrify (湿润区域感电，范围眩晕)
 *   Element.Plant  + Element.Wind    → Reaction.Bloom     (孢子扩散，资源点新生)
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "SIPElementReactionSubsystem.generated.h"

/**
 * 元素反应事件委托
 * @param ImpactLocation  反应发生位置（世界坐标）
 * @param ReactionTag     反应类型（Reaction.Burn 等）
 * @param ZoneActor       触发反应的 ZoneActor
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FSIPReactionEvent,
	FVector, ImpactLocation,
	FGameplayTag, ReactionTag,
	AActor*, ZoneActor
);

UCLASS()
class SIP_API USIPElementReactionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * 查询两种元素组合的反应结果
	 * @param ZoneElement     区域自身的元素（如 Element.Plant）
	 * @param IncomingElement 药水弹丸携带的元素（如 Element.Fire）
	 * @return  反应 Tag（找不到时返回无效 Tag）
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Elemental")
	FGameplayTag QueryReaction(const FGameplayTag& ZoneElement, const FGameplayTag& IncomingElement) const;

	/**
	 * 由 SIPElementalZoneActor 调用
	 * 查询反应 → 若存在则广播事件
	 */
	void ProcessElementHit(
		const FGameplayTag& ZoneElement,
		const FGameplayTag& IncomingElement,
		FVector ImpactLocation,
		AActor* ZoneActor
	);

	/** 反应发生时的广播委托（UI HUD、音效系统监听此事件） */
	UPROPERTY(BlueprintAssignable, Category = "SIP|Elemental")
	FSIPReactionEvent OnElementalReaction;

private:
	void BuildReactionTable();

	// 反应表：ZoneElement → (IncomingElement → ReactionTag)
	TMap<FGameplayTag, TMap<FGameplayTag, FGameplayTag>> ReactionTable;
};
