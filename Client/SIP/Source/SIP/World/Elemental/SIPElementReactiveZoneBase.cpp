#include "World/Elemental/SIPElementReactiveZoneBase.h"

#include "Character/SIPCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "World/Elemental/SIPElementReactionVisual.h"
#include "World/SIPElementReactionSubsystem.h"
#include "SIPLogCategory.h"

ASIPElementReactiveZoneBase::ASIPElementReactiveZoneBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBounds"));
	ZoneBounds->SetBoxExtent(FVector(300.0f, 300.0f, 200.0f));
	ZoneBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ZoneBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneBounds->SetGenerateOverlapEvents(true);
	RootComponent = ZoneBounds;

	VisualActorComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("VisualActor"));
	VisualActorComponent->SetupAttachment(ZoneBounds);
}

void ASIPElementReactiveZoneBase::BeginPlay()
{
	Super::BeginPlay();

	MaxReactionHealth = FMath::Max(MaxReactionHealth, 0.1f);
	ReactionHealth = MaxReactionHealth;
	bHasTriggeredReaction = false;
	TriggeredReactionTag = FGameplayTag::EmptyTag;

	if (bDisableVisualActorCollision)
	{
		if (AActor* VisualActor = GetVisualActor())
		{
			VisualActor->SetActorEnableCollision(false);
		}
	}
}

void ASIPElementReactiveZoneBase::ReceiveElementHit(const FGameplayTag& IncomingElement, const FVector& ImpactLocation)
{
	FSIPElementImpactContext ImpactContext;
	ImpactContext.IncomingElement = IncomingElement;
	ImpactContext.SurfaceDamage = FMath::Max(MaxReactionHealth, 0.1f);
	ImpactContext.ImpactLocation = ImpactLocation;
	ImpactContext.SourceActor = nullptr;
	ImpactContext.InstigatorActor = nullptr;

	ReceiveElementImpact_Implementation(ImpactContext);
}

void ASIPElementReactiveZoneBase::ReceiveElementImpact_Implementation(const FSIPElementImpactContext& ImpactContext)
{
	if (bHasTriggeredReaction && !bAllowRepeatedReactions)
	{
		UE_LOG(LogSIP, Verbose, TEXT("%s already triggered reaction [%s], ignoring elemental impact."),
			*GetName(),
			*TriggeredReactionTag.ToString());
		return;
	}

	if (!ZoneElementTag.IsValid() || !ImpactContext.IncomingElement.IsValid())
	{
		UE_LOG(LogSIP, Warning, TEXT("%s has invalid element configuration. Zone=[%s], Incoming=[%s]."),
			*GetName(),
			*ZoneElementTag.ToString(),
			*ImpactContext.IncomingElement.ToString());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	USIPElementReactionSubsystem* ReactionSubsystem = World->GetSubsystem<USIPElementReactionSubsystem>();
	if (!ReactionSubsystem)
	{
		return;
	}

	const FGameplayTag ReactionTag = ReactionSubsystem->QueryReaction(ZoneElementTag, ImpactContext.IncomingElement);
	if (!ReactionTag.IsValid())
	{
		return;
	}

	const float AppliedSurfaceDamage = FMath::Max(ImpactContext.SurfaceDamage, 0.0f);
	if (AppliedSurfaceDamage <= 0.0f)
	{
		return;
	}

	ReactionHealth = FMath::Max(ReactionHealth - AppliedSurfaceDamage, 0.0f);
	OnSurfaceDamageAccepted(ReactionTag, ImpactContext);

	UE_LOG(LogSIP, Log, TEXT("%s took surface damage %.2f from [%s]. Reaction health: %.2f / %.2f."),
		*GetName(),
		AppliedSurfaceDamage,
		*ImpactContext.IncomingElement.ToString(),
		ReactionHealth,
		MaxReactionHealth);

	if (ReactionHealth > 0.0f)
	{
		return;
	}

	bHasTriggeredReaction = true;
	TriggeredReactionTag = ReactionTag;

	const FVector ReactionLocation = ResolveReactionLocation(ImpactContext);
	ReactionSubsystem->ProcessElementHit(ZoneElementTag, ImpactContext.IncomingElement, ReactionLocation, this);
	OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);

	if (bAllowRepeatedReactions)
	{
		ReactionHealth = MaxReactionHealth;
	}
}

float ASIPElementReactiveZoneBase::GetReactionHealthPercent() const
{
	return MaxReactionHealth > 0.0f ? ReactionHealth / MaxReactionHealth : 0.0f;
}

AActor* ASIPElementReactiveZoneBase::GetVisualActor() const
{
	return VisualActorComponent ? VisualActorComponent->GetChildActor() : nullptr;
}

