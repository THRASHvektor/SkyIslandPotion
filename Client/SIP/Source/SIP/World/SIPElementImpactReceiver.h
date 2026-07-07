#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "World/SIPElementImpactTypes.h"
#include "SIPElementImpactReceiver.generated.h"

UINTERFACE(BlueprintType)
class SIP_API USIPElementImpactReceiver : public UInterface
{
	GENERATED_BODY()
};

class SIP_API ISIPElementImpactReceiver
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SIP|Elemental")
	void ReceiveElementImpact(const FSIPElementImpactContext& ImpactContext);
};
