// Copyright Epic Games, Inc. All Rights Reserved.

#include "World/SIPFireSemanticVFXTestbedActor.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName TestbedTag(TEXT("SIP.SemanticVFX.FireTestbed"));
	const FName LavaChannelLeakTag(TEXT("FIRE_SLOT_01_LAVA_CHANNEL_LEAK"));
	const FName CraterRimPulseTag(TEXT("FIRE_SLOT_02_CRATER_RIM_PULSE"));
	const FName CliffCollapseTag(TEXT("FIRE_SLOT_05_CLIFF_COLLAPSE_DISTORTION"));
	const FName ReactionImpactTag(TEXT("FIRE_SLOT_06_REACTION_IMPACT_BURST"));

	UNiagaraSystem* FindNiagaraSystem(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<UNiagaraSystem> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}
}

ASIPFireSemanticVFXTestbedActor::ASIPFireSemanticVFXTestbedActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	Tags.Add(TestbedTag);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	LavaChannelLeakFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX_LavaChannelLeak"));
	LavaChannelLeakFX->SetupAttachment(SceneRoot);

	CraterRimPulseFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX_CraterRimPulse"));
	CraterRimPulseFX->SetupAttachment(SceneRoot);

	CliffCollapseDistortionFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX_CliffCollapseDistortion"));
	CliffCollapseDistortionFX->SetupAttachment(SceneRoot);

	ReactionImpactBurstPreviewFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VFX_ReactionImpactBurstPreview"));
	ReactionImpactBurstPreviewFX->SetupAttachment(SceneRoot);

	LavaChannelLeakSystem = FindNiagaraSystem(TEXT("/Game/SlashFX_SoftTofu/Niagara/Fire/NS_BottomSpark_Fire_.NS_BottomSpark_Fire_"));
	CraterRimPulseSystem = FindNiagaraSystem(TEXT("/Game/SlashFX_SoftTofu/Niagara/Fire/NS_Slash_Projectile_rotate_Fire_.NS_Slash_Projectile_rotate_Fire_"));
	CliffCollapseDistortionSystem = FindNiagaraSystem(TEXT("/Game/SlashFX_SoftTofu/Niagara/Distortion/NS_Slash_Distort_.NS_Slash_Distort_"));
	ReactionImpactBurstSystem = FindNiagaraSystem(TEXT("/Game/SlashFX_SoftTofu/Niagara/Fire/NS_Hit_Slash_Direction_Fire_.NS_Hit_Slash_Direction_Fire_"));
}

void ASIPFireSemanticVFXTestbedActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildSlotLayout();
}

void ASIPFireSemanticVFXTestbedActor::BeginPlay()
{
	Super::BeginPlay();
	RebuildSlotLayout();
}

void ASIPFireSemanticVFXTestbedActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRetriggerOneShotPreviewInEditor)
	{
		return;
	}

	LavaLeakTimerSeconds += DeltaSeconds;
	CraterPulseTimerSeconds += DeltaSeconds;
	CliffCollapseTimerSeconds += DeltaSeconds;
	ReactionBurstTimerSeconds += DeltaSeconds;

	if (bPreviewLavaChannelLeak && LavaLeakTimerSeconds >= FMath::Max(0.2f, LavaLeakIntervalSeconds))
	{
		LavaLeakTimerSeconds = 0.f;
		RetriggerOneShot(LavaChannelLeakFX);
	}

	if (bPreviewCraterRimPulse && CraterPulseTimerSeconds >= FMath::Max(0.2f, CraterPulseIntervalSeconds))
	{
		CraterPulseTimerSeconds = 0.f;
		RetriggerOneShot(CraterRimPulseFX);
	}

	if (bPreviewCliffCollapse && CliffCollapseTimerSeconds >= FMath::Max(0.2f, CliffCollapseIntervalSeconds))
	{
		CliffCollapseTimerSeconds = 0.f;
		RetriggerOneShot(CliffCollapseDistortionFX);
	}

	if (bPreviewReactionBurst && ReactionBurstTimerSeconds >= FMath::Max(0.2f, ReactionBurstIntervalSeconds))
	{
		ReactionBurstTimerSeconds = 0.f;
		RetriggerOneShot(ReactionImpactBurstPreviewFX);
	}
}

