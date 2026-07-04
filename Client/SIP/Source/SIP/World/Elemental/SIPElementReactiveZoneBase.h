#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "World/SIPElementImpactReceiver.h"
#include "SIPElementReactiveZoneBase.generated.h"

class UBoxComponent;
class UChildActorComponent;
class UMaterialInterface;
class UNiagaraSystem;

UCLASS(Abstract, Blueprintable)
class SIP_API ASIPElementReactiveZoneBase : public AActor, public ISIPElementImpactReceiver
{
	GENERATED_BODY()

public:
	ASIPElementReactiveZoneBase();

	virtual void ReceiveElementImpact_Implementation(const FSIPElementImpactContext& ImpactContext) override;

	UFUNCTION(BlueprintCallable, Category = "SIP|Elemental")
	void ReceiveElementHit(const FGameplayTag& IncomingElement, const FVector& ImpactLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Zone")
	TObjectPtr<UBoxComponent> ZoneBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Zone|Visual")
	TObjectPtr<UChildActorComponent> VisualActorComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Zone|Visual")
	bool bDisableVisualActorCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Zone")
	FGameplayTag ZoneElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Zone|Reaction", meta = (ClampMin = "0.1"))
	float MaxReactionHealth = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Zone|Reaction")
	float ReactionHealth = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Zone|Reaction")
	bool bHasTriggeredReaction = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Zone|Reaction")
	FGameplayTag TriggeredReactionTag;

	UFUNCTION(BlueprintCallable, Category = "SIP|Zone|Reaction")
	float GetReactionHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "SIP|Zone|Visual")
	AActor* GetVisualActor() const;

protected:
	virtual void BeginPlay() override;

	virtual void OnSurfaceDamageAccepted(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext);
	virtual void OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Zone|Reaction", DisplayName = "On Surface Damage Accepted")
	void K2_OnSurfaceDamageAccepted(FGameplayTag ReactionTag, const FSIPElementImpactContext& ImpactContext);

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Zone|Reaction", DisplayName = "On Element Reaction Triggered")
	void K2_OnReactionTriggered(FGameplayTag ReactionTag, const FSIPElementImpactContext& ImpactContext, FVector ReactionLocation);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Zone")
	FGameplayTag ZoneStateTag;

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Zone", DisplayName = "On Reaction Applied")
	void K2_OnReactionApplied(FGameplayTag ReactionTag, FVector Location);

	bool bAllowRepeatedReactions = false;

	FVector ResolveReactionLocation(const FSIPElementImpactContext& ImpactContext) const;

	int32 ApplyDamageToOverlappingCharacters(float DamageAmount, AActor* DamageInstigator);
	int32 ApplyMaterialToVisualActor(UMaterialInterface* Material);
	void SetVisualActorHidden(bool bHide, bool bDisableCollision);
	void NotifyVisualReactionStarted(const FGameplayTag& ReactionTag, const FVector& ReactionLocation) const;
	void NotifyVisualReactionFinished(const FGameplayTag& ReactionTag) const;
	FTransform ResolveVisualEffectTransform(const FVector& LocalOffset = FVector::ZeroVector) const;
	void PlayReactionVFX(UNiagaraSystem* VFX, const FVector& Location) const;

	static int32 ApplyMaterialToActorMeshes(AActor* TargetActor, UMaterialInterface* Material);
};
