// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
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
#include "GameplayEffectExtension.h"
#include "SIPHealthSet.generated.h"

/**
 * Z 说明：
 * ATTRIBUTE_ACCESSORS 宏 —— Lyra 风格
 * 一次性生成 4 个访问器：
 *   static FGameplayAttribute GetXxxAttribute();  // 属性引用
 *   float GetXxx() const;                          // 读当前值
 *   void  SetXxx(float NewVal);                    // 直接写当前值（走 ASC 内部路径）
 *   void  InitXxx(float NewVal);                   // 初始化 base 值
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Z 说明：
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
	 * Z 说明：获取 AttributeSet 所在的 World
	 * 某些功能需要获取 World（如播放特效、生成物体）
	 */
	UWorld* GetWorld() const override;
	
	/**
	 * Z 说明：获取拥有此 AttributeSet 的 ASC
	 * 用于在 AttributeSet 内部访问 ASC 进行操作
	 */
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

public:

	/**
	 * Z 说明：属性成员声明在前，访问器宏展开在后
	 * 因为 GAMEPLAYATTRIBUTE_PROPERTY_GETTER 内部使用 GET_MEMBER_NAME_CHECKED
	 * 需要成员在展开位置可见
	 */

	// Z 说明：当前生命值（主属性）
	// Primary Attribute
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;

	// Z 说明：最大生命值（主属性）
	// 影响：决定角色死亡线
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;

	// Z 说明：治疗量（临时属性 / meta）
	// 用途：GE 使用，标记即将治疗的血量
	// 特性：会在 PostGameplayEffectExecute 中被消费（转移到 Health 并清零）
	// Clamped between 0 and MaxHealth
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health", ReplicatedUsing = OnRep_Healing)
	FGameplayAttributeData Healing;

	// Z 说明：伤害量（临时属性 / meta，不复制）
	// 用途：伤害 GE 写入本属性，PostGameplayEffectExecute 里将其转移到 Health 上并清零
	// 不需要网络复制（变化结果会体现在 Health 上，Health 本身会复制）
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Health|Meta")
	FGameplayAttributeData Damage;

	/**
	 * Z 说明：使用 ATTRIBUTE_ACCESSORS 一次性生成属性访问器
	 * 每个属性会得到 GetXxxAttribute() / GetXxx() / SetXxx() / InitXxx() 四个函数
	 * 使得 PostGameplayEffectExecute 里能用 GetHealth() / SetHealth() 等语义化 API
	 */
	ATTRIBUTE_ACCESSORS(USIPHealthSet, Health);
	ATTRIBUTE_ACCESSORS(USIPHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(USIPHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(USIPHealthSet, Damage);

	// Z 说明：属性变化时的标签缓存
	// 用于在 GE 回调中识别是哪个属性发生了变化
	// Cache tags
	FGameplayTag Tag_MaxHealthChanged;
	FGameplayTag Tag_HealthChanged;

protected:

	/**
	 * Z 说明：网络复制属性列表
	 * 定义哪些属性需要复制到客户端
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Z 说明：属性基础值改变前的回调
	 * @param Attribute - 改变的属性
	 * @param NewValue - 新的属性值（可以修改）
	 * 
	 * 用途：在属性生效前进行最后修改
	 * 如：护盾减免、伤害上限等
	 */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
	/**
	 * Z 说明：属性改变前的回调
	 * @param Attribute - 改变的属性
	 * @param NewValue - 新的属性值（可以修改）
	 * 
	 * 注意：这个在 PreAttributeBaseChange 之后调用
	 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	/**
	 * Z 说明：属性改变后的回调
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
	 * Z 说明：GE 实际执行（Instant / 周期 Tick）完成后的回调
	 * 用途：将临时的 Damage / Healing 属性转移到 Health，实现「所有伤害/治疗都走 GE」
	 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/**
	 * Z 说明：属性钳制
	 * 确保属性值在合法范围内
	 * 如：生命值不能小于0，不能大于最大生命值
	 */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	/**
	 * Z 说明：网络复制回调
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
