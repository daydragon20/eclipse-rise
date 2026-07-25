#pragma once

#include "CoreMinimal.h"
#include "Strategy/EclipseCampaignTransaction.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "Strategy/EclipseLiberationTypes.h"

/**
 * Pure liberation core (SPEC-P2-05, 14.5 step 2; headless per GDD 14.3.2 — no
 * engine actor headers, deterministic, unit-tested). The spec's claim in code:
 * the as-built strategic machinery already carries a liberation — authored
 * completion -> one CampaignState transaction of existing SetRegionOwner
 * mutations -> the board answers. These functions only decide and compose;
 * they never log and never touch the bus — data damage travels through
 * out-params and the wiring layer (14.5 step 3) commits, broadcasts and warns
 * (the EclipseStrategyLogic house pattern).
 */
namespace EclipseLiberationLogic
{
	/** Audit source every liberation transaction carries (locked decision 2: `Source = "LiberationInstance"`). */
	inline const FName LiberationTransactionSource(TEXT("LiberationInstance"));

	/**
	 * True when the completed mission is this row's trigger: id match (the
	 * stable Event.Mission.Completed payload id, SPEC-P2-04 seam) AND the
	 * optional beat gate is satisfied against the committed StoryFlags in
	 * campaign state. Empty RequiredBeatTag = ungated (14.3.5); a row with no
	 * TriggerMissionId never triggers — damaged data is not a wildcard.
	 */
	ECLIPSE_API bool IsLiberationTriggered(const FEclipseCampaignState& State, const FEclipseLiberationRow& Row, FName CompletedMissionId);

	/**
	 * Composes the liberation as ONE transaction of SetRegionOwner mutations
	 * (Source = LiberationTransactionSource), monotone and state-derived
	 * (locked decision 3 — no stored marker):
	 * - regions already at NewOwner are excluded (a no-op SetRegionOwner is
	 *   rejected by ValidateMutation and the atomic commit would reject the
	 *   whole transaction with it);
	 * - unknown region ids are dropped and reported via OutDroppedRegionIds
	 *   (14.3.5: a typo must not reject the whole liberation), as is a
	 *   duplicate id — its second flip would be a no-op mid-transaction;
	 * - mutation order = row order (= commit order = event order,
	 *   deterministic for tests and the map).
	 * An empty Mutations array means nothing to commit: no transaction, no
	 * events — the caller logs its one line (idempotent re-entry, player
	 * pre-flips absorbed; the resolution never un-flips, never double-pays).
	 *
	 * Step-3 wiring obligations this pure API cannot cover itself (the spec
	 * matrix rows live there, MANDATORY in the wiring tests): missing
	 * table/no-matching-row = no flip + one logged warning, never a throw;
	 * never commit an empty transaction (CommitTransaction rejects it); warn
	 * once when OutDroppedRegionIds is non-empty.
	 */
	ECLIPSE_API FEclipseCampaignTransaction ResolveLiberationTransaction(const FEclipseCampaignState& State, const FEclipseLiberationRow& Row, TArray<FName>& OutDroppedRegionIds);

	/**
	 * The "trio is done" fold for consumers and the debug report (locked
	 * decision 4 — no phase event; completeness is derived, not stored): true
	 * iff every region the row names is at NewOwner. Unknown ids are
	 * excluded exactly as resolution drops them (14.3.5 — damaged data
	 * degrades, it never wedges the fold); a row that names no existing region
	 * is never complete.
	 */
	ECLIPSE_API bool IsLiberationComplete(const FEclipseCampaignState& State, const FEclipseLiberationRow& Row);
}
