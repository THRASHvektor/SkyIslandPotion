// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "SIPPlayerController.generated.h"

/**
 * 项目自定义的玩家控制器。
 * 主要职责是在 Enhanced Input 收集完本帧输入后，
 * 把整理好的能力输入继续转交给自定义 ASC 处理。
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class SIP_API ASIPPlayerController : public APlayerController
{
    GENERATED_BODY()
    
    // 控制器构造函数，当前逻辑很轻，主要行为集中在 PostProcessInput。
    ASIPPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
    // 给 Pawn 的 ASC 一个统一的逐帧入口，用来消费缓存的能力输入。
    virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
};
