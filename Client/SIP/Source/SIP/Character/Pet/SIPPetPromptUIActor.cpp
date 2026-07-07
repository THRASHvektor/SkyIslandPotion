// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Pet/SIPPetPromptUIActor.h"

#include "Character/Pet/Components/SIPPetCliffBridgeComponent.h"
#include "Character/Pet/Components/SIPPetMoodComponent.h"
#include "Character/Pet/Components/SIPPetPersonalityComponent.h"
#include "Character/Pet/Components/SIPPetPersonalityJsonComponent.h"
#include "Character/Pet/Components/SIPPetPromptSpawnComponent.h"
#include "Character/Pet/SIPPetPromptSettings.h"
#include "Character/Pet/UI/SIPPetThoughtWidget.h"
#include "Character/Pet/UI/SIPPetPromptWidget.h"
#include "Character/Pet/World/SIPPetElementalFieldActor.h"
#include "Character/SIPEnemyCharacter.h"
#include "Combat/SIPCombatStatics.h"
#include "Components/InputComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

namespace
{
const FName SIPShadowCloakTag(TEXT("SIPShadowCloaked"));

FString CodepointString(TCHAR Codepoint)
{
	FString Result;
	Result.AppendChar(Codepoint);
	return Result;
}

void AddSpeciesRule(
	USIPPetPromptSpawnComponent* SpawnComponent,
	const FName SpeciesName,
	const TSubclassOf<AActor> PetClass,
	const TArray<FString>& Keywords)
{
	if (!SpawnComponent || !PetClass)
	{
		return;
	}

	FSIPPetSpeciesClassRule Rule;
	Rule.SpeciesName = SpeciesName;
	Rule.PetClass = PetClass;
	Rule.Keywords = Keywords;
	SpawnComponent->SpeciesClassRules.Add(Rule);
}
}

ASIPPetPromptUIActor::ASIPPetPromptUIActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PromptToggleKey = EKeys::Tab;
	WindFieldKey = EKeys::R;
	WindFieldActorClass = ASIPPetElementalFieldActor::StaticClass();
	ThoughtWidgetClass = USIPPetThoughtWidget::StaticClass();
}

void ASIPPetPromptUIActor::BeginPlay()
{
	Super::BeginPlay();
	TryInitializePromptUI();
}

void ASIPPetPromptUIActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (RuntimePromptSpawnComponent)
	{
		RuntimePromptSpawnComponent->OnPromptPetSpawned.RemoveDynamic(this, &ASIPPetPromptUIActor::HandlePromptPetSpawned);
	}

	if (ActiveMoodComponent)
	{
		ActiveMoodComponent->OnMoodThoughtGenerated.RemoveDynamic(this, &ASIPPetPromptUIActor::HandleMoodThoughtGenerated);
	}

	HideThoughtWidget();
	Super::EndPlay(EndPlayReason);
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

	if (bUseProjectPetPromptSettings)
	{
		const USIPPetPromptSettings* PetPromptSettings = GetDefault<USIPPetPromptSettings>();
		if (PetPromptSettings)
		{
			if (UClass* SettingsPetClass = PetPromptSettings->DefaultPetClass.LoadSynchronous())
			{
				RuntimePromptSpawnComponent->PetClass = SettingsPetClass;
			}

			if (UClass* SettingsPromptWidgetClass = PetPromptSettings->PromptWidgetClass.LoadSynchronous())
			{
				RuntimePromptSpawnComponent->PromptWidgetClass = SettingsPromptWidgetClass;
			}

			if (UClass* SettingsWindFieldActorClass = PetPromptSettings->WindFieldActorClass.LoadSynchronous())
			{
				WindFieldActorClass = SettingsWindFieldActorClass;
			}

			if (UNiagaraSystem* SettingsWindFieldVFX = PetPromptSettings->WindFieldVFX.LoadSynchronous())
			{
				WindFieldVFX = SettingsWindFieldVFX;
			}

			if (UNiagaraSystem* SettingsWaterFieldVFX = PetPromptSettings->WaterFieldVFX.LoadSynchronous())
			{
				WaterFieldVFX = SettingsWaterFieldVFX;
			}

			if (UClass* SettingsThunderStrikeActorClass = PetPromptSettings->ThunderStrikeActorClass.LoadSynchronous())
			{
				ThunderStrikeActorClass = SettingsThunderStrikeActorClass;
			}

			if (UNiagaraSystem* SettingsPlantFieldVFX = PetPromptSettings->PlantFieldVFX.LoadSynchronous())
			{
				PlantFieldVFX = SettingsPlantFieldVFX;
			}

			if (UNiagaraSystem* SettingsShadowCloakVFX = PetPromptSettings->ShadowCloakVFX.LoadSynchronous())
			{
				ShadowCloakVFX = SettingsShadowCloakVFX;
			}
			bEnableShadowCloakVFX = PetPromptSettings->bEnableShadowCloakVFX;

			if (UStaticMesh* SettingsBridgeStepMesh = PetPromptSettings->BridgeStepMesh.LoadSynchronous())
			{
				BridgeStepMeshOverride = SettingsBridgeStepMesh;
			}

			if (UMaterialInterface* SettingsBridgeStepMaterial = PetPromptSettings->BridgeStepMaterial.LoadSynchronous())
			{
				BridgeStepMaterialOverride = SettingsBridgeStepMaterial;
			}

			RuntimePromptSpawnComponent->bUseQwenApi = PetPromptSettings->bUseQwenApi;
			RuntimePromptSpawnComponent->bShowResultJsonInWidget = PetPromptSettings->bShowResultJsonInWidget;
			RuntimePromptSpawnComponent->bAddSimpleFollowComponent = PetPromptSettings->bAddSimpleFollowComponent;
			RuntimePromptSpawnComponent->SpawnDistance = PetPromptSettings->SpawnDistance;
			RuntimePromptSpawnComponent->SpawnSideOffset = PetPromptSettings->SpawnSideOffset;
			bShowPromptWidgetOnBeginPlay = PetPromptSettings->bShowPromptWidgetOnBeginPlay;
			PromptToggleKey = PetPromptSettings->PromptToggleKey;

			if (UClass* SettingsCatClass = PetPromptSettings->CatPetClass.LoadSynchronous())
			{
				CatPetClass = SettingsCatClass;
			}

			if (UClass* SettingsDogClass = PetPromptSettings->DogPetClass.LoadSynchronous())
			{
				DogPetClass = SettingsDogClass;
			}

			if (UClass* SettingsDragonClass = PetPromptSettings->DragonPetClass.LoadSynchronous())
			{
				DragonPetClass = SettingsDragonClass;
			}
		}
	}

	if (PetClass)
	{
		RuntimePromptSpawnComponent->PetClass = PetClass;
	}

	if (!bUseProjectPetPromptSettings)
	{
		RuntimePromptSpawnComponent->bUseQwenApi = bUseQwenApi;
		RuntimePromptSpawnComponent->bShowResultJsonInWidget = bShowResultJsonInWidget;
		RuntimePromptSpawnComponent->bAddSimpleFollowComponent = bAddSimpleFollowComponent;
		RuntimePromptSpawnComponent->SpawnDistance = SpawnDistance;
		RuntimePromptSpawnComponent->SpawnSideOffset = SpawnSideOffset;
	}

	RuntimePromptSpawnComponent->bAutoShowPromptWidget = false;
	ConfigureSpeciesClassRules(RuntimePromptSpawnComponent);
	BindPromptSpawnEvents();

	if (PromptWidgetClass)
	{
		RuntimePromptSpawnComponent->PromptWidgetClass = PromptWidgetClass;
	}

	if (bShowPromptWidgetOnBeginPlay)
	{
		RuntimePromptSpawnComponent->ShowPromptWidget(PlayerController);
	}
	else
	{
		RuntimePromptSpawnComponent->HidePromptWidget();
		PlayerController->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}

	BindActivePetMood(ResolveActivePetActor());
	BindPromptInput(PlayerController);
	BindSkillInput(PlayerController);
}

void ASIPPetPromptUIActor::BindPromptInput(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	EnableInput(PlayerController);
	if (InputComponent)
	{
		if (PromptToggleKey.IsValid())
		{
			FInputKeyBinding& ConfiguredBinding = InputComponent->BindKey(PromptToggleKey, IE_Pressed, this, &ASIPPetPromptUIActor::TogglePromptWidget);
			ConfiguredBinding.bConsumeInput = false;
			ConfiguredBinding.bExecuteWhenPaused = true;
		}

		if (PromptToggleKey != EKeys::Tab)
		{
			FInputKeyBinding& TabBinding = InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ASIPPetPromptUIActor::TogglePromptWidget);
			TabBinding.bConsumeInput = false;
			TabBinding.bExecuteWhenPaused = true;
		}
	}
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

