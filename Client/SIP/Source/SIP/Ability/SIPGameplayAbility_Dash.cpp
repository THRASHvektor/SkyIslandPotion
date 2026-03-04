#include "SIPGameplayAbility_Dash.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "SIPLogCategory.h"
#include "SIPGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "GameplayEffect.h"
#include "NiagaraFunctionLibrary.h"

USIPGameplayAbility_Dash::USIPGameplayAbility_Dash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Dash")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("InputTag.Dash")));
}

bool USIPGameplayAbility_Dash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (MovementComp && MovementComp->IsFalling())
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("DashAbility: Cannot dash while falling"));
		return false;
	}

	return true;
}

void USIPGameplayAbility_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	FVector DashDirection = CalculateDashDirection();
	
	if (PerformDash(DashDirection))
	{
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash completed successfully"));

		if (DashCooldownEffect)
		{
			UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
			if (ASC)
			{
				FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
				EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
				
				ASC->ApplyGameplayEffectToSelf(
					DashCooldownEffect->GetDefaultObject<UGameplayEffect>(),
					1.0f,
					EffectContext
				);
				
				UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash cooldown applied"));
			}
		}
	}
	else
	{
		UE_LOG(LogSIPAbilitySystem, Warning, TEXT("Dash failed"));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USIPGameplayAbility_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FVector USIPGameplayAbility_Dash::CalculateDashDirection() const
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	FVector DashDirection = FVector::ForwardVector;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (PC)
	{
		FRotator ControlRotation = PC->GetControlRotation();
		ControlRotation.Pitch = 0.0f;
		ControlRotation.Roll = 0.0f;
		
		FVector ForwardDir = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::Y);
		FVector RightDir = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);
		
		float MoveForward = Character->GetInputAxisValue(TEXT("MoveForward"));
		float MoveRight = Character->GetInputAxisValue(TEXT("MoveRight"));

		if (FMath::Abs(MoveForward) > 0.1f || FMath::Abs(MoveRight) > 0.1f)
		{
			DashDirection = (ForwardDir * MoveForward + RightDir * MoveRight).GetSafeNormal();
		}
		else
		{
			DashDirection = ForwardDir;
		}
	}

	return DashDirection;
}

bool USIPGameplayAbility_Dash::PerformDash(const FVector& DashDirection)
{
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return false;
	}

	FVector StartLocation = Character->GetActorLocation();
	FVector DashTarget = StartLocation + DashDirection * DashDistance;

	if (bCheckCollision)
	{
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(Character);

		FHitResult HitResult;
		bool bHit = UKismetSystemLibrary::LineTraceSingle(
			Character,
			StartLocation,
			DashTarget,
			UEngineTypes::ConvertToTraceType(ECC_WorldStatic),
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			HitResult,
			true
		);

		if (bHit)
		{
			DashTarget = HitResult.Location - DashDirection * 50.0f;
			UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash blocked by collision, adjusting target"));
		}
	}

	FVector MidPoint = (StartLocation + DashTarget) * 0.5f;
	MidPoint.Z = FMath::Max(StartLocation.Z, DashTarget.Z) + 20.0f;

	if (DashTrailEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			DashTrailEffect,
			MidPoint,
			Character->GetActorRotation()
		);
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash trail effect spawned"));
	}

	Character->SetActorLocation(DashTarget, true);
	
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (MovementComp)
	{
		MovementComp->Velocity = DashDirection * 100.0f;
	}

	if (DashLandedEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			DashLandedEffect,
			DashTarget,
			Character->GetActorRotation()
		);
		UE_LOG(LogSIPAbilitySystem, Log, TEXT("Dash landed effect spawned"));
	}

	return true;
}
