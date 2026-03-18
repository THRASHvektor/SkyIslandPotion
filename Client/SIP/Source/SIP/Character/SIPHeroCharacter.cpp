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
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "MotionWarpingComponent.h"
#include "TimerManager.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Ability/SIPAbilitySet.h"
#include "Input/SIPInputComponent.h"
#include "Controller/SIPPlayerController.h"
#include "./Components/InteractionComponent.h"
#include "./Components/SIPHeroAnimationBridgeComponent.h"
#include "./Components/SIPSandboxLocomotionComponent.h"

namespace
{
	struct FHeroAnimationPrototypeAssetPaths
	{
		const TCHAR* SkeletalMeshPath = nullptr;
		const TCHAR* AnimBlueprintClassPath = nullptr;
	};

	ESIPSandboxDesiredGait GetSampleCompatibleGait(const ESIPSandboxDesiredGait InGait)
	{
		// 当前迁移过来的样例本地只带了走路/跑步的 Pose Search 数据库。
		// 因此先保留冲刺的玩法速度，但动画选择暂时继续复用跑步资产，
		// 等专门的冲刺数据库补齐后再切回真正的冲刺表现。
		return InGait == ESIPSandboxDesiredGait::Sprint ? ESIPSandboxDesiredGait::Run : InGait;
	}

	bool IsKnownTemplateAnimBlueprintPath(const FString& ClassPath)
	{
		return
			ClassPath.Contains(TEXT("/Game/Characters/Mannequins/Animations/ABP_")) ||
			ClassPath.Contains(TEXT("/Game/StylizedEnvironment/Demo/Characters/Mannequins/Animations/ABP_")) ||
			ClassPath.Contains(TEXT("ABP_Quinn_C")) ||
			ClassPath.Contains(TEXT("ABP_Manny_C"));
	}

	const FHeroAnimationPrototypeAssetPaths* GetHeroAnimationPrototypeAssetPaths(const ESIPHeroAnimationPrototypePreset Preset)
	{
		static const FHeroAnimationPrototypeAssetPaths MannyLocomotionPaths
		{
			TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
			TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C")
		};

		static const FHeroAnimationPrototypeAssetPaths CombatMagicUnarmedPaths
		{
			TEXT("/Game/CombatMagicAnims/Demo/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
			TEXT("/Game/CombatMagicAnims/Demo/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")
		};

		switch (Preset)
		{
		case ESIPHeroAnimationPrototypePreset::MannyLocomotion:
			return &MannyLocomotionPaths;
		case ESIPHeroAnimationPrototypePreset::CombatMagicUnarmed:
			return &CombatMagicUnarmedPaths;
		default:
			return nullptr;
		}
	}
}

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
	PrimaryActorTick.bCanEverTick = true;
	
	// Z 说明：设置胶囊体碰撞体大小
	// 标准人类角色大小：直径42，高96
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Z 说明：控制器旋转设置
	// 当该值为 false 时，控制器旋转不会直接带动角色旋转。
	// 这样角色移动方向可以独立于摄像机方向
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Z 说明：角色移动配置
	// 当 `bOrientRotationToMovement` 为 true 时，角色会自动转向移动方向。
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
	bCachedCameraBoomCollisionTest = CameraBoom->bDoCollisionTest;

	// Z 说明：创建跟随摄像机
	// 绑定到摄像机臂的插槽
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	
	// Z 说明：不使用控制器旋转
	// 摄像机不独立旋转，只跟随臂移动
	FollowCamera->bUsePawnControlRotation = false;

	// Z 说明：创建交互组件
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	HeroAnimationBridgeComponent = CreateDefaultSubobject<USIPHeroAnimationBridgeComponent>(TEXT("HeroAnimationBridgeComponent"));
	
	// Z 变更说明：MotionWarping 统一为单一权威引用。
	// 之前代码里出现过同一组件的双字段镜像，后续蓝图/代码若分别读写不同字段，
	// 很容易产生“WarpTarget 已更新但动画消费到旧值”的错位问题。
	// 这里明确只保留 MotionWarping 这一个入口，避免语义分叉。
	UMotionWarpingComponent* CreatedMotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	MotionWarping = CreatedMotionWarpingComponent;
	SandboxLocomotionComponent = CreateDefaultSubobject<USIPSandboxLocomotionComponent>(TEXT("SandboxLocomotionComponent"));
}

