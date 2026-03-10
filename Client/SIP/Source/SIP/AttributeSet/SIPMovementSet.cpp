// Copyright Epic Games, Inc. All Rights Reserved.

#include "AttributeSet/SIPMovementSet.h"
#include "AbilitySystemInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SIPLogCategory.h"

USIPMovementSet::USIPMovementSet()
	: MoveSpeed(600.0f)
{
}

UWorld* USIPMovementSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);
	return Outer->GetWorld();
}

UAbilitySystemComponent* USIPMovementSet::GetOwningAbilitySystemComponent() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOuter()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

void USIPMovementSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPMovementSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void USIPMovementSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void USIPMovementSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		UE_LOG(LogSIP, Log, TEXT("MoveSpeed changed: %.2f -> %.2f"), OldValue, NewValue);

		if (ACharacter* Character = Cast<ACharacter>(GetOuter()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = NewValue;
			}
		}
	}
}

void USIPMovementSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPMovementSet, MoveSpeed, OldMoveSpeed);

	// 客户端收到复制值时同步到 CharacterMovement
	if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		if (ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = MoveSpeed.GetCurrentValue();
			}
		}
	}
}
