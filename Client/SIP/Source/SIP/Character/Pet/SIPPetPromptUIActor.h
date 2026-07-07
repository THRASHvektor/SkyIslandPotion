// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Pet/Components/SIPPetMoodComponent.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "SIPPetPromptUIActor.generated.h"

class ASIPPetElementalFieldActor;
class UMaterialInterface;
class UMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMesh;
class USIPPetPersonalityJsonComponent;
class USIPPetPersonalityComponent;
class USIPPetPromptSpawnComponent;
class USIPPetThoughtWidget;
class USIPPetPromptWidget;
enum class ESIPPetElementType : uint8;

UCLASS(Blueprintable)
class SIP_API ASIPPetPromptUIActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPPetPromptUIActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bUseProjectPetPromptSettings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	TSubclassOf<AActor> PetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|Species")
	TSubclassOf<AActor> CatPetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|Species")
	TSubclassOf<AActor> DogPetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|Species")
	TSubclassOf<AActor> DragonPetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bUseQwenApi = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bShowResultJsonInWidget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bAddSimpleFollowComponent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	float SpawnDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	float SpawnSideOffset = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	TSubclassOf<USIPPetPromptWidget> PromptWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|UI")
	bool bShowPromptWidgetOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|UI")
	FKey PromptToggleKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood|UI")
	TSubclassOf<USIPPetThoughtWidget> ThoughtWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood|UI", meta = (ClampMin = "0.5"))
	float ThoughtWidgetDuration = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	bool bEnableWindFieldInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	FKey WindFieldKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	TSubclassOf<ASIPPetElementalFieldActor> WindFieldActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	TObjectPtr<UNiagaraSystem> WindFieldVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	TObjectPtr<UNiagaraSystem> WaterFieldVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Thunder", meta = (DisplayName = "Thunder Strike BP"))
	TSubclassOf<AActor> ThunderStrikeActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	TObjectPtr<UNiagaraSystem> PlantFieldVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	TObjectPtr<UNiagaraSystem> ShadowCloakVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Shadow")
	bool bEnableShadowCloakVFX = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Shadow")
	FVector ShadowCloakVFXOffset = FVector(0.0f, 0.0f, -45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Shadow", meta = (ClampMin = "0.05", ClampMax = "3.0"))
	float ShadowCloakVFXScale = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Shadow")
	bool bHidePlayerMeshDuringShadowCloak = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Rock Bridge")
	TObjectPtr<UStaticMesh> BridgeStepMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Rock Bridge")
	TObjectPtr<UMaterialInterface> BridgeStepMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Legacy", meta = (ClampMin = "50.0"))
	float WindFieldSpawnDistance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	FVector WindFieldSpawnOffset = FVector(0.0f, 0.0f, -88.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill", meta = (ClampMin = "50.0"))
	float WindFieldRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill", meta = (ClampMin = "0.1"))
	float WindFieldDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	FVector WindFieldLaunchVelocity = FVector(0.0f, 0.0f, 950.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Thunder", meta = (ClampMin = "100.0"))
	float ThunderStrikeDistance = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Thunder", meta = (ClampMin = "50.0"))
	float ThunderStrikeRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Thunder", meta = (ClampMin = "0.0"))
	float ThunderStrikeDamage = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill|Thunder", meta = (ClampMin = "0.1"))
	float ThunderStrikeDuration = 1.35f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Prompt")
	TObjectPtr<USIPPetPromptSpawnComponent> RuntimePromptSpawnComponent;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Skill")
	void ActivateWindField();

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Prompt")
	void TogglePromptWidget();

private:
	void TryInitializePromptUI();
	void BindPromptInput(APlayerController* PlayerController);
	void BindSkillInput(APlayerController* PlayerController);
	void BindPromptSpawnEvents();
	void BindActivePetMood(AActor* PetActor);
	void HideThoughtWidget();
	void ActivateThunderStrike(UWorld* World, APawn* PlayerPawn);
	FVector ResolveThunderStrikeLocation(UWorld* World, APawn* PlayerPawn) const;
	void ApplyThunderStrikeDamage(UWorld* World, APawn* PlayerPawn, const FVector& StrikeLocation) const;
	void ApplyShadowCloak(APawn* PlayerPawn, float Duration);
	void ClearShadowCloak();
	UNiagaraSystem* ResolveFieldVFXForElement(ESIPPetElementType ElementType) const;
	USIPPetPromptSpawnComponent* FindOrCreatePromptComponent(AActor* TargetActor);
	void ConfigureSpeciesClassRules(USIPPetPromptSpawnComponent* SpawnComponent) const;
	USIPPetPersonalityComponent* ResolveActivePetPersonality() const;
	USIPPetPersonalityJsonComponent* ResolveActivePetJsonPersonality() const;
	AActor* ResolveActivePetActor() const;

	UFUNCTION()
	void HandlePromptPetSpawned(AActor* SpawnedPet, USIPPetPersonalityJsonComponent* PersonalityComponent);

	UFUNCTION()
	void HandleMoodThoughtGenerated(ESIPPetMoodType Mood, const FString& Thought);

	FTimerHandle RetryTimerHandle;
	FTimerHandle ShadowCloakTimerHandle;
	FTimerHandle ThoughtWidgetTimerHandle;
	TWeakObjectPtr<APawn> ShadowCloakedPawn;
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ShadowCloakVFXComponent;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> HiddenShadowCloakMeshes;
	UPROPERTY(Transient)
	TObjectPtr<USIPPetMoodComponent> ActiveMoodComponent;
	UPROPERTY(Transient)
	TObjectPtr<USIPPetThoughtWidget> ActiveThoughtWidget;
	int32 RetryCount = 0;
};
