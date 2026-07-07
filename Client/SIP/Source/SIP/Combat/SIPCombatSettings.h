// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Templates/SubclassOf.h"

#include "SIPCombatSettings.generated.h"

class UGameplayEffect;

/**
 * SIP combat-related project settings.
 * Configures the default GameplayEffect used to apply damage across the whole project,
 * so gameplay code can call USIPCombatStatics::ApplyDamageToTarget without knowing which GE to use.
 *
 * Reachable in editor: Project Settings -> Game -> "SIP Combat".
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="SIP Combat"))
class SIP_API USIPCombatSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USIPCombatSettings();

    /**
     * The default GameplayEffect used for one-shot damage application.
     * Should be an Instant GE that has a Modifier on USIPHealthSet.Damage using SetByCaller (data tag: SIPData.Damage).
     */
    UPROPERTY(EditAnywhere, Config, Category="Damage", meta=(AllowAbstract="false"))
    TSoftClassPtr<UGameplayEffect> DefaultDamageEffect;

    /**
     * Optional default heal GE (SetByCaller data tag: SIPData.Heal, target attribute: Healing).
     * Not required for damage flow.
     */
    UPROPERTY(EditAnywhere, Config, Category="Heal", meta=(AllowAbstract="false"))
    TSoftClassPtr<UGameplayEffect> DefaultHealEffect;

    static const USIPCombatSettings& Get();
};
