// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPInputComponent.h"

#include "EnhancedInputSubsystems.h"


USIPInputComponent::USIPInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void USIPInputComponent::AddInputMappings(const USIPInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to add something from your input config if required
}

void USIPInputComponent::RemoveInputMappings(const USIPInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to remove input mappings that you may have added above
}

void USIPInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
