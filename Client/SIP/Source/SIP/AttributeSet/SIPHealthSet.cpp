// Copyright Epic Games, Inc. All Rights Reserved.

#include "AttributeSet/SIPHealthSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/SIPCharacter.h"
#include "SIPLogCategory.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// 本文件：USIPHealthSet 实现
// 目的：管理角色的 Health、MaxHealth、Healing、MoveSpeed 等属性
// 说明：此 AttributeSet 负责属性的初始值、同步、变更时的修正与通知。
// 注释里会指出关键行为、潜在边界情况、以及和 Lyra 风格的对比建议。

USIPHealthSet::USIPHealthSet()
	: Health(100.0f)
	, MaxHealth(100.0f)
	, Healing(0.0f)
	, MoveSpeed(600.0f)
{
	// 初始化并缓存常用的 GameplayTag（用于在 GameplayEffects / Ability 中标识属性变化）
	Tag_MaxHealthChanged = FGameplayTag::RequestGameplayTag(FName("Health.MaxChanged"));
	Tag_HealthChanged = FGameplayTag::RequestGameplayTag(FName("Health.Changed"));
}

// 获取 World 指针：AttributeSet 的 Outer 期望为 AbilitySystemComponent
UWorld* USIPHealthSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

// AttributeSet 的 Outer 是 OwnerActor（角色），通过 IAbilitySystemInterface 接口获取正确的 ASC
UAbilitySystemComponent* USIPHealthSet::GetOwningAbilitySystemComponent() const
{
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOuter()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

// 注册需要复制的属性及其通知行为
void USIPHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// REPNOTIFY_Always 表示无论值是否真正变化都会触发 OnRep 通知（在某些情况下这能保证客户端与服务器状态一致）
	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, Healing, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPHealthSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

// 在属性基础值被改变前拦截（Base value change），用于对即将设置的基础值进行裁剪/限制
void USIPHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// 统一将边界裁剪逻辑封装到 ClampAttribute 中，避免重复
	ClampAttribute(Attribute, NewValue);
}

// 在属性当前值被改变前拦截（比如 GameplayEffect 修改属性时）
void USIPHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 示例日志：跟踪 Health 的新值（仅开发/调试用）
	if (Attribute == GetHealthAttribute())
	{
		UE_LOG(LogSIP, Warning, TEXT("USIPHealthSet::PreAttributeChange Health: %f"), NewValue);
	}
}

// 在属性值实际改变之后调用（OldValue -> NewValue）
void USIPHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 当 MaxHealth 改变时，如果当前的 Health 超过新的 MaxHealth，需要把 Health 的 base 值也限制到新的 Max
	if (Attribute == GetMaxHealthAttribute())
	{
		// 注意：这里使用 GetHealthAttribute().GetNumericValue(this) 来获取当前 Health 值
		if (GetHealthAttribute().GetNumericValue(this) > NewValue)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ASC)
			{
				// 通过 SetNumericAttributeBase 调整 Health 的 base 值到新上限
				// 这会影响以 base 为基础计算的最终属性（特别是在存在 Buff/Debuff 时，需要确认预期行为）
				ASC->SetNumericAttributeBase(GetHealthAttribute(), NewValue);
			}
		}
	}

	// Health 转换状态检测（从 <=0 到 >0 则视为复活；从 >0 到 <=0 则视为死亡）
	if (Attribute == GetHealthAttribute())
	{
		if (OldValue <= 0.0f && NewValue > 0.0f)
		{
			UE_LOG(LogSIP, Log, TEXT("Player Revived"));

			if (ASIPCharacter* SIPCharacter = Cast<ASIPCharacter>(GetOuter()))
			{
				SIPCharacter->HandleRevived();
			}
		}
		else if (OldValue > 0.0f && NewValue <= 0.0f)
		{
			UE_LOG(LogSIP, Log, TEXT("Player Died"));

			if (ASIPCharacter* SIPCharacter = Cast<ASIPCharacter>(GetOuter()))
			{
				SIPCharacter->HandleOutOfHealth();
			}
		}
	}

	// MoveSpeed 变化的处理：同步到角色的 CharacterMovement->MaxWalkSpeed
	if (Attribute == GetMoveSpeedAttribute())
	{
		UE_LOG(LogSIP, Log, TEXT("MoveSpeed changed: %f -> %f"), OldValue, NewValue);

		// AttributeSet 是用 NewObject<>(ASC->GetOwner(), ...) 创建的，Outer 就是 Character，直接 Cast 最可靠
		if (ACharacter* Character = Cast<ACharacter>(GetOuter()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = NewValue;
				UE_LOG(LogSIP, Log, TEXT("MaxWalkSpeed synced to %f"), NewValue);
			}
		}
	}
}



void USIPHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// 将各属性的有效区间/下界进行约束，避免出现非法值
	if (Attribute == GetHealthAttribute())
	{
		// Health 的上限依赖当前 MaxHealth（使用 GetNumericValue 读取当前 MaxHealth）
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealthAttribute().GetNumericValue(this));
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// MaxHealth 最低为 1，避免除以 0 或逻辑错误
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		// 移动速度不能为负
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

// 以下 OnRep_* 系列是在属性复制到客户端时触发的回调，用于在客户端做额外处理或触发蓝图事件
void USIPHealthSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, Health, OldHealth);
}

void USIPHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, MaxHealth, OldMaxHealth);
}

void USIPHealthSet::OnRep_Healing(const FGameplayAttributeData& OldHealing)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, Healing, OldHealing);
}

void USIPHealthSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPHealthSet, MoveSpeed, OldMoveSpeed);

	// 当属性复制到客户端时，也需要将 MoveSpeed 同步到本地的 CharacterMovement
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (ASC)
	{
		AActor* Avatar = ASC->GetAvatarActor();
		if (ACharacter* Character = Cast<ACharacter>(Avatar))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = MoveSpeed.GetCurrentValue();
				UE_LOG(LogSIP, Verbose, TEXT("OnRep_MoveSpeed synced MaxWalkSpeed to %f"), MoveSpeed.GetCurrentValue());
			}
		}
	}
}
