#include "SIPGameplayAbility_Attack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Character/Components/SIPHeroAnimationBridgeComponent.h"
#include "Character/SIPCharacter.h"
#include "Character/SIPHeroCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/SIPAttackComboDataAsset.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

namespace
{
	/**
	 * 运行时原型攻击片段的轻量描述。
	 *
	 * 这些片段不会直接裸播，
	 * 而是会在运行时被包成动态蒙太奇后再交给能力系统使用。
	 */
	struct FPrototypeAttackAnimationSpec
	{
		const TCHAR* AnimationAssetPath = nullptr;
		float HitWindowStartDelay = 0.0f;
		float HitWindowEndDelay = 0.0f;
	};

	/**
	 * 攻击能力内部统一使用的调试输出。
	 *
	 * 这里故意同时打日志和屏幕提示，
	 * 因为连招调试本质上大多是“时序问题”，
	 * 而时序问题在动作还没播完时最容易观察。
	 */
	void EmitAttackDebug(UObject* ContextObject, const FString& Message, const bool bOnScreen)
	{
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("%s"), *Message);

		if (!bOnScreen || !GEngine)
		{
			return;
		}

		const uint64 MessageKey = uint64(uintptr_t(ContextObject)) + 1001ull;
		GEngine->AddOnScreenDebugMessage(MessageKey, 1.6f, FColor::Cyan, Message);
	}

	/**
	 * 取得内建的 Rune Dagger 默认连招列表。
	 *
	 * 当资源侧连招数据缺失，
	 * 或者虽然数组存在但结构体实际上是空壳时，
	 * 运行时就会退回这份默认列表。
	 */
	const TArray<FSIPAttackComboEntry>& GetDefaultAttackComboEntries()
	{
		return USIPAttackComboDataAsset::BuildRuneDaggerAttackComboEntriesV2();
	}

	/**
	 * 计算角色面朝方向与当前移动方向的有符号夹角。
	 *
	 * 负值表示需要向右修正。
	 * 正值表示需要向左修正。
	 */
	float GetSignedTurnAngleDegrees(const ASIPCharacter* SourceCharacter)
	{
		if (!SourceCharacter || SourceCharacter->GetVelocity().SizeSquared2D() <= KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}

		return SIPCombatSemantic::GetSignedTurnAngleDegrees(SourceCharacter->GetActorForwardVector(), SourceCharacter->GetVelocity());
	}

	/**
	 * 给旧动画原型预设保留的非 RuneDagger 原型动画选择路径。
	 *
	 * 当前主要是兼容历史导入的 demo 动画内容。
	 */
	const FPrototypeAttackAnimationSpec* GetPrototypeAttackAnimationSpec(const ASIPCharacter* SourceCharacter)
	{
		const ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(SourceCharacter);
		if (!HeroCharacter)
		{
			return nullptr;
		}

		switch (HeroCharacter->GetHeroAnimationPrototypePreset())
		{
		case ESIPHeroAnimationPrototypePreset::CombatMagicUnarmed:
		{
			static const FPrototypeAttackAnimationSpec CombatMagicUnarmedAttack01
			{
				TEXT("/Game/CombatMagicAnims/Demo/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"),
				0.30f,
				0.58f
			};
			return &CombatMagicUnarmedAttack01;
		}
		default:
			return nullptr;
		}
	}
}

/**
 * 构造函数只负责建立这项攻击能力在 GAS 里的默认身份。
 *
 * 真正重要的运行时决策都要等到激活时再做，
 * 因为 ComboIndex、动量、语义状态以及桥接层是否可用，全都是动态的。
 */
USIPGameplayAbility_Attack::USIPGameplayAbility_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	AbilityTags.AddTag(SIPGameplayTags::InputTag_Attack);
	ActivationBlockedTags.AddTag(SIPGameplayTags::State_Dead);
	WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_Unarmed;
}

/**
 * 在挂任何异步任务之前，先拒绝无效或死亡角色的攻击激活请求。
 */
bool USIPGameplayAbility_Attack::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	return SourceCharacter && !SourceCharacter->IsDeadOrDying();
}

/**
 * 单次攻击按键进入能力后的主入口。
 *
 * 流程：
 * 1. 解析当前武器模块和连招状态。
 * 2. Commit GAS 能力。
 * 3. 清掉这次真正触发能力的按键缓存，避免它被误当成后续追输入。
 * 4. 优先走动画驱动的攻击路径。
 * 5. 如果表现层链路建立失败，再回退到旧的即时攻击逻辑。
 */
