// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPResourcePlantActor.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/SIPResourcePlantRecipe.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FLinearColor AddGlow(const FLinearColor& Base, const FLinearColor& Glow, float Alpha)
	{
		return FMath::Lerp(Base, Glow, FMath::Clamp(Alpha, 0.f, 1.f));
	}

	float SeverityAlpha(const FSIPResourcePlantCollapseMetrics& Metrics)
	{
		return FMath::Clamp(Metrics.Severity / 3.f, 0.f, 1.f);
	}

	float DispersionAlpha(const FSIPResourcePlantCollapseMetrics& Metrics)
	{
		return FMath::Clamp(Metrics.Dispersion / 1.25f, 0.f, 1.f);
	}
}

ASIPResourcePlantActor::ASIPResourcePlantActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
	InteractionBounds->SetupAttachment(SceneRoot);
	InteractionBounds->SetBoxExtent(FVector(130.f, 130.f, 210.f));
	InteractionBounds->SetRelativeLocation(FVector(0.f, 0.f, 135.f));
	InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBounds->SetCollisionResponseToAllChannels(ECR_Overlap);

	BodyShellRootSlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RootSlot_Root"));
	BodyShellRootSlot->SetupAttachment(SceneRoot);
	RootSlot = BodyShellRootSlot;

	BodyShellAxisSlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StemSlot_Stem"));
	BodyShellAxisSlot->SetupAttachment(SceneRoot);
	StemSlot = BodyShellAxisSlot;

	PrimarySilhouetteSlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RewardSlot_Petal"));
	PrimarySilhouetteSlot->SetupAttachment(SceneRoot);
	PetalRewardSlot = PrimarySilhouetteSlot;

	CoreSlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreSlot_Core"));
	CoreSlot->SetupAttachment(SceneRoot);

	OrbitNodeASlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RewardSlot_Spore"));
	OrbitNodeASlot->SetupAttachment(SceneRoot);
	SporeRewardSlot = OrbitNodeASlot;

	OrbitNodeBSlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RewardSlot_Crystal"));
	OrbitNodeBSlot->SetupAttachment(SceneRoot);
	CrystalRewardSlot = OrbitNodeBSlot;

	OrbitProxyRingSlot = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReactionShellSlot"));
	OrbitProxyRingSlot->SetupAttachment(SceneRoot);
	ReactionShellSlot = OrbitProxyRingSlot;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		PreviewCubeMesh = CubeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		PreviewSphereMesh = SphereMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		PreviewCylinderMesh = CylinderMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		PreviewConeMesh = ConeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MaterialFinder.Succeeded())
	{
		PreviewBaseMaterial = MaterialFinder.Object;
	}
}

void ASIPResourcePlantActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bConfigureFromPCGProperties)
	{
		ApplyPCGPropertyConfiguration();
		return;
	}

	RebuildPlantPreview();
}

void ASIPResourcePlantActor::BeginPlay()
{
	Super::BeginPlay();

	if (bConfigureFromPCGProperties)
	{
		ApplyPCGPropertyConfiguration();
		return;
	}

	RebuildPlantPreview();
}

void ASIPResourcePlantActor::RebuildPlantPreview()
{
	ResolvedCollapseVector = ResolveCollapseVector();
	ResolvedCollapseMetrics = CalculateCollapseMetrics(ResolvedCollapseVector);

	const float Scale = FMath::Max(0.1f, PlantScale);
	const ESIPResourcePlantPreviewFamily EffectiveFamily = GetEffectivePreviewFamily();
	FRandomStream Random(PlantSeed);
	const float YawOffset = Random.FRandRange(-12.f, 12.f);

	switch (EffectiveFamily)
	{
	case ESIPResourcePlantPreviewFamily::AetherVine:
		InteractionBounds->SetBoxExtent(FVector(190.f, 190.f, 250.f) * Scale);
		InteractionBounds->SetRelativeLocation(FVector(0.f, 0.f, 160.f) * Scale);
		ConfigureAetherVinePreview(Scale, ResolvedCollapseVector, ResolvedCollapseMetrics, YawOffset);
		break;
	case ESIPResourcePlantPreviewFamily::CrownLily:
	default:
		InteractionBounds->SetBoxExtent(FVector(150.f, 150.f, 230.f) * Scale);
		InteractionBounds->SetRelativeLocation(FVector(0.f, 0.f, 145.f) * Scale);
		ConfigureCrownLilyPreview(Scale, ResolvedCollapseVector, ResolvedCollapseMetrics, YawOffset);
		break;
	}

	ApplyRecipeMeshes(Scale, ResolvedCollapseVector, ResolvedCollapseMetrics);
}

