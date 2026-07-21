// Squad-AI scenario suite, headless tier (GDD 14.4: runs per merge; a silent
// order failure is a release blocker BY TEST). The decision table is exercised
// exhaustively — no input combination may map to silence; the in-world variant
// of these scenarios attaches to the graybox map in the PIE pass.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Squad/EclipseSquadOrderLogic.h"

namespace EclipseSquadTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadOrderDecisionTest,
	"Eclipse.Squad.Orders.DecisionTableNeverSilent",
	EclipseSquadTest::TestFlags)

bool FEclipseSquadOrderDecisionTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	// Scenario: move order with a route -> acknowledged.
	{
		FEclipseOrderWorldFacts Facts;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::MoveTo, Facts);
		TestTrue(TEXT("Move with route accepted"), Decision.bAccepted);
	}

	// Scenario: blocked route -> refusal with NoRoute, never timeout-silence (SPEC-P1-06).
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bHasPathToTarget = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::MoveTo, Facts);
		TestFalse(TEXT("Blocked move refused"), Decision.bAccepted);
		TestTrue(TEXT("Refusal carries NoRoute"), Decision.Reason == EEclipseOrderRefusalReason::NoRoute);
	}

	// Scenario: focus on dead/invalid target -> refusal (SPEC-P1-06 scenario list).
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bTargetValid = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::FocusTarget, Facts);
		TestFalse(TEXT("Invalid target refused"), Decision.bAccepted);
		TestTrue(TEXT("Refusal carries InvalidTarget"), Decision.Reason == EEclipseOrderRefusalReason::InvalidTarget);
	}

	// Scenario: focus without line of sight -> "Can't see it".
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bTargetVisible = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::FocusTarget, Facts);
		TestFalse(TEXT("Unseen target refused"), Decision.bAccepted);
		TestTrue(TEXT("Refusal carries NoLineOfSight"), Decision.Reason == EEclipseOrderRefusalReason::NoLineOfSight);
	}

	// Scenario: downed soldier refuses everything with the same reason.
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bSoldierConscious = false;
		for (const EEclipseSquadOrder Order : { EEclipseSquadOrder::MoveTo, EEclipseSquadOrder::FocusTarget, EEclipseSquadOrder::Hold, EEclipseSquadOrder::Regroup })
		{
			const FEclipseOrderDecision Decision = DecideOrder(Order, Facts);
			TestFalse(TEXT("Downed soldier refuses"), Decision.bAccepted);
			TestTrue(TEXT("Refusal carries Downed"), Decision.Reason == EEclipseOrderRefusalReason::Downed);
		}
	}

	// Exhaustive zero-silence sweep: every order x every fact combination yields
	// either acceptance or a named reason (the GDD 8.4 release-blocker contract).
	for (int32 OrderIndex = 0; OrderIndex < 4; ++OrderIndex)
	{
		for (int32 Bits = 0; Bits < 16; ++Bits)
		{
			FEclipseOrderWorldFacts Facts;
			Facts.bSoldierConscious = (Bits & 1) != 0;
			Facts.bHasPathToTarget = (Bits & 2) != 0;
			Facts.bTargetValid = (Bits & 4) != 0;
			Facts.bTargetVisible = (Bits & 8) != 0;

			const FEclipseOrderDecision Decision = DecideOrder(static_cast<EEclipseSquadOrder>(OrderIndex), Facts);
			TestTrue(TEXT("Accepted or named reason — never silence"),
				Decision.bAccepted || Decision.Reason != EEclipseOrderRefusalReason::None);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadBarkDeterminismTest,
	"Eclipse.Squad.Orders.BarkSelection",
	EclipseSquadTest::TestFlags)

bool FEclipseSquadBarkDeterminismTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	const TArray<FString> Pool = { TEXT("On it."), TEXT("Moving."), TEXT("Copy that.") };
	const FGuid SoldierA(1, 2, 3, 4);
	const FGuid SoldierB(4, 3, 2, 1);

	TestEqual(TEXT("Same soldier, same salt, same voice"), PickBarkLine(Pool, SoldierA, 7), PickBarkLine(Pool, SoldierA, 7));
	TestTrue(TEXT("Line comes from the pool"), Pool.Contains(PickBarkLine(Pool, SoldierB, 7)));
	TestEqual(TEXT("Empty pool still answers (silence forbidden)"), PickBarkLine({}, SoldierA, 7), FString(TEXT("Copy.")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
