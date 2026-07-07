// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/Components/SIPPetPromptSpawnComponent.h"

#include "Character/Pet/SIPPetCharacter.h"
#include "Character/Pet/Components/SIPPetCliffBridgeComponent.h"
#include "Character/Pet/Components/SIPPetFollowComponent.h"
#include "Character/Pet/Components/SIPPetMoodComponent.h"
#include "Character/Pet/Components/SIPPetPersonalityJsonComponent.h"
#include "Character/Pet/SIPPetPromptSettings.h"
#include "Character/Pet/UI/SIPPetPromptWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"

namespace
{
bool PromptContainsKeyword(const FString& LowerPrompt, const FString& Keyword)
{
	if (Keyword.IsEmpty())
	{
		return false;
	}

	return LowerPrompt.Contains(Keyword.ToLower());
}
}

USIPPetPromptSpawnComponent::USIPPetPromptSpawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PetClass = ASIPPetCharacter::StaticClass();
	PromptWidgetClass = USIPPetPromptWidget::StaticClass();
}

void USIPPetPromptSpawnComponent::BeginPlay()
{
	Super::BeginPlay();

	const USIPPetPromptSettings* PetPromptSettings = GetDefault<USIPPetPromptSettings>();
	const bool bProjectAllowsAutoShow = PetPromptSettings ? PetPromptSettings->bShowPromptWidgetOnBeginPlay : false;
	if (bAutoShowPromptWidget && bProjectAllowsAutoShow)
	{
		ShowPromptWidget();
	}
}

AActor* USIPPetPromptSpawnComponent::SpawnPetFromPrompt(const FString& Prompt)
{
	return SpawnPetFromPromptNearActor(Prompt, GetOwner());
}

AActor* USIPPetPromptSpawnComponent::SpawnPetFromPromptNearActor(const FString& Prompt, AActor* SpawnNearActor)
{
	UWorld* World = GetWorld();
	if (!World || !SpawnNearActor)
	{
		return nullptr;
	}

	if (!PetClass)
	{
		PetClass = ASIPPetCharacter::StaticClass();
	}

	FName ResolvedSpeciesName = NAME_None;
	TSubclassOf<AActor> ResolvedPetClass = ResolvePetClassFromPrompt(Prompt, ResolvedSpeciesName);
	LastResolvedSpeciesName = ResolvedSpeciesName;

	if (bUseQwenApi && bWaitForPersonalityBeforeSpawn)
	{
		PendingPrompt = Prompt;
		PendingSpawnNearActor = SpawnNearActor;
		PendingResolvedPetClass = ResolvedPetClass;
		PendingResolvedSpeciesName = ResolvedSpeciesName;

		if (PendingPersonalityComponent)
		{
			PendingPersonalityComponent->OnPersonalityJsonGenerated.RemoveAll(this);
			PendingPersonalityComponent->DestroyComponent();
			PendingPersonalityComponent = nullptr;
		}

		PendingPersonalityComponent = NewObject<USIPPetPersonalityJsonComponent>(this, TEXT("PendingPetPersonalityJsonComponent"));
		if (PendingPersonalityComponent)
		{
			PendingPersonalityComponent->bAutoApplyGeneratedConfig = false;
			PendingPersonalityComponent->bApplyColorToOwnerMeshes = false;
			PendingPersonalityComponent->bApplyBridgeStyleToCliffBridge = false;
			PendingPersonalityComponent->bShowRuntimeDebugMessage = false;
			PendingPersonalityComponent->OnPersonalityJsonGenerated.AddDynamic(this, &USIPPetPromptSpawnComponent::HandlePendingPersonalityGenerated);
			PendingPersonalityComponent->GeneratePersonalityFromTextWithQwen(Prompt);
			return nullptr;
		}
	}

	return SpawnFinalPet(SpawnNearActor, Prompt, TEXT(""), false, ResolvedPetClass, ResolvedSpeciesName);
}

void USIPPetPromptSpawnComponent::HandlePendingPersonalityGenerated(bool bSuccess, const FString& JsonString)
{
	AActor* SpawnNearActor = PendingSpawnNearActor.Get();
	if (SpawnNearActor)
	{
		SpawnFinalPet(SpawnNearActor, PendingPrompt, JsonString, !JsonString.IsEmpty(), PendingResolvedPetClass, PendingResolvedSpeciesName);
	}

	if (PendingPersonalityComponent)
	{
		PendingPersonalityComponent->OnPersonalityJsonGenerated.RemoveAll(this);
		PendingPersonalityComponent->DestroyComponent();
		PendingPersonalityComponent = nullptr;
	}

	PendingSpawnNearActor.Reset();
	PendingPrompt.Reset();
	PendingResolvedPetClass = nullptr;
	PendingResolvedSpeciesName = NAME_None;
}

