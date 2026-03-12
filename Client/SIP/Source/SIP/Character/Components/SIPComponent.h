// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Logging/LogMacros.h"
#include "SIPComponent.generated.h"

/*
* 组件基类，用于所有角色的可扩展功能组件
*/
UCLASS(config=Game)
class USIPComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPComponent(const FObjectInitializer& ObjectInitializer);
	
protected:

	// To add mapping context
	virtual void BeginPlay() override;
};

