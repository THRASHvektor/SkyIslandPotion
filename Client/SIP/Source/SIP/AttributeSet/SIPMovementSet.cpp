// Copyright Epic Games, Inc. All Rights Reserved.

#include "AttributeSet/SIPMovementSet.h"
#include "AbilitySystemInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SIPLogCategory.h"

// 当还没有 GameplayEffect 修改速度时，默认使用这份基础移动速度。
USIPMovementSet::USIPMovementSet()
	: MoveSpeed(600.0f)
{
}

// 属性集本身不直接持有 World，这里转而从外部拥有者获取。
UWorld* USIPMovementSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);
	return Outer->GetWorld();
}

// 当玩法逻辑需要 ASC 时，通过拥有者角色把它解析出来。
UAbilitySystemComponent* USIPMovementSet::GetOwningAbilitySystemComponent() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOuter()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

// 复制移动速度属性，保证模拟代理也能同步更新 CharacterMovement。
void USIPMovementSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(USIPMovementSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

// 在提交新的基础值前先做钳制，避免错误配置写入非法速度。
void USIPMovementSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

// 属性变化后立刻同步到 CharacterMovement，保证移动组件实时生效。
void USIPMovementSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		UE_LOG(LogSIP, Log, TEXT("MoveSpeed changed: %.2f -> %.2f"), OldValue, NewValue);

		if (ACharacter* Character = Cast<ACharacter>(GetOuter()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = NewValue;
			}
		}
	}
}

// 客户端收到复制后的 MoveSpeed 时，也要在本地同步更新 CharacterMovement。
void USIPMovementSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USIPMovementSet, MoveSpeed, OldMoveSpeed);

	// 客户端收到复制值后，同步把速度写回 CharacterMovement。
	if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		if (ACharacter* Character = Cast<ACharacter>(ASC->GetAvatarActor()))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->MaxWalkSpeed = MoveSpeed.GetCurrentValue();
			}
		}
	}
}
