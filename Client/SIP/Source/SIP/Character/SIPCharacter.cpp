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

// 创建角色通用基础结构，并挂上项目自定义的 ASC。
ASIPCharacter::ASIPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	AbilitySystemComponent = CreateDefaultSubobject<USIPAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

// 保留稳定的角色启动钩子，给主角和敌人派生类继续扩展。
void ASIPCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogSIPCharacter, Warning, TEXT("%s DDDDDDDDDDDDBeginPlay called."), *GetName());
}

// 在角色离开世界前回收 AbilitySet 赋予的能力和效果。
void ASIPCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AbilitySystemComponent)
	{
		AbilitySetHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	}
	Super::EndPlay(EndPlayReason);
}

// 初始化 ASC、授予配置好的 AbilitySet，并保证一定存在可用的生命属性集。
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

// 面向引擎和玩法系统的 GAS 接口入口。
UAbilitySystemComponent* ASIPCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// 方便需要 SIP 自定义 ASC 辅助函数的代码路径直接拿到具体类型。
USIPAbilitySystemComponent* ASIPCharacter::GetSIPAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// 生命属性查询统一经过 ASC，兼容运行时动态挂载的属性集。
USIPHealthSet* ASIPCharacter::GetSIPHealthSet() const
{
	if (AbilitySystemComponent)
	{
		return const_cast<USIPHealthSet*>(AbilitySystemComponent->GetSet<USIPHealthSet>());
	}
	return nullptr;
}

// 即便属性集缺失，也安全读取当前生命值。
float ASIPCharacter::GetCurrentHealth() const
{
	if (const USIPHealthSet* HealthSet = GetSIPHealthSet())
	{
		return HealthSet->Health.GetCurrentValue();
	}

	return 0.0f;
}

// 即便属性集缺失，也安全读取当前最大生命值。
float ASIPCharacter::GetMaxHealth() const
{
	if (const USIPHealthSet* HealthSet = GetSIPHealthSet())
	{
		return HealthSet->MaxHealth.GetCurrentValue();
	}

	return 0.0f;
}

// 提供统一的死亡状态查询，供能力和战斗逻辑共用。
bool ASIPCharacter::IsDeadOrDying() const
{
	return bIsDead;
}

// 通过 ASC 直接驱动 Health 属性，应用战斗伤害。
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

// 通过 ASC 回血，保证属性复制和监听方行为一致。
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

// 进入一次性死亡状态，补齐标准死亡标签，并启动死亡表现流程。
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

// 反转死亡阶段标签，并恢复复活流程需要的移动与表现状态。
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

// 原生死亡流程的最终回调，蓝图可通过 K2_OnDeath 继续扩展表现。
void ASIPCharacter::OnDeath()
{
	UE_LOG(LogSIP, Log, TEXT("%s has died."), *GetName());
	K2_OnDeath();
}

// 死亡第一阶段回调：停止交互、关闭移动，并在需要时启动溶解效果。
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

// 复活阶段回调：恢复碰撞、移动，以及溶解材质状态。
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

// 收集 Mesh 材质并启动定时器，按时间推进溶解参数。
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

// 以固定节奏推进溶解动画，Alpha 走满后结束效果。
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

// 停止溶解定时器，并在需要时于效果完成后销毁 Actor。
void ASIPCharacter::FinishDeathDissolve()
{
	GetWorldTimerManager().ClearTimer(DeathDissolveTimerHandle);

	if (bDestroyActorOnDissolveComplete)
	{
		Destroy();
	}
}
