#pragma once

#include "CoreMinimal.h"
#include "Squad/EclipseSquadTypes.h"

/**
 * Order decisions as a pure table (GDD 14.3.2). Controllers gather the world
 * facts (path exists? target visible/valid? soldier standing?); this decides
 * accept-or-refuse-with-reason. The scenario suite (GDD 14.4 squad bar) tests
 * this table exhaustively headless — the "zero silent failures" contract lives
 * here, not in tick timing.
 */
namespace EclipseSquadOrderLogic
{
	struct FEclipseOrderWorldFacts
	{
		bool bSoldierConscious = true;
		bool bHasPathToTarget = true;
		bool bTargetValid = true;
		bool bTargetVisible = true;
	};

	struct FEclipseOrderDecision
	{
		bool bAccepted = false;
		EEclipseOrderRefusalReason Reason = EEclipseOrderRefusalReason::None;
	};

	/** Every order gets exactly one answer; no input combination maps to silence. */
	ECLIPSE_API FEclipseOrderDecision DecideOrder(EEclipseSquadOrder Order, const FEclipseOrderWorldFacts& Facts);

	/** Deterministic bark pick (soldier id + order salt) so a given soldier has a stable voice in tests and replays. */
	ECLIPSE_API FString PickBarkLine(const TArray<FString>& Pool, const FGuid& SoldierId, uint32 Salt);

	/**
	 * Per-class push modulation (SPEC-P2-01, GDD 9.5: "aggressive soldiers push
	 * further"): extends the ordered point PushDistanceCm past itself along the
	 * soldier->order direction. Zero push or a degenerate direction returns the
	 * ordered point unchanged — obeying the letter of the order stays default.
	 */
	ECLIPSE_API FVector ComputePushedOrderPoint(const FVector& SoldierLocation, const FVector& OrderedLocation, float PushDistanceCm);

	/**
	 * One ring sample's cover score (the SPEC-P1-06 scorer, factored pure so the
	 * class bias is testable headless). Blocked line-of-fire dominates; ties go
	 * to the sample nearest the order; LaneBias > 0 (Sniper) prefers covered
	 * samples with a longer clear lane to the threat.
	 */
	ECLIPSE_API float ScoreCoverSample(bool bBlocksThreatLine, float DistanceToOrderCm, float DistanceToThreatCm, float LaneBias);
}
