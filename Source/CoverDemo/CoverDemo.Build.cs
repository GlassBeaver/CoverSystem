// Copyright (c) 2018 David Nadaski. All Rights Reserved.

using UnrealBuildTool;

public class CoverDemo : ModuleRules
{
	public CoverDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Landscape", "Navmesh", "NavigationSystem" });
        PrivateDependencyModuleNames.AddRange(new string[] { "AIModule", "GameplayTasks", "UMG" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        PublicDefinitions.Add("DEBUG_RENDERING=!(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR");
    }
}
