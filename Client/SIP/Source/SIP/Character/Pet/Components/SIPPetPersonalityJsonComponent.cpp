// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/Components/SIPPetPersonalityJsonComponent.h"

#include "Character/Pet/Components/SIPPetCliffBridgeComponent.h"
#include "Character/Pet/Components/SIPPetPersonalityComponent.h"
#include "Components/MeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const TCHAR* JsonFieldPetName = TEXT("petName");
	const TCHAR* JsonFieldElement = TEXT("element");
	const TCHAR* JsonFieldArchetype = TEXT("archetype");
	const TCHAR* JsonFieldPrimaryColor = TEXT("primaryColor");
	const TCHAR* JsonFieldPersonality = TEXT("personality");
	const TCHAR* JsonFieldBehaviourWeights = TEXT("behaviourWeights");
	const TCHAR* JsonFieldBridgeStyle = TEXT("bridgeStyle");
	const TCHAR* JsonFieldFieldEffect = TEXT("fieldEffect");
	const TCHAR* JsonFieldSummary = TEXT("summary");

	FLinearColor ColorFromHex(const FString& HexString, const FLinearColor& Fallback)
	{
		auto HexDigitToInt = [](TCHAR Char) -> int32
		{
			if (Char >= '0' && Char <= '9')
			{
				return Char - '0';
			}
			if (Char >= 'a' && Char <= 'f')
			{
				return 10 + Char - 'a';
			}
			if (Char >= 'A' && Char <= 'F')
			{
				return 10 + Char - 'A';
			}
			return 0;
		};

		FString Clean = HexString;
		Clean.RemoveFromStart(TEXT("#"));

		if (Clean.Len() != 6)
		{
			return Fallback;
		}

		const int32 R = HexDigitToInt(Clean[0]) * 16 + HexDigitToInt(Clean[1]);
		const int32 G = HexDigitToInt(Clean[2]) * 16 + HexDigitToInt(Clean[3]);
		const int32 B = HexDigitToInt(Clean[4]) * 16 + HexDigitToInt(Clean[5]);

		return FLinearColor(
			static_cast<float>(R) / 255.0f,
			static_cast<float>(G) / 255.0f,
			static_cast<float>(B) / 255.0f,
			1.0f);
	}

	FString ColorToHex(const FLinearColor& Color)
	{
		const FColor SRGB = Color.ToFColor(true);
		return FString::Printf(TEXT("#%02X%02X%02X"), SRGB.R, SRGB.G, SRGB.B);
	}

	FString UnicodeString(std::initializer_list<TCHAR> Chars)
	{
		FString Result;
		Result.Reserve(static_cast<int32>(Chars.size()));
		for (const TCHAR Char : Chars)
		{
			Result.AppendChar(Char);
		}
		return Result;
	}

	void TryReadFloatField(const TSharedPtr<FJsonObject>& Object, const FString& FieldName, float& OutValue)
	{
		if (!Object.IsValid())
		{
			return;
		}

		double NumberValue = 0.0;
		if (Object->TryGetNumberField(FieldName, NumberValue))
		{
			OutValue = static_cast<float>(NumberValue);
		}
	}
}

USIPPetPersonalityJsonComponent::USIPPetPersonalityJsonComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ColorParameterNames = {
		TEXT("BaseColor"),
		TEXT("Base Color"),
		TEXT("Color"),
		TEXT("Tint"),
		TEXT("Param"),
		TEXT("Albedo"),
		TEXT("BodyColor"),
		TEXT("PetColor"),
		TEXT("ElementColor"),
		TEXT("EmissiveColor")
	};
}

FString USIPPetPersonalityJsonComponent::GeneratePersonalityFromText(const FString& Prompt)
{
	CurrentConfig = BuildConfigFromPrompt(Prompt);
	LastGeneratedJson = ConfigToJson(CurrentConfig);

	if (bAutoApplyGeneratedConfig)
	{
		ApplyCurrentConfigToOwner();
	}

	return LastGeneratedJson;
}

