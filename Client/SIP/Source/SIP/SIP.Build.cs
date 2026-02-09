// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SIP : ModuleRules
{
	public SIP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "SIP"
            }
        );

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });


	}
}
