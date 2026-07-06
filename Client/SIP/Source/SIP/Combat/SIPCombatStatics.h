// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"

#include "SIPCombatStatics.generated.h"

class AActor;
class UGameplayEffect;
class UObject;

/**
 * Central helpers for combat damage / healing routed through GAS GameplayEffects.
 * Any gameplay code that needs to hurt or heal a target must go through this API instead of
 * touching Health directly, so all damage flows through the standard GameplayEffect pipeline
 * (context, instigator, source object, tags, cues, immunity, PostGameplayEffectExecute etc.).
 */
UCLASS()
class SIP_API USIPCombatStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Apply damage to a target actor using the project's default damage GE (or an override).
     *
     * @param Target             Actor that owns the AbilitySystemComponent to be hurt.
     * @param DamageAmount       Positive damage amount. Non-positive amounts are ignored.
     * @param Instigator         Actor that caused the damage (weapon owner, ability source, etc.).
     * @param SourceObject       Optional gameplay-source (e.g. the ability, projectile, zone).
     * @param DamageEffectClass  Optional GE class override. When null, USIPCombatSettings::DefaultDamageEffect is used.
     * @return true if the effect was applied to the target.
     */
    UFUNCTION(BlueprintCallable, Category="SIP|Combat", meta=(AdvancedDisplay="SourceObject,DamageEffectClass"))
    static bool ApplyDamageToTarget(
        AActor* Target,
        float DamageAmount,
        AActor* Instigator = nullptr,
        UObject* SourceObject = nullptr,
        TSubclassOf<UGameplayEffect> DamageEffectClass = nullptr);

    /**
     * Apply healing to a target using the project's default heal GE (or an override).
     * Does nothing if no heal GE is configured.
     */
    UFUNCTION(BlueprintCallable, Category="SIP|Combat", meta=(AdvancedDisplay="SourceObject,HealEffectClass"))
    static bool ApplyHealToTarget(
        AActor* Target,
        float HealAmount,
        AActor* Instigator = nullptr,
        UObject* SourceObject = nullptr,
        TSubclassOf<UGameplayEffect> HealEffectClass = nullptr);
};
