// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "SIPHealthSet.generated.h"

UCLASS()
class USIPHealthSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	USIPHealthSet();

	UWorld* GetWorld() const override;
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

public:

	// 使用 GAS 宏定义 Getter 函数
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPHealthSet, Health);
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPHealthSet, MaxHealth);
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPHealthSet, Healing);
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPHealthSet, MoveSpeed);

	// Primary Attribute
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;

	// Clamped between 0 and MaxHealth
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_Healing)
	FGameplayAttributeData Healing;

	// Secondary Attribute
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;

	// Cache tags
	FGameplayTag Tag_MaxHealthChanged;
	FGameplayTag Tag_HealthChanged;

protected:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Healing(const FGameplayAttributeData& OldHealing);

	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
};
