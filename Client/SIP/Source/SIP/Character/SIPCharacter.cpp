// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "AttributeSet/SIPHealthSet.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"


ASIPCharacter::ASIPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	AbilitySystemComponent = CreateDefaultSubobject<USIPAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void ASIPCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASIPCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

UAbilitySystemComponent* ASIPCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USIPAbilitySystemComponent* ASIPCharacter::GetSIPAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USIPHealthSet* ASIPCharacter::GetSIPHealthSet() const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->GetSet<USIPHealthSet>();
	}
	return nullptr;
}

void ASIPCharacter::OnDeath()
{
	UE_LOG(LogSIP, Log, TEXT("%s has died."), *GetName());
}

void ASIPCharacter::OnDeathStarted()
{
	UE_LOG(LogSIP, Log, TEXT("%s death started."), *GetName());

	EnableInput(nullptr);
}

void ASIPCharacter::OnDeathStopped()
{
	UE_LOG(LogSIP, Log, TEXT("%s death stopped (revived)."), *GetName());
}
