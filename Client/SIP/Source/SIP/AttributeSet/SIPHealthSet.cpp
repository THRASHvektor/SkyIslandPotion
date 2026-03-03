// Copyright Epic Games, Inc. All Rights Reserved.

#include "AttributeSet/SIPHealthSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SIPLogCategory.h"
#include "Net/UnrealNetwork.h"

USIPHealthSet::USIPHealthSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
	, Healing(0.0f)
	, MoveSpeed(600.0f)
{
	Tag_MaxHealthChanged = FGameplayTag::RequestGameplayTag(FName("SIP.Health.MaxHealthChanged"));
	Tag_HealthChanged = FGameplayTag::RequestGameplayTag(FName("SIP.Health.HealthChanged"));
}

UWorld* USIPHealthSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UAbilitySystemComponent* USIPHealthSet::GetOwningAbilitySystemComponent() const
{
	return Cast<UAbilitySystemComponent>(GetOuter());
}

void USIPHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, Healing, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void USIPHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void USIPHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		UE_LOG(LogSIP, Warning, TEXT("USIPHealthSet::PreAttributeChange Health: %f"), NewValue);
	}
}

void USIPHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		if (GetHealth() > NewValue)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ASC)
			{
				ASC->SetNumericAttributeBase(GetHealthAttribute(), NewValue);
			}
		}
	}

	if (Attribute == GetHealthAttribute())
	{
		if (OldValue <= 0.0f && NewValue > 0.0f)
		{
			UE_LOG(LogSIP, Log, TEXT("Player Revived"));
		}
		else if (OldValue > 0.0f && NewValue <= 0.0f)
		{
			UE_LOG(LogSIP, Log, TEXT("Player Died"));
		}
	}
}

void USIPHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void USIPHealthSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, Health, OldHealth);
}

void USIPHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, MaxHealth, OldMaxHealth);
}

void USIPHealthSet::OnRep_Healing(const FGameplayAttributeData& OldHealing)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, Healing, OldHealing);
}

void USIPHealthSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, MoveSpeed, OldMoveSpeed);
}
