// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * ASIPCharacter 是项目中的角色基类
 * 继承自 ACharacter（UE 的角色基类），并实现了 IAbilitySystemInterface
 * 
 * 为什么要继承 ACharacter？
 * 1. ACharacter 已经包含了角色移动、胶囊体碰撞等基础功能
 * 2. 可以直接使用 CharacterMovementComponent 进行角色控制
 * 3. 继承 AActor 的生命周期和网络复制功能
 * 
 * IAbilitySystemInterface 接口：
 * 让角色可以与 GAS 系统对接
 * 其他系统可以通过 GetAbilitySystemComponent() 获取 ASC
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "Ability/SIPAbilitySet.h"
#include "SIPCharacter.generated.h"

class UAbilitySystemComponent;
class USIPAbilitySystemComponent;
class UAttributeSet;
class USIPHealthSet;
class AActor;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UAnimMontage;
class UAnimInstance;
struct FGameplayAbilitySpec;
struct FTimerHandle;

/**
 * Z 说明：
 * ASIPCharacter 是所有可交互游戏角色的基类
 * 
 * 主要功能：
 * 1. 挂载 AbilitySystemComponent（ASC）实现技能系统
 * 2. 通过 AbilitySets 数组配置角色拥有的技能
 * 3. 提供死亡回调接口
 * 
 * 继承层次：
 * AActor → APawn → ACharacter → ASIPCharacter → ASIPHeroCharacter
 */
UCLASS()
class ASIPCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASIPCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Z 说明：实现 IAbilitySystemInterface 接口
	 * 返回角色的 AbilitySystemComponent
	 * 
	 * 重要：这是 GAS 系统的核心接口
	 * 各种 GAS 功能都依赖于通过此接口获取 ASC
	 */
	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * Z 说明：获取项目自定义的 ASC
	 * 返回 USIPAbilitySystemComponent 类型
	 * 提供比基类更具体的功能
	 */
	USIPAbilitySystemComponent* GetSIPAbilitySystemComponent() const;

	/**
	 * Z 说明：获取角色的生命值属性集
	 * 返回 USIPHealthSet 实例
	 * 用于读取/修改角色的生命值
	 * 
	 * 使用场景：
	 * - UI 显示血量
	 * - 技能读取角色血量
	 * - 死亡判断
	 */
	// 新增：获取Character的Health属性集
	USIPHealthSet* GetSIPHealthSet() const;

	float GetCurrentHealth() const;
	float GetMaxHealth() const;
	bool IsDeadOrDying() const;
	bool ApplyCombatDamage(float DamageAmount, AActor* DamageInstigator = nullptr);
	bool RestoreHealth(float HealAmount);
	void HandleOutOfHealth();
	void HandleRevived();

	/**
	 * Z 说明：角色的技能集列表
	 * 每个 AbilitySet 包含一组技能、属性、被动效果
	 * 在 PostInitializeComponents 中会被授予给角色
	 * 
	 * 使用方式：
	 * 在 Blueprint 中配置此数组
	 * 每个角色可以有不同的技能配置
	 */
	// 用于赋予角色ability的列表，映射Inputtag和ability
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Abilities")
	TArray<TObjectPtr<USIPAbilitySet>> AbilitySets;

	/**
	 * Z 说明：死亡回调函数
	 * 当角色死亡时调用
	 * 可以在这里处理：停止移动、播放死亡动画、销毁等
	 */
	// 新增：死亡处理回调
	virtual void OnDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Death", DisplayName = "On Death")
	void K2_OnDeath();
	
	/**
	 * Z 说明：开始死亡回调
	 * 在死亡动画开始时调用
	 */
	virtual void OnDeathStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Death", DisplayName = "On Death Started")
	void K2_OnDeathStarted();
	
	/**
	 * Z 说明：死亡结束（复活）回调
	 * 在死亡动画结束或角色复活时调用
	 */
	virtual void OnDeathStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Death", DisplayName = "On Death Stopped")
	void K2_OnDeathStopped();

