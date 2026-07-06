// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SIPPetEcoNode.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class ESIPPetEcoNodeType : uint8
{
	BridgeSeed UMETA(DisplayName = "Bridge Seed"),
	ResourceBloom UMETA(DisplayName = "Resource Bloom"),
	EnemyNest UMETA(DisplayName = "Enemy Nest"),
	HealSpring UMETA(DisplayName = "Heal Spring"),
	WindPulse UMETA(DisplayName = "Wind Pulse")
};

UCLASS(Blueprintable)
class SIP_API ASIPPetEcoNode : public AActor
{
	GENERATED_BODY()

public:
	ASIPPetEcoNode();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UStaticMeshComponent> NodeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UTextRenderComponent> LabelText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	ESIPPetEcoNodeType NodeType = ESIPPetEcoNodeType::BridgeSeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	float ExploreValue = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	float DangerValue = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	bool bActivated = false;

	void ConfigureNode(ESIPPetEcoNodeType InType, UStaticMesh* Mesh, UMaterialInterface* Material, const FLinearColor& Color);
	void MarkActivated();

	FVector GetInteractionLocation() const;
	FString GetNodeName() const;
	FLinearColor GetNodeColor() const;
};