void ASIPResourcePlantActor::RerollCollapseVector()
{
	FRandomStream Random(PlantSeed ^ 137);
	PlantSeed = Random.RandRange(1, 2000000000);
	bUseExplicitCollapseVector = false;
	RebuildPlantPreview();
}

FSIPResourcePlantCollapseVector ASIPResourcePlantActor::ApplyPCGPropertyConfiguration()
{
	return ConfigureFromPCG(
		PlantRecipe,
		PreviewFamily,
		CollapseBand,
		PlantSeed,
		PlantScale,
		CollapseTemperatureScale,
		bUseRecipeDefaultCollapseBand);
}

FSIPResourcePlantCollapseVector ASIPResourcePlantActor::ConfigureFromPCG(
	USIPResourcePlantRecipe* InRecipe,
	ESIPResourcePlantPreviewFamily InFamily,
	ESIPResourcePlantCollapseBand InCollapseBand,
	int32 InSeed,
	float InScale,
	float InCollapseTemperatureScale,
	bool bInUseRecipeDefaultCollapseBand)
{
	PlantRecipe = InRecipe;
	PreviewFamily = InRecipe ? InRecipe->Family : InFamily;
	CollapseBand = InCollapseBand;
	PlantSeed = InSeed;
	PlantScale = FMath::Max(0.1f, InScale);
	CollapseTemperatureScale = FMath::Clamp(InCollapseTemperatureScale, 0.05f, 2.5f);
	bUseRecipeDefaultCollapseBand = bInUseRecipeDefaultCollapseBand;
	bUseExplicitCollapseVector = false;

	RebuildPlantPreview();
	return ResolvedCollapseVector;
}

FSIPResourcePlantCollapseVector ASIPResourcePlantActor::GetActiveCollapseVector() const
{
	return ResolvedCollapseVector;
}

FSIPResourcePlantCollapseMetrics ASIPResourcePlantActor::GetActiveCollapseMetrics() const
{
	return ResolvedCollapseMetrics;
}

void ASIPResourcePlantActor::ValidateAssignedRecipe(const FSIPResourcePlantRecipeValidationOptions& Options, TArray<FSIPResourcePlantRecipeValidationIssue>& OutIssues) const
{
	OutIssues.Reset();

	if (!PlantRecipe)
	{
		FSIPResourcePlantRecipeValidationIssue Issue;
		Issue.Severity = ESIPResourcePlantRecipeIssueSeverity::Error;
		Issue.Code = ESIPResourcePlantRecipeIssueCode::MissingRecipe;
		Issue.Slot = ESIPResourcePlantSlot::BodyShell;
		Issue.CollapseLevel = INDEX_NONE;
		Issue.Message = TEXT("No PlantRecipe is assigned to this resource plant actor.");
		OutIssues.Add(Issue);
		return;
	}

	PlantRecipe->ValidateRecipe(Options, OutIssues);
}

bool ASIPResourcePlantActor::IsAssignedRecipeReadyForAssembly(const FSIPResourcePlantRecipeValidationOptions& Options) const
{
	TArray<FSIPResourcePlantRecipeValidationIssue> Issues;
	ValidateAssignedRecipe(Options, Issues);

	return !Issues.ContainsByPredicate([](const FSIPResourcePlantRecipeValidationIssue& Issue)
	{
		return Issue.Severity == ESIPResourcePlantRecipeIssueSeverity::Error;
	});
}

FSIPResourcePlantCollapseVector ASIPResourcePlantActor::ResolveCollapseVector() const
{
	if (bUseExplicitCollapseVector)
	{
		return SIPResourcePlantAssembly::SanitizeCollapseVector(ExplicitCollapseVector);
	}

	return SampleCollapseVector(PlantSeed);
}

FSIPResourcePlantCollapseVector ASIPResourcePlantActor::SampleCollapseVector(int32 Seed) const
{
	return SIPResourcePlantAssembly::SampleCollapseVector(Seed, GetEffectiveCollapseBand(), GetEffectivePreviewFamily(), CollapseTemperatureScale);
}