USIPSandboxLocomotionComponent* ASIPHeroCharacter::GetSandboxLocomotionComponentBP() const
{
	return SandboxLocomotionComponent;
}

bool ASIPHeroCharacter::WantsToWalk() const
{
	return SandboxLocomotionComponent ? SandboxLocomotionComponent->WantsToWalk() : false;
}

bool ASIPHeroCharacter::WantsToSprint() const
{
	return SandboxLocomotionComponent ? SandboxLocomotionComponent->WantsToSprint() : false;
}

bool ASIPHeroCharacter::WantsToAim() const
{
	return SandboxLocomotionComponent ? SandboxLocomotionComponent->WantsToAim() : false;
}

bool ASIPHeroCharacter::WantsToStrafe() const
{
	return SandboxLocomotionComponent ? SandboxLocomotionComponent->WantsToStrafe() : false;
}

bool ASIPHeroCharacter::IsTraversalActive() const
{
	return SandboxLocomotionComponent ? SandboxLocomotionComponent->IsTraversalActive() : false;
}

ESIPSandboxDesiredGait ASIPHeroCharacter::GetDesiredGait() const
{
	return SandboxLocomotionComponent
		? GetSampleCompatibleGait(SandboxLocomotionComponent->GetDesiredGait())
		: ESIPSandboxDesiredGait::Run;
}

float ASIPHeroCharacter::GetDesiredMaxWalkSpeed() const
{
	return SandboxLocomotionComponent ? SandboxLocomotionComponent->GetDesiredMaxWalkSpeed() : GetCharacterMovement()->MaxWalkSpeed;
}

bool ASIPHeroCharacter::ShouldUseSandboxControllerDesiredRotation() const
{
	return SandboxLocomotionComponent ? SandboxLocomotionComponent->ShouldUseControllerDesiredRotation() : false;
}

void ASIPHeroCharacter::SetTraversalActive(const bool bEnabled)
{
	// Z 变更说明：Traversal 期间关闭 CameraBoom 碰撞测试，避免贴边攀爬时镜头被墙体挤压。
	// 进入 Traversal 时缓存原始开关，退出后恢复，保证该逻辑对其他状态“可逆”。
	if (CameraBoom)
	{
		if (bEnabled)
		{
			bCachedCameraBoomCollisionTest = CameraBoom->bDoCollisionTest;
			CameraBoom->bDoCollisionTest = false;
		}
		else
		{
			CameraBoom->bDoCollisionTest = bCachedCameraBoomCollisionTest;
		}
	}

	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetTraversalActive(bEnabled);
	}
}

bool ASIPHeroCharacter::TryConsumeJumpForTraversal_Implementation()
{
	// Z 变更说明：默认返回 false，表示“是否吃掉 Jump 输入”由蓝图实现决定。
	// 蓝图 `BP_ThirdPersonCharacter_Sandbox` 会覆盖此入口，并在检测成功时返回 true，
	// 从而阻断普通 Jump，改走 Traversal 动作链。
	return false;
}

