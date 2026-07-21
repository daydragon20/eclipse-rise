using UnrealBuildTool;

public class EclipseEditorTarget : TargetRules
{
	public EclipseEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		// V7 = UE 5.8 defaults; required to match the installed engine's shared build
		// environment (V5's relaxed warning levels are rejected for editor targets).
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		// Single-player only (GDD 12.1): no networking. UE 5.8 removed the target-level
		// bUseIris switch; Iris is now opt-in per module via SetupIrisSupport, which no
		// Eclipse module calls — the cost-avoidance intent stands.

		ExtraModuleNames.Add("Eclipse");
		ExtraModuleNames.Add("EclipseEditor");
	}
}
