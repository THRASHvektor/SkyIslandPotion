// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * `USIPInputConfig` 是项目的输入配置数据资产。
 * 它把 `InputAction` 和 `GameplayTag` 关联起来，供角色输入绑定和 GAS 输入派发共同使用。
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
 * `FSIPInputAction` 用于描述一条输入动作配置。
 * 它把一个 `InputAction` 和对应的输入标签绑定在一起。
 */
USTRUCT(BlueprintType)
struct FSIPInputAction
{
	GENERATED_BODY()

public:
	/**
	 * Z 说明：输入动作资源。
	 * 来自 Enhanced Input 系统，用于描述键盘、手柄等输入行为。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	/**
	 * Z 说明：输入标签。
	 * 技能系统和原生输入逻辑都会通过这个 Tag 来匹配对应输入。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * Z 说明：
 * 输入配置资产容器，按用途拆成两类输入：
 * 1. `NativeInputActions`：需要在 C++ / Blueprint 中手动绑定的输入。
 * 2. `AbilityInputActions`：会自动进入 ASC 输入流水线的技能输入。
 */
UCLASS(BlueprintType, Const)
class USIPInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	USIPInputConfig(const FObjectInitializer& ObjectInitializer);

	/**
	 * Z 说明：根据 Tag 查找原生输入动作。
	 * 常用于移动、视角等需要手动绑定到角色函数的输入。
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pawn")
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	/**
	 * Z 说明：统一查找输入动作。
	 * 调用方不关心输入属于原生还是技能输入时，可以直接使用这个入口。
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pawn")
	const UInputAction* FindInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

	/**
	 * Z 说明：根据 Tag 查找技能输入动作。
	 * 常用于自动绑定到 ASC 的技能输入链路。
	 */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pawn")
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;

public:
	/**
	 * Z 说明：原生输入动作列表。
	 * 这些输入需要在代码里手动绑定到函数，例如移动、视角控制等。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FSIPInputAction> NativeInputActions;

	/**
	 * Z 说明：技能输入动作列表。
	 * 这些输入会自动绑定到拥有匹配输入标签的能力。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FSIPInputAction> AbilityInputActions;
};
