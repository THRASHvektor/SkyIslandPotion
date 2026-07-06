// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SIPPetPromptSpawnComponent.generated.h"

class USIPPetCliffBridgeComponent;
class USIPPetFollowComponent;
class USIPPetPersonalityJsonComponent;
class USIPPetPromptWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSIPPromptPetSpawnedSignature, AActor*, SpawnedPet, USIPPetPersonalityJsonComponent*, PersonalityComponent);

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPPetPromptSpawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPPetPromptSpawnComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Prompt")
	AActor* SpawnPetFromPrompt(const FString& Prompt);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Prompt")
	AActor* SpawnPetFromPromptNearActor(const FString& Prompt, AActor* SpawnNearActor);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Prompt")
	USIPPetPromptWidget* ShowPromptWidget(APlayerController* OwningPlayer = nullptr);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Prompt")
	void HidePromptWidget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	TSubclassOf<AActor> PetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	float SpawnDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	float SpawnSideOffset = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	float SpawnZOffset = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bUseQwenApi = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bDestroyPreviousPromptPet = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bWaitForPersonalityBeforeSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bSpawnDefaultControllerForPet = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt")
	bool bAddSimpleFollowComponent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|UI")
	bool bAutoShowPromptWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|UI")
	bool bShowResultJsonInWidget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Prompt|UI")
	TSubclassOf<USIPPetPromptWidget> PromptWidgetClass;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Prompt")
	TObjectPtr<AActor> LastSpawnedPet;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Prompt|UI")
	TObjectPtr<USIPPetPromptWidget> ActivePromptWidget;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Prompt")
	FSIPPromptPetSpawnedSignature OnPromptPetSpawned;

private:
	UFUNCTION()
	void HandlePendingPersonalityGenerated(bool bSuccess, const FString& JsonString);

	FVector ResolveSpawnLocation(AActor* SpawnNearActor) const;
	AActor* SpawnFinalPet(AActor* SpawnNearActor, const FString& Prompt, const FString& PersonalityJson, bool bPersonalityReady);
	void EnsurePetComponents(AActor* PetActor, AActor* FollowTarget, USIPPetPersonalityJsonComponent*& OutPersonalityComponent, USIPPetCliffBridgeComponent*& OutBridgeComponent) const;
	void SetBlueprintComponentReference(AActor* PetActor, USIPPetPersonalityJsonComponent* PersonalityComponent) const;
	void SetBlueprintFollowTargetReferences(AActor* PetActor, AActor* FollowTarget) const;

	UPROPERTY(Transient)
	TObjectPtr<USIPPetPersonalityJsonComponent> PendingPersonalityComponent;

	TWeakObjectPtr<AActor> PendingSpawnNearActor;
	FString PendingPrompt;
};
