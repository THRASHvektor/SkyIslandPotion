// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SIPGameplayAbility.h"
#include "SIPGameplayAbility_ThrowPotion.generated.h"

class ASIPCharacter;
class ASIPPotionProjectile;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class USIPHeroAnimationBridgeComponent;

/**
 * 该能力仍然把激活、Commit 和冷却交给 GAS 管理，
 * 但真正的抛掷释放时机改为由动画层事件或本地回退时序来控制。
 */
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

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	// 启动动画驱动的投掷流程，同时挂好事件监听和时间回退。
	bool StartAnimationDrivenThrow(ASIPCharacter* SourceCharacter);

	// 当当前主角预设支持原型动画时，解析运行时动画资源和时序参数。
	UAnimMontage* ResolveThrowMontageForCharacter(ASIPCharacter* SourceCharacter, float& OutReleaseDelay, float& OutAnimationDuration);

	// 动画驱动链路完全无法启动时的最终保底逻辑。
	void ExecuteLegacyThrow(ASIPCharacter* SourceCharacter);

	// 按角色手部插槽、相机朝向和控制器方向生成真正的药瓶投射物。
	void SpawnPotionProjectile(ASIPCharacter* SourceCharacter) const;

	UFUNCTION()
	void OnThrowReleaseEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnThrowReleaseFallbackElapsed();

	UFUNCTION()
	void OnThrowAnimationCompleted();

	UFUNCTION()
	void OnThrowAnimationInterrupted();

	UFUNCTION()
	void OnThrowFallbackDurationElapsed();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	TSubclassOf<ASIPPotionProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion", meta = (Categories = "Element"))
	FGameplayTag PotionElementTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	float ThrowSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	float LaunchPitchOffset = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion")
	FName HandSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion|Animation")
	TObjectPtr<UAnimMontage> ThrowMontage;

	// 运行时根据原型动画构建投掷蒙太奇时使用的插槽名。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion|Animation")
	FName ThrowMontageSlotName = TEXT("UpperBody_Cast");

	// 当预设可用时，优先使用原型投掷/施法动画而不是手工指定的蒙太奇。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion|Animation")
	bool bPreferPrototypeThrowAnimation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion|Animation", meta = (Categories = "State.Combat.WeaponModule"))
	FGameplayTag WeaponModuleTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion|Animation", meta = (ClampMin = "0.0"))
	float ThrowReleaseDelay = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SIP|ThrowPotion|Animation", meta = (ClampMin = "0.05"))
	float ThrowAnimationDuration = 0.6f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ThrowReleaseTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> ThrowReleaseFallbackTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ThrowMontageTask;

	// 本次能力执行期间，由原型动画序列动态构建出来的运行时蒙太奇。
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> RuntimeThrowMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> ThrowDurationTask;

	TWeakObjectPtr<USIPHeroAnimationBridgeComponent> ActiveAnimationBridge;

	bool bHasSpawnedProjectile = false;
};
