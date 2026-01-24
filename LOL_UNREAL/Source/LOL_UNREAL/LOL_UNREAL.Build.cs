// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LOL_UNREAL : ModuleRules
{
	public LOL_UNREAL(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"LOL_UNREAL",
			"LOL_UNREAL/Variant_Strategy",
			"LOL_UNREAL/Variant_Strategy/UI",
			"LOL_UNREAL/Variant_TwinStick",
			"LOL_UNREAL/Variant_TwinStick/AI",
			"LOL_UNREAL/Variant_TwinStick/Gameplay",
			"LOL_UNREAL/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
