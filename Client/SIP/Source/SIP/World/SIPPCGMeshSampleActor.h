// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPPCGMeshSampleActor.generated.h"

class UPCGComponent;
class UStaticMeshComponent;

/**
 * Test actor for proving PCG can sample an actual mesh primitive, then spawn resource plants.
 */
UCLASS(Blueprintable)
class SIP_API ASIPPCGMeshSampleActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPPCGMeshSampleActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|PCG Mesh Sample")
	TObjectPtr<UStaticMeshComponent> SampleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|PCG Mesh Sample")
	TObjectPtr<UPCGComponent> PCGComponent;
};
