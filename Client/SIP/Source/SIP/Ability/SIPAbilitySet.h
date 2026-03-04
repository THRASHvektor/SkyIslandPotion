#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
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
 * FLyraAbilitySet_GameplayEffect
 *
 *	Data used by the ability set to grant gameplay effects.
 *  新增：用于支持GAS的GameplayEffect系统，允许在AbilitySet中同时授权GameplayEffect
 */
USTRUCT(BlueprintType)
struct FSIPAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:

	// Gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	// Level of gameplay effect to grant.
	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.0f;
};


/**
 * FSIPAbilitySet_AttributeSet
 *
 *	一个用于注册的AttributeSet结构体
 * 新增：用于支持GAS的Attribute系统，允许在AbilitySet中同时授权AttributeSet
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

	// 新增：用于追踪授权的GameplayEffect句柄，便于后续移除
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);

	
	// 新增：用于追踪授权的AttributeSet，便于后续移除
	void AddAttributeSet(UAttributeSet* AttributeSet);

	void TakeFromAbilitySystem(UAbilitySystemComponent* ASC);

protected:

	// Handles to the granted abilities.
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	// 新增：用于存储已授权的GameplayEffect句柄
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;
	
	// 新增：用于追踪已授权的AttributeSet，便于后续移除
	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
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

	// 新增：要赋予的GameplayEffect列表，用于初始属性设置或被动效果
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FSIPAbilitySet_GameplayEffect> GrantedGameplayEffects;
};
