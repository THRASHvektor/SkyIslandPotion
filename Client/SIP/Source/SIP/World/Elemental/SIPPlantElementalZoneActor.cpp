#include "World/Elemental/SIPPlantElementalZoneActor.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"
#include "TimerManager.h"

ASIPPlantElementalZoneActor::ASIPPlantElementalZoneActor()
{
	ZoneElementTag = SIPGameplayTags::Element_Plant;
}

void ASIPPlantElementalZoneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BurnDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(BurnFinishTimerHandle);
	StopBurningVFX();

	Super::EndPlay(EndPlayReason);
}

void ASIPPlantElementalZoneActor::OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation)
{
	if (ReactionTag == SIPGameplayTags::Reaction_Burn)
	{
		ASIPElementReactiveZoneBase::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
		ZoneStateTag = SIPGameplayTags::Zone_Burning;

		NotifyVisualReactionStarted(ReactionTag, ReactionLocation);
		PlayReactionVFX(BurnStartVFX.Get(), ReactionLocation);
		K2_OnReactionApplied(ReactionTag, ReactionLocation);

		OnBurnReaction(ImpactContext);
		UE_LOG(LogSIP, Log, TEXT("%s applied plant burn reaction at %s."), *GetName(), *ReactionLocation.ToString());
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Bloom)
	{
		ASIPElementReactiveZoneBase::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
		ZoneStateTag = SIPGameplayTags::Zone_Bloomed;
		NotifyVisualReactionStarted(ReactionTag, ReactionLocation);
		K2_OnReactionApplied(ReactionTag, ReactionLocation);

		OnBloomReaction(ReactionLocation);
	}
	else
	{
		Super::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
	}
}

void ASIPPlantElementalZoneActor::OnBurnReaction(const FSIPElementImpactContext& ImpactContext)
{
	StartBurning(ImpactContext);
}

void ASIPPlantElementalZoneActor::StartBurning(const FSIPElementImpactContext& ImpactContext)
{
	if (bIsBurning)
	{
		return;
	}

	bIsBurning = true;
	BurnDamageInstigator = ImpactContext.InstigatorActor;

	const int32 BurningMaterialSlots = ApplyMaterialToVisualActor(BurningMaterial.Get());
	UE_LOG(LogSIP, Log, TEXT("%s switched %d material slot(s) to burning material."), *GetName(), BurningMaterialSlots);

	if (BurningLoopVFX)
	{
		const FTransform VFXTransform = ResolveVisualEffectTransform(BurningLoopVFXLocalOffset);
		ActiveBurningLoopVFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			BurningLoopVFX.Get(),
			VFXTransform.GetLocation(),
			VFXTransform.Rotator(),
			FVector::OneVector,
			false,
			true);
	}

	if (BurnCharacterDamage > 0.0f)
	{
		ApplyBurnDamageTick();

		if (BurnDuration > BurnDamageInterval)
		{
			GetWorldTimerManager().SetTimer(
				BurnDamageTimerHandle,
				this,
				&ASIPPlantElementalZoneActor::ApplyBurnDamageTick,
				BurnDamageInterval,
				true);
		}
	}

	GetWorldTimerManager().SetTimer(
		BurnFinishTimerHandle,
		this,
		&ASIPPlantElementalZoneActor::FinishBurning,
		FMath::Max(BurnDuration, 0.05f),
		false);
}

void ASIPPlantElementalZoneActor::ApplyBurnDamageTick()
{
	if (BurnCharacterDamage <= 0.0f)
	{
		return;
	}

	const int32 DamagedCount = ApplyDamageToOverlappingCharacters(BurnCharacterDamage, BurnDamageInstigator.Get());
	UE_LOG(LogSIP, Log, TEXT("%s Burn tick damaged %d character(s)."), *GetName(), DamagedCount);
}

void ASIPPlantElementalZoneActor::FinishBurning()
{
	GetWorldTimerManager().ClearTimer(BurnDamageTimerHandle);
	bIsBurning = false;

	const int32 BurntMaterialSlots = ApplyMaterialToVisualActor(BurntMaterial.Get());
	StopBurningVFX();
	NotifyVisualReactionFinished(SIPGameplayTags::Reaction_Burn);

	UE_LOG(LogSIP, Log, TEXT("%s burn finished. Switched %d material slot(s) to burnt material."), *GetName(), BurntMaterialSlots);
}

void ASIPPlantElementalZoneActor::StopBurningVFX()
{
	if (!ActiveBurningLoopVFX)
	{
		return;
	}

	ActiveBurningLoopVFX->Deactivate();
	ActiveBurningLoopVFX->DestroyComponent();
	ActiveBurningLoopVFX = nullptr;
}

void ASIPPlantElementalZoneActor::OnBloomReaction(const FVector& ReactionLocation)
{
	PlayReactionVFX(BloomVFX.Get(), ReactionLocation);
	UE_LOG(LogSIP, Log, TEXT("%s Bloom reaction completed."), *GetName());
}