FSIPResourcePlantCollapseMetrics ASIPResourcePlantActor::CalculateCollapseMetrics(const FSIPResourcePlantCollapseVector& Vector) const
{
	return SIPResourcePlantAssembly::CalculateCollapseMetrics(Vector);
}

bool ASIPResourcePlantActor::PassesHardConstraints(const FSIPResourcePlantCollapseVector& Vector) const
{
	return SIPResourcePlantAssembly::PassesHardConstraints(Vector);
}

ESIPResourcePlantPreviewFamily ASIPResourcePlantActor::GetEffectivePreviewFamily() const
{
	return PlantRecipe ? PlantRecipe->Family : PreviewFamily;
}

ESIPResourcePlantCollapseBand ASIPResourcePlantActor::GetEffectiveCollapseBand() const
{
	if (PlantRecipe && bUseRecipeDefaultCollapseBand)
	{
		return PlantRecipe->DefaultCollapseBand;
	}

	return CollapseBand;
}

void ASIPResourcePlantActor::ConfigureCrownLilyPreview(float Scale, const FSIPResourcePlantCollapseVector& Vector, const FSIPResourcePlantCollapseMetrics& Metrics, float YawOffset)
{
	const float BodyAlpha = LevelAlpha(Vector.BodyShell);
	const float PrimaryAlpha = LevelAlpha(Vector.PrimarySilhouette);
	const float CoreAlpha = LevelAlpha(Vector.Core);
	const float OrbitAlpha = LevelAlpha(Vector.OrbitSet);
	const float GlobalAlpha = SeverityAlpha(Metrics);
	const float DriftAlpha = DispersionAlpha(Metrics);

	const float AxisTilt = 4.f + BodyAlpha * 8.f + DriftAlpha * 10.f;
	const float CrownLift = 24.f + PrimaryAlpha * 48.f + DriftAlpha * 18.f;
	const float OrbitSpread = 56.f + OrbitAlpha * 96.f + DriftAlpha * 34.f;
	const bool bShowHighCollapseRing = bShowCollapseProxyRing && (Vector.OrbitSet >= 2 || Metrics.Dispersion > 0.28f);

	const FLinearColor RootColor = AddGlow(FLinearColor(0.10f, 0.07f, 0.04f), FLinearColor(0.45f, 0.12f, 0.02f), BodyAlpha * 0.65f + GlobalAlpha * 0.25f);
	const FLinearColor StemColor = AddGlow(FLinearColor(0.16f, 0.08f, 0.03f), FLinearColor(0.85f, 0.28f, 0.04f), BodyAlpha * 0.35f + GlobalAlpha * 0.35f);
	const FLinearColor CoreColor = AddGlow(FLinearColor(1.0f, 0.62f, 0.05f), FLinearColor(1.0f, 0.16f, 0.02f), CoreAlpha * 0.55f);
	const FLinearColor CrownColor = AddGlow(FLinearColor(0.85f, 0.12f, 0.04f), FLinearColor(1.0f, 0.46f, 0.08f), PrimaryAlpha * 0.55f + GlobalAlpha * 0.20f);
	const FLinearColor OrbitColor = AddGlow(FLinearColor(1.0f, 0.70f, 0.10f), FLinearColor(1.0f, 0.22f, 0.02f), OrbitAlpha * 0.65f);

	ConfigureSlot(
		BodyShellRootSlot,
		PreviewCylinderMesh,
		FVector(0.f, 0.f, 12.f) * Scale,
		FRotator(0.f, 0.f, 0.f),
		FVector(0.95f + BodyAlpha * 0.10f, 0.95f + BodyAlpha * 0.10f, 0.14f) * Scale,
		RootColor,
		true,
		TEXT("Runtime.BodyShell.Root"));

	ConfigureSlot(
		BodyShellAxisSlot,
		PreviewCylinderMesh,
		FVector(0.f, 0.f, 94.f + BodyAlpha * 6.f) * Scale,
		FRotator(0.f, YawOffset, AxisTilt),
		FVector(0.15f + BodyAlpha * 0.03f, 0.15f + BodyAlpha * 0.03f, 1.42f + BodyAlpha * 0.08f) * Scale,
		StemColor,
		true,
		TEXT("Runtime.BodyShell.Axis"));

	ConfigureSlot(
		CoreSlot,
		PreviewSphereMesh,
		FVector(0.f, 0.f, 184.f + CoreAlpha * 22.f + DriftAlpha * 10.f) * Scale,
		FRotator(0.f, YawOffset * 0.25f, 0.f),
		FVector(0.32f + CoreAlpha * 0.07f, 0.32f + CoreAlpha * 0.07f, 0.32f + CoreAlpha * 0.07f) * Scale,
		CoreColor,
		true,
		TEXT("Runtime.Core"));

	ConfigureSlot(
		PrimarySilhouetteSlot,
		PreviewCubeMesh,
		FVector(0.f, 0.f, 220.f + CrownLift) * Scale,
		FRotator(0.f, 24.f + YawOffset, 16.f + PrimaryAlpha * 18.f + DriftAlpha * 12.f),
		FVector(1.42f + PrimaryAlpha * 0.24f, 0.16f + PrimaryAlpha * 0.04f, 0.32f) * Scale,
		CrownColor,
		true,
		TEXT("Runtime.PrimarySilhouette.Crown"));

	ConfigureSlot(
		OrbitNodeASlot,
		PreviewSphereMesh,
		FVector(OrbitSpread, 16.f + OrbitAlpha * 28.f, 216.f + CrownLift * 0.76f) * Scale,
		FRotator(0.f, 0.f, 0.f),
		FVector(0.16f + OrbitAlpha * 0.06f, 0.16f + OrbitAlpha * 0.06f, 0.16f + OrbitAlpha * 0.06f) * Scale,
		OrbitColor,
		bShowOrbitSet,
		TEXT("Runtime.OrbitSet.EmberA"));

	ConfigureSlot(
		OrbitNodeBSlot,
		PreviewSphereMesh,
		FVector(-OrbitSpread * 0.74f, -42.f - OrbitAlpha * 34.f, 232.f + CrownLift * 0.58f) * Scale,
		FRotator(0.f, 0.f, 0.f),
		FVector(0.13f + OrbitAlpha * 0.07f, 0.13f + OrbitAlpha * 0.07f, 0.13f + OrbitAlpha * 0.07f) * Scale,
		AddGlow(FLinearColor(1.f, 0.78f, 0.12f), FLinearColor(1.f, 0.22f, 0.02f), OrbitAlpha),
		bShowOrbitSet,
		TEXT("Runtime.OrbitSet.EmberB"));

	ConfigureSlot(
		OrbitProxyRingSlot,
		PreviewCylinderMesh,
		FVector(0.f, 0.f, 206.f + CrownLift * 0.58f) * Scale,
		FRotator(88.f, YawOffset, DriftAlpha * 12.f),
		FVector(1.20f + OrbitAlpha * 0.85f, 1.20f + OrbitAlpha * 0.85f, 0.022f) * Scale,
		FLinearColor(1.f, 0.55f, 0.08f, 0.42f),
		bShowOrbitSet && bShowHighCollapseRing,
		TEXT("Runtime.FXProxy.CollapseOrbit"));
}

