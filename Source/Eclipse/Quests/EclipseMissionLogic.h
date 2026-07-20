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
	/** Legal phase progression of the universal loop (GDD 11.1); Debrief is terminal-adjacent. */
	ECLIPSE_API bool CanAdvancePhase(EEclipseMissionPhase From, EEclipseMissionPhase To);

	/** All mandatory objectives complete? (Optionals never gate success — GDD 11.4.) */
	ECLIPSE_API bool AreMandatoryObjectivesComplete(const TArray<FEclipseObjectiveDef>& Objectives, const TArray<FName>& CompletedIds);

	/**
	 * Compose the debrief transaction (GDD 14.3.3: ground gameplay proposes,
	 * the transaction commits — this is the proposal). Fail-forward (GDD 11.4):
	 * failure still commits casualties and partial intel; it never rolls back.
	 * Success progresses the region one step (Dominion->Contested->Player).
	 */
	ECLIPSE_API FEclipseCampaignTransaction ComposeConsequences(
		const FEclipseMissionOutcome& Outcome,
		const FEclipseMissionRewards& Rewards,
		const TArray<FEclipseResolvedCasualty>& Casualties,
		const FEclipseCampaignState& State,
		const FGameplayTag& CreditsTag,
		const FGameplayTag& MaterialsTag,
		const FGameplayTag& IntelTag,
		bool bProgressRegionOnSuccess);
}
