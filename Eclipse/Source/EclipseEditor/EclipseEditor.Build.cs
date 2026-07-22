using UnrealBuildTool;

public class EclipseEditor : ModuleRules
{
	public EclipseEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Eclipse"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetRegistry",
			"GameplayTags",
			"AssetTools", // wav -> USoundWave import tasks (EclipseGenerateVoices commandlet)
			"HTTP"        // manual HttpManager pump — commandlets have no game loop to tick it (16.12)
		});
	}
}
