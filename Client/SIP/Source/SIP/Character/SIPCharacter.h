// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SIPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

// TODO： 建立相关的Component框架
/*
* 角色基类，考虑后续增加Pawn基类
*/
UCLASS(config=Game)
class ASIPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASIPCharacter();
	

protected:


protected:

	// To add mapping context
	virtual void BeginPlay();
};

