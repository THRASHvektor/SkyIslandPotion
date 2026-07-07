// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SIPPetPersonalityComponent.generated.h"

UENUM(BlueprintType)
enum class ESIPPetPersonalityType : uint8
{
	Curious UMETA(DisplayName = "Curious"),
	Protective UMETA(DisplayName = "Protective"),
	Brave UMETA(DisplayName = "Brave"),
	Timid UMETA(DisplayName = "Timid"),
	Independent UMETA(DisplayName = "Independent"),
	Gentle UMETA(DisplayName = "Gentle")
};

USTRUCT(BlueprintType)
struct FSIPPetPersonalityRuntimeTraits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Curiosity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Protectiveness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Bravery = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Independence = 0.5f;
};

USTRUCT(BlueprintType)
struct FSIPPetBehaviourTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FString BehaviourLabel = TEXT("Balanced");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0"))
	float FollowStartDistance = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0"))
	float FollowStopDistance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0"))
	float StrandedTeleportDistance = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0"))
	float StrandedTeleportDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0"))
	float MoveRepathInterval = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FVector WindLaunchVelocity = FVector(0.0f, 0.0f, 950.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FString SkillLabel = TEXT("Wind Updraft");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FLinearColor SkillColor = FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "50.0"))
	float SkillRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.1"))
	float SkillDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	float BridgeUtilityBias = 0.75f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIPPetRuntimePersonalityAppliedSignature, ESIPPetPersonalityType, PersonalityType);

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPPetPersonalityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPPetPersonalityComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Personality")
	void GeneratePersonalityFromPrompt(const FString& Prompt);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Personality")
	void ApplyPersonality(ESIPPetPersonalityType NewPersonalityType);

	UFUNCTION(BlueprintPure, Category = "SIP|Pet Personality")
	FString GetPersonalityDebugText() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	bool bApplyDefaultPromptOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (EditCondition = "bApplyDefaultPromptOnBeginPlay"))
	FString DefaultPrompt = TEXT("a curious wind cat that helps me explore floating islands");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	ESIPPetPersonalityType PersonalityType = ESIPPetPersonalityType::Curious;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FSIPPetPersonalityRuntimeTraits Traits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FSIPPetBehaviourTuning BehaviourTuning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality|Visual")
	bool bApplyPersonalityColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality|Visual")
	TArray<FName> ColorParameterNames;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Personality")
	FString LastPrompt;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Personality")
	FSIPPetRuntimePersonalityAppliedSignature OnPersonalityApplied;

private:
	void RebuildTraitsAndTuning();
	void ApplyTuningToOwner();
	void ApplyColorToOwner() const;
	FLinearColor GetPersonalityColor() const;
	static bool PromptContainsAny(const FString& LowerPrompt, const TArray<FString>& Keywords);
};
