// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "World/SIPResourcePlantAssembly.h"
#include "SIPResourcePlantRecipe.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FSIPResourcePlantMeshCollapseVariant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe", meta = (ClampMin = "0", ClampMax = "3"))
	int32 CollapseLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	TObjectPtr<UMaterialInterface> OverrideMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	FTransform LocalAdjustment = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	FName RuntimeSlotTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	bool bVisible = true;
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantMeshSlotRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	ESIPResourcePlantSlot Slot = ESIPResourcePlantSlot::BodyShell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	TObjectPtr<UMaterialInterface> OverrideMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	FTransform LocalAdjustment = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	FName RuntimeSlotTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	bool bVisible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	TArray<FSIPResourcePlantMeshCollapseVariant> CollapseVariants;

	const FSIPResourcePlantMeshCollapseVariant* FindCollapseVariant(int32 CollapseLevel) const;
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantFXSlotRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	ESIPResourcePlantSlot AnchorSlot = ESIPResourcePlantSlot::Core;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	FTransform LocalAdjustment = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Recipe")
	FGameplayTagContainer TriggerTags;
};

UENUM(BlueprintType)
enum class ESIPResourcePlantRecipeIssueSeverity : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Error UMETA(DisplayName = "Error")
};

UENUM(BlueprintType)
enum class ESIPResourcePlantRecipeIssueCode : uint8
{
	None UMETA(DisplayName = "None"),
	MissingRecipe UMETA(DisplayName = "Missing Recipe"),
	MissingSlot UMETA(DisplayName = "Missing Slot"),
	MissingMesh UMETA(DisplayName = "Missing Mesh"),
	MissingCollapseVariant UMETA(DisplayName = "Missing Collapse Variant"),
	MissingAssemblyHint UMETA(DisplayName = "Missing Assembly Hint"),
	InvalidAssemblyScale UMETA(DisplayName = "Invalid Assembly Scale"),
	DuplicateSlot UMETA(DisplayName = "Duplicate Slot"),
	DuplicateCollapseVariant UMETA(DisplayName = "Duplicate Collapse Variant")
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantRecipeValidationOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Validation")
	bool bRequireAllSemanticSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Validation")
	bool bRequireAssemblyHints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Validation")
	bool bRequireCollapseVariants = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Validation")
	bool bRequireMeshes = true;
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantRecipeValidationIssue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Validation")
	ESIPResourcePlantRecipeIssueSeverity Severity = ESIPResourcePlantRecipeIssueSeverity::Error;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Validation")
	ESIPResourcePlantRecipeIssueCode Code = ESIPResourcePlantRecipeIssueCode::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Validation")
	ESIPResourcePlantSlot Slot = ESIPResourcePlantSlot::BodyShell;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Validation")
	int32 CollapseLevel = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Validation")
	FString Message;
};

UCLASS(BlueprintType)
class SIP_API USIPResourcePlantRecipe : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	FName RecipeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	ESIPResourcePlantPreviewFamily Family = ESIPResourcePlantPreviewFamily::CrownLily;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	ESIPResourcePlantCollapseBand DefaultCollapseBand = ESIPResourcePlantCollapseBand::Weathered_C1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	FGameplayTag ElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	FGameplayTagContainer GameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	FSIPResourcePlantAssemblyHint AssemblyHint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	TArray<FSIPResourcePlantMeshSlotRecipe> MeshSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant|Recipe")
	TArray<FSIPResourcePlantFXSlotRecipe> FXSlots;

	const FSIPResourcePlantMeshSlotRecipe* FindSlotRecipe(ESIPResourcePlantSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "SIP|Resource Plant|Validation")
	void ValidateRecipe(const FSIPResourcePlantRecipeValidationOptions& Options, TArray<FSIPResourcePlantRecipeValidationIssue>& OutIssues) const;

	UFUNCTION(BlueprintCallable, Category = "SIP|Resource Plant|Validation")
	bool IsReadyForAssembly(const FSIPResourcePlantRecipeValidationOptions& Options) const;

	UFUNCTION(BlueprintPure, Category = "SIP|Resource Plant|Validation")
	static int32 CountIssues(const TArray<FSIPResourcePlantRecipeValidationIssue>& Issues, ESIPResourcePlantRecipeIssueCode Code);
};
