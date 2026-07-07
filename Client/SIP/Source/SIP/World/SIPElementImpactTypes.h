#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SIPElementImpactTypes.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FSIPElementImpactContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element")
	FGameplayTag IncomingElement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element", meta = (ClampMin = "0.0"))
	float SurfaceDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Element")
	FVector ImpactLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Source")
	TObjectPtr<AActor> InstigatorActor = nullptr;
};
