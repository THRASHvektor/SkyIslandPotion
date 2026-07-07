// Copyright Epic Games, Inc. All Rights Reserved.

#include "Demo/SIPDemoPet.h"

#include "Demo/SIPDemoEnemy.h"
#include "Demo/SIPPetEcoDemoManager.h"
#include "Demo/SIPPetEcoNode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"

ASIPDemoPet::ASIPDemoPet()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.35f));

	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreMesh"));
	CoreMesh->SetupAttachment(RootComponent);
	CoreMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreMesh->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
	CoreMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 38.0f));

	ThoughtText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ThoughtText"));
	ThoughtText->SetupAttachment(RootComponent);
	ThoughtText->SetHorizontalAlignment(EHTA_Center);
	ThoughtText->SetVerticalAlignment(EVRTA_TextCenter);
	ThoughtText->SetWorldSize(58.0f);
	ThoughtText->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
	ThoughtText->SetRelativeRotation(FRotator(65.0f, 0.0f, 0.0f));
	ThoughtText->SetTextRenderColor(FColor::Cyan);
}

void ASIPDemoPet::InitializePet(ASIPPetEcoDemoManager* InManager, UStaticMesh* BodyStaticMesh, UStaticMesh* CoreStaticMesh, UMaterialInterface* Material)
{
	Manager = InManager;

	if (BodyStaticMesh)
	{
		BodyMesh->SetStaticMesh(BodyStaticMesh);
	}

	if (CoreStaticMesh)
	{
		CoreMesh->SetStaticMesh(CoreStaticMesh);
	}

	if (Material)
	{
		UMaterialInstanceDynamic* BodyMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (BodyMaterial)
		{
			BodyMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.20f, 0.95f, 0.55f));
			BodyMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.20f, 0.95f, 0.55f));
			BodyMesh->SetMaterial(0, BodyMaterial);
		}

		UMaterialInstanceDynamic* CoreMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (CoreMaterial)
		{
			CoreMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.95f, 0.15f));
			CoreMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.95f, 0.95f, 0.15f));
			CoreMesh->SetMaterial(0, CoreMaterial);
		}
	}

	MakeDecision();
	UpdateThoughtText();
}

void ASIPDemoPet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PulseTime += DeltaSeconds;
	const float Pulse = 1.0f + FMath::Sin(PulseTime * 5.0f) * 0.12f;
	CoreMesh->SetRelativeScale3D(FVector(0.25f * Pulse));
	AddActorWorldRotation(FRotator(0.0f, DeltaSeconds * 55.0f, 0.0f));

	if (!Manager)
	{
		return;
	}

	DecisionTimer -= DeltaSeconds;
	if (DecisionTimer <= 0.0f)
	{
		MakeDecision();
		DecisionTimer = DecisionInterval;
	}

	if (TargetEnemy && CurrentIntent == ESIPDemoPetIntent::AttackEnemy)
	{
		const FVector TargetLocation = TargetEnemy->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
		MoveToward(TargetLocation, DeltaSeconds);

		if (FVector::Dist(GetActorLocation(), TargetLocation) <= InteractionDistance)
		{
			Manager->NotifyPetReachedEnemy(this, TargetEnemy);
			TargetEnemy = nullptr;
			CurrentIntent = ESIPDemoPetIntent::Celebrate;
			DecisionTimer = 0.8f;
		}
	}
	else if (TargetNode && CurrentIntent == ESIPDemoPetIntent::TriggerEcoNode)
	{
		const FVector TargetLocation = TargetNode->GetInteractionLocation();
		MoveToward(TargetLocation, DeltaSeconds);

		if (FVector::Dist(GetActorLocation(), TargetLocation) <= InteractionDistance)
		{
			Manager->NotifyPetReachedNode(this, TargetNode);
			TargetNode = nullptr;
			CurrentIntent = ESIPDemoPetIntent::Celebrate;
			DecisionTimer = 0.8f;
		}
	}
	else
	{
		MoveToward(Manager->GetPetRestLocation(), DeltaSeconds);
	}

	UpdateThoughtText();
}

