// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "SIPPetCharacter.generated.h"
class USIPAbilitySystemComponent;
class UAttributeSet;
UCLASS()
class SIP_API ASIPPetCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASIPPetCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	// 安装GAS
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	USIPAbilitySystemComponent* AbilitySystemComponent;

	// 安装属性集
	UPROPERTY()
	const UAttributeSet* AttributeSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pet Data")
    class USIPPetDataAsset* PetData;
};
