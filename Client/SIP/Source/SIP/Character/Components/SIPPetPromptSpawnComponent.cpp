// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Components/SIPPetPromptSpawnComponent.h"

#include "Character/SIPPetCharacter.h"
#include "Character/Components/SIPPetCliffBridgeComponent.h"
#include "Character/Components/SIPPetFollowComponent.h"
#include "Character/Components/SIPPetPersonalityJsonComponent.h"
#include "UI/SIPPetPromptWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"

USIPPetPromptSpawnComponent::USIPPetPromptSpawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PetClass = ASIPPetCharacter::StaticClass();
	PromptWidgetClass = USIPPetPromptWidget::StaticClass();
}

void USIPPetPromptSpawnComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoShowPromptWidget)
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

	if (bUseQwenApi && bWaitForPersonalityBeforeSpawn)
	{
		PendingPrompt = Prompt;
		PendingSpawnNearActor = SpawnNearActor;

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

	return SpawnFinalPet(SpawnNearActor, Prompt, TEXT(""), false);
}

void USIPPetPromptSpawnComponent::HandlePendingPersonalityGenerated(bool bSuccess, const FString& JsonString)
{
	AActor* SpawnNearActor = PendingSpawnNearActor.Get();
	if (SpawnNearActor)
	{
		SpawnFinalPet(SpawnNearActor, PendingPrompt, JsonString, !JsonString.IsEmpty());
	}

	if (PendingPersonalityComponent)
	{
		PendingPersonalityComponent->OnPersonalityJsonGenerated.RemoveAll(this);
		PendingPersonalityComponent->DestroyComponent();
		PendingPersonalityComponent = nullptr;
	}

	PendingSpawnNearActor.Reset();
	PendingPrompt.Reset();
}

AActor* USIPPetPromptSpawnComponent::SpawnFinalPet(AActor* SpawnNearActor, const FString& Prompt, const FString& PersonalityJson, bool bPersonalityReady)
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

	const FVector SpawnLocation = ResolveSpawnLocation(SpawnNearActor);
	const FRotator SpawnRotation = SpawnNearActor->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	AActor* SpawnedPet = World->SpawnActorDeferred<AActor>(
		PetClass,
		SpawnTransform,
		SpawnParams.Owner,
		nullptr,
		SpawnParams.SpawnCollisionHandlingOverride);
	if (!SpawnedPet)
	{
		return nullptr;
	}

	USIPPetPersonalityJsonComponent* PersonalityComponent = nullptr;
	USIPPetCliffBridgeComponent* BridgeComponent = nullptr;
	EnsurePetComponents(SpawnedPet, SpawnNearActor, PersonalityComponent, BridgeComponent);
	SetBlueprintComponentReference(SpawnedPet, PersonalityComponent);
	SetBlueprintFollowTargetReferences(SpawnedPet, SpawnNearActor);
	if (APawn* DeferredPetPawn = Cast<APawn>(SpawnedPet))
	{
		DeferredPetPawn->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	}
	SpawnedPet->FinishSpawning(SpawnTransform);

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
