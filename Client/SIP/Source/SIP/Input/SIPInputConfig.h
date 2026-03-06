// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * USIPInputConfig 是项目的输入配置数据资产
 * 继承自 UDataAsset，用于存储输入映射配置
 * 
 * 输入配置的作用：
 * 1. 将 InputAction（增强输入系统的输入动作）映射到 GameplayTag
 * 2. 分离输入逻辑和技能系统，便于配置管理
 * 3. 支持在 Blueprint 中配置，无需硬编码
 * 
 * 为什么使用数据资产？
 * - 可以直接在 Editor 中编辑
 * - 可以在 Blueprint 中引用
 * - 易于扩展和维护
 */

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "SIPInputConfig.generated.h"

class UInputAction;
class UObject;
struct FFrame;

/**
 * Z 说明：
 * FSIPInputAction 是单个输入动作的配置结构体
 * 将 InputAction 与 GameplayTag 关联
 * 
 * 使用方式：
 * - NativeInputActions: 需要手动绑定的输入（如移动、视角）
 * - AbilityInputActions: 会自动绑定到技能的输入（如攻击、跳跃）
 */
USTRUCT(BlueprintType)
struct FSIPInputAction
{
	GENERATED_BODY()

public:

	/**
	 * Z 说明：输入动作
	 * 来自 Enhanced Input 系统的 InputAction 资源
	 * 定义了按键/手柄按键
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	/**
	 * Z 说明：输入标签
	 * 用于标识这个输入动作
	 * Ability 通过这个 Tag 来匹配输入
	 * 
	 * 标签层级：
	 * - InputTag.Move: 移动
	 * - InputTag.Jump: 跳跃
	 * - InputTag.Dash: 闪现
	 * - InputTag.Attack: 攻击
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * Z 说明：
 * USIPInputConfig 是输入配置的容器
 * 包含两类输入：
 * 1. NativeInputActions: 需要手动在 C++/Blueprint 中绑定
 * 2. AbilityInputActions: 会自动绑定到 ASC 的技能系统
 */
UCLASS(BlueprintType, Const)
class USIPInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	USIPInputConfig(const FObjectInitializer& ObjectInitializer);

	/**
	 * Z 说明：根据 Tag 查找原生输入动作
	 * 用于手动绑定输入到角色函数
	 * 
	 * @param InputTag - 要查找的输入标签
	 * @param bLogNotFound - 是否在找不到时输出日志
	 * @return 找到的 InputAction，未找到返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pawn")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	/**
	 * Z 说明：根据 Tag 查找技能输入动作
	 * 用于自动绑定输入到技能系统
	 * 
	 * @param InputTag - 要查找的输入标签
	 * @param bLogNotFound - 是否在找不到时输出日志
	 * @return 找到的 InputAction，未找到返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pawn")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

public:
	/**
	 * Z 说明：原生输入动作列表
	 * 这些输入需要在代码中手动绑定到函数
	 * 如：移动、视角控制
	 */
	// List of input actions used by the owner.  These input actions are mapped to a gameplay tag and must be manually bound.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FSIPInputAction> NativeInputActions;

	/**
	 * Z 说明：技能输入动作列表
	 * 这些输入会自动绑定到 ASC 的技能系统
	 * 按下时触发对应 Tag 的技能激活
	 * 
	 * 原理：
	 * 1. 遍历此数组，绑定到 Input_AbilityInputTagPressed/Released
	 * 2. 当按下时，ASC 查找匹配 Tag 的 Ability 并激活
	 */
	//// List of input actions used by the owner.  These input actions are mapped to a gameplay tag and are automatically bound to abilities with matching input tags.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FSIPInputAction> AbilityInputActions;
};
