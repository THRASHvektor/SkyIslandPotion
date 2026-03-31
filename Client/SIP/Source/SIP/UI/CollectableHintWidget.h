// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CollectableHintWidget.generated.h"

/**
 * 可收集物聚焦时使用的轻量提示控件封装。
 * 物品侧只需要提供文本内容，具体表现交给 Widget 蓝图处理。
 */
UCLASS()
class SIP_API UCollectableHintWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 更新当前聚焦物品的提示文本，不直接暴露内部绑定的 TextBlock。
    UFUNCTION(BlueprintCallable, Category = "CollectableHint")
    void SetHintText(const FText& Text);

protected:
    // 从 Widget 蓝图绑定过来的文本控件，用于显示交互提示。
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* HintText;
};