void USIPGameplayAbility_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(SourceCharacter);
	const FGameplayTag ResolvedWeaponModuleTag = ResolveWeaponModuleTagForCharacter(SourceCharacter);
	if (!SourceCharacter || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasAppliedAttackHit = false;
	bAnimationBridgeAttackFinalized = false;

	if (HeroCharacter)
	{
		HeroCharacter->ClearBufferedAttackInput();
		if (bDebugComboFlow)
		{
			// 注意：GetResolvedAttackComboIndex 有副作用（超时时会重置 ComboIndex），
			// 必须使用与实际连招解析相同的窗口值，避免 debug 路径误重置状态。
			const float ActiveResetWindow =
				AttackComboDataAsset
					? AttackComboDataAsset->ComboResetWindowSeconds
					: ComboResetWindowSeconds;
			const int32 RequestedComboIndex = HeroCharacter->GetResolvedAttackComboIndex(ActiveResetWindow);
			EmitAttackDebug(
				this,
				FString::Printf(
					TEXT("[Attack] Pressed Module=%s ComboIndex=%d Speed=%.1f Ice=%s"),
					*ResolvedWeaponModuleTag.ToString(),
					RequestedComboIndex,
					SourceCharacter->GetVelocity().Size2D(),
					HeroCharacter->IsOnIceSurface() ? TEXT("Y") : TEXT("N")),
				bDebugComboOnScreen);
		}
	}

	if (StartAnimationDrivenAttack(SourceCharacter))
	{
		return;
	}

	ExecuteLegacyAttack(SourceCharacter);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

/**
 * 所有退出路径共用的收尾逻辑。
 *
 * 这里最微妙的点在于：
 * 动画桥接层面对“正常结束”“缓冲连招交接”“中断取消”三种情况的处理并不相同。
 *
 * `bAnimationBridgeAttackFinalized` 的作用，
 * 是避免某条退出路径已经收尾过桥接层之后，
 * 另一条路径又重复 Finish 或 Cancel 一次。
 */
void USIPGameplayAbility_Attack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActiveAnimationBridge.IsValid())
	{
		if (!bAnimationBridgeAttackFinalized)
		{
			if (bWasCancelled)
			{
				ActiveAnimationBridge->CancelAttackAnimation();
			}
			else
			{
				ActiveAnimationBridge->FinishAttackAnimation(false);
			}
		}

		ActiveAnimationBridge.Reset();
	}

	AttackHitWindowTask = nullptr;
	AttackHitWindowEndTask = nullptr;
	AttackHitFallbackTask = nullptr;
	AttackMontageTask = nullptr;
	AttackDurationTask = nullptr;
	RuntimeAttackMontage = nullptr;
	bAnimationBridgeAttackFinalized = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/**
 * 基于球形重叠检测的近战目标收集。
 *
 * 当前故意保持简单，
 * 因为这一阶段攻击原型的重点仍然是：
 * 时序、动画路由和语义连招流，而不是复杂命中体积。
 */
TArray<ASIPCharacter*> USIPGameplayAbility_Attack::CollectTargets(ASIPCharacter* SourceCharacter) const
{
	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceCharacter);

	const float EffectiveAttackRange = AttackRange * GetAttackRangeMultiplier(SourceCharacter);
	const FVector StartLocation = SourceCharacter->GetActorLocation() + SourceCharacter->GetActorForwardVector() * EffectiveAttackRange;
	UKismetSystemLibrary::SphereOverlapActors(
		SourceCharacter,
		StartLocation,
		AttackRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(ECC_Pawn)},
		ASIPCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappingActors);

	TArray<ASIPCharacter*> Targets;
	for (AActor* OverlappingActor : OverlappingActors)
	{
		ASIPCharacter* TargetCharacter = Cast<ASIPCharacter>(OverlappingActor);
		if (TargetCharacter && !TargetCharacter->IsDeadOrDying())
		{
			Targets.AddUnique(TargetCharacter);
		}
	}

	return Targets;
}

/**
 * 冰面高动量攻击可以稍微延长有效攻击距离。
 *
 * 这样做是为了避免：
 * 视觉上看起来是大幅滑切，
 * 但命中检测却仍然像很短的小平砍一样频繁空挥。
 */
float USIPGameplayAbility_Attack::GetAttackRangeMultiplier(const ASIPCharacter* SourceCharacter) const
{
	if (!bEnableIceMomentumAttack)
	{
		return 1.0f;
	}

	const ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(SourceCharacter);
	if (!HeroCharacter || !HeroCharacter->IsOnIceSurface())
	{
		return 1.0f;
	}

	return SourceCharacter->GetVelocity().Size2D() >= IceMomentumMinSpeed
		? FMath::Max(1.0f, IceMomentumAttackRangeMultiplier)
		: 1.0f;
}

/**
 * 建立事件驱动的攻击表现链。
 *
 * 职责包括：
 * 1. 订阅动画桥接层发出的 Gameplay Event。
 * 2. 当桥接层时序不可用时，提供本地计时回退。
 * 3. 启动蒙太奇播放，或启动固定时长的本地回退计时器。
 */
