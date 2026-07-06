// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPDemoEnemy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class SIP_API ASIPDemoEnemy : public AActor
{
	GENERATED_BODY()

public:
	ASIPDemoEnemy();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UStaticMeshComponent> EnemyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UTextRenderComponent> LabelText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	float MaxHealth = 60.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	float Health = 60.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	bool bDefeated = false;

	void ConfigureEnemy(UStaticMesh* Mesh, UMaterialInterface* Material);
	void TakeDemoDamage(float DamageAmount);

	bool IsAlive() const { return !bDefeated && Health > 0.0f; }

private:
	float BobTime = 0.0f;
	FVector StartLocation = FVector::ZeroVector;
};
