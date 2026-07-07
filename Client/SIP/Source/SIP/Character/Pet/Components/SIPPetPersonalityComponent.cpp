// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/Components/SIPPetPersonalityComponent.h"

#include "Character/Pet/AI/SIPPetAIController.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

USIPPetPersonalityComponent::USIPPetPersonalityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	ColorParameterNames = {
		TEXT("Param"),
		TEXT("BaseColor"),
		TEXT("Base Color"),
		TEXT("Color"),
		TEXT("Tint"),
		TEXT("PetColor"),
		TEXT("ElementColor")
	};
}

void USIPPetPersonalityComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bApplyDefaultPromptOnBeginPlay)
	{
		GeneratePersonalityFromPrompt(DefaultPrompt);
	}
	else
	{
		RebuildTraitsAndTuning();
		ApplyTuningToOwner();
	}
}

void USIPPetPersonalityComponent::GeneratePersonalityFromPrompt(const FString& Prompt)
{
	LastPrompt = Prompt;
	const FString LowerPrompt = Prompt.ToLower();

	if (PromptContainsAny(LowerPrompt, {TEXT("protect"), TEXT("guardian"), TEXT("loyal"), TEXT("safe")}))
	{
		PersonalityType = ESIPPetPersonalityType::Protective;
	}
	else if (PromptContainsAny(LowerPrompt, {TEXT("brave"), TEXT("bold"), TEXT("fighter"), TEXT("combat")}))
	{
		PersonalityType = ESIPPetPersonalityType::Brave;
	}
	else if (PromptContainsAny(LowerPrompt, {TEXT("timid"), TEXT("shy"), TEXT("careful"), TEXT("scared")}))
	{
		PersonalityType = ESIPPetPersonalityType::Timid;
	}
	else if (PromptContainsAny(LowerPrompt, {TEXT("independent"), TEXT("free"), TEXT("scout"), TEXT("solo")}))
	{
		PersonalityType = ESIPPetPersonalityType::Independent;
	}
	else if (PromptContainsAny(LowerPrompt, {TEXT("gentle"), TEXT("kind"), TEXT("heal"), TEXT("soft")}))
	{
		PersonalityType = ESIPPetPersonalityType::Gentle;
	}
	else
	{
		PersonalityType = ESIPPetPersonalityType::Curious;
	}

	ApplyPersonality(PersonalityType);
}

void USIPPetPersonalityComponent::ApplyPersonality(ESIPPetPersonalityType NewPersonalityType)
{
	PersonalityType = NewPersonalityType;
	RebuildTraitsAndTuning();
	ApplyTuningToOwner();
	OnPersonalityApplied.Broadcast(PersonalityType);
}

FString USIPPetPersonalityComponent::GetPersonalityDebugText() const
{
	return FString::Printf(
		TEXT("Personality=%s | C=%.2f P=%.2f B=%.2f I=%.2f | %s | Follow %.0f/%.0f | Recover %.1fs"),
		*StaticEnum<ESIPPetPersonalityType>()->GetNameStringByValue(static_cast<int64>(PersonalityType)),
		Traits.Curiosity,
		Traits.Protectiveness,
		Traits.Bravery,
		Traits.Independence,
		*BehaviourTuning.BehaviourLabel,
		BehaviourTuning.FollowStartDistance,
		BehaviourTuning.FollowStopDistance,
		BehaviourTuning.StrandedTeleportDelay);
}

