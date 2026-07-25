#include "Base/EclipseBaseLogic.h"

namespace EclipseBaseLogic
{

namespace
{
	/** Staff that actually counts: assignment beyond the per-site cap has no effect (data caps, not code — GDD 14.2). */
	int32 EffectiveStaffCount(const FEclipseFacilityState& Facility, const FEclipseBaseTuningParams& Tuning)
	{
		return FMath::Min(Facility.AssignedSoldierIds.Num(), FMath::Max(0, Tuning.MaxCrewPerSite));
	}

	/** Shared completion mutation: rush and the day tick must agree on what "built" means. */
	FEclipseFacilityCompletion CompleteConstruction(FEclipseFacilityState& Facility)
	{
		Facility.Level += 1;
		Facility.DaysRemaining = 0;

		FEclipseFacilityCompletion Completion;
		Completion.SlotId = Facility.SlotId;
		Completion.FacilityId = Facility.FacilityId;
		Completion.NewLevel = Facility.Level;
		// Capture the released staff BEFORE the reset so the commit can emit
		// StaffAssigned(RoleTag none) per soldier (SPEC-P2-03 review R6/d).
		Completion.ReleasedSoldierIds = Facility.AssignedSoldierIds;

		// The crew's job is done; keeping them assigned would silently convert
		// construction crew into analysts (SPEC-P2-03 keeps that an explicit order).
		Facility.AssignedSoldierIds.Reset();
		return Completion;
	}
}

const FEclipseFacilityLevelData* GetLevelData(const FEclipseFacilityRow* Row, int32 Level)
{
	if (Row == nullptr || Level < 1 || !Row->Levels.IsValidIndex(Level - 1))
	{
		return nullptr; // missing data is an answer, never a crash (GDD 14.3.5)
	}
	return &Row->Levels[Level - 1];
}

const FEclipseBaseSlotDef* FindSlotDef(TConstArrayView<FEclipseBaseSlotDef> Slots, FName SlotId)
{
	for (const FEclipseBaseSlotDef& Slot : Slots)
	{
		if (Slot.SlotId == SlotId)
		{
			return &Slot;
		}
	}
	return nullptr;
}

bool ValidateBuildOrder(const FEclipseBaseState& BaseState, TConstArrayView<FEclipseBaseSlotDef> Slots,
	FName SlotId, FName FacilityId, const FEclipseFacilityRow* FacilityRow,
	int32 AvailableMaterials, int32 AvailableCredits, int32& OutTargetLevel, FString& OutError)
{
	OutTargetLevel = 0;

	const FEclipseBaseSlotDef* Slot = FindSlotDef(Slots, SlotId);
	if (Slot == nullptr)
	{
		OutError = FString::Printf(TEXT("Build: unknown slot '%s'"), *SlotId.ToString());
		return false;
	}

	if (FacilityId.IsNone() || FacilityRow == nullptr)
	{
		OutError = FString::Printf(TEXT("Build: unknown facility '%s' (no DT_Facilities row)"), *FacilityId.ToString());
		return false;
	}

	// Placement is authored (locked decision 2): the slot's data decides, never code.
	if (!Slot->AllowedFacilityRows.Contains(FacilityId))
	{
		OutError = FString::Printf(TEXT("Build: facility '%s' is not allowed at slot '%s'"), *FacilityId.ToString(), *SlotId.ToString());
		return false;
	}

	// State-only invariants + target level: one source of truth shared with the
	// mutation layer's validation (GDD 14.3.3 - the commit re-checks these).
	int32 TargetLevel = 1;
	if (!ValidateStartConstructionState(BaseState, SlotId, FacilityId, TargetLevel, OutError))
	{
		return false;
	}

	const FEclipseFacilityLevelData* LevelData = GetLevelData(FacilityRow, TargetLevel);
	if (LevelData == nullptr)
	{
		// Max level is the Levels array length: "only the Workshop has an L2" is
		// slice data, not a code rule (SPEC-P2-03 cost table).
		OutError = FString::Printf(TEXT("Build: '%s' has no level %d data (max level reached?)"), *FacilityId.ToString(), TargetLevel);
		return false;
	}

	// Affordability pre-check; the ledger's atomic insufficient-funds rejection
	// (P1-03) remains the hard gate when the wrapper commits the spend.
	if (AvailableMaterials < LevelData->CostMaterials)
	{
		OutError = FString::Printf(TEXT("Build: insufficient materials (%d < %d)"), AvailableMaterials, LevelData->CostMaterials);
		return false;
	}
	if (AvailableCredits < LevelData->CostCredits)
	{
		OutError = FString::Printf(TEXT("Build: insufficient credits (%d < %d)"), AvailableCredits, LevelData->CostCredits);
		return false;
	}

	OutTargetLevel = TargetLevel;
	return true;
}

bool ValidateStartConstructionState(const FEclipseBaseState& BaseState, FName SlotId, FName FacilityId, int32& OutTargetLevel, FString& OutError)
{
	OutTargetLevel = 1;
	if (SlotId.IsNone() || FacilityId.IsNone())
	{
		OutError = TEXT("Build: slot and facility ids must be set");
		return false;
	}

	if (const FEclipseFacilityState* Existing = BaseState.FindBySlot(SlotId))
	{
		if (Existing->DaysRemaining > 0)
		{
			OutError = FString::Printf(TEXT("Build: slot '%s' is already under construction"), *SlotId.ToString());
			return false;
		}
		if (Existing->Level > 0)
		{
			if (Existing->FacilityId != FacilityId)
			{
				OutError = FString::Printf(TEXT("Build: slot '%s' is occupied by '%s'"), *SlotId.ToString(), *Existing->FacilityId.ToString());
				return false;
			}
			OutTargetLevel = Existing->Level + 1; // same facility, operational: this is an upgrade order
		}
	}
	return true;
}

FEclipseFacilityState& StartConstruction(FEclipseBaseState& BaseState, FName SlotId, FName FacilityId, int32 BuildDays)
{
	FEclipseFacilityState* Facility = BaseState.FindBySlot(SlotId);
	if (Facility == nullptr)
	{
		Facility = &BaseState.Facilities.AddDefaulted_GetRef();
		Facility->SlotId = SlotId;
	}
	Facility->FacilityId = FacilityId;
	// Construction is never free time-wise: even a 0-day data row costs one
	// strategic day. Rush is the only instant path (5.4 money vs. time).
	Facility->DaysRemaining = FMath::Max(1, BuildDays);
	return *Facility;
}

FEclipseFacilityState& StartConstruction(FEclipseBaseState& BaseState, FName SlotId, FName FacilityId, const FEclipseFacilityLevelData& LevelData)
{
	return StartConstruction(BaseState, SlotId, FacilityId, LevelData.BuildDays);
}

TArray<FEclipseFacilityCompletion> TickConstructionDay(FEclipseBaseState& BaseState, const FEclipseBaseTuningParams& Tuning)
{
	TArray<FEclipseFacilityCompletion> Completions;
	for (FEclipseFacilityState& Facility : BaseState.Facilities)
	{
		if (Facility.DaysRemaining <= 0)
		{
			continue; // operational or empty: nothing to tick
		}

		Facility.DaysRemaining -= 1;

		// A crewed site finishes when the remainder is within CrewDayReduction:
		// on a continuously crewed build that is exactly the spec's flat
		// "crew -1 build day", and the min-1 floor holds because completion
		// always costs at least this one tick.
		const bool bCrewed = EffectiveStaffCount(Facility, Tuning) > 0;
		const int32 CrewShave = bCrewed ? FMath::Max(0, Tuning.CrewDayReduction) : 0;
		if (Facility.DaysRemaining - CrewShave <= 0)
		{
			Completions.Add(CompleteConstruction(Facility));
		}
	}
	return Completions;
}

int32 ComputeRushCost(const FEclipseFacilityState* Facility, const FEclipseBaseTuningParams& Tuning)
{
	if (Facility == nullptr || Facility->DaysRemaining <= 0)
	{
		return 0;
	}
	return Tuning.RushCostCreditsPerDay * Facility->DaysRemaining;
}

bool ValidateRush(const FEclipseFacilityState* Facility, const FEclipseBaseTuningParams& Tuning, int32 AvailableCredits, FString& OutError)
{
	if (Facility == nullptr || Facility->DaysRemaining <= 0)
	{
		OutError = TEXT("Rush: nothing under construction at that slot");
		return false;
	}

	const int32 Cost = ComputeRushCost(Facility, Tuning);
	if (AvailableCredits < Cost)
	{
		OutError = FString::Printf(TEXT("Rush: insufficient credits (%d < %d)"), AvailableCredits, Cost);
		return false;
	}
	return true;
}

bool ValidateRushState(const FEclipseBaseState& BaseState, FName SlotId, FString& OutError)
{
	const FEclipseFacilityState* Facility = BaseState.FindBySlot(SlotId);
	if (Facility == nullptr || Facility->DaysRemaining <= 0)
	{
		OutError = FString::Printf(TEXT("Rush: nothing under construction at slot '%s'"), *SlotId.ToString());
		return false;
	}
	return true;
}

FEclipseFacilityCompletion ApplyRush(FEclipseFacilityState& Facility)
{
	if (Facility.DaysRemaining <= 0)
	{
		// Guard (SPEC-P2-03 review R6/e): rushing an operational site is a no-op
		// - a double-apply must never double-bump the level. NewLevel = 0 tells
		// the caller nothing completed.
		FEclipseFacilityCompletion NoOp;
		NoOp.SlotId = Facility.SlotId;
		NoOp.FacilityId = Facility.FacilityId;
		return NoOp;
	}
	return CompleteConstruction(Facility);
}

bool ValidateStaffChange(const FEclipseBaseState& BaseState, FName SlotId, const FGuid& SoldierId, bool bAssign, FString& OutError)
{
	if (!SoldierId.IsValid())
	{
		OutError = TEXT("Staff: soldier id is not set");
		return false;
	}

	const FEclipseFacilityState* Facility = BaseState.FindBySlot(SlotId);
	if (Facility == nullptr)
	{
		// An empty, never-built slot has no facility state to staff; the crew
		// arrives with the construction order, not before.
		OutError = FString::Printf(TEXT("Staff: no facility at slot '%s'"), *SlotId.ToString());
		return false;
	}

	if (bAssign)
	{
		// One person, one job (SPEC-P2-03 staffing v1: assignment makes the
		// soldier undeployable - a double booking would double-spend them).
		if (const FEclipseFacilityState* Other = FindAssignment(BaseState, SoldierId))
		{
			OutError = FString::Printf(TEXT("Staff: soldier %s is already assigned at slot '%s'"), *SoldierId.ToString(), *Other->SlotId.ToString());
			return false;
		}
	}
	else if (!Facility->AssignedSoldierIds.Contains(SoldierId))
	{
		OutError = FString::Printf(TEXT("Staff: soldier %s is not assigned at slot '%s'"), *SoldierId.ToString(), *SlotId.ToString());
		return false;
	}
	return true;
}

FEclipseFacilityState* ApplyStaffChange(FEclipseBaseState& BaseState, FName SlotId, const FGuid& SoldierId, bool bAssign)
{
	FEclipseFacilityState* Facility = BaseState.FindBySlot(SlotId);
	if (Facility == nullptr)
	{
		return nullptr;
	}
	if (bAssign)
	{
		Facility->AssignedSoldierIds.AddUnique(SoldierId);
	}
	else
	{
		Facility->AssignedSoldierIds.Remove(SoldierId);
	}
	return Facility;
}

const FEclipseFacilityState* FindAssignment(const FEclipseBaseState& BaseState, const FGuid& SoldierId)
{
	if (!SoldierId.IsValid())
	{
		return nullptr;
	}
	return BaseState.Facilities.FindByPredicate(
		[&SoldierId](const FEclipseFacilityState& Facility) { return Facility.AssignedSoldierIds.Contains(SoldierId); });
}

TArray<FEclipseStaffRelease> ReleaseSoldierAssignments(FEclipseBaseState& BaseState, const FGuid& SoldierId)
{
	TArray<FEclipseStaffRelease> Releases;
	if (!SoldierId.IsValid())
	{
		return Releases;
	}
	for (FEclipseFacilityState& Facility : BaseState.Facilities)
	{
		if (Facility.AssignedSoldierIds.Remove(SoldierId) > 0)
		{
			FEclipseStaffRelease& Release = Releases.AddDefaulted_GetRef();
			Release.SlotId = Facility.SlotId;
			Release.FacilityId = Facility.FacilityId;
		}
	}
	return Releases;
}

FEclipseFacilityYieldParams ComputeFacilityYields(const FEclipseBaseState& BaseState, const FEclipseBaseTuningParams& Tuning, FEclipseFacilityRowResolver FindFacilityRow)
{
	FEclipseFacilityYieldParams Params;
	for (const FEclipseFacilityState& Facility : BaseState.Facilities)
	{
		if (Facility.Level < 1)
		{
			continue; // first build still in progress: no yield yet
		}

		// Upgrades keep producing at the current level; a missing row or level
		// entry is a silent skip here (the wrapper logs it once - GDD 14.3.5).
		const FEclipseFacilityLevelData* LevelData = GetLevelData(FindFacilityRow(Facility.FacilityId), Facility.Level);
		if (LevelData == nullptr)
		{
			continue;
		}

		for (const TPair<FGameplayTag, int32>& Yield : LevelData->YieldPerDay)
		{
			if (Yield.Key.IsValid() && Yield.Value != 0)
			{
				Params.YieldPerDay.FindOrAdd(Yield.Key) += Yield.Value;
			}
		}

		// Analyst bonus: staff at an OPERATIONAL site (staff on a building site
		// is the construction crew), only where the facility's own data already
		// yields the bonus resource - the IC rule in data form.
		if (Facility.DaysRemaining == 0 && Tuning.AnalystBonusResource.IsValid() && Tuning.AnalystIntelBonusPerDay > 0
			&& LevelData->YieldPerDay.Contains(Tuning.AnalystBonusResource))
		{
			const int32 Analysts = EffectiveStaffCount(Facility, Tuning);
			if (Analysts > 0)
			{
				Params.YieldPerDay.FindOrAdd(Tuning.AnalystBonusResource) += Analysts * Tuning.AnalystIntelBonusPerDay;
			}
		}
	}
	return Params;
}

} // namespace EclipseBaseLogic
