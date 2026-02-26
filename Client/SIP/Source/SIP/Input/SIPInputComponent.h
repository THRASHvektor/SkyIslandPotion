// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnhancedInputComponent.h"
#include "SIPInputConfig.h"

#include "SIPInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;
class UObject;


/**
 *
 *	通过此组件，在外部系统中可以通过GameplayTag来绑定输入事件，避免了直接引用InputAction资产的需要
 */
UCLASS(Config = Input)
class USIPInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:

	USIPInputComponent(const FObjectInitializer& ObjectInitializer);

	void AddInputMappings(const USIPInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;
	void RemoveInputMappings(const USIPInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const;

	template<class UserClass, typename FuncType>
	void BindNativeAction(const USIPInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound);

	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const USIPInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);

	void RemoveBinds(TArray<uint32>& BindHandles);
};

/*
* 提供最基本操作的绑定
*/
template<class UserClass, typename FuncType>
void USIPInputComponent::BindNativeAction(const USIPInputConfig* InputConfig, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func, bool bLogIfNotFound)
{
	check(InputConfig);
	if (const UInputAction* IA = InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

/*
* 提供操作对应能力实现的行为的绑定（待实现）
*/
template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void USIPInputComponent::BindAbilityActions(const USIPInputConfig* InputConfig, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputConfig);

	/*for (const FLyraInputAction& Action : InputConfig->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());
			}

			if (ReleasedFunc)
			{
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
			}
		}
	}*/
}
