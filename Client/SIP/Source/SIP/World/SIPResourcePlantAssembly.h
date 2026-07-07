// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SIPResourcePlantAssembly.generated.h"

UENUM(BlueprintType)
enum class ESIPResourcePlantPreviewFamily : uint8
{
	CrownLily UMETA(DisplayName = "Crown Lily"),
	AetherVine UMETA(DisplayName = "Aether Vine")
};

UENUM(BlueprintType)
enum class ESIPResourcePlantCollapseBand : uint8
{
	Stable_C0 UMETA(DisplayName = "Stable C0"),
	Weathered_C1 UMETA(DisplayName = "Weathered C1"),
	Fractured_C2 UMETA(DisplayName = "Fractured C2"),
	Unstable_C3 UMETA(DisplayName = "Unstable C3")
};

UENUM(BlueprintType)
enum class ESIPResourcePlantSlot : uint8
{
	BodyShell UMETA(DisplayName = "Body Shell"),
	PrimarySilhouette UMETA(DisplayName = "Primary Silhouette"),
	Core UMETA(DisplayName = "Core"),
	OrbitSet UMETA(DisplayName = "Orbit Set")
};

UENUM(BlueprintType)
enum class ESIPResourcePlantAssemblyAxisMapping : uint8
{
	Identity UMETA(DisplayName = "Identity"),
	SourceYUpToUnrealZUp UMETA(DisplayName = "Source Y-Up To Unreal Z-Up")
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantCollapseVector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant", meta = (ClampMin = "0", ClampMax = "3"))
	int32 BodyShell = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant", meta = (ClampMin = "0", ClampMax = "3"))
	int32 PrimarySilhouette = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant", meta = (ClampMin = "0", ClampMax = "3"))
	int32 Core = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant", meta = (ClampMin = "0", ClampMax = "3"))
	int32 OrbitSet = 0;
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantCollapseMetrics
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant")
	float Severity = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant")
	float Dispersion = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SIP|Resource Plant")
	bool bPassedHardConstraints = true;
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantSlotAssemblyHint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly")
	ESIPResourcePlantSlot Slot = ESIPResourcePlantSlot::BodyShell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly")
	FVector SourcePlacementCenter = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly")
	FRotator LocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly")
	FVector LocalScale = FVector::OneVector;
};

USTRUCT(BlueprintType)
struct FSIPResourcePlantAssemblyHint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly")
	ESIPResourcePlantAssemblyAxisMapping AxisMapping = ESIPResourcePlantAssemblyAxisMapping::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly", meta = (ClampMin = "0.001"))
	float SourceToUnrealScale = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly")
	FVector SourceAnchor = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Resource Plant|Assembly")
	TArray<FSIPResourcePlantSlotAssemblyHint> Slots;
};

namespace SIPResourcePlantAssembly
{
	SIP_API FSIPResourcePlantCollapseVector SanitizeCollapseVector(const FSIPResourcePlantCollapseVector& Vector);
	SIP_API FSIPResourcePlantCollapseMetrics CalculateCollapseMetrics(const FSIPResourcePlantCollapseVector& Vector);
	SIP_API bool PassesHardConstraints(const FSIPResourcePlantCollapseVector& Vector);
	SIP_API FSIPResourcePlantCollapseVector SampleCollapseVector(int32 Seed, ESIPResourcePlantCollapseBand Band, ESIPResourcePlantPreviewFamily Family, float TemperatureScale);
	SIP_API float LevelAlpha(int32 Level);
	SIP_API const FSIPResourcePlantSlotAssemblyHint* FindSlotHint(const FSIPResourcePlantAssemblyHint& Hint, ESIPResourcePlantSlot Slot);
	SIP_API FVector ConvertSourceOffsetToUnreal(const FVector& SourceOffset, ESIPResourcePlantAssemblyAxisMapping AxisMapping, float SourceToUnrealScale);
	SIP_API FTransform ResolveSlotTransform(const FSIPResourcePlantAssemblyHint& Hint, ESIPResourcePlantSlot Slot);
}