void USIPPetPersonalityJsonComponent::GeneratePersonalityFromTextWithQwen(const FString& Prompt)
{
	LastApiError.Reset();

	const FString ApiKey = FPlatformMisc::GetEnvironmentVariable(TEXT("DASHSCOPE_API_KEY"));
	if (ApiKey.IsEmpty())
	{
		LastApiError = TEXT("DASHSCOPE_API_KEY is not set. Restart UE after running setx.");
		if (bFallbackToLocalParserOnApiFailure)
		{
			const FString LocalJson = GeneratePersonalityFromText(Prompt);
			OnPersonalityJsonGenerated.Broadcast(false, LocalJson);
			return;
		}

		OnPersonalityJsonGenerated.Broadcast(false, TEXT(""));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(DashScopeEndpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	Request->SetTimeout(QwenTimeoutSeconds);
	Request->SetContentAsString(BuildQwenRequestBody(Prompt));
	Request->OnProcessRequestComplete().BindUObject(this, &USIPPetPersonalityJsonComponent::HandleQwenResponse, Prompt);
	Request->ProcessRequest();
}

bool USIPPetPersonalityJsonComponent::ApplyPersonalityJson(const FString& JsonString)
{
	FSIPPetPersonalityConfig ParsedConfig;
	if (!JsonToConfig(JsonString, ParsedConfig))
	{
		return false;
	}

	CurrentConfig = ParsedConfig;
	LastGeneratedJson = ConfigToJson(CurrentConfig);
	ApplyCurrentConfigToOwner();
	return true;
}

void USIPPetPersonalityJsonComponent::ApplyCurrentConfigToOwner()
{
	if (bApplyColorToOwnerMeshes)
	{
		ApplyColorToOwnerMeshes(CurrentConfig.PrimaryColor);
	}

	if (bApplyBridgeStyleToCliffBridge)
	{
		if (USIPPetCliffBridgeComponent* BridgeComponent = GetOwner() ? GetOwner()->FindComponentByClass<USIPPetCliffBridgeComponent>() : nullptr)
		{
			ApplyBridgeStyleToCliffBridge(BridgeComponent);
		}
	}

	if (AActor* Owner = GetOwner())
	{
		USIPPetPersonalityComponent* RuntimePersonality = Owner->FindComponentByClass<USIPPetPersonalityComponent>();
		if (!RuntimePersonality)
		{
			RuntimePersonality = NewObject<USIPPetPersonalityComponent>(Owner, TEXT("RuntimePetPersonalityComponent"));
			if (RuntimePersonality)
			{
				RuntimePersonality->RegisterComponent();
				Owner->AddInstanceComponent(RuntimePersonality);
			}
		}

		if (RuntimePersonality)
		{
			RuntimePersonality->bApplyPersonalityColor = false;
			ESIPPetPersonalityType RuntimeType = ESIPPetPersonalityType::Curious;
			if (CurrentConfig.BehaviourWeights.AvoidCombat >= 0.8f || CurrentConfig.Traits.Bravery <= 0.3f)
			{
				RuntimeType = ESIPPetPersonalityType::Timid;
			}
			else if (CurrentConfig.Traits.Protectiveness >= 0.75f && CurrentConfig.BehaviourWeights.AvoidCombat >= 0.55f)
			{
				RuntimeType = ESIPPetPersonalityType::Gentle;
			}
			else if (CurrentConfig.Traits.Protectiveness >= 0.75f)
			{
				RuntimeType = ESIPPetPersonalityType::Protective;
			}
			else if (CurrentConfig.Traits.Bravery >= 0.75f)
			{
				RuntimeType = ESIPPetPersonalityType::Brave;
			}
			else if (CurrentConfig.Traits.Independence >= 0.75f)
			{
				RuntimeType = ESIPPetPersonalityType::Independent;
			}

			RuntimePersonality->LastPrompt = LastGeneratedJson;
			RuntimePersonality->ApplyPersonality(RuntimeType);
		}
	}

	if (bApplyColorToOwnerMeshes)
	{
		ApplyColorToOwnerMeshes(GetPaletteColorForElement(CurrentConfig.Element));
	}

	OnPersonalityApplied.Broadcast(CurrentConfig);
}

FString USIPPetPersonalityJsonComponent::GetCurrentPersonalityJson() const
{
	return LastGeneratedJson.IsEmpty() ? ConfigToJson(CurrentConfig) : LastGeneratedJson;
}

FSIPPetPersonalityConfig USIPPetPersonalityJsonComponent::BuildConfigFromPrompt(const FString& Prompt) const
{
	const FString LowerPrompt = Prompt.ToLower();
	FSIPPetPersonalityConfig Config;

	if (ContainsAny(LowerPrompt, {TEXT("rock"), TEXT("stone"), TEXT("earth"), TEXT("stable"), TEXT("defense"), TEXT("reliable"), TEXT("岩"), TEXT("石"), TEXT("土"), TEXT("稳定"), TEXT("防御"), TEXT("可靠")}))
	{
		Config.Element = ESIPPetElementType::Rock;
		Config.PetName = TEXT("Stone Warden");
		Config.Archetype = TEXT("StableProtector");
		Config.PrimaryColor = FLinearColor(0.55f, 0.42f, 0.25f);
		Config.BridgeStyle.MaterialTheme = TEXT("RockBridge");
		Config.FieldEffect = TEXT("StoneGuard");
		Config.BridgeStyle.ArcHeight = 90.0f;
		Config.BridgeStyle.StepOverlap = 0.08f;
		Config.Traits.Protectiveness = 0.8f;
		Config.BehaviourWeights.BuildBridge = 0.85f;
		Config.Summary = TEXT("A stable rock pet that creates safer, flatter bridges.");
	}
	else if (ContainsAny(LowerPrompt, {TEXT("water"), TEXT("heal"), TEXT("healing"), TEXT("flow"), TEXT("gentle"), TEXT("blue"), TEXT("水"), TEXT("治疗"), TEXT("治愈"), TEXT("流动"), TEXT("温和"), TEXT("温柔")}))
	{
		Config.Element = ESIPPetElementType::Water;
		Config.PetName = TEXT("Tide Gleam");
		Config.Archetype = TEXT("GentleSupport");
		Config.PrimaryColor = FLinearColor(0.18f, 0.55f, 1.0f);
		Config.BridgeStyle.MaterialTheme = TEXT("WaterPath");
		Config.FieldEffect = TEXT("HealingSpring");
		Config.BridgeStyle.ArcHeight = 130.0f;
		Config.BridgeStyle.StepOverlap = 0.22f;
		Config.Traits.Protectiveness = 0.85f;
		Config.BehaviourWeights.FollowPlayer = 0.9f;
		Config.BehaviourWeights.AvoidCombat = 0.75f;
		Config.Summary = TEXT("A gentle water pet that supports the player and favors safer traversal.");
	}
	else if (ContainsAny(LowerPrompt, {TEXT("thunder"), TEXT("lightning"), TEXT("electric"), TEXT("fire"), TEXT("flame"), TEXT("burst"), TEXT("agile"), TEXT("雷"), TEXT("电"), TEXT("火"), TEXT("爆发"), TEXT("敏捷"), TEXT("勇敢")}))
	{
		Config.Element = ESIPPetElementType::Thunder;
		Config.PetName = TEXT("Volt Sprite");
		Config.Archetype = TEXT("BraveStriker");
		Config.PrimaryColor = FLinearColor(0.90f, 0.75f, 0.12f);
		Config.BridgeStyle.MaterialTheme = TEXT("SparkSteps");
		Config.FieldEffect = TEXT("ElectricField");
		Config.BridgeStyle.ArcHeight = 210.0f;
		Config.BridgeStyle.StepOverlap = 0.16f;
		Config.Traits.Bravery = 0.9f;
		Config.BehaviourWeights.AvoidCombat = 0.15f;
		Config.Summary = TEXT("A brave thunder pet that reacts quickly and prefers aggressive support.");
	}
	else if (ContainsAny(LowerPrompt, {TEXT("plant"), TEXT("leaf"), TEXT("flower"), TEXT("vine"), TEXT("life"), TEXT("grow"), TEXT("resource"), TEXT("草"), TEXT("花"), TEXT("植物"), TEXT("生命"), TEXT("成长"), TEXT("藤"), TEXT("资源")}))
	{
		Config.Element = ESIPPetElementType::Plant;
		Config.PetName = TEXT("Bloom Bud");
		Config.Archetype = TEXT("ResourceSeeker");
		Config.PrimaryColor = FLinearColor(0.45f, 0.95f, 0.28f);
		Config.BridgeStyle.MaterialTheme = TEXT("VineBridge");
		Config.FieldEffect = TEXT("BloomField");
		Config.BridgeStyle.ArcHeight = 160.0f;
		Config.BridgeStyle.StepOverlap = 0.28f;
		Config.Traits.Curiosity = 0.8f;
		Config.BehaviourWeights.ExploreUnknown = 0.85f;
		Config.Summary = TEXT("A plant pet that reveals resources and grows organic traversal paths.");
	}
	else if (ContainsAny(LowerPrompt, {TEXT("shadow"), TEXT("dark"), TEXT("stealth"), TEXT("mist"), TEXT("hidden"), TEXT("careful"), TEXT("scout"), TEXT("影"), TEXT("暗"), TEXT("暗影"), TEXT("雾"), TEXT("隐秘"), TEXT("谨慎"), TEXT("侦察")}))
	{
		Config.Element = ESIPPetElementType::Shadow;
		Config.PetName = TEXT("Mist Shade");
		Config.Archetype = TEXT("CarefulScout");
		Config.PrimaryColor = FLinearColor(0.20f, 0.12f, 0.42f);
		Config.BridgeStyle.MaterialTheme = TEXT("ShadowSteps");
		Config.FieldEffect = TEXT("MistVeil");
		Config.BridgeStyle.ArcHeight = 120.0f;
		Config.BridgeStyle.StepOverlap = 0.35f;
		Config.Traits.Independence = 0.85f;
		Config.BehaviourWeights.AvoidCombat = 0.9f;
		Config.Summary = TEXT("A shadow pet that scouts carefully and avoids direct danger.");
	}
	else if (ContainsAny(LowerPrompt, {TEXT("wind"), TEXT("free"), TEXT("light"), TEXT("lightweight"), TEXT("speed"), TEXT("fast")}))
	{
		Config.Element = ESIPPetElementType::Wind;
		Config.PetName = TEXT("Windling");
		Config.Archetype = TEXT("CuriousBridgeHelper");
		Config.PrimaryColor = FLinearColor(0.15f, 0.92f, 0.48f);
		Config.BridgeStyle.MaterialTheme = TEXT("FloatingLeaves");
		Config.FieldEffect = TEXT("WindField");
		Config.BridgeStyle.ArcHeight = 240.0f;
		Config.BridgeStyle.StepOverlap = 0.26f;
		Config.Traits.Curiosity = 0.85f;
		Config.BehaviourWeights.BuildBridge = 0.9f;
		Config.Summary = TEXT("A curious wind pet that actively helps the player cross gaps.");
	}
	else
	{
		Config.Element = ESIPPetElementType::Wind;
		Config.PetName = TEXT("Windling");
		Config.Archetype = TEXT("CuriousBridgeHelper");
		Config.PrimaryColor = FLinearColor(0.15f, 0.92f, 0.48f);
		Config.BridgeStyle.MaterialTheme = TEXT("FloatingLeaves");
		Config.FieldEffect = TEXT("WindField");
		Config.BridgeStyle.ArcHeight = 240.0f;
		Config.BridgeStyle.StepOverlap = 0.26f;
		Config.Traits.Curiosity = 0.85f;
		Config.BehaviourWeights.BuildBridge = 0.9f;
		Config.Summary = TEXT("A curious wind pet that actively helps the player cross gaps.");
	}

	if (ContainsAny(LowerPrompt, {TEXT("timid"), TEXT("shy"), TEXT("careful"), TEXT("胆小"), TEXT("害羞"), TEXT("谨慎")}))
	{
		Config.Traits.Bravery = 0.22f;
		Config.BehaviourWeights.AvoidCombat = 0.85f;
		Config.Archetype = TEXT("CautiousHelper");
	}

	if (ContainsAny(LowerPrompt, {TEXT("protect"), TEXT("guardian"), TEXT("主人"), TEXT("保护"), TEXT("守护")}))
	{
		Config.Traits.Protectiveness = 0.92f;
		Config.BehaviourWeights.FollowPlayer = 0.88f;
		Config.BehaviourWeights.BuildBridge = FMath::Max(Config.BehaviourWeights.BuildBridge, 0.9f);
	}

	if (ContainsAny(LowerPrompt, {TEXT("explore"), TEXT("curious"), TEXT("adventure"), TEXT("探索"), TEXT("好奇"), TEXT("冒险")}))
	{
		Config.Traits.Curiosity = 0.9f;
		Config.BehaviourWeights.ExploreUnknown = 0.9f;
	}

	if (ContainsAny(LowerPrompt, {TEXT("bridge"), TEXT("gap"), TEXT("cliff"), TEXT("cross"), TEXT("桥"), TEXT("悬崖"), TEXT("断崖"), TEXT("过")}))
	{
		Config.BehaviourWeights.BuildBridge = 0.95f;
	}

	if (bUseElementColorPalette)
	{
		Config.PrimaryColor = GetPaletteColorForElement(Config.Element);
	}

	Config.Traits.Curiosity = Clamp01(Config.Traits.Curiosity);
	Config.Traits.Bravery = Clamp01(Config.Traits.Bravery);
	Config.Traits.Protectiveness = Clamp01(Config.Traits.Protectiveness);
	Config.Traits.Independence = Clamp01(Config.Traits.Independence);
	Config.BehaviourWeights.FollowPlayer = Clamp01(Config.BehaviourWeights.FollowPlayer);
	Config.BehaviourWeights.BuildBridge = Clamp01(Config.BehaviourWeights.BuildBridge);
	Config.BehaviourWeights.ExploreUnknown = Clamp01(Config.BehaviourWeights.ExploreUnknown);
	Config.BehaviourWeights.AvoidCombat = Clamp01(Config.BehaviourWeights.AvoidCombat);
	return Config;
}

FString USIPPetPersonalityJsonComponent::ConfigToJson(const FSIPPetPersonalityConfig& Config) const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(JsonFieldPetName, Config.PetName);
	Root->SetStringField(JsonFieldElement, ElementToString(Config.Element));
	Root->SetStringField(JsonFieldArchetype, Config.Archetype.ToString());
	Root->SetStringField(JsonFieldPrimaryColor, ColorToHex(Config.PrimaryColor));
	Root->SetStringField(JsonFieldFieldEffect, Config.FieldEffect.ToString());
	Root->SetStringField(JsonFieldSummary, Config.Summary);

	TSharedRef<FJsonObject> Traits = MakeShared<FJsonObject>();
	Traits->SetNumberField(TEXT("curiosity"), Config.Traits.Curiosity);
	Traits->SetNumberField(TEXT("bravery"), Config.Traits.Bravery);
	Traits->SetNumberField(TEXT("protectiveness"), Config.Traits.Protectiveness);
	Traits->SetNumberField(TEXT("independence"), Config.Traits.Independence);
	Root->SetObjectField(JsonFieldPersonality, Traits);

	TSharedRef<FJsonObject> Weights = MakeShared<FJsonObject>();
	Weights->SetNumberField(TEXT("followPlayer"), Config.BehaviourWeights.FollowPlayer);
	Weights->SetNumberField(TEXT("buildBridge"), Config.BehaviourWeights.BuildBridge);
	Weights->SetNumberField(TEXT("exploreUnknown"), Config.BehaviourWeights.ExploreUnknown);
	Weights->SetNumberField(TEXT("avoidCombat"), Config.BehaviourWeights.AvoidCombat);
	Root->SetObjectField(JsonFieldBehaviourWeights, Weights);

	TSharedRef<FJsonObject> BridgeStyle = MakeShared<FJsonObject>();
	BridgeStyle->SetStringField(TEXT("materialTheme"), Config.BridgeStyle.MaterialTheme.ToString());
	BridgeStyle->SetNumberField(TEXT("arcHeight"), Config.BridgeStyle.ArcHeight);
	BridgeStyle->SetNumberField(TEXT("stepOverlap"), Config.BridgeStyle.StepOverlap);
	BridgeStyle->SetNumberField(TEXT("bridgeLifetime"), Config.BridgeStyle.BridgeLifetime);
	Root->SetObjectField(JsonFieldBridgeStyle, BridgeStyle);

	FString JsonString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root, Writer);
	return JsonString;
}