void ASIPPetPromptUIActor::BindPromptSpawnEvents()
{
	if (!RuntimePromptSpawnComponent)
	{
		return;
	}

	if (!RuntimePromptSpawnComponent->OnPromptPetSpawned.IsAlreadyBound(this, &ASIPPetPromptUIActor::HandlePromptPetSpawned))
	{
		RuntimePromptSpawnComponent->OnPromptPetSpawned.AddDynamic(this, &ASIPPetPromptUIActor::HandlePromptPetSpawned);
	}
}

void ASIPPetPromptUIActor::BindActivePetMood(AActor* PetActor)
{
	if (ActiveMoodComponent)
	{
		ActiveMoodComponent->OnMoodThoughtGenerated.RemoveDynamic(this, &ASIPPetPromptUIActor::HandleMoodThoughtGenerated);
		ActiveMoodComponent = nullptr;
	}

	if (!PetActor)
	{
		return;
	}

	ActiveMoodComponent = PetActor->FindComponentByClass<USIPPetMoodComponent>();
	if (ActiveMoodComponent && !ActiveMoodComponent->OnMoodThoughtGenerated.IsAlreadyBound(this, &ASIPPetPromptUIActor::HandleMoodThoughtGenerated))
	{
		ActiveMoodComponent->OnMoodThoughtGenerated.AddDynamic(this, &ASIPPetPromptUIActor::HandleMoodThoughtGenerated);
	}
}

void ASIPPetPromptUIActor::HideThoughtWidget()
{
	GetWorldTimerManager().ClearTimer(ThoughtWidgetTimerHandle);
	if (ActiveThoughtWidget)
	{
		ActiveThoughtWidget->RemoveFromParent();
		ActiveThoughtWidget = nullptr;
	}
}

void ASIPPetPromptUIActor::HandlePromptPetSpawned(AActor* SpawnedPet, USIPPetPersonalityJsonComponent* PersonalityComponent)
{
	BindActivePetMood(SpawnedPet);
}

void ASIPPetPromptUIActor::HandleMoodThoughtGenerated(ESIPPetMoodType Mood, const FString& Thought)
{
	const FString TrimmedThought = Thought.TrimStartAndEnd();
	if (TrimmedThought.IsEmpty())
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	if (!ThoughtWidgetClass)
	{
		ThoughtWidgetClass = USIPPetThoughtWidget::StaticClass();
	}

	if (!ActiveThoughtWidget)
	{
		ActiveThoughtWidget = CreateWidget<USIPPetThoughtWidget>(PlayerController, ThoughtWidgetClass);
		if (ActiveThoughtWidget)
		{
			ActiveThoughtWidget->AddToViewport(30);
		}
	}

	if (ActiveThoughtWidget)
	{
		ActiveThoughtWidget->SetThoughtText(TrimmedThought);
		GetWorldTimerManager().ClearTimer(ThoughtWidgetTimerHandle);
		GetWorldTimerManager().SetTimer(ThoughtWidgetTimerHandle, this, &ASIPPetPromptUIActor::HideThoughtWidget, ThoughtWidgetDuration, false);
	}
}

void ASIPPetPromptUIActor::ActivateThunderStrike(UWorld* World, APawn* PlayerPawn)
{
	if (!World || !PlayerPawn)
	{
		return;
	}

	if (!WindFieldActorClass)
	{
		WindFieldActorClass = ASIPPetElementalFieldActor::StaticClass();
	}

	const FVector StrikeLocation = ResolveThunderStrikeLocation(World, PlayerPawn);
	const FTransform StrikeTransform(FRotator::ZeroRotator, StrikeLocation);

	if (ThunderStrikeActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = PlayerPawn;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AActor* StrikeActor = World->SpawnActor<AActor>(
			ThunderStrikeActorClass,
			StrikeLocation,
			PlayerPawn->GetActorRotation(),
			SpawnParams))
		{
			if (ThunderStrikeDuration > 0.0f)
			{
				StrikeActor->SetLifeSpan(ThunderStrikeDuration);
			}
		}
	}

	ApplyThunderStrikeDamage(World, PlayerPawn, StrikeLocation);
}