void ASIPResourcePlantActor::ConfigureAetherVinePreview(float Scale, const FSIPResourcePlantCollapseVector& Vector, const FSIPResourcePlantCollapseMetrics& Metrics, float YawOffset)
{
	const float BodyAlpha = LevelAlpha(Vector.BodyShell);
	const float PrimaryAlpha = LevelAlpha(Vector.PrimarySilhouette);
	const float CoreAlpha = LevelAlpha(Vector.Core);
	const float OrbitAlpha = LevelAlpha(Vector.OrbitSet);
	const float GlobalAlpha = SeverityAlpha(Metrics);
	const float DriftAlpha = DispersionAlpha(Metrics);

	const float ArcLift = 12.f + PrimaryAlpha * 42.f + DriftAlpha * 22.f;
	const float OrbitSpread = 92.f + OrbitAlpha * 110.f + DriftAlpha * 40.f;
	const bool bShowStarRing = bShowCollapseProxyRing && (Vector.OrbitSet >= 1 || Vector.PrimarySilhouette >= 2 || Metrics.Dispersion > 0.20f);

	const FLinearColor RootColor = AddGlow(FLinearColor(0.08f, 0.04f, 0.17f), FLinearColor(0.18f, 0.06f, 0.34f), BodyAlpha * 0.75f);
	const FLinearColor VineColor = AddGlow(FLinearColor(0.10f, 0.08f, 0.34f), FLinearColor(0.18f, 0.42f, 1.0f), BodyAlpha * 0.35f + GlobalAlpha * 0.35f);
	const FLinearColor ArcColor = AddGlow(FLinearColor(0.18f, 0.10f, 0.52f), FLinearColor(0.64f, 0.36f, 1.0f), PrimaryAlpha * 0.55f + 0.20f);
	const FLinearColor CoreColor = AddGlow(FLinearColor(0.15f, 0.65f, 1.0f), FLinearColor(0.95f, 0.78f, 1.0f), CoreAlpha * 0.55f);
	const FLinearColor OrbitColor = AddGlow(FLinearColor(0.75f, 0.63f, 1.0f), FLinearColor(0.24f, 0.84f, 1.0f), OrbitAlpha * 0.65f);

	ConfigureSlot(
		BodyShellRootSlot,
		PreviewSphereMesh,
		FVector(0.f, 0.f, 22.f) * Scale,
		FRotator(0.f, 0.f, 0.f),
		FVector(0.45f + BodyAlpha * 0.08f, 0.35f + BodyAlpha * 0.08f, 0.22f + BodyAlpha * 0.04f) * Scale,
		RootColor,
		true,
		TEXT("Runtime.BodyShell.RootKnot"));

	ConfigureSlot(
		BodyShellAxisSlot,
		PreviewCylinderMesh,
		FVector(-28.f - BodyAlpha * 8.f, 0.f, 118.f + ArcLift * 0.22f) * Scale,
		FRotator(24.f + BodyAlpha * 10.f, YawOffset + 18.f, -28.f - DriftAlpha * 12.f),
		FVector(0.13f + BodyAlpha * 0.03f, 0.13f + BodyAlpha * 0.03f, 1.72f + BodyAlpha * 0.20f) * Scale,
		VineColor,
		true,
		TEXT("Runtime.BodyShell.MainFlow"));

	ConfigureSlot(
		CoreSlot,
		PreviewSphereMesh,
		FVector(-38.f - DriftAlpha * 10.f, 0.f, 174.f + ArcLift) * Scale,
		FRotator(0.f, 0.f, 0.f),
		FVector(0.34f + CoreAlpha * 0.08f, 0.34f + CoreAlpha * 0.08f, 0.34f + CoreAlpha * 0.08f) * Scale,
		CoreColor,
		true,
		TEXT("Runtime.Core"));

	ConfigureSlot(
		PrimarySilhouetteSlot,
		PreviewCylinderMesh,
		FVector(42.f + PrimaryAlpha * 10.f, 0.f, 224.f + ArcLift) * Scale,
		FRotator(0.f, YawOffset + 62.f, 62.f + PrimaryAlpha * 18.f + DriftAlpha * 12.f),
		FVector(0.10f + PrimaryAlpha * 0.03f, 0.10f + PrimaryAlpha * 0.03f, 2.10f + PrimaryAlpha * 0.25f) * Scale,
		ArcColor,
		true,
		TEXT("Runtime.PrimarySilhouette.PrebakedArc"));

	ConfigureSlot(
		OrbitNodeASlot,
		PreviewSphereMesh,
		FVector(OrbitSpread * 0.55f, 42.f + OrbitAlpha * 34.f, 250.f + ArcLift) * Scale,
		FRotator(0.f, 0.f, 0.f),
		FVector(0.15f + OrbitAlpha * 0.07f, 0.15f + OrbitAlpha * 0.07f, 0.15f + OrbitAlpha * 0.07f) * Scale,
		OrbitColor,
		bShowOrbitSet,
		TEXT("Runtime.OrbitSet.StarNodeA"));

	ConfigureSlot(
		OrbitNodeBSlot,
		PreviewCubeMesh,
		FVector(OrbitSpread, -22.f - OrbitAlpha * 46.f, 286.f + ArcLift * 0.72f) * Scale,
		FRotator(22.f + OrbitAlpha * 12.f, YawOffset + 18.f, 26.f),
		FVector(0.26f + OrbitAlpha * 0.10f, 0.08f + OrbitAlpha * 0.03f, 0.26f + OrbitAlpha * 0.10f) * Scale,
		AddGlow(FLinearColor(0.25f, 0.82f, 1.0f), FLinearColor(0.86f, 0.64f, 1.0f), OrbitAlpha),
		bShowOrbitSet,
		TEXT("Runtime.OrbitSet.GlyphPlateB"));

	ConfigureSlot(
		OrbitProxyRingSlot,
		PreviewCylinderMesh,
		FVector(4.f, 0.f, 214.f + ArcLift * 0.62f) * Scale,
		FRotator(0.f, YawOffset + 12.f, 78.f + DriftAlpha * 16.f),
		FVector(0.72f + OrbitAlpha * 0.72f, 0.72f + OrbitAlpha * 0.72f, 0.018f) * Scale,
		FLinearColor(0.82f, 0.64f, 1.0f, 0.42f),
		bShowOrbitSet && bShowStarRing,
		TEXT("Runtime.FXProxy.StarOrbit"));
}