void ASIPElementReactiveZoneBase::OnSurfaceDamageAccepted(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext)
{
	K2_OnSurfaceDamageAccepted(ReactionTag, ImpactContext);
}

void ASIPElementReactiveZoneBase::OnReactionTriggered(const FGameplayTag& ReactionTag, const FSIPElementImpactContext& ImpactContext, const FVector& ReactionLocation)
{
	K2_OnReactionTriggered(ReactionTag, ImpactContext, ReactionLocation);

	UE_LOG(LogSIP, Log, TEXT("%s triggered reaction [%s] at %s."),
		*GetName(),
		*ReactionTag.ToString(),
		*ReactionLocation.ToString());
}

FVector ASIPElementReactiveZoneBase::ResolveReactionLocation(const FSIPElementImpactContext& ImpactContext) const
{
	return ImpactContext.ImpactLocation.IsNearlyZero()
		? GetActorLocation()
		: ImpactContext.ImpactLocation;
}

int32 ASIPElementReactiveZoneBase::ApplyDamageToOverlappingCharacters(float DamageAmount, AActor* DamageInstigator)
{
	if (DamageAmount <= 0.0f)
	{
		return 0;
	}

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ASIPCharacter::StaticClass());

	int32 DamagedCharacterCount = 0;
	for (AActor* OverlappingActor : OverlappingActors)
	{
		ASIPCharacter* Character = Cast<ASIPCharacter>(OverlappingActor);
		if (!Character || Character->IsDeadOrDying())
		{
			continue;
		}

		if (Character->ApplyCombatDamage(DamageAmount, DamageInstigator))
		{
			++DamagedCharacterCount;
		}
	}

	return DamagedCharacterCount;
}

int32 ASIPElementReactiveZoneBase::ApplyMaterialToVisualActor(UMaterialInterface* Material)
{
	if (!Material)
	{
		return 0;
	}

	AActor* VisualActor = GetVisualActor();
	return ApplyMaterialToActorMeshes(VisualActor ? VisualActor : this, Material);
}

void ASIPElementReactiveZoneBase::SetVisualActorHidden(bool bHide, bool bDisableCollision)
{
	AActor* VisualActor = GetVisualActor();
	if (!VisualActor)
	{
		return;
	}

	VisualActor->SetActorHiddenInGame(bHide);

	if (bDisableCollision)
	{
		VisualActor->SetActorEnableCollision(!bHide && !bDisableVisualActorCollision);
	}
}

void ASIPElementReactiveZoneBase::NotifyVisualReactionStarted(const FGameplayTag& ReactionTag, const FVector& ReactionLocation) const
{
	AActor* VisualActor = GetVisualActor();
	if (!VisualActor || !VisualActor->GetClass()->ImplementsInterface(USIPElementReactionVisual::StaticClass()))
	{
		return;
	}

	ISIPElementReactionVisual::Execute_OnElementReactionStarted(VisualActor, ReactionTag, ReactionLocation);
}

void ASIPElementReactiveZoneBase::NotifyVisualReactionFinished(const FGameplayTag& ReactionTag) const
{
	AActor* VisualActor = GetVisualActor();
	if (!VisualActor || !VisualActor->GetClass()->ImplementsInterface(USIPElementReactionVisual::StaticClass()))
	{
		return;
	}

	ISIPElementReactionVisual::Execute_OnElementReactionFinished(VisualActor, ReactionTag);
}

FTransform ASIPElementReactiveZoneBase::ResolveVisualEffectTransform(const FVector& LocalOffset) const
{
	const AActor* VisualActor = GetVisualActor();
	const FTransform AnchorTransform = VisualActor ? VisualActor->GetActorTransform() : GetActorTransform();

	return FTransform(
		AnchorTransform.GetRotation(),
		AnchorTransform.TransformPosition(LocalOffset),
		FVector::OneVector);
}

int32 ASIPElementReactiveZoneBase::ApplyMaterialToActorMeshes(AActor* TargetActor, UMaterialInterface* Material)
{
	if (!TargetActor || !Material)
	{
		return 0;
	}

	TArray<UMeshComponent*> MeshComponents;
	TargetActor->GetComponents<UMeshComponent>(MeshComponents);

	int32 ChangedMaterialSlots = 0;
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			MeshComponent->SetMaterial(MaterialIndex, Material);
			++ChangedMaterialSlots;
		}
	}

	return ChangedMaterialSlots;
}

void ASIPElementReactiveZoneBase::PlayReactionVFX(UNiagaraSystem* VFX, const FVector& Location) const
{
	if (!VFX)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		VFX,
		Location,
		FRotator::ZeroRotator,
		FVector::OneVector,
		true,
		true);
}
