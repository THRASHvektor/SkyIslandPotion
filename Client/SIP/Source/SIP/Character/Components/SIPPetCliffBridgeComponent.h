// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SIPPetCliffBridgeComponent.generated.h"

class AStaticMeshActor;
class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ESIPPetBridgeScanMode : uint8
{
	OwnerForward UMETA(DisplayName = "Owner Forward"),
	RadialSearch UMETA(DisplayName = "Radial Search")
};

USTRUCT(BlueprintType)
struct FSIPPetBridgeCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Pet Bridge")
	bool bFound = false;

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Pet Bridge")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Pet Bridge")
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Pet Bridge")
	FVector LandingLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "SIP|Pet Bridge")
	float GapDistance = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIPPetBridgeBuiltSignature, const FSIPPetBridgeCandidate&, Candidate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIPPetBridgeScanSignature, bool, bFoundCandidate);

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPPetCliffBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPPetCliffBridgeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Manually scan using the configured ScanMode. Returns true when a bridge was built. */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	bool ScanAndBuildBridge();

	/** Manually scan in a specific world-space direction. Useful when your BP knows the next island direction. */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	bool ScanAndBuildBridgeInDirection(FVector WorldDirection);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	void ClearGeneratedBridge();

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	bool FindBridgeCandidate(FSIPPetBridgeCandidate& OutCandidate);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	bool FindBridgeCandidateInDirection(FVector WorldDirection, FSIPPetBridgeCandidate& OutCandidate);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	bool bAutoScan = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	ESIPPetBridgeScanMode ScanMode = ESIPPetBridgeScanMode::RadialSearch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "0.1"))
	float AutoScanInterval = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "1"))
	int32 RadialScanDirections = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "100.0"))
	float EdgeProbeDistance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "300.0"))
	float MinLandingDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "500.0"))
	float MaxLandingDistance = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "50.0"))
	float LandingSearchStep = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "100.0"))
	float TraceUpHeight = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "100.0"))
	float TraceDownDepth = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "80.0"))
	float StepSpacing = 210.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	bool bAutoFitStepSpacing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float StepOverlapRatio = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "2", ClampMax = "128"))
	int32 MaxStepCount = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "0.0"))
	float StepArcHeight = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	FVector StepScale = FVector(1.8f, 1.25f, 0.28f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	float StepZOffset = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_WorldStatic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	TObjectPtr<UStaticMesh> FloatingStepMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	TObjectPtr<UMaterialInterface> FloatingStepMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	bool bClearPreviousBridgeBeforeBuild = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	bool bBuildOnlyOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI")
	bool bUseUtilityDecision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinBridgeUtilityScore = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PersonalityBridgeBias = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PersonalityCuriosity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PersonalityProtectiveness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0"))
	float PetEnergy = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0"))
	float MaxPetEnergy = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0"))
	float BridgeEnergyCost = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0"))
	float EnergyRegenPerSecond = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Utility AI", meta = (ClampMin = "0.0"))
	float BridgeCooldown = 8.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Bridge|Utility AI")
	float CurrentBridgeUtilityScore = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Bridge|Utility AI")
	FString LastDecisionReason;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Bridge")
	FSIPPetBridgeBuiltSignature OnBridgeBuilt;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Bridge")
	FSIPPetBridgeScanSignature OnScanFinished;

protected:
	bool BuildBridge(const FSIPPetBridgeCandidate& Candidate);
	float CalculateBridgeUtilityScore(const FSIPPetBridgeCandidate& Candidate) const;
	float CalculateStepSpacing() const;
	bool TraceGroundAt(const FVector& WorldLocation, FHitResult& OutHit) const;
	FVector GetOwnerFeetLocation() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AStaticMeshActor>> GeneratedStepActors;

	bool bHasBuiltBridge = false;
	float ScanTimer = 0.0f;
	float CurrentBridgeCooldown = 0.0f;
};
