// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SIPAbilitySet.generated.h"

class UGameplayAbility;
class UAttributeSet;
class UAbilitySystemComponent;
struct FGameplayAbilitySpecHandle;

/**
 * FSIPAbilitySet_GameplayAbility
 *
 *	一个用于注册的GameplayAbility结构体，包含了Ability和对应的InputTag
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:

	// Gameplay ability to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayAbility> Ability = nullptr;

	// Tag used to process input for the ability.
	UPROPERTY(EditDefaultsOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};


/**
 * FSIPAbilitySet_AttributeSet
 *
 *	一个用于注册的AttributeSet结构体
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_AttributeSet
{
	GENERATED_BODY()

public:

	// AttributeSet to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeSet> AttributeSet = nullptr;
};


/**
 * FSIPAbilitySet_GrantedHandles
 *
 *	用于储存已经赋予给角色的GA的句柄
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:

	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void AddAttributeSet(UAttributeSet* AttributeSet);

	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);

protected:

	// Handles to the granted abilities.
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	// // Handles to the granted gameplay effects.
	// UPROPERTY()
	// TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	// // Pointers to the granted attribute sets
	// UPROPERTY()
	// TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};


/**
 * USIPAbilitySet
 *
 *	一组不可变的数据集，用于储存赋予的GA
 */
UCLASS(BlueprintType, Const)
class USIPAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	USIPAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 将技能赋予给特定ASC，返回的handle可用于移除技能
	void GiveToAbilitySystem(UAbilitySystemComponent* ASC, FSIPAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;

protected:

	// 要赋予的Ability列表
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FSIPAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	// 要赋予的AttributeSet列表
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Attributes", meta=(TitleProperty=AttributeSet))
	TArray<FSIPAbilitySet_AttributeSet> GrantedAttributeSets;
};
