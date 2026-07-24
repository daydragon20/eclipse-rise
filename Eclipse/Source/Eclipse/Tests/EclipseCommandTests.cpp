// Command Mode headless tier (SPEC-P2-02 Stage A). The fail-safe invariant —
// "mode ends => dilation exactly 1.0, always" — is swept here as pure state
// transitions (the Gauntlet 20x enter/exit runs the same ladder in-world); the
// selection cycle is exercised over its full edge set. Dilation never reaches
// the order logic, so these tests never need a world.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Squad/EclipseCommandLogic.h"
#include "Squad/EclipseCommandModeTuning.h"

namespace EclipseCommandTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCommandDilationFailSafeTest,
	"Eclipse.Command.DilationFailSafeAlwaysRestores",
	EclipseCommandTest::TestFlags)

bool FEclipseCommandDilationFailSafeTest::RunTest(const FString& Parameters)
{
	using namespace EclipseCommandLogic;
	constexpr float Factor = 0.30f;

	// 20x rapid enter/exit (the SPEC-P2-02 Gauntlet number): after every exit
	// the desired rate is exactly 1.0 — not approximately, exactly.
	FEclipseCommandModeState State;
	for (int32 Cycle = 0; Cycle < 20; ++Cycle)
	{
		State = ApplySignal(State, EEclipseCommandSignal::HoldPressed, /*Now*/ Cycle);
		TestTrue(TEXT("Held after press"), State.bHeld);
		TestEqual(TEXT("Held rate is the data factor"), DesiredDilation(State, Factor), Factor);

		State = ApplySignal(State, EEclipseCommandSignal::HoldReleased, /*Now*/ Cycle + 0.5);
		TestFalse(TEXT("Idle after release"), State.bHeld);
		TestEqual(TEXT("Idle rate is exactly 1.0"), DesiredDilation(State, Factor), 1.0f);
	}

	// Every fail-safe signal ends the hold (locked decision 5's full list).
	for (const EEclipseCommandSignal Signal : { EEclipseCommandSignal::HoldReleased, EEclipseCommandSignal::PawnDowned, EEclipseCommandSignal::MissionEnded, EEclipseCommandSignal::LevelTravel })
	{
		FEclipseCommandModeState Held;
		Held = ApplySignal(Held, EEclipseCommandSignal::HoldPressed, 0.0);
		Held = ApplySignal(Held, Signal, 1.0);
		TestFalse(TEXT("Fail-safe signal ends the hold"), Held.bHeld);
		TestEqual(TEXT("Fail-safe restores exactly 1.0"), DesiredDilation(Held, Factor), 1.0f);
	}

	// Signals on an idle state are no-ops, and a second press mid-hold keeps
	// the original wall stamp + order count (no telemetry reset mid-hold).
	FEclipseCommandModeState Idle;
	Idle = ApplySignal(Idle, EEclipseCommandSignal::PawnDowned, 5.0);
	TestFalse(TEXT("Signal on idle stays idle"), Idle.bHeld);

	FEclipseCommandModeState Held;
	Held = ApplySignal(Held, EEclipseCommandSignal::HoldPressed, 10.0);
	Held.OrdersIssuedWhileHeld = 3;
	Held.SelectedSoldier = FGuid(7, 7, 7, 7);
	Held = ApplySignal(Held, EEclipseCommandSignal::HoldPressed, 20.0);
	TestEqual(TEXT("Re-press keeps the enter stamp"), Held.EnteredWallSeconds, 10.0);
	TestEqual(TEXT("Re-press keeps the order count"), Held.OrdersIssuedWhileHeld, 3);

	// A fresh hold resets the telemetry window; the selection survives across
	// holds (header contract: re-entering keeps your soldier).
	Held = ApplySignal(Held, EEclipseCommandSignal::HoldReleased, 21.0);
	Held = ApplySignal(Held, EEclipseCommandSignal::HoldPressed, 30.0);
	TestEqual(TEXT("New hold restamps"), Held.EnteredWallSeconds, 30.0);
	TestEqual(TEXT("New hold resets the order count"), Held.OrdersIssuedWhileHeld, 0);
	TestEqual(TEXT("Selection survives across holds"), Held.SelectedSoldier, FGuid(7, 7, 7, 7));

	// Tactician full pause is data (0.0) and flows through the same math.
	FEclipseCommandModeState Paused;
	Paused = ApplySignal(Paused, EEclipseCommandSignal::HoldPressed, 0.0);
	TestEqual(TEXT("Tactician factor 0 = pause"), EclipseCommandLogic::DesiredDilation(Paused, 0.0f), 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCommandSelectionCycleTest,
	"Eclipse.Command.SelectionCycleCoversRoster",
	EclipseCommandTest::TestFlags)

bool FEclipseCommandSelectionCycleTest::RunTest(const FString& Parameters)
{
	using namespace EclipseCommandLogic;

	const FGuid A(1, 0, 0, 1);
	const FGuid B(2, 0, 0, 2);
	const FGuid C(3, 0, 0, 3);
	const TArray<FGuid> Squad = { A, B, C };

	// Empty roster: no selection, in either direction.
	TestFalse(TEXT("Empty roster yields no selection"), CycleSelection({}, FGuid(), +1).IsValid());
	TestFalse(TEXT("Empty roster yields no selection (prev)"), CycleSelection({}, A, -1).IsValid());

	// No current selection: next enters at the front, prev at the back.
	TestEqual(TEXT("First next-step selects the first soldier"), CycleSelection(Squad, FGuid(), +1), A);
	TestEqual(TEXT("First prev-step selects the last soldier"), CycleSelection(Squad, FGuid(), -1), C);

	// Full wrap in both directions.
	TestEqual(TEXT("A -> B"), CycleSelection(Squad, A, +1), B);
	TestEqual(TEXT("B -> C"), CycleSelection(Squad, B, +1), C);
	TestEqual(TEXT("C wraps to A"), CycleSelection(Squad, C, +1), A);
	TestEqual(TEXT("A wraps back to C"), CycleSelection(Squad, A, -1), C);
	TestEqual(TEXT("C -> B (prev)"), CycleSelection(Squad, C, -1), B);

	// The selected soldier died mid-hold (not in the alive list any more):
	// cycling restarts from the edge instead of crashing or sticking.
	const FGuid Dead(9, 9, 9, 9);
	TestEqual(TEXT("Stale selection restarts at the front"), CycleSelection(Squad, Dead, +1), A);
	TestEqual(TEXT("Stale selection restarts at the back (prev)"), CycleSelection(Squad, Dead, -1), C);

	// One-soldier squad: cycling stays put, never invalidates.
	const TArray<FGuid> Solo = { B };
	TestEqual(TEXT("Solo next stays"), CycleSelection(Solo, B, +1), B);
	TestEqual(TEXT("Solo prev stays"), CycleSelection(Solo, B, -1), B);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCommandTuningDefaultsTest,
	"Eclipse.Command.TuningDefaultsMatchLockedSpec",
	EclipseCommandTest::TestFlags)

bool FEclipseCommandTuningDefaultsTest::RunTest(const FString& Parameters)
{
	// The code defaults are the 14.3.5 fallback when DA_CommandModeTuning is
	// missing — they must equal the SPEC-P2-02 locked numbers, or a missing
	// asset silently changes the feel the R3 verdict was rendered on.
	const UEclipseCommandModeTuningAsset* Defaults = GetDefault<UEclipseCommandModeTuningAsset>();
	TestEqual(TEXT("Dilation 0.30 (4.1.2)"), Defaults->DilationFactor, 0.30f);
	TestEqual(TEXT("Tactician 0.0 = pause, data-ready"), Defaults->TacticianDilationFactor, 0.0f);
	TestEqual(TEXT("Stage A enters on a hard cut"), Defaults->EnterBlendSeconds, 0.0f);
	TestEqual(TEXT("Stage A exits on a hard cut"), Defaults->ExitBlendSeconds, 0.0f);
	TestEqual(TEXT("Camera pullback 15% (4.1.1, applied Stage B)"), Defaults->CameraPullbackPercent, 15.0f);
	TestEqual(TEXT("Sync-strike cap 4 (Stage B data)"), Defaults->MaxSyncStrikeMarks, 4);
	TestEqual(TEXT("Pick range matches the aim-reach fallback"), Defaults->SoldierSelectMaxRangeCm, 10000.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
