// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "SIPPlayerController.generated.h"

UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class SIP_API ASIPPlayerController : public APlayerController
{
    GENERATED_BODY()
    
    ASIPPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
    virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
};