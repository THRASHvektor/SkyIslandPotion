// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/SIPCombatStatics.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/SIPCombatSettings.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "SIPGameplayTags.h"
#include "SIPLogCategory.h"

namespace
{
    // Resolve the effective GE class, falling back to project defaults from USIPCombatSettings.
    TSubclassOf<UGameplayEffect> ResolveEffectClass(TSubclassOf<UGameplayEffect> Override, TSoftClassPtr<UGameplayEffect> DefaultSoftClass)
    {
        if (Override)
        {
            return Override;
        }

        if (DefaultSoftClass.IsNull())
        {
            return nullptr;
        }

        // Load synchronously - project defaults are expected to be tiny GE classes and should stay in memory.
        return DefaultSoftClass.LoadSynchronous();
    }

    bool ApplySetByCallerEffect(
        AActor* Target,
        TSubclassOf<UGameplayEffect> EffectClass,
        const FGameplayTag& SetByCallerTag,
        float Magnitude,
        AActor* Instigator,
        UObject* SourceObject)
    {
        if (!Target || !EffectClass || !SetByCallerTag.IsValid() || Magnitude <= 0.0f)
        {
            return false;
        }

        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
        if (!TargetASC)
        {
            UE_LOG(LogSIP, Verbose, TEXT("SIPCombatStatics: target %s has no ASC, skipping GE %s."),
                *GetNameSafe(Target), *GetNameSafe(EffectClass));
            return false;
        }

        // Build the effect context. Using the target's ASC keeps the context in the correct world/net role.
        FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
        AActor* EffectiveInstigator = Instigator ? Instigator : Target;
        ContextHandle.AddInstigator(EffectiveInstigator, EffectiveInstigator);
        if (SourceObject)
        {
            ContextHandle.AddSourceObject(SourceObject);
        }

        const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
        if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
        {
            UE_LOG(LogSIP, Warning, TEXT("SIPCombatStatics: failed to build outgoing spec for %s on %s."),
                *GetNameSafe(EffectClass), *GetNameSafe(Target));
            return false;
        }

        SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerTag, Magnitude);

        const FActiveGameplayEffectHandle ActiveHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
        return ActiveHandle.WasSuccessfullyApplied();
    }
}

bool USIPCombatStatics::ApplyDamageToTarget(
    AActor* Target,
    float DamageAmount,
    AActor* Instigator,
    UObject* SourceObject,
    TSubclassOf<UGameplayEffect> DamageEffectClass)
{
    if (DamageAmount <= 0.0f)
    {
        return false;
    }

    const TSubclassOf<UGameplayEffect> EffectClass = ResolveEffectClass(DamageEffectClass, USIPCombatSettings::Get().DefaultDamageEffect);
    if (!EffectClass)
    {
        UE_LOG(LogSIP, Warning, TEXT("SIPCombatStatics: no damage GE resolved (Project Settings -> Game -> SIP Combat -> DefaultDamageEffect is empty)."));
        return false;
    }

    return ApplySetByCallerEffect(Target, EffectClass, SIPGameplayTags::SIPData_Damage, DamageAmount, Instigator, SourceObject);
}

bool USIPCombatStatics::ApplyHealToTarget(
    AActor* Target,
    float HealAmount,
    AActor* Instigator,
    UObject* SourceObject,
    TSubclassOf<UGameplayEffect> HealEffectClass)
{
    if (HealAmount <= 0.0f)
    {
        return false;
    }

    const TSubclassOf<UGameplayEffect> EffectClass = ResolveEffectClass(HealEffectClass, USIPCombatSettings::Get().DefaultHealEffect);
    if (!EffectClass)
    {
        UE_LOG(LogSIP, Verbose, TEXT("SIPCombatStatics: no heal GE resolved."));
        return false;
    }

    return ApplySetByCallerEffect(Target, EffectClass, SIPGameplayTags::SIPData_Heal, HealAmount, Instigator, SourceObject);
}
