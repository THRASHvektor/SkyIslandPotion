#pragma once

#include "Combat/SIPCombatSemanticResolver.h"
#include "CoreMinimal.h"
#include "SIPGameplayAbility.h"
#include "SIPGameplayAbility_Attack.generated.h"

class ASIPCharacter;
class ASIPHeroCharacter;
class UAnimMontage;
class UAnimSequenceBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayEvent;
class USIPAttackComboDataAsset;
class USIPHeroAnimationBridgeComponent;

/**
 * 一条可被运行时选中的连招候选条目。
 *
 * 这条数据同时服务于三层语义：
 * 1. 资源侧作者手工配置的连招定义。
 * 2. 运行时通过 ComboIndex 维护的连段记忆。
 * 3. 第一版 Ice Rune Dagger 语义解析器输出的动作家族筛选。
 *
 * 也就是说，它既可以按“原始移动条件”命中，
 * 也可以按“语义战斗条件”命中，
 * 还可以两者同时要求。
 */
USTRUCT(BlueprintType)
struct FSIPAttackComboEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	FName EntryId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0"))
	int32 ComboIndex = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 NextComboIndex = INDEX_NONE;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (Categories = "State.Combat.WeaponModule"))
	FGameplayTag WeaponModuleTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TSoftObjectPtr<UAnimSequenceBase> Animation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	FName SlotName = TEXT("FullBody_Combat");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float HitWindowStartDelay = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float HitWindowEndDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	bool bRequireIceSurface = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.0"))
	float MinGroundSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	float MaxGroundSpeed = -1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MinAbsTurnAngleDegrees = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	float MaxAbsTurnAngleDegrees = -1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Semantic", meta = (Categories = "State.Combat.ActionFamily"))
	FGameplayTag RequiredActionFamilyTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Semantic", meta = (Categories = "State.Combat.BodyState"))
	FGameplayTag RequiredBodyStateTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo|Semantic")
	FName RequiredVariant = NAME_None;

	// 0 = 任意方向，-1 = 右转，1 = 左转
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 RequiredTurnSign = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 Priority = 0;
};

/**
 * 当前主角近战攻击能力的核心实现。
 *
 * 当前职责：
 * 1. 通过 GAS 接收攻击输入。
 * 2. 解析当前应该命中的连招条目。
 * 3. 向动画桥接层请求本次攻击的表现时序。
 * 4. 维护缓冲输入，让下一段连击能顺着接上。
 *
 * 这个类现在仍然承担了偏多的“表现层责任”，
 * 长期看会继续拆分；
 * 但在当前阶段，它仍然是玩法、时序和语义连招选择相交的主入口。
 */
UCLASS()
class SIP_API USIPGameplayAbility_Attack : public USIPGameplayAbility
{
	GENERATED_BODY()

public:
	USIPGameplayAbility_Attack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/**
	 * 只允许有效且存活的角色激活攻击能力。
	 */
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	/**
	 * 启动一次攻击。
	 *
	 * 顺序是：
	 * 1. Commit GAS 状态。
	 * 2. 优先走动画驱动的攻击链。
	 * 3. 如果动画链无法建立，再回退到旧的即时攻击路径。
	 */
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/**
	 * 结束攻击能力并清理所有异步任务。
	 *
	 * 无论是正常结束还是取消，都要确保动画桥接层退出到一致状态，
	 * 避免把上一段攻击的时序残留到下一段。
	 */
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	/**
	 * 收集当前命中窗口内可被近战命中的目标。
	 */
	TArray<ASIPCharacter*> CollectTargets(ASIPCharacter* SourceCharacter) const;

	/**
	 * 当攻击属于冰面高动量打击时，扩展有效攻击距离。
	 */
	float GetAttackRangeMultiplier(const ASIPCharacter* SourceCharacter) const;

	/**
	 * 为本次攻击挂载完整的表现层执行链。
	 *
	 * 包括：
	 * 1. Gameplay Event 监听。
	 * 2. 动画桥接层时序。
	 * 3. 蒙太奇播放或本地回退计时器。
	 */
	bool StartAnimationDrivenAttack(ASIPCharacter* SourceCharacter);

	/**
	 * 为本次攻击解析实际播放的动态蒙太奇包装，
	 * 并返回对应的命中窗口与时长信息。
	 */
	UAnimMontage* ResolveAttackMontageForCharacter(
		ASIPCharacter* SourceCharacter,
		float& OutHitWindowStartDelay,
		float& OutHitWindowEndDelay,
		float& OutAnimationDuration);

	/**
	 * 把资源里请求的 Slot 名称翻译成当前 AnimBP 真正能消费的 Slot。
	 */
	FName ResolvePlayableSlotName(FName RequestedSlotName) const;

