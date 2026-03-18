// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPInputConfig.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(SIPInputConfig)


// 输入配置本身只是数据资产，因此原生构造函数保持为空。
USIPInputConfig::USIPInputConfig(const FObjectInitializer& ObjectInitializer)
{
}

// 先在手动绑定列表中查找，这些通常是移动、视角之类的原生输入。
const UInputAction* USIPInputConfig::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FSIPInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find NativeInputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}

// 给调用方提供统一查询入口，不必关心输入属于原生还是技能输入。
const UInputAction* USIPInputConfig::FindInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	if (const UInputAction* NativeAction = FindNativeInputActionForTag(InputTag, false))
	{
		return NativeAction;
	}

	if (const UInputAction* AbilityAction = FindAbilityInputActionForTag(InputTag, false))
	{
		return AbilityAction;
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find any InputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}

// 再到技能输入列表里查找，这些输入会进入 ASC 的能力输入处理链路。
const UInputAction* USIPInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FSIPInputAction& Action : AbilityInputActions)
	{
		if (Action.InputAction && (Action.InputTag == InputTag))
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find AbilityInputAction for InputTag [%s] on InputConfig [%s]."), *InputTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}
