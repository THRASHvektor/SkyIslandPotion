// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SIPCharacter.h"
#include "Input/SIPInputConfig.h"
#include "SIPHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
struct FInputActionValue;


//TODO： 1. 目前先在角色类中直接绑定输入，后续最好通过输入组件的方式来实现 2. 最好把玩家的操控独立成一个组件，方便随时切换操作对象（宠物、坐骑）
/*
* 玩家所操控的英雄类
*/
UCLASS(config=Game)
class ASIPHeroCharacter : public ASIPCharacter
{
	GENERATED_BODY()

public:
	ASIPHeroCharacter(const FObjectInitializer& ObjectInitializer);

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InputMappingContext;

	/** InputConfig */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Input")
	TObjectPtr<USIPInputConfig> InputConfig;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	// INPUT
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	
	/** Called for movement input */
	void Input_Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Input_Look(const FInputActionValue& Value);
			
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PostInitializeComponents() override;
	
	// To add mapping context
	virtual void BeginPlay();

	
};