FVector ASIPPetPromptUIActor::ResolveThunderStrikeLocation(UWorld* World, APawn* PlayerPawn) const
{
	if (!World || !PlayerPawn)
	{
		return FVector::ZeroVector;
	}

	const FVector TargetCenter = PlayerPawn->GetActorLocation() + PlayerPawn->GetActorForwardVector() * ThunderStrikeDistance;
	const FVector TraceStart = TargetCenter + FVector(0.0f, 0.0f, 1800.0f);
	const FVector TraceEnd = TargetCenter - FVector(0.0f, 0.0f, 2600.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SIPPetThunderStrikeTrace), false, PlayerPawn);
	if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return HitResult.ImpactPoint + FVector(0.0f, 0.0f, 18.0f);
	}

	return TargetCenter;
}

void ASIPPetPromptUIActor::ApplyThunderStrikeDamage(UWorld* World, APawn* PlayerPawn, const FVector& StrikeLocation) const
{
	if (!World || ThunderStrikeDamage <= 0.0f)
	{
		return;
	}

	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASIPEnemyCharacter::StaticClass(), EnemyActors);
	const float StrikeRadiusSquared = FMath::Square(FMath::Max(0.0f, ThunderStrikeRadius));

	for (AActor* EnemyActor : EnemyActors)
	{
		if (!EnemyActor || FVector::DistSquared2D(EnemyActor->GetActorLocation(), StrikeLocation) > StrikeRadiusSquared)
		{
			continue;
		}

		USIPCombatStatics::ApplyDamageToTarget(EnemyActor, ThunderStrikeDamage, PlayerPawn, nullptr);
	}
}

void ASIPPetPromptUIActor::TogglePromptWidget()
{
	if (!RuntimePromptSpawnComponent)
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (RuntimePromptSpawnComponent->ActivePromptWidget)
	{
		RuntimePromptSpawnComponent->HidePromptWidget();
		if (PlayerController)
		{
			PlayerController->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
		}
		return;
	}

	RuntimePromptSpawnComponent->ShowPromptWidget(PlayerController);
}