AActor* USIPPetPromptSpawnComponent::SpawnFinalPet(AActor* SpawnNearActor, const FString& Prompt, const FString& PersonalityJson, bool bPersonalityReady, TSubclassOf<AActor> ResolvedPetClass, FName ResolvedSpeciesName)
{
	UWorld* World = GetWorld();
	if (!World || !SpawnNearActor)
	{
		return nullptr;
	}

	if (bDestroyPreviousPromptPet && IsValid(LastSpawnedPet))
	{
		LastSpawnedPet->Destroy();
		LastSpawnedPet = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = GetOwner();
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		SpawnParams.Instigator = const_cast<APawn*>(OwnerPawn);
	}

	TSubclassOf<AActor> ClassToSpawn = ResolvedPetClass ? ResolvedPetClass : PetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ASIPPetCharacter::StaticClass();
	}
	LastResolvedSpeciesName = ResolvedSpeciesName;

	const FVector SpawnLocation = ResolveSpawnLocation(SpawnNearActor);
	const FRotator SpawnRotation = SpawnNearActor->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	AActor* SpawnedPet = World->SpawnActorDeferred<AActor>(
		ClassToSpawn,
		SpawnTransform,
		SpawnParams.Owner,
		nullptr,
		SpawnParams.SpawnCollisionHandlingOverride);
	if (!SpawnedPet)
	{
		return nullptr;
	}

	if (APawn* DeferredPetPawn = Cast<APawn>(SpawnedPet))
	{
		DeferredPetPawn->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	}
	SpawnedPet->FinishSpawning(SpawnTransform);

	USIPPetPersonalityJsonComponent* PersonalityComponent = nullptr;
	USIPPetCliffBridgeComponent* BridgeComponent = nullptr;
	EnsurePetComponents(SpawnedPet, SpawnNearActor, PersonalityComponent, BridgeComponent);
	if (USIPPetMoodComponent* MoodComponent = SpawnedPet->FindComponentByClass<USIPPetMoodComponent>())
	{
		MoodComponent->BindToPersonalityComponent(PersonalityComponent);
	}
	SetBlueprintComponentReference(SpawnedPet, PersonalityComponent);
	SetBlueprintFollowTargetReferences(SpawnedPet, SpawnNearActor);

	if (ASIPPetCharacter* PetCharacter = Cast<ASIPPetCharacter>(SpawnedPet))
	{
		PetCharacter->CurrentPetState = EPetState::Companion;
	}

	if (bSpawnDefaultControllerForPet)
	{
		if (APawn* PetPawn = Cast<APawn>(SpawnedPet))
		{
			PetPawn->SpawnDefaultController();
		}
	}

	LastSpawnedPet = SpawnedPet;
	OnPromptPetSpawned.Broadcast(SpawnedPet, PersonalityComponent);

	if (PersonalityComponent)
	{
		if (bPersonalityReady && !PersonalityJson.IsEmpty())
		{
			if (!PersonalityComponent->ApplyPersonalityJson(PersonalityJson))
			{
				PersonalityComponent->GeneratePersonalityFromText(Prompt);
			}
		}
		else if (bUseQwenApi)
		{
			PersonalityComponent->GeneratePersonalityFromTextWithQwen(Prompt);
		}
		else
		{
			PersonalityComponent->GeneratePersonalityFromText(Prompt);
		}
	}

	return SpawnedPet;
}

USIPPetPromptWidget* USIPPetPromptSpawnComponent::ShowPromptWidget(APlayerController* OwningPlayer)
{
	if (ActivePromptWidget)
	{
		return ActivePromptWidget;
	}

	if (!OwningPlayer)
	{
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			OwningPlayer = Cast<APlayerController>(OwnerPawn->GetController());
		}
	}

	if (!OwningPlayer || !PromptWidgetClass)
	{
		return nullptr;
	}

	ActivePromptWidget = CreateWidget<USIPPetPromptWidget>(OwningPlayer, PromptWidgetClass);
	if (!ActivePromptWidget)
	{
		return nullptr;
	}

	ActivePromptWidget->InitializePromptWidget(this);
	ActivePromptWidget->AddToViewport(20);

	OwningPlayer->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ActivePromptWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	OwningPlayer->SetInputMode(InputMode);

	return ActivePromptWidget;
}

void USIPPetPromptSpawnComponent::HidePromptWidget()
{
	if (ActivePromptWidget)
	{
		ActivePromptWidget->RemoveFromParent();
		ActivePromptWidget = nullptr;
	}
}

void USIPPetPromptSpawnComponent::SetBlueprintComponentReference(AActor* PetActor, USIPPetPersonalityJsonComponent* PersonalityComponent) const
{
	if (!PetActor || !PersonalityComponent)
	{
		return;
	}

	static const FName PersonalityJsonVariableName(TEXT("SIPPetPersonalityJson"));
	if (FObjectProperty* ObjectProperty = FindFProperty<FObjectProperty>(PetActor->GetClass(), PersonalityJsonVariableName))
	{
		if (ObjectProperty->PropertyClass && PersonalityComponent->IsA(ObjectProperty->PropertyClass))
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(PetActor, PersonalityComponent);
		}
	}
}