bool USIPPetPersonalityJsonComponent::JsonToConfig(const FString& JsonString, FSIPPetPersonalityConfig& OutConfig) const
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	OutConfig = FSIPPetPersonalityConfig();
	FString StringValue;
	if (Root->TryGetStringField(JsonFieldPetName, StringValue))
	{
		OutConfig.PetName = StringValue;
	}
	if (Root->TryGetStringField(JsonFieldElement, StringValue))
	{
		OutConfig.Element = ElementFromString(StringValue);
	}
	if (Root->TryGetStringField(JsonFieldArchetype, StringValue))
	{
		OutConfig.Archetype = FName(*StringValue);
	}
	if (Root->TryGetStringField(JsonFieldPrimaryColor, StringValue))
	{
		OutConfig.PrimaryColor = ColorFromHex(StringValue, OutConfig.PrimaryColor);
	}
	if (bUseElementColorPalette)
	{
		OutConfig.PrimaryColor = GetPaletteColorForElement(OutConfig.Element);
	}
	if (Root->TryGetStringField(JsonFieldFieldEffect, StringValue))
	{
		OutConfig.FieldEffect = FName(*StringValue);
	}
	if (Root->TryGetStringField(JsonFieldSummary, StringValue))
	{
		OutConfig.Summary = StringValue;
	}

	const TSharedPtr<FJsonObject>* Traits = nullptr;
	if (Root->TryGetObjectField(JsonFieldPersonality, Traits) && Traits && Traits->IsValid())
	{
		TryReadFloatField(*Traits, TEXT("curiosity"), OutConfig.Traits.Curiosity);
		TryReadFloatField(*Traits, TEXT("bravery"), OutConfig.Traits.Bravery);
		TryReadFloatField(*Traits, TEXT("protectiveness"), OutConfig.Traits.Protectiveness);
		TryReadFloatField(*Traits, TEXT("independence"), OutConfig.Traits.Independence);
	}

	const TSharedPtr<FJsonObject>* Weights = nullptr;
	if (Root->TryGetObjectField(JsonFieldBehaviourWeights, Weights) && Weights && Weights->IsValid())
	{
		TryReadFloatField(*Weights, TEXT("followPlayer"), OutConfig.BehaviourWeights.FollowPlayer);
		TryReadFloatField(*Weights, TEXT("buildBridge"), OutConfig.BehaviourWeights.BuildBridge);
		TryReadFloatField(*Weights, TEXT("exploreUnknown"), OutConfig.BehaviourWeights.ExploreUnknown);
		TryReadFloatField(*Weights, TEXT("avoidCombat"), OutConfig.BehaviourWeights.AvoidCombat);
	}

	const TSharedPtr<FJsonObject>* BridgeStyle = nullptr;
	if (Root->TryGetObjectField(JsonFieldBridgeStyle, BridgeStyle) && BridgeStyle && BridgeStyle->IsValid())
	{
		if ((*BridgeStyle)->TryGetStringField(TEXT("materialTheme"), StringValue))
		{
			OutConfig.BridgeStyle.MaterialTheme = FName(*StringValue);
		}
		TryReadFloatField(*BridgeStyle, TEXT("arcHeight"), OutConfig.BridgeStyle.ArcHeight);
		TryReadFloatField(*BridgeStyle, TEXT("stepOverlap"), OutConfig.BridgeStyle.StepOverlap);
		TryReadFloatField(*BridgeStyle, TEXT("bridgeLifetime"), OutConfig.BridgeStyle.BridgeLifetime);
	}

	return true;
}

