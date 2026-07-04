#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "SIPElementReactionVisual.generated.h"

UINTERFACE(BlueprintType)
class SIP_API USIPElementReactionVisual : public UInterface
{
	GENERATED_BODY()
};

class SIP_API ISIPElementReactionVisual
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SIP|Elemental|Visual")
	void OnElementReactionStarted(FGameplayTag ReactionTag, FVector ReactionLocation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SIP|Elemental|Visual")
	void OnElementReactionFinished(FGameplayTag ReactionTag);
};
