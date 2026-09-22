// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Action_RogueLike : ModuleRules
{
	public Action_RogueLike(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"Paper2D",
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
			"Action_RogueLike",
			"Action_RogueLike/Variant_Strategy",
			"Action_RogueLike/Variant_Strategy/UI",
			"Action_RogueLike/Variant_TwinStick",
			"Action_RogueLike/Variant_TwinStick/AI",
			"Action_RogueLike/Variant_TwinStick/Gameplay",
			"Action_RogueLike/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
