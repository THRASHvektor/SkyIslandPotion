// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/SIPResourcePlantRecipe.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "World/SIPResourcePlantAssembly.h"
#include "SIPResourcePlantActor.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;

/**
 * PCG-spawnable resource plant actor.
 *
 * PCG should spawn one actor per plant. The actor owns four semantic mesh slots:
 * BodyShell, PrimarySilhouette, Core, and OrbitSet. The current preview uses
 * basic meshes, but the same transforms can later be fed by Meshy/PBR imports.
 */
UCLASS(Blueprintable)
class SIP_API ASIPResourcePlantActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPResourcePlantActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "SIP|Resource Plant")
	void RebuildPlantPreview();

	UFUNCTION(BlueprintCallable, Category = "SIP|Resource Plant|Collapse")
	void RerollCollapseVector();

	UFUNCTION(BlueprintCallable, Category = "SIP|Resource Plant|PCG")
	FSIPResourcePlantCollapseVector ApplyPCGPropertyConfiguration();

	UFUNCTION(BlueprintCallable, Category = "SIP|Resource Plant|PCG")
	FSIPResourcePlantCollapseVector ConfigureFromPCG(
		USIPResourcePlantRecipe* InRecipe,
		ESIPResourcePlantPreviewFamily InFamily,
		ESIPResourcePlantCollapseBand InCollapseBand,
		int32 InSeed,
		float InScale,
		float InCollapseTemperatureScale = 1.0f,
		bool bInUseRecipeDefaultCollapseBand = false);

	UFUNCTION(BlueprintPure, Category = "SIP|Resource Plant|Collapse")
	FSIPResourcePlantCollapseVector GetActiveCollapseVector() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Resource Plant|Collapse")
	FSIPResourcePlantCollapseMetrics GetActiveCollapseMetrics() const;

	UFUNCTION(BlueprintCallable, Category = "SIP|Resource Plant|Recipe")
	void ValidateAssignedRecipe(const FSIPResourcePlantRecipeValidationOptions& Options, TArray<FSIPResourcePlantRecipeValidationIssue>& OutIssues) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Resource Plant|Recipe")
	bool IsAssignedRecipeReadyForAssembly(const FSIPResourcePlantRecipeValidationOptions& Options) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Components")
	TObjectPtr<UBoxComponent> InteractionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Slots")
	TObjectPtr<UStaticMeshComponent> BodyShellRootSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Slots")
	TObjectPtr<UStaticMeshComponent> BodyShellAxisSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Slots")
	TObjectPtr<UStaticMeshComponent> PrimarySilhouetteSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Slots")
	TObjectPtr<UStaticMeshComponent> CoreSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Slots")
	TObjectPtr<UStaticMeshComponent> OrbitNodeASlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Slots")
	TObjectPtr<UStaticMeshComponent> OrbitNodeBSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Slots")
	TObjectPtr<UStaticMeshComponent> OrbitProxyRingSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Legacy Slots")
	TObjectPtr<UStaticMeshComponent> RootSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Legacy Slots")
	TObjectPtr<UStaticMeshComponent> StemSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Legacy Slots")
	TObjectPtr<UStaticMeshComponent> PetalRewardSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Legacy Slots")
	TObjectPtr<UStaticMeshComponent> SporeRewardSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Legacy Slots")
	TObjectPtr<UStaticMeshComponent> CrystalRewardSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Legacy Slots")
	TObjectPtr<UStaticMeshComponent> ReactionShellSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Definition")
	FGameplayTag ResourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Definition")
	FGameplayTag ElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Definition")
	FGameplayTagContainer AffixTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Definition")
	int32 PlantSeed = 42;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|PCG")
	bool bConfigureFromPCGProperties = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Definition")
	ESIPResourcePlantPreviewFamily PreviewFamily = ESIPResourcePlantPreviewFamily::CrownLily;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	TObjectPtr<USIPResourcePlantRecipe> PlantRecipe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Collapse")
	ESIPResourcePlantCollapseBand CollapseBand = ESIPResourcePlantCollapseBand::Weathered_C1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Collapse")
	bool bUseRecipeDefaultCollapseBand = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Collapse")
	bool bUseExplicitCollapseVector = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Collapse", meta = (EditCondition = "bUseExplicitCollapseVector"))
	FSIPResourcePlantCollapseVector ExplicitCollapseVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Collapse", meta = (ClampMin = "0.05", ClampMax = "2.5"))
	float CollapseTemperatureScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Collapse")
	FSIPResourcePlantCollapseVector ResolvedCollapseVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Collapse")
	FSIPResourcePlantCollapseMetrics ResolvedCollapseMetrics;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview", meta = (ClampMin = "0.1"))
	float PlantScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview")
	bool bShowOrbitSet = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview")
	bool bShowCollapseProxyRing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview")
	TObjectPtr<UStaticMesh> PreviewCubeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview")
	TObjectPtr<UStaticMesh> PreviewSphereMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview")
	TObjectPtr<UStaticMesh> PreviewCylinderMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview")
	TObjectPtr<UStaticMesh> PreviewConeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Preview")
	TObjectPtr<UMaterialInterface> PreviewBaseMaterial;

private:
	FSIPResourcePlantCollapseVector ResolveCollapseVector() const;
	FSIPResourcePlantCollapseVector SampleCollapseVector(int32 Seed) const;
	FSIPResourcePlantCollapseMetrics CalculateCollapseMetrics(const FSIPResourcePlantCollapseVector& Vector) const;
	bool PassesHardConstraints(const FSIPResourcePlantCollapseVector& Vector) const;
	ESIPResourcePlantPreviewFamily GetEffectivePreviewFamily() const;
	ESIPResourcePlantCollapseBand GetEffectiveCollapseBand() const;

	void ConfigureCrownLilyPreview(float Scale, const FSIPResourcePlantCollapseVector& Vector, const FSIPResourcePlantCollapseMetrics& Metrics, float YawOffset);
	void ConfigureAetherVinePreview(float Scale, const FSIPResourcePlantCollapseVector& Vector, const FSIPResourcePlantCollapseMetrics& Metrics, float YawOffset);
	void ApplyRecipeMeshes(float Scale, const FSIPResourcePlantCollapseVector& Vector, const FSIPResourcePlantCollapseMetrics& Metrics);
	bool ConfigureRecipeSlot(ESIPResourcePlantSlot Slot, int32 CollapseLevel, UStaticMeshComponent* Component, const FLinearColor& Color, FName FallbackRuntimeSlotTag, float Scale);
	void ConfigureSlot(UStaticMeshComponent* Slot, UStaticMesh* Mesh, const FVector& LocalLocation, const FRotator& LocalRotation, const FVector& LocalScale, const FLinearColor& Color, bool bVisible, FName RuntimeSlotTag, bool bApplyPreviewMaterial = true) const;
	void ApplySlotMaterial(UStaticMeshComponent* Slot, const FLinearColor& Color) const;
	void HideSlot(UStaticMeshComponent* Slot) const;

	float LevelAlpha(int32 Level) const;
};