void ASIPHeroCharacter::RefreshSandboxThreadSafeState()
{
	// Z 变更说明：把主角关键运动状态同步为 AnimBP 可安全读取的快照字段。
	// 目标：
	// 1) 降低蓝图层直接跨组件取值带来的时序抖动；
	// 2) 让 ABP/Chooser 统一读同一份状态源；
	// 3) 通过 LastFrame 字段支持“边沿检测”（如落地帧、状态切换帧）。
	if (LastSandboxStateSyncFrame != GFrameCounter)
	{
		SandboxMovementModeLastFrame = SandboxMovementMode;
		LastSandboxStateSyncFrame = GFrameCounter;
	}

	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const FVector CurrentVelocity = GetVelocity();
	const float CurrentGroundSpeed = CurrentVelocity.Size2D();
	const bool bIsMovingNow = CurrentGroundSpeed > 3.0f;
	const bool bUseControllerDesiredRotation = MovementComponent ? MovementComponent->bUseControllerDesiredRotation : false;
	const TEnumAsByte<EMovementMode> CurrentMovementMode =
		MovementComponent
			? TEnumAsByte<EMovementMode>(MovementComponent->MovementMode)
			: TEnumAsByte<EMovementMode>(EMovementMode::MOVE_None);

	SandboxVelocity = CurrentVelocity;
	SandboxGroundSpeed = CurrentGroundSpeed;
	SandboxIsMoving = bIsMovingNow;
	SandboxMovementMode = CurrentMovementMode;
	SandboxUseControllerDesiredRotation = bUseControllerDesiredRotation;
	SandboxRotationMode = bUseControllerDesiredRotation ? ESIPSandboxRotationMode::ControllerDesired : ESIPSandboxRotationMode::OrientToMovement;
	SandboxStance = bIsCrouched ? ESIPSandboxStance::Crouching : ESIPSandboxStance::Standing;

	if (SandboxLocomotionComponent)
	{
		SandboxWantsToWalk = SandboxLocomotionComponent->WantsToWalk();
		SandboxWantsToSprint = SandboxLocomotionComponent->WantsToSprint();
		SandboxWantsToAim = SandboxLocomotionComponent->WantsToAim();
		SandboxWantsToStrafe = SandboxLocomotionComponent->WantsToStrafe();
		SandboxTraversalActive = SandboxLocomotionComponent->IsTraversalActive();
		SandboxDesiredGait = GetSampleCompatibleGait(SandboxLocomotionComponent->GetDesiredGait());
		SandboxDesiredMaxWalkSpeed = SandboxLocomotionComponent->GetDesiredMaxWalkSpeed();
	}
	else
	{
		SandboxWantsToWalk = false;
		SandboxWantsToSprint = false;
		SandboxWantsToAim = false;
		SandboxWantsToStrafe = false;
		SandboxTraversalActive = false;
		SandboxDesiredGait = ESIPSandboxDesiredGait::Run;
		SandboxDesiredMaxWalkSpeed = MovementComponent ? MovementComponent->MaxWalkSpeed : 0.0f;
	}

	if (SandboxTraversalActive)
	{
		SandboxMovementState = ESIPSandboxMovementState::Traversal;
	}
	else if (CurrentMovementMode == MOVE_Falling)
	{
		SandboxMovementState = ESIPSandboxMovementState::InAir;
	}
	else
	{
		SandboxMovementState = ESIPSandboxMovementState::Grounded;
	}
}

void ASIPHeroCharacter::ClearJustLandedFlag()
{
	JustLanded = false;
}

/**
 * Z 说明：BeginPlay
 * 角色开始游戏时调用
 * 调用基类的 BeginPlay
 */
void ASIPHeroCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Z 变更说明：开局即应用动画策略，确保运行时 AnimBP 与蓝图预期一致。
	// 这一步是解决“Traversal 计算链和实际动画消费链不一致”的关键入口之一。
	ApplyHeroAnimationBlueprintPolicy();
	RefreshSandboxThreadSafeState();
}

void ASIPHeroCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshSandboxThreadSafeState();
}

void ASIPHeroCharacter::Landed(const FHitResult& Hit)
{
	LandVelocity = GetVelocity();
	JustLanded = true;

	Super::Landed(Hit);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ASIPHeroCharacter::ClearJustLandedFlag);
	}
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
	// Z 变更说明：在组件初始化后再次应用动画策略，覆盖父类/模板遗留默认值。
	// 这么做是为了兼容不同构造路径（编辑器预览、热重载、运行时 Spawn）。
	ApplyHeroAnimationBlueprintPolicy();

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
/**
 * Z 说明：ApplyHeroAnimationBlueprintPolicy
 * 运行时统一处理主角动画蓝图策略：
 * 1. 先禁用 SkeletalMesh 上的默认 Post Process Anim Blueprint
 * 2. 如果配置了自定义 AnimBP，则强制切换到该类
 * 3. 如果没有配置自定义 AnimBP，则移除 Quinn / Manny 等模板遗留 ABP
 */
