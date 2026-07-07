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
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Ability/SIPAbilitySystemComponent.h"
#include "Ability/SIPAbilitySet.h"
#include "AttributeSet/SIPHealthSet.h"
#include "Combat/SIPCombatStatics.h"
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

// 角色 Z 轴低于关卡 KillZ 时由引擎调用；将坠落死亡纳入项目统一的 GAS 伤害管线。
// 引擎的 KillZ 巡检会每帧持续触发，因此这里做幂等保护：本次生命周期最多处理一次。
void ASIPCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	// 已死亡时直接吞掉：绝不能调 Super::FellOutOfWorld，因为引擎默认实现是
	// SetActorHiddenInGame(true) + Destroy()，会立刻让角色消失，覆盖掉我们正在播的死亡蒙太奇。
	if (IsDeadOrDying())
	{
		return;
	}

	// 一次性保护：只有第一次坠落触发才走完整流程。
	// 后续 tick 的 FellOutOfWorld 调用直接吞掉，避免重复施加 GE / 日志刷屏。
	if (bHasFellOutOfWorld)
	{
		return;
	}
	bHasFellOutOfWorld = true;

	const float HealthBefore = GetCurrentHealth();
	UE_LOG(LogSIPCharacter, Log, TEXT("%s fell out of world (KillZ). Applying %.1f damage via GAS. Health before = %.2f."),
		*GetName(), FellOutOfWorldDamage, HealthBefore);

	// 通过项目的 GAS 伤害通道施加一次性大伤害，让 HealthSet 属性变化自然触发 HandleOutOfHealth。
	// 这样死亡表现（Montage、溶解、K2_OnDeath 等）与普通战斗死亡完全一致。
	const bool bDamageApplied = USIPCombatStatics::ApplyDamageToTarget(this, FellOutOfWorldDamage, this, nullptr, nullptr);
	const float HealthAfter = GetCurrentHealth();

	UE_LOG(LogSIPCharacter, Log, TEXT("%s KillZ GAS damage result: applied=%s, Health %.2f -> %.2f, bIsDead=%s."),
		*GetName(),
		bDamageApplied ? TEXT("true") : TEXT("false"),
		HealthBefore, HealthAfter,
		IsDeadOrDying() ? TEXT("true") : TEXT("false"));

	// 保底：若 GE 未配置、施加失败，或 GE 配置错误导致 Health 完全没变化，直接进入死亡流程。
	// 这样即使 DefaultDamageEffect 资源有问题，角色也不会在虚空里无限下坠。
	if (!IsDeadOrDying() && (!bDamageApplied || HealthAfter >= HealthBefore))
	{
		UE_LOG(LogSIPCharacter, Warning,
			TEXT("%s KillZ damage via GAS did not actually reduce health (check DefaultDamageEffect: Instant + Modifier on USIPHealthSet.Damage with SetByCaller tag SIPData.Damage). Falling back to HandleOutOfHealth."),
			*GetName());
		HandleOutOfHealth();
	}
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

// 通过 GAS GE 流程施加伤害，避免直接写 Health。
// 保留旧接口仅作为渐进式兼容 wrapper，内部完全重定向到 USIPCombatStatics。
bool ASIPCharacter::ApplyCombatDamage(float DamageAmount, AActor* DamageInstigator)
{
	if (DamageAmount <= 0.0f || IsDeadOrDying() || !AbilitySystemComponent)
	{
		return false;
	}

	return USIPCombatStatics::ApplyDamageToTarget(this, DamageAmount, DamageInstigator, nullptr, nullptr);
}

