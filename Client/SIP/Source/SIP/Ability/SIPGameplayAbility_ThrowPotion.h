// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * Z 说明：
 * USIPGameplayAbility_ThrowPotion 是投掷药水的 GAS 技能
 *
 * 激活流程：
 * 1. CommitAbility（消耗资源/应用 CD）
 * 2. 计算投掷方向（摄像机前方 + 向上 30° 弧度）
 * 3. 在角色手部 Socket 位置 Spawn SIPPotionProjectile
 * 4. 同步设置弹丸的 ElementTag 和初速度
 * 5. EndAbility
 *
 * 配置要点（在 BP 子类中设置）：
 * - ProjectileClass：对应元素的弹丸 BP 子类
 * - PotionElementTag：此技能投掷的元素类型
 * - ThrowSpeed：初速度（默认 1200）
 * - 冷却通过 GE_Cooldown 资产实现（CooldownGameplayEffectClass + CooldownTags）
 */

#pragma once

#include "CoreMinimal.h"
#include "SIPGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SIPGameplayAbility_ThrowPotion.generated.h"

class ASIPPotionProjectile;

UCLASS()
class SIP_API USIPGameplayAbility_ThrowPotion : public USIPGameplayAbility
{
	GENERATED_BODY()

public:
	USIPGameplayAbility_ThrowPotion(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags
	) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

public:
	/** 要生成的弹丸类（每种元素对应一个 BP 子类） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	TSubclassOf<ASIPPotionProjectile> ProjectileClass;

	/** 此技能投掷的元素类型（Element.Fire / Element.Ice 等） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion", meta = (Categories = "Element"))
	FGameplayTag PotionElementTag;

	/** 弹丸初速度（cm/s） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	float ThrowSpeed = 1200.f;

	/**
	 * 抬头角度（度）
	 * 投掷方向在摄像机朝向基础上向上偏转此角度，形成抛物线
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	float LaunchPitchOffset = 20.f;

	/** 手部 Socket 名称（弹丸生成位置） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	FName HandSocketName = TEXT("hand_r");
};