void ASIPHeroCharacter::ApplyHeroAnimationBlueprintPolicy()
{
	// Z 变更说明：本函数是“最终动画控制权收口点”。
	// 规则优先级：
	// 1) 如果启用 prototype 且允许优先，则使用 prototype Mesh/AnimBP；
	// 2) 否则优先显式 HeroAnimBlueprintClassOverride；
	// 3) 若都无，则按配置决定是否清理模板遗留 ABP。
	// 目标是让 Traversal/MotionWarping 的数据生产与动画消费始终在同一语义链上。
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	USkeletalMesh* PrototypeMesh = nullptr;
	UClass* PrototypeAnimBlueprintClass = nullptr;
	const bool bHasPrototypeAssets = ResolveHeroAnimationPrototypeAssets(PrototypeMesh, PrototypeAnimBlueprintClass);

	USkeletalMesh* OverrideMesh = HeroSkeletalMeshOverride ? HeroSkeletalMeshOverride.Get() : PrototypeMesh;
	if (OverrideMesh && MeshComponent->GetSkeletalMeshAsset() != OverrideMesh)
	{
		MeshComponent->SetSkeletalMesh(OverrideMesh, false);
		UE_LOG(LogSIPCharacter, Log, TEXT("[%s] Applied hero SkeletalMesh override [%s]."), *GetNameSafe(this), *GetNameSafe(OverrideMesh));
	}

	const bool bShouldDisablePostProcessAnimationBlueprint = bHasPrototypeAssets ? false : bDisablePostProcessAnimationBlueprint;
	if (MeshComponent->GetDisablePostProcessBlueprint() != bShouldDisablePostProcessAnimationBlueprint)
	{
		MeshComponent->SetDisablePostProcessBlueprint(bShouldDisablePostProcessAnimationBlueprint);
		UE_LOG(
			LogSIPCharacter,
			Log,
			TEXT("[%s] %s SkeletalMesh post process animation blueprint."),
			*GetNameSafe(this),
			bShouldDisablePostProcessAnimationBlueprint ? TEXT("Disabled") : TEXT("Enabled"));
	}

	UClass* CurrentAnimClass = MeshComponent->GetAnimClass();
	UClass* ExplicitAnimBlueprintOverride = HeroAnimBlueprintClassOverride.Get();
	UClass* OverrideAnimClass = ExplicitAnimBlueprintOverride;
	const bool bHasExplicitAnimBlueprintOverride = (ExplicitAnimBlueprintOverride != nullptr);
	const bool bShouldPreferPrototypePreset = bHasPrototypeAssets && bPreferAnimationPrototypePreset;

	if (bHasPrototypeAssets && HeroAnimationPrototypePreset != ESIPHeroAnimationPrototypePreset::None && bHasExplicitAnimBlueprintOverride)
	{
		UE_LOG(
			LogSIPCharacter,
			Log,
			TEXT("[%s] Hero animation preset [%s] is active and %s explicit AnimBP override [%s]."),
			*GetNameSafe(this),
			*StaticEnum<ESIPHeroAnimationPrototypePreset>()->GetValueAsString(HeroAnimationPrototypePreset),
			bShouldPreferPrototypePreset ? TEXT("overrides") : TEXT("is overridden by"),
			*GetNameSafe(ExplicitAnimBlueprintOverride));
	}

	if (bShouldPreferPrototypePreset)
	{
		OverrideAnimClass = PrototypeAnimBlueprintClass;
	}
	else if (!OverrideAnimClass && bHasPrototypeAssets)
	{
		OverrideAnimClass = PrototypeAnimBlueprintClass;
	}

	const bool bAllowTemplateAnimBlueprintOverride =
		(bHasExplicitAnimBlueprintOverride && bAllowExplicitTemplateAnimationBlueprintOverride) ||
		(!bHasExplicitAnimBlueprintOverride && bHasPrototypeAssets && OverrideAnimClass != nullptr);

	if (OverrideAnimClass && bBlockTemplateAnimationBlueprints && IsTemplateAnimationBlueprintClass(OverrideAnimClass) && !bAllowTemplateAnimBlueprintOverride)
	{
		UE_LOG(LogSIPCharacter, Warning, TEXT("[%s] Ignoring template AnimBP override [%s]."), *GetNameSafe(this), *GetNameSafe(OverrideAnimClass));
		OverrideAnimClass = nullptr;
	}

	if (OverrideAnimClass)
	{
		if (CurrentAnimClass != OverrideAnimClass)
		{
			MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			MeshComponent->SetAnimInstanceClass(OverrideAnimClass);
			UE_LOG(LogSIPCharacter, Log, TEXT("[%s] Applied hero AnimBP override [%s]."), *GetNameSafe(this), *GetNameSafe(OverrideAnimClass));
		}
		return;
	}

	if (bBlockTemplateAnimationBlueprints && IsTemplateAnimationBlueprintClass(CurrentAnimClass))
	{
		UE_LOG(LogSIPCharacter, Warning, TEXT("[%s] Removed template AnimBP [%s]."), *GetNameSafe(this), *GetNameSafe(CurrentAnimClass));
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		MeshComponent->SetAnimInstanceClass(nullptr);
	}
}

