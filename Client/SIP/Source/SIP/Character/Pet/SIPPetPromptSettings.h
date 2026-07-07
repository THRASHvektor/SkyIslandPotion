// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "SIPPetPromptSettings.generated.h"

class ASIPPetElementalFieldActor;
class ASIPPetPromptUIActor;
class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;
class USIPPetPromptWidget;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "SIP Pet Prompt"))
class SIP_API USIPPetPromptSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Prompt")
	TSoftClassPtr<AActor> DefaultPetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Species")
	TSoftClassPtr<AActor> CatPetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Species")
	TSoftClassPtr<AActor> DogPetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Species")
	TSoftClassPtr<AActor> DragonPetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Prompt")
	bool bUseQwenApi = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Prompt")
	bool bShowResultJsonInWidget = false;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Prompt")
	bool bAddSimpleFollowComponent = false;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Prompt", meta = (ClampMin = "0.0"))
	float SpawnDistance = 180.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Prompt")
	float SpawnSideOffset = 90.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<USIPPetPromptWidget> PromptWidgetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	bool bShowPromptWidgetOnBeginPlay = false;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	FKey PromptToggleKey = EKeys::Tab;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	bool bAutoCreatePromptUIActor = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<ASIPPetPromptUIActor> PromptUIActorClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftClassPtr<ASIPPetElementalFieldActor> WindFieldActorClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<UNiagaraSystem> WindFieldVFX;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<UNiagaraSystem> WaterFieldVFX;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Thunder", meta = (DisplayName = "Thunder Strike BP"))
	TSoftClassPtr<AActor> ThunderStrikeActorClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<UNiagaraSystem> PlantFieldVFX;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<UNiagaraSystem> ShadowCloakVFX;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Skill")
	bool bEnableShadowCloakVFX = true;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rock Bridge")
	TSoftObjectPtr<UStaticMesh> BridgeStepMesh;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rock Bridge")
	TSoftObjectPtr<UMaterialInterface> BridgeStepMaterial;
};
