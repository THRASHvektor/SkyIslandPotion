// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/SIPAnimNotifyState_GameplayEventWindow.h"

#include "Animation/SIPAnimationNotifyHelpers.h"

/**
 * Z 说明：
 * SIPAnimNotifyState_GameplayEventWindow.cpp 实现持续型动画事件窗口。
 *
 * 设计思路：
 * 1. Begin / End 统一复用同一套发送函数。
 * 2. 优先通过桥接组件转发，保证动画层事件入口一致。
 * 3. 没有桥接组件时，直接向 Owner Actor 发送 Gameplay Event。
 */
namespace
{
	// Z 说明：统一发送动画事件，避免 Begin/End 重复实现相同逻辑
	void SendNotifyEvent(USkeletalMeshComponent* MeshComp, const FGameplayTag& EventTag)
	{
		SIPAnimationNotifyHelpers::SendGameplayEventFromMesh(MeshComp, EventTag);
	}
}

// Z 说明：Notify State 开始时发送 BeginEventTag
void USIPAnimNotifyState_GameplayEventWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	SendNotifyEvent(MeshComp, BeginEventTag);
}

// Z 说明：Notify State 结束时发送 EndEventTag
void USIPAnimNotifyState_GameplayEventWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	SendNotifyEvent(MeshComp, EndEventTag);
}

// Z 说明：让编辑器里的 Notify State 名称直接显示起止标签，便于调整窗口帧段
FString USIPAnimNotifyState_GameplayEventWindow::GetNotifyName_Implementation() const
{
	if (BeginEventTag.IsValid() || EndEventTag.IsValid())
	{
		return FString::Printf(TEXT("%s -> %s"), *BeginEventTag.ToString(), *EndEventTag.ToString());
	}

	return Super::GetNotifyName_Implementation();
}
