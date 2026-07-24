#pragma once

#include "CoreMinimal.h"

ECLIPSESAVESYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogEclipseSave, Log, All);

namespace EclipseSave
{
	/** File magic ("ECLS"), so a corrupt/foreign file fails fast and loudly. */
	inline constexpr uint32 FileMagic = 0x45434C53;

	/**
	 * Bump on any breaking change to the container or a provider payload —
	 * and add a migration + test in the same commit (GDD 14.3.6).
	 * v2: Campaign block gained UnlockedLoadoutTags (SPEC-P1-03).
	 * v3: Campaign block gained the roster ClassId tail (SPEC-P2-01).
	 * v4: Campaign block gained the base facility tail (SPEC-P2-03).
	 * v5: Campaign block gained the story-flag tail (SPEC-P2-04).
	 */
	inline constexpr int32 CurrentSchemaVersion = 5;

	inline const FString AutosaveSlotName = TEXT("Autosave");
	inline const FString ManualSlotName = TEXT("Manual");
}
