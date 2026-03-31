#include "SIPAbilitySystemComponent.h"
#include "SIPGameplayTags.h"
#include "SIPGameplayAbility.h"

/**
 * ProcessAbilityInput 是技能系统输入处理的核心函数
 * 它在角色的 Tick 中被调用，负责：
 * 1. 检查输入是否被屏蔽（如被眩晕时）
 * 2. 处理按住、长按、松开等输入状态
 * 3. 激活符合条件的 Ability
 * 4. 清理临时输入缓存
 */

USIPAbilitySystemComponent::USIPAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
{
	
}


void USIPAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
    // 如果有 GE（如眩晕、冰冻）添加了此标签，则禁止所有技能输入
	if (HasMatchingGameplayTag(SIPGameplayTags::TAG_Gameplay_AbilityInputBlocked))
	{
        // Z 清空所有输入缓存，阻止技能激活
		InputPressedSpecHandles.Reset();
        InputReleasedSpecHandles.Reset();
        InputHeldSpecHandles.Reset();
		return;
	}

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

    // 只有 ActivationPolicy == WhileInputActive 的技能才会在持续按住时重复激活
    // OnInputTriggered 的技能（Attack/Dash等）不走此分支，避免每帧重复尝试激活
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const USIPGameplayAbility* SIPAbility = Cast<USIPGameplayAbility>(AbilitySpec->Ability);
				if (SIPAbility && SIPAbility->ActivationPolicy == ESIPAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

    // 遍历此帧按下的输入，设置 InputPressed 标志并尝试激活
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
                    // Z 如果 Ability 已经激活（如持续技能），传递输入事件给它
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
                    // Z 如果 Ability 未激活，尝试激活它
                    // 典型的如 Dash、攻击等一次性技能
                    AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

    // 为什么要批量激活？
    // 避免按住输入时，激活技能的同时又发送输入事件导致重复处理
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

    // 通知正在激活的 Ability 输入已释放
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
                    // Z 如果 Ability 正在运行，传递释放事件
					AbilitySpecInputReleased(*AbilitySpec);
				}
				else
				{
                    // Z 边缘情况：快速按下松开的技能
                    // 可能技能正在激活过程中，需要通知它
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

    // 清空此帧的输入缓存，等待下一帧重新收集
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

/**
 * AbilityInputTagPressed 是输入绑定系统的核心函数
 * 当玩家按下与技能绑定的输入键时，Enhanced Input 系统会调用此函数
 * 
 * 数据流：
 * 1. 玩家按下键盘/手柄按键
 * 2. Enhanced Input 系统触发 InputAction
 * 3. SIPHeroCharacter 捕获 InputAction，调用本函数
 * 4. 本函数遍历所有已注册的 Ability，找到匹配的
 * 5. 将匹配的 Ability Handle 加入待处理列表
 */
void USIPAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
            // Z 使用 DynamicAbilityTags 进行匹配
            // 这是 AbilitySet 赋予 Ability 时设置的 InputTag
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
                // Z 加入按下列表和按住列表
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

/**
 * AbilityInputTagReleased 与 AbilityInputTagPressed 对应
 * 当玩家松开输入键时调用
 * 
 * 为什么要分开按下和松开？
 * 1. 有些技能是按下触发（如攻击）
 * 2. 有些技能是按住持续触发（如蓄力）
 * 3. 有些技能是松开触发（如冲刺结束）
 */
void USIPAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
                // Z 加入松开列表，从按住列表中移除
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

/**
 * AbilitySpecInputPressed 重写父类函数
 * 当 AbilitySpec 的输入状态改变时，通知 Ability 本身
 * 
 * 核心作用：
 * 触发 Ability 内部的 WaitInputPress 等节点
 * 让 Ability 可以响应输入事件（不只是激活，还有持续过程中的输入变化）
 */
void USIPAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

    // Z 调用网络复制事件，通知服务端
	if (Spec.IsActive())
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
	}
}

/**
 * AbilitySpecInputReleased 重写父类函数
 * 当 AbilitySpec 的输入释放时，通知 Ability 本身
 * 
 * 核心作用：
 * 触发 Ability 内部的 WaitInputRelease 等节点
 * 让技能可以在激活过程中响应输入释放（如蓄力技能）
 */
void USIPAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

    // Z 调用网络复制事件，通知服务端
	if (Spec.IsActive())
	{
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
	}
}
