// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPPetElementalFieldActor.h"

#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

ASIPPetElementalFieldActor::ASIPPetElementalFieldActor()
{
	PrimaryActorTick.bCanEverTick = false;

	FieldBounds = CreateDefaultSubobject<USphereComponent>(TEXT("FieldBounds"));
	FieldBounds->SetSphereRadius(FieldRadius);
	FieldBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FieldBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	FieldBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = FieldBounds;

	FieldVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FieldVFXComponent"));
	FieldVFXComponent->SetupAttachment(RootComponent);
	FieldVFXComponent->bAutoActivate = true;
}

void ASIPPetElementalFieldActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FieldBounds)
	{
		FieldBounds->SetSphereRadius(FieldRadius);
	}

	if (FieldVFXComponent)
	{
		FieldVFXComponent->SetAsset(FieldVFX);
		FieldVFXComponent->SetVariableLinearColor(TEXT("User.Color"), FieldColor);
		FieldVFXComponent->SetVariableFloat(TEXT("User.Radius"), FieldRadius);
	}
}

void ASIPPetElementalFieldActor::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(Duration);

	if (FieldVFXComponent)
	{
		FieldVFXComponent->SetAsset(FieldVFX);
		FieldVFXComponent->SetVariableLinearColor(TEXT("User.Color"), FieldColor);
		FieldVFXComponent->SetVariableFloat(TEXT("User.Radius"), FieldRadius);
		FieldVFXComponent->Activate(true);
	}
}
