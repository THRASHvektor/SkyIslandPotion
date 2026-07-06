// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Demo/SIPPetEcoNode.h"
#include "GameFramework/Actor.h"
#include "SIPPetEcoDemoManager.generated.h"

class ASIPDemoEnemy;
class ASIPDemoPet;
class ASIPPetEcoNode;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class SIP_API ASIPPetEcoDemoManager : public AActor
{
	GENERATED_BODY()

public:
	ASIPPetEcoDemoManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Pet Eco Demo")
	TObjectPtr<UTextRenderComponent> DemoTitleText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	bool bShowDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	int32 WorldSeed = 2026;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo", meta = (ClampMin = "0.5", ClampMax = "4.0"))
	float DemoScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo", meta = (ClampMin = "4", ClampMax = "32"))
	int32 BridgeStepCount = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	TSubclassOf<ASIPDemoPet> PetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	TSubclassOf<ASIPPetEcoNode> EcoNodeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Eco Demo")
	TSubclassOf<ASIPDemoEnemy> EnemyClass;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Eco Demo")
	void BuildDemoScene();

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Eco Demo")
	void ClearDemoScene();

	void NotifyPetReachedNode(ASIPDemoPet* Pet, ASIPPetEcoNode* Node);
	void NotifyPetReachedEnemy(ASIPDemoPet* Pet, ASIPDemoEnemy* Enemy);

	FVector GetPetRestLocation() const;
	bool ShouldShowDebug() const { return bShowDebug; }
	bool IsBridgeBuilt() const { return bBridgeBuilt; }

	const TArray<TObjectPtr<ASIPPetEcoNode>>& GetEcoNodes() const { return EcoNodes; }
	const TArray<TObjectPtr<ASIPDemoEnemy>>& GetDemoEnemies() const { return DemoEnemies; }

private:
	UStaticMeshComponent* CreateDemoMeshComponent(
		const FName ComponentName,
		UStaticMesh* Mesh,
		const FVector& RelativeLocation,
		const FVector& RelativeScale,
		const FLinearColor& Color,
		bool bEnableCollision);

	ASIPPetEcoNode* SpawnEcoNode(const FVector& Location, ESIPPetEcoNodeType NodeType);
	ASIPDemoEnemy* SpawnDemoEnemy(const FVector& Location);

	void GenerateBaseIslands();
	void GenerateBridge();
	void GenerateResourceBloom(const FVector& CenterLocation);
	void GenerateHealPulse(const FVector& CenterLocation);
	void GenerateWindPulse(const FVector& CenterLocation);
	void SetTitle(const FString& Message, const FColor& Color);

	UMaterialInterface* GetDemoMaterial() const;
	UStaticMesh* GetCubeMesh() const;
	UStaticMesh* GetSphereMesh() const;
	UStaticMesh* GetCylinderMesh() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> GeneratedComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASIPPetEcoNode>> EcoNodes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASIPDemoEnemy>> DemoEnemies;

	UPROPERTY(Transient)
	TObjectPtr<ASIPDemoPet> DemoPet;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DemoMaterial;

	FVector MainIslandCenter = FVector::ZeroVector;
	FVector FarIslandCenter = FVector(2300.0f, 0.0f, 220.0f);
	FVector BridgeStart = FVector(750.0f, 0.0f, 110.0f);
	FVector BridgeEnd = FVector(1700.0f, 0.0f, 245.0f);

	bool bBridgeBuilt = false;
	float DebugTime = 0.0f;
};
