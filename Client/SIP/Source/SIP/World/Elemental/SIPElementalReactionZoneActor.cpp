#include "World/Elemental/SIPElementalReactionZoneActor.h"

#include "Engine/World.h"
#include "PCGComponent.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

ASIPElementalReactionZoneActor::ASIPElementalReactionZoneActor()
{
	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
}

void ASIPElementalReactionZoneActor::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoGenerateVegetation && ZoneElementTag == SIPGameplayTags::Element_Plant)
	{
		FTimerHandle DummyHandle;
		GetWorldTimerManager().SetTimer(DummyHandle, [this]()
		{
			GenerateVegetation(VegetationSeed);
		}, 0.1f, false);
	}
}

void ASIPElementalReactionZoneActor::OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation)
{
	Super::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
	ApplyReaction(ReactionTag, ReactionLocation);
}

void ASIPElementalReactionZoneActor::ApplyReaction(const FGameplayTag& ReactionTag, const FVector& ImpactLocation)
{
	if (ReactionTag == SIPGameplayTags::Reaction_Burn)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Burning;
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Melt)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Frozen;
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Freeze)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Frozen;
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Electrify)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Electrified;
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Bloom)
	{
		ZoneStateTag = SIPGameplayTags::Zone_Bloomed;
	}

	HideDefaultActors();
	ClearPCGContent();

	if (ReactionTag == SIPGameplayTags::Reaction_Bloom)
	{
		const int32 BloomSeed = VegetationSeed + FMath::RandRange(1000, 9999);
		GenerateVegetation(BloomSeed);
		UE_LOG(LogSIP, Log, TEXT("%s Bloom: vegetation regenerated with seed=%d."), *GetName(), BloomSeed);
	}

	K2_OnReactionApplied(ReactionTag, ImpactLocation);

	UE_LOG(LogSIP, Log, TEXT("%s applied reaction [%s] at %s"),
		*GetName(),
		*ReactionTag.ToString(),
		*ImpactLocation.ToString());
}

void ASIPElementalReactionZoneActor::HideDefaultActors()
{
	for (AActor* Actor : LinkedDefaultActors)
	{
		if (Actor)
		{
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
		}
	}
}

void ASIPElementalReactionZoneActor::GenerateVegetation(int32 Seed)
{
	if (!PCGComponent)
	{
		return;
	}

	if (!PCGComponent->GetGraph())
	{
		UE_LOG(LogSIP, Warning,
			TEXT("%s GenerateVegetation: PCGComponent has no Graph assigned! "
			     "Please set it in Blueprint editor: Components panel -> PCGComponent -> Details -> Graph."),
			*GetName());
		return;
	}

	const int32 UsedSeed = (Seed != 0) ? Seed : VegetationSeed;
	PCGComponent->Seed = UsedSeed;
	PCGComponent->GenerateLocal(false);

	UE_LOG(LogSIP, Log, TEXT("%s vegetation generated (Seed=%d)."), *GetName(), UsedSeed);
}

void ASIPElementalReactionZoneActor::ClearPCGContent()
{
	if (!PCGComponent)
	{
		return;
	}

	PCGComponent->CleanupLocal(true);
	UE_LOG(LogSIP, Log, TEXT("%s PCG content cleared."), *GetName());
}
