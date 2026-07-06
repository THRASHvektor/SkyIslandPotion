// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/SIPPetPromptUIActor.h"

#include "Character/Components/SIPPetPromptSpawnComponent.h"
#include "UI/SIPPetPromptWidget.h"
#include "World/SIPPetElementalFieldActor.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ASIPPetPromptUIActor::ASIPPetPromptUIActor()
{
	PrimaryActorTick.bCanEverTick = false;
	WindFieldKey = EKeys::R;
	WindFieldActorClass = ASIPPetElementalFieldActor::StaticClass();
}

void ASIPPetPromptUIActor::BeginPlay()
{
	Super::BeginPlay();
	TryInitializePromptUI();
}

void ASIPPetPromptUIActor::TryInitializePromptUI()
{
	if (RuntimePromptSpawnComponent && RuntimePromptSpawnComponent->ActivePromptWidget)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerPawn || !PlayerController)
	{
		if (RetryCount < 20)
		{
			++RetryCount;
			GetWorldTimerManager().SetTimer(RetryTimerHandle, this, &ASIPPetPromptUIActor::TryInitializePromptUI, 0.25f, false);
		}
		return;
	}

	RuntimePromptSpawnComponent = FindOrCreatePromptComponent(PlayerPawn);
	if (!RuntimePromptSpawnComponent)
	{
		return;
	}

	if (PetClass)
	{
		RuntimePromptSpawnComponent->PetClass = PetClass;
	}

	RuntimePromptSpawnComponent->bUseQwenApi = bUseQwenApi;
	RuntimePromptSpawnComponent->bShowResultJsonInWidget = bShowResultJsonInWidget;
	RuntimePromptSpawnComponent->bAddSimpleFollowComponent = bAddSimpleFollowComponent;
	RuntimePromptSpawnComponent->SpawnDistance = SpawnDistance;
	RuntimePromptSpawnComponent->SpawnSideOffset = SpawnSideOffset;
	RuntimePromptSpawnComponent->bAutoShowPromptWidget = false;

	if (PromptWidgetClass)
	{
		RuntimePromptSpawnComponent->PromptWidgetClass = PromptWidgetClass;
	}

	RuntimePromptSpawnComponent->ShowPromptWidget(PlayerController);
	BindSkillInput(PlayerController);
}

void ASIPPetPromptUIActor::BindSkillInput(APlayerController* PlayerController)
{
	if (!bEnableWindFieldInput || !PlayerController || !WindFieldKey.IsValid())
	{
		return;
	}

	EnableInput(PlayerController);
	if (InputComponent)
	{
		InputComponent->BindKey(WindFieldKey, IE_Pressed, this, &ASIPPetPromptUIActor::ActivateWindField);
	}
}

void ASIPPetPromptUIActor::ActivateWindField()
{
	UWorld* World = GetWorld();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!World || !PlayerPawn)
	{
		return;
	}

	if (!WindFieldActorClass)
	{
		WindFieldActorClass = ASIPPetElementalFieldActor::StaticClass();
	}

	const FVector SpawnLocation = PlayerPawn->GetActorLocation()
		+ PlayerPawn->GetActorForwardVector() * WindFieldSpawnOffset.X
		+ PlayerPawn->GetActorRightVector() * WindFieldSpawnOffset.Y
		+ FVector(0.0f, 0.0f, WindFieldSpawnOffset.Z);
	const FTransform SpawnTransform(PlayerPawn->GetActorRotation(), SpawnLocation);

	ASIPPetElementalFieldActor* FieldActor = World->SpawnActorDeferred<ASIPPetElementalFieldActor>(
		WindFieldActorClass,
		SpawnTransform,
		this,
		PlayerPawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!FieldActor)
	{
		return;
	}

	FieldActor->FieldVFX = WindFieldVFX;
	FieldActor->FieldRadius = WindFieldRadius;
	FieldActor->Duration = WindFieldDuration;
	FieldActor->FieldColor = FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
	FieldActor->FinishSpawning(SpawnTransform);

	if (ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn))
	{
		PlayerCharacter->LaunchCharacter(WindFieldLaunchVelocity, true, true);
	}
	else
	{
		PlayerPawn->AddActorWorldOffset(WindFieldLaunchVelocity * 0.05f, true);
	}
}

USIPPetPromptSpawnComponent* ASIPPetPromptUIActor::FindOrCreatePromptComponent(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return nullptr;
	}

	if (USIPPetPromptSpawnComponent* ExistingComponent = TargetActor->FindComponentByClass<USIPPetPromptSpawnComponent>())
	{
		return ExistingComponent;
	}

	USIPPetPromptSpawnComponent* NewComponent = NewObject<USIPPetPromptSpawnComponent>(TargetActor, TEXT("RuntimePetPromptSpawnComponent"));
	if (!NewComponent)
	{
		return nullptr;
	}

	NewComponent->RegisterComponent();
	TargetActor->AddInstanceComponent(NewComponent);
	return NewComponent;
}
