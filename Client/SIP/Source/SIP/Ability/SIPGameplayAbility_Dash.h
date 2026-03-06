#pragma once
/**
 * Z 说明：
 * USIPGameplayAbility_Dash 是闪现技能的实现
 * 继承自 UGameplayAbility
 * 
 * 技能特点：
 * 1. 快速位移技能，类似闪现
 * 2. 支持碰撞检测，防止穿墙
 * 3. 带有冷却时间（通过 GameplayEffect 实现）
 * 4. 支持 Niagara 视觉效果
 * 
 * 使用方式：
 * 1. 在 Blueprint 中配置 DashDistance（位移距离）
 * 2. 配置 DashCooldownEffect（冷却效果）
 * 3. 配置 DashTrailEffect/DashLandedEffect（视觉特效）
 */

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SIPGameplayAbility_Dash.generated.h"

class UGameplayEffect;
class UNiagaraSystem;
class USceneComponent;

/**
 * Z 说明：
 * USIPGameplayAbility_Dash 闪现技能类
 * 
 * 核心功能：
 * 1. 激活时计算位移方向
 * 2. 执行位移（可配置碰撞检测）
 * 3. 应用冷却效果
 * 4. 播放视觉特效
 */
UCLASS()
class SIP_API USIPGameplayAbility_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * Z 说明：构造函数
	 * 初始化技能属性
	 */
	USIPGameplayAbility_Dash(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/**
	 * Z 说明：激活技能
	 * 技能被激活时调用
	 * 执行位移、效果应用、特效播放
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/**
	 * Z 说明：结束技能
	 * 技能执行完毕后调用
	 * 清理工作
	 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * Z 说明：检查是否能激活技能
	 * 在激活前检查前置条件
	 * 
	 * 检查条件：
	 * - 基础条件（ASC 状态）
	 * - 角色有效性
	 * - 角色是否在空中（不允许在空中使用）
	 */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

private:
	/**
	 * Z 说明：计算位移方向
	 * 根据当前输入和摄像机方向计算位移方向
	 * 
	 * 优先级：
	 * 1. 有移动输入时，按输入方向
	 * 2. 无输入时，按摄像机朝向
	 */
	FVector CalculateDashDirection() const;

	/**
	 * Z 说明：执行位移
	 * 实际执行闪现逻辑
	 * 
	 * 步骤：
	 * 1. 计算起点和终点
	 * 2. 碰撞检测（如启用）
	 * 3. 播放特效
	 * 4. 移动角色
	 */
	bool PerformDash(const FVector& DashDirection);

public:
	/**
	 * Z 说明：位移距离
	 * 闪现的距离（单位：厘米）
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDistance = 600.0f;

	/**
	 * Z 说明：冷却时间
	 * 技能冷却时间（秒）
	 * 注意：实际冷却通过 GE 实现，此字段可能用于 Blueprint 显示
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashCooldown = 2.0f;

	/**
	 * Z 说明：冷却效果类
	 * 用于应用冷却的 GameplayEffect
	 * 必须在 Blueprint 中配置
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	TSubclassOf<UGameplayEffect> DashCooldownEffect;

	/**
	 * Z 说明：位移拖尾特效
	 * 闪现过程中显示的粒子效果
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Dash|VFX")
	UNiagaraSystem* DashTrailEffect;

	/**
	 * Z 说明：落地特效
	 * 闪现结束时显示的粒子效果
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Dash|VFX")
	UNiagaraSystem* DashLandedEffect;

	/**
	 * Z 说明：是否检查碰撞
	 * true: 启用碰撞检测，防止穿墙
	 * false: 直接位移，不检查碰撞
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	bool bCheckCollision = true;
};
