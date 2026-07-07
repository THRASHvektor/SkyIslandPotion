// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPPetElementalFieldActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USphereComponent;

UCLASS(Blueprintable)
class SIP_API ASIPPetElementalFieldActor : public AActor
{
	GENERATED_BODY()

public:
	ASIPPetElementalFieldActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Field")
	TObjectPtr<USphereComponent> FieldBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Field")
	TObjectPtr<UNiagaraComponent> FieldVFXComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Field")
	TObjectPtr<UNiagaraSystem> FieldVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Field", meta = (ClampMin = "50.0"))
	float FieldRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Field", meta = (ClampMin = "0.1"))
	float Duration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Field")
	FLinearColor FieldColor = FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
};