bool USIPGameplayAbility_Attack::StartAnimationDrivenAttack(ASIPCharacter* SourceCharacter)
{
	USIPHeroAnimationBridgeComponent* AnimationBridge = SourceCharacter->FindComponentByClass<USIPHeroAnimationBridgeComponent>();
	float ResolvedHitWindowStartDelay = AttackHitWindowStartDelay;
	float ResolvedHitWindowEndDelay = AttackHitWindowEndDelay;
	float ResolvedAnimationDuration = AttackAnimationDuration;
	UAnimMontage* ResolvedAttackMontage = ResolveAttackMontageForCharacter(SourceCharacter, ResolvedHitWindowStartDelay, ResolvedHitWindowEndDelay, ResolvedAnimationDuration);
	const FGameplayTag ResolvedWeaponModuleTag = ResolveWeaponModuleTagForCharacter(SourceCharacter);

	bAnimationBridgeAttackFinalized = false;

	AttackHitWindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SIPGameplayTags::Event_Animation_Attack_HitWindow_Start, nullptr, false, true);
	if (AttackHitWindowTask)
	{
		AttackHitWindowTask->EventReceived.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackHitWindowEvent);
		AttackHitWindowTask->ReadyForActivation();
	}

	AttackHitWindowEndTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SIPGameplayTags::Event_Animation_Attack_HitWindow_End, nullptr, false, true);
	if (AttackHitWindowEndTask)
	{
		AttackHitWindowEndTask->EventReceived.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackHitWindowEndEvent);
		AttackHitWindowEndTask->ReadyForActivation();
	}

	bool bHasBridgeTiming = false;
	if (AnimationBridge)
	{
		ActiveAnimationBridge = AnimationBridge;

		// 使用 GA 已解析的描述符和施法阶段传给桥接层，
		// 避免桥接层二次解析产生不一致的语义状态。
		const FGameplayTag EffectiveCastPhase = ResolvedCastPhaseForCurrentAttack.IsValid()
			? ResolvedCastPhaseForCurrentAttack
			: SIPGameplayTags::State_Combat_Cast_PreCast;

		if (ResolvedCombatDescriptorForCurrentAttack.HasResolvedAction())
		{
			bHasBridgeTiming = AnimationBridge->RequestAttackAnimation(
				ResolvedHitWindowStartDelay,
				ResolvedHitWindowEndDelay,
				ResolvedWeaponModuleTag,
				EffectiveCastPhase,
				ResolvedCombatDescriptorForCurrentAttack);
		}
		else
		{
			bHasBridgeTiming = AnimationBridge->RequestAttackAnimation(
				ResolvedHitWindowStartDelay,
				ResolvedHitWindowEndDelay,
				ResolvedWeaponModuleTag,
				EffectiveCastPhase);
		}

		if (bHasBridgeTiming)
		{
			UE_LOG(LogSIPAbilitySystem, Log, TEXT("Attack ability using animation bridge timing for [%s]."), *GetNameSafe(SourceCharacter));
		}
		else
		{
			ActiveAnimationBridge.Reset();
			UE_LOG(LogSIPAbilitySystem, Warning, TEXT("Attack ability failed to arm HeroAnimationBridgeComponent timing on [%s], using local timing fallback."), *GetNameSafe(SourceCharacter));
		}
	}
	else
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("Attack ability did not find HeroAnimationBridgeComponent on [%s], using local timing fallback."), *GetNameSafe(SourceCharacter));
	}

	if (!bHasBridgeTiming)
	{
		AttackHitFallbackTask = UAbilityTask_WaitDelay::WaitDelay(this, ResolvedHitWindowStartDelay);
		if (AttackHitFallbackTask)
		{
			AttackHitFallbackTask->OnFinish.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackHitWindowFallbackElapsed);
			AttackHitFallbackTask->ReadyForActivation();
		}
	}

	if (ResolvedAttackMontage)
	{
		AttackMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ResolvedAttackMontage);
		if (AttackMontageTask)
		{
			AttackMontageTask->OnCompleted.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackAnimationCompleted);
			AttackMontageTask->OnInterrupted.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackAnimationInterrupted);
			AttackMontageTask->OnCancelled.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackAnimationInterrupted);
			AttackMontageTask->OnBlendOut.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackAnimationBlendingOut);
			AttackMontageTask->ReadyForActivation();
		}
	}
	else
	{
		AttackDurationTask = UAbilityTask_WaitDelay::WaitDelay(this, ResolvedAnimationDuration);
		if (AttackDurationTask)
		{
			AttackDurationTask->OnFinish.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackFallbackDurationElapsed);
			AttackDurationTask->ReadyForActivation();
		}
	}

	return AttackHitWindowTask || AttackHitWindowEndTask || AttackHitFallbackTask || AttackMontageTask || AttackDurationTask;
}

