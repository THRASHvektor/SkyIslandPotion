// Copyright Epic Games, Inc. All Rights Reserved.

#include "Demo/SIPPetEcoDemoManager.h"

#include "Demo/SIPDemoEnemy.h"
#include "Demo/SIPDemoPet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ASIPPetEcoDemoManager::ASIPPetEcoDemoManager()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	DemoTitleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DemoTitleText"));
	DemoTitleText->SetupAttachment(RootComponent);
	DemoTitleText->SetHorizontalAlignment(EHTA_Center);
	DemoTitleText->SetVerticalAlignment(EVRTA_TextCenter);
	DemoTitleText->SetWorldSize(86.0f);
	DemoTitleText->SetRelativeLocation(FVector(600.0f, -850.0f, 420.0f));
	DemoTitleText->SetRelativeRotation(FRotator(62.0f, 0.0f, 0.0f));
	DemoTitleText->SetText(FText::FromString(TEXT("Pet Eco AI / PCG Demo")));

	PetClass = ASIPDemoPet::StaticClass();
	EcoNodeClass = ASIPPetEcoNode::StaticClass();
	EnemyClass = ASIPDemoEnemy::StaticClass();

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	CubeMesh = CubeAsset.Object;
	SphereMesh = SphereAsset.Object;
	CylinderMesh = CylinderAsset.Object;
	DemoMaterial = MaterialAsset.Object;
}

void ASIPPetEcoDemoManager::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		BuildDemoScene();
	}
}

void ASIPPetEcoDemoManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bShowDebug || !DemoPet)
	{
		return;
	}

	DebugTime += DeltaSeconds;
	if (DebugTime >= 0.2f)
	{
		DebugTime = 0.0f;
		DrawDebugString(
			GetWorld(),
			DemoPet->GetActorLocation() + FVector(0.0f, 0.0f, 210.0f),
			DemoPet->GetIntentText(),
			nullptr,
			FColor::Cyan,
			0.22f,
			true);
	}
}

void ASIPPetEcoDemoManager::BuildDemoScene()
{
	ClearDemoScene();

	MainIslandCenter = GetActorLocation();
	FarIslandCenter = GetActorLocation() + FVector(2300.0f, 0.0f, 220.0f) * DemoScale;
	BridgeStart = GetActorLocation() + FVector(760.0f, 0.0f, 120.0f) * DemoScale;
	BridgeEnd = GetActorLocation() + FVector(1690.0f, 0.0f, 245.0f) * DemoScale;

	GenerateBaseIslands();

	SpawnEcoNode(GetActorLocation() + FVector(520.0f, -280.0f, 150.0f) * DemoScale, ESIPPetEcoNodeType::BridgeSeed);
	SpawnEcoNode(GetActorLocation() + FVector(-410.0f, 330.0f, 140.0f) * DemoScale, ESIPPetEcoNodeType::ResourceBloom);
	SpawnEcoNode(GetActorLocation() + FVector(220.0f, 470.0f, 140.0f) * DemoScale, ESIPPetEcoNodeType::HealSpring);
	SpawnEcoNode(GetActorLocation() + FVector(1460.0f, -260.0f, 280.0f) * DemoScale, ESIPPetEcoNodeType::WindPulse);
	SpawnEcoNode(GetActorLocation() + FVector(2030.0f, 280.0f, 330.0f) * DemoScale, ESIPPetEcoNodeType::EnemyNest);

	SpawnDemoEnemy(GetActorLocation() + FVector(2060.0f, 30.0f, 360.0f) * DemoScale);

	FActorSpawnParameters PetParams;
	PetParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TSubclassOf<ASIPDemoPet> PetSpawnClass = PetClass;
	if (!PetSpawnClass)
	{
		PetSpawnClass = ASIPDemoPet::StaticClass();
	}

	DemoPet = GetWorld()->SpawnActor<ASIPDemoPet>(
		PetSpawnClass,
		GetPetRestLocation(),
		FRotator::ZeroRotator,
		PetParams);

	if (DemoPet)
	{
		DemoPet->InitializePet(this, GetSphereMesh(), GetSphereMesh(), GetDemoMaterial());
	}

	SetTitle(TEXT("Pet Eco AI: sensing island nodes..."), FColor::Cyan);
}

