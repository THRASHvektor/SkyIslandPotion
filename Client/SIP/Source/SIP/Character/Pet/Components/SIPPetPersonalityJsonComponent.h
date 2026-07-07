// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HttpFwd.h"
#include "SIPPetPersonalityJsonComponent.generated.h"

class USIPPetCliffBridgeComponent;

UENUM(BlueprintType)
enum class ESIPPetElementType : uint8
{
	Wind UMETA(DisplayName = "Wind"),
	Rock UMETA(DisplayName = "Rock"),
	Water UMETA(DisplayName = "Water"),
	Thunder UMETA(DisplayName = "Thunder"),
	Plant UMETA(DisplayName = "Plant"),
	Shadow UMETA(DisplayName = "Shadow")
};

USTRUCT(BlueprintType)
struct FSIPPetPersonalityTraits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Curiosity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Bravery = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Protectiveness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Independence = 0.5f;
};

USTRUCT(BlueprintType)
struct FSIPPetBehaviourWeights
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FollowPlayer = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BuildBridge = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExploreUnknown = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AvoidCombat = 0.5f;
};

USTRUCT(BlueprintType)
struct FSIPPetBridgeStyleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FName MaterialTheme = TEXT("WindField");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0"))
	float ArcHeight = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float StepOverlap = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality", meta = (ClampMin = "1.0"))
	float BridgeLifetime = 18.0f;
};

USTRUCT(BlueprintType)
struct FSIPPetPersonalityConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FString PetName = TEXT("Sky Pet");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	ESIPPetElementType Element = ESIPPetElementType::Wind;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FName Archetype = TEXT("BalancedHelper");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FLinearColor PrimaryColor = FLinearColor(0.20f, 0.95f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FSIPPetPersonalityTraits Traits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FSIPPetBehaviourWeights BehaviourWeights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FSIPPetBridgeStyleConfig BridgeStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FName FieldEffect = TEXT("WindField");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	FString Summary = TEXT("A balanced companion pet.");
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSIPPetPersonalityAppliedSignature, const FSIPPetPersonalityConfig&, Config);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSIPPetPersonalityJsonGeneratedSignature, bool, bSuccess, const FString&, JsonString);

UCLASS(ClassGroup = (SIP), meta = (BlueprintSpawnableComponent))
class SIP_API USIPPetPersonalityJsonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USIPPetPersonalityJsonComponent();

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Personality")
	FString GeneratePersonalityFromText(const FString& Prompt);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Personality")
	void GeneratePersonalityFromTextWithQwen(const FString& Prompt);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Personality")
	bool ApplyPersonalityJson(const FString& JsonString);

	UFUNCTION(BlueprintCallable, Category = "SIP|Pet Personality")
	void ApplyCurrentConfigToOwner();

	UFUNCTION(BlueprintPure, Category = "SIP|Pet Personality")
	FString GetCurrentPersonalityJson() const;

	UFUNCTION(BlueprintPure, Category = "SIP|Pet Personality")
	FSIPPetPersonalityConfig GetCurrentConfig() const { return CurrentConfig; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	bool bAutoApplyGeneratedConfig = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	bool bApplyColorToOwnerMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	bool bUseElementColorPalette = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	bool bShowRuntimeDebugMessage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	TArray<FName> ColorParameterNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality")
	bool bApplyBridgeStyleToCliffBridge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality|Qwen")
	bool bFallbackToLocalParserOnApiFailure = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality|Qwen")
	FString QwenModelName = TEXT("qwen-plus");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality|Qwen")
	FString DashScopeEndpoint = TEXT("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SIP|Pet Personality|Qwen", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float QwenTimeoutSeconds = 30.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Personality")
	FString LastGeneratedJson;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Personality")
	FString LastApiError;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "SIP|Pet Personality")
	FSIPPetPersonalityConfig CurrentConfig;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Personality")
	FSIPPetPersonalityAppliedSignature OnPersonalityApplied;

	UPROPERTY(BlueprintAssignable, Category = "SIP|Pet Personality")
	FSIPPetPersonalityJsonGeneratedSignature OnPersonalityJsonGenerated;

private:
	FSIPPetPersonalityConfig BuildConfigFromPrompt(const FString& Prompt) const;
	FString ConfigToJson(const FSIPPetPersonalityConfig& Config) const;
	bool JsonToConfig(const FString& JsonString, FSIPPetPersonalityConfig& OutConfig) const;
	void ApplyColorToOwnerMeshes(const FLinearColor& Color) const;
	void ApplyBridgeStyleToCliffBridge(USIPPetCliffBridgeComponent* BridgeComponent) const;
	FLinearColor GetPaletteColorForElement(ESIPPetElementType Element) const;
	void HandleQwenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString OriginalPrompt);
	FString BuildQwenRequestBody(const FString& Prompt) const;
	bool ExtractJsonFromQwenResponse(const FString& ResponseString, FString& OutJsonString, FString& OutError) const;
	FString BuildSchemaPrompt(const FString& Prompt) const;

	static float Clamp01(float Value);
	static bool ContainsAny(const FString& Source, const TArray<FString>& Keywords);
	static FString ElementToString(ESIPPetElementType Element);
	static ESIPPetElementType ElementFromString(const FString& ElementString);
};
