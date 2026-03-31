// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * ASIPHeroCharacter 是玩家控制的主角角色类
 * 继承自 ASIPCharacter，是玩家在游戏中实际控制的对象
 * 
 * 主要功能：
 * 1. 管理摄像机（SpringArm + FollowCamera）
 * 2. 处理增强输入系统（Enhanced Input）
 * 3. 绑定输入到技能系统
 * 4. 初始化并授予角色技能
 * 
 * 设计思路：
 * - 使用 Enhanced Input 系统，支持更灵活的输入配置
 * - 通过 InputConfig 数据资产配置输入映射
 * - 继承自 ASIPCharacter，复用 GAS 能力系统
 */

#pragma once

#include "CoreMinimal.h"
#include "Combat/SIPCombatSemanticResolver.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SIPCharacter.h"
#include "Character/Components/SIPSandboxLocomotionComponent.h"
#include "Input/SIPInputConfig.h"
#include "SIPHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInteractionComponent;
class UMotionWarpingComponent;
class USIPHeroAnimationBridgeComponent;
class USIPContextualCameraComponent;
class UAnimInstance;
class UInputMappingContext;
class UGameplayEffect;
class USkeletalMesh;
struct FInputActionValue;
struct FActiveGameplayEffectHandle;
struct FHitResult;

UENUM(BlueprintType)
enum class ESIPHeroAnimationPrototypePreset : uint8
{
	None UMETA(DisplayName = "None"),
	MannyLocomotion UMETA(DisplayName = "Manny Locomotion"),
	CombatMagicUnarmed UMETA(DisplayName = "CombatMagic Unarmed")
};

UENUM(BlueprintType)
enum class ESIPSandboxMovementState : uint8
{
	Grounded UMETA(DisplayName = "Grounded"),
	InAir UMETA(DisplayName = "In Air"),
	Traversal UMETA(DisplayName = "Traversal")
};

UENUM(BlueprintType)
enum class ESIPSandboxRotationMode : uint8
{
	OrientToMovement UMETA(DisplayName = "Orient To Movement"),
	ControllerDesired UMETA(DisplayName = "Controller Desired")
};

UENUM(BlueprintType)
enum class ESIPSandboxStance : uint8
{
	Standing UMETA(DisplayName = "Standing"),
	Crouching UMETA(DisplayName = "Crouching")
};


/**
 * TODO 注释：代码优化方向
 * 1. 目前先在角色类中直接绑定输入，后续最好通过输入组件的方式来实现
 * 2. 最好把玩家的操控独立成一个组件，方便随时切换操作对象（宠物、坐骑）
 */

/**
 * ASIPHeroCharacter 是玩家所操控的英雄类
 * 
 * 继承层次：
 * AActor → APawn → ACharacter → ASIPCharacter → ASIPHeroCharacter
 * 
 * 使用场景：
 * - 玩家控制的角色
 * - 需要摄像机跟随
 * - 需要技能系统
 */
UCLASS(config=Game)
class ASIPHeroCharacter : public ASIPCharacter
{
	GENERATED_BODY()

public:
	/**
	 * 初始化组件和角色参数
	 */
	ASIPHeroCharacter(const FObjectInitializer& ObjectInitializer);

