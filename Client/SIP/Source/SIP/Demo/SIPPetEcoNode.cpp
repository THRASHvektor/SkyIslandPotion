// Copyright Epic Games, Inc. All Rights Reserved.

#include "Demo/SIPPetEcoNode.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ASIPPetEcoNode::ASIPPetEcoNode()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	NodeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeMesh"));
	NodeMesh->SetupAttachment(RootComponent);
	NodeMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	NodeMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	NodeMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.6f));

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(RootComponent);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetWorldSize(72.0f);
	LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));
	LabelText->SetRelativeRotation(FRotator(65.0f, 0.0f, 0.0f));
}

void ASIPPetEcoNode::ConfigureNode(ESIPPetEcoNodeType InType, UStaticMesh* Mesh, UMaterialInterface* Material, const FLinearColor& Color)
{
	NodeType = InType;
	bActivated = false;

	if (Mesh)
	{
		NodeMesh->SetStaticMesh(Mesh);
	}

	if (Material)
	{
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
			NodeMesh->SetMaterial(0, DynamicMaterial);
		}
	}

	LabelText->SetText(FText::FromString(GetNodeName()));
	LabelText->SetTextRenderColor(Color.ToFColor(true));

	switch (NodeType)
	{
	case ESIPPetEcoNodeType::BridgeSeed:
		ExploreValue = 300.0f;
		DangerValue = 0.0f;
		break;
	case ESIPPetEcoNodeType::ResourceBloom:
		ExploreValue = 180.0f;
		DangerValue = 0.0f;
		break;
	case ESIPPetEcoNodeType::EnemyNest:
		ExploreValue = 120.0f;
		DangerValue = 180.0f;
		break;
	case ESIPPetEcoNodeType::HealSpring:
		ExploreValue = 90.0f;
		DangerValue = -80.0f;
		break;
	case ESIPPetEcoNodeType::WindPulse:
		ExploreValue = 220.0f;
		DangerValue = 0.0f;
		break;
	default:
		break;
	}
}

void ASIPPetEcoNode::MarkActivated()
{
	bActivated = true;
	NodeMesh->SetRelativeScale3D(FVector(0.95f, 0.95f, 0.95f));
	LabelText->SetText(FText::FromString(GetNodeName() + TEXT(" DONE")));
	LabelText->SetTextRenderColor(FColor::White);
}

FVector ASIPPetEcoNode::GetInteractionLocation() const
{
	return GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
}

FString ASIPPetEcoNode::GetNodeName() const
{
	switch (NodeType)
	{
	case ESIPPetEcoNodeType::BridgeSeed:
		return TEXT("Bridge Seed");
	case ESIPPetEcoNodeType::ResourceBloom:
		return TEXT("Resource Bloom");
	case ESIPPetEcoNodeType::EnemyNest:
		return TEXT("Enemy Nest");
	case ESIPPetEcoNodeType::HealSpring:
		return TEXT("Heal Spring");
	case ESIPPetEcoNodeType::WindPulse:
		return TEXT("Wind Pulse");
	default:
		return TEXT("Eco Node");
	}
}

FLinearColor ASIPPetEcoNode::GetNodeColor() const
{
	switch (NodeType)
	{
	case ESIPPetEcoNodeType::BridgeSeed:
		return FLinearColor(0.15f, 0.85f, 0.35f);
	case ESIPPetEcoNodeType::ResourceBloom:
		return FLinearColor(0.25f, 0.95f, 0.90f);
	case ESIPPetEcoNodeType::EnemyNest:
		return FLinearColor(0.95f, 0.20f, 0.12f);
	case ESIPPetEcoNodeType::HealSpring:
		return FLinearColor(0.25f, 0.55f, 1.0f);
	case ESIPPetEcoNodeType::WindPulse:
		return FLinearColor(0.80f, 1.0f, 0.25f);
	default:
		return FLinearColor::White;
	}
}
