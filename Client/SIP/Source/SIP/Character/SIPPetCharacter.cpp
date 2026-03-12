// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SIPPetCharacter.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Ability/Pet/SIPPetAttributeSet.h"      
#include "Data/SIPPetDataAsset.h"         
#include "Components/SkeletalMeshComponent.h"

// Sets default values
ASIPPetCharacter::ASIPPetCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AbilitySystemComponent = CreateDefaultSubobject<USIPAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// 创建属性集
	//AttributeSet = CreateDefaultSubobject<USIPPetAttributeSet>(TEXT("AttributeSet"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent* ASIPPetCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASIPPetCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
	if (PetData)
	{
		// 自动换上模型
		if (PetData->PetMesh)
		{
			GetMesh()->SetSkeletalMesh(PetData->PetMesh);
		}

		// 初始化属性集
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->SetNumericAttributeBase(USIPPetAttributeSet::GetHealthAttribute(), PetData->BaseHealth);
			AbilitySystemComponent->SetNumericAttributeBase(USIPPetAttributeSet::GetMaxHealthAttribute(), PetData->BaseHealth);
			AbilitySystemComponent->SetNumericAttributeBase(USIPPetAttributeSet::GetWorkSpeedAttribute(), PetData->BaseWorkSpeed);
		}
	}
}
void ASIPPetCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

