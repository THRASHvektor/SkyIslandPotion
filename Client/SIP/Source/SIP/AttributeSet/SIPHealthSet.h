// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * USIPHealthSet 是角色的生命值属性集
 * 继承自 UAttributeSet，是 GAS 中用于存储和管理角色属性的核心类
 * 
 * AttributeSet 的作用：
 * 1. 存储角色的各种属性（生命值、最大生命值、移动速度等）
 * 2. 提供网络复制支持，多人游戏时同步属性给客户端
 * 3. 提供属性变化回调，可以在属性变化时执行逻辑
 * 
 * 典型的 AttributeSet：
 * - HealthSet（生命值）
 * - ManaSet（魔法值）
 * - StaminaSet（体力）
 * - DamageSet（伤害计算用）
 */

#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "SIPHealthSet.generated.h"

/**
 * USIPHealthSet 定义了角色的生命值相关属性
 * 包括：当前生命值、最大生命值、治疗量、移动速度
 * 
 * 使用方式：
 * 1. 通过 AbilitySet 授权给角色
 * 2. 角色获取 ASC 后，可以通过 GetSIPHealthSet() 获取实例
 * 3. 使用 GE 修改属性时，会自动触发网络复制和回调
 */
UCLASS()
class USIPHealthSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	USIPHealthSet();

	/**
	 * 某些功能需要获取 World（如播放特效、生成物体）
	 */
	UWorld* GetWorld() const override;
	
	/**
	 * 用于在 AttributeSet 内部访问 ASC 进行操作
	 */
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

public:

	/**
	 * 这些宏会自动生成 GetHealth()、GetMaxHealth() 等函数
	 * 方便在其他代码中获取属性值
	 */
	// 使用 GAS 宏定义 Getter 函数
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPHealthSet, Health);
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPHealthSet, MaxHealth);
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPHealthSet, Healing);

	// Primary Attribute
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;

	// 影响：决定角色死亡线
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;

	// 用途：GE 使用，标记即将治疗的血量
	// 特性：会在 PostAttributeChange 中被消费（减为0）
	// Clamped between 0 and MaxHealth
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_Healing)
	FGameplayAttributeData Healing;

	// 用于在 GE 回调中识别是哪个属性发生了变化
	// Cache tags
	FGameplayTag Tag_MaxHealthChanged;
	FGameplayTag Tag_HealthChanged;

protected:

	/**
	 * 定义哪些属性需要复制到客户端
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @param Attribute - 改变的属性
	 * @param NewValue - 新的属性值（可以修改）
	 * 
	 * 用途：在属性生效前进行最后修改
	 * 如：护盾减免、伤害上限等
	 */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
	/**
	 * @param Attribute - 改变的属性
	 * @param NewValue - 新的属性值（可以修改）
	 * 
	 * 注意：这个在 PreAttributeBaseChange 之后调用
	 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	/**
	 * @param Attribute - 改变的属性
	 * @param OldValue - 旧值
	 * @param NewValue - 新值
	 * 
	 * 用途：
	 * 1. 播放受伤/治疗特效
	 * 2. 检查死亡条件
	 * 3. 更新 UI
	 */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/**
	 * 确保属性值在合法范围内
	 * 如：生命值不能小于0，不能大于最大生命值
	 */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	/**
	 * 当客户端收到服务器复制的属性值时调用
	 * 用于在客户端更新 UI 等
	 */
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Healing(const FGameplayAttributeData& OldHealing);
};
