// Copyright Epic Games, Inc. All Rights Reserved.
/**
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
struct FGameplayAbilitySpec;
struct FTimerHandle;

/**
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
	 * 返回角色的 AbilitySystemComponent
	 * 
	 * 重要：这是 GAS 系统的核心接口
	 * 各种 GAS 功能都依赖于通过此接口获取 ASC
	 */
	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * 返回 USIPAbilitySystemComponent 类型
	 * 提供比基类更具体的功能
	 */
	USIPAbilitySystemComponent* GetSIPAbilitySystemComponent() const;

	/**
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
	 * 当角色死亡时调用
	 * 可以在这里处理：停止移动、播放死亡动画、销毁等
	 */
	// 新增：死亡处理回调
	virtual void OnDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Death", DisplayName = "On Death")
	void K2_OnDeath();
	
	/**
	 * 在死亡动画开始时调用
	 */
	virtual void OnDeathStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Death", DisplayName = "On Death Started")
	void K2_OnDeathStarted();
	
	/**
	 * 在死亡动画结束或角色复活时调用
	 */
	virtual void OnDeathStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Death", DisplayName = "On Death Stopped")
	void K2_OnDeathStopped();

protected:

	/**
	 * 角色开始游戏时调用
	 * 这里可以添加初始逻辑
	 */
	// To add mapping context
	virtual void BeginPlay();

	/**
	 * 在所有组件初始化完成后调用
	 * 
	 * 重要：这里是我们初始化 ASC 和授予技能的时机
	 * 因为此时所有组件都已经创建完成
	 */
	// 在这里初始化组件
	virtual void PostInitializeComponents() override;

	// Actor 销毁时清理授予的技能/属性/效果
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void StartDeathDissolve();
	void UpdateDeathDissolve();
	void FinishDeathDissolve();
	
protected:
	/**
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Death|Dissolve", meta = (AllowPrivateAccess = "true"))
	bool bUseDeathDissolve = true;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|State", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DeathDissolveMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMeshComponent>> DeathDissolveMeshComponents;

	float DeathDissolveElapsedTime = 0.0f;
	FTimerHandle DeathDissolveTimerHandle;
};
