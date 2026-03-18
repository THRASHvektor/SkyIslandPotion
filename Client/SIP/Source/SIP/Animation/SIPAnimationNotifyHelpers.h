#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class USkeletalMeshComponent;

// 自定义 Notify 共用的转发辅助，保证 Gameplay Event 的派发行为一致。
namespace SIPAnimationNotifyHelpers
{
	// 优先走动画桥接组件；如果没有桥接组件，则直接把 Gameplay Event 发给 Owner。
	void SendGameplayEventFromMesh(USkeletalMeshComponent* MeshComp, const FGameplayTag& EventTag);
}