/**
 * 解析本次攻击最终实际播放的动画。
 *
 * 优先级顺序：
 * 1. 由 ComboIndex + 语义上下文选中的连招条目。
 * 2. 如果存在，则使用旧的原型预设动画片段。
 * 3. 最后再回退到能力自身持有的固定蒙太奇。
 */
UAnimMontage* USIPGameplayAbility_Attack::ResolveAttackMontageForCharacter(ASIPCharacter* SourceCharacter, float& OutHitWindowStartDelay, float& OutHitWindowEndDelay, float& OutAnimationDuration)
{
	OutHitWindowStartDelay = AttackHitWindowStartDelay;
	OutHitWindowEndDelay = AttackHitWindowEndDelay;
	OutAnimationDuration = AttackAnimationDuration;
	RuntimeAttackMontage = nullptr;
	ResolvedCombatDescriptorForCurrentAttack = FSIPCombatActionDescriptor();
	ResolvedCastPhaseForCurrentAttack = FGameplayTag();

	if (!bPreferPrototypeAttackAnimation)
	{
		return AttackMontage;
	}

	ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(SourceCharacter);
	const FGameplayTag ResolvedWeaponModuleTag = ResolveWeaponModuleTagForCharacter(SourceCharacter);
	FGameplayTag OutCastPhaseTag;
	const FSIPCombatActionDescriptor ResolvedCombatDescriptor = ResolveCombatDescriptorForCharacter(HeroCharacter, ResolvedWeaponModuleTag, OutCastPhaseTag);
	ResolvedCombatDescriptorForCurrentAttack = ResolvedCombatDescriptor;
	ResolvedCastPhaseForCurrentAttack = OutCastPhaseTag;

	if (const FSIPAttackComboEntry* ComboEntry = ResolveComboEntryForCharacter(HeroCharacter, ResolvedWeaponModuleTag, ResolvedCombatDescriptor))
	{
		if (UAnimSequenceBase* ComboAnimation = ComboEntry->Animation.LoadSynchronous())
		{
			OutHitWindowStartDelay = ComboEntry->HitWindowStartDelay;
			OutHitWindowEndDelay = ComboEntry->HitWindowEndDelay;
			OutAnimationDuration = ComboAnimation->GetPlayLength();
			const FName RequestedSlotName = ComboEntry->SlotName.IsNone() ? AttackMontageSlotName : ComboEntry->SlotName;
			const FName ResolvedSlotName = ResolvePlayableSlotName(RequestedSlotName);
			// 动态 BlendOut：高速时延长混出，配合 Inertialization 给 DeadBlending 更多衰减时间。
			const float GroundSpeed = SourceCharacter ? SourceCharacter->GetVelocity().Size2D() : 0.0f;
			const float DynamicBlendOut = FMath::Lerp(0.25f, 0.65f, FMath::Clamp((GroundSpeed - 100.0f) / 300.0f, 0.0f, 1.0f));
			RuntimeAttackMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(ComboAnimation, ResolvedSlotName, 0.08f, DynamicBlendOut, 1.0f, 1, -1.0f, 0.0f);
			if (RuntimeAttackMontage)
			{
				// ABP 拓扑修正后 DefaultSlot 直连 OffsetRootBone，
				// DeadBlending 节点在 AO 路径上而非 Slot→RootBone 之间，
				// Inertialization 找不到正确节点会回退但产生瞬移。
				// 改用标准混合，DynamicBlendOut 控制过渡时长。
				RuntimeAttackMontage->BlendModeOut = EMontageBlendMode::Standard;
				if (HeroCharacter)
				{
					const int32 ResolvedNextComboIndex =
						ComboEntry->NextComboIndex != INDEX_NONE
							? ComboEntry->NextComboIndex
							: ComboEntry->ComboIndex + 1;
					HeroCharacter->CommitAttackComboIndex(ResolvedNextComboIndex);
				}

				if (bDebugComboFlow)
				{
					EmitAttackDebug(
						this,
						FString::Printf(
							TEXT("[Attack] Combo=%s Slot=%s RequestedSlot=%s Module=%s Action=%s Variant=%s"),
							*ComboEntry->EntryId.ToString(),
							*ResolvedSlotName.ToString(),
							*RequestedSlotName.ToString(),
							*ResolvedWeaponModuleTag.ToString(),
							*ResolvedCombatDescriptor.ActionFamilyTag.ToString(),
							*ResolvedCombatDescriptor.DesiredVariant.ToString()),
						bDebugComboOnScreen);
				}

				return RuntimeAttackMontage;
			}
		}
	}

	const FPrototypeAttackAnimationSpec* PrototypeAttackSpec = GetPrototypeAttackAnimationSpec(SourceCharacter);
	if (!PrototypeAttackSpec)
	{
		return AttackMontage;
	}

	UAnimSequenceBase* PrototypeAttackAnimation = LoadObject<UAnimSequenceBase>(nullptr, PrototypeAttackSpec->AnimationAssetPath);
	if (!PrototypeAttackAnimation)
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("Attack ability failed to load prototype attack animation [%s]."), PrototypeAttackSpec->AnimationAssetPath);
		return AttackMontage;
	}

	OutHitWindowStartDelay = PrototypeAttackSpec->HitWindowStartDelay;
	OutHitWindowEndDelay = PrototypeAttackSpec->HitWindowEndDelay;
	OutAnimationDuration = PrototypeAttackAnimation->GetPlayLength();
	RuntimeAttackMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(PrototypeAttackAnimation, ResolvePlayableSlotName(AttackMontageSlotName), 0.1f, 0.15f, 1.0f, 1, -1.0f, 0.0f);
	if (RuntimeAttackMontage)
	{
		RuntimeAttackMontage->BlendModeOut = EMontageBlendMode::Standard;
	}

	if (!RuntimeAttackMontage)
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("Attack ability failed to build dynamic montage from prototype animation [%s]."), PrototypeAttackSpec->AnimationAssetPath);
		return AttackMontage;
	}

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("Attack ability using prototype attack animation [%s]."), PrototypeAttackSpec->AnimationAssetPath);
	return RuntimeAttackMontage;
}

