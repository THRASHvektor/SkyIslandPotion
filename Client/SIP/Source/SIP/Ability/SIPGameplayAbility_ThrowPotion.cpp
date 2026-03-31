// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/SIPGameplayAbility_ThrowPotion.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Ability/SIPPotionProjectile.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Camera/CameraComponent.h"
#include "Character/Components/SIPContextualCameraComponent.h"
#include "Character/Components/SIPHeroAnimationBridgeComponent.h"
#include "Character/SIPCharacter.h"
#include "Character/SIPHeroCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

namespace
{
	// 当主角预设需要运行时动态蒙太奇时，这里保存原型动画及其时序信息。
	struct FPrototypeThrowAnimationSpec
	{
		const TCHAR* AnimationAssetPath = nullptr;
		float ReleaseDelay = 0.0f;
	};

	// 根据当前主角动画预设，映射出可选的投掷原型动画和释放时机。
	const FPrototypeThrowAnimationSpec* GetPrototypeThrowAnimationSpec(const ASIPCharacter* SourceCharacter)
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
			static const FPrototypeThrowAnimationSpec CombatMagicUnarmedThrow
			{
				TEXT("/Game/CombatMagicAnims/Animations/AS_SpellAttack.AS_SpellAttack"),
				0.55f
			};
			return &CombatMagicUnarmedThrow;
		}
		default:
			return nullptr;
		}
	}
}

// 将投掷药水配置为一次性能力，并在死亡状态下阻止激活。
USIPGameplayAbility_ThrowPotion::USIPGameplayAbility_ThrowPotion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	ActivationPolicy = ESIPAbilityActivationPolicy::OnInputTriggered;
	ActivationBlockedTags.AddTag(SIPGameplayTags::State_Dead);
	WeaponModuleTag = SIPGameplayTags::State_Combat_WeaponModule_FlaskRig;
}

// 投掷前要求投射物类型有效、角色存活，并通过基础 GAS 激活校验。
bool USIPGameplayAbility_ThrowPotion::CanActivateAbility(
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

	if (!ProjectileClass)
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("ThrowPotion: ProjectileClass not set!"));
		return false;
	}

	const ASIPCharacter* Char = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	return Char && !Char->IsDeadOrDying();
}

// 先 Commit 这次投掷，再把真正的出手时机交给动画事件来驱动。
void USIPGameplayAbility_ThrowPotion::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(ActorInfo->AvatarActor.Get());
	if (!SourceCharacter || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasSpawnedProjectile = false;

	if (StartAnimationDrivenThrow(SourceCharacter))
	{
		return;
	}

	ExecuteLegacyThrow(SourceCharacter);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

// 能力结束时清理仍然存活的动画桥接引用和异步任务。
void USIPGameplayAbility_ThrowPotion::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActiveAnimationBridge.IsValid())
	{
		ActiveAnimationBridge->CancelThrowAnimation();
		ActiveAnimationBridge.Reset();
	}

	ThrowReleaseTask = nullptr;
	ThrowReleaseFallbackTask = nullptr;
	ThrowMontageTask = nullptr;
	RuntimeThrowMontage = nullptr;
	ThrowDurationTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// 先挂好释放事件监听，再选择桥接时序或本地回退去触发它。
