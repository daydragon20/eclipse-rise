using UnrealBuildTool;

public class EclipseTarget : TargetRules
{
	public EclipseTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		// Single-player only: Iris replication is deliberately disabled (GDD 12.1, cost avoidance).
		bUseIris = false;

		ExtraModuleNames.Add("Eclipse");
	}
}
