// Unit tests for SPEC-P1-04 (GDD 14.4): adjacency legality and graph
// validation are pure logic — no fixtures needed.

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
			Definition.ConnectedRegionIds = Edges;
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
		Definitions[5].ConnectedRegionIds.Empty();
		Definitions[2].ConnectedRegionIds.Remove(TEXT("Isolated"));
		TestFalse(TEXT("Orphan node detected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("Orphan named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Orphan")); }));
	}

	// Asymmetric edge.
	{
		TArray<FEclipseRegionDefinition> Definitions = EclipseStrategyTest::MakeDefinitions();
		Definitions[0].ConnectedRegionIds.Add(TEXT("Depot")); // Depot does not list Underworks back
		TestFalse(TEXT("Asymmetric edge detected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("Asymmetry named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Asymmetric")); }));
	}

	// Duplicate id + unknown neighbor.
	{
		TArray<FEclipseRegionDefinition> Definitions = EclipseStrategyTest::MakeDefinitions();
		Definitions[1].RegionId = TEXT("Underworks");
		Definitions[3].ConnectedRegionIds.Add(TEXT("Ghost"));
		TestFalse(TEXT("Duplicate + unknown neighbor detected"), EclipseStrategyLogic::ValidateGraph(Definitions, Errors));
		TestTrue(TEXT("Duplicate named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("Duplicate")); }));
		TestTrue(TEXT("Unknown neighbor named"), Errors.ContainsByPredicate([](const FString& E) { return E.Contains(TEXT("unknown neighbor")); }));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
