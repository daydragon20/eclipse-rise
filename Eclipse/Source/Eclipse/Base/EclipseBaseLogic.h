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

	/** One completed build step - the commit turns these into Event.Base.FacilityBuilt/Upgraded in the same commit. */
	struct FEclipseFacilityCompletion
	{
		FName SlotId;
		FName FacilityId;
		/** The level that just became operational (1 = Built, >= 2 = Upgraded; 0 = nothing completed, e.g. the ApplyRush no-op guard). */
		int32 NewLevel = 0;
		/** Staff released by this completion (captured before the reset) - the commit emits StaffAssigned(RoleTag none) per id. */
		TArray<FGuid> ReleasedSoldierIds;
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
	 * The state-only subset of build-order validation (no slot-graph, no data
	 * table, no funds): slot not already under construction; an occupied slot is
	 * only legal as an upgrade of the same facility. This is what the mutation
	 * layer can check against FEclipseCampaignState alone (GDD 14.3.3); the
	 * wrapper's ValidateBuildOrder covers placement/data/funds on top.
	 * OutTargetLevel = the level this order would build (1 = new, >= 2 = upgrade).
	 */
	ECLIPSE_API bool ValidateStartConstructionState(const FEclipseBaseState& BaseState, FName SlotId, FName FacilityId, int32& OutTargetLevel, FString& OutError);

	/**
	 * Begin construction at a validated slot (call ValidateBuildOrder first).
	 * DaysRemaining = max(1, BuildDays): even a zero-day data row takes one
	 * strategic day - construction is never instant except via rush (5.4).
	 */
	ECLIPSE_API FEclipseFacilityState& StartConstruction(FEclipseBaseState& BaseState, FName SlotId, FName FacilityId, int32 BuildDays);
	ECLIPSE_API FEclipseFacilityState& StartConstruction(FEclipseBaseState& BaseState, FName SlotId, FName FacilityId, const FEclipseFacilityLevelData& LevelData);

	/**
	 * One strategic day of construction (runs inside the AdvanceDay commit):
	 * every site decrements 1; a crewed site finishes when its remainder is
	 * within CrewDayReduction ("the crew works the finishing days"), which
	 * yields exactly the spec's flat "crew -1 build day, min 1" on a
	 * continuously crewed build. Completion bumps Level, zeroes DaysRemaining
	 * and releases the crew (their job is done; analyst duty is a new order).
	 *
	 * Crew-timing rule (SPEC-P2-03 review, decided): a crew present on the
	 * finishing day earns the full -1 even if assigned last-minute - accepted
	 * as a data rule because the coarse 1-day clock makes "crew present on the
	 * day the site could finish" the entire contract, and the optimizer still
	 * pays the real price (one soldier-day of undeployability per day saved);
	 * accumulating crew-days would add hidden fractional state to the save
	 * schema (v5) to punish a trade that is already fair.
	 */
	ECLIPSE_API TArray<FEclipseFacilityCompletion> TickConstructionDay(FEclipseBaseState& BaseState, const FEclipseBaseTuningParams& Tuning);

	/** Rush price right now: RushCostCreditsPerDay x DaysRemaining (0 when nothing is under construction). */
	ECLIPSE_API int32 ComputeRushCost(const FEclipseFacilityState* Facility, const FEclipseBaseTuningParams& Tuning);

	/** Rush is legal iff the slot is under construction and credits cover the price (5.4: available, never comfortable). */
	ECLIPSE_API bool ValidateRush(const FEclipseFacilityState* Facility, const FEclipseBaseTuningParams& Tuning, int32 AvailableCredits, FString& OutError);

	/** The state-only rush precondition (something is actually under construction at that slot) - the mutation layer's check. */
	ECLIPSE_API bool ValidateRushState(const FEclipseBaseState& BaseState, FName SlotId, FString& OutError);

	/**
	 * Instant completion (call ValidateRush first). The credit spend and the
	 * FacilityBuilt/Upgraded emission happen in that same commit (SPEC-P2-03
	 * clock rules). Guarded: an operational site (DaysRemaining <= 0) is a
	 * no-op returning NewLevel = 0 - a double-apply can never double-bump.
	 */
	ECLIPSE_API FEclipseFacilityCompletion ApplyRush(FEclipseFacilityState& Facility);

	/**
	 * State-only staffing validation (SPEC-P2-03 staffing v1). Assign: the slot
	 * must hold a facility state (a building site takes a crew, an operational
	 * site an analyst - the role is positional) and the soldier may hold only
	 * one base job. Unassign: the soldier must actually be assigned there.
	 * Roster-level checks (soldier exists, is Available) and the tuning cap
	 * (MaxCrewPerSite) belong to the callers that hold that data; assignment
	 * beyond the cap stays harmless (EffectiveStaffCount clamps every effect).
	 */
	ECLIPSE_API bool ValidateStaffChange(const FEclipseBaseState& BaseState, FName SlotId, const FGuid& SoldierId, bool bAssign, FString& OutError);

	/** Apply a validated staff change; returns the touched facility (null only on unvalidated input). */
	ECLIPSE_API FEclipseFacilityState* ApplyStaffChange(FEclipseBaseState& BaseState, FName SlotId, const FGuid& SoldierId, bool bAssign);

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
