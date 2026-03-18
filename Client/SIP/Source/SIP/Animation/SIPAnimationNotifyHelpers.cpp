#include "Animation/SIPAnimationNotifyHelpers.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/Components/SIPHeroAnimationBridgeComponent.h"
#include "Components/SkeletalMeshComponent.h"

// 统一收口 Notify 转发逻辑，保证有桥接和无桥接两种路径行为一致。
void SIPAnimationNotifyHelpers::SendGameplayEventFromMesh(USkeletalMeshComponent* MeshComp, const FGameplayTag& EventTag)
{
	if (!MeshComp || !EventTag.IsValid())
	{
		return;
	}

	if (AActor* OwnerActor = MeshComp->GetOwner())
	{
		if (USIPHeroAnimationBridgeComponent* AnimationBridge = OwnerActor->FindComponentByClass<USIPHeroAnimationBridgeComponent>())
		{
			AnimationBridge->NotifyAnimationEvent(EventTag);
			return;
		}

		FGameplayEventData Payload;
		Payload.EventTag = EventTag;
		Payload.Instigator = OwnerActor;
		Payload.Target = OwnerActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
	}
}
