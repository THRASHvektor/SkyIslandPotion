// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SIPPetPromptWidget.generated.h"

class UButton;
class UMultiLineEditableTextBox;
class UTextBlock;
class USIPPetPersonalityJsonComponent;
class USIPPetPromptSpawnComponent;

UCLASS()
class SIP_API USIPPetPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializePromptWidget(USIPPetPromptSpawnComponent* InPromptSpawnComponent);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Prompt")
	void SetStatusText(const FString& InStatus);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Prompt")
	void SetResultJson(bool bSuccess, const FString& JsonString);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void BindPromptSpawnComponent();

	UFUNCTION()
	void HandleGenerateClicked();

	UFUNCTION()
	void HandleClearClicked();

	UFUNCTION()
	void HandlePromptPetSpawned(AActor* SpawnedPet, USIPPetPersonalityJsonComponent* PersonalityComponent);

	UFUNCTION()
	void HandlePersonalityJsonGenerated(bool bSuccess, const FString& JsonString);

	UPROPERTY(Transient)
	TObjectPtr<USIPPetPromptSpawnComponent> PromptSpawnComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMultiLineEditableTextBox> PromptInput;

	UPROPERTY(Transient)
	TObjectPtr<UButton> GenerateButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ClearButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UMultiLineEditableTextBox> ResultJsonBox;

	UPROPERTY(Transient)
	TObjectPtr<USIPPetPersonalityJsonComponent> ActivePersonalityComponent;
};
