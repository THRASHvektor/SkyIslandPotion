// Copyright Epic Games, Inc. All Rights Reserved.

#include "Ability/SIPGameplayAbility_ThrowPotion.h"
#include "Ability/SIPPotionProjectile.h"
#include "Character/SIPCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

USIPGameplayAbility_ThrowPotion::USIPGameplayAbility_ThrowPotion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	ActivationPolicy = ESIPAbilityActivationPolicy::OnInputTriggered;
	ActivationBlockedTags.AddTag(SIPGameplayTags::State_Dead);
}

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

	// ── 1. 计算投掷方向 ───────────────────────────────────────────
	// 优先用摄像机朝向保证"所见即所投"
	FVector ThrowDirection = SourceCharacter->GetActorForwardVector();

	if (const UCameraComponent* Cam = SourceCharacter->FindComponentByClass<UCameraComponent>())
	{
		ThrowDirection = Cam->GetForwardVector();
	}
	else if (const AController* Controller = SourceCharacter->GetController())
	{
		FRotator ControlRot = Controller->GetControlRotation();
		ThrowDirection = ControlRot.Vector();
	}

	// 在水平朝向基础上叠加上仰角，形成抛物线轨迹
	const FRotator PitchRotation(LaunchPitchOffset, 0.f, 0.f);
	ThrowDirection = PitchRotation.RotateVector(ThrowDirection).GetSafeNormal();

	// ── 2. 计算生成位置（手部 Socket 或角色前方偏移）────────────────
	FVector SpawnLocation = SourceCharacter->GetActorLocation() + FVector(1000000.f, 0, 60000.f);

	if (USkeletalMeshComponent* Mesh = SourceCharacter->GetMesh())
	{
		if (Mesh->DoesSocketExist(HandSocketName))
		{
			SpawnLocation = Mesh->GetSocketLocation(HandSocketName);
		}
	}

	// ── 3. 生成弹丸 ───────────────────────────────────────────────
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

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