	/**
	 * 决定本次攻击应该以哪个武器模块语义对外表现。
	 */
	FGameplayTag ResolveWeaponModuleTagForCharacter(const ASIPCharacter* SourceCharacter) const;

	/**
	 * 向共享战斗语义层请求当前动作描述符，
	 * 同时输出解析时实际使用的施法阶段标签。
	 */
	FSIPCombatActionDescriptor ResolveCombatDescriptorForCharacter(ASIPHeroCharacter* HeroCharacter, const FGameplayTag& ResolvedWeaponModuleTag, FGameplayTag& OutResolvedCastPhaseTag) const;

	/**
	 * 根据当前 ComboIndex 和战斗上下文，
	 * 选出最合适的一条连招配置。
	 */
	const FSIPAttackComboEntry* ResolveComboEntryForCharacter(
		ASIPHeroCharacter* HeroCharacter,
		const FGameplayTag& ResolvedWeaponModuleTag,
		const FSIPCombatActionDescriptor& CombatDescriptor) const;

	/**
	 * 检查某一条连招配置是否适用于当前运行时上下文。
	 */
	bool DoesComboEntryMatchContext(
		const FSIPAttackComboEntry& Entry,
		const ASIPHeroCharacter* HeroCharacter,
		const FGameplayTag& ResolvedWeaponModuleTag,
		int32 ComboIndex,
		const FSIPCombatActionDescriptor& CombatDescriptor) const;

	/**
	 * 动画驱动路径无法启动时的最终回退攻击逻辑。
	 */
	void ExecuteLegacyAttack(ASIPCharacter* SourceCharacter);

	UFUNCTION()
	/**
	 * 命中窗口打开时，真正执行一次攻击结算。
	 */
	void OnAttackHitWindowEvent(FGameplayEventData Payload);

	UFUNCTION()
	/**
	 * 结束当前命中窗口，并在需要时把缓冲输入交给下一段攻击。
	 */
	void OnAttackHitWindowEndEvent(FGameplayEventData Payload);

	UFUNCTION()
	/**
	 * 命中窗口开启事件的本地计时回退。
	 */
	void OnAttackHitWindowFallbackElapsed();

	UFUNCTION()
	/**
	 * 处理蒙太奇自然结束或本地时长回退结束。
	 */
	void OnAttackAnimationCompleted();

	UFUNCTION()
	/**
	 * 处理中断，并防止桥接层残留过期战斗状态。
	 */
	void OnAttackAnimationInterrupted();

	UFUNCTION()
	/**
	 * 蒙太奇 BlendOut 开始时触发，提前通知桥接层退出战斗状态，
	 * 让 PostAttackMMSuppression grace 与 BlendOut 并行运行。
	 */
	void OnAttackAnimationBlendingOut();

	UFUNCTION()
	/**
	 * 没有激活蒙太奇任务时使用的本地时长回退。
	 */
	void OnAttackFallbackDurationElapsed();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float DamageAmount = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRange = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float AttackRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Ice")
	bool bEnableIceMomentumAttack = true;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Ice", meta = (ClampMin = "0.0"))
	float IceMomentumMinSpeed = 220.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Ice", meta = (ClampMin = "1.0"))
	float IceMomentumAttackRangeMultiplier = 1.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	FName AttackMontageSlotName = TEXT("FullBody_Combat");

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation")
	bool bPreferPrototypeAttackAnimation = true;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation", meta = (Categories = "State.Combat.WeaponModule"))
	FGameplayTag WeaponModuleTag;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation", meta = (ClampMin = "0.0"))
	float AttackHitWindowStartDelay = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation", meta = (ClampMin = "0.0"))
	float AttackHitWindowEndDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Animation", meta = (ClampMin = "0.05"))
	float AttackAnimationDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo")
	TObjectPtr<USIPAttackComboDataAsset> AttackComboDataAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo", meta = (ClampMin = "0.05"))
	float ComboResetWindowSeconds = 2.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo", meta = (ClampMin = "0.0"))
	float BufferedComboInputWindowSeconds = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Combo")
	TArray<FSIPAttackComboEntry> ComboEntries;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Debug")
	bool bDebugComboFlow = true;

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Debug")
	bool bDebugComboOnScreen = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttackHitWindowTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> AttackHitWindowEndTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> AttackHitFallbackTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> AttackMontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> RuntimeAttackMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> AttackDurationTask;

	TWeakObjectPtr<USIPHeroAnimationBridgeComponent> ActiveAnimationBridge;

	FSIPCombatActionDescriptor ResolvedCombatDescriptorForCurrentAttack;
	FGameplayTag ResolvedCastPhaseForCurrentAttack;

	bool bHasAppliedAttackHit = false;
	bool bAnimationBridgeAttackFinalized = false;
};
