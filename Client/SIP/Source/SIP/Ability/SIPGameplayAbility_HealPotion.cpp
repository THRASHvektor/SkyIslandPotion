#include "SIPGameplayAbility_HealPotion.h"

#include "Character/SIPCharacter.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

USIPGameplayAbility_HealPotion::USIPGameplayAbility_HealPotion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	AbilityTags.AddTag(SIPGameplayTags::InputTag_Potion_Heal);
	AbilityTags.AddTag(SIPGameplayTags::Vitality_Healing);
	ActivationBlockedTags.AddTag(SIPGameplayTags::State_Dead);
}

bool USIPGameplayAbility_HealPotion::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	return SourceCharacter && !SourceCharacter->IsDeadOrDying() && SourceCharacter->GetCurrentHealth() < SourceCharacter->GetMaxHealth();
}

void USIPGameplayAbility_HealPotion::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	if (!SourceCharacter || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SourceCharacter->RestoreHealth(HealAmount);
	UE_LOG(LogSIPAbilitySystem, Log, TEXT("Healing potion restored %.2f health."), HealAmount);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}