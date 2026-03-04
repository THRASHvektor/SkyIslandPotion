// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPHeroCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Ability/SIPAbilitySet.h"
#include "GameplayEffect.h"



ASIPHeroCharacter::ASIPHeroCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Set movement parameters, also can set them in character blueprint
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

}

void ASIPHeroCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void ASIPHeroCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 初始化AbilitySystemComponent流程，后续可移至基类
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		
		UE_LOG(LogSIPCharacter, Log, TEXT("ASC Initialized. Granting abilities..."));

		// grant abilities
		FSIPAbilitySet_GrantedHandles GrantedHandles;
		for(const TObjectPtr<USIPAbilitySet>& Set : AbilitySets)
		{
			if (Set)
			{
				Set->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles);
			}
		}
					
		// 打印已激活的Ability信息用于调试
		UE_LOG(LogSIPCharacter, Log, TEXT("Total Activatable Abilities: %d"), AbilitySystemComponent->GetActivatableAbilities().Num());
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			UE_LOG(LogSIPCharacter, Log, TEXT("  Ability: %s, DynamicTags: %s"), 
				*GetNameSafe(Spec.Ability), 
				*Spec.DynamicAbilityTags.ToString());
		}
	}
}

void ASIPHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	check(PlayerController);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	check(Subsystem);

	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(InputMappingContext, 0);
	
	/* 后续可将下列绑定操作统一移至IC完成 */
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		if (InputConfig)
		{
			// // Jumping
			// EnhancedInputComponent->BindAction(InputConfig->FindNativeInputActionForTag(SIPGameplayTags::InputTag_Jump), ETriggerEvent::Started, this, &ACharacter::Jump);
			// EnhancedInputComponent->BindAction(InputConfig->FindNativeInputActionForTag(SIPGameplayTags::InputTag_Jump), ETriggerEvent::Completed, this, &ACharacter::StopJumping);

			// Moving
			EnhancedInputComponent->BindAction(InputConfig->FindNativeInputActionForTag(SIPGameplayTags::InputTag_Move), ETriggerEvent::Triggered, this, &ASIPHeroCharacter::Input_Move);

			// Looking
			EnhancedInputComponent->BindAction(InputConfig->FindNativeInputActionForTag(SIPGameplayTags::InputTag_Look_Mouse), ETriggerEvent::Triggered, this, &ASIPHeroCharacter::Input_Look);

			TArray<uint32> BindHandles;	// 可将此数列缓存用于解绑
			// Ability Input
			// 按下用Started（瞬间触发一次），松开用Completed
			for (const FSIPInputAction& Action : InputConfig->AbilityInputActions)
			{
				BindHandles.Add(EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Started, this, &ASIPHeroCharacter::Input_AbilityInputTagPressed, Action.InputTag).GetHandle());
				BindHandles.Add(EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &ASIPHeroCharacter::Input_AbilityInputTagReleased, Action.InputTag).GetHandle());
			}
			

		}
		else
		{
			UE_LOG(LogSIPCharacter, Error, TEXT("'%s' Failed to find an valid input config."), *GetNameSafe(this));
		}
		
	}
	else
	{
		UE_LOG(LogSIPCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASIPHeroCharacter::Input_Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASIPHeroCharacter::Input_Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void ASIPHeroCharacter::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	UE_LOG(LogSIPCharacter, Log, TEXT("Input_AbilityInputTagPressed: %s"), *InputTag.ToString());
	if(USIPAbilitySystemComponent* SIPASC = GetSIPAbilitySystemComponent())
	{
		SIPASC->AbilityInputTagPressed(InputTag);
	}
}

void ASIPHeroCharacter::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	UE_LOG(LogSIPCharacter, Log, TEXT("Input_AbilityInputTagReleased: %s"), *InputTag.ToString());
	if(USIPAbilitySystemComponent* SIPASC = GetSIPAbilitySystemComponent())
	{
		SIPASC->AbilityInputTagReleased(InputTag);
	}
}