void USIPPetPersonalityJsonComponent::ApplyColorToOwnerMeshes(const FLinearColor& Color) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(MaterialIndex);
			if (!DynamicMaterial)
			{
				continue;
			}

			for (const FName& ParameterName : ColorParameterNames)
			{
				DynamicMaterial->SetVectorParameterValue(ParameterName, Color);
			}
		}
	}

}

void USIPPetPersonalityJsonComponent::ApplyBridgeStyleToCliffBridge(USIPPetCliffBridgeComponent* BridgeComponent) const
{
	if (!BridgeComponent)
	{
		return;
	}

	BridgeComponent->StepArcHeight = CurrentConfig.BridgeStyle.ArcHeight;
	BridgeComponent->StepOverlapRatio = CurrentConfig.BridgeStyle.StepOverlap;
	BridgeComponent->bAutoScan = false;
	BridgeComponent->PersonalityBridgeBias = CurrentConfig.BehaviourWeights.BuildBridge;
	BridgeComponent->PersonalityCuriosity = CurrentConfig.Traits.Curiosity;
	BridgeComponent->PersonalityProtectiveness = CurrentConfig.Traits.Protectiveness;
}

FLinearColor USIPPetPersonalityJsonComponent::GetPaletteColorForElement(ESIPPetElementType Element) const
{
	switch (Element)
	{
	case ESIPPetElementType::Rock:
		return FLinearColor(0.62f, 0.42f, 0.20f, 1.0f);
	case ESIPPetElementType::Water:
		return FLinearColor(0.02f, 0.32f, 1.0f, 1.0f);
	case ESIPPetElementType::Thunder:
		return FLinearColor(0.62f, 0.12f, 1.0f, 1.0f);
	case ESIPPetElementType::Plant:
		return FLinearColor(0.55f, 1.0f, 0.16f, 1.0f);
	case ESIPPetElementType::Shadow:
		return FLinearColor(0.16f, 0.04f, 0.42f, 1.0f);
	case ESIPPetElementType::Wind:
	default:
		return FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
	}
}

