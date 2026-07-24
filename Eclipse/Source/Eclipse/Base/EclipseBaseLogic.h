#pragma once

#include "CoreMinimal.h"
#include "Base/EclipseBaseTypes.h"
#include "Containers/ArrayView.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "Templates/Function.h"

/**
 * Hollow Point construction math as pure logic (GDD 14.3.2, SPEC-P2-03 step 2).
 * No engine actor headers: the subsystem wrapper (step 3) loads the layout
 * asset, resolves DT_Facilities rows and commits wallet deltas through the
 * CampaignState transaction API (GDD 14.3.3); this file only validates and
 * mutates FEclipseBaseState - headless-testable forever.
 *
 * Missing data (null row, unknown slot, absent level entry) is always a
 * validation rejection or a skipped yield, never a crash (GDD 14.3.5).
 */
namespace EclipseBaseLogic
{
	/** DA_BaseTuning as plain data (the wrapper copies the asset's fields; tests build it directly). */
	struct FEclipseBaseTuningParams
	{
		int32 RushCostCreditsPerDay = 60;
		int32 CrewDayReduction = 1;
		int32 AnalystIntelBonusPerDay = 1;
		int32 MaxCrewPerSite = 1;
		FGameplayTag AnalystBonusResource;
	};

	/** Caller-supplied DT_Facilities lookup; may return null for unknown ids (graceful skip, GDD 14.3.5). */
	using FEclipseFacilityRowResolver = TFunctionRef<const FEclipseFacilityRow*(FName)>;

	/** One completed build step - the wrapper turns these into Event.Base.FacilityBuilt/Upgraded in the same commit. */
	struct FEclipseFacilityCompletion
	{
		FName SlotId;
		FName FacilityId;
		/** The level that just became operational (1 = Built, >= 2 = Upgraded). */
		int32 NewLevel = 0;
	};

	/**
	 * Facility yields entering the existing economy day-tick, parallel to region
	 * yields (SPEC-P2-03: one deterministic tick keeps the 12.3 replayability
	 * contract).
	 */
	struct FEclipseFacilityYieldParams
	{
		TMap<FGameplayTag, int32> YieldPerDay;
	};

	/** Level data for a 1-based level; null when the row is null or the level has no entry (e.g. Barracks L2 in the slice). */
	ECLIPSE_API const FEclipseFacilityLevelData* GetLevelData(const FEclipseFacilityRow* Row, int32 Level);

	ECLIPSE_API const FEclipseBaseSlotDef* FindSlotDef(TConstArrayView<FEclipseBaseSlotDef> Slots, FName SlotId);

	/**
	 * Full build-order validation on the slot-graph (SPEC-P2-03): slot known,
	 * facility allowed at the slot (authored placement, locked decision 2), slot
	 * empty (or a legal upgrade of the same facility), level data present, and
	 * funds sufficient. The funds check is a pre-check for UX; the ledger's own
	 * insufficient-funds rejection (P1-03) stays the hard gate at commit time.
	 * On success OutTargetLevel is the level to build (1 = new, >= 2 = upgrade).
	 */
	ECLIPSE_API bool ValidateBuildOrder(const FEclipseBaseState& BaseState, TConstArrayView<FEclipseBaseSlotDef> Slots,
		FName SlotId, FName FacilityId, const FEclipseFacilityRow* FacilityRow,
		int32 AvailableMaterials, int32 AvailableCredits, int32& OutTargetLevel, FString& OutError);

	/**
	 * Begin construction at a validated slot (call ValidateBuildOrder first).
	 * DaysRemaining = max(1, BuildDays): even a zero-day data row takes one
	 * strategic day - construction is never instant except via rush (5.4).
	 */
	ECLIPSE_API FEclipseFacilityState& StartConstruction(FEclipseBaseState& BaseState, FName SlotId, FName FacilityId, const FEclipseFacilityLevelData& LevelData);

	/**
	 * One strategic day of construction (consumes Event.Campaign.DayAdvanced in
	 * the wrapper): every site decrements 1; a crewed site finishes when its
	 * remainder is within CrewDayReduction ("the crew works the finishing days"),
	 * which yields exactly the spec's flat "crew -1 build day, min 1" on a
	 * continuously crewed build. Completion bumps Level, zeroes DaysRemaining
	 * and releases the crew (their job is done; analyst duty is a new order).
	 */
	ECLIPSE_API TArray<FEclipseFacilityCompletion> TickConstructionDay(FEclipseBaseState& BaseState, const FEclipseBaseTuningParams& Tuning);

	/** Rush price right now: RushCostCreditsPerDay x DaysRemaining (0 when nothing is under construction). */
	ECLIPSE_API int32 ComputeRushCost(const FEclipseFacilityState* Facility, const FEclipseBaseTuningParams& Tuning);

	/** Rush is legal iff the slot is under construction and credits cover the price (5.4: available, never comfortable). */
	ECLIPSE_API bool ValidateRush(const FEclipseFacilityState* Facility, const FEclipseBaseTuningParams& Tuning, int32 AvailableCredits, FString& OutError);

	/**
	 * Instant completion (call ValidateRush first). The wrapper commits the
	 * credit spend and emits Event.Base.FacilityBuilt in that same commit
	 * (SPEC-P2-03 clock rules).
	 */
	ECLIPSE_API FEclipseFacilityCompletion ApplyRush(FEclipseFacilityState& Facility);

	/**
	 * Daily facility output (operational levels only; sites still building their
	 * first level yield nothing, upgrades keep the current level's yield).
	 * Staffed analysts add AnalystIntelBonusPerDay per soldier (capped at
	 * MaxCrewPerSite) to AnalystBonusResource - only at operational facilities
	 * whose own level data already yields that resource, so the bonus is the
	 * IC-analyst rule in data form, not an IC branch (SPEC-P2-03 staffing v1).
	 * Unknown facility rows are skipped (the wrapper logs; GDD 14.3.5).
	 */
	ECLIPSE_API FEclipseFacilityYieldParams ComputeFacilityYields(const FEclipseBaseState& BaseState, const FEclipseBaseTuningParams& Tuning, FEclipseFacilityRowResolver FindFacilityRow);
}
