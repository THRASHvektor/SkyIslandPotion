#include "SIPAbilitySet.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "SIPLogCategory.h"


void FSIPAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FSIPAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
    
	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

    AbilitySpecHandles.Reset();

	// for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	// {
	// 	if (Handle.IsValid())
	// 	{
	// 		LyraASC->RemoveActiveGameplayEffect(Handle);
	// 	}
	// }

	// for (UAttributeSet* Set : GrantedAttributeSets)
	// {
	// 	LyraASC->RemoveSpawnedAttribute(Set);
	// }

	
	// GameplayEffectHandles.Reset();
	// GrantedAttributeSets.Reset();
}




USIPAbilitySet::USIPAbilitySet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USIPAbilitySet::GiveToAbilitySystem(UAbilitySystemComponent* ASC, FSIPAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject) const
{
    check(ASC);

    // Grant the gameplay abilities.
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
}