#pragma once

#include "CoreMinimal.h"
#include "Quests/EclipseMissionTypes.h"
#include "Strategy/EclipseCampaignTransaction.h"

/**
 * Mission phase + consequence rules as pure logic (GDD 14.3.2). The seam where
 * hybrid games rot (SPEC-P1-05: "it must be airtight before anything is
 * pretty") is exactly this file: nothing here touches actors, the bus or the
 * campaign — it computes, the subsystem commits.
 *
 * // PLACEHOLDER(GDD 12.1): phase sequencing moves into a StateTree asset when
 * // the graybox level is authored; the transition *rules* stay here either way.
 */

/** Rewards resolved from the picked offer (SPEC-P1-04 row). */
struct FEclipseMissionRewards
{
	int32 Credits = 0;
	int32 Materials = 0;
	int32 Intel = 0;
};

/** Casualty resolution input (policy decided at debrief — SPEC-P1-07 wires the rules). */
struct FEclipseResolvedCasualty
{
	FGuid SoldierId;
	FString SoldierName;
	FName Cause;
	bool bDead = false;
	int32 DaysOut = 0;
};

namespace EclipseMissionLogic
{
	/**
	 * Canonical PhaseName of the alarm sub-phase (SPEC-P2-04): the fact "alarm"
	 * travels as Event.Mission.PhaseChanged(PhaseName=Alarm, bAuthoredSubPhase=
	 * true) — one constant so emitter, consumers and tests can never drift.
	 */
	inline const FName AlarmSubPhaseName = FName(TEXT("Alarm"));

	/** Legal phase progression of the universal loop (GDD 11.1); Debrief is terminal-adjacent. */
	ECLIPSE_API bool CanAdvancePhase(EEclipseMissionPhase From, EEclipseMissionPhase To);

	/** Canonical payload name of an outer loop phase (PhaseChanged emission); None/Finished map to NAME_None — they are runtime rest states, not broadcast phases. */
	ECLIPSE_API FName GetPhaseName(EEclipseMissionPhase Phase);

	/** All mandatory objectives complete? (Optionals never gate success — GDD 11.4.) */
	ECLIPSE_API bool AreMandatoryObjectivesComplete(const TArray<FEclipseObjectiveDef>& Objectives, const TArray<FName>& CompletedIds);

	/**
	 * Split the run's completed optionals into paid vs voided (SPEC-P2-04).
	 * Latch semantics: both bools mean "did it EVER happen this run" — a
	 * stabilized soldier still went down, so zero-casualty stays lost (M1.4's
	 * strictest reading), and a silenced alarm stays raised. Voided optionals
	 * surface as "missed" in the debrief; the wallet never sees them. One
	 * function feeds both the payout (ComposeConsequences) and the outcome's
	 * MissedOptionalObjectiveIds so UI and wallet cannot disagree.
	 */
	ECLIPSE_API void EvaluateOptionalObjectives(
		const TArray<FEclipseObjectiveDef>& Objectives,
		const TArray<FName>& CompletedIds,
		bool bAlarmRaised,
		bool bAnySoldierDowned,
		TArray<FEclipseObjectiveDef>& OutPaid,
		TArray<FName>& OutMissedIds);

	/**
	 * Compose the debrief transaction (GDD 14.3.3: ground gameplay proposes,
	 * the transaction commits — this is the proposal). Fail-forward (GDD 11.4):
	 * failure still commits casualties and partial intel; it never rolls back.
	 * Success progresses the region one step (Dominion->Contested->Player).
	 * A valid CompletionBeatTag rides the same atomic commit on success
	 * (SPEC-P2-04 decision 12) — never on loss, and skipped when the flag is
	 * already set, because SetStoryFlag rejects duplicates atomically and a
	 * rejected debrief would drop the whole consequence set.
	 *
	 * Optional stretch payouts (SPEC-P2-04): completed optionals that survive
	 * EvaluateOptionalObjectives compose AdjustResource mutations (Reason
	 * "OptionalObjective") into the SAME atomic transaction — success only,
	 * like the base reward: fail-forward salvages intel, it never pays
	 * stretch bonuses. The run's latches ride the Outcome parameter
	 * (bAlarmRaised + DownedSoldierIds, which retains stabilized soldiers).
	 */
	ECLIPSE_API FEclipseCampaignTransaction ComposeConsequences(
		const FEclipseMissionOutcome& Outcome,
		const FEclipseMissionRewards& Rewards,
		const TArray<FEclipseResolvedCasualty>& Casualties,
		const FEclipseCampaignState& State,
		const FGameplayTag& CreditsTag,
		const FGameplayTag& MaterialsTag,
		const FGameplayTag& IntelTag,
		bool bProgressRegionOnSuccess,
		const FGameplayTag& CompletionBeatTag = FGameplayTag(),
		const TArray<FEclipseObjectiveDef>& Objectives = TArray<FEclipseObjectiveDef>());
}