bool USIPGameplayAbility_ThrowPotion::StartAnimationDrivenThrow(ASIPCharacter* SourceCharacter)
{
	USIPHeroAnimationBridgeComponent* AnimationBridge = SourceCharacter->FindComponentByClass<USIPHeroAnimationBridgeComponent>();
	float ResolvedReleaseDelay = ThrowReleaseDelay;
	float ResolvedAnimationDuration = ThrowAnimationDuration;
	UAnimMontage* ResolvedThrowMontage = ResolveThrowMontageForCharacter(SourceCharacter, ResolvedReleaseDelay, ResolvedAnimationDuration);

	ThrowReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SIPGameplayTags::Event_Animation_Throw_Release, nullptr, false, true);
	if (ThrowReleaseTask)
	{
		ThrowReleaseTask->EventReceived.AddDynamic(this, &USIPGameplayAbility_ThrowPotion::OnThrowReleaseEvent);
		ThrowReleaseTask->ReadyForActivation();
	}

	bool bHasBridgeTiming = false;
	if (AnimationBridge)
	{
		ActiveAnimationBridge = AnimationBridge;
		bHasBridgeTiming = AnimationBridge->RequestThrowAnimation(
			ResolvedReleaseDelay,
			WeaponModuleTag,
			SIPGameplayTags::State_Combat_Cast_PreCast);
		if (bHasBridgeTiming)
		{
			UE_LOG(LogSIPAbilitySystem, Log, TEXT("ThrowPotion ability using animation bridge timing for [%s]."), *GetNameSafe(SourceCharacter));
		}
		else
		{
			ActiveAnimationBridge.Reset();
			UE_LOG(LogSIPAbilitySystem, Warning, TEXT("ThrowPotion ability failed to arm HeroAnimationBridgeComponent timing on [%s], using local timing fallback."), *GetNameSafe(SourceCharacter));
		}
	}
	else
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("ThrowPotion ability did not find HeroAnimationBridgeComponent on [%s], using local timing fallback."), *GetNameSafe(SourceCharacter));
	}

	if (!bHasBridgeTiming)
	{
		ThrowReleaseFallbackTask = UAbilityTask_WaitDelay::WaitDelay(this, ResolvedReleaseDelay);
		if (ThrowReleaseFallbackTask)
		{
			ThrowReleaseFallbackTask->OnFinish.AddDynamic(this, &USIPGameplayAbility_ThrowPotion::OnThrowReleaseFallbackElapsed);
			ThrowReleaseFallbackTask->ReadyForActivation();
		}
	}

	if (ResolvedThrowMontage)
	{
		ThrowMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ResolvedThrowMontage);
		if (ThrowMontageTask)
		{
			ThrowMontageTask->OnCompleted.AddDynamic(this, &USIPGameplayAbility_ThrowPotion::OnThrowAnimationCompleted);
			ThrowMontageTask->OnInterrupted.AddDynamic(this, &USIPGameplayAbility_ThrowPotion::OnThrowAnimationInterrupted);
			ThrowMontageTask->OnCancelled.AddDynamic(this, &USIPGameplayAbility_ThrowPotion::OnThrowAnimationInterrupted);
			ThrowMontageTask->ReadyForActivation();
		}
	}
	else
	{
		ThrowDurationTask = UAbilityTask_WaitDelay::WaitDelay(this, ResolvedAnimationDuration);
		if (ThrowDurationTask)
		{
			ThrowDurationTask->OnFinish.AddDynamic(this, &USIPGameplayAbility_ThrowPotion::OnThrowFallbackDurationElapsed);
			ThrowDurationTask->ReadyForActivation();
		}
	}

	return ThrowReleaseTask || ThrowReleaseFallbackTask || ThrowMontageTask || ThrowDurationTask;
}

// 当优先使用原型动画时，根据当前主角预设构建运行时蒙太奇。
UAnimMontage* USIPGameplayAbility_ThrowPotion::ResolveThrowMontageForCharacter(
	ASIPCharacter* SourceCharacter,
	float& OutReleaseDelay,
	float& OutAnimationDuration)
{
	OutReleaseDelay = ThrowReleaseDelay;
	OutAnimationDuration = ThrowAnimationDuration;
	RuntimeThrowMontage = nullptr;

	if (!bPreferPrototypeThrowAnimation)
	{
		return ThrowMontage;
	}

	const FPrototypeThrowAnimationSpec* PrototypeThrowSpec = GetPrototypeThrowAnimationSpec(SourceCharacter);
	if (!PrototypeThrowSpec)
	{
		return ThrowMontage;
	}

	UAnimSequenceBase* PrototypeThrowAnimation = LoadObject<UAnimSequenceBase>(nullptr, PrototypeThrowSpec->AnimationAssetPath);
	if (!PrototypeThrowAnimation)
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("ThrowPotion ability failed to load prototype throw animation [%s]."), PrototypeThrowSpec->AnimationAssetPath);
		return ThrowMontage;
	}

	OutReleaseDelay = PrototypeThrowSpec->ReleaseDelay;
	OutAnimationDuration = PrototypeThrowAnimation->GetPlayLength();
	RuntimeThrowMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(
		PrototypeThrowAnimation,
		ThrowMontageSlotName,
		0.1f,
		0.15f,
		1.0f,
		1,
		-1.0f,
		0.0f);

	if (!RuntimeThrowMontage)
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("ThrowPotion ability failed to build dynamic montage from prototype animation [%s]."), PrototypeThrowSpec->AnimationAssetPath);
		return ThrowMontage;
	}

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("ThrowPotion ability using prototype throw animation [%s]."), PrototypeThrowSpec->AnimationAssetPath);
	return RuntimeThrowMontage;
}

// 如果动画驱动链路完全无法启动，就走这里作为最终保底。
void USIPGameplayAbility_ThrowPotion::ExecuteLegacyThrow(ASIPCharacter* SourceCharacter)
{
	SpawnPotionProjectile(SourceCharacter);
}

