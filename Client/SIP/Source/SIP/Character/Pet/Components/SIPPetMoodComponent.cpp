// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/Components/SIPPetMoodComponent.h"

#include "Character/Pet/Components/SIPPetPersonalityJsonComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

USIPPetMoodComponent::USIPPetMoodComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USIPPetMoodComponent::BeginPlay()
{
	Super::BeginPlay();
	MoodCheckTimer = 0.75f;
	ThoughtCooldownTimer = 0.0f;

	if (AActor* Owner = GetOwner())
	{
		if (USIPPetPersonalityJsonComponent* PersonalityJson = Owner->FindComponentByClass<USIPPetPersonalityJsonComponent>())
		{
			BindToPersonalityComponent(PersonalityJson);
		}
	}
}

void USIPPetMoodComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	MoodCheckTimer -= DeltaTime;
	ThoughtCooldownTimer = FMath::Max(0.0f, ThoughtCooldownTimer - DeltaTime);
	if (MoodCheckTimer <= 0.0f)
	{
		MoodCheckTimer = FMath::Max(0.5f, MoodCheckInterval);
		EvaluateMood();
	}
}

void USIPPetMoodComponent::ForceMood(ESIPPetMoodType NewMood, const FString& TriggerReason)
{
	SetMoodIfChanged(NewMood, TriggerReason);
}

void USIPPetMoodComponent::GenerateThoughtForCurrentMood(const FString& TriggerReason)
{
	if (ThoughtCooldownTimer > 0.0f || bQwenRequestInFlight)
	{
		return;
	}

	LastMoodTrigger = TriggerReason;
	ThoughtCooldownTimer = MinThoughtInterval;
	if (bUseQwenThoughts)
	{
		RequestQwenThought(TriggerReason);
	}
	else
	{
		PublishThought(BuildLocalThought(TriggerReason));
	}
}

void USIPPetMoodComponent::BindToPersonalityComponent(USIPPetPersonalityJsonComponent* PersonalityComponent)
{
	if (!PersonalityComponent)
	{
		return;
	}

	if (!PersonalityComponent->OnPersonalityApplied.IsAlreadyBound(this, &USIPPetMoodComponent::HandlePersonalityApplied))
	{
		PersonalityComponent->OnPersonalityApplied.AddDynamic(this, &USIPPetMoodComponent::HandlePersonalityApplied);
	}
}

void USIPPetMoodComponent::EvaluateMood()
{
	APawn* PlayerPawn = ResolvePlayerPawn();
	AActor* Owner = GetOwner();
	if (!PlayerPawn || !Owner)
	{
		return;
	}

	FSIPPetPersonalityConfig Config;
	if (const USIPPetPersonalityJsonComponent* PersonalityJson = Owner->FindComponentByClass<USIPPetPersonalityJsonComponent>())
	{
		Config = PersonalityJson->GetCurrentConfig();
	}

	const float LonelyThreshold = LonelyDistance * FMath::Lerp(1.25f, 0.85f, Config.BehaviourWeights.FollowPlayer);
	const float ExcitedThreshold = ExcitedPlayerSpeed * FMath::Lerp(1.15f, 0.75f, Config.Traits.Curiosity);
	const float Distance = FVector::Dist(PlayerPawn->GetActorLocation(), Owner->GetActorLocation());
	if (Distance >= LonelyThreshold)
	{
		SetMoodIfChanged(ESIPPetMoodType::Lonely, TEXT("the player moved far away"));
		return;
	}

	if (const ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn))
	{
		if (const UCharacterMovementComponent* MoveComp = PlayerCharacter->GetCharacterMovement())
		{
			if (MoveComp->IsFalling())
			{
				const bool bProtectiveOrTimid = Config.Traits.Protectiveness >= 0.65f || Config.BehaviourWeights.AvoidCombat >= 0.65f;
				SetMoodIfChanged(bProtectiveOrTimid ? ESIPPetMoodType::Nervous : ESIPPetMoodType::Alert, TEXT("the player is airborne near floating islands"));
				return;
			}
		}
	}

	const float PlayerSpeed2D = PlayerPawn->GetVelocity().Size2D();
	if (PlayerSpeed2D >= ExcitedThreshold)
	{
		SetMoodIfChanged(ESIPPetMoodType::Excited, TEXT("the player started moving quickly"));
		return;
	}

	if (Distance >= LonelyThreshold * 0.62f && Config.Traits.Protectiveness >= 0.7f)
	{
		SetMoodIfChanged(ESIPPetMoodType::Alert, TEXT("the protective pet is watching the player from a distance"));
		return;
	}

	if (Distance <= 450.0f && PlayerSpeed2D <= 120.0f && Config.Traits.Curiosity >= 0.72f)
	{
		SetMoodIfChanged(ESIPPetMoodType::Playful, TEXT("the curious pet noticed a quiet moment"));
		return;
	}

	SetMoodIfChanged(ESIPPetMoodType::Calm, TEXT("the exploration pace became steady"));
}

