#include "SIPGameplayAbility_Sprint.h"
#include "AbilitySystemComponent.h"
#include "SIPLogCategory.h"
#include "GameplayEffect.h"

// 冲刺是一个按住维持的能力，会持续施加移动速度效果。
USIPGameplayAbility_Sprint::USIPGameplayAbility_Sprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 配置为实例化执行，每次激活都会创建新的能力实例。
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	
	// 冲刺属于按住生效型能力，只要输入保持就持续激活。
	ActivationPolicy = ESIPAbilityActivationPolicy::WhileInputActive;
	
	// 默认标签，使用 InputTag.Sprint 与输入配置匹配。
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Sprint")));
	
	// 激活时阻止重复触发同类冲刺能力。
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Sprint")));
}

// 当前基础校验已经足够，后续如有需要可继续补充冲刺专属规则。
bool USIPGameplayAbility_Sprint::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

// 对自己施加配置好的冲刺 GE，让移动速度在按住期间持续由 GAS 驱动。
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
		// 构造效果上下文并把冲刺效果施加到自己身上。
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
		
		SprintEffectHandle = ASC->ApplyGameplayEffectToSelf(
			SprintEffectClass->GetDefaultObject<UGameplayEffect>(),
			1.0f,
			EffectContext
		);

		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Sprint started, effect applied. Handle: %s"), *SprintEffectHandle.ToString());
	}
}

// 松开输入时，立刻结束这个持续型冲刺能力。
void USIPGameplayAbility_Sprint::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// 能力结束时移除冲刺 GE，让速度回到其他系统当前控制的结果。
void USIPGameplayAbility_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 移除冲刺效果。
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
