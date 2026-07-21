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
}
