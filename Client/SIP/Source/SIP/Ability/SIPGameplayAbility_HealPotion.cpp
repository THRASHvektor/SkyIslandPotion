#include "SIPGameplayAbility_HealPotion.h"

#include "Character/SIPCharacter.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

// 将治疗药水注册为一次性能力，并绑定到治疗输入标签。
USIPGameplayAbility_HealPotion::USIPGameplayAbility_HealPotion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	AbilityTags.AddTag(SIPGameplayTags::InputTag_Potion_Heal);
	AbilityTags.AddTag(SIPGameplayTags::Vitality_Healing);
	ActivationBlockedTags.AddTag(SIPGameplayTags::State_Dead);
}

// 治疗药水只适用于存活且当前血量未满的角色。
bool USIPGameplayAbility_HealPotion::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	return SourceCharacter && !SourceCharacter->IsDeadOrDying() && SourceCharacter->GetCurrentHealth() < SourceCharacter->GetMaxHealth();
}

// 先 Commit，确保冷却和消耗规则生效，再立刻回血并结束能力。
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
