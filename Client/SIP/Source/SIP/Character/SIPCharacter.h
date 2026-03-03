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
// TODO： 建立相关的Component框架
/*
* 角色基类，用于所有会有AI、移动等行为的Entity，包含宠物、坐骑等
*/
UCLASS(config=Game)
class ASIPCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASIPCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	USIPAbilitySystemComponent* GetSIPAbilitySystemComponent() const;

	// 用于赋予角色ability的列表，映射Inputtag和ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Abilities")
	TArray<TObjectPtr<USIPAbilitySet>> AbilitySets;

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

