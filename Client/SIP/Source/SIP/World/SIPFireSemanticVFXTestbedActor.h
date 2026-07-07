// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPFireSemanticVFXTestbedActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

UCLASS(Blueprintable)
class SIP_API ASIPFireSemanticVFXTestbedActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPFireSemanticVFXTestbedActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

public:
	UFUNCTION(BlueprintCallable, Category = "SIP|Semantic VFX")
	void RebuildSlotLayout();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Semantic VFX|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Semantic VFX|Slots")
	TObjectPtr<UNiagaraComponent> LavaChannelLeakFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Semantic VFX|Slots")
	TObjectPtr<UNiagaraComponent> CraterRimPulseFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Semantic VFX|Slots")
	TObjectPtr<UNiagaraComponent> CliffCollapseDistortionFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Semantic VFX|Slots")
	TObjectPtr<UNiagaraComponent> ReactionImpactBurstPreviewFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Island", meta = (ClampMin = "100.0"))
	float IslandRadiusCm = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Island", meta = (ClampMin = "100.0"))
	float IslandHalfHeightCm = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Island")
	float SurfaceLiftCm = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Island")
	FName SourceActorLabel = TEXT("model");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview")
	bool bPreviewLavaChannelLeak = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview")
	bool bPreviewCraterRimPulse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview")
	bool bPreviewReactionBurst = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview")
	bool bPreviewCliffCollapse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview")
	bool bRetriggerOneShotPreviewInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview")
	bool bApplyReadablePreviewParameters = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview", meta = (ClampMin = "0.05", ClampMax = "4.0"))
	float PreviewIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview", meta = (ClampMin = "0.1", ClampMax = "8.0"))
	float PreviewScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview", meta = (ClampMin = "0.2"))
	float LavaLeakIntervalSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview", meta = (ClampMin = "0.2"))
	float CraterPulseIntervalSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview", meta = (ClampMin = "0.2"))
	float CliffCollapseIntervalSeconds = 5.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Preview", meta = (ClampMin = "0.2"))
	float ReactionBurstIntervalSeconds = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Assets")
	TObjectPtr<UNiagaraSystem> LavaChannelLeakSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Assets")
	TObjectPtr<UNiagaraSystem> CraterRimPulseSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Assets")
	TObjectPtr<UNiagaraSystem> CliffCollapseDistortionSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Semantic VFX|Assets")
	TObjectPtr<UNiagaraSystem> ReactionImpactBurstSystem;

private:
	void ConfigureSlot(UNiagaraComponent* Component, UNiagaraSystem* System, const FVector& LocalLocation, const FRotator& LocalRotation, const FVector& LocalScale, bool bActive, FName SlotTag) const;
	void ApplyReadablePreviewParameters(UNiagaraComponent* Component, FName SlotTag) const;
	void RetriggerOneShot(UNiagaraComponent* Component) const;

	float LavaLeakTimerSeconds = 0.f;
	float CraterPulseTimerSeconds = 0.f;
	float CliffCollapseTimerSeconds = 0.f;
	float ReactionBurstTimerSeconds = 0.f;
};
