#pragma once

#include "CoreMinimal.h"
#include "Squad/EclipseSquadTypes.h"

/**
 * Order decisions as a pure table (GDD 14.3.2). Controllers gather the world
 * facts (path exists? target visible/valid? soldier standing?); this decides
 * accept-or-refuse-with-reason. The scenario suite (GDD 14.4 squad bar) tests
 * this table exhaustively headless — the "zero silent failures" contract lives
 * here, not in tick timing.
 *
 * Also home to the order round-trip meter: the answer bar of GDD 8.4 / the R3
 * gauntlet is a property of the order path, so it is measured and judged here
 * rather than in whatever UI happens to display it.
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

	/**
	 * The clock the round-trip meter runs on: FPlatformTime, i.e. the wall clock,
	 * NEVER the world clock. Command Mode dilates the world to 0.30 (SPEC-P2-02
	 * locked decision 2), so a game-time stamp would report a round trip 3.3x
	 * shorter than the tester actually waited — the one measurement error that
	 * could fabricate an R3 "true". One named seam so the test can assert the
	 * clock the subsystem really samples, with the world dilated.
	 */
	ECLIPSE_API double NowWallSeconds();

	/**
	 * Order round-trip tally (R3 criterion 1): per order the wall-clock seconds
	 * from issue to its ack/refusal, judged against the 1 s answer bar. Plain
	 * data driven by explicit stamps so the headless tier can sweep it — the
	 * subsystem feeds it, nobody else owns these numbers.
	 */
	struct ECLIPSE_API FEclipseOrderRoundTripStats
	{
		/** The R3 bar: every order answered within one second of real time. */
		static constexpr double BarSeconds = 1.0;

		int32 SampleCount = 0;

		/** Samples at or under BarSeconds; SampleCount - WithinBarCount = late answers. */
		int32 WithinBarCount = 0;

		double WorstSeconds = 0.0;
		double TotalSeconds = 0.0;

		/** Record one issue->answer round trip. Negative input is clamped to 0: a backwards clock is not a 0.0 s success story. */
		void NoteRoundTrip(double Seconds);

		void Reset();

		double GetAverageSeconds() const;
	};

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
	ECLIPSE_API float ScoreCoverSample(bool bBlocksThreatLine, float DistanceToOrderCm, float DistanceToThreatCm, float LaneBias,
		float CoverBlockBonus = 10.0f, float DistanceWeightPerCm = 0.001f);
}