/**
 * 处理运行时攻击表现要走的 Slot 名称。
 *
 * Translate authored combat slot names into slots that the current AnimBP can
 * actually consume.
 *
 * ABP_SandboxCharacter currently guarantees `DefaultSlot` and `UpperBody`.
 * `FullBody_Combat` is still an authored combat lane name, but until the AnimBP
 * owns a real slot node for it we remap that request into `DefaultSlot`.
 *
 * Once the AnimBP exposes a real `FullBody_Combat` slot, this function can be
 * simplified back into a near pass-through.
 */
FName USIPGameplayAbility_Attack::ResolvePlayableSlotName(const FName RequestedSlotName) const
{
	static const FName FullBodyCombatSlotName(TEXT("FullBody_Combat"));
	static const FName DefaultSlotName(TEXT("DefaultSlot"));

	// FullBody_Combat 在 ABP 中存在但位于 LayeredBoneBlend 之前，
	// 战斗蒙太奇走它会与缓存的 LocomotionBase 混合产生瞬移。
	// 保持走 DefaultSlot（经 Inertialization DeadBlending 到
	// OffsetRootBone 的标准路径），抽动问题通过延长宽限期修复。
	if (RequestedSlotName == FullBodyCombatSlotName)
	{
		return DefaultSlotName;
	}

	return RequestedSlotName.IsNone() ? DefaultSlotName : RequestedSlotName;
}

/**
 * 解析本次攻击应该对外表现成哪个武器模块标签。
 *
 * 当前优先级：
 * 1. 数据资产上的默认值。
 * 2. 能力类自身的默认值。
 * 3. 装备系统尚未接完时，硬回退到 Rune Dagger。
 */
FGameplayTag USIPGameplayAbility_Attack::ResolveWeaponModuleTagForCharacter(const ASIPCharacter* SourceCharacter) const
{
	if (AttackComboDataAsset && AttackComboDataAsset->DefaultWeaponModuleTag.IsValid())
	{
		return AttackComboDataAsset->DefaultWeaponModuleTag;
	}

	if (WeaponModuleTag.IsValid())
	{
		return WeaponModuleTag;
	}

	return SIPGameplayTags::State_Combat_WeaponModule_RuneDagger;
}

/**
 * 向共享语义解析器请求当前动作的语义答案。
 *
 * 如果桥接层当前已经带着一个 `DelayedRestart` 描述符，
 * 这里会优先复用它，
 * 让下一次连按能够接着语义尾态往下走，
 * 而不是突然塌回普通中性攻击。
 */
