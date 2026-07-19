using UnrealBuildTool;

public class EclipseEditorTarget : TargetRules
{
	public EclipseEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		// Single-player only: Iris replication is deliberately disabled (GDD 12.1, cost avoidance).
		bUseIris = false;

		ExtraModuleNames.Add("Eclipse");
		ExtraModuleNames.Add("EclipseEditor");
	}
}
