using UnrealBuildTool;

public class EclipseSaveSystem : ModuleRules
{
	public EclipseSaveSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Flat module layout (matches the game module's GDD 12.2 convention):
		// expose the module root so dependents include headers by name.
		PublicIncludePaths.Add(ModuleDirectory);

		// Mechanism only: the plugin knows how to store versioned payload blocks
		// and run migrations. It never includes game types — game systems register
		// as providers (keeps the plugin reusable and the dependency arrow pointing
		// game -> plugin, per GDD 12.2 plugin plan).
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}
