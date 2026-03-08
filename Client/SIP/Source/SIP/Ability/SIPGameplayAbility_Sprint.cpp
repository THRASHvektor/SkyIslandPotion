#include "SIPGameplayAbility_Sprint.h"
#include "AbilitySystemComponent.h"
#include "SIPLogCategory.h"
#include "GameplayEffect.h"

USIPGameplayAbility_Sprint::USIPGameplayAbility_Sprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 设置为实例化模式，每个执行创建新实例
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	
	// 默认标签 - 使用InputTag.Sprint与输入配置匹配
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Sprint")));
	
	// 激活时阻止其他冲刺技能
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Sprint")));
}

bool USIPGameplayAbility_Sprint::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void USIPGameplayAbility_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!SprintEffectClass)
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("SprintAbility: SprintEffectClass is not set!"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		// 应用冲刺加速效果
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
		
		SprintEffectHandle = ASC->ApplyGameplayEffectToSelf(
			SprintEffectClass->GetDefaultObject<UGameplayEffect>(),
			1.0f,  // Level
			EffectContext
		);

		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Sprint started, effect applied. Handle: %s"), *SprintEffectHandle.ToString());
	}
}

void USIPGameplayAbility_Sprint::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USIPGameplayAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 移除冲刺效果
	if (SprintEffectHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC)
		{
			ASC->RemoveActiveGameplayEffect(SprintEffectHandle);
			UE_LOG(LogSIPAbilitySystem, Log, TEXT("Sprint ended, effect removed. Handle: %s"), *SprintEffectHandle.ToString());
		}
		SprintEffectHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