/**
 * Z 说明：IsTemplateAnimationBlueprintClass
 * 用于识别项目里仍然残留的模板默认动画蓝图
 */
/**
 * Z 说明：把“动画原型预设”解析成可直接在运行时应用的资源。
 * 这里专门做成代码解析，是为了让主角蓝图只切一个枚举就能看到 Manny / Unarmed 原型，
 * 不需要再手工逐个改 Mesh、AnimBP 和相关运行时策略。
 */
bool ASIPHeroCharacter::ResolveHeroAnimationPrototypeAssets(USkeletalMesh*& OutSkeletalMesh, UClass*& OutAnimBlueprintClass) const
{
	// Z 变更说明：把“预设枚举 -> 资源路径 -> 运行时对象”这条链路集中在这里。
	// 这样蓝图只改枚举就能切换原型，不必同时改 Mesh、AnimBP 和策略布尔组合。
	OutSkeletalMesh = nullptr;
	OutAnimBlueprintClass = nullptr;

	const FHeroAnimationPrototypeAssetPaths* PrototypeAssetPaths = GetHeroAnimationPrototypeAssetPaths(HeroAnimationPrototypePreset);
	if (!PrototypeAssetPaths)
	{
		return false;
	}

	if (PrototypeAssetPaths->SkeletalMeshPath)
	{
		OutSkeletalMesh = LoadObject<USkeletalMesh>(nullptr, PrototypeAssetPaths->SkeletalMeshPath);
		if (!OutSkeletalMesh)
		{
			UE_LOG(LogSIPCharacter, Warning, TEXT("[%s] Failed to load hero animation prototype mesh [%s]."), *GetNameSafe(this), PrototypeAssetPaths->SkeletalMeshPath);
		}
	}

	if (PrototypeAssetPaths->AnimBlueprintClassPath)
	{
		OutAnimBlueprintClass = LoadClass<UAnimInstance>(nullptr, PrototypeAssetPaths->AnimBlueprintClassPath);
		if (!OutAnimBlueprintClass)
		{
			UE_LOG(LogSIPCharacter, Warning, TEXT("[%s] Failed to load hero animation prototype AnimBP [%s]."), *GetNameSafe(this), PrototypeAssetPaths->AnimBlueprintClassPath);
		}
	}

	return OutSkeletalMesh != nullptr || OutAnimBlueprintClass != nullptr;
}