void ASIPPetEcoDemoManager::ClearDemoScene()
{
	for (const TObjectPtr<UStaticMeshComponent>& ComponentPtr : GeneratedComponents)
	{
		UStaticMeshComponent* Component = ComponentPtr.Get();
		if (Component)
		{
			Component->DestroyComponent();
		}
	}
	GeneratedComponents.Empty();

	for (const TObjectPtr<ASIPPetEcoNode>& NodePtr : EcoNodes)
	{
		ASIPPetEcoNode* Node = NodePtr.Get();
		if (IsValid(Node))
		{
			Node->Destroy();
		}
	}
	EcoNodes.Empty();

	for (const TObjectPtr<ASIPDemoEnemy>& EnemyPtr : DemoEnemies)
	{
		ASIPDemoEnemy* Enemy = EnemyPtr.Get();
		if (IsValid(Enemy))
		{
			Enemy->Destroy();
		}
	}
	DemoEnemies.Empty();

	if (IsValid(DemoPet))
	{
		DemoPet->Destroy();
	}
	DemoPet = nullptr;
	bBridgeBuilt = false;
}

void ASIPPetEcoDemoManager::NotifyPetReachedNode(ASIPDemoPet* Pet, ASIPPetEcoNode* Node)
{
	if (!Pet || !Node || Node->bActivated)
	{
		return;
	}

	Node->MarkActivated();

	switch (Node->NodeType)
	{
	case ESIPPetEcoNodeType::BridgeSeed:
		GenerateBridge();
		GenerateWindPulse(Node->GetActorLocation());
		SetTitle(TEXT("Pet AI chose Bridge Seed: procedural path generated"), FColor::Green);
		break;
	case ESIPPetEcoNodeType::ResourceBloom:
		GenerateResourceBloom(Node->GetActorLocation());
		SetTitle(TEXT("Pet AI chose Resource Bloom: crystals spawned"), FColor::Cyan);
		break;
	case ESIPPetEcoNodeType::EnemyNest:
		SetTitle(TEXT("Pet AI marked enemy nest: switching to combat"), FColor::Red);
		break;
	case ESIPPetEcoNodeType::HealSpring:
		GenerateHealPulse(Node->GetActorLocation());
		SetTitle(TEXT("Pet AI activated Heal Spring: safe zone visible"), FColor::Blue);
		break;
	case ESIPPetEcoNodeType::WindPulse:
		GenerateBridge();
		GenerateWindPulse(Node->GetActorLocation());
		SetTitle(TEXT("Pet AI activated Wind Pulse: bridge reinforced"), FColor::Green);
		break;
	default:
		break;
	}
}

void ASIPPetEcoDemoManager::NotifyPetReachedEnemy(ASIPDemoPet* Pet, ASIPDemoEnemy* Enemy)
{
	if (!Pet || !Enemy || !Enemy->IsAlive())
	{
		return;
	}

	Enemy->TakeDemoDamage(35.0f);
	GenerateWindPulse(Enemy->GetActorLocation());
	SetTitle(Enemy->IsAlive() ? TEXT("Pet AI attacked enemy") : TEXT("Pet AI cleared combat threat"), FColor::Red);
}

FVector ASIPPetEcoDemoManager::GetPetRestLocation() const
{
	return GetActorLocation() + FVector(-220.0f, -180.0f, 210.0f) * DemoScale;
}

UStaticMeshComponent* ASIPPetEcoDemoManager::CreateDemoMeshComponent(
	const FName ComponentName,
	UStaticMesh* Mesh,
	const FVector& RelativeLocation,
	const FVector& RelativeScale,
	const FLinearColor& Color,
	bool bEnableCollision)
{
	const FName UniqueComponentName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), ComponentName);
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, UniqueComponentName);
	if (!Component)
	{
		return nullptr;
	}

	Component->SetStaticMesh(Mesh ? Mesh : GetCubeMesh());
	Component->SetRelativeLocation(RelativeLocation);
	Component->SetRelativeScale3D(RelativeScale);
	Component->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Component->SetCollisionResponseToAllChannels(bEnableCollision ? ECR_Block : ECR_Ignore);
	Component->SetMobility(EComponentMobility::Movable);
	Component->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	if (UMaterialInterface* Material = GetDemoMaterial())
	{
		UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
		if (DynamicMaterial)
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Component->SetMaterial(0, DynamicMaterial);
		}
	}

	Component->RegisterComponent();
	AddInstanceComponent(Component);
	GeneratedComponents.Add(Component);
	return Component;
}