void USIPPetPromptSpawnComponent::SetBlueprintFollowTargetReferences(AActor* PetActor, AActor* FollowTarget) const
{
	if (!PetActor || !FollowTarget)
	{
		return;
	}

	const FName ObjectReferenceNames[] = {
		TEXT("FollowTarget"),
		TEXT("FollowActor"),
		TEXT("TargetActor"),
		TEXT("TargetPlayer"),
		TEXT("PlayerActor"),
		TEXT("OwnerPlayer"),
		TEXT("Player"),
		TEXT("PlayerPawn")
	};

	for (const FName& VariableName : ObjectReferenceNames)
	{
		if (FObjectProperty* ObjectProperty = FindFProperty<FObjectProperty>(PetActor->GetClass(), VariableName))
		{
			if (ObjectProperty->PropertyClass && FollowTarget->IsA(ObjectProperty->PropertyClass))
			{
				ObjectProperty->SetObjectPropertyValue_InContainer(PetActor, FollowTarget);
			}
		}
	}

	const FName BoolReferenceNames[] = {
		TEXT("bIsFollowing"),
		TEXT("bFollowPlayer"),
		TEXT("bShouldFollow"),
		TEXT("bIsCompanion")
	};

	for (const FName& VariableName : BoolReferenceNames)
	{
		if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(PetActor->GetClass(), VariableName))
		{
			BoolProperty->SetPropertyValue_InContainer(PetActor, true);
		}
	}
}

FVector USIPPetPromptSpawnComponent::ResolveSpawnLocation(AActor* SpawnNearActor) const
{
	if (!SpawnNearActor)
	{
		return FVector::ZeroVector;
	}

	FVector Forward = SpawnNearActor->GetActorForwardVector();
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		Forward = FVector::ForwardVector;
	}

	FVector Right = SpawnNearActor->GetActorRightVector();
	Right.Z = 0.0f;
	if (!Right.Normalize())
	{
		Right = FVector::RightVector;
	}

	return SpawnNearActor->GetActorLocation()
		+ Forward * SpawnDistance
		+ Right * SpawnSideOffset
		+ FVector(0.0f, 0.0f, SpawnZOffset);
}

TSubclassOf<AActor> USIPPetPromptSpawnComponent::ResolvePetClassFromPrompt(const FString& Prompt, FName& OutSpeciesName) const
{
	OutSpeciesName = NAME_None;
	const FString LowerPrompt = Prompt.ToLower();

	for (const FSIPPetSpeciesClassRule& Rule : SpeciesClassRules)
	{
		if (!Rule.PetClass)
		{
			continue;
		}

		for (const FString& Keyword : Rule.Keywords)
		{
			if (PromptContainsKeyword(LowerPrompt, Keyword))
			{
				OutSpeciesName = Rule.SpeciesName;
				return Rule.PetClass;
			}
		}
	}

	if (PetClass)
	{
		return PetClass;
	}

	return ASIPPetCharacter::StaticClass();
}

void USIPPetPromptSpawnComponent::EnsurePetComponents(
	AActor* PetActor,
	AActor* FollowTarget,
	USIPPetPersonalityJsonComponent*& OutPersonalityComponent,
	USIPPetCliffBridgeComponent*& OutBridgeComponent) const
{
	OutPersonalityComponent = nullptr;
	OutBridgeComponent = nullptr;

	if (!PetActor)
	{
		return;
	}

	OutPersonalityComponent = PetActor->FindComponentByClass<USIPPetPersonalityJsonComponent>();
	if (!OutPersonalityComponent)
	{
		OutPersonalityComponent = NewObject<USIPPetPersonalityJsonComponent>(PetActor, TEXT("PetPersonalityJsonComponent"));
		if (OutPersonalityComponent)
		{
			OutPersonalityComponent->RegisterComponent();
			PetActor->AddInstanceComponent(OutPersonalityComponent);
		}
	}

	OutBridgeComponent = PetActor->FindComponentByClass<USIPPetCliffBridgeComponent>();
	if (!OutBridgeComponent)
	{
		OutBridgeComponent = NewObject<USIPPetCliffBridgeComponent>(PetActor, TEXT("PetCliffBridgeComponent"));
		if (OutBridgeComponent)
		{
			OutBridgeComponent->RegisterComponent();
			PetActor->AddInstanceComponent(OutBridgeComponent);
		}
	}

	if (!PetActor->FindComponentByClass<USIPPetMoodComponent>())
	{
		USIPPetMoodComponent* MoodComponent = NewObject<USIPPetMoodComponent>(PetActor, TEXT("PetMoodComponent"));
		if (MoodComponent)
		{
			MoodComponent->RegisterComponent();
			PetActor->AddInstanceComponent(MoodComponent);
		}
	}

	if (bAddSimpleFollowComponent)
	{
		USIPPetFollowComponent* FollowComponent = PetActor->FindComponentByClass<USIPPetFollowComponent>();
		if (!FollowComponent)
		{
			FollowComponent = NewObject<USIPPetFollowComponent>(PetActor, TEXT("PetSimpleFollowComponent"));
			if (FollowComponent)
			{
				FollowComponent->RegisterComponent();
				PetActor->AddInstanceComponent(FollowComponent);
			}
		}

		if (FollowComponent)
		{
			FollowComponent->SetFollowTarget(FollowTarget);
		}
	}
}
