#include "SIPAbilitySystemComponent.h"
#include "SIPGameplayTags.h"


USIPAbilitySystemComponent::USIPAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void USIPAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
    // 可激活该Tag（比如通过GE）来禁止玩家输入
	if (HasMatchingGameplayTag(SIPGameplayTags::TAG_Gameplay_AbilityInputBlocked))
	{
		// ClearAbilityInput();
        InputPressedSpecHandles.Reset();
        InputReleasedSpecHandles.Reset();
        InputHeldSpecHandles.Reset();
		return;
	}

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	// 处理此帧里正在按住的技能输入
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
                AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
                /* Lyra通过ActivationPolicy来支持长按多次激发技能 */
				// const UGameplayAbility* AbilityCDO = AbilitySpec->Ability;
				// if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EGameplayAbilityActivationPolicy::WhileInputActive)
				// {
				// 	AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				// }
			}
		}
	}

	// 处理此帧里按下的技能输入
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
                    AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
                    /* Lyra通过ActivationPolicy来支持长按多次激发技能 */
					// const UGameplayAbility* AbilityCDO = AbilitySpec->Ability;
					// if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EGameplayAbilityActivationPolicy::OnInputTriggered)
					// {
					// 	AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					// }
				}
			}
		}
	}

	//
	// Try to activate all the abilities that are from presses and holds.
	// We do it all at once so that held inputs don't activate the ability
	// and then also send a input event to the ability because of the press.
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	// 处理此帧里抬起的技能输入
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	// 清理缓存的输入
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void USIPAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void USIPAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void USIPAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// 加上下面WaitInputPress才生效
	if (Spec.IsActive())
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
	}
}

void USIPAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

    // 加上下面WaitInputRelease才生效
	if (Spec.IsActive())
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
	}
}