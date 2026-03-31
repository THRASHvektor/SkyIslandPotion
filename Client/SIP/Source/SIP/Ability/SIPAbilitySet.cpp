// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * SIPAbilitySet.cpp 实现了技能集的授予逻辑
 * 
 * 主要功能：
 * 1. 将配置好的 Ability 授予给 ASC
 * 2. 将配置好的 AttributeSet 添加到 ASC
 * 3. 将配置好的 GameplayEffect 应用到 ASC
 * 4. 管理授予句柄，用于后续移除
 */

#include "SIPAbilitySet.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "SIPLogCategory.h"
#include "AttributeSet.h"

/**
 * 用于追踪已授予的技能，便于后续移除
 */
void FSIPAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

/**
 * 用于追踪已授予的效果，便于后续移除
 */
void FSIPAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

/**
 * 用于追踪已授予的属性集，便于后续移除
 */
void FSIPAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* AttributeSet)
{
	if (AttributeSet)
	{
		GrantedAttributeSets.Add(AttributeSet);
	}
}

/**
 * 角色死亡或需要重置技能时调用
 * 
 * 移除流程：
 * 1. 遍历 AbilitySpecHandles，清除所有技能
 * 2. 遍历 GrantedAttributeSets，移除所有属性集
 * 3. 重置句柄数组
 */
void FSIPAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	GameplayEffectHandles.Reset();
    
	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	AbilitySpecHandles.Reset();

	for (UAttributeSet* Set : GrantedAttributeSets)
	{
		if (Set)
		{
			ASC->RemoveSpawnedAttribute(Set);
		}
	}
	
	GrantedAttributeSets.Reset();
}

USIPAbilitySet::USIPAbilitySet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

/**
 * 在角色初始化时调用，授予所有配置好的 Ability、AttributeSet、GameplayEffect
 * 
 * 授予流程：
 * 1. 遍历 GrantedGameplayAbilities，创建 AbilitySpec 并授予
 *    - 创建 FGameplayAbilitySpec
 *    - 设置 SourceObject
 *    - 添加 InputTag 到 DynamicAbilityTags
 *    - 调用 ASC->GiveAbility() 授予技能
 * 2. 遍历 GrantedAttributeSets，创建并添加属性集
 *    - 使用 NewObject 创建 AttributeSet 实例
 *    - 调用 ASC->AddAttributeSetSubobject() 添加到 ASC
 * 3. 遍历 GrantedGameplayEffects，应用效果
 *    - 创建 EffectContext
 *    - 调用 ASC->ApplyGameplayEffectToSelf() 应用效果
 */
void USIPAbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* ASC, FSIPAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(ASC);

	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FSIPAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];

		if (!IsValid(AbilityToGrant.Ability))
		{
			UE_LOG(LogSIPAbilitySystem, Error, TEXT("GrantedGameplayAbilities[%d] on ability set [%s] is not valid."), AbilityIndex, *GetNameSafe(this));
			continue;
		}

		UGameplayAbility* AbilityCDO = AbilityToGrant.Ability->GetDefaultObject<UGameplayAbility>();

		FGameplayAbilitySpec AbilitySpec(AbilityCDO);
		AbilitySpec.SourceObject = SourceObject;
        
		// 这样 ASC 可以通过 InputTag 找到对应的 Ability
		AbilitySpec.DynamicAbilityTags.AddTag(AbilityToGrant.InputTag);

		const FGameplayAbilitySpecHandle AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	for (int32 AttributeSetIndex = 0; AttributeSetIndex < GrantedAttributeSets.Num(); ++AttributeSetIndex)
	{
		const FSIPAbilitySet_AttributeSet& AttributeSetToGrant = GrantedAttributeSets[AttributeSetIndex];

		if (!IsValid(AttributeSetToGrant.AttributeSet))
		{
			UE_LOG(LogSIPAbilitySystem, Error, TEXT("GrantedAttributeSets[%d] on ability set [%s] is not valid."), AttributeSetIndex, *GetNameSafe(this));
			continue;
		}

		UAttributeSet* NewAttributeSet = NewObject<UAttributeSet>(ASC->GetOwner(), AttributeSetToGrant.AttributeSet);
        
		ASC->AddAttributeSetSubobject(NewAttributeSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewAttributeSet);
		}
	}

	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FSIPAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOG(LogSIPAbilitySystem, Error, TEXT("GrantedGameplayEffects[%d] on ability set [%s] is not valid."), EffectIndex, *GetNameSafe(this));
			continue;
		}

		UGameplayEffect* EffectCDO = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
        
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(SourceObject);

		const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectToSelf(
			EffectCDO, 
			EffectToGrant.EffectLevel, 
			EffectContext
		);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(EffectHandle);
		}
	}
}