FSIPCombatActionDescriptor USIPGameplayAbility_Attack::ResolveCombatDescriptorForCharacter(ASIPHeroCharacter* HeroCharacter, const FGameplayTag& ResolvedWeaponModuleTag, FGameplayTag& OutResolvedCastPhaseTag) const
{
	FSIPCombatActionDescriptor Descriptor;
	OutResolvedCastPhaseTag = SIPGameplayTags::State_Combat_Cast_PreCast;
	if (!HeroCharacter)
	{
		return Descriptor;
	}

	// Determine cast phase from bridge context:
	// - Fresh attack (no active semantic tail) → PreCast → SlideEntry 可匹配
	// - Continuation (tail present)             → Release → DriftSlash/DriftTurnSlash
	FGameplayTag ResolvedCastPhaseTag = SIPGameplayTags::State_Combat_Cast_PreCast;
	FSIPCombatResolutionContext ResolutionContext;

	if (USIPHeroAnimationBridgeComponent* AnimationBridge = HeroCharacter->GetHeroAnimationBridgeComponent())
	{
		const FSIPCombatActionDescriptor ExistingDescriptor = AnimationBridge->GetCurrentCombatActionDescriptor();

		// DelayedRestart tail → directly reuse the descriptor.
		if (ExistingDescriptor.HasResolvedAction() &&
			AnimationBridge->GetCurrentWeaponModuleTag().MatchesTagExact(ResolvedWeaponModuleTag) &&
			ExistingDescriptor.ActionFamilyTag.MatchesTagExact(SIPGameplayTags::State_Combat_ActionFamily_DelayedRestart))
		{
			OutResolvedCastPhaseTag = SIPGameplayTags::State_Combat_Cast_Release;
			return ExistingDescriptor;
		}

		// Any other active semantic tail → this is a continuation attack.
		if (ExistingDescriptor.HasResolvedAction())
		{
			ResolvedCastPhaseTag = SIPGameplayTags::State_Combat_Cast_Release;
		}

		ResolutionContext.PreviousBodyStateTag = AnimationBridge->GetCurrentCombatBodyStateTag();
	}

	OutResolvedCastPhaseTag = ResolvedCastPhaseTag;

	const FSIPCombatFeatureVector FeatureVector = SIPCombatSemantic::BuildHeroCombatFeatureVector(
		HeroCharacter,
		ResolvedWeaponModuleTag,
		ResolvedCastPhaseTag,
		GetSignedTurnAngleDegrees(HeroCharacter));

	return SIPCombatSemantic::ResolveIceRuneDaggerGoldenPath(FeatureVector, ResolutionContext);
}

/**
 * 为当前 ComboIndex 选出最合适的连招条目。
 *
 * 这里故意对“缺资源”的情况比较宽容：
 * 如果资产提供的数组是空的，
 * 或者虽然不空但本质上只是空结构体壳子，
 * 就直接回退到内建的 Rune Dagger 默认条目，
 * 不让整条攻击链返回空结果。
 */
const FSIPAttackComboEntry* USIPGameplayAbility_Attack::ResolveComboEntryForCharacter(ASIPHeroCharacter* HeroCharacter, const FGameplayTag& ResolvedWeaponModuleTag, const FSIPCombatActionDescriptor& CombatDescriptor) const
{
	const TArray<FSIPAttackComboEntry>& PrimaryEntries =
		(AttackComboDataAsset && USIPAttackComboDataAsset::HasMeaningfulComboEntries(AttackComboDataAsset->ComboEntries))
			? AttackComboDataAsset->ComboEntries
			: (USIPAttackComboDataAsset::HasMeaningfulComboEntries(ComboEntries) ? ComboEntries : GetDefaultAttackComboEntries());
	const TArray<FSIPAttackComboEntry>& SemanticFallbackEntries = GetDefaultAttackComboEntries();
	const bool bNeedSemanticFallback = (&PrimaryEntries != &SemanticFallbackEntries)
		&& CombatDescriptor.HasResolvedAction();
	if (!HeroCharacter || PrimaryEntries.IsEmpty())
	{
		return nullptr;
	}

	const float ActiveComboResetWindowSeconds =
		AttackComboDataAsset
			? AttackComboDataAsset->ComboResetWindowSeconds
			: ComboResetWindowSeconds;
	const int32 RequestedComboIndex = HeroCharacter->GetResolvedAttackComboIndex(ActiveComboResetWindowSeconds);

	auto FindBestMatch = [&](const int32 ComboIndex) -> const FSIPAttackComboEntry*
	{
		const FSIPAttackComboEntry* BestEntry = nullptr;
		for (const FSIPAttackComboEntry& Entry : PrimaryEntries)
		{
			if (!DoesComboEntryMatchContext(Entry, HeroCharacter, ResolvedWeaponModuleTag, ComboIndex, CombatDescriptor))
			{
				continue;
			}

			if (!BestEntry || Entry.Priority > BestEntry->Priority)
			{
				BestEntry = &Entry;
			}
		}

		if (bNeedSemanticFallback)
		{
			for (const FSIPAttackComboEntry& Entry : SemanticFallbackEntries)
			{
				if (!DoesComboEntryMatchContext(Entry, HeroCharacter, ResolvedWeaponModuleTag, ComboIndex, CombatDescriptor))
				{
					continue;
				}

				if (!BestEntry || Entry.Priority > BestEntry->Priority)
				{
					BestEntry = &Entry;
				}
			}
		}

		return BestEntry;
	};

	if (const FSIPAttackComboEntry* Match = FindBestMatch(RequestedComboIndex))
	{
		return Match;
	}

	if (RequestedComboIndex > 0)
	{
		HeroCharacter->ResetAttackComboState();
		return FindBestMatch(0);
	}

	return nullptr;
}