// 优先从手部插槽生成投射物，并尽量使用相机或控制器朝向来决定投掷方向。
void USIPGameplayAbility_ThrowPotion::SpawnPotionProjectile(ASIPCharacter* SourceCharacter) const
{
	if (!SourceCharacter || !ProjectileClass)
	{
		return;
	}

	FVector ThrowDirection = SourceCharacter->GetActorForwardVector();
	bool bResolvedViewDirection = false;

	if (const ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(SourceCharacter))
	{
		if (const USIPContextualCameraComponent* ContextualCamera = HeroCharacter->GetContextualCameraComponent())
		{
			// 优先走统一镜头服务，确保投掷朝向跟随当前真正生效的镜头语境。
			ThrowDirection = ContextualCamera->GetViewDirection();
			bResolvedViewDirection = true;
		}
	}

	if (!bResolvedViewDirection)
	{
		// 保留旧链路作为保险：旧蓝图或非主角角色可能仍然只暴露原始 CameraComponent / Controller 朝向。
		if (const UCameraComponent* Cam = SourceCharacter->FindComponentByClass<UCameraComponent>())
		{
			ThrowDirection = Cam->GetForwardVector();
		}
		else if (const AController* Controller = SourceCharacter->GetController())
		{
			FRotator ControlRot = Controller->GetControlRotation();
			ThrowDirection = ControlRot.Vector();
		}
	}

	const FRotator PitchRotation(LaunchPitchOffset, 0.f, 0.f);
	ThrowDirection = PitchRotation.RotateVector(ThrowDirection).GetSafeNormal();

	FVector SpawnLocation = SourceCharacter->GetActorLocation() + SourceCharacter->GetActorForwardVector() * 100.0f + FVector(0.0f, 0.0f, 60.0f);

	if (USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
	{
		if (Mesh->DoesSocketExist(HandSocketName))
		{
			SpawnLocation = Mesh->GetSocketLocation(HandSocketName);
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = SourceCharacter;
	SpawnParams.Owner = SourceCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASIPPotionProjectile* Projectile = GetWorld()->SpawnActor<ASIPPotionProjectile>(
		ProjectileClass,
		SpawnLocation,
		ThrowDirection.Rotation(),
		SpawnParams
	);

	if (Projectile)
	{
		Projectile->ElementTag = PotionElementTag;
		Projectile->ProjectileMovement->Velocity = ThrowDirection * ThrowSpeed;

		UE_LOG(LogSIPAbilitySystem, Log,
			TEXT("ThrowPotion: spawned [%s] at %s, dir=%s, speed=%.0f"),
			*PotionElementTag.ToString(),
			*SpawnLocation.ToString(),
			*ThrowDirection.ToString(),
			ThrowSpeed);
	}
	else
	{
		UE_LOG(LogSIPAbilitySystem, Error, TEXT("ThrowPotion: failed to spawn projectile!"));
	}
}

// 真正的投射物释放回调；真实动画事件和回退定时器最终都会走到这里。
void USIPGameplayAbility_ThrowPotion::OnThrowReleaseEvent(FGameplayEventData Payload)
{
	if (bHasSpawnedProjectile || !IsActive())
	{
		return;
	}

	ASIPCharacter* SourceCharacter = Cast<ASIPCharacter>(GetAvatarActorFromActorInfo());
	if (!SourceCharacter)
	{
		return;
	}

	bHasSpawnedProjectile = true;
	SpawnPotionProjectile(SourceCharacter);

	UE_LOG(LogSIPAbilitySystem, Log, TEXT("ThrowPotion: release event received [%s]."), *Payload.EventTag.ToString());
}

// 如果桥接组件或蒙太奇始终没有送来释放事件，就走本地回退。
void USIPGameplayAbility_ThrowPotion::OnThrowReleaseFallbackElapsed()
{
	FGameplayEventData Payload;
	Payload.EventTag = SIPGameplayTags::Event_Animation_Throw_Release;
	OnThrowReleaseEvent(Payload);
}

// 蒙太奇正常播完后直接收尾能力，此时投射物释放通常已经发生。
void USIPGameplayAbility_ThrowPotion::OnThrowAnimationCompleted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

// 投掷被打断或取消时，以取消状态结束能力。
void USIPGameplayAbility_ThrowPotion::OnThrowAnimationInterrupted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

// 纯时长回退的收尾行为与蒙太奇正常结束保持一致。
void USIPGameplayAbility_ThrowPotion::OnThrowFallbackDurationElapsed()
{
	OnThrowAnimationCompleted();
}