bool ASIPFireSemanticVFXTestbedActor::ShouldTickIfViewportsOnly() const
{
	return bRetriggerOneShotPreviewInEditor;
}

void ASIPFireSemanticVFXTestbedActor::RebuildSlotLayout()
{
	const float Radius = FMath::Max(100.f, IslandRadiusCm);
	const float HalfHeight = FMath::Max(100.f, IslandHalfHeightCm);
	const float Lift = SurfaceLiftCm;
	const float ScaleMultiplier = FMath::Clamp(PreviewScaleMultiplier, 0.1f, 8.0f);
	const float LavaChannelZ = Lift + HalfHeight * 0.78f;
	const float CraterRimZ = Lift + HalfHeight * 0.92f;
	const float CliffAnomalyZ = Lift + HalfHeight * 0.34f;
	const float ReactionBurstZ = Lift + HalfHeight * 0.86f;

	ConfigureSlot(
		LavaChannelLeakFX,
		LavaChannelLeakSystem,
		FVector(Radius * 0.21f, -Radius * 0.05f, LavaChannelZ),
		FRotator(0.f, 12.f, 0.f),
		FVector(2.8f, 2.8f, 2.8f) * ScaleMultiplier,
		bPreviewLavaChannelLeak,
		LavaChannelLeakTag);

	ConfigureSlot(
		CraterRimPulseFX,
		CraterRimPulseSystem,
		FVector(Radius * 0.42f, Radius * 0.16f, CraterRimZ),
		FRotator(0.f, 58.f, 0.f),
		FVector(2.3f, 2.3f, 2.3f) * ScaleMultiplier,
		bPreviewCraterRimPulse,
		CraterRimPulseTag);

	ConfigureSlot(
		CliffCollapseDistortionFX,
		CliffCollapseDistortionSystem,
		FVector(-Radius * 0.78f, Radius * 0.23f, CliffAnomalyZ),
		FRotator(0.f, -34.f, 0.f),
		FVector(2.2f, 2.2f, 2.2f) * ScaleMultiplier,
		bPreviewCliffCollapse,
		CliffCollapseTag);

	ConfigureSlot(
		ReactionImpactBurstPreviewFX,
		ReactionImpactBurstSystem,
		FVector(Radius * 0.16f, Radius * 0.35f, ReactionBurstZ),
		FRotator(0.f, 112.f, 0.f),
		FVector(2.0f, 2.0f, 2.0f) * ScaleMultiplier,
		bPreviewReactionBurst,
		ReactionImpactTag);
}

void ASIPFireSemanticVFXTestbedActor::ConfigureSlot(
	UNiagaraComponent* Component,
	UNiagaraSystem* System,
	const FVector& LocalLocation,
	const FRotator& LocalRotation,
	const FVector& LocalScale,
	bool bActive,
	FName SlotTag) const
{
	if (!Component)
	{
		return;
	}

	if (System)
	{
		Component->SetAsset(System);
	}

	Component->SetRelativeLocation(LocalLocation);
	Component->SetRelativeRotation(LocalRotation);
	Component->SetRelativeScale3D(LocalScale);
	Component->SetAutoActivate(bActive);
	if (bActive)
	{
		ApplyReadablePreviewParameters(Component, SlotTag);
		Component->ResetSystem();
		Component->Activate(true);
	}
	else
	{
		Component->Deactivate();
	}
	Component->ComponentTags.Reset();
	Component->ComponentTags.Add(TestbedTag);
	Component->ComponentTags.Add(SlotTag);
}

