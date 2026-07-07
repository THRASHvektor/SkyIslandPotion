// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Pet/Components/SIPPetPersonalityJsonComponent.h"
#include "Components/ActorComponent.h"
#include "HttpFwd.h"
#include "SIPPetMoodComponent.generated.h"

UENUM(BlueprintType)
enum class ESIPPetMoodType : uint8
{
	Calm UMETA(DisplayName = "Calm"),
	Excited UMETA(DisplayName = "Excited"),
	Nervous UMETA(DisplayName = "Nervous"),
	Lonely UMETA(DisplayName = "Lonely"),
	Playful UMETA(DisplayName = "Playful"),
	Alert UMETA(DisplayName = "Alert")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSIPPetMoodThoughtSignature, ESIPPetMoodType, Mood, const FString&, Thought);

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPPetMoodComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPPetMoodComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Mood")
	void ForceMood(ESIPPetMoodType NewMood, const FString& TriggerReason);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Mood")
	void GenerateThoughtForCurrentMood(const FString& TriggerReason);

	void BindToPersonalityComponent(USIPPetPersonalityJsonComponent* PersonalityComponent);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood")
	bool bUseQwenThoughts = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood")
	bool bShowThoughtOnScreen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood", meta = (ClampMin = "0.5"))
	float MoodCheckInterval = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood", meta = (ClampMin = "1.0"))
	float MinThoughtInterval = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood", meta = (ClampMin = "0.0"))
	float LonelyDistance = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood", meta = (ClampMin = "0.0"))
	float ExcitedPlayerSpeed = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood|Qwen")
	FString QwenModelName = TEXT("qwen-plus");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood|Qwen")
	FString DashScopeEndpoint = TEXT("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Mood|Qwen", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float QwenTimeoutSeconds = 20.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Mood")
	ESIPPetMoodType CurrentMood = ESIPPetMoodType::Calm;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Mood")
	FString LastMoodTrigger;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Mood")
	FString LastThought;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Mood")
	FString LastApiError;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Mood")
	FSIPPetMoodThoughtSignature OnMoodThoughtGenerated;

private:
	void EvaluateMood();
	void SetMoodIfChanged(ESIPPetMoodType NewMood, const FString& TriggerReason);
	void RequestQwenThought(const FString& TriggerReason);
	void HandleQwenThoughtResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString TriggerReason);
	void PublishThought(const FString& Thought);
	FString LimitThoughtLength(const FString& Thought, int32 MaxWords) const;
	FString BuildLocalThought(const FString& TriggerReason) const;
	FString BuildQwenRequestBody(const FString& TriggerReason) const;
	bool ExtractThoughtFromQwenResponse(const FString& ResponseString, FString& OutThought, FString& OutError) const;
	FString GetElementName() const;
	FString GetPersonalityName() const;
	FString GetMoodName() const;
	APawn* ResolvePlayerPawn() const;

	UFUNCTION()
	void HandlePersonalityApplied(const FSIPPetPersonalityConfig& Config);

	float MoodCheckTimer = 0.0f;
	float ThoughtCooldownTimer = 0.0f;
	bool bQwenRequestInFlight = false;
	bool bHasGeneratedOpeningThought = false;
};
