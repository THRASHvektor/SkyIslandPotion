// Copyright Epic Games, Inc. All Rights Reserved.

#include "SIPCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Ability/SIPAbilitySet.h"
#include "AttributeSet/SIPHealthSet.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"


ASIPCharacter::ASIPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	AbilitySystemComponent = CreateDefaultSubobject<USIPAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void ASIPCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASIPCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySetHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	}
	Super::EndPlay(EndPlayReason);
}

void ASIPCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		UE_LOG(LogSIPCharacter, Log, TEXT("%s ASC initialized. Granting abilities..."), *GetName());

		for (const TObjectPtr<USIPAbilitySet>& Set : AbilitySets)
		{
			if (Set)
			{
				Set->GiveToAbilitySystem(AbilitySystemComponent, &AbilitySetHandles, this);
			}
		}

		if (!GetSIPHealthSet())
		{
			USIPHealthSet* DefaultHealthSet = NewObject<USIPHealthSet>(this, USIPHealthSet::StaticClass());
			AbilitySystemComponent->AddAttributeSetSubobject(DefaultHealthSet);
			UE_LOG(LogSIPCharacter, Warning, TEXT("%s had no HealthSet configured. Created fallback HealthSet."), *GetName());
		}

		if (USIPHealthSet* HealthSet = GetSIPHealthSet())
		{
			const float ClampedMaxHealth = FMath::Max(DefaultMaxHealth, 1.0f);
			const float ClampedStartingHealth = FMath::Clamp(DefaultStartingHealth, 0.0f, ClampedMaxHealth);

			if (HealthSet->MaxHealth.GetCurrentValue() <= 0.0f)
			{
				AbilitySystemComponent->SetNumericAttributeBase(USIPHealthSet::GetMaxHealthAttribute(), ClampedMaxHealth);
			}

			if (HealthSet->Health.GetCurrentValue() <= 0.0f)
			{
				AbilitySystemComponent->SetNumericAttributeBase(USIPHealthSet::GetHealthAttribute(), ClampedStartingHealth);
			}

			UE_LOG(LogSIPCharacter, Log, TEXT("%s Health initialized to %.2f / %.2f"), *GetName(), HealthSet->Health.GetCurrentValue(), HealthSet->MaxHealth.GetCurrentValue());
		}
	}
}

UAbilitySystemComponent* ASIPCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USIPAbilitySystemComponent* ASIPCharacter::GetSIPAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USIPHealthSet* ASIPCharacter::GetSIPHealthSet() const
{
	if (AbilitySystemComponent)
	{
		return const_cast<USIPHealthSet*>(AbilitySystemComponent->GetSet<USIPHealthSet>());
	}
	return nullptr;
}

float ASIPCharacter::GetCurrentHealth() const
{
	if (const USIPHealthSet* HealthSet = GetSIPHealthSet())
	{
		return HealthSet->Health.GetCurrentValue();
	}

	return 0.0f;
}

float ASIPCharacter::GetMaxHealth() const
{
	if (const USIPHealthSet* HealthSet = GetSIPHealthSet())
	{
		return HealthSet->MaxHealth.GetCurrentValue();
	}

	return 0.0f;
}

bool ASIPCharacter::IsDeadOrDying() const
{
	return bIsDead;
}

bool ASIPCharacter::ApplyCombatDamage(float DamageAmount, AActor* DamageInstigator)
{
	if (DamageAmount <= 0.0f || IsDeadOrDying() || !AbilitySystemComponent)
	{
		return false;
	}

	const USIPHealthSet* HealthSet = GetSIPHealthSet();
	if (!HealthSet)
	{
		UE_LOG(LogSIP, Warning, TEXT("%s has no HealthSet. Damage ignored."), *GetName());
		return false;
	}

	const float CurrentMaxHealth = HealthSet->MaxHealth.GetCurrentValue();
	if (CurrentMaxHealth <= 0.0f)
	{
		UE_LOG(LogSIP, Warning, TEXT("%s has invalid MaxHealth %.2f. Damage ignored until AttributeSet is configured."), *GetName(), CurrentMaxHealth);
		return false;
	}

	const float NewHealth = FMath::Clamp(GetCurrentHealth() - DamageAmount, 0.0f, CurrentMaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(USIPHealthSet::GetHealthAttribute(), NewHealth);

	UE_LOG(LogSIP, Log, TEXT("%s took %.2f damage from %s. Health: %.2f/%.2f"), *GetName(), DamageAmount, *GetNameSafe(DamageInstigator), NewHealth, CurrentMaxHealth);
	return true;
}

bool ASIPCharacter::RestoreHealth(float HealAmount)
{
	if (HealAmount <= 0.0f || !AbilitySystemComponent)
	{
		return false;
	}

	const float NewHealth = FMath::Clamp(GetCurrentHealth() + HealAmount, 0.0f, GetMaxHealth());
	AbilitySystemComponent->SetNumericAttributeBase(USIPHealthSet::GetHealthAttribute(), NewHealth);
	return true;
}

void ASIPCharacter::HandleOutOfHealth()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(SIPGameplayTags::State_Dead);
		AbilitySystemComponent->AddLooseGameplayTag(SIPGameplayTags::Death);
		AbilitySystemComponent->AddLooseGameplayTag(SIPGameplayTags::DeathStarted);
	}

	OnDeathStarted();
	OnDeath();
}

