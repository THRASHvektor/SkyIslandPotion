// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPAbilitySet.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "SIPLogCategory.h"
#include "AttributeSet.h"

// 用于存储已授权的AbilitySpec句柄
void FSIPAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

// 新增：用于存储已授权的GameplayEffect句柄
void FSIPAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

// 用于存储已授权的AttributeSet指针
void FSIPAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* AttributeSet)
{
	if (AttributeSet)
	{
		GrantedAttributeSets.Add(AttributeSet);
	}
}

void FSIPAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
    // 
	check(ASC);
    
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

    // 新增：赋予GameplayEffects（用于初始属性设置或被动效果）
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
