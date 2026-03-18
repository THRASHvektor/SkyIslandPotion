// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/SIPAnimNotify_GameplayEvent.h"

#include "Animation/SIPAnimationNotifyHelpers.h"

/**
 * Z 说明：
 * SIPAnimNotify_GameplayEvent.cpp 实现单帧动画事件 Notify。
 *
 * 发送策略：
 * 1. 优先让桥接组件处理事件，保持动画层与 GAS 的统一入口。
 * 2. 没有桥接组件时，直接向拥有者 Actor 发送 Gameplay Event。
 */
void USIPAnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !EventTag.IsValid())
	{
		return;
	}

	SIPAnimationNotifyHelpers::SendGameplayEventFromMesh(MeshComp, EventTag);
}

// Z 说明：让编辑器里的 Notify 名称直接显示事件标签，便于调试
FString USIPAnimNotify_GameplayEvent::GetNotifyName_Implementation() const
{
	return EventTag.IsValid() ? EventTag.ToString() : Super::GetNotifyName_Implementation();
}
