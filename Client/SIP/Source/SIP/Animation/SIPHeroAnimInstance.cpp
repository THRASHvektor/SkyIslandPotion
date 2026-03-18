// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/SIPHeroAnimInstance.h"

#include "Character/Components/SIPHeroAnimationBridgeComponent.h"
#include "Character/SIPHeroCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

/**
 * Z 说明：
 * SIPHeroAnimInstance.cpp 实现主角动画实例基础类。
 *
 * 主要功能：
 * 1. 在动画实例初始化时定位主角与桥接组件。
 * 2. 每帧从桥接组件同步移动、战斗和事件状态。
 * 3. 当桥接组件尚未就绪时，提供一份基础移动状态回退。
 */
void USIPHeroAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheAnimationReferences();
	SyncFromAnimationBridge();
}

/**
 * Z 说明：NativeUpdateAnimation
 * 每帧刷新当前 AnimInstance 所需的表现层数据
 */
void USIPHeroAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningHeroCharacter || !AnimationBridgeComponent)
	{
		CacheAnimationReferences();
	}

	SyncFromAnimationBridge();
}

/**
 * Z 说明：CacheAnimationReferences
 * 重新查找主角角色与动画桥接组件引用
 */
void USIPHeroAnimInstance::CacheAnimationReferences()
{
	ASIPHeroCharacter* HeroCharacter = Cast<ASIPHeroCharacter>(TryGetPawnOwner());
	if (!HeroCharacter)
	{
		HeroCharacter = Cast<ASIPHeroCharacter>(GetOwningActor());
	}

	OwningHeroCharacter = HeroCharacter;
	AnimationBridgeComponent = HeroCharacter ? HeroCharacter->GetHeroAnimationBridgeComponent() : nullptr;
}

/**
 * Z 说明：SyncFromAnimationBridge
 * 优先从桥接组件同步数据，没有桥接组件时回退到角色基础移动状态
 */
void USIPHeroAnimInstance::SyncFromAnimationBridge()
{
	USIPHeroAnimationBridgeComponent* AnimationBridge = AnimationBridgeComponent.Get();
	ASIPHeroCharacter* HeroCharacter = OwningHeroCharacter.Get();

	if (AnimationBridge)
	{
		bHasAnimationBridge = true;
		GroundSpeed = AnimationBridge->GetGroundSpeed();
		Velocity = AnimationBridge->GetVelocity();
		bIsMoving = AnimationBridge->IsMoving();
		bIsFalling = AnimationBridge->IsFalling();
		bIsJumping = AnimationBridge->IsJumping();
		bIsInCombatPresentation = AnimationBridge->IsInCombatPresentation();
		ActiveAnimationStateTags = AnimationBridge->GetAnimationStateTags();
		LastRequestedActionTag = AnimationBridge->GetLastRequestedActionTag();
		return;
	}

	ResetAnimationState();

	if (!HeroCharacter)
	{
		return;
	}

	Velocity = HeroCharacter->GetVelocity();
	GroundSpeed = FVector(Velocity.X, Velocity.Y, 0.0f).Size();
	bIsMoving = GroundSpeed > KINDA_SMALL_NUMBER;

	if (const UCharacterMovementComponent* MovementComponent = HeroCharacter->GetCharacterMovement())
	{
		bIsFalling = MovementComponent->IsFalling();
		bIsJumping = bIsFalling && Velocity.Z > 0.0f;
	}
}

// Z 说明：清空动画实例缓存的桥接状态，避免引用失效时保留旧值
void USIPHeroAnimInstance::ResetAnimationState()
{
	GroundSpeed = 0.0f;
	Velocity = FVector::ZeroVector;
	bIsMoving = false;
	bIsFalling = false;
	bIsJumping = false;
	bHasAnimationBridge = false;
	bIsInCombatPresentation = false;
	ActiveAnimationStateTags.Reset();
	LastRequestedActionTag = FGameplayTag();
}