void USIPPetMoodComponent::SetMoodIfChanged(ESIPPetMoodType NewMood, const FString& TriggerReason)
{
	if (CurrentMood == NewMood)
	{
		return;
	}

	CurrentMood = NewMood;
	GenerateThoughtForCurrentMood(TriggerReason);
}

void USIPPetMoodComponent::RequestQwenThought(const FString& TriggerReason)
{
	LastApiError.Reset();

	const FString ApiKey = FPlatformMisc::GetEnvironmentVariable(TEXT("DASHSCOPE_API_KEY"));
	if (ApiKey.IsEmpty())
	{
		LastApiError = TEXT("DASHSCOPE_API_KEY is not set.");
		PublishThought(BuildLocalThought(TriggerReason));
		return;
	}

	bQwenRequestInFlight = true;
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(DashScopeEndpoint);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	Request->SetTimeout(QwenTimeoutSeconds);
	Request->SetContentAsString(BuildQwenRequestBody(TriggerReason));
	Request->OnProcessRequestComplete().BindUObject(this, &USIPPetMoodComponent::HandleQwenThoughtResponse, TriggerReason);
	Request->ProcessRequest();
}

void USIPPetMoodComponent::HandleQwenThoughtResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, FString TriggerReason)
{
	bQwenRequestInFlight = false;

	if (!bWasSuccessful || !Response.IsValid())
	{
		LastApiError = TEXT("Qwen mood thought request failed.");
		PublishThought(BuildLocalThought(TriggerReason));
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	const FString ResponseString = Response->GetContentAsString();
	if (ResponseCode < 200 || ResponseCode >= 300)
	{
		LastApiError = FString::Printf(TEXT("Qwen HTTP %d: %s"), ResponseCode, *ResponseString);
		PublishThought(BuildLocalThought(TriggerReason));
		return;
	}

	FString Thought;
	FString Error;
	if (!ExtractThoughtFromQwenResponse(ResponseString, Thought, Error))
	{
		LastApiError = Error;
		PublishThought(BuildLocalThought(TriggerReason));
		return;
	}

	PublishThought(Thought);
}

void USIPPetMoodComponent::PublishThought(const FString& Thought)
{
	LastThought = LimitThoughtLength(Thought, 18);
	if (LastThought.IsEmpty())
	{
		LastThought = LimitThoughtLength(BuildLocalThought(LastMoodTrigger), 18);
	}

	OnMoodThoughtGenerated.Broadcast(CurrentMood, LastThought);
}

FString USIPPetMoodComponent::LimitThoughtLength(const FString& Thought, int32 MaxWords) const
{
	FString CleanThought = Thought.TrimStartAndEnd();
	CleanThought.ReplaceInline(TEXT("\r"), TEXT(" "));
	CleanThought.ReplaceInline(TEXT("\n"), TEXT(" "));
	while (CleanThought.Contains(TEXT("  ")))
	{
		CleanThought.ReplaceInline(TEXT("  "), TEXT(" "));
	}

	TArray<FString> Words;
	CleanThought.ParseIntoArray(Words, TEXT(" "), true);
	if (Words.Num() <= MaxWords)
	{
		return CleanThought.Left(120);
	}

	TArray<FString> KeptWords;
	for (int32 Index = 0; Index < MaxWords && Index < Words.Num(); ++Index)
	{
		KeptWords.Add(Words[Index]);
	}

	FString Limited = FString::Join(KeptWords, TEXT(" "));
	Limited.RemoveFromEnd(TEXT("."));
	Limited.RemoveFromEnd(TEXT("!"));
	Limited.RemoveFromEnd(TEXT("?"));
	return Limited + TEXT("...");
}

FString USIPPetMoodComponent::BuildLocalThought(const FString& TriggerReason) const
{
	const FString Element = GetElementName();
	const FString Personality = GetPersonalityName();
	switch (CurrentMood)
	{
	case ESIPPetMoodType::Excited:
		return FString::Printf(TEXT("I feel %s energy buzzing. Let's keep moving!"), *Element.ToLower());
	case ESIPPetMoodType::Nervous:
		return TEXT("The air feels thin here... please stay careful.");
	case ESIPPetMoodType::Lonely:
		return TEXT("Hey, wait for me... the sky feels too wide alone.");
	case ESIPPetMoodType::Playful:
		return TEXT("This place makes my paws want to dance.");
	case ESIPPetMoodType::Alert:
		return TEXT("Something feels close. I should keep watch.");
	case ESIPPetMoodType::Calm:
	default:
		return FString::Printf(TEXT("My %s %s nature feels ready for this journey."), *Personality.ToLower(), *Element.ToLower());
	}
}

FString USIPPetMoodComponent::BuildQwenRequestBody(const FString& TriggerReason) const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("model"), QwenModelName);

	TArray<TSharedPtr<FJsonValue>> Messages;

	TSharedRef<FJsonObject> SystemMessage = MakeShared<FJsonObject>();
	SystemMessage->SetStringField(TEXT("role"), TEXT("system"));
	SystemMessage->SetStringField(TEXT("content"), TEXT("You are a small elemental companion pet in a fantasy sky island game. Write one short in-character thought. Return only one English sentence under 16 words. No markdown."));
	Messages.Add(MakeShared<FJsonValueObject>(SystemMessage));

	const FString UserPrompt = FString::Printf(
		TEXT("Element: %s\nPersonality: %s\nMood: %s\nTrigger: %s\nWrite the pet's current thought."),
		*GetElementName(),
		*GetPersonalityName(),
		*GetMoodName(),
		*TriggerReason);

	TSharedRef<FJsonObject> UserMessage = MakeShared<FJsonObject>();
	UserMessage->SetStringField(TEXT("role"), TEXT("user"));
	UserMessage->SetStringField(TEXT("content"), UserPrompt);
	Messages.Add(MakeShared<FJsonValueObject>(UserMessage));

	Root->SetArrayField(TEXT("messages"), Messages);
	Root->SetNumberField(TEXT("temperature"), 0.75);

	FString Body;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	FJsonSerializer::Serialize(Root, Writer);
	return Body;
}

