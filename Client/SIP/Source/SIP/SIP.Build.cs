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

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core", 
            "CoreUObject", 
            "Engine",
            "UMG",
            "GameplayTags",
            "InputCore", 
            "GameplayAbilities",
            "GameplayTasks",
            "EnhancedInput",
            "AnimationLocomotionLibraryRuntime",
            "AnimationWarpingRuntime",
            "MotionWarping",
            "PoseSearch",
            "Chooser",
            "ProxyTable",
            "StateTreeModule",
            "NiagaraCore",
            "Niagara",
            "AIModule",
            "NavigationSystem",
            "HTTP",
            "Json",
            "PCG",
            "DeveloperSettings" });
        
        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore",
        });


	}
}
