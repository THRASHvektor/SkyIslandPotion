// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/SIPPetPromptWorldSubsystem.h"

#include "Character/Pet/SIPPetPromptSettings.h"
#include "Character/Pet/SIPPetPromptUIActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void USIPPetPromptWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld())
	{
		return;
	}

	const USIPPetPromptSettings* PetPromptSettings = GetDefault<USIPPetPromptSettings>();
	if (!PetPromptSettings || !PetPromptSettings->bAutoCreatePromptUIActor)
	{
		return;
	}

	for (TActorIterator<ASIPPetPromptUIActor> It(&InWorld); It; ++It)
	{
		return;
	}

	TSubclassOf<ASIPPetPromptUIActor> UIActorClass = PetPromptSettings->PromptUIActorClass.LoadSynchronous();
	if (!UIActorClass)
	{
		UIActorClass = ASIPPetPromptUIActor::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = TEXT("AutoPetPromptUIActor");
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	InWorld.SpawnActor<ASIPPetPromptUIActor>(UIActorClass, FTransform::Identity, SpawnParams);
}
