#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/Components/SIPComponent.h"
#include "SIPSandboxLocomotionComponent.generated.h"

class ASIPCharacter;
class USIPAbilitySystemComponent;
class UCharacterMovementComponent;

/**
 * 供沙盒动画层使用的轻量移动语义枚举。
 * 实际速度仍可由 GAS 控制，而动画层通过这个枚举稳定选择走路、跑步和冲刺资源。
 */
UENUM(BlueprintType)
enum class ESIPSandboxDesiredGait : uint8
{
	Walk,
	Run,
	Sprint
};

/**
 * 把玩家的移动意图同时桥接到三个位置：
 * 1. CharacterMovement 的转向与速度规则。
 * 2. 供玩法和动画消费的 Loose Gameplay Tags。
 * 3. 主角身上供 AnimBP 读取的一份轻量线程安全快照。
 */
UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPSandboxLocomotionComponent : public USIPComponent
{
	GENERATED_BODY()

public:
	// 组件本身是轻量事件驱动结构，不需要开启 Tick。
	USIPSandboxLocomotionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 缓存拥有者引用，并把初始移动状态同步到移动组件和动画层。
	virtual void BeginPlay() override;

	// 更新步行意图，会限制移动速度并改变期望步态。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void SetWalkIntent(bool bEnabled);

	// 更新冲刺意图，会抬高移动速度并改变期望步态。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void SetSprintIntent(bool bEnabled);

	// 切换瞄准表现状态，并启用由控制器驱动的朝向。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void SetAimIntent(bool bEnabled);

	// 切换横移表现状态，并启用由控制器驱动的朝向。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void SetStrafeIntent(bool bEnabled);

	// 标记当前处于 Traversal，让 Root Motion 暂时接管移动。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void SetTraversalActive(bool bEnabled);
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void SetIceSurfaceActive(bool bEnabled);

	// 把下蹲输入转发给拥有者角色，并同步下蹲标签。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void HandleCrouchPressed();

	// 把取消下蹲输入转发给拥有者角色，并同步下蹲标签。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void HandleCrouchReleased();

	// 当外部战斗语义（如武器模组、施法阶段）变化时，
	// 重新评估由该组件负责的移动语义和 CharacterMovement 配置。
	UFUNCTION(BlueprintCallable, Category = "SIP|Sandbox|Locomotion")
	void HandleExternalSemanticStateChanged();

	// 查询玩家当前是否偏好步行节奏。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	bool WantsToWalk() const { return bWalkIntent; }

	// 查询玩家当前是否偏好冲刺节奏。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	bool WantsToSprint() const { return bSprintIntent; }

	// 查询玩家当前是否处于瞄准移动模式。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	bool WantsToAim() const { return bAimIntent; }

	// 查询玩家当前是否请求横移移动模式。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	bool WantsToStrafe() const { return bStrafeIntent; }

	// 查询当前是否由 Traversal 逻辑接管移动。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	bool IsTraversalActive() const { return bTraversalActive; }
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	bool IsIceSurfaceActive() const { return bIceSurfaceActive; }

	// 判断 CharacterMovement 是否应该面向控制器，而不是面向移动方向。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	bool ShouldUseControllerDesiredRotation() const;

	// 返回当前意图组合下的高层步态结果。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	ESIPSandboxDesiredGait GetDesiredGait() const;

	// 返回当前步态请求对应的目标移动速度。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	float GetDesiredMaxWalkSpeed() const;

	// 暴露未被覆盖过的基础移动速度，便于动画逻辑做对比。
	UFUNCTION(BlueprintPure, Category = "SIP|Sandbox|Locomotion")
	float GetBaseMoveSpeed() const { return CachedBaseMoveSpeed; }

private:
	/**
	 * 从 loose gameplay tags 中判断当前是否处于 FlaskRig 施法窗口。
	 */
	bool IsFlaskRigCasting() const;

	/**
	 * 判断当前是否处于 Ice Rune Dagger 的战斗转向语义状态。
	 *
	 * 这些状态需要的转向手感更接近战斗，而不是普通冰面移动。
	 */
	bool IsIceRuneDaggerCombatSteeringActive() const;
	bool IsAttackMontageActive() const;

	// 在玩法开始后解析并缓存角色、ASC 和移动组件。
	void CacheOwnerReferences();

	// 在瞄准、横移或 Traversal 状态变化后重新配置转向规则。
	void RefreshRotationMode();
	void RefreshSurfaceMovementProfile();

	// 在 CharacterMovement 上应用或移除步行/冲刺速度覆盖。
	void RefreshWalkSpeedOverride();

	// 把移动表现标签镜像同步到拥有者 ASC。
	void SetLooseStateTag(const FGameplayTag& Tag, bool bEnabled);

	// 把最新移动快照回写给主角，供 AnimBP 安全读取。
	void SyncOwnerAnimationState();

	// 缓存拥有者角色，供移动和下蹲请求复用。
	TWeakObjectPtr<ASIPCharacter> OwnerCharacter;

	// 缓存 ASC，降低同步移动表现标签时的查找开销。
	TWeakObjectPtr<USIPAbilitySystemComponent> OwnerAbilitySystemComponent;

	// 缓存移动组件，负责接收转向和速度覆盖。
	TWeakObjectPtr<UCharacterMovementComponent> OwnerMovementComponent;

	// 打开后，组件会把请求到的步态直接写回 MaxWalkSpeed。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion")
	bool bDriveWalkSpeedOverride = true;

	// 步行意图激活时使用的最大速度上限。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion", meta = (ClampMin = "0.0"))
	float WalkSpeedCap = 165.0f;

	// 冲刺意图激活时保证的最低速度。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion", meta = (ClampMin = "0.0"))
	float SprintSpeedFloor = 650.0f;

	// 面向移动方向时使用的默认转向速度。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion")
	FRotator MovementRotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// 控制器朝向模式下使用的更快转向速度。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion")
	FRotator StrafeRotationRate = FRotator(0.0f, 720.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Combat")
	FRotator IceCombatRotationRate = FRotator(0.0f, 860.0f, 0.0f);

	// 炼金投掷处于预备/释放阶段时，临时压低移动速度，
	// 让角色更像“收身准备出手”，同时也给 MM 一个更清晰的语义信号。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Combat", meta = (ClampMin = "0.0"))
	float FlaskRigCastSpeedCap = 320.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Ice", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float IceMaxAccelerationMultiplier = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Ice", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float IceBrakingDecelerationMultiplier = 0.20f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Ice", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float IceGroundFrictionMultiplier = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Ice", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float IceBrakingFrictionFactorMultiplier = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Ice", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float IceRotationRateMultiplier = 0.65f;

	// 冰面冲刺速度上限。
	// 防止角色达到实际轨迹与 PoseSearch 动画轨迹严重不匹配的高速区，
	// 在该速度下 trajectory 分歧足够小，PoseSearch 可以稳定选到合理候选。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Ice", meta = (ClampMin = "0.0"))
	float IceSprintSpeedCap = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Sandbox|Locomotion|Ice")
	bool bStartOnIceForDebug = false;

	// 来自输入侧的粘性意图开关状态。
	bool bWalkIntent = false;
	bool bSprintIntent = false;
	bool bAimIntent = false;
	bool bStrafeIntent = false;
	bool bTraversalActive = false;
	bool bIceSurfaceActive = false;

	// 记录当前 MaxWalkSpeed 是否正被本组件覆盖。
	bool bWalkSpeedOverridden = false;

	// 组件覆盖步态前原本存在的移动速度值。
	float CachedBaseMoveSpeed = 0.0f;
	float CachedBaseMaxAcceleration = 0.0f;
	float CachedBaseBrakingDecelerationWalking = 0.0f;
	float CachedBaseGroundFriction = 0.0f;
	float CachedBaseBrakingFrictionFactor = 0.0f;
	bool bHasCachedBaseMovementProfile = false;
};
