// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "SIPAnimNotifyState_GameplayEventWindow.generated.h"

/**
 * Z 说明：
 * 自定义 Notify State，用于描述一个持续时间内的动画事件窗口。
 *
 * 用途：
 * 1. 窗口开始时发送 BeginEventTag。
 * 2. 窗口结束时发送 EndEventTag。
 * 3. 适合攻击命中窗口、招架窗口等需要起止边界的动画逻辑。
 */
UCLASS(meta = (DisplayName = "SIP Gameplay Event Window"))
class SIP_API USIPAnimNotifyState_GameplayEventWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// Z 说明：窗口开始时发送 BeginEventTag
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	
	// Z 说明：窗口结束时发送 EndEventTag
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
	// Z 说明：在编辑器轨道中显示“开始标签 -> 结束标签”
	virtual FString GetNotifyName_Implementation() const override;

	// Z 说明：窗口开始时发送的 Gameplay Tag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Animation", meta = (Categories = "Event.Animation"))
	FGameplayTag BeginEventTag;

	// Z 说明：窗口结束时发送的 Gameplay Tag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SIP|Animation", meta = (Categories = "Event.Animation"))
	FGameplayTag EndEventTag;
};