void ASIPPetPromptUIActor::ConfigureSpeciesClassRules(USIPPetPromptSpawnComponent* SpawnComponent) const
{
	if (!SpawnComponent)
	{
		return;
	}

	SpawnComponent->SpeciesClassRules.Reset();

	AddSpeciesRule(
		SpawnComponent,
		TEXT("Cat"),
		CatPetClass,
		{
			TEXT("cat"),
			TEXT("kitty"),
			TEXT("feline"),
			CodepointString(0x732B)
		});

	AddSpeciesRule(
		SpawnComponent,
		TEXT("Dog"),
		DogPetClass,
		{
			TEXT("dog"),
			TEXT("puppy"),
			TEXT("canine"),
			CodepointString(0x72D7),
			CodepointString(0x72AC)
		});

	AddSpeciesRule(
		SpawnComponent,
		TEXT("Dragon"),
		DragonPetClass,
		{
			TEXT("dragon"),
			TEXT("drake"),
			TEXT("wyrm"),
			CodepointString(0x9F99)
		});
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

	const USIPPetPersonalityComponent* PersonalityComponent = ResolveActivePetPersonality();
	const USIPPetPersonalityJsonComponent* JsonPersonalityComponent = ResolveActivePetJsonPersonality();
	const ESIPPetElementType ElementType = JsonPersonalityComponent ? JsonPersonalityComponent->GetCurrentConfig().Element : ESIPPetElementType::Wind;

	FSIPPetBehaviourTuning SkillTuning;
	if (PersonalityComponent)
	{
		SkillTuning = PersonalityComponent->BehaviourTuning;
	}
	else
	{
		SkillTuning.SkillLabel = TEXT("Default Wind Updraft");
		SkillTuning.SkillColor = FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
		SkillTuning.SkillRadius = WindFieldRadius;
		SkillTuning.SkillDuration = WindFieldDuration;
		SkillTuning.WindLaunchVelocity = WindFieldLaunchVelocity;
	}

	FLinearColor SkillColor = FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
	float SkillRadius = SkillTuning.SkillRadius;
	float SkillDuration = SkillTuning.SkillDuration;
	FVector SkillLaunchVelocity = FVector::ZeroVector;
	bool bSpawnField = true;

	switch (ElementType)
	{
	case ESIPPetElementType::Rock:
		SkillColor = FLinearColor(0.60f, 0.42f, 0.22f, 1.0f);
		bSpawnField = false;
		if (AActor* PetActor = ResolveActivePetActor())
		{
			if (USIPPetCliffBridgeComponent* BridgeComponent = PetActor->FindComponentByClass<USIPPetCliffBridgeComponent>())
			{
				if (BridgeStepMeshOverride)
				{
					BridgeComponent->FloatingStepMesh = BridgeStepMeshOverride;
				}

				if (BridgeStepMaterialOverride)
				{
					BridgeComponent->FloatingStepMaterial = BridgeStepMaterialOverride;
				}

				BridgeComponent->bAutoScan = false;
				BridgeComponent->SuppressBridgeScanForSeconds(0.1f);
				BridgeComponent->ForceBuildBridgeInDirection(PlayerPawn->GetActorForwardVector());
			}
		}
		break;
	case ESIPPetElementType::Water:
		SkillColor = FLinearColor(0.02f, 0.32f, 1.0f, 1.0f);
		SkillRadius = FMath::Max(SkillRadius, 560.0f);
		SkillDuration = FMath::Max(SkillDuration, 12.0f);
		SkillLaunchVelocity = FVector(0.0f, 0.0f, 520.0f);
		break;
	case ESIPPetElementType::Thunder:
		ActivateThunderStrike(World, PlayerPawn);
		return;
	case ESIPPetElementType::Plant:
		SkillColor = FLinearColor(0.55f, 1.0f, 0.16f, 1.0f);
		SkillRadius = FMath::Max(SkillRadius, 640.0f);
		SkillDuration = FMath::Max(SkillDuration, 14.0f);
		SkillLaunchVelocity = FVector(0.0f, 0.0f, 720.0f);
		break;
	case ESIPPetElementType::Shadow:
		SkillColor = FLinearColor(0.16f, 0.04f, 0.42f, 1.0f);
		SkillRadius = 360.0f;
		SkillDuration = 5.0f;
		SkillLaunchVelocity = FVector::ZeroVector;
		bSpawnField = false;
		ApplyShadowCloak(PlayerPawn, SkillDuration);
		break;
	case ESIPPetElementType::Wind:
	default:
		SkillColor = FLinearColor(0.08f, 1.0f, 0.42f, 1.0f);
		SkillLaunchVelocity = SkillTuning.WindLaunchVelocity;
		break;
	}

	const FVector SpawnLocation = PlayerPawn->GetActorLocation()
		+ PlayerPawn->GetActorForwardVector() * WindFieldSpawnOffset.X
		+ PlayerPawn->GetActorRightVector() * WindFieldSpawnOffset.Y
		+ FVector(0.0f, 0.0f, WindFieldSpawnOffset.Z);
	const FTransform SpawnTransform(PlayerPawn->GetActorRotation(), SpawnLocation);

	if (bSpawnField)
	{
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

		FieldActor->FieldVFX = ResolveFieldVFXForElement(ElementType);
		FieldActor->FieldRadius = SkillRadius;
		FieldActor->Duration = SkillDuration;
		FieldActor->FieldColor = SkillColor;
		FieldActor->FinishSpawning(SpawnTransform);
	}

	const FVector LaunchVelocity =
		PlayerPawn->GetActorForwardVector() * SkillLaunchVelocity.X
		+ PlayerPawn->GetActorRightVector() * SkillLaunchVelocity.Y
		+ FVector(0.0f, 0.0f, SkillLaunchVelocity.Z);

	if (!LaunchVelocity.IsNearlyZero())
	{
		if (ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerPawn))
		{
			PlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);
		}
		else
		{
			PlayerPawn->AddActorWorldOffset(LaunchVelocity * 0.05f, true);
		}
	}

}

