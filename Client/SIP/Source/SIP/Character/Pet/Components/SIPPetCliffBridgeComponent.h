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

	/** Skill entry point: keeps landing validation, but ignores utility, cooldown and energy so the manual rock skill remains reliable. */
	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	bool ForceBuildBridgeInDirection(FVector WorldDirection);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	void ClearGeneratedBridge();

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	void SuppressBridgeScanForSeconds(float Duration);

	UFUNCTION(BlueprintPure, Category = "SIP|Pet Bridge")
	bool IsBridgeActive() const;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	bool FindBridgeCandidate(FSIPPetBridgeCandidate& OutCandidate);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Bridge")
	bool FindBridgeCandidateInDirection(FVector WorldDirection, FSIPPetBridgeCandidate& OutCandidate);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	bool bAutoScan = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	ESIPPetBridgeScanMode ScanMode = ESIPPetBridgeScanMode::RadialSearch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "0.1"))
	float AutoScanInterval = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "1"))
	int32 RadialScanDirections = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "100.0"))
	float EdgeProbeDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Slope")
	bool bTreatSteepSlopeAsGap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Slope", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float MaxWalkableSlopeAngle = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Height")
	bool bLimitLandingHeightDifference = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Height", meta = (ClampMin = "0.0"))
	float MaxLandingDrop = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Height", meta = (ClampMin = "0.0"))
	float MaxLandingRise = 3500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Navigation")
	bool bRequireNavMeshLanding = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Navigation")
	FVector NavMeshProjectionExtent = FVector(700.0f, 700.0f, 3000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Navigation", meta = (ClampMin = "0.0"))
	float MaxNavMeshProjection2DDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge|Navigation", meta = (ClampMin = "0.0"))
	float MaxNavMeshProjectionZDistance = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "300.0"))
	float MinLandingDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "500.0"))
	float MaxLandingDistance = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "50.0"))
	float LandingSearchStep = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "100.0"))
	float TraceUpHeight = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "100.0"))
	float TraceDownDepth = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "80.0"))
	float StepSpacing = 210.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge")
	bool bAutoFitStepSpacing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float StepOverlapRatio = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Bridge", meta = (ClampMin = "2", ClampMax = "128"))
	int32 MaxStepCount = 128;

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
	bool bDrawDebug = false;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Bridge|Debug")
	FString LastScanDebug;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Bridge")
	FSIPPetBridgeBuiltSignature OnBridgeBuilt;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Bridge")
	FSIPPetBridgeScanSignature OnScanFinished;

protected:
	bool BuildBridge(const FSIPPetBridgeCandidate& Candidate);
	float CalculateBridgeUtilityScore(const FSIPPetBridgeCandidate& Candidate) const;
	float CalculateStepSpacing() const;
	bool IsGroundHitWalkable(const FHitResult& GroundHit) const;
	bool IsLandingHeightAllowed(const FHitResult& StartGroundHit, const FHitResult& LandingHit) const;
	bool IsLandingOnNavMesh(const FHitResult& LandingHit, FVector& OutNavLocation) const;
	bool TraceGroundAt(const FVector& WorldLocation, FHitResult& OutHit) const;
	FVector GetOwnerFeetLocation() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AStaticMeshActor>> GeneratedStepActors;

	bool bHasBuiltBridge = false;
	float ScanTimer = 0.0f;
	float CurrentBridgeCooldown = 0.0f;
	float ScanSuppressionTimer = 0.0f;
};
