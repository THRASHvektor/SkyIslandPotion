// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SIPCharacter.generated.h"

// TODO： 建立相关的Component框架
/*
* 角色基类，用于所有会有AI、移动等行为的Entity，包含宠物、坐骑等
*/
UCLASS(config=Game)
class ASIPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASIPCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	

protected:


protected:

	// To add mapping context
	virtual void BeginPlay();

	// 在这里创建角色需要的组件
	virtual void CreateComponent();

	// 在这里初始化组件
	virtual void PostInitializeComponents() override;
};

