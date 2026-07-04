#include "World/Elemental/SIPWaterElementalZoneActor.h"

#include "AbilitySystemComponent.h"
#include "Character/SIPCharacter.h"
#include "GameplayEffect.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"
#include "TimerManager.h"

ASIPWaterElementalZoneActor::ASIPWaterElementalZoneActor()
{
	ZoneElementTag = SIPGameplayTags::Element_Water;
	bAllowRepeatedReactions = true;
}

void ASIPWaterElementalZoneActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ElectrifyDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(ElectrifyFinishTimerHandle);
	GetWorldTimerManager().ClearTimer(FreezeFinishTimerHandle);

	FinishFreeze();
	FinishElectrify();

	Super::EndPlay(EndPlayReason);
}

void ASIPWaterElementalZoneActor::OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation)
{
	if (ReactionTag == SIPGameplayTags::Reaction_Electrify)
	{
		ASIPElementReactiveZoneBase::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
		ZoneStateTag = SIPGameplayTags::Zone_Electrified;
		NotifyVisualReactionStarted(ReactionTag, ReactionLocation);
		PlayReactionVFX(ElectrifyStartVFX.Get(), ReactionLocation);
		K2_OnReactionApplied(ReactionTag, ReactionLocation);

		OnElectrifyReaction(ImpactContext);
		UE_LOG(LogSIP, Log, TEXT("%s applied water electrify reaction at %s."), *GetName(), *ReactionLocation.ToString());
	}
	else if (ReactionTag == SIPGameplayTags::Reaction_Freeze)
	{
		ASIPElementReactiveZoneBase::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
		ZoneStateTag = SIPGameplayTags::Zone_Frozen;
		NotifyVisualReactionStarted(ReactionTag, ReactionLocation);
		PlayReactionVFX(FreezeStartVFX.Get(), ReactionLocation);
		K2_OnReactionApplied(ReactionTag, ReactionLocation);

		OnFreezeReaction(ImpactContext);
		UE_LOG(LogSIP, Log, TEXT("%s applied water freeze reaction at %s."), *GetName(), *ReactionLocation.ToString());
	}
	else
	{
		Super::OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);
	}
}

void ASIPWaterElementalZoneActor::OnElectrifyReaction(const FSIPElementImpactContext& ImpactContext)
{
	StartElectrify(ImpactContext);
}

void ASIPWaterElementalZoneActor::StartElectrify(const FSIPElementImpactContext& ImpactContext)
{
	bIsElectrified = true;
	ElectrifyDamageInstigator = ImpactContext.InstigatorActor;

	if (ElectrifyLoopVFX && !ActiveElectrifyLoopVFX)
	{
		const FTransform VFXTransform = ResolveVisualEffectTransform(ElectrifyLoopVFXLocalOffset);
		ActiveElectrifyLoopVFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ElectrifyLoopVFX.Get(),
			VFXTransform.GetLocation(),
			VFXTransform.Rotator(),
			FVector::OneVector,
			false,
			true);
	}

	GetWorldTimerManager().ClearTimer(ElectrifyDamageTimerHandle);
	if (ElectrifyCharacterDamage > 0.0f)
	{
		ApplyElectrifyDamageTick();

		if (ElectrifyDuration > ElectrifyDamageInterval)
		{
			GetWorldTimerManager().SetTimer(
				ElectrifyDamageTimerHandle,
				this,
				&ASIPWaterElementalZoneActor::ApplyElectrifyDamageTick,
				ElectrifyDamageInterval,
				true);
		}
	}

	GetWorldTimerManager().ClearTimer(ElectrifyFinishTimerHandle);
	GetWorldTimerManager().SetTimer(
		ElectrifyFinishTimerHandle,
		this,
		&ASIPWaterElementalZoneActor::FinishElectrify,
		FMath::Max(ElectrifyDuration, 0.05f),
		false);
}

void ASIPWaterElementalZoneActor::ApplyElectrifyDamageTick()
{
	if (ElectrifyCharacterDamage <= 0.0f)
	{
		return;
	}

	const int32 DamagedCount = ApplyDamageToOverlappingCharacters(ElectrifyCharacterDamage, ElectrifyDamageInstigator.Get());
	UE_LOG(LogSIP, Log, TEXT("%s Electrify tick damaged %d character(s)."), *GetName(), DamagedCount);
}

