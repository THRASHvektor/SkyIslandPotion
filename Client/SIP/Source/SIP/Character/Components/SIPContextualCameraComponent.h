// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Components/SIPComponent.h"
#include "SIPContextualCameraComponent.generated.h"

class ASIPHeroCharacter;
class UCameraComponent;
class USIPHeroAnimationBridgeComponent;
class USpringArmComponent;

UENUM(BlueprintType)
enum class ESIPCameraContext : uint8
{
	Exploration UMETA(DisplayName = "Exploration"),
	Combat UMETA(DisplayName = "Combat"),
	Traversal UMETA(DisplayName = "Traversal")
};

USTRUCT(BlueprintType)
struct FSIPCameraModeSettings
{
	GENERATED_BODY()

	// 当前镜头语境下的目标臂长。
	// 运行时会逐帧插值到该值，避免上下文切换时镜头生硬跳变。
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera")
	float TargetArmLength = 400.0f;

	// 当前镜头语境下的构图偏移，用来控制偏肩、抬高等取景差异。
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera")
	FVector SocketOffset = FVector::ZeroVector;

	// 当前镜头语境下的视野角度。
	// 轻微 FOV 变化可以强化探索、战斗、Traversal 的观感差异。
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera", meta = (ClampMin = "5.0"))
	float FieldOfView = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera")
	bool bEnableCameraLag = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera", meta = (ClampMin = "0.0"))
	float CameraLagSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera")
	bool bEnableCameraRotationLag = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera", meta = (ClampMin = "0.0"))
	float CameraRotationLagSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SIP|Camera")
	bool bDoCollisionTest = true;
};

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPContextualCameraComponent : public USIPComponent
{
	GENERATED_BODY()

public:
	USIPContextualCameraComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "SIP|Camera")
	ESIPCameraContext GetCurrentCameraContext() const { return CurrentCameraContext; }

	UFUNCTION(BlueprintPure, Category = "SIP|Camera")
	bool IsInCameraContext(const ESIPCameraContext Context) const { return CurrentCameraContext == Context; }

	UFUNCTION(BlueprintPure, Category = "SIP|Camera")
	FVector GetViewLocation() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Camera")
	FVector GetViewDirection() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Camera")
	FTransform GetViewTransform() const;

	UFUNCTION(BlueprintCallable, Category = "SIP|Camera")
	void ForceCameraContext(ESIPCameraContext Context, float HoldTime = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "SIP|Camera")
	void ClearForcedCameraContext();

private:
	// 缓存主角侧的摄像机相关组件，只有未就绪时才重试，避免每帧重复查找。
	void CacheOwnerReferences();

	// 在这里统一求当前应使用的镜头语境。
	// 当前优先级：Traversal > Combat > Exploration。
	ESIPCameraContext ResolveDesiredContext() const;

	// 当前“战斗镜头”既包含显式战斗表现态，也包含瞄准/横移这类战斗式移动。
	bool ShouldUseCombatContext() const;

	// FlaskRig 在预备 / 释放阶段需要更聚焦的构图，帮助玩家读清投掷动作。
	bool ShouldUseFlaskRigCastFraming() const;

	const FSIPCameraModeSettings& GetSettingsForContext(ESIPCameraContext Context) const;

	// 每帧把同一套镜头策略统一应用到 CameraBoom / FollowCamera。
	void ApplyCameraSettings(const FSIPCameraModeSettings& Settings, float DeltaTime);

private:
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Camera|Contexts")
	FSIPCameraModeSettings ExplorationSettings;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Camera|Contexts")
	FSIPCameraModeSettings CombatSettings;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Camera|Contexts")
	FSIPCameraModeSettings TraversalSettings;

	// 炼金投掷时使用的战斗特写镜头配置。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Camera|Contexts")
	FSIPCameraModeSettings FlaskRigCastSettings;

	UPROPERTY(EditDefaultsOnly, Category = "SIP|Camera|Contexts", meta = (ClampMin = "0.0"))
	float BlendInterpSpeed = 8.0f;

	// 防止战斗/探索边界来回抖动时频繁切镜头。
	UPROPERTY(EditDefaultsOnly, Category = "SIP|Camera|Contexts", meta = (ClampMin = "0.0"))
	float MinContextHoldTime = 0.15f;

	UPROPERTY(Transient)
	TObjectPtr<ASIPHeroCharacter> OwningHeroCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> FollowCamera = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USIPHeroAnimationBridgeComponent> AnimationBridgeComponent = nullptr;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "SIP|Camera")
	ESIPCameraContext CurrentCameraContext = ESIPCameraContext::Exploration;

	UPROPERTY(Transient)
	ESIPCameraContext ForcedCameraContext = ESIPCameraContext::Exploration;

	bool bHasForcedContext = false;
	float ForcedContextRemainingTime = 0.0f;
	float TimeInCurrentContext = 0.0f;
};
