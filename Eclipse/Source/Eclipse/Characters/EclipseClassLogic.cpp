#include "Characters/EclipseClassLogic.h"

namespace EclipseClassLogic
{

FEclipseResolvedClassKit ResolveClassKit(const FEclipseSoldierRecord& Record, const FEclipseClassDefRow* ClassRow)
{
	FEclipseResolvedClassKit Kit;
	Kit.ClassId = Record.ClassId;

	if (ClassRow == nullptr)
	{
		// Classless fallback (GDD 14.3.5): the soldier keeps the shared body,
		// the platform-default weapon and no signature verb. The unresolved id
		// stays on the kit so UI/logs can name what the data was missing.
		return Kit;
	}

	Kit.DisplayName = ClassRow->DisplayName;
	Kit.WeaponRow = ClassRow->WeaponRow;
	Kit.BodyDefOverride = ClassRow->BodyDefOverride;
	Kit.SignatureVerb = ClassRow->SignatureVerb;
	Kit.StabilizeWindowSeconds = FMath::Max(0.0f, ClassRow->StabilizeWindowSeconds);
	Kit.KillzoneRangeCm = FMath::Max(0.0f, ClassRow->KillzoneRangeCm);
	Kit.BarkSet = ClassRow->BarkSet;
	Kit.OrderPushDistanceCm = FMath::Max(0.0f, ClassRow->OrderPushDistanceCm);
	Kit.CoverLaneBias = FMath::Max(0.0f, ClassRow->CoverLaneBias);
	Kit.bAutoTriage = ClassRow->bAutoTriage;
	return Kit;
}

bool IsStabilizeWindowOpen(double DownedAtSeconds, double NowSeconds, float WindowSeconds)
{
	if (WindowSeconds <= 0.0f)
	{
		return false; // data says this soldier cannot stabilize — no class branch anywhere
	}
	// Inclusive edge: a save at exactly Window seconds counts (the drama beat).
	// Clock skew (Now < DownedAt) reads as elapsed 0 rather than rejecting a
	// legitimate save over floating-point noise.
	const double Elapsed = FMath::Max(0.0, NowSeconds - DownedAtSeconds);
	return Elapsed <= static_cast<double>(WindowSeconds);
}

} // namespace EclipseClassLogic
