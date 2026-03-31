// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * USIPGameplayAbility 是项目中所有技能的 C++ 基类
 * 继承自 UGameplayAbility，扩展了技能激活策略
 *
 * 核心扩展：
 * - ActivationPolicy：控制技能何时被 ProcessAbilityInput 激活
 *   - OnInputTriggered：按键按下时激活一次（Attack、Dash、HealPotion）
 *   - WhileInputActive：持续按住期间保持激活（Sprint）
 */

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SIPGameplayAbility.generated.h"

/**
 *
 * OnInputTriggered：
 *   - 只在输入按下瞬间尝试激活一次
 *   - 适用于：攻击、闪现、使用药水等一次性技能
 *
 * WhileInputActive：
 *   - 在输入持续按住期间，每帧尝试激活（技能未激活时）
 *   - 适用于：冲刺、蓄力等持续型技能
 */
UENUM(BlueprintType)
enum class ESIPAbilityActivationPolicy : uint8
{
	// 按键按下时激活一次
	OnInputTriggered  UMETA(DisplayName = "On Input Triggered"),

	// 持续按住时保持激活
	WhileInputActive  UMETA(DisplayName = "While Input Active"),
};

/**
 * USIPGameplayAbility
 * 所有 SIP 技能的基类，提供激活策略配置
 */
UCLASS(Abstract)
class SIP_API USIPGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USIPGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 技能激活策略，由 ProcessAbilityInput 读取判断 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|Ability")
	ESIPAbilityActivationPolicy ActivationPolicy = ESIPAbilityActivationPolicy::OnInputTriggered;
};