void USIPPetPersonalityComponent::RebuildTraitsAndTuning()
{
	Traits = FSIPPetPersonalityRuntimeTraits();
	BehaviourTuning = FSIPPetBehaviourTuning();

	switch (PersonalityType)
	{
	case ESIPPetPersonalityType::Protective:
		Traits.Protectiveness = 0.95f;
		Traits.Curiosity = 0.35f;
		Traits.Bravery = 0.65f;
		Traits.Independence = 0.15f;
		BehaviourTuning.BehaviourLabel = TEXT("Protective: close guard, fast recovery");
		BehaviourTuning.FollowStartDistance = 240.0f;
		BehaviourTuning.FollowStopDistance = 110.0f;
		BehaviourTuning.StrandedTeleportDistance = 850.0f;
		BehaviourTuning.StrandedTeleportDelay = 0.55f;
		BehaviourTuning.BridgeUtilityBias = 0.9f;
		BehaviourTuning.WindLaunchVelocity = FVector(0.0f, 0.0f, 980.0f);
		BehaviourTuning.SkillLabel = TEXT("Protective Guard Field");
		BehaviourTuning.SkillColor = FLinearColor(0.10f, 0.58f, 1.0f, 1.0f);
		BehaviourTuning.SkillRadius = 520.0f;
		BehaviourTuning.SkillDuration = 10.0f;
		break;
	case ESIPPetPersonalityType::Brave:
		Traits.Bravery = 0.95f;
		Traits.Curiosity = 0.65f;
		Traits.Protectiveness = 0.45f;
		Traits.Independence = 0.55f;
		BehaviourTuning.BehaviourLabel = TEXT("Brave: forward support, assertive movement");
		BehaviourTuning.FollowStartDistance = 420.0f;
		BehaviourTuning.FollowStopDistance = 210.0f;
		BehaviourTuning.StrandedTeleportDistance = 1200.0f;
		BehaviourTuning.StrandedTeleportDelay = 1.1f;
		BehaviourTuning.BridgeUtilityBias = 0.72f;
		BehaviourTuning.WindLaunchVelocity = FVector(420.0f, 0.0f, 850.0f);
		BehaviourTuning.SkillLabel = TEXT("Brave Gust Rush");
		BehaviourTuning.SkillColor = FLinearColor(1.0f, 0.45f, 0.12f, 1.0f);
		BehaviourTuning.SkillRadius = 340.0f;
		BehaviourTuning.SkillDuration = 5.0f;
		break;
	case ESIPPetPersonalityType::Timid:
		Traits.Curiosity = 0.2f;
		Traits.Protectiveness = 0.72f;
		Traits.Bravery = 0.18f;
		Traits.Independence = 0.1f;
		BehaviourTuning.BehaviourLabel = TEXT("Timid: safe close follow, vertical escape");
		BehaviourTuning.FollowStartDistance = 220.0f;
		BehaviourTuning.FollowStopDistance = 95.0f;
		BehaviourTuning.StrandedTeleportDistance = 700.0f;
		BehaviourTuning.StrandedTeleportDelay = 0.35f;
		BehaviourTuning.BridgeUtilityBias = 0.58f;
		BehaviourTuning.WindLaunchVelocity = FVector(0.0f, 0.0f, 1250.0f);
		BehaviourTuning.SkillLabel = TEXT("Timid Escape Updraft");
		BehaviourTuning.SkillColor = FLinearColor(0.55f, 0.82f, 1.0f, 1.0f);
		BehaviourTuning.SkillRadius = 300.0f;
		BehaviourTuning.SkillDuration = 4.5f;
		break;
	case ESIPPetPersonalityType::Independent:
		Traits.Curiosity = 0.82f;
		Traits.Protectiveness = 0.25f;
		Traits.Bravery = 0.58f;
		Traits.Independence = 0.95f;
		BehaviourTuning.BehaviourLabel = TEXT("Independent: wider roaming distance");
		BehaviourTuning.FollowStartDistance = 620.0f;
		BehaviourTuning.FollowStopDistance = 340.0f;
		BehaviourTuning.StrandedTeleportDistance = 1750.0f;
		BehaviourTuning.StrandedTeleportDelay = 2.2f;
		BehaviourTuning.BridgeUtilityBias = 0.82f;
		BehaviourTuning.WindLaunchVelocity = FVector(250.0f, 0.0f, 980.0f);
		BehaviourTuning.SkillLabel = TEXT("Independent Scout Pulse");
		BehaviourTuning.SkillColor = FLinearColor(0.65f, 0.32f, 1.0f, 1.0f);
		BehaviourTuning.SkillRadius = 680.0f;
		BehaviourTuning.SkillDuration = 6.0f;
		break;
	case ESIPPetPersonalityType::Gentle:
		Traits.Curiosity = 0.45f;
		Traits.Protectiveness = 0.82f;
		Traits.Bravery = 0.35f;
		Traits.Independence = 0.2f;
		BehaviourTuning.BehaviourLabel = TEXT("Gentle: long support field, soft follow");
		BehaviourTuning.FollowStartDistance = 300.0f;
		BehaviourTuning.FollowStopDistance = 155.0f;
		BehaviourTuning.StrandedTeleportDistance = 1050.0f;
		BehaviourTuning.StrandedTeleportDelay = 0.85f;
		BehaviourTuning.BridgeUtilityBias = 0.78f;
		BehaviourTuning.WindLaunchVelocity = FVector(0.0f, 0.0f, 1050.0f);
		BehaviourTuning.SkillLabel = TEXT("Gentle Support Breeze");
		BehaviourTuning.SkillColor = FLinearColor(0.35f, 1.0f, 0.72f, 1.0f);
		BehaviourTuning.SkillRadius = 560.0f;
		BehaviourTuning.SkillDuration = 12.0f;
		break;
	case ESIPPetPersonalityType::Curious:
	default:
		Traits.Curiosity = 0.95f;
		Traits.Protectiveness = 0.45f;
		Traits.Bravery = 0.55f;
		Traits.Independence = 0.62f;
		BehaviourTuning.BehaviourLabel = TEXT("Curious: explores farther, eager bridge helper");
		BehaviourTuning.FollowStartDistance = 480.0f;
		BehaviourTuning.FollowStopDistance = 260.0f;
		BehaviourTuning.StrandedTeleportDistance = 1500.0f;
		BehaviourTuning.StrandedTeleportDelay = 1.4f;
		BehaviourTuning.BridgeUtilityBias = 0.96f;
		BehaviourTuning.WindLaunchVelocity = FVector(180.0f, 0.0f, 1080.0f);
		BehaviourTuning.SkillLabel = TEXT("Curious Exploration Lift");
		BehaviourTuning.SkillColor = FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
		BehaviourTuning.SkillRadius = 460.0f;
		BehaviourTuning.SkillDuration = 8.0f;
		break;
	}
}

