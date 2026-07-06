// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPDemoPet.generated.h"

class ASIPDemoEnemy;
class ASIPPetEcoDemoManager;
class ASIPPetEcoNode;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class ESIPDemoPetIntent : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	FollowPlayer UMETA(DisplayName = "Follow Player"),
	TriggerEcoNode UMETA(DisplayName = "Trigger Eco Node"),
	AttackEnemy UMETA(DisplayName = "Attack Enemy"),
	Celebrate UMETA(DisplayName = "Celebrate")
};

UCLASS(Blueprintable)
class SIP_API ASIPDemoPet : public AActor
{
	GENERATED_BODY()

public:
	ASIPDemoPet();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UTextRenderComponent> ThoughtText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	float MoveSpeed = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	float DecisionInterval = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	float InteractionDistance = 95.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	ESIPDemoPetIntent CurrentIntent = ESIPDemoPetIntent::Idle;

	void InitializePet(ASIPPetEcoDemoManager* InManager, UStaticMesh* BodyStaticMesh, UStaticMesh* CoreStaticMesh, UMaterialInterface* Material);

	FString GetIntentText() const;

private:
	void MakeDecision();
	void MoveToward(const FVector& TargetLocation, float DeltaSeconds);
	void UpdateThoughtText();
	float ScoreNode(const ASIPPetEcoNode* Node) const;
	float ScoreEnemy(const ASIPDemoEnemy* Enemy) const;

	UPROPERTY(Transient)
	TObjectPtr<ASIPPetEcoDemoManager> Manager;

	UPROPERTY(Transient)
	TObjectPtr<ASIPPetEcoNode> TargetNode;

	UPROPERTY(Transient)
	TObjectPtr<ASIPDemoEnemy> TargetEnemy;

	float DecisionTimer = 0.0f;
	float PulseTime = 0.0f;
};
