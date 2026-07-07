// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SIPPetThoughtWidget.generated.h"

class UTextBlock;
class UCanvasPanelSlot;

UCLASS()
class SIP_API USIPPetThoughtWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Thought")
	void SetThoughtText(const FString& Thought);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildWidgetTree();
	int32 CountWords(const FString& Text) const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ThoughtText;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> ThoughtBorderSlot;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> ThoughtTailSlot;
};