void USIPPetPersonalityJsonComponent::HandleQwenResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString OriginalPrompt)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		LastApiError = TEXT("Qwen request failed before receiving a valid HTTP response.");
		if (bFallbackToLocalParserOnApiFailure)
		{
			const FString LocalJson = GeneratePersonalityFromText(OriginalPrompt);
			OnPersonalityJsonGenerated.Broadcast(false, LocalJson);
		}
		else
		{
			OnPersonalityJsonGenerated.Broadcast(false, TEXT(""));
		}
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	const FString ResponseString = Response->GetContentAsString();
	if (ResponseCode < 200 || ResponseCode >= 300)
	{
		LastApiError = FString::Printf(TEXT("Qwen HTTP %d: %s"), ResponseCode, *ResponseString);
		if (bFallbackToLocalParserOnApiFailure)
		{
			const FString LocalJson = GeneratePersonalityFromText(OriginalPrompt);
			OnPersonalityJsonGenerated.Broadcast(false, LocalJson);
		}
		else
		{
			OnPersonalityJsonGenerated.Broadcast(false, TEXT(""));
		}
		return;
	}

	FString ExtractedJson;
	FString ExtractError;
	if (!ExtractJsonFromQwenResponse(ResponseString, ExtractedJson, ExtractError))
	{
		LastApiError = ExtractError;
		if (bFallbackToLocalParserOnApiFailure)
		{
			const FString LocalJson = GeneratePersonalityFromText(OriginalPrompt);
			OnPersonalityJsonGenerated.Broadcast(false, LocalJson);
		}
		else
		{
			OnPersonalityJsonGenerated.Broadcast(false, TEXT(""));
		}
		return;
	}

	if (!ApplyPersonalityJson(ExtractedJson))
	{
		LastApiError = TEXT("Qwen returned JSON, but it did not match the expected pet schema.");
		if (bFallbackToLocalParserOnApiFailure)
		{
			const FString LocalJson = GeneratePersonalityFromText(OriginalPrompt);
			OnPersonalityJsonGenerated.Broadcast(false, LocalJson);
		}
		else
		{
			OnPersonalityJsonGenerated.Broadcast(false, ExtractedJson);
		}
		return;
	}

	OnPersonalityJsonGenerated.Broadcast(true, LastGeneratedJson);
}