	/**
	 * 用于控制第三人称视角的距离和旋转
	 * 
	 * 功能：
	 * - 拉近/拉远摄像机
	 * - 碰撞时自动收缩
	 * - 跟随角色旋转
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/**
	 * 绑定到 CameraBoom，自动跟随角色
	 * 
	 * 特点：
	 * - 不随控制器旋转，独立于手臂旋转
	 * - 提供稳定的第三人称视角
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/**
	 * 用于处理与可交互对象的交互逻辑
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USIPHeroAnimationBridgeComponent> HeroAnimationBridgeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USIPContextualCameraComponent> ContextualCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USIPSandboxLocomotionComponent> SandboxLocomotionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxWantsToWalk = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxWantsToSprint = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxWantsToAim = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxWantsToStrafe = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxTraversalActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxOnIce = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxUseControllerDesiredRotation = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	ESIPSandboxDesiredGait SandboxDesiredGait = ESIPSandboxDesiredGait::Run;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	float SandboxDesiredMaxWalkSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<EMovementMode> SandboxMovementMode = MOVE_Walking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<EMovementMode> SandboxMovementModeLastFrame = MOVE_Walking;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	ESIPSandboxMovementState SandboxMovementState = ESIPSandboxMovementState::Grounded;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	ESIPSandboxRotationMode SandboxRotationMode = ESIPSandboxRotationMode::OrientToMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	ESIPSandboxStance SandboxStance = ESIPSandboxStance::Standing;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	float SandboxGroundSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	FVector SandboxVelocity = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool JustLanded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	FVector LandVelocity = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SandboxWeaponModuleTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SandboxCastPhaseTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SandboxCombatActionFamilyTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	FGameplayTag SandboxCombatBodyStateTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	FName SandboxCombatDesiredVariant = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxCombatUseMomentumWarp = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	ESIPRecoveryBias SandboxCombatRecoveryBias = ESIPRecoveryBias::Fast;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	ESIPChainWindowPolicy SandboxCombatChainWindowPolicy = ESIPChainWindowPolicy::Normal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	bool SandboxShouldSuppressMotionMatching = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Sandbox|ThreadSafe", meta = (AllowPrivateAccess = "true"))
	ESIPSemanticLocomotionMode SandboxSemanticLocomotionMode = ESIPSemanticLocomotionMode::Default;

	/**
	 * 用于把主角运行时直接切到项目里现成的 Manny / Unarmed 资源链。
	 * 是否压过显式 AnimBP Override，由 bPreferAnimationPrototypePreset 控制。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	ESIPHeroAnimationPrototypePreset HeroAnimationPrototypePreset = ESIPHeroAnimationPrototypePreset::None;

	/**
	 * 开启后可以直接压过之前遗留的 BP_HeroAnimInstance 之类的临时承载蓝图，
	 * 更适合当前“先把真原型跑起来”的阶段。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	bool bPreferAnimationPrototypePreset = true;

	/**
	 * 如果为空，则允许动画原型预设自动提供对应的 Mesh。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMesh> HeroSkeletalMeshOverride;

	/**
	 * 开启后会自动移除 ABP_Quinn / ABP_Manny 这类模板遗留 AnimBP
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	bool bBlockTemplateAnimationBlueprints = true;

	/**
	 * - 若设置，则会在运行时强制覆盖蓝图里继承下来的默认 AnimBP
	 * - 若未设置且开启了模板屏蔽，则会直接清空模板 AnimBP
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UAnimInstance> HeroAnimBlueprintClassOverride;

	/**
	 * 屏蔽模板的目的是避免蓝图残留误引用，不是阻止我们主动拿模板资源做原型。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	bool bAllowExplicitTemplateAnimationBlueprintOverride = true;

	/**
	 * 开启后可一并切断 Quinn / Manny 网格上的默认后处理动画蓝图
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Animation", meta = (AllowPrivateAccess = "true"))
	bool bDisablePostProcessAnimationBlueprint = true;

	/**
	 * 来自 Enhanced Input 系统
	 * 定义了按键到 InputAction 的映射
	 * 
	 * 使用方式：
	 * 在 Blueprint 中配置此属性，引用项目中的 IMC 资源
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* InputMappingContext;

	/**
	 * 定义了 InputAction 到 GameplayTag 的映射
	 * 
	 * 作用：
	 * - NativeInputActions: 手动绑定的输入（移动、视角）
	 * - AbilityInputActions: 自动绑定到技能的输入（攻击、跳跃）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Input")
	TObjectPtr<USIPInputConfig> InputConfig;

	/**
	 */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	
	/**
	 */
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	FORCEINLINE USIPHeroAnimationBridgeComponent* GetHeroAnimationBridgeComponent() const { return HeroAnimationBridgeComponent; }

