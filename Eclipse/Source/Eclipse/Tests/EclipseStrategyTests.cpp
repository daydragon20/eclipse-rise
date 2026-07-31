// Unit tests for SPEC-P1-04 (GDD 14.4): adjacency legality and graph
// validation are pure logic — no fixtures needed.
//
// Lane costs, lane status and routing live in EclipseLaneRoutingTests.cpp; this
// file stays the structural half (who borders whom, and does the data hold up).

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Strategy/EclipseStrategyLogic.h"

namespace EclipseStrategyTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/**
	 * The SPEC-P1-04 six-node board in miniature:
	 *   Underworks(P) — Checkpoint(D) — Factory(D)
	 *        |                             |
	 *   Housing(D) ————————————————— Depot(C)      + Isolated(D) far side via Factory
	 */
	TArray<FEclipseRegionDefinition> MakeDefinitions()
	{
		auto MakeRegion = [](FName Id, EEclipseRegionType Type, std::initializer_list<FName> Edges)
		{
			FEclipseRegionDefinition Definition;
			Definition.RegionId = Id;
			Definition.RegionType = Type;
			Definition.Lanes = EclipseRegionGraph::OpenLanes(TArray<FName>(Edges));
			return Definition;
		};

		return {
			MakeRegion(TEXT("Underworks"), EEclipseRegionType::Industrial, { TEXT("Checkpoint"), TEXT("Housing") }),
			MakeRegion(TEXT("Checkpoint"), EEclipseRegionType::Checkpoint, { TEXT("Underworks"), TEXT("Factory") }),
			MakeRegion(TEXT("Factory"), EEclipseRegionType::Industrial, { TEXT("Checkpoint"), TEXT("Depot"), TEXT("Isolated") }),
			MakeRegion(TEXT("Housing"), EEclipseRegionType::Residential, { TEXT("Underworks"), TEXT("Depot") }),
			MakeRegion(TEXT("Depot"), EEclipseRegionType::Industrial, { TEXT("Factory"), TEXT("Housing") }),
			MakeRegion(TEXT("Isolated"), EEclipseRegionType::Residential, { TEXT("Factory") }),
		};
	}

	FEclipseCampaignState MakeState()
	{
		FEclipseCampaignState State;
		auto AddRegion = [&State](FName Id, EEclipseRegionOwner Owner)
		{
			FEclipseRegionState& Region = State.Regions.AddDefaulted_GetRef();
			Region.RegionId = Id;
			Region.Owner = Owner;
		};
		AddRegion(TEXT("Underworks"), EEclipseRegionOwner::Player);
		AddRegion(TEXT("Checkpoint"), EEclipseRegionOwner::Dominion);
		AddRegion(TEXT("Factory"), EEclipseRegionOwner::Dominion);
		AddRegion(TEXT("Housing"), EEclipseRegionOwner::Contested);
		AddRegion(TEXT("Depot"), EEclipseRegionOwner::Dominion);
		AddRegion(TEXT("Isolated"), EEclipseRegionOwner::Dominion);
		return State;
	}

	/** The lane record on Definitions[Index] pointing at NeighborId (checked). */
	FEclipseLaneDefinition& LaneOf(TArray<FEclipseRegionDefinition>& Definitions, int32 Index, FName NeighborId)
	{
		FEclipseLaneDefinition* Lane = Definitions[Index].Lanes.FindByPredicate(
			[NeighborId](const FEclipseLaneDefinition& L) { return L.NeighborRegionId == NeighborId; });
		check(Lane != nullptr);
		return *Lane;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStrategyAdjacencyTest,
	"Eclipse.Strategy.MiniMap.AdjacencyLegality",
	EclipseStrategyTest::TestFlags)