bool ASIPHeroCharacter::IsTemplateAnimationBlueprintClass(const UClass* AnimClass) const
{
	return AnimClass && IsKnownTemplateAnimBlueprintPath(AnimClass->GetPathName());
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

			if (SandboxLocomotionComponent)
			{
				if (const UInputAction* WalkAction = InputConfig->FindInputActionForTag(SIPGameplayTags::InputTag_Walk, false))
				{
					SIPIC->BindAction(WalkAction, ETriggerEvent::Started, this, &ASIPHeroCharacter::Input_WalkPressed);
					SIPIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &ASIPHeroCharacter::Input_WalkReleased);
				}

				if (const UInputAction* SprintAction = InputConfig->FindInputActionForTag(SIPGameplayTags::InputTag_Sprint, false))
				{
					SIPIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ASIPHeroCharacter::Input_SprintPressed);
					SIPIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASIPHeroCharacter::Input_SprintReleased);
				}

				if (const UInputAction* AimAction = InputConfig->FindInputActionForTag(SIPGameplayTags::InputTag_Aim, false))
				{
					SIPIC->BindAction(AimAction, ETriggerEvent::Started, this, &ASIPHeroCharacter::Input_AimPressed);
					SIPIC->BindAction(AimAction, ETriggerEvent::Completed, this, &ASIPHeroCharacter::Input_AimReleased);
				}

				if (const UInputAction* StrafeAction = InputConfig->FindInputActionForTag(SIPGameplayTags::InputTag_Strafe, false))
				{
					SIPIC->BindAction(StrafeAction, ETriggerEvent::Started, this, &ASIPHeroCharacter::Input_StrafePressed);
					SIPIC->BindAction(StrafeAction, ETriggerEvent::Completed, this, &ASIPHeroCharacter::Input_StrafeReleased);
				}

				if (const UInputAction* CrouchAction = InputConfig->FindInputActionForTag(SIPGameplayTags::InputTag_Crouch, false))
				{
					SIPIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASIPHeroCharacter::Input_CrouchPressed);
					SIPIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ASIPHeroCharacter::Input_CrouchReleased);
				}
			}

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
	// 在翻越/攀爬期间，让 Traversal 的 Root Motion 接管角色移动。
	if (SandboxLocomotionComponent && SandboxLocomotionComponent->IsTraversalActive())
	{
		return;
	}

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
		// Y 对应前后方向，X 对应左右方向。
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
		// X 控制左右转向，Y 控制上下视角。
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

/**
 * Z 说明：Input_WalkPressed / Released
 * 把慢走意图同步到 SandboxLocomotionComponent，
 * 让主角在保留 SIP 输入链的前提下切入 sandbox locomotion 的 gait 语义。
 */
void ASIPHeroCharacter::Input_WalkPressed()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetWalkIntent(true);
	}
}

void ASIPHeroCharacter::Input_WalkReleased()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetWalkIntent(false);
	}
}

/**
 * Z 说明：Input_SprintPressed / Released
 * 这里只维护 locomotion 的冲刺意图；
 * 真正的移动速度增益仍由 GAS 的 Sprint Ability 负责。
 */
void ASIPHeroCharacter::Input_SprintPressed()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetSprintIntent(true);
	}
}

void ASIPHeroCharacter::Input_SprintReleased()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetSprintIntent(false);
	}
}

/**
 * Z 说明：Input_AimPressed / Released
 * 先通过桥接组件把瞄准/平移意图喂回 CharacterMovement 和 ASC loose tag，
 * 后面蓝图再拿这些状态去驱动 sandbox 的转向、分层和姿态选择。
 */
void ASIPHeroCharacter::Input_AimPressed()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetAimIntent(true);
	}
}

void ASIPHeroCharacter::Input_AimReleased()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetAimIntent(false);
	}
}

void ASIPHeroCharacter::Input_StrafePressed()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetStrafeIntent(true);
	}
}

void ASIPHeroCharacter::Input_StrafeReleased()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->SetStrafeIntent(false);
	}
}

/**
 * Z 说明：Input_CrouchPressed / Released
 * 当前先做成按下/松开驱动 crouch 的轻接入版本，
 * 这样不会破坏现有 SIP 主角链，后面要改成 sandbox 原版策略时也只需要收口到组件内。
 */
void ASIPHeroCharacter::Input_CrouchPressed()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->HandleCrouchPressed();
	}
}

void ASIPHeroCharacter::Input_CrouchReleased()
{
	if (SandboxLocomotionComponent)
	{
		SandboxLocomotionComponent->HandleCrouchReleased();
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

	if (InputTag.MatchesTagExact(SIPGameplayTags::InputTag_Jump) && TryConsumeJumpForTraversal())
	{
		bTraversalConsumedJumpInput = true;
		UE_LOG(LogSIPCharacter, Log, TEXT("Jump input consumed by sandbox traversal for [%s]."), *GetNameSafe(this));
		return;
	}

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

	if (InputTag.MatchesTagExact(SIPGameplayTags::InputTag_Jump) && bTraversalConsumedJumpInput)
	{
		bTraversalConsumedJumpInput = false;
		return;
	}

	if(USIPAbilitySystemComponent* SIPASC = GetSIPAbilitySystemComponent())
	{
		SIPASC->AbilityInputTagReleased(InputTag);
	}
}
