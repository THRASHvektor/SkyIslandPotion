// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "SIPPetPromptUIActor.generated.h"

class ASIPPetElementalFieldActor;
class UNiagaraSystem;
class USIPPetPromptSpawnComponent;
class USIPPetPromptWidget;

UCLASS(Blueprintable)
class SIP_API ASIPPetPromptUIActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPPetPromptUIActor();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	TSubclassOf<AActor> PetClass;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	bool bEnableWindFieldInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	FKey WindFieldKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	TSubclassOf<ASIPPetElementalFieldActor> WindFieldActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Skill")
	TObjectPtr<UNiagaraSystem> WindFieldVFX;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Prompt")
	TObjectPtr<USIPPetPromptSpawnComponent> RuntimePromptSpawnComponent;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Skill")
	void ActivateWindField();

private:
	void TryInitializePromptUI();
	void BindSkillInput(APlayerController* PlayerController);
	USIPPetPromptSpawnComponent* FindOrCreatePromptComponent(AActor* TargetActor);

	FTimerHandle RetryTimerHandle;
	int32 RetryCount = 0;
};