void ASIPFireSemanticVFXTestbedActor::ApplyReadablePreviewParameters(UNiagaraComponent* Component, FName SlotTag) const
{
	if (!Component || !bApplyReadablePreviewParameters)
	{
		return;
	}

	const float Intensity = FMath::Clamp(PreviewIntensity, 0.05f, 4.0f);
	const auto Count = [Intensity](float Value)
	{
		return FMath::RoundToInt(FMath::Max(1.0f, Value * Intensity));
	};

	Component->SetVariableLinearColor(TEXT("User.Fire_Color"), FLinearColor(1.0f, 0.23f, 0.02f, 1.0f));
	Component->SetVariableLinearColor(TEXT("User.Spark_Color"), FLinearColor(1.0f, 0.58f, 0.05f, 1.0f));
	Component->SetVariableLinearColor(TEXT("User.Color_"), FLinearColor(1.0f, 0.34f, 0.06f, 1.0f));
	Component->SetVariableFloat(TEXT("User.Alpha"), 1.35f * Intensity);
	Component->SetVariableFloat(TEXT("User.Emissive"), 18.0f * Intensity);
	Component->SetVariableFloat(TEXT("User.Brightness_Hit"), 24.0f * Intensity);

	if (SlotTag == LavaChannelLeakTag)
	{
		Component->SetVariableInt(TEXT("User.Spawn Count_Spark"), Count(42.0f));
		Component->SetVariableVec2(TEXT("User.Sprite Size Spark"), FVector2D(82.0f, 82.0f) * Intensity);
		Component->SetVariableFloat(TEXT("User.Velocity Speed_Spark"), 760.0f * Intensity);
		return;
	}

	if (SlotTag == CraterRimPulseTag)
	{
		Component->SetVariableFloat(TEXT("User.Slash_Size"), 520.0f * Intensity);
		Component->SetVariableFloat(TEXT("User.Ring Radius"), 340.0f * Intensity);
		Component->SetVariableFloat(TEXT("User.SpawnRate"), 180.0f * Intensity);
		Component->SetVariableInt(TEXT("User.Spawn Count_Fire"), Count(28.0f));
		Component->SetVariableVec2(TEXT("User.Size_Fire"), FVector2D(150.0f, 150.0f) * Intensity);
		return;
	}

	if (SlotTag == CliffCollapseTag)
	{
		Component->SetVariableFloat(TEXT("User.Size_Distort"), 220.0f * Intensity);
		Component->SetVariableFloat(TEXT("User.Size_Shockwave"), 360.0f * Intensity);
		Component->SetVariableFloat(TEXT("User.Alpha"), 0.85f * Intensity);
		return;
	}

	if (SlotTag == ReactionImpactTag)
	{
		Component->SetVariableFloat(TEXT("User.Brightness_Hit"), 36.0f * Intensity);
		Component->SetVariableInt(TEXT("User.Spawn Count_Fire"), Count(44.0f));
		Component->SetVariableInt(TEXT("User.Spawn Count_Spark"), Count(72.0f));
		Component->SetVariableVec2(TEXT("User.Size_Fire"), FVector2D(210.0f, 210.0f) * Intensity);
		Component->SetVariableVec2(TEXT("User.Sprite Size Spark"), FVector2D(130.0f, 130.0f) * Intensity);
		Component->SetVariableFloat(TEXT("User.Velocity Speed_Fire"), 900.0f * Intensity);
	}
}

void ASIPFireSemanticVFXTestbedActor::RetriggerOneShot(UNiagaraComponent* Component) const
{
	if (!Component || !Component->GetAsset())
	{
		return;
	}

	FName SlotTag;
	for (const FName& ComponentTag : Component->ComponentTags)
	{
		if (ComponentTag != TestbedTag)
		{
			SlotTag = ComponentTag;
			break;
		}
	}

	ApplyReadablePreviewParameters(Component, SlotTag);
	Component->DeactivateImmediate();
	Component->Activate(true);
}
