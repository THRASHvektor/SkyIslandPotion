#include "World/Elemental/SIPIceElementalZoneActor.h"

#include "Components/BoxComponent.h"
#include "NiagaraSystem.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

ASIPIceElementalZoneActor::ASIPIceElementalZoneActor()
{
	ZoneElementTag = SIPGameplayTags::Element_Ice;
}

void ASIPIceElementalZoneActor::OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation)
{
	if (ReactionTag == SIPGameplayTags::Reaction_Melt)
	{
		ASIPElementReactiveZoneBase::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
		K2_OnReactionApplied(ReactionTag, ReactionLocation);

		OnMeltReaction(ImpactContext, ReactionLocation);
	}
	else
	{
		Super::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
	}
}

void ASIPIceElementalZoneActor::OnMeltReaction(const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation)
{
	NotifyVisualReactionStarted(SIPGameplayTags::Reaction_Melt, ReactionLocation);

	if (bHideVisualActorOnMelt)
	{
		SetVisualActorHidden(true, true);
	}

	if (bDisableZoneCollisionOnMelt && ZoneBounds)
	{
		ZoneBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	PlayReactionVFX(MeltVFX.Get(), ReactionLocation);
	UE_LOG(LogSIP, Log, TEXT("%s Melt disabled ice support collision."), *GetName());
}
