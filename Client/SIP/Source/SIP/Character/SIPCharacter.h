// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "SIPCharacter.generated.h"

class USIPAbilitySet;
class UAbilitySystemComponent;
class USIPAbilitySystemComponent;
class UAttributeSet;
class USIPHealthSet;

UCLASS()
class ASIPCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASIPCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	USIPAbilitySystemComponent* GetSIPAbilitySystemComponent() const;

	// AttributeSet getters
	USIPHealthSet* GetSIPHealthSet() const;

	// 用于赋予角色ability的列表，映射Inputtag和ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Abilities")
	TArray<TObjectPtr<USIPAbilitySet>> AbilitySets;

	// Death handling
	virtual void OnDeath();
	virtual void OnDeathStarted();
	virtual void OnDeathStopped();

protected:

	// To add mapping context
	virtual void BeginPlay();

	// 在这里初始化组件
	virtual void PostInitializeComponents() override;
	
protected:
	/** Ability System */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<USIPAbilitySystemComponent> AbilitySystemComponent;
};