void ASIPResourcePlantActor::ApplyRecipeMeshes(float Scale, const FSIPResourcePlantCollapseVector& Vector, const FSIPResourcePlantCollapseMetrics& Metrics)
{
	if (!PlantRecipe)
	{
		return;
	}

	const float GlobalAlpha = SeverityAlpha(Metrics);
	const FLinearColor BodyColor = AddGlow(FLinearColor(0.16f, 0.12f, 0.08f), FLinearColor(0.95f, 0.32f, 0.08f), LevelAlpha(Vector.BodyShell) * 0.35f + GlobalAlpha * 0.20f);
	const FLinearColor PrimaryColor = AddGlow(FLinearColor(0.45f, 0.18f, 0.10f), FLinearColor(1.0f, 0.48f, 0.12f), LevelAlpha(Vector.PrimarySilhouette) * 0.40f + GlobalAlpha * 0.20f);
	const FLinearColor CoreColor = AddGlow(FLinearColor(0.95f, 0.65f, 0.18f), FLinearColor(1.0f, 0.18f, 0.05f), LevelAlpha(Vector.Core) * 0.50f);
	const FLinearColor OrbitColor = AddGlow(FLinearColor(0.72f, 0.66f, 1.0f), FLinearColor(0.22f, 0.84f, 1.0f), LevelAlpha(Vector.OrbitSet) * 0.55f);

	const bool bAppliedBodyShell = ConfigureRecipeSlot(
		ESIPResourcePlantSlot::BodyShell,
		Vector.BodyShell,
		BodyShellRootSlot,
		BodyColor,
		TEXT("Runtime.BodyShell.Recipe"),
		Scale);

	if (bAppliedBodyShell)
	{
		HideSlot(BodyShellAxisSlot);
	}

	const bool bAppliedPrimarySilhouette = ConfigureRecipeSlot(
		ESIPResourcePlantSlot::PrimarySilhouette,
		Vector.PrimarySilhouette,
		PrimarySilhouetteSlot,
		PrimaryColor,
		TEXT("Runtime.PrimarySilhouette.Recipe"),
		Scale);

	const bool bAppliedCore = ConfigureRecipeSlot(
		ESIPResourcePlantSlot::Core,
		Vector.Core,
		CoreSlot,
		CoreColor,
		TEXT("Runtime.Core.Recipe"),
		Scale);

	const bool bAppliedOrbitSet = ConfigureRecipeSlot(
		ESIPResourcePlantSlot::OrbitSet,
		Vector.OrbitSet,
		OrbitNodeASlot,
		OrbitColor,
		TEXT("Runtime.OrbitSet.Recipe"),
		Scale);

	if (bAppliedOrbitSet)
	{
		HideSlot(OrbitNodeBSlot);
	}

	if (bAppliedBodyShell || bAppliedPrimarySilhouette || bAppliedCore || bAppliedOrbitSet)
	{
		HideSlot(OrbitProxyRingSlot);
	}
}

