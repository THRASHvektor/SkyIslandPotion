// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPInputComponent.h"

#include "EnhancedInputSubsystems.h"

// 在 EnhancedInputComponent 之上补一层项目自定义的基于 Tag 的绑定辅助。
USIPInputComponent::USIPInputComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	
}

// 预留扩展点，后续如果输入配置映射需要附带额外逻辑，可以收口在这里。
void USIPInputComponent::AddInputMappings(const USIPInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// 如果后续需要根据输入配置追加自定义映射逻辑，可以在这里处理。
}

// 与 AddInputMappings 对称的扩展点，方便未来统一清理自定义映射。
void USIPInputComponent::RemoveInputMappings(const USIPInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// 如果上面额外添加了自定义映射，这里负责对应的移除逻辑。
}

// 批量移除外部缓存的绑定句柄，并在同一处清空调用方数组。
void USIPInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
