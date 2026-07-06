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
            "HTTP",
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
            "Json",
            "PCG" });
        
        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate",
            "SlateCore",
        });


	}
}