	FORCEINLINE USIPContextualCameraComponent* GetContextualCameraComponent() const { return ContextualCameraComponent; }

	FORCEINLINE USIPSandboxLocomotionComponent* GetSandboxLocomotionComponent() const { return SandboxLocomotionComponent; }

	FORCEINLINE ESIPHeroAnimationPrototypePreset GetHeroAnimationPrototypePreset() const { return HeroAnimationPrototypePreset; }

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	USIPSandboxLocomotionComponent* GetSandboxLocomotionComponentBP() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	bool WantsToWalk() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	bool WantsToSprint() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	bool WantsToAim() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	bool WantsToStrafe() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	bool IsTraversalActive() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	bool IsOnIceSurface() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	ESIPSandboxDesiredGait GetDesiredGait() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	float GetDesiredMaxWalkSpeed() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox")
	bool ShouldUseSandboxControllerDesiredRotation() const;

	int32 GetResolvedAttackComboIndex(float ResetWindowSeconds);

	void CommitAttackComboIndex(int32 NextComboIndex);

	void ResetAttackComboState();

	void BufferAttackInput();

	void ClearBufferedAttackInput();

	bool ConsumeBufferedAttackInput(float MaxAgeSeconds);

	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox")
	void SetTraversalActive(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox")
	void SetIceSurfaceActive(bool bEnabled);

	UFUNCTION(BlueprintNativeEvent, Category = "SIP|Sandbox|Traversal")
	bool TryConsumeJumpForTraversal();

	void RefreshSandboxThreadSafeState();

protected:


	/**
	 * 当玩家按下技能按键时调用
	 * 将 InputTag 传递给 ASC 进行处理
	 */
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);

	/**
	 * 当玩家释放技能按键时调用
	 * 将 InputTag 传递给 ASC 进行处理
	 */
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	
	/**
	 * 处理 WASD/手柄摇杆输入
	 * 将输入转换为角色移动方向
	 */
	void Input_Move(const FInputActionValue& Value);

	/**
	 * 处理鼠标移动/手柄右摇杆
	 * 控制摄像机旋转
	 */
	void Input_Look(const FInputActionValue& Value);

	void Input_WalkPressed();
	void Input_WalkReleased();
	void Input_SprintPressed();
	void Input_SprintReleased();
	void Input_AimPressed();
	void Input_AimReleased();
	void Input_StrafePressed();
	void Input_StrafeReleased();
	void Input_CrouchPressed();
	void Input_CrouchReleased();

	/**
	 * UE 生命周期函数
	 * 在此绑定增强输入系统
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * 在此授予角色技能
	 */
	virtual void PostInitializeComponents() override;
	
	/**
	 * 可以在此添加角色初始化逻辑
	 */
	virtual void BeginPlay();

	virtual void Tick(float DeltaSeconds) override;

	virtual void Landed(const FHitResult& Hit) override;

	/**
	 */
	void ApplyHeroAnimationBlueprintPolicy();

	/**
	 * 这里只返回预设对应的资源，不覆盖蓝图里显式指定的 Override。
	 */
	bool ResolveHeroAnimationPrototypeAssets(USkeletalMesh*& OutSkeletalMesh, UClass*& OutAnimBlueprintClass) const;

	/**
	 */
	bool IsTemplateAnimationBlueprintClass(const UClass* AnimClass) const;

	virtual bool TryConsumeJumpForTraversal_Implementation();

	void ClearJustLandedFlag();

	uint64 LastSandboxStateSyncFrame = MAX_uint64;
	bool bTraversalConsumedJumpInput = false;
	int32 AttackComboIndex = 0;
	double LastAttackComboTimeSeconds = -1.0;
	bool bAttackInputBuffered = false;
	double LastAttackInputBufferedTimeSeconds = -1.0;
};
