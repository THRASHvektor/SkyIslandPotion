#include "SIPGameplayAbility_Attack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Character/SIPCharacter.h"
#include "Character/SIPHeroCharacter.h"
#include "Character/Components/SIPHeroAnimationBridgeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

namespace
{
	struct FPrototypeAttackAnimationSpec
	{
		const TCHAR* AnimationAssetPath = nullptr;
		float HitWindowStartDelay = 0.0f;
		float HitWindowEndDelay = 0.0f;
	};

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
 * Z 说明：
 * SIPGameplayAbility_Attack.cpp 实现主角近战攻击能力。
 *
 * 主要流程：
 * 1. 激活能力并完成 Commit。
 * 2. 启动动画驱动攻击链路，监听命中窗口事件。
 * 3. 在命中窗口开启时收集目标并结算伤害。
 * 4. 由蒙太奇结束、打断或固定时长回退来收尾能力。
 */
USIPGameplayAbility_Attack::USIPGameplayAbility_Attack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	AbilityTags.AddTag(SIPGameplayTags::InputTag_Attack);
	ActivationBlockedTags.AddTag(SIPGameplayTags::State_Dead);
}

/**
 * Z 说明：CanActivateAbility
 * 只有角色有效且未死亡时，攻击能力才允许激活
 */
bool USIPGameplayAbility_Attack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	return SourceCharacter && !SourceCharacter->IsDeadOrDying();
}

/**
 * Z 说明：ActivateAbility
 * 优先进入动画驱动链路：
 * 1. Commit 能力
 * 2. 重置本轮攻击命中状态
 * 3. 尝试启动动画驱动攻击
 * 4. 如果链路失败，再回退到旧版即时攻击
 */
void USIPGameplayAbility_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	if (!SourceCharacter || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasAppliedAttackHit = false;

	if (StartAnimationDrivenAttack(SourceCharacter))
	{
		return;
	}

	ExecuteLegacyAttack(SourceCharacter);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

/**
 * Z 说明：EndAbility
 * 结束能力时需要清空所有异步任务，避免旧事件残留到下一次攻击
 */
void USIPGameplayAbility_Attack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActiveAnimationBridge.IsValid())
	{
		ActiveAnimationBridge->CancelAttackAnimation();
		ActiveAnimationBridge.Reset();
	}

	AttackHitWindowTask = nullptr;
	AttackHitFallbackTask = nullptr;
	AttackMontageTask = nullptr;
	AttackDurationTask = nullptr;
	RuntimeAttackMontage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

/**
 * Z 说明：CollectTargets
 * 使用球形检测收集主角前方的可受击目标
 */
TArray<ASIPCharacter*> USIPGameplayAbility_Attack::CollectTargets(ASIPCharacter* SourceCharacter) const
{
	TArray<AActor*> OverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceCharacter);

	const FVector StartLocation = SourceCharacter->GetActorLocation() + SourceCharacter->GetActorForwardVector() * AttackRange;
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
 * Z 说明：StartAnimationDrivenAttack
 * 启动攻击的表现层链路。
 *
 * 处理顺序：
 * 1. 先挂好 Gameplay Event 监听，避免桥接事件先发后收不到。
 * 2. 再创建延时回退，确保没有 Notify 时也能触发命中。
 * 3. 如果存在动画桥接组件，则请求其分发攻击事件。
 * 4. 最后启动蒙太奇或固定时长任务，负责结束能力。
 */
bool USIPGameplayAbility_Attack::StartAnimationDrivenAttack(ASIPCharacter* SourceCharacter)
{
	USIPHeroAnimationBridgeComponent* AnimationBridge = SourceCharacter->FindComponentByClass<USIPHeroAnimationBridgeComponent>();
	float ResolvedHitWindowStartDelay = AttackHitWindowStartDelay;
	float ResolvedHitWindowEndDelay = AttackHitWindowEndDelay;
	float ResolvedAnimationDuration = AttackAnimationDuration;
	UAnimMontage* ResolvedAttackMontage = ResolveAttackMontageForCharacter(SourceCharacter, ResolvedHitWindowStartDelay, ResolvedHitWindowEndDelay, ResolvedAnimationDuration);

	AttackHitWindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SIPGameplayTags::Event_Animation_Attack_HitWindow_Start, nullptr, false, true);
	if (AttackHitWindowTask)
	{
		AttackHitWindowTask->EventReceived.AddDynamic(this, &USIPGameplayAbility_Attack::OnAttackHitWindowEvent);
		AttackHitWindowTask->ReadyForActivation();
	}

	bool bHasBridgeTiming = false;
	if (AnimationBridge)
	{
		ActiveAnimationBridge = AnimationBridge;
		bHasBridgeTiming = AnimationBridge->RequestAttackAnimation(ResolvedHitWindowStartDelay, ResolvedHitWindowEndDelay);
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

	return AttackHitWindowTask || AttackHitFallbackTask || AttackMontageTask || AttackDurationTask;
}

UAnimMontage* USIPGameplayAbility_Attack::ResolveAttackMontageForCharacter(ASIPCharacter* SourceCharacter, float& OutHitWindowStartDelay, float& OutHitWindowEndDelay, float& OutAnimationDuration)
{
	OutHitWindowStartDelay = AttackHitWindowStartDelay;
	OutHitWindowEndDelay = AttackHitWindowEndDelay;
	OutAnimationDuration = AttackAnimationDuration;
	RuntimeAttackMontage = nullptr;

	if (!bPreferPrototypeAttackAnimation)
	{
		return AttackMontage;
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
	RuntimeAttackMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(PrototypeAttackAnimation, AttackMontageSlotName, 0.1f, 0.15f, 1.0f, 1, -1.0f, 0.0f);

	if (!RuntimeAttackMontage)
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("Attack ability failed to build dynamic montage from prototype animation [%s]."), PrototypeAttackSpec->AnimationAssetPath);
		return AttackMontage;
	}

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("Attack ability using prototype attack animation [%s]."), PrototypeAttackSpec->AnimationAssetPath);
	return RuntimeAttackMontage;
}

/**
 * Z 说明：ExecuteLegacyAttack
 * 旧版攻击逻辑：
 * 1. 直接播放蒙太奇
 * 2. 立即收集目标并结算伤害
 *
 * 该逻辑仅作为链路异常时的最终保底
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
 * Z 说明：OnAttackHitWindowEvent
 * 在命中窗口开启时统一结算伤害，并且只允许本轮攻击触发一次
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
 * Z 说明：OnAttackHitWindowFallbackElapsed
 * 当动画事件未到达时，手动构造一个命中窗口事件作为兜底
 */
void USIPGameplayAbility_Attack::OnAttackHitWindowFallbackElapsed()
{
	FGameplayEventData Payload;
	Payload.EventTag = SIPGameplayTags::Event_Animation_Attack_HitWindow_Start;
	OnAttackHitWindowEvent(Payload);
}

// Z 说明：攻击动画正常播放完成后结束能力
void USIPGameplayAbility_Attack::OnAttackAnimationCompleted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

// Z 说明：攻击动画被取消或打断后结束能力，并标记为取消
void USIPGameplayAbility_Attack::OnAttackAnimationInterrupted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

// Z 说明：没有蒙太奇时，固定时长到期后按“播放完成”处理
void USIPGameplayAbility_Attack::OnAttackFallbackDurationElapsed()
{
	OnAttackAnimationCompleted();
}
