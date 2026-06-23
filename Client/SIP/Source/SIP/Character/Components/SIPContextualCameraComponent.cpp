// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Components/SIPContextualCameraComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/Components/SIPHeroAnimationBridgeComponent.h"
#include "Character/SIPHeroCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "SIPGameplayTags.h"

USIPContextualCameraComponent::USIPContextualCameraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;

	ExplorationSettings.TargetArmLength = 400.0f;
	ExplorationSettings.SocketOffset = FVector(0.0f, 0.0f, 15.0f);
	ExplorationSettings.FieldOfView = 90.0f;
	ExplorationSettings.bEnableCameraLag = true;
	ExplorationSettings.CameraLagSpeed = 10.0f;
	ExplorationSettings.bEnableCameraRotationLag = false;
	ExplorationSettings.CameraRotationLagSpeed = 10.0f;
	ExplorationSettings.bDoCollisionTest = true;

	CombatSettings.TargetArmLength = 340.0f;
	CombatSettings.SocketOffset = FVector(0.0f, 48.0f, 22.0f);
	CombatSettings.FieldOfView = 85.0f;
	CombatSettings.bEnableCameraLag = true;
	CombatSettings.CameraLagSpeed = 14.0f;
	CombatSettings.bEnableCameraRotationLag = true;
	CombatSettings.CameraRotationLagSpeed = 12.0f;
	CombatSettings.bDoCollisionTest = true;

	TraversalSettings.TargetArmLength = 260.0f;
	TraversalSettings.SocketOffset = FVector(0.0f, 24.0f, 34.0f);
	TraversalSettings.FieldOfView = 82.0f;
	TraversalSettings.bEnableCameraLag = true;
	TraversalSettings.CameraLagSpeed = 18.0f;
	TraversalSettings.bEnableCameraRotationLag = true;
	TraversalSettings.CameraRotationLagSpeed = 18.0f;
	TraversalSettings.bDoCollisionTest = true;

	FlaskRigCastSettings.TargetArmLength = 300.0f;
	FlaskRigCastSettings.SocketOffset = FVector(0.0f, 62.0f, 28.0f);
	FlaskRigCastSettings.FieldOfView = 80.0f;
	FlaskRigCastSettings.bEnableCameraLag = true;
	FlaskRigCastSettings.CameraLagSpeed = 16.0f;
	FlaskRigCastSettings.bEnableCameraRotationLag = true;
	FlaskRigCastSettings.CameraRotationLagSpeed = 14.0f;
	FlaskRigCastSettings.bDoCollisionTest = true;
}

void USIPContextualCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheOwnerReferences();
}

void USIPContextualCameraComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwningHeroCharacter || !CameraBoom || !FollowCamera)
	{
		CacheOwnerReferences();
	}

	if (!OwningHeroCharacter || !CameraBoom || !FollowCamera)
	{
		return;
	}

	if (bHasForcedContext)
	{
		// 强制镜头语境是给少量特殊镜头时刻准备的“逃生口”，
		// 避免外部玩法代码直接伸手去改 SpringArm 运行时参数。
		if (ForcedContextRemainingTime > 0.0f)
		{
			ForcedContextRemainingTime = FMath::Max(0.0f, ForcedContextRemainingTime - DeltaTime);
		}
		else
		{
			ClearForcedCameraContext();
		}
	}

	const ESIPCameraContext DesiredContext = bHasForcedContext ? ForcedCameraContext : ResolveDesiredContext();
	if (DesiredContext != CurrentCameraContext)
	{
		// Traversal 允许更快接管镜头；
		// 其他较软的上下文切换则等待一个短暂保持时间，减少探索/战斗边界抖动。
		const bool bCanInterruptCurrentContext =
			TimeInCurrentContext >= MinContextHoldTime ||
			DesiredContext == ESIPCameraContext::Traversal ||
			CurrentCameraContext == ESIPCameraContext::Traversal;

		if (bCanInterruptCurrentContext)
		{
			CurrentCameraContext = DesiredContext;
			TimeInCurrentContext = 0.0f;
		}
	}

	TimeInCurrentContext += DeltaTime;
	ApplyCameraSettings(GetSettingsForContext(CurrentCameraContext), DeltaTime);
}

FVector USIPContextualCameraComponent::GetViewLocation() const
{
	if (FollowCamera)
	{
		return FollowCamera->GetComponentLocation();
	}

	if (OwningHeroCharacter)
	{
		return OwningHeroCharacter->GetActorLocation();
	}

	return FVector::ZeroVector;
}

FVector USIPContextualCameraComponent::GetViewDirection() const
{
	if (FollowCamera)
	{
		return FollowCamera->GetForwardVector();
	}

	if (OwningHeroCharacter)
	{
		return OwningHeroCharacter->GetActorForwardVector();
	}

	return FVector::ForwardVector;
}