ASIPPetEcoNode* ASIPPetEcoDemoManager::SpawnEcoNode(const FVector& Location, ESIPPetEcoNodeType NodeType)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<ASIPPetEcoNode> NodeSpawnClass = EcoNodeClass;
	if (!NodeSpawnClass)
	{
		NodeSpawnClass = ASIPPetEcoNode::StaticClass();
	}

	ASIPPetEcoNode* Node = GetWorld()->SpawnActor<ASIPPetEcoNode>(
		NodeSpawnClass,
		Location,
		FRotator::ZeroRotator,
		Params);

	if (Node)
	{
		Node->ConfigureNode(NodeType, GetSphereMesh(), GetDemoMaterial(), Node->GetNodeColor());
		EcoNodes.Add(Node);
	}

	return Node;
}

ASIPDemoEnemy* ASIPPetEcoDemoManager::SpawnDemoEnemy(const FVector& Location)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TSubclassOf<ASIPDemoEnemy> EnemySpawnClass = EnemyClass;
	if (!EnemySpawnClass)
	{
		EnemySpawnClass = ASIPDemoEnemy::StaticClass();
	}

	ASIPDemoEnemy* Enemy = GetWorld()->SpawnActor<ASIPDemoEnemy>(
		EnemySpawnClass,
		Location,
		FRotator::ZeroRotator,
		Params);

	if (Enemy)
	{
		Enemy->ConfigureEnemy(GetSphereMesh(), GetDemoMaterial());
		DemoEnemies.Add(Enemy);
	}

	return Enemy;
}

void ASIPPetEcoDemoManager::GenerateBaseIslands()
{
	CreateDemoMeshComponent(
		TEXT("MainIsland"),
		GetCubeMesh(),
		FVector(0.0f, 0.0f, -40.0f) * DemoScale,
		FVector(15.0f, 10.0f, 0.8f) * DemoScale,
		FLinearColor(0.12f, 0.34f, 0.18f),
		true);

	CreateDemoMeshComponent(
		TEXT("MainIslandUnderside"),
		GetSphereMesh(),
		FVector(0.0f, 0.0f, -230.0f) * DemoScale,
		FVector(7.0f, 5.0f, 2.2f) * DemoScale,
		FLinearColor(0.18f, 0.13f, 0.09f),
		true);

	CreateDemoMeshComponent(
		TEXT("FarIsland"),
		GetCubeMesh(),
		FarIslandCenter - GetActorLocation() + FVector(0.0f, 0.0f, -50.0f) * DemoScale,
		FVector(10.0f, 8.0f, 0.8f) * DemoScale,
		FLinearColor(0.12f, 0.22f, 0.36f),
		true);

	CreateDemoMeshComponent(
		TEXT("FarIslandUnderside"),
		GetSphereMesh(),
		FarIslandCenter - GetActorLocation() + FVector(0.0f, 0.0f, -230.0f) * DemoScale,
		FVector(5.0f, 4.0f, 2.0f) * DemoScale,
		FLinearColor(0.13f, 0.10f, 0.16f),
		true);

	for (int32 Index = 0; Index < 9; ++Index)
	{
		const float Angle = Index * 40.0f;
		const float Radius = 520.0f + (Index % 3) * 70.0f;
		const FVector Offset(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
			70.0f + (Index % 2) * 35.0f);

		CreateDemoMeshComponent(
			*FString::Printf(TEXT("FloatingShard_%d"), Index),
			GetSphereMesh(),
			Offset * DemoScale,
			FVector(0.7f, 0.7f, 0.22f) * DemoScale,
			FLinearColor(0.18f, 0.42f, 0.26f),
			true);
	}
}

