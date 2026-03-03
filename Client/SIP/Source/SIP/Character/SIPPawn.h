// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Logging/LogMacros.h"
#include "SIPPawn.generated.h"

/*
* Pawn基类，用于游戏内简单实体，例如可交互、道具等
*/
UCLASS(config=Game)
class ASIPPawn : public APawn
{
	GENERATED_BODY()

public:
	ASIPPawn(const FObjectInitializer& ObjectInitializer);

protected:

	// To add mapping context
	virtual void BeginPlay();

    // 在这里初始化组件
	virtual void PostInitializeComponents() override;
};