bool ASIPResourcePlantActor::ConfigureRecipeSlot(ESIPResourcePlantSlot Slot, int32 CollapseLevel, UStaticMeshComponent* Component, const FLinearColor& Color, FName FallbackRuntimeSlotTag, float Scale)
{
	if (!PlantRecipe || !Component)
	{
		return false;
	}

	const FSIPResourcePlantMeshSlotRecipe* SlotRecipe = PlantRecipe->FindSlotRecipe(Slot);
	if (!SlotRecipe)
	{
		return false;
	}

	const FSIPResourcePlantMeshCollapseVariant* CollapseVariant = SlotRecipe->FindCollapseVariant(CollapseLevel);
	const bool bResolvedVisible = CollapseVariant ? CollapseVariant->bVisible : SlotRecipe->bVisible;

	if (!bResolvedVisible)
	{
		HideSlot(Component);
		return true;
	}

	TSoftObjectPtr<UStaticMesh> MeshRef = SlotRecipe->Mesh;
	if (CollapseVariant && !CollapseVariant->Mesh.IsNull())
	{
		MeshRef = CollapseVariant->Mesh;
	}

	UStaticMesh* Mesh = MeshRef.LoadSynchronous();
	if (!Mesh)
	{
		HideSlot(Component);
		return true;
	}

	const bool bHasAssemblyHint = SIPResourcePlantAssembly::FindSlotHint(PlantRecipe->AssemblyHint, Slot) != nullptr;
	const FTransform AssemblyTransform = bHasAssemblyHint
		? SIPResourcePlantAssembly::ResolveSlotTransform(PlantRecipe->AssemblyHint, Slot)
		: FTransform::Identity;

	const FTransform VariantAdjustment = CollapseVariant ? CollapseVariant->LocalAdjustment : FTransform::Identity;
	const FQuat Rotation = AssemblyTransform.GetRotation() * SlotRecipe->LocalAdjustment.GetRotation() * VariantAdjustment.GetRotation();
	const FVector LocalLocation = (AssemblyTransform.GetLocation() + SlotRecipe->LocalAdjustment.GetLocation() + VariantAdjustment.GetLocation()) * Scale;
	const FVector LocalScale = AssemblyTransform.GetScale3D() * SlotRecipe->LocalAdjustment.GetScale3D() * VariantAdjustment.GetScale3D() * Scale;
	const FName SlotRuntimeTag = SlotRecipe->RuntimeSlotTag.IsNone() ? FallbackRuntimeSlotTag : SlotRecipe->RuntimeSlotTag;
	const FName RuntimeSlotTag = CollapseVariant && !CollapseVariant->RuntimeSlotTag.IsNone() ? CollapseVariant->RuntimeSlotTag : SlotRuntimeTag;

	ConfigureSlot(
		Component,
		Mesh,
		LocalLocation,
		Rotation.Rotator(),
		LocalScale,
		Color,
		true,
		RuntimeSlotTag,
		false);

	UMaterialInterface* MaterialOverride = SlotRecipe->OverrideMaterial;
	if (CollapseVariant && CollapseVariant->OverrideMaterial)
	{
		MaterialOverride = CollapseVariant->OverrideMaterial;
	}

	if (MaterialOverride)
	{
		Component->SetMaterial(0, MaterialOverride);
	}

	return true;
}