void ASIPPetEcoDemoManager::GenerateBridge()
{
	if (bBridgeBuilt)
	{
		return;
	}

	bBridgeBuilt = true;
	const int32 Steps = FMath::Max(BridgeStepCount, 4);
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(Steps - 1);
		const FVector WorldPoint = FMath::Lerp(BridgeStart, BridgeEnd, Alpha);
		const float ArcHeight = FMath::Sin(Alpha * PI) * 140.0f * DemoScale;
		const FVector RelativePoint = WorldPoint - GetActorLocation() + FVector(0.0f, 0.0f, ArcHeight);

		const float WidthPulse = 1.0f + FMath::Sin(Alpha * PI * 4.0f) * 0.12f;
		CreateDemoMeshComponent(
			*FString::Printf(TEXT("PetGeneratedBridge_%d"), Index),
			GetCubeMesh(),
			RelativePoint,
			FVector(1.55f, 1.0f * WidthPulse, 0.18f) * DemoScale,
			FLinearColor(0.20f, 0.82f, 0.32f),
			true);

		if (Index % 2 == 0)
		{
			CreateDemoMeshComponent(
				*FString::Printf(TEXT("BridgeGlow_%d"), Index),
				GetSphereMesh(),
				RelativePoint + FVector(0.0f, 0.0f, 42.0f) * DemoScale,
				FVector(0.18f, 0.18f, 0.18f) * DemoScale,
				FLinearColor(0.82f, 1.0f, 0.26f),
				false);
		}
	}
}

void ASIPPetEcoDemoManager::GenerateResourceBloom(const FVector& CenterLocation)
{
	FRandomStream Rand(WorldSeed + 88);
	for (int32 Index = 0; Index < 12; ++Index)
	{
		const float Angle = Rand.FRandRange(0.0f, 360.0f);
		const float Radius = Rand.FRandRange(80.0f, 260.0f) * DemoScale;
		const FVector Offset(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
			Rand.FRandRange(15.0f, 85.0f) * DemoScale);

		CreateDemoMeshComponent(
			*FString::Printf(TEXT("PetCrystal_%d"), Index),
			GetSphereMesh(),
			CenterLocation - GetActorLocation() + Offset,
			FVector(0.22f, 0.22f, Rand.FRandRange(0.35f, 0.7f)) * DemoScale,
			FLinearColor(0.15f, 0.95f, 1.0f),
			true);
	}
}

void ASIPPetEcoDemoManager::GenerateHealPulse(const FVector& CenterLocation)
{
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const float Angle = Index * 36.0f;
		const FVector Offset(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * 230.0f,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * 230.0f,
			25.0f);

		CreateDemoMeshComponent(
			*FString::Printf(TEXT("HealPulse_%d"), Index),
			GetSphereMesh(),
			CenterLocation - GetActorLocation() + Offset * DemoScale,
			FVector(0.16f, 0.16f, 0.16f) * DemoScale,
			FLinearColor(0.25f, 0.70f, 1.0f),
			false);
	}
}

void ASIPPetEcoDemoManager::GenerateWindPulse(const FVector& CenterLocation)
{
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const float Angle = Index * 45.0f;
		const FVector Offset(
			FMath::Cos(FMath::DegreesToRadians(Angle)) * 150.0f,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * 150.0f,
			80.0f + Index * 16.0f);

		CreateDemoMeshComponent(
			*FString::Printf(TEXT("WindPulse_%d_%d"), FMath::RoundToInt(CenterLocation.X), Index),
			GetSphereMesh(),
			CenterLocation - GetActorLocation() + Offset * DemoScale,
			FVector(0.10f, 0.10f, 0.10f) * DemoScale,
			FLinearColor(0.78f, 1.0f, 0.18f),
			false);
	}
}

void ASIPPetEcoDemoManager::SetTitle(const FString& Message, const FColor& Color)
{
	DemoTitleText->SetText(FText::FromString(Message));
	DemoTitleText->SetTextRenderColor(Color);
}

UMaterialInterface* ASIPPetEcoDemoManager::GetDemoMaterial() const
{
	return DemoMaterial.Get();
}

UStaticMesh* ASIPPetEcoDemoManager::GetCubeMesh() const
{
	return CubeMesh.Get();
}

UStaticMesh* ASIPPetEcoDemoManager::GetSphereMesh() const
{
	return SphereMesh ? SphereMesh.Get() : CubeMesh.Get();
}

UStaticMesh* ASIPPetEcoDemoManager::GetCylinderMesh() const
{
	return CylinderMesh ? CylinderMesh.Get() : CubeMesh.Get();
}
