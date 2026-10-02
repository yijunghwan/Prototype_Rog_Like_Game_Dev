using UnrealBuildTool;

public class Action_RogueLikeEditor : ModuleRules
{
	public Action_RogueLikeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"PropertyEditor",
			"UnrealEd",
			"Action_RogueLike"
		});
	}
}