FString USIPPetPersonalityJsonComponent::BuildQwenRequestBody(const FString& Prompt) const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), QwenModelName);

	TArray<TSharedPtr<FJsonValue>> Messages;

	TSharedRef<FJsonObject> SystemMessage = MakeShared<FJsonObject>();
	SystemMessage->SetStringField(TEXT("role"), TEXT("system"));
	SystemMessage->SetStringField(TEXT("content"), TEXT("You convert pet descriptions into strict gameplay JSON. Return JSON only. No markdown."));
	Messages.Add(MakeShared<FJsonValueObject>(SystemMessage));

	TSharedRef<FJsonObject> UserMessage = MakeShared<FJsonObject>();
	UserMessage->SetStringField(TEXT("role"), TEXT("user"));
	UserMessage->SetStringField(TEXT("content"), BuildSchemaPrompt(Prompt));
	Messages.Add(MakeShared<FJsonValueObject>(UserMessage));

	Root->SetArrayField(TEXT("messages"), Messages);
	Root->SetNumberField(TEXT("temperature"), 0.2);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);
	return Body;
}

bool USIPPetPersonalityJsonComponent::ExtractJsonFromQwenResponse(const FString& ResponseString, FString& OutJsonString, FString& OutError) const
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("Could not parse Qwen response as JSON.");
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
	if (!Root->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->Num() == 0)
	{
		OutError = TEXT("Qwen response has no choices array.");
		return false;
	}

	const TSharedPtr<FJsonObject> ChoiceObject = (*Choices)[0]->AsObject();
	if (!ChoiceObject.IsValid())
	{
		OutError = TEXT("Qwen choice is not an object.");
		return false;
	}

	const TSharedPtr<FJsonObject>* MessageObject = nullptr;
	if (!ChoiceObject->TryGetObjectField(TEXT("message"), MessageObject) || !MessageObject || !MessageObject->IsValid())
	{
		OutError = TEXT("Qwen choice has no message object.");
		return false;
	}

	FString Content;
	if (!(*MessageObject)->TryGetStringField(TEXT("content"), Content))
	{
		OutError = TEXT("Qwen message has no content string.");
		return false;
	}

	Content.TrimStartAndEndInline();
	Content.RemoveFromStart(TEXT("```json"));
	Content.RemoveFromStart(TEXT("```"));
	Content.RemoveFromEnd(TEXT("```"));
	Content.TrimStartAndEndInline();

	const int32 FirstBrace = Content.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
	const int32 LastBrace = Content.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	if (FirstBrace == INDEX_NONE || LastBrace == INDEX_NONE || LastBrace <= FirstBrace)
	{
		OutError = FString::Printf(TEXT("Qwen content did not contain a JSON object: %s"), *Content);
		return false;
	}

	OutJsonString = Content.Mid(FirstBrace, LastBrace - FirstBrace + 1);
	return true;
}