bool FEclipseStrategyAdjacencyTest::RunTest(const FString& Parameters)
{
	const TArray<FEclipseRegionDefinition> Definitions = EclipseStrategyTest::MakeDefinitions();
	const FEclipseCampaignState State = EclipseStrategyTest::MakeState();
	FString Reason;

	TestTrue(TEXT("Adjacent Dominion region is a legal target"),
		EclipseStrategyLogic::IsMissionTargetLegal(State, Definitions, TEXT("Checkpoint"), Reason));
	TestTrue(TEXT("Adjacent contested region is a legal target"),
		EclipseStrategyLogic::IsMissionTargetLegal(State, Definitions, TEXT("Housing"), Reason));

	TestFalse(TEXT("Non-adjacent region rejected (GDD 3.1 rule 1 in miniature)"),
		EclipseStrategyLogic::IsMissionTargetLegal(State, Definitions, TEXT("Factory"), Reason));
	TestTrue(TEXT("Rejection explains the adjacency rule"), Reason.Contains(TEXT("adjacent")));

	TestFalse(TEXT("Player-held region is not a target"),
		EclipseStrategyLogic::IsMissionTargetLegal(State, Definitions, TEXT("Underworks"), Reason));
	TestFalse(TEXT("Unknown region rejected"),
		EclipseStrategyLogic::IsMissionTargetLegal(State, Definitions, TEXT("Nowhere"), Reason));

	const TArray<FName> Legal = EclipseStrategyLogic::GetLegalMissionTargets(State, Definitions);
	TestEqual(TEXT("Exactly the two border regions are legal"), Legal.Num(), 2);
	TestTrue(TEXT("Checkpoint listed"), Legal.Contains(FName(TEXT("Checkpoint"))));
	TestTrue(TEXT("Housing listed"), Legal.Contains(FName(TEXT("Housing"))));

	// Taking the checkpoint opens the factory — the tester's stateable reason
	// (SPEC-P1-04 DoD: "checkpoint first, it unlocks the factory node").
	FEclipseCampaignState Advanced = State;
	Advanced.Regions[1].Owner = EEclipseRegionOwner::Player;
	TestTrue(TEXT("Factory becomes legal after the checkpoint falls"),
		EclipseStrategyLogic::IsMissionTargetLegal(Advanced, Definitions, TEXT("Factory"), Reason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStrategyGraphValidationTest,
	"Eclipse.Strategy.MiniMap.GraphValidation",
	EclipseStrategyTest::TestFlags)

bool FEclipseStrategyGraphValidationTest::RunTest(const FString& Parameters)
{
	TArray<FString> Errors;
	TestTrue(TEXT("The reference board validates clean"),
		EclipseStrategyLogic::ValidateGraph(EclipseStrategyTest::MakeDefinitions(), Errors));

	// Orphan node.
	{
		TArray<FEclipseRegionDefinition> Definitions = EclipseStrategyTest::MakeDefinitions();
		Definitions[5].Lanes.Empty();
		Definitions[2].Lanes.RemoveAll([](const FEclipseLaneDefinition& L) { return L.NeighborRegionId == TEXT("Isolated"); });
		TestFalse(TEXT("Orphan node detected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("Orphan named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Orphan")); }));
	}

	// Asymmetric edge.
	{
		TArray<FEclipseRegionDefinition> Definitions = EclipseStrategyTest::MakeDefinitions();
		Definitions[0].Lanes.Add(FEclipseLaneDefinition(TEXT("Depot"))); // Depot does not list Underworks back
		TestFalse(TEXT("Asymmetric edge detected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("Asymmetry named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Asymmetric")); }));
	}

	// Duplicate id + unknown neighbor.
	{
		TArray<FEclipseRegionDefinition> Definitions = EclipseStrategyTest::MakeDefinitions();
		Definitions[1].RegionId = TEXT("Underworks");
		Definitions[3].Lanes.Add(FEclipseLaneDefinition(TEXT("Ghost")));
		TestFalse(TEXT("Duplicate + unknown neighbor detected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("Duplicate named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Duplicate")); }));
		TestTrue(TEXT("Unknown neighbor named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("unknown neighbor")); }));
	}

	return true;
}

/**
 * FALSIFICATION 2 — symmetry survives the schema change, and it now means more
 * than "the reverse edge exists".
 *
 * The control experiment runs first and runs on EVERY case: the same board with
 * the same mutation applied to BOTH ends validates green. Without that, a red
 * bar proves only that the validator dislikes something.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLaneSymmetryTest,
	"Eclipse.Strategy.Lanes.SymmetryIsEnforcedOnTheWholeRecord",
	EclipseStrategyTest::TestFlags)

bool FEclipseLaneSymmetryTest::RunTest(const FString& Parameters)
{
	using namespace EclipseStrategyTest;
	TArray<FString> Errors;

	// Control: the untouched board is green. Every red below is measured against this.
	TestTrue(TEXT("CONTROL — the symmetric reference board validates green"),
		EclipseStrategyLogic::ValidateGraph(MakeDefinitions(), Errors));
	TestEqual(TEXT("CONTROL — and it produces zero errors"), Errors.Num(), 0);

	/**
	 * One knob, applied to one end (must go red) and then to both ends (must go
	 * green again). Both halves matter: the first shows the validator sees it,
	 * the second shows it is asymmetry it objects to and not the value itself.
	 */
	auto CheckKnob = [this](const TCHAR* What, const TCHAR* ExpectedFragment,
		TFunctionRef<void(FEclipseLaneDefinition&)> Mutate)
	{
		TArray<FString> LocalErrors;

		TArray<FEclipseRegionDefinition> OneEnd = MakeDefinitions();
		Mutate(LaneOf(OneEnd, 0, TEXT("Checkpoint")));
		const bool bOneEndValid = EclipseStrategyLogic::ValidateGraph(OneEnd, LocalErrors);
		TestFalse(FString::Printf(TEXT("%s on one end alone is REJECTED"), What), bOneEndValid);
		TestTrue(FString::Printf(TEXT("%s: the error names the asymmetry"), What),
			LocalErrors.ContainsByPredicate([ExpectedFragment](const FString& E) { return E.Contains(ExpectedFragment); }));

		TArray<FEclipseRegionDefinition> BothEnds = MakeDefinitions();
		Mutate(LaneOf(BothEnds, 0, TEXT("Checkpoint")));
		Mutate(LaneOf(BothEnds, 1, TEXT("Underworks")));
		TestTrue(FString::Printf(TEXT("%s on BOTH ends is accepted — it is the asymmetry that is rejected"), What),
			EclipseStrategyLogic::ValidateGraph(BothEnds, LocalErrors));
	};

	CheckKnob(TEXT("Travel cost"), TEXT("Asymmetric lane cost"),
		[](FEclipseLaneDefinition& Lane) { Lane.TravelDays = 4; });
	CheckKnob(TEXT("Risk"), TEXT("Asymmetric lane cost"),
		[](FEclipseLaneDefinition& Lane) { Lane.Risk = 25; });
	CheckKnob(TEXT("Smuggler surcharge"), TEXT("Asymmetric lane cost"),
		[](FEclipseLaneDefinition& Lane) { Lane.SmugglerDelayDays = 3; });
	CheckKnob(TEXT("Status"), TEXT("Asymmetric lane status"),
		[](FEclipseLaneDefinition& Lane) { Lane.Status = EEclipseLaneStatus::SmugglerOnly; });

	// The gate has its own message, and its own both-ends control.
	{
		TArray<FEclipseRegionDefinition> OneEnd = MakeDefinitions();
		LaneOf(OneEnd, 0, TEXT("Checkpoint")).Status = EEclipseLaneStatus::SpireGated;
		LaneOf(OneEnd, 0, TEXT("Checkpoint")).GateRegionId = TEXT("Factory");
		LaneOf(OneEnd, 1, TEXT("Underworks")).Status = EEclipseLaneStatus::SpireGated;
		LaneOf(OneEnd, 1, TEXT("Underworks")).GateRegionId = TEXT("Depot"); // different gate
		TestFalse(TEXT("Same status, different gate is still asymmetric"),
			EclipseStrategyLogic::ValidateGraph(OneEnd, Errors));
		TestTrue(TEXT("The gate mismatch is named"),
			Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Asymmetric lane gate")); }));

		TArray<FEclipseRegionDefinition> BothEnds = MakeDefinitions();
		for (const TPair<int32, FName> End : { TPair<int32, FName>(0, FName(TEXT("Checkpoint"))), TPair<int32, FName>(1, FName(TEXT("Underworks"))) })
		{
			FEclipseLaneDefinition& Lane = LaneOf(BothEnds, End.Key, End.Value);
			Lane.Status = EEclipseLaneStatus::SpireGated;
			Lane.GateRegionId = TEXT("Factory");
		}
		TestTrue(TEXT("CONTROL — the same gate on both ends validates green"),
			EclipseStrategyLogic::ValidateGraph(BothEnds, Errors));
	}

	// A gate id is only legal on the status that reads it.
	{
		TArray<FEclipseRegionDefinition> Definitions = MakeDefinitions();
		LaneOf(Definitions, 0, TEXT("Checkpoint")).GateRegionId = TEXT("Factory");
		LaneOf(Definitions, 1, TEXT("Underworks")).GateRegionId = TEXT("Factory");
		TestFalse(TEXT("A gate on a non-gated lane is rejected"),
			EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("...and says so"),
			Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("is not SpireGated")); }));
	}

	// SpireGated without a gate, and with an unknown gate.
	{
		TArray<FEclipseRegionDefinition> Definitions = MakeDefinitions();
		LaneOf(Definitions, 0, TEXT("Checkpoint")).Status = EEclipseLaneStatus::SpireGated;
		LaneOf(Definitions, 1, TEXT("Underworks")).Status = EEclipseLaneStatus::SpireGated;
		TestFalse(TEXT("SpireGated with no gate named is rejected"),
			EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("...and says which lane"),
			Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("names no gate region")); }));

		LaneOf(Definitions, 0, TEXT("Checkpoint")).GateRegionId = TEXT("Nowhere");
		LaneOf(Definitions, 1, TEXT("Underworks")).GateRegionId = TEXT("Nowhere");
		TestFalse(TEXT("SpireGated on an unknown gate is rejected"),
			EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("...and names the missing gate"),
			Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("gated by unknown region")); }));
	}

	// Zero-cost and duplicate lanes.
	{
		TArray<FEclipseRegionDefinition> Definitions = MakeDefinitions();
		LaneOf(Definitions, 0, TEXT("Checkpoint")).TravelDays = 0;
		LaneOf(Definitions, 1, TEXT("Underworks")).TravelDays = 0;
		TestFalse(TEXT("A free lane is rejected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("...and explains why"),
			Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("not a lane")); }));
	}
	{
		TArray<FEclipseRegionDefinition> Definitions = MakeDefinitions();
		Definitions[0].Lanes.Add(FEclipseLaneDefinition(TEXT("Checkpoint")));
		TestFalse(TEXT("Two lanes to the same place are rejected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("...and says duplicate"),
			Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("duplicate lanes")); }));
	}

	return true;
}