void ASIPResourcePlantActor::ConfigureSlot(UStaticMeshComponent* Slot, UStaticMesh* Mesh, const FVector& LocalLocation, const FRotator& LocalRotation, const FVector& LocalScale, const FLinearColor& Color, bool bVisible, FName RuntimeSlotTag, bool bApplyPreviewMaterial) const
{
	if (!Slot)
	{
		return;
	}

	Slot->SetStaticMesh(Mesh ? Mesh : PreviewCubeMesh.Get());
	Slot->SetRelativeLocation(LocalLocation);
	Slot->SetRelativeRotation(LocalRotation);
	Slot->SetRelativeScale3D(LocalScale);
	Slot->SetVisibility(bVisible, true);
	Slot->SetHiddenInGame(!bVisible);
	Slot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Slot->ComponentTags.Reset();
	Slot->ComponentTags.Add(FName(*Slot->GetName()));
	Slot->ComponentTags.Add(RuntimeSlotTag);

	if (bApplyPreviewMaterial)
	{
		ApplySlotMaterial(Slot, Color);
	}
	else
	{
		Slot->EmptyOverrideMaterials();
	}
}

void ASIPResourcePlantActor::ApplySlotMaterial(UStaticMeshComponent* Slot, const FLinearColor& Color) const
{
	if (!Slot || !PreviewBaseMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* DynamicMaterial = Slot->CreateAndSetMaterialInstanceDynamicFromMaterial(0, PreviewBaseMaterial);
	if (!DynamicMaterial)
	{
		return;
	}

	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
}

void ASIPResourcePlantActor::HideSlot(UStaticMeshComponent* Slot) const
{
	if (!Slot)
	{
		return;
	}

	Slot->SetVisibility(false, true);
	Slot->SetHiddenInGame(true);
	Slot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

float ASIPResourcePlantActor::LevelAlpha(int32 Level) const
{
	return SIPResourcePlantAssembly::LevelAlpha(Level);
}