FString USIPPetPersonalityJsonComponent::BuildSchemaPrompt(const FString& Prompt) const
{
	return FString::Printf(TEXT(
		"Convert this player description into one JSON object for an Unreal Engine pet companion.\n"
		"Player description: %s\n\n"
		"Return exactly this schema with no extra text:\n"
		"{\n"
		"  \"petName\": \"short English name\",\n"
		"  \"element\": \"Wind|Rock|Water|Thunder|Plant|Shadow\",\n"
		"  \"archetype\": \"short identifier\",\n"
		"  \"primaryColor\": \"#RRGGBB\",\n"
		"  \"personality\": {\n"
		"    \"curiosity\": 0.0,\n"
		"    \"bravery\": 0.0,\n"
		"    \"protectiveness\": 0.0,\n"
		"    \"independence\": 0.0\n"
		"  },\n"
		"  \"behaviourWeights\": {\n"
		"    \"followPlayer\": 0.0,\n"
		"    \"buildBridge\": 0.0,\n"
		"    \"exploreUnknown\": 0.0,\n"
		"    \"avoidCombat\": 0.0\n"
		"  },\n"
		"  \"bridgeStyle\": {\n"
		"    \"materialTheme\": \"FloatingLeaves|RockBridge|WaterPath|SparkSteps|VineBridge|ShadowSteps\",\n"
		"    \"arcHeight\": 180,\n"
		"    \"stepOverlap\": 0.2,\n"
		"    \"bridgeLifetime\": 18\n"
		"  },\n"
		"  \"fieldEffect\": \"WindField|StoneGuard|HealingSpring|ElectricField|BloomField|MistVeil\",\n"
		"  \"summary\": \"one sentence summary\"\n"
		"}\n"
		"Rules: numeric values must be between 0 and 1 except arcHeight and bridgeLifetime. "
		"If the description is Chinese, still output the JSON field values in English identifiers."),
		*Prompt);
}