/**
 * The pre-lane migration: content authored before lanes existed must come
 * forward as open unit-cost lanes, exactly once, without overwriting anything
 * that was authored deliberately.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLaneMigrationTest,
	"Eclipse.Strategy.Lanes.LegacyEdgesFoldForwardOnce",
	EclipseStrategyTest::TestFlags)

bool FEclipseLaneMigrationTest::RunTest(const FString& Parameters)
{
	TArray<FEclipseRegionDefinition> Definitions;
	{
		FEclipseRegionDefinition& A = Definitions.AddDefaulted_GetRef();
		A.RegionId = TEXT("A");
		A.ConnectedRegionIds_DEPRECATED = { TEXT("B") };

		FEclipseRegionDefinition& B = Definitions.AddDefaulted_GetRef();
		B.RegionId = TEXT("B");
		// B already has an AUTHORED lane back, with a real cost on it.
		FEclipseLaneDefinition& Authored = B.Lanes.AddDefaulted_GetRef();
		Authored.NeighborRegionId = TEXT("A");
		Authored.TravelDays = 3;
		Authored.Risk = 12;
		B.ConnectedRegionIds_DEPRECATED = { TEXT("A") };
	}

	// A graph that still carries legacy edges is invalid on purpose: half its
	// topology would be invisible to routing.
	TArray<FString> Errors;
	TestFalse(TEXT("Un-migrated legacy content is rejected by the validator"),
		EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
	TestTrue(TEXT("...and names the migration"),
		Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("pre-lane ConnectedRegionIds")); }));

	const int32 Created = EclipseRegionGraph::UpgradeLegacyLanes(Definitions);
	TestEqual(TEXT("Exactly one lane was created — B's authored lane was left alone"), Created, 1);
	TestEqual(TEXT("A has its lane"), Definitions[0].Lanes.Num(), 1);
	TestEqual(TEXT("New lanes are open"), Definitions[0].Lanes[0].Status, EEclipseLaneStatus::Open);
	TestEqual(TEXT("New lanes cost one day"), Definitions[0].Lanes[0].TravelDays, 1);
	TestEqual(TEXT("B still has exactly one lane"), Definitions[1].Lanes.Num(), 1);
	TestEqual(TEXT("B's AUTHORED cost survived the migration"), Definitions[1].Lanes[0].TravelDays, 3);
	TestEqual(TEXT("B's authored risk survived too"), Definitions[1].Lanes[0].Risk, 12);

	TestEqual(TEXT("The legacy array is emptied — never two sources of truth"),
		Definitions[0].ConnectedRegionIds_DEPRECATED.Num(), 0);
	TestEqual(TEXT("Re-running the migration is a no-op"),
		EclipseRegionGraph::UpgradeLegacyLanes(Definitions), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
