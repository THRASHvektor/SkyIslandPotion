// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * SIPHeroCharacter.cpp 实现了玩家角色的核心功能
 * 
 * 主要功能：
 * 1. 角色初始化配置（移动参数、摄像机）
 * 2. 增强输入系统绑定
 * 3. 技能系统初始化和技能授予
 */

#include "SIPHeroCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Ability/SIPAbilitySet.h"
#include "Input/SIPInputComponent.h"
#include "Controller/SIPPlayerController.h"
#include "./Components/InteractionComponent.h"

/**
 * Z 说明：构造函数
 * 初始化角色组件和参数
 * 
 * 初始化内容：
 * 1. 胶囊体大小
 * 2. 角色旋转参数
 * 3. 移动参数（跳跃、行走、空中控制）
 * 4. 摄像机组件
 */
ASIPHeroCharacter::ASIPHeroCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	// Z 说明：设置胶囊体碰撞体大小
	// 标准人类角色大小：直径42，高96
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Z 说明：控制器旋转设置
	// false: 控制器旋转时角色不跟随旋转
	// 这样角色移动方向可以独立于摄像机方向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Z 说明：角色移动配置
	// bOrientRotationToMovement: true = 角色会自动转向移动方向
	// 这是第三人称游戏的常见设置
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	// Z 说明：旋转速率
	// 角色转向移动方向的速度
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Z 说明：跳跃参数
	// 跳跃时的垂直速度
	GetCharacterMovement()->JumpZVelocity = 700.f;
	
	// Z 说明：空中控制
	// 在空中时对移动输入的响应程度（0-1）
	// 0.35 表示在空中可以部分控制移动方向
	GetCharacterMovement()->AirControl = 0.35f;
	
	// Z 说明：行走速度
	// 正常行走时的最大速度
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	
	// Z 说明：最小模拟行走速度
	// 手柄摇杆推动时的最小速度
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	
	// Z 说明：行走减速
	// 松开按键后停止的速度
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	
	// Z 说明：下落减速
	// 空中移动停止时的减速
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Z 说明：创建摄像机臂组件
	// 摄像机跟随的基础
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	
	// Z 说明：摄像机距离
	// 摄像机在角色后方400单位
	CameraBoom->TargetArmLength = 400.0f;
	
	// Z 说明：使用控制器旋转
	// 摄像机臂跟随控制器旋转（鼠标/手柄控制视角）
	CameraBoom->bUsePawnControlRotation = true;

	// Z 说明：创建跟随摄像机
	// 绑定到摄像机臂的插槽
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	
	// Z 说明：不使用控制器旋转
	// 摄像机不独立旋转，只跟随臂移动
	FollowCamera->bUsePawnControlRotation = false;

	// Z 说明：创建交互组件
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
}

/**
 * Z 说明：BeginPlay
 * 角色开始游戏时调用
 * 调用基类的 BeginPlay
 */
void ASIPHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
}

/**
 * Z 说明：PostInitializeComponents
 * 组件初始化完成后调用
 * 在此初始化技能系统
 */
void ASIPHeroCharacter::PostInitializeComponents()
{
	// Z 说明：调用基类初始化
	Super::PostInitializeComponents();

	if (AbilitySystemComponent)
	{
		UE_LOG(LogSIPCharacter, Log, TEXT("Hero activatable abilities: %d"), AbilitySystemComponent->GetActivatableAbilities().Num());
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			UE_LOG(LogSIPCharacter, Log, TEXT("  Ability: %s, DynamicTags: %s"), *GetNameSafe(Spec.Ability), *Spec.DynamicAbilityTags.ToString());
		}
	}
}

/**
 * Z 说明：SetupPlayerInputComponent
 * 设置输入组件，绑定增强输入系统
 * 
 * 绑定流程：
 * 1. 获取本地玩家子系统
 * 2. 添加输入映射上下文
 * 3. 绑定移动、视角输入
 * 4. 绑定技能输入
 */
void ASIPHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Z 说明：获取玩家控制器
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	check(PlayerController);

	// Z 说明：获取增强输入子系统
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	check(Subsystem);

	// Z 说明：清除旧映射，添加新映射
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(InputMappingContext, 0);

	
	// 简化输入绑定，直接使用项目自定义的 SIPInputComponent(继承自 UEnhancedInputComponent)
	// 注意将项目设置中的输入 默认类-输入组件 改为 SIPInputComponent，否则无法使用此功能
	if (USIPInputComponent* SIPIC = Cast<USIPInputComponent>(PlayerInputComponent))
	{
		if (InputConfig)
		{
			SIPIC->BindNativeAction(InputConfig, SIPGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ASIPHeroCharacter::Input_Move, true);
			SIPIC->BindNativeAction(InputConfig, SIPGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ASIPHeroCharacter::Input_Look, true);

			TArray<uint32> BindHandles;
			// 一行代码设置所有 GA 技能的按下与松开绑定
			SIPIC->BindAbilityActions(InputConfig, this, &ASIPHeroCharacter::Input_AbilityInputTagPressed, &ASIPHeroCharacter::Input_AbilityInputTagReleased, BindHandles);
		}
	}
	
	// // Z 说明：绑定增强输入组件
	// if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	// {
	// 	if (InputConfig)
	// 	{
	// 		// Z 说明：绑定移动输入
	// 		EnhancedInputComponent->BindAction(
	// 			InputConfig->FindNativeInputActionForTag(SIPGameplayTags::InputTag_Move),
	// 			ETriggerEvent::Triggered,
	// 			this,
	// 			&ASIPHeroCharacter::Input_Move
	// 		);
	//
	// 		// Z 说明：绑定视角输入
	// 		EnhancedInputComponent->BindAction(
	// 			InputConfig->FindNativeInputActionForTag(SIPGameplayTags::InputTag_Look_Mouse),
	// 			ETriggerEvent::Triggered,
	// 			this,
	// 			&ASIPHeroCharacter::Input_Look
	// 		);
	//
	// 		// Z 说明：绑定技能输入
	// 		// 遍历 AbilityInputActions，绑定每个技能输入
	// 		TArray<uint32> BindHandles;
	// 		for (const FSIPInputAction& Action : InputConfig->AbilityInputActions)
	// 		{
	// 			// Z 说明：按下时激活技能（Started）
	// 			BindHandles.Add(
	// 				EnhancedInputComponent->BindAction(
	// 					Action.InputAction,
	// 					ETriggerEvent::Started,
	// 					this,
	// 					&ASIPHeroCharacter::Input_AbilityInputTagPressed,
	// 					Action.InputTag
	// 				).GetHandle()
	// 			);
	//
	// 			// Z 说明：松开时取消技能（Completed）
	// 			BindHandles.Add(
	// 				EnhancedInputComponent->BindAction(
	// 					Action.InputAction,
	// 					ETriggerEvent::Completed,
	// 					this,
	// 					&ASIPHeroCharacter::Input_AbilityInputTagReleased,
	// 					Action.InputTag
	// 				).GetHandle()
	// 			);
	// 		}
	// 	}
	// 	else
	// 	{
	// 		UE_LOG(LogSIPCharacter, Error, TEXT("'%s' Failed to find an valid input config."), *GetNameSafe(this));
	// 	}
	// }
	// else
	// {
	// 	UE_LOG(LogSIPCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	// }
}



/**
 * Z 说明：Input_Move
 * 处理移动输入
 * 
 * 输入处理流程：
 * 1. 获取输入向量（摇杆/键盘）
 * 2. 根据控制器旋转获取前后/左右方向
 * 3. 调用 AddMovementInput 移动角色
 */
void ASIPHeroCharacter::Input_Move(const FInputActionValue& Value)
{
	// Z 说明：获取二维输入向量
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Z 说明：获取控制器旋转（Yaw，只关心水平方向）
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Z 说明：获取前方向量
		// 根据 Yaw 旋转，计算世界坐标中的"前方"
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// Z 说明：获取右方向量
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Z 说明：应用移动输入
		// Y 对应前后（Forward），X 对应左右（Right）
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

/**
 * Z 说明：Input_Look
 * 处理视角输入
 * 
 * 输入处理：
 * - 鼠标移动 X → 角色左右转（Yaw）
 * - 鼠标移动 Y → 摄像机上下看（Pitch）
 */
void ASIPHeroCharacter::Input_Look(const FInputActionValue& Value)
{
	// Z 说明：获取二维视角输入
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Z 说明：添加视角输入
		// X = 左右转，Y = 上下看
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

/**
 * Z 说明：Input_AbilityInputTagPressed
 * 技能输入按下回调
 * 
 * 流程：
 * 1. 接收 InputTag（如 InputTag_Dash）
 * 2. 传递给 ASC 的 AbilityInputTagPressed
 * 3. ASC 内部查找匹配的 Ability 并激活
 */
void ASIPHeroCharacter::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	UE_LOG(LogSIPCharacter, Log, TEXT("Input_AbilityInputTagPressed: %s"), *InputTag.ToString());
	if(USIPAbilitySystemComponent* SIPASC = GetSIPAbilitySystemComponent())
	{
		SIPASC->AbilityInputTagPressed(InputTag);
	}
}

/**
 * Z 说明：Input_AbilityInputTagReleased
 * 技能输入释放回调
 * 
 * 流程：
 * 1. 接收 InputTag
 * 2. 传递给 ASC 的 AbilityInputTagReleased
 * 3. ASC 内部处理技能释放（如取消蓄力）
 */
void ASIPHeroCharacter::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	UE_LOG(LogSIPCharacter, Log, TEXT("Input_AbilityInputTagReleased: %s"), *InputTag.ToString());
	if(USIPAbilitySystemComponent* SIPASC = GetSIPAbilitySystemComponent())
	{
		SIPASC->AbilityInputTagReleased(InputTag);
	}
}