float USIPPetPersonalityJsonComponent::Clamp01(float Value)
{
	return FMath::Clamp(Value, 0.0f, 1.0f);
}

bool USIPPetPersonalityJsonComponent::ContainsAny(const FString& Source, const TArray<FString>& Keywords)
{
	for (const FString& Keyword : Keywords)
	{
		if (Source.Contains(Keyword))
		{
			return true;
		}
	}

	return false;
}

FString USIPPetPersonalityJsonComponent::ElementToString(ESIPPetElementType Element)
{
	switch (Element)
	{
	case ESIPPetElementType::Rock:
		return TEXT("Rock");
	case ESIPPetElementType::Water:
		return TEXT("Water");
	case ESIPPetElementType::Thunder:
		return TEXT("Thunder");
	case ESIPPetElementType::Plant:
		return TEXT("Plant");
	case ESIPPetElementType::Shadow:
		return TEXT("Shadow");
	case ESIPPetElementType::Wind:
	default:
		return TEXT("Wind");
	}
}

ESIPPetElementType USIPPetPersonalityJsonComponent::ElementFromString(const FString& ElementString)
{
	const FString Lower = ElementString.ToLower();
	if (Lower == TEXT("rock") || Lower == TEXT("stone") || Lower.Contains(TEXT("岩")) || Lower.Contains(TEXT("石")))
	{
		return ESIPPetElementType::Rock;
	}
	if (Lower == TEXT("water") || Lower.Contains(TEXT("水")))
	{
		return ESIPPetElementType::Water;
	}
	if (Lower == TEXT("thunder") || Lower == TEXT("lightning") || Lower == TEXT("electric") || Lower == TEXT("fire") || Lower == TEXT("flame") || Lower.Contains(TEXT("雷")) || Lower.Contains(TEXT("电")) || Lower.Contains(TEXT("火")))
	{
		return ESIPPetElementType::Thunder;
	}
	if (Lower == TEXT("plant") || Lower == TEXT("leaf") || Lower == TEXT("vine") || Lower.Contains(TEXT("植物")) || Lower.Contains(TEXT("花")) || Lower.Contains(TEXT("藤")))
	{
		return ESIPPetElementType::Plant;
	}
	if (Lower == TEXT("shadow") || Lower == TEXT("dark") || Lower == TEXT("mist") || Lower.Contains(TEXT("影")) || Lower.Contains(TEXT("暗")) || Lower.Contains(TEXT("雾")))
	{
		return ESIPPetElementType::Shadow;
	}
	if (Lower == TEXT("wind") || Lower.Contains(TEXT("风")))
	{
		return ESIPPetElementType::Wind;
	}

	return ESIPPetElementType::Wind;
}
