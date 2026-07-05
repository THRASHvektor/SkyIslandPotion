// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/SIPCombatSettings.h"

USIPCombatSettings::USIPCombatSettings()
{
    CategoryName = TEXT("Game");
    SectionName = TEXT("SIP Combat");
}

const USIPCombatSettings& USIPCombatSettings::Get()
{
    return *GetDefault<USIPCombatSettings>();
}