bool USIPPetMoodComponent::ExtractThoughtFromQwenResponse(const FString& ResponseString, FString& OutThought, FString& OutError) const
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("Could not parse Qwen thought response as JSON.");
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
	if (!Root->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->Num() == 0)
	{
		OutError = TEXT("Qwen thought response has no choices.");
		return false;
	}

	const TSharedPtr<FJsonObject> ChoiceObject = (*Choices)[0]->AsObject();
	const TSharedPtr<FJsonObject>* MessageObject = nullptr;
	if (!ChoiceObject.IsValid() || !ChoiceObject->TryGetObjectField(TEXT("message"), MessageObject) || !MessageObject || !MessageObject->IsValid())
	{
		OutError = TEXT("Qwen thought response has no message.");
		return false;
	}

	FString Content;
	if (!(*MessageObject)->TryGetStringField(TEXT("content"), Content))
	{
		OutError = TEXT("Qwen thought message has no content.");
		return false;
	}

	Content.TrimStartAndEndInline();
	Content.RemoveFromStart(TEXT("\""));
	Content.RemoveFromEnd(TEXT("\""));
	OutThought = LimitThoughtLength(Content, 18);
	return !OutThought.IsEmpty();
}

FString USIPPetMoodComponent::GetElementName() const
{
	if (const USIPPetPersonalityJsonComponent* PersonalityJson = GetOwner() ? GetOwner()->FindComponentByClass<USIPPetPersonalityJsonComponent>() : nullptr)
	{
		switch (PersonalityJson->GetCurrentConfig().Element)
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

	return TEXT("Wind");
}

FString USIPPetMoodComponent::GetPersonalityName() const
{
	if (const USIPPetPersonalityJsonComponent* PersonalityJson = GetOwner() ? GetOwner()->FindComponentByClass<USIPPetPersonalityJsonComponent>() : nullptr)
	{
		const FSIPPetPersonalityConfig Config = PersonalityJson->GetCurrentConfig();
		if (Config.Traits.Bravery >= 0.75f)
		{
			return TEXT("Brave");
		}
		if (Config.Traits.Protectiveness >= 0.75f)
		{
			return TEXT("Protective");
		}
		if (Config.Traits.Independence >= 0.75f)
		{
			return TEXT("Independent");
		}
		if (Config.BehaviourWeights.AvoidCombat >= 0.75f)
		{
			return TEXT("Timid");
		}
		if (Config.Traits.Curiosity >= 0.75f)
		{
			return TEXT("Curious");
		}
	}

	return TEXT("Gentle");
}

FString USIPPetMoodComponent::GetMoodName() const
{
	switch (CurrentMood)
	{
	case ESIPPetMoodType::Excited:
		return TEXT("Excited");
	case ESIPPetMoodType::Nervous:
		return TEXT("Nervous");
	case ESIPPetMoodType::Lonely:
		return TEXT("Lonely");
	case ESIPPetMoodType::Playful:
		return TEXT("Playful");
	case ESIPPetMoodType::Alert:
		return TEXT("Alert");
	case ESIPPetMoodType::Calm:
	default:
		return TEXT("Calm");
	}
}

APawn* USIPPetMoodComponent::ResolvePlayerPawn() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}

void USIPPetMoodComponent::HandlePersonalityApplied(const FSIPPetPersonalityConfig& Config)
{
	if (bHasGeneratedOpeningThought)
	{
		return;
	}

	bHasGeneratedOpeningThought = true;
	ThoughtCooldownTimer = 0.0f;
	GenerateThoughtForCurrentMood(TEXT("the pet has just formed its elemental personality"));
}