void ASIPCharacter::HandleRevived()
{
	if (!bIsDead)
	{
		return;
	}

	bIsDead = false;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(SIPGameplayTags::State_Dead);
		AbilitySystemComponent->RemoveLooseGameplayTag(SIPGameplayTags::Death);
		AbilitySystemComponent->RemoveLooseGameplayTag(SIPGameplayTags::DeathStarted);
		AbilitySystemComponent->AddLooseGameplayTag(SIPGameplayTags::DeathStopped);
	}

	OnDeathStopped();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(SIPGameplayTags::DeathStopped);
	}
}

void ASIPCharacter::OnDeath()
{
	UE_LOG(LogSIP, Log, TEXT("%s has died."), *GetName());
	K2_OnDeath();
}

void ASIPCharacter::OnDeathStarted()
{
	UE_LOG(LogSIP, Log, TEXT("%s death started."), *GetName());

	DisableInput(Cast<APlayerController>(GetController()));
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	StartDeathDissolve();
	K2_OnDeathStarted();
}

void ASIPCharacter::OnDeathStopped()
{
	UE_LOG(LogSIP, Log, TEXT("%s death stopped (revived)."), *GetName());

	SetActorEnableCollision(true);
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	GetWorldTimerManager().ClearTimer(DeathDissolveTimerHandle);
	DeathDissolveElapsedTime = 0.0f;
	for (UMaterialInstanceDynamic* DynamicMaterial : DeathDissolveMaterials)
	{
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(DeathDissolveParameterName, DeathDissolveStartValue);
		}
	}
	DeathDissolveMaterials.Reset();
	DeathDissolveMeshComponents.Reset();
	K2_OnDeathStopped();
}

void ASIPCharacter::StartDeathDissolve()
{
	if (!bUseDeathDissolve)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DeathDissolveTimerHandle);
	DeathDissolveMaterials.Reset();
	DeathDissolveMeshComponents.Reset();
	DeathDissolveElapsedTime = 0.0f;

	TArray<UMeshComponent*> MeshComponents;
	GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			if (UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(MaterialIndex))
			{
				DynamicMaterial->SetScalarParameterValue(DeathDissolveParameterName, DeathDissolveStartValue);
				DeathDissolveMaterials.Add(DynamicMaterial);
				DeathDissolveMeshComponents.Add(MeshComponent);
			}
		}
	}

	if (DeathDissolveMaterials.Num() == 0)
	{
		UE_LOG(LogSIPCharacter, Warning, TEXT("%s death dissolve found no mesh materials to animate."), *GetName());
		return;
	}

	UE_LOG(LogSIPCharacter, Log, TEXT("%s death dissolve started on %d material instance(s)."), *GetName(), DeathDissolveMaterials.Num());

	GetWorldTimerManager().SetTimer(DeathDissolveTimerHandle, this, &ASIPCharacter::UpdateDeathDissolve, 0.03f, true);
}

void ASIPCharacter::UpdateDeathDissolve()
{
	DeathDissolveElapsedTime += 0.03f;

	const float Alpha = FMath::Clamp(DeathDissolveElapsedTime / FMath::Max(DeathDissolveDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float DissolveValue = FMath::Lerp(DeathDissolveStartValue, DeathDissolveEndValue, Alpha);

	for (UMaterialInstanceDynamic* DynamicMaterial : DeathDissolveMaterials)
	{
		if (DynamicMaterial)
		{
			DynamicMaterial->SetScalarParameterValue(DeathDissolveParameterName, DissolveValue);
		}
	}

	if (Alpha >= 1.0f)
	{
		FinishDeathDissolve();
	}
}

void ASIPCharacter::FinishDeathDissolve()
{
	GetWorldTimerManager().ClearTimer(DeathDissolveTimerHandle);

	if (bDestroyActorOnDissolveComplete)
	{
		Destroy();
	}
}