void ASIPWaterElementalZoneActor::FinishElectrify()
{
	const bool bWasElectrified = bIsElectrified || (ActiveElectrifyLoopVFX != nullptr);

	GetWorldTimerManager().ClearTimer(ElectrifyDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(ElectrifyFinishTimerHandle);
	bIsElectrified = false;
	StopLoopingVFX(ActiveElectrifyLoopVFX);

	if (bWasElectrified)
	{
		NotifyVisualReactionFinished(SIPGameplayTags::Reaction_Electrify);
	}

	UE_LOG(LogSIP, Log, TEXT("%s electrify finished."), *GetName());
}

void ASIPWaterElementalZoneActor::OnFreezeReaction(const FSIPElementImpactContext& ImpactContext)
{
	StartFreeze(ImpactContext);
}

void ASIPWaterElementalZoneActor::StartFreeze(const FSIPElementImpactContext& ImpactContext)
{
	bIsFrozen = true;

	if (FreezeLoopVFX && !ActiveFreezeLoopVFX)
	{
		const FTransform VFXTransform = ResolveVisualEffectTransform(FreezeLoopVFXLocalOffset);
		ActiveFreezeLoopVFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			FreezeLoopVFX.Get(),
			VFXTransform.GetLocation(),
			VFXTransform.Rotator(),
			FVector::OneVector,
			false,
			true);
	}

	const int32 AffectedCount = ApplyFreezeSlowEffectToOverlappingCharacters(ImpactContext.InstigatorActor.Get());
	UE_LOG(LogSIP, Log, TEXT("%s applied freeze slow GameplayEffect to %d character(s)."), *GetName(), AffectedCount);

	GetWorldTimerManager().ClearTimer(FreezeFinishTimerHandle);
	GetWorldTimerManager().SetTimer(
		FreezeFinishTimerHandle,
		this,
		&ASIPWaterElementalZoneActor::FinishFreeze,
		FMath::Max(FreezeDuration, 0.05f),
		false);
}

int32 ASIPWaterElementalZoneActor::ApplyFreezeSlowEffectToOverlappingCharacters(AActor* EffectInstigator)
{
	if (!FreezeSlowGameplayEffectClass)
	{
		UE_LOG(LogSIP, Warning, TEXT("%s freeze slow requested, but FreezeSlowGameplayEffectClass is not configured."), *GetName());
		return 0;
	}

	const UGameplayEffect* FreezeSlowEffectCDO = FreezeSlowGameplayEffectClass->GetDefaultObject<UGameplayEffect>();
	if (!FreezeSlowEffectCDO)
	{
		return 0;
	}

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ASIPCharacter::StaticClass());

	int32 AppliedEffectCount = 0;
	for (AActor* OverlappingActor : OverlappingActors)
	{
		ASIPCharacter* Character = Cast<ASIPCharacter>(OverlappingActor);
		if (!Character || Character->IsDeadOrDying())
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = Character->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);
		EffectContext.AddInstigator(EffectInstigator, this);

		const FActiveGameplayEffectHandle EffectHandle = TargetASC->ApplyGameplayEffectToSelf(
			FreezeSlowEffectCDO,
			1.0f,
			EffectContext);

		if (EffectHandle.IsValid())
		{
			++AppliedEffectCount;
		}
	}

	return AppliedEffectCount;
}

void ASIPWaterElementalZoneActor::FinishFreeze()
{
	const bool bWasFrozen = bIsFrozen || (ActiveFreezeLoopVFX != nullptr);

	GetWorldTimerManager().ClearTimer(FreezeFinishTimerHandle);

	bIsFrozen = false;
	StopLoopingVFX(ActiveFreezeLoopVFX);

	if (bWasFrozen)
	{
		NotifyVisualReactionFinished(SIPGameplayTags::Reaction_Freeze);
	}

	UE_LOG(LogSIP, Log, TEXT("%s freeze finished."), *GetName());
}

void ASIPWaterElementalZoneActor::StopLoopingVFX(TObjectPtr<UNiagaraComponent>& VFXComponent)
{
	if (!VFXComponent)
	{
		return;
	}

	VFXComponent->Deactivate();
	VFXComponent->DestroyComponent();
	VFXComponent = nullptr;
}