void ASIPDemoPet::MakeDecision()
{
	if (!Manager)
	{
		return;
	}

	float BestScore = 0.0f;
	ASIPPetEcoNode* BestNode = nullptr;
	ASIPDemoEnemy* BestEnemy = nullptr;

	for (const TObjectPtr<ASIPDemoEnemy>& EnemyPtr : Manager->GetDemoEnemies())
	{
		ASIPDemoEnemy* Enemy = EnemyPtr.Get();
		const float EnemyScore = ScoreEnemy(Enemy);
		if (EnemyScore > BestScore)
		{
			BestScore = EnemyScore;
			BestEnemy = Enemy;
			BestNode = nullptr;
		}
	}

	for (const TObjectPtr<ASIPPetEcoNode>& NodePtr : Manager->GetEcoNodes())
	{
		ASIPPetEcoNode* Node = NodePtr.Get();
		const float NodeScore = ScoreNode(Node);
		if (NodeScore > BestScore)
		{
			BestScore = NodeScore;
			BestNode = Node;
			BestEnemy = nullptr;
		}
	}

	TargetNode = BestNode;
	TargetEnemy = BestEnemy;

	if (TargetEnemy)
	{
		CurrentIntent = ESIPDemoPetIntent::AttackEnemy;
	}
	else if (TargetNode)
	{
		CurrentIntent = ESIPDemoPetIntent::TriggerEcoNode;
	}
	else
	{
		CurrentIntent = ESIPDemoPetIntent::FollowPlayer;
	}
}

void ASIPDemoPet::MoveToward(const FVector& TargetLocation, float DeltaSeconds)
{
	const FVector CurrentLocation = GetActorLocation();
	const FVector ToTarget = TargetLocation - CurrentLocation;
	const float Distance = ToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Direction = ToTarget / Distance;
	const FVector NewLocation = CurrentLocation + Direction * FMath::Min(Distance, MoveSpeed * DeltaSeconds);
	SetActorLocation(NewLocation);

	if (Direction.SizeSquared2D() > KINDA_SMALL_NUMBER)
	{
		SetActorRotation(Direction.Rotation());
	}

	if (Manager->ShouldShowDebug())
	{
		DrawDebugLine(GetWorld(), CurrentLocation, TargetLocation, FColor::Cyan, false, 0.08f, 0, 3.0f);
	}
}

void ASIPDemoPet::UpdateThoughtText()
{
	ThoughtText->SetText(FText::FromString(GetIntentText()));
}

float ASIPDemoPet::ScoreNode(const ASIPPetEcoNode* Node) const
{
	if (!Node || Node->bActivated)
	{
		return 0.0f;
	}

	const float Distance = FVector::Dist(GetActorLocation(), Node->GetActorLocation());
	const float DistancePenalty = Distance * 0.08f;
	float Score = Node->ExploreValue + Node->DangerValue - DistancePenalty;

	if (Manager && Node->NodeType == ESIPPetEcoNodeType::BridgeSeed && !Manager->IsBridgeBuilt())
	{
		Score += 240.0f;
	}

	if (Manager && Node->NodeType == ESIPPetEcoNodeType::ResourceBloom && Manager->IsBridgeBuilt())
	{
		Score += 80.0f;
	}

	return FMath::Max(0.0f, Score);
}

float ASIPDemoPet::ScoreEnemy(const ASIPDemoEnemy* Enemy) const
{
	if (!Enemy || !Enemy->IsAlive())
	{
		return 0.0f;
	}

	const float Distance = FVector::Dist(GetActorLocation(), Enemy->GetActorLocation());
	return FMath::Max(0.0f, 320.0f - Distance * 0.12f);
}

FString ASIPDemoPet::GetIntentText() const
{
	switch (CurrentIntent)
	{
	case ESIPDemoPetIntent::FollowPlayer:
		return TEXT("AI: Guarding");
	case ESIPDemoPetIntent::TriggerEcoNode:
		return TargetNode ? FString::Printf(TEXT("AI: %s"), *TargetNode->GetNodeName()) : TEXT("AI: Sensing");
	case ESIPDemoPetIntent::AttackEnemy:
		return TEXT("AI: Protect");
	case ESIPDemoPetIntent::Celebrate:
		return TEXT("AI: Done!");
	case ESIPDemoPetIntent::Idle:
	default:
		return TEXT("AI: Idle");
	}
}