protected:

	/**
	 * Z 说明：BeginPlay 是 UE 的生命周期函数
	 * 角色开始游戏时调用
	 * 这里可以添加初始逻辑
	 */
	// To add mapping context
	virtual void BeginPlay();

	/**
	 * Z 说明：PostInitializeComponents 是 UE 的生命周期函数
	 * 在所有组件初始化完成后调用
	 * 
	 * 重要：这里是我们初始化 ASC 和授予技能的时机
	 * 因为此时所有组件都已经创建完成
	 */
	// 在这里初始化组件
	virtual void PostInitializeComponents() override;

	// Actor 销毁时清理授予的技能/属性/效果
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Z 说明：角色掉出世界（Z 轴低于 WorldSettings.KillZ）时的回调。
	 * 引擎默认会走 TakeDamage 通道，但项目的战斗伤害必须走 GAS GE 流程，
	 * 因此这里覆盖为通过 USIPCombatStatics::ApplyDamageToTarget 施加一次性大伤害，
	 * 让 HealthSet 属性变化自然触发标准死亡流程（HandleOutOfHealth）。
	 */
	virtual void FellOutOfWorld(const class UDamageType& DmgType) override;

	void StartDeathDissolve();
	void UpdateDeathDissolve();
	void FinishDeathDissolve();

	/**
	 * Z 说明：播放配置好的死亡蒙太奇（若配置了）。
	 * 返回蒙太奇长度（秒）。返回 <=0 表示未播放（未配置或 AnimInstance 缺失）。
	 * 内部会绑定一次性回调，播放结束（Ended/Interrupted）时触发溶解流程。
	 */
	float PlayDeathMontage();

	/** Z 说明：死亡蒙太奇播放结束回调。用于在动画播完后再启动溶解。 */
	UFUNCTION()
	void OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	
protected:
	/**
	 * Z 说明：AbilitySystemComponent 组件
	 * 这是 GAS 的核心组件
	 * 负责管理所有技能和属性
	 * 
	 * 组件特点：
	 * - VisibleAnywhere: 在编辑器中可见
	 * - BlueprintReadOnly: Blueprint 中只读
	 */
	/** Ability System */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<USIPAbilitySystemComponent> AbilitySystemComponent;

	// 记录通过 AbilitySets 授予的所有句柄，用于 EndPlay 时清理
	FSIPAbilitySet_GrantedHandles AbilitySetHandles;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Health", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float DefaultMaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DefaultStartingHealth = 100.0f;

	/**
	 * Z 说明：角色掉出世界（低于 KillZ）时通过 GAS GE 施加的伤害值。
	 * 默认给一个足够大的值以确保任何血量配置的角色都会因此死亡；
	 * 如需保留"跌落但不必致死"的语义，可在派生类或 BP 中调低此值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float FellOutOfWorldDamage = 999999.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Dissolve", meta = (AllowPrivateAccess = "true"))
	bool bUseDeathDissolve = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Dissolve", meta = (AllowPrivateAccess = "true", ClampMin = "0.05"))
	float DeathDissolveDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Dissolve", meta = (AllowPrivateAccess = "true"))
	FName DeathDissolveParameterName = TEXT("DissolveAmount");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Dissolve", meta = (AllowPrivateAccess = "true"))
	float DeathDissolveStartValue = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Dissolve", meta = (AllowPrivateAccess = "true"))
	float DeathDissolveEndValue = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Dissolve", meta = (AllowPrivateAccess = "true"))
	bool bDestroyActorOnDissolveComplete = false;

	/**
	 * Z 说明：死亡时播放的动画蒙太奇（在 Mesh 的 AnimInstance 上播放）。
	 * 蓝图角色只需拖入一个 UAnimMontage 资源即可。为空则跳过动画播放，行为与旧版一致。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DeathMontage;

	/** Z 说明：死亡蒙太奇播放速率倍数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Animation", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float DeathMontagePlayRate = 1.0f;

	/**
	 * Z 说明：是否等死亡蒙太奇播放结束后再启动溶解。
	 * true  = 先播完动画 → 再溶解（推荐，动画完整表现）
	 * false = 动画与溶解同时进行（保持旧行为）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Animation", meta = (AllowPrivateAccess = "true"))
	bool bWaitDeathMontageBeforeDissolve = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|State", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	/**
	 * Z 说明：本次生命周期是否已处理过 FellOutOfWorld。
	 * 引擎的 KillZ 每帧检查，一旦角色 Z < KillZ 会持续调用 FellOutOfWorld，
	 * 所以需要一次性保护，防止重复施加伤害 GE / 重复日志刷屏。复活时（HandleRevived）应重置。
	 */
	UPROPERTY(Transient)
	bool bHasFellOutOfWorld = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DeathDissolveMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> DeathDissolveMeshComponents;

	float DeathDissolveElapsedTime = 0.0f;
	FTimerHandle DeathDissolveTimerHandle;
};
