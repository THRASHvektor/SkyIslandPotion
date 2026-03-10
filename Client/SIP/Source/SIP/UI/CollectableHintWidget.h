// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CollectableHintWidget.generated.h"

UCLASS()
class SIP_API UCollectableHintWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category = "CollectableHint")
    void SetHintText(const FText& Text);

protected:

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* HintText;

};