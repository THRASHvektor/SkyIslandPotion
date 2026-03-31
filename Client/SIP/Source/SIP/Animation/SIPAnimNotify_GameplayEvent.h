// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "SIPAnimNotify_GameplayEvent.generated.h"

/**
 * 自定义单帧动画 Notify。
 *
 * 用途：
 * 1. 在蒙太奇或动画序列的某一帧发送 Gameplay Event。
 * 2. 如果角色带有 HeroAnimationBridgeComponent，则优先通过桥接组件转发。
 * 3. 如果没有桥接组件，则直接向 Owner Actor 发送 Gameplay Event。
 */
UCLASS(meta = (DisplayName = "SIP Gameplay Event"))
class SIP_API USIPAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
	virtual FString GetNotifyName_Implementation() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Animation", meta = (Categories = "Event.Animation"))
	FGameplayTag EventTag;
};