/**
 * 用当前运行时上下文去评估一条连招条目是否可用。
 *
 * 一条条目可以只要求：
 * 1. 原始移动条件。
 * 2. 语义动作家族条件。
 * 3. 也可以两者同时成立。
 */
bool USIPGameplayAbility_Attack::DoesComboEntryMatchContext(const FSIPAttackComboEntry& Entry, const ASIPHeroCharacter* HeroCharacter, const FGameplayTag& ResolvedWeaponModuleTag, const int32 ComboIndex, const FSIPCombatActionDescriptor& CombatDescriptor) const
{
	if (!HeroCharacter || Entry.ComboIndex != ComboIndex)
	{
		return false;
	}

	if (Entry.WeaponModuleTag.IsValid() && !Entry.WeaponModuleTag.MatchesTagExact(ResolvedWeaponModuleTag))
	{
		return false;
	}

	if (Entry.bRequireIceSurface && !HeroCharacter->IsOnIceSurface())
	{
		return false;
	}

	const bool bUsesSemanticFilter =
		Entry.RequiredActionFamilyTag.IsValid() ||
		Entry.RequiredBodyStateTag.IsValid() ||
		!Entry.RequiredVariant.IsNone();
	if (bUsesSemanticFilter)
	{
		if (!CombatDescriptor.HasResolvedAction())
		{
			return false;
		}

		if (Entry.RequiredActionFamilyTag.IsValid() && !CombatDescriptor.ActionFamilyTag.MatchesTagExact(Entry.RequiredActionFamilyTag))
		{
			return false;
		}

		if (Entry.RequiredBodyStateTag.IsValid() && !CombatDescriptor.BodyStateTag.MatchesTagExact(Entry.RequiredBodyStateTag))
		{
			return false;
		}

		if (!Entry.RequiredVariant.IsNone() && Entry.RequiredVariant != CombatDescriptor.DesiredVariant)
		{
			return false;
		}
	}

	const float GroundSpeed = HeroCharacter->GetVelocity().Size2D();
	if (GroundSpeed < Entry.MinGroundSpeed)
	{
		return false;
	}

	if (Entry.MaxGroundSpeed >= 0.0f && GroundSpeed > Entry.MaxGroundSpeed)
	{
		return false;
	}

	const float SignedTurnAngleDegrees = GetSignedTurnAngleDegrees(HeroCharacter);
	const float AbsTurnAngleDegrees = FMath::Abs(SignedTurnAngleDegrees);
	if (AbsTurnAngleDegrees < Entry.MinAbsTurnAngleDegrees)
	{
		return false;
	}

	if (Entry.MaxAbsTurnAngleDegrees >= 0.0f && AbsTurnAngleDegrees > Entry.MaxAbsTurnAngleDegrees)
	{
		return false;
	}

	if (Entry.RequiredTurnSign > 0 && SignedTurnAngleDegrees <= 0.0f)
	{
		return false;
	}

	if (Entry.RequiredTurnSign < 0 && SignedTurnAngleDegrees >= 0.0f)
	{
		return false;
	}

	return !Entry.Animation.IsNull();
}

/**
 * 只有在动画驱动链完全建不起来时才会使用的旧版回退攻击。
 */
void USIPGameplayAbility_Attack::ExecuteLegacyAttack(ASIPCharacter* SourceCharacter)
{
	if (AttackMontage && SourceCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInstance = SourceCharacter->GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(AttackMontage);
		}
	}

	const TArray<ASIPCharacter*> Targets = CollectTargets(SourceCharacter);
	for (ASIPCharacter* Target : Targets)
	{
		Target->ApplyCombatDamage(DamageAmount, SourceCharacter);
	}

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("Attack ability hit %d target(s)."), Targets.Num());
}

/**
 * 打开当前命中窗口，并保证每次攻击只结算一次真正伤害。
 */
void USIPGameplayAbility_Attack::OnAttackHitWindowEvent(FGameplayEventData Payload)
{
	if (bHasAppliedAttackHit || !IsActive())
	{
		return;
	}

	ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(GetAvatarActorFromActorInfo());
	if (!SourceCharacter)
	{
		return;
	}

	bHasAppliedAttackHit = true;

	const TArray<ASIPCharacter*> Targets = CollectTargets(SourceCharacter);
	for (ASIPCharacter* Target : Targets)
	{
		Target->ApplyCombatDamage(DamageAmount, SourceCharacter);
	}

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("Attack ability hit %d target(s) during animation event [%s]."), Targets.Num(), *Payload.EventTag.ToString());
}

/**
 * 当桥接层路径不可用时，用本地计时器合成一个“命中窗口开启”事件。
 */