void USIPPetPersonalityComponent::ApplyTuningToOwner()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (APawn* OwnerPawn = Cast<APawn>(Owner))
	{
		if (ASIPPetAIController* PetController = Cast<ASIPPetAIController>(OwnerPawn->GetController()))
		{
			PetController->ApplyPersonalityTuning(BehaviourTuning);
		}
	}

	if (bApplyPersonalityColor)
	{
		ApplyColorToOwner();
	}
}

void USIPPetPersonalityComponent::ApplyColorToOwner() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);

	const FLinearColor Color = GetPersonalityColor();
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

FLinearColor USIPPetPersonalityComponent::GetPersonalityColor() const
{
	switch (PersonalityType)
	{
	case ESIPPetPersonalityType::Protective:
		return FLinearColor(0.10f, 0.58f, 1.0f, 1.0f);
	case ESIPPetPersonalityType::Brave:
		return FLinearColor(1.0f, 0.45f, 0.12f, 1.0f);
	case ESIPPetPersonalityType::Timid:
		return FLinearColor(0.55f, 0.82f, 1.0f, 1.0f);
	case ESIPPetPersonalityType::Independent:
		return FLinearColor(0.65f, 0.32f, 1.0f, 1.0f);
	case ESIPPetPersonalityType::Gentle:
		return FLinearColor(0.35f, 1.0f, 0.72f, 1.0f);
	case ESIPPetPersonalityType::Curious:
	default:
		return FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
	}
}

bool USIPPetPersonalityComponent::PromptContainsAny(const FString& LowerPrompt, const TArray<FString>& Keywords)
{
	for (const FString& Keyword : Keywords)
	{
		if (LowerPrompt.Contains(Keyword))
		{
			return true;
		}
	}

	return false;
}
