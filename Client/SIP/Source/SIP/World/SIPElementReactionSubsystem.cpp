// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPElementReactionSubsystem.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

void USIPElementReactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BuildReactionTable();
	UE_LOG(LogSIP, Log, TEXT("SIPElementReactionSubsystem initialized with %d zone-element entries."), ReactionTable.Num());
}

void USIPElementReactionSubsystem::BuildReactionTable()
{
	// ─────────────────────────────────────────────────────────────
	// 反应矩阵（POC Demo 5种）
	// 格式：ReactionTable[ZoneElement][IncomingElement] = ReactionTag
	// ─────────────────────────────────────────────────────────────

	// 植物区域 + 火药水 = 燃烧：植被清除，道路开放
	ReactionTable.FindOrAdd(SIPGameplayTags::Element_Plant).Add(
		SIPGameplayTags::Element_Fire, SIPGameplayTags::Reaction_Burn);

	// 冰区域 + 火药水 = 融化：冰结构坍塌，隐藏区域显现
	ReactionTable.FindOrAdd(SIPGameplayTags::Element_Ice).Add(
		SIPGameplayTags::Element_Fire, SIPGameplayTags::Reaction_Melt);

	// 水区域 + 冰药水 = 冻结：水面冰封，生成可行走平台
	ReactionTable.FindOrAdd(SIPGameplayTags::Element_Water).Add(
		SIPGameplayTags::Element_Ice, SIPGameplayTags::Reaction_Freeze);

	// 水区域 + 雷药水 = 感电：区域内所有敌人眩晕
	ReactionTable.FindOrAdd(SIPGameplayTags::Element_Water).Add(
		SIPGameplayTags::Element_Thunder, SIPGameplayTags::Reaction_Electrify);

	// Backward compatibility for older assets that still use Element.Heal as water.
	ReactionTable.FindOrAdd(SIPGameplayTags::Element_Heal).Add(
		SIPGameplayTags::Element_Ice, SIPGameplayTags::Reaction_Freeze);
	ReactionTable.FindOrAdd(SIPGameplayTags::Element_Heal).Add(
		SIPGameplayTags::Element_Thunder, SIPGameplayTags::Reaction_Electrify);

	// 植物区域 + 风药水 = 绽放：孢子扩散，周边生成新资源节点
	ReactionTable.FindOrAdd(SIPGameplayTags::Element_Plant).Add(
		SIPGameplayTags::Element_Wind, SIPGameplayTags::Reaction_Bloom);
}

FGameplayTag USIPElementReactionSubsystem::QueryReaction(
	const FGameplayTag& ZoneElement,
	const FGameplayTag& IncomingElement) const
{
	const TMap<FGameplayTag, FGameplayTag>* InnerMap = ReactionTable.Find(ZoneElement);
	if (!InnerMap)
	{
		return FGameplayTag::EmptyTag;
	}

	const FGameplayTag* ResultTag = InnerMap->Find(IncomingElement);
	return ResultTag ? *ResultTag : FGameplayTag::EmptyTag;
}

void USIPElementReactionSubsystem::ProcessElementHit(
	const FGameplayTag& ZoneElement,
	const FGameplayTag& IncomingElement,
	FVector ImpactLocation,
	AActor* ZoneActor)
{
	if (!ZoneElement.IsValid() || !IncomingElement.IsValid())
	{
		return;
	}

	const FGameplayTag ReactionTag = QueryReaction(ZoneElement, IncomingElement);
	if (!ReactionTag.IsValid())
	{
		UE_LOG(LogSIP, Verbose, TEXT("No reaction for Zone[%s] + Incoming[%s]"),
			*ZoneElement.ToString(), *IncomingElement.ToString());
		return;
	}

	UE_LOG(LogSIP, Log, TEXT("Elemental reaction triggered: %s + %s = %s at %s"),
		*ZoneElement.ToString(),
		*IncomingElement.ToString(),
		*ReactionTag.ToString(),
		*ImpactLocation.ToString());

	OnElementalReaction.Broadcast(ImpactLocation, ReactionTag, ZoneActor);
}
