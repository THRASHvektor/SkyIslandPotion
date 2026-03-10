// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 */

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class UAbilitySystemComponent;

/*
* 可交互接口，供场景中可交互对象实现
* 例如：可采集的药草、可对话的 NPC、可打开的宝箱等
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class SIP_API IInteractable
{
    GENERATED_BODY()

public:
    // 获取交互提示文字（例如："采集药草、对话"）
    virtual FText GetInteractText() const = 0;

    /**
     * 如果后期需要由蓝图决定是否执行cpp，可将其合并成一个BlueprintNativeEvent
     */
    // 进行交互，供可交互对象实现交互结果，此函数提供蓝图实现接口，可用来实现一些游戏逻辑
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Interaction")
    void OnInteract_BP(UAbilitySystemComponent* InteractorASC);

    // 进行交互，供可交互对象实现交互结果，一般由character的交互组件调用，一些流程性代码必须在cpp中实现
    virtual void Interact(UAbilitySystemComponent* InteractorASC) { Execute_OnInteract_BP(_getUObject(), InteractorASC); };


    // 当玩家看向/靠近物体，且该物体被选为“最佳目标”时调用
    virtual void OnBeginFocus() = 0;


    // 当玩家视线移开或离开范围时调用
    virtual void OnEndFocus() = 0;
};