void USIPGameplayAbility_Attack::OnAttackHitWindowFallbackElapsed()
{
	FGameplayEventData Payload;
	Payload.EventTag = SIPGameplayTags::Event_Animation_Attack_HitWindow_Start;
	OnAttackHitWindowEvent(Payload);
}

/**
 * 关闭当前命中窗口，并在合适时把缓冲输入交给下一次攻击激活。
 */
void USIPGameplayAbility_Attack::OnAttackHitWindowEndEvent(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}

	ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(GetAvatarActorFromActorInfo());
	if (!HeroCharacter)
	{
		return;
	}

	const float ActiveBufferedComboInputWindowSeconds =
		AttackComboDataAsset
			? AttackComboDataAsset->BufferedComboInputWindowSeconds
			: BufferedComboInputWindowSeconds;
	const bool bConsumeBufferedInput = HeroCharacter->ConsumeBufferedAttackInput(ActiveBufferedComboInputWindowSeconds);
	if (bDebugComboFlow)
	{
		EmitAttackDebug(
			this,
			FString::Printf(TEXT("[Attack] WindowEnd BufferedFollowUp=%s"), bConsumeBufferedInput ? TEXT("Y") : TEXT("N")),
			bDebugComboOnScreen);
	}

	if (!bConsumeBufferedInput)
	{
		return;
	}

	USIPAbilitySystemComponent* SIPASC = HeroCharacter->GetSIPAbilitySystemComponent();
	if (!SIPASC)
	{
		return;
	}

	if (ActiveAnimationBridge.IsValid() && !bAnimationBridgeAttackFinalized)
	{
		ActiveAnimationBridge->FinishAttackAnimation(true);
		bAnimationBridgeAttackFinalized = true;
	}

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}

	SIPASC->AbilityInputTagPressed(SIPGameplayTags::InputTag_Attack);
}

/**
 * 处理当前攻击表现链的正常结束。
 *
 * 这里既负责保留桥接层上的语义尾态，
 * 也负责在玩家于缓冲窗口内再次按下攻击时，
 * 重新触发下一次攻击输入。
 */
void USIPGameplayAbility_Attack::OnAttackAnimationCompleted()
{
	// BlendOut 回调已处理桥接层通知和缓冲输入。此处仅做兜底 + EndAbility。
	if (ActiveAnimationBridge.IsValid() && !bAnimationBridgeAttackFinalized)
	{
		ActiveAnimationBridge->FinishAttackAnimation(false);
		bAnimationBridgeAttackFinalized = true;
	}

	if (ActiveAnimationBridge.IsValid())
	{
		ActiveAnimationBridge->NotifyMontageFullyEnded();
	}

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void USIPGameplayAbility_Attack::OnAttackAnimationBlendingOut()
{
	if (!ActiveAnimationBridge.IsValid() || bAnimationBridgeAttackFinalized)
	{
		return;
	}

	bool bConsumeBufferedInput = false;
	USIPAbilitySystemComponent* SIPASC = nullptr;

	if (ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(GetAvatarActorFromActorInfo()))
	{
		const float ActiveBufferedComboInputWindowSeconds =
			AttackComboDataAsset
				? AttackComboDataAsset->BufferedComboInputWindowSeconds
				: BufferedComboInputWindowSeconds;
		bConsumeBufferedInput = HeroCharacter->ConsumeBufferedAttackInput(ActiveBufferedComboInputWindowSeconds);
		SIPASC = HeroCharacter->GetSIPAbilitySystemComponent();

		if (bDebugComboFlow)
		{
			EmitAttackDebug(
				this,
				FString::Printf(TEXT("[Attack] Completed BufferedFollowUp=%s"), bConsumeBufferedInput ? TEXT("Y") : TEXT("N")),
				bDebugComboOnScreen);
		}
	}

	ActiveAnimationBridge->FinishAttackAnimation(bConsumeBufferedInput);
	bAnimationBridgeAttackFinalized = true;

	if (bConsumeBufferedInput && SIPASC)
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
		SIPASC->AbilityInputTagPressed(SIPGameplayTags::InputTag_Attack);
	}
}

/**
 * 取消当前表现链，并确保桥接层不会残留过期状态。
 */
void USIPGameplayAbility_Attack::OnAttackAnimationInterrupted()
{
	if (ActiveAnimationBridge.IsValid() && !bAnimationBridgeAttackFinalized)
	{
		ActiveAnimationBridge->CancelAttackAnimation();
		bAnimationBridgeAttackFinalized = true;
	}

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

/**
 * 当没有蒙太奇任务在跑时，使用固定时长回退到“攻击已结束”逻辑。
 */
void USIPGameplayAbility_Attack::OnAttackFallbackDurationElapsed()
{
	OnAttackAnimationCompleted();
}