void ASIPPetPromptUIActor::ApplyShadowCloak(APawn* PlayerPawn, float Duration)
{
	if (!PlayerPawn || Duration <= 0.0f)
	{
		return;
	}

	if (APawn* PreviousPawn = ShadowCloakedPawn.Get())
	{
		PreviousPawn->Tags.Remove(SIPShadowCloakTag);
	}

	if (!PlayerPawn->ActorHasTag(SIPShadowCloakTag))
	{
		PlayerPawn->Tags.Add(SIPShadowCloakTag);
	}

	ShadowCloakedPawn = PlayerPawn;

	if (bHidePlayerMeshDuringShadowCloak)
	{
		for (UMeshComponent* HiddenMesh : HiddenShadowCloakMeshes)
		{
			if (HiddenMesh)
			{
				HiddenMesh->SetHiddenInGame(false);
			}
		}
		HiddenShadowCloakMeshes.Reset();

		TArray<UMeshComponent*> MeshComponents;
		PlayerPawn->GetComponents<UMeshComponent>(MeshComponents);
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (MeshComponent && MeshComponent->IsVisible() && !MeshComponent->bHiddenInGame)
			{
				MeshComponent->SetHiddenInGame(true);
				HiddenShadowCloakMeshes.Add(MeshComponent);
			}
		}
	}

	if (bEnableShadowCloakVFX && ShadowCloakVFX)
	{
		if (ShadowCloakVFXComponent)
		{
			ShadowCloakVFXComponent->DestroyComponent();
			ShadowCloakVFXComponent = nullptr;
		}

		ShadowCloakVFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			ShadowCloakVFX,
			PlayerPawn->GetRootComponent(),
			NAME_None,
			ShadowCloakVFXOffset,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true,
			true,
			ENCPoolMethod::None,
			true);
		if (ShadowCloakVFXComponent)
		{
			ShadowCloakVFXComponent->SetWorldScale3D(FVector(FMath::Max(0.05f, ShadowCloakVFXScale)));
		}
	}

	GetWorldTimerManager().ClearTimer(ShadowCloakTimerHandle);
	GetWorldTimerManager().SetTimer(
		ShadowCloakTimerHandle,
		this,
		&ASIPPetPromptUIActor::ClearShadowCloak,
		Duration,
		false);
}

void ASIPPetPromptUIActor::ClearShadowCloak()
{
	if (APawn* PlayerPawn = ShadowCloakedPawn.Get())
	{
		PlayerPawn->Tags.Remove(SIPShadowCloakTag);
	}

	if (ShadowCloakVFXComponent)
	{
		ShadowCloakVFXComponent->DestroyComponent();
		ShadowCloakVFXComponent = nullptr;
	}

	for (UMeshComponent* HiddenMesh : HiddenShadowCloakMeshes)
	{
		if (HiddenMesh)
		{
			HiddenMesh->SetHiddenInGame(false);
		}
	}
	HiddenShadowCloakMeshes.Reset();

	ShadowCloakedPawn.Reset();
}

UNiagaraSystem* ASIPPetPromptUIActor::ResolveFieldVFXForElement(ESIPPetElementType ElementType) const
{
	switch (ElementType)
	{
	case ESIPPetElementType::Water:
		return WaterFieldVFX ? WaterFieldVFX : WindFieldVFX;
	case ESIPPetElementType::Plant:
		return PlantFieldVFX ? PlantFieldVFX : WindFieldVFX;
	case ESIPPetElementType::Wind:
	default:
		return WindFieldVFX;
	}
}

USIPPetPersonalityComponent* ASIPPetPromptUIActor::ResolveActivePetPersonality() const
{
	if (AActor* PetActor = ResolveActivePetActor())
	{
		return PetActor->FindComponentByClass<USIPPetPersonalityComponent>();
	}

	return nullptr;
}

USIPPetPersonalityJsonComponent* ASIPPetPromptUIActor::ResolveActivePetJsonPersonality() const
{
	if (AActor* PetActor = ResolveActivePetActor())
	{
		return PetActor->FindComponentByClass<USIPPetPersonalityJsonComponent>();
	}

	return nullptr;
}

AActor* ASIPPetPromptUIActor::ResolveActivePetActor() const
{
	if (RuntimePromptSpawnComponent && RuntimePromptSpawnComponent->LastSpawnedPet)
	{
		return RuntimePromptSpawnComponent->LastSpawnedPet;
	}

	TArray<AActor*> PetActors;
	UGameplayStatics::GetAllActorsOfClass(this, APawn::StaticClass(), PetActors);
	for (AActor* Actor : PetActors)
	{
		if (!Actor || Actor == UGameplayStatics::GetPlayerPawn(this, 0))
		{
			continue;
		}

		if (Actor->FindComponentByClass<USIPPetPersonalityComponent>() || Actor->FindComponentByClass<USIPPetPersonalityJsonComponent>())
		{
			return Actor;
		}
	}

	return nullptr;
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
