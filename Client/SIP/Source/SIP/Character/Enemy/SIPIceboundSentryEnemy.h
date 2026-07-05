#pragma once

#include "CoreMinimal.h"
#include "Character/SIPEnemyCharacter.h"
#include "TimerManager.h"
#include "SIPIceboundSentryEnemy.generated.h"

class ASIPCharacter;
class USphereComponent;

UCLASS(Blueprintable)
class SIP_API ASIPIceboundSentryEnemy : public ASIPEnemyCharacter
{
	GENERATED_BODY()

public:
	ASIPIceboundSentryEnemy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void OnDeath() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Icebound Sentry")
	TObjectPtr<USphereComponent> AttackRangeSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Icebound Sentry|Attack", meta = (ClampMin = "0.1"))
	float AttackInterval = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Icebound Sentry|Attack", meta = (ClampMin = "0.0"))
	float AttackRange = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Icebound Sentry|Attack")
	bool bRequireLineOfSight = true;

	/** Damage dealt per shot. Routed through USIPCombatStatics -> project default damage GE via SetByCaller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Icebound Sentry|Attack", meta = (ClampMin = "0.0"))
	float AttackDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Icebound Sentry|Debug")
	bool bDrawAttackDebug = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "SIP|Icebound Sentry", DisplayName = "On Sentry Fired")
	void K2_OnSentryFired(ASIPCharacter* TargetCharacter, bool bGameplayEffectApplied);

private:
	void TryFireAtPlayer();
	ASIPCharacter* FindBestPlayerTarget() const;
	bool HasLineOfSightToTarget(const ASIPCharacter* TargetCharacter) const;
	void UpdateAttackRangeSphere();

	FTimerHandle AttackTimerHandle;
};
