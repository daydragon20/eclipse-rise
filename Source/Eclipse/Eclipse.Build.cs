using UnrealBuildTool;

public class Eclipse : ModuleRules
{
	public Eclipse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// GDD 12.2 fixes the layout to subsystem folders directly under the module
		// (no Public/Private split); expose the module root so includes are written
		// as "Core/..." etc. from anywhere in (or dependent on) this module.
		PublicIncludePaths.Add(ModuleDirectory);

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