// 通过 GAS GE 流程施加治疗（如未配置默认治疗 GE 则回退到直接写 base。待后续全部接入后可删除回退）。
bool ASIPCharacter::RestoreHealth(float HealAmount)
{
	if (HealAmount <= 0.0f || !AbilitySystemComponent)
	{
		return false;
	}

	if (USIPCombatStatics::ApplyHealToTarget(this, HealAmount, this, nullptr, nullptr))
	{
		return true;
	}

	// Fallback: 目前项目可能还没配默认治疗 GE，此处继续使用旧行为避免回血完全失效。
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
	bHasFellOutOfWorld = false;

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

	// 死亡瞬间打印当前世界坐标和 Mesh 可见性，用来定位"角色消失"是相机脱离还是 Mesh 被隐藏。
	// KillZ 掉落场景下，角色本身位置就在关卡下方虚空中，若相机没有跟随，视觉上就会像"消失"。
	if (const USkeletalMeshComponent* MeshComp = GetMesh())
	{
		UE_LOG(LogSIPCharacter, Log,
			TEXT("%s death diagnostics: Location=%s, MeshHidden=%s, ActorHidden=%s."),
			*GetName(),
			*GetActorLocation().ToString(),
			MeshComp->bHiddenInGame ? TEXT("true") : TEXT("false"),
			IsHidden() ? TEXT("true") : TEXT("false"));
	}

	DisableInput(Cast<APlayerController>(GetController()));
	SetActorEnableCollision(false);

	// 关键：停止移动 + 切到 MOVE_None + 清速度。
	// 若不做，KillZ 掉落场景下角色会继续下坠，引擎每帧重入 FellOutOfWorld，
	// 导致死亡蒙太奇被打断。切到 MOVE_None 后不再受重力影响，角色会"钉"在死亡位置。
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
		MoveComp->SetMovementMode(MOVE_None);
		MoveComp->Velocity = FVector::ZeroVector;
	}

	// 先尝试播放死亡蒙太奇。如果成功且配置为“等待动画”，则推迟溶解，等 OnDeathMontageEnded 触发。
	const float MontageLength = PlayDeathMontage();
	const bool bDeferDissolve = bWaitDeathMontageBeforeDissolve && MontageLength > 0.0f;

	if (!bDeferDissolve)
	{
		StartDeathDissolve();
	}

	K2_OnDeathStarted();
}

// 在角色 Mesh 的 AnimInstance 上播放配置好的死亡蒙太奇，并绑定结束回调。
float ASIPCharacter::PlayDeathMontage()
{
	if (!DeathMontage)
	{
		return 0.0f;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogSIPCharacter, Warning, TEXT("%s cannot play DeathMontage: no AnimInstance on Mesh."), *GetName());
		return 0.0f;
	}

	const float PlayRate = FMath::Max(DeathMontagePlayRate, 0.01f);
	const float Duration = AnimInstance->Montage_Play(DeathMontage, PlayRate);
	if (Duration <= 0.0f)
	{
		UE_LOG(LogSIPCharacter, Warning, TEXT("%s DeathMontage failed to start."), *GetName());
		return 0.0f;
	}

	// 仅针对本次死亡蒙太奇绑定一次性结束回调（End 与中断都会触发）。
	FOnMontageEnded EndedDelegate;
	EndedDelegate.BindUObject(this, &ASIPCharacter::OnDeathMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndedDelegate, DeathMontage);

	UE_LOG(LogSIPCharacter, Log, TEXT("%s death montage started (length=%.2fs, rate=%.2f)."), *GetName(), Duration, PlayRate);
	return Duration;
}

// 死亡蒙太奇播放结束后回调：若配置为“等动画后溶解”，则在此时启动溶解。
void ASIPCharacter::OnDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != DeathMontage)
	{
		return;
	}

	UE_LOG(LogSIPCharacter, Log, TEXT("%s death montage ended (interrupted=%s)."), *GetName(), bInterrupted ? TEXT("true") : TEXT("false"));

	// 只有在“推迟溶解”模式下才需要在此启动；否则 OnDeathStarted 已经启动过了。
	if (bIsDead && bWaitDeathMontageBeforeDissolve)
	{
		StartDeathDissolve();
	}
}

// 复活阶段回调：恢复碰撞、移动，以及溶解材质状态。
void ASIPCharacter::OnDeathStopped()
{
	UE_LOG(LogSIP, Log, TEXT("%s death stopped (revived)."), *GetName());

	SetActorEnableCollision(true);
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	// 复活时如果死亡蒙太奇还在播，先停掉，避免后续回调影响新状态。
	if (DeathMontage)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
			{
				if (AnimInstance->Montage_IsPlaying(DeathMontage))
				{
					AnimInstance->Montage_Stop(0.15f, DeathMontage);
				}
			}
		}
	}

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
