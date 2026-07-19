using UnrealBuildTool;

public class Eclipse : ModuleRules
{
	public Eclipse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Dependency set mirrors the Phase 1 architecture decisions (GDD 12.1):
		// GAS for all abilities/attributes, EnhancedInput for context stacks,
		// AIModule/NavigationSystem for squad AI, GameplayTags for the event bus.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"GameplayTasks",
			"GameplayAbilities",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"UMG",
			"CommonUI",
			"CommonInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
	}
}
