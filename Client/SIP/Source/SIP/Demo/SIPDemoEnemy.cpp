// Copyright Epic Games, Inc. All Rights Reserved.

#include "Demo/SIPDemoEnemy.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ASIPDemoEnemy::ASIPDemoEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	EnemyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyMesh"));
	EnemyMesh->SetupAttachment(RootComponent);
	EnemyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EnemyMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	EnemyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.8f));

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(RootComponent);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetWorldSize(72.0f);
	LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
	LabelText->SetRelativeRotation(FRotator(65.0f, 0.0f, 0.0f));
	LabelText->SetText(FText::FromString(TEXT("Demo Enemy")));
	LabelText->SetTextRenderColor(FColor::Red);
}

void ASIPDemoEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDefeated)
	{
		return;
	}

	BobTime += DeltaSeconds;
	const FVector BobOffset(0.0f, 0.0f, FMath::Sin(BobTime * 3.0f) * 18.0f);
	SetActorLocation(StartLocation + BobOffset);
	AddActorWorldRotation(FRotator(0.0f, DeltaSeconds * 45.0f, 0.0f));
}

void ASIPDemoEnemy::ConfigureEnemy(UStaticMesh* Mesh, UMaterialInterface* Material)
{
	StartLocation = GetActorLocation();
	Health = MaxHealth;
	bDefeated = false;

	if (Mesh)
	{
		EnemyMesh->SetStaticMesh(Mesh);
	}

	if (Material)
	{
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (DynamicMaterial)
		{
			const FLinearColor EnemyColor(0.95f, 0.10f, 0.08f);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), EnemyColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), EnemyColor);
			EnemyMesh->SetMaterial(0, DynamicMaterial);
		}
	}
}

void ASIPDemoEnemy::TakeDemoDamage(float DamageAmount)
{
	if (!IsAlive())
	{
		return;
	}

	Health = FMath::Max(0.0f, Health - DamageAmount);
	LabelText->SetText(FText::FromString(FString::Printf(TEXT("Enemy %.0f HP"), Health)));

	const float HealthAlpha = MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
	EnemyMesh->SetRelativeScale3D(FVector(0.55f + HealthAlpha * 0.35f));

	if (Health <= 0.0f)
	{
		bDefeated = true;
		SetActorEnableCollision(false);
		EnemyMesh->SetVisibility(false);
		LabelText->SetText(FText::FromString(TEXT("Enemy Cleared")));
		LabelText->SetTextRenderColor(FColor::Green);
	}
}
