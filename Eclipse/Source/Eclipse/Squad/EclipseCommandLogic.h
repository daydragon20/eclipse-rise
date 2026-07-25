#pragma once

#include "CoreMinimal.h"

/**
 * Pure Command Mode core (SPEC-P2-02, 14.5 step 2). Two responsibilities and
 * nothing else: the hold/dilation state machine and soldier-selection cycling.
 * Time dilation appears here only as a *decision* (what the world rate should
 * be) — applying it is the wrapper component's job (locked decision 5), and
 * the order/targeting logic never branches on it. No engine-actor headers
 * (GDD 14.3.2).
 */
namespace EclipseCommandLogic
{
	/** Everything that can end or start the held mode (SPEC-P2-02 fail-safe list). */
	enum class EEclipseCommandSignal : uint8
	{
		HoldPressed,
		HoldReleased,
		PawnDowned,
		MissionEnded,
		LevelTravel
	};

	/** Held-mode state; plain data so tests can drive it exhaustively. */
	struct ECLIPSE_API FEclipseCommandModeState
	{
		bool bHeld = false;

		/** Wall-clock enter stamp (locked decision 2: feel telemetry runs on real seconds). */
		double EnteredWallSeconds = 0.0;

		int32 OrdersIssuedWhileHeld = 0;

		/** Invalid guid = no selection = orders go to everyone (locked decision 4). */
		FGuid SelectedSoldier;
	};

	/**
	 * Transition on a signal. Every signal except HoldPressed ends the hold —
	 * the fail-safe invariant "mode ends => dilation 1.0, always" reduces to
	 * this single switch, which the tests sweep. Selection survives across
	 * holds (re-entering keeps your soldier); the wall stamp and order count
	 * reset per hold.
	 */
	ECLIPSE_API FEclipseCommandModeState ApplySignal(const FEclipseCommandModeState& State, EEclipseCommandSignal Signal, double NowWallSeconds);

	/** The world rate this state wants: DilationFactor while held, exactly 1.0 otherwise. */
	ECLIPSE_API float DesiredDilation(const FEclipseCommandModeState& State, float DilationFactor);

	/**
	 * Usage-pull tally for R3 criterion 5 ("an unused mode is a failed mode"):
	 * how often the tester enters Command Mode per encounter beat. Fed by the
	 * existing ModeEntered path — no new counter in the widget, no second source
	 * of truth (the component owns the state, the HUD consumes it).
	 *
	 * Beat segmentation, honest about being a proxy: a beat closes on an explicit
	 * tester mark, on the alarm sub-phase (the site going loud starts a new
	 * encounter), or when an entry arrives after a long idle gap. A fight the
	 * tester never opened the mode in leaves no fact behind and is therefore
	 * invisible to the automatic count — which is exactly why criterion 5 keeps
	 * its manual mark: marking an empty beat records the 0 that fails the bar.
	 */
	struct ECLIPSE_API FEclipseEncounterBeatTally
	{
		int32 EntriesThisBeat = 0;

		/** Entries and beats already closed; the average is judged over these only. */
		int32 ClosedBeatEntries = 0;
		int32 ClosedBeatCount = 0;

		/** Wall-clock stamp of the last entry (idle-gap segmentation runs on real seconds, like every feel measurement here). */
		double LastEntryWallSeconds = 0.0;

		/** An entry happened: closes the previous beat first when the idle gap says the fight was over. */
		void NoteModeEntered(double NowWallSeconds, double IdleGapSeconds);

		/** Close the running beat. bCountEmptyBeat records a beat with zero entries — true only for an explicit tester mark, which asserts the beat happened. */
		void BeginNextBeat(bool bCountEmptyBeat);

		void Reset();

		/** Entries per closed beat; 0 while nothing has closed yet (the criterion reads that as "not measured"). */
		float GetAverageEntriesPerBeat() const;
	};

	/**
	 * Cycle selection over the alive roster (locked decision 4): no selection
	 * steps onto the first (next) or last (prev) soldier; stepping wraps; an id
	 * that is no longer in the list (died mid-hold) restarts from the edge.
	 * Empty list = no selection. The list is the caller's alive-and-deployed
	 * set, in stable roster order.
	 */
	ECLIPSE_API FGuid CycleSelection(const TArray<FGuid>& AliveSoldiers, const FGuid& Current, int32 Direction);
}
