// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * USIPMovementSet 是角色移动属性集
 * 从 USIPHealthSet 中拆分出来，遵循单一职责原则
 *
 * 包含属性：
 * - MoveSpeed：角色移动速度，PostAttributeChange 时同步到 CharacterMovement
 *
 * 注意：GE_Sprint 蓝图中属性路径需从
 *   SIPHealthSet.MoveSpeed → SIPMovementSet.MoveSpeed
 */

#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "SIPMovementSet.generated.h"

UCLASS()
class SIP_API USIPMovementSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	USIPMovementSet();

	UWorld* GetWorld() const override;
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(USIPMovementSet, MoveSpeed);

	// 角色移动速度，PostAttributeChange 会同步到 CharacterMovement->MaxWalkSpeed
	UPROPERTY(BlueprintReadOnly, Category = "SIP|Movement", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

protected:
	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
};
