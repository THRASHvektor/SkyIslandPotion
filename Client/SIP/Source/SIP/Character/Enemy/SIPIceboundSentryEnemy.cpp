#include "Character/Enemy/SIPIceboundSentryEnemy.h"

#include "Character/SIPCharacter.h"
#include "Combat/SIPCombatStatics.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "SIPLogCategory.h"
#include "TimerManager.h"

ASIPIceboundSentryEnemy::ASIPIceboundSentryEnemy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	AttackRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttackRangeSphere"));
	AttackRangeSphere->SetupAttachment(GetRootComponent());
	AttackRangeSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AttackRangeSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AttackRangeSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AttackRangeSphere->SetGenerateOverlapEvents(true);
	UpdateAttackRangeSphere();
}

void ASIPIceboundSentryEnemy::BeginPlay()
{
	Super::BeginPlay();

	UpdateAttackRangeSphere();

	if (AttackInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			AttackTimerHandle,
			this,
			&ASIPIceboundSentryEnemy::TryFireAtPlayer,
			AttackInterval,
			true,
			AttackInterval);
	}
}

void ASIPIceboundSentryEnemy::OnDeath()
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	Super::OnDeath();
}

void ASIPIceboundSentryEnemy::TryFireAtPlayer()
{
	if (IsDeadOrDying())
	{
		return;
	}

	ASIPCharacter* TargetCharacter = FindBestPlayerTarget();
	if (!TargetCharacter)
	{
		return;
	}

	if (bRequireLineOfSight && !HasLineOfSightToTarget(TargetCharacter))
	{
		return;
	}

	const bool bGameplayEffectApplied = USIPCombatStatics::ApplyDamageToTarget(TargetCharacter, AttackDamage, this, this);
	K2_OnSentryFired(TargetCharacter, bGameplayEffectApplied);

	if (bDrawAttackDebug && GetWorld())
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), TargetCharacter->GetActorLocation(), FColor::Cyan, false, AttackInterval * 0.75f, 0, 3.0f);
	}

	UE_LOG(LogSIP, Log, TEXT("%s fired at %s. GameplayEffectApplied=%s."), *GetName(), *GetNameSafe(TargetCharacter), bGameplayEffectApplied ? TEXT("true") : TEXT("false"));
}

ASIPCharacter* ASIPIceboundSentryEnemy::FindBestPlayerTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ASIPCharacter* BestTarget = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const float AttackRangeSquared = FMath::Square(FMath::Max(0.0f, AttackRange));

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();
		ASIPCharacter* Candidate = PlayerController ? Cast<ASIPCharacter>(PlayerController->GetPawn()) : nullptr;
		if (!Candidate || Candidate->IsDeadOrDying())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared > AttackRangeSquared || DistanceSquared >= BestDistanceSquared)
		{
			continue;
		}

		BestTarget = Candidate;
		BestDistanceSquared = DistanceSquared;
	}

	return BestTarget;
}

bool ASIPIceboundSentryEnemy::HasLineOfSightToTarget(const ASIPCharacter* TargetCharacter) const
{
	if (!TargetCharacter || !GetWorld())
	{
		return false;
	}

	const FVector TraceStart = GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	const FVector TraceEnd = TargetCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SIPIceboundSentryLineOfSight), false, this);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(TargetCharacter);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	return !bHit;
}

void ASIPIceboundSentryEnemy::UpdateAttackRangeSphere()
{
	if (AttackRangeSphere)
	{
		AttackRangeSphere->SetSphereRadius(FMath::Max(0.0f, AttackRange));
	}
}