FTransform USIPContextualCameraComponent::GetViewTransform() const
{
	if (FollowCamera)
	{
		return FollowCamera->GetComponentTransform();
	}

	return FTransform(GetViewDirection().Rotation(), GetViewLocation());
}

void USIPContextualCameraComponent::ForceCameraContext(const ESIPCameraContext Context, const float HoldTime)
{
	bHasForcedContext = true;
	ForcedCameraContext = Context;
	ForcedContextRemainingTime = HoldTime;
	CurrentCameraContext = Context;
	TimeInCurrentContext = 0.0f;
}

void USIPContextualCameraComponent::ClearForcedCameraContext()
{
	bHasForcedContext = false;
	ForcedContextRemainingTime = 0.0f;
}

void USIPContextualCameraComponent::CacheOwnerReferences()
{
	OwningHeroCharacter = Cast<ASIPHeroCharacter>(GetOwner());
	CameraBoom = OwningHeroCharacter ? OwningHeroCharacter->GetCameraBoom() : nullptr;
	FollowCamera = OwningHeroCharacter ? OwningHeroCharacter->GetFollowCamera() : nullptr;
	AnimationBridgeComponent = OwningHeroCharacter ? OwningHeroCharacter->GetHeroAnimationBridgeComponent() : nullptr;
}

ESIPCameraContext USIPContextualCameraComponent::ResolveDesiredContext() const
{
	if (!OwningHeroCharacter)
	{
		return ESIPCameraContext::Exploration;
	}

	// Traversal 是对空间关系最敏感的动作状态，因此镜头优先级最高。
	if (OwningHeroCharacter->IsTraversalActive())
	{
		return ESIPCameraContext::Traversal;
	}

	if (ShouldUseCombatContext())
	{
		return ESIPCameraContext::Combat;
	}

	return ESIPCameraContext::Exploration;
}

bool USIPContextualCameraComponent::ShouldUseCombatContext() const
{
	if (!OwningHeroCharacter)
	{
		return false;
	}

	// 瞄准/横移本身就说明玩家希望进入更紧凑的战斗构图，
	// 不必等到蒙太奇或战斗表现态真正抬起后再切镜头。
	if (OwningHeroCharacter->WantsToAim() || OwningHeroCharacter->WantsToStrafe())
	{
		return true;
	}

	return AnimationBridgeComponent && AnimationBridgeComponent->IsInCombatPresentation();
}

bool USIPContextualCameraComponent::ShouldUseFlaskRigCastFraming() const
{
	if (!AnimationBridgeComponent)
	{
		return false;
	}

	const bool bIsFlaskRig =
		AnimationBridgeComponent->HasCurrentWeaponModuleTag(SIPGameplayTags::State_Combat_WeaponModule_FlaskRig);

	const bool bIsActiveCastPhase =
		AnimationBridgeComponent->IsInCastPhase(SIPGameplayTags::State_Combat_Cast_PreCast) ||
		AnimationBridgeComponent->IsInCastPhase(SIPGameplayTags::State_Combat_Cast_Release);

	return bIsFlaskRig && bIsActiveCastPhase;
}

const FSIPCameraModeSettings& USIPContextualCameraComponent::GetSettingsForContext(const ESIPCameraContext Context) const
{
	switch (Context)
	{
	case ESIPCameraContext::Combat:
		return ShouldUseFlaskRigCastFraming() ? FlaskRigCastSettings : CombatSettings;
	case ESIPCameraContext::Traversal:
		return TraversalSettings;
	default:
		return ExplorationSettings;
	}
}

void USIPContextualCameraComponent::ApplyCameraSettings(const FSIPCameraModeSettings& Settings, const float DeltaTime)
{
	const float EffectiveInterpSpeed = BlendInterpSpeed > 0.0f ? BlendInterpSpeed : BIG_NUMBER;

	if (CameraBoom)
	{
		// 统一由这个组件接管 CameraBoom 的运行时调参，
		// 避免 Hero/Ability 在项目各处散落一次性镜头写操作。
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, Settings.TargetArmLength, DeltaTime, EffectiveInterpSpeed);
		CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, Settings.SocketOffset, DeltaTime, EffectiveInterpSpeed);
		CameraBoom->bEnableCameraLag = Settings.bEnableCameraLag;
		CameraBoom->CameraLagSpeed = Settings.CameraLagSpeed;
		CameraBoom->bEnableCameraRotationLag = Settings.bEnableCameraRotationLag;
		CameraBoom->CameraRotationLagSpeed = Settings.CameraRotationLagSpeed;
		CameraBoom->bDoCollisionTest = Settings.bDoCollisionTest;
	}

	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, Settings.FieldOfView, DeltaTime, EffectiveInterpSpeed));
	}
}
