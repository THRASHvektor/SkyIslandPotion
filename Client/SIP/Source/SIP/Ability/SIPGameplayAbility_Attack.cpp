#include "SIPGameplayAbility_Attack.h"

#include "Character/SIPCharacter.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

USIPGameplayAbility_Attack::USIPGameplayAbility_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	AbilityTags.AddTag(SIPGameplayTags::InputTag_Attack);
	ActivationBlockedTags.AddTag(SIPGameplayTags::State_Dead);
}

bool USIPGameplayAbility_Attack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	return SourceCharacter && !SourceCharacter->IsDeadOrDying();
}

void USIPGameplayAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	if (!SourceCharacter || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AttackMontage && SourceCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInstance = SourceCharacter->GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(AttackMontage);
		}
	}

	const TArray<ASIPCharacter*> Targets = CollectTargets(SourceCharacter);
	for (ASIPCharacter* Target : Targets)
	{
		Target->ApplyCombatDamage(DamageAmount, SourceCharacter);
	}

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("Attack ability hit %d target(s)."), Targets.Num());
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

TArray<ASIPCharacter*> USIPGameplayAbility_Attack::CollectTargets(ASIPCharacter* SourceCharacter) const
{
	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceCharacter);

	const FVector StartLocation = SourceCharacter->GetActorLocation() + SourceCharacter->GetActorForwardVector() * AttackRange;
	UKismetSystemLibrary::SphereOverlapActors(
		SourceCharacter,
		StartLocation,
		AttackRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(ECC_Pawn)},
		ASIPCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappingActors);

	TArray<ASIPCharacter*> Targets;
	for (AActor* OverlappingActor : OverlappingActors)
	{
		ASIPCharacter* TargetCharacter = Cast<ASIPCharacter>(OverlappingActor);
		if (TargetCharacter && !TargetCharacter->IsDeadOrDying())
		{
			Targets.AddUnique(TargetCharacter);
		}
	}

	return Targets;
}