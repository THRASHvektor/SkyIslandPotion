#pragma once

#include "CoreMinimal.h"
#include "SIPGameplayAbility.h"
#include "SIPGameplayAbility_Attack.generated.h"

class ASIPCharacter;
class UAnimMontage;
class UAnimSequenceBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayEvent;
class USIPHeroAnimationBridgeComponent;

/**
 * Z 说明：
 * SIPGameplayAbility_Attack 负责主角近战攻击能力。
 *
 * 设计目标：
 * 1. 保留 GAS 对输入、Commit、阻塞标签和能力生命周期的控制。
 * 2. 把“何时真正造成伤害”从按键瞬间延后到动画事件或本地回退节点。
 * 3. 在动画桥接组件不可用时，仍然维持一套可用的表现层时序。
 */
UCLASS()
class SIP_API USIPGameplayAbility_Attack : public USIPGameplayAbility
{
	GENERATED_BODY()

public:
	// Z 说明：构造函数，初始化攻击能力的基础标签与实例化策略。
	USIPGameplayAbility_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Z 说明：校验攻击能力是否允许激活，例如角色是否存活。
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

	// Z 说明：激活能力，优先进入动画驱动链路，失败时再回退到旧逻辑。
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	// Z 说明：结束能力时统一清理桥接引用和异步任务。
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// Z 说明：收集攻击范围内的有效目标。
	TArray<ASIPCharacter*> CollectTargets(ASIPCharacter* SourceCharacter) const;

	float GetAttackRangeMultiplier(const ASIPCharacter* SourceCharacter) const;

	// Z 说明：启动动画驱动攻击流程，等待命中窗口事件或延时回退。
	bool StartAnimationDrivenAttack(ASIPCharacter* SourceCharacter);

	// Z 说明：根据当前主角动画原型预设解析本次攻击要播放的动画资源与时序。
	UAnimMontage* ResolveAttackMontageForCharacter(ASIPCharacter* SourceCharacter, float& OutHitWindowStartDelay, float& OutHitWindowEndDelay, float& OutAnimationDuration);

	// Z 说明：旧版即时伤害逻辑，仅作为最后回退使用。
	void ExecuteLegacyAttack(ASIPCharacter* SourceCharacter);

	// Z 说明：收到攻击命中窗口事件后，在此时刻真正结算伤害。
	UFUNCTION()
	void OnAttackHitWindowEvent(FGameplayEventData Payload);

	// Z 说明：没有收到动画事件时，使用本地延时回退触发一次命中窗口。
	UFUNCTION()
	void OnAttackHitWindowFallbackElapsed();

	// Z 说明：攻击动画正常播完时结束能力。
	UFUNCTION()
	void OnAttackAnimationCompleted();

	// Z 说明：攻击动画被打断或取消时结束能力。
	UFUNCTION()
	void OnAttackAnimationInterrupted();

	// Z 说明：没有蒙太奇时，使用固定时长回退结束能力。
	UFUNCTION()
	void OnAttackFallbackDurationElapsed();

protected:
	// Z 说明：攻击造成的基础伤害。
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float DamageAmount = 25.0f;

	// Z 说明：攻击检测中心点距离角色前方的偏移。
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRange = 180.0f;

	// Z 说明：攻击碰撞球半径。
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Ice")
	bool bEnableIceMomentumAttack = true;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Ice", meta = (ClampMin = "0.0"))
	float IceMomentumMinSpeed = 220.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Ice", meta = (ClampMin = "1.0"))
	float IceMomentumAttackRangeMultiplier = 1.35f;

	// Z 说明：攻击表现使用的蒙太奇，可为空。
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Z 说明：运行时由原型动画构建动态蒙太奇时使用的插槽名。
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	FName AttackMontageSlotName = TEXT("UpperBody");

	// Z 说明：当主角处于支持的动画原型预设时，是否优先使用原型自带攻击序列构建动态蒙太奇。
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	bool bPreferPrototypeAttackAnimation = true;

	// Z 说明：命中窗口开始延时；没有 Notify 时也会作为本地回退时机。
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation", meta = (ClampMin = "0.0"))
	float AttackHitWindowStartDelay = 0.15f;

	// Z 说明：命中窗口结束延时，主要用于桥接组件同步战斗状态标签。
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation", meta = (ClampMin = "0.0"))
	float AttackHitWindowEndDelay = 0.35f;

	// Z 说明：攻击总时长回退值，用于没有蒙太奇时控制能力结束时机。
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation", meta = (ClampMin = "0.05"))
	float AttackAnimationDuration = 0.6f;

private:
	// Z 说明：等待攻击命中窗口 Gameplay Event 的异步任务。
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttackHitWindowTask;

	// Z 说明：攻击命中窗口的本地延时回退任务。
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> AttackHitFallbackTask;

	// Z 说明：攻击蒙太奇播放任务。
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> AttackMontageTask;

	// Z 说明：由原型攻击序列在运行时构建的动态蒙太奇，生命周期跟随本次 Ability 执行。
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> RuntimeAttackMontage;

	// Z 说明：没有蒙太奇时用于结束能力的固定时长任务。
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> AttackDurationTask;

	// Z 说明：当前生效的动画桥接组件，用于同步事件和状态。
	TWeakObjectPtr<USIPHeroAnimationBridgeComponent> ActiveAnimationBridge;

	// Z 说明：防止一次攻击在同一轮能力生命周期中重复结算伤害。
	bool bHasAppliedAttackHit = false;
};
