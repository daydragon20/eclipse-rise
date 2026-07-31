// What a lane COSTS and whether it is OPEN (GDD 3.1 rules 2 and 4), plus the
// Dominion Response Tier's effect on both (GDD 9.4). Pure logic — no fixtures,
// no world, no widgets.
//
// The three falsifications this file exists to answer are named in the test
// names. Each one is written so that a version of the code where lane status
// does nothing would go RED, which is the only property that makes a test worth
// the disk it sits on.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Strategy/EclipseStrategyLogic.h"

namespace EclipseLaneTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	using EclipseStrategyLogic::EEclipseTransitMode;
	using EclipseStrategyLogic::FEclipseRoute;
	using EclipseStrategyLogic::FEclipseRouteQuery;

	/**
	 * GDD 3.1's Spire Beta, scaled to a district. Everything below is authored
	 * so that ONE ownership flip on SpireBeta changes the answer to three
	 * different questions: which path, how long, and whether Target is supplied.
	 *
	 *            SpireBeta                     (the gate node itself)
	 *              |1d
	 *   Home(P) ——1d—— Relay ==gated by SpireBeta, 1d== Target
	 *      |2d                                            |2d
	 *   Waypoint ——2d—— Depot ————————————————————————————
	 *      |1d
	 *   Quarry ——2d—— Relay          (equal days, wildly different risk)
	 *
	 *   Home ~~1d, SMUGGLER-ONLY~~ Vault      (never military, in any state)
	 *   Marooned ——1d—— Hermit                (its own island: no route at all)
	 */
	TArray<FEclipseRegionDefinition> MakeBoard()
	{
		TArray<FEclipseRegionDefinition> Board;

		auto AddRegion = [&Board](FName Id)
		{
			FEclipseRegionDefinition& Definition = Board.AddDefaulted_GetRef();
			Definition.RegionId = Id;
			return &Definition;
		};
		for (const TCHAR* Id : { TEXT("Home"), TEXT("Relay"), TEXT("SpireBeta"), TEXT("Waypoint"),
			TEXT("Depot"), TEXT("Target"), TEXT("Quarry"), TEXT("Vault"), TEXT("Marooned"), TEXT("Hermit") })
		{
			AddRegion(Id);
		}

		auto Find = [&Board](FName Id) -> FEclipseRegionDefinition&
		{
			FEclipseRegionDefinition* Found = Board.FindByPredicate(
				[Id](const FEclipseRegionDefinition& D) { return D.RegionId == Id; });
			check(Found != nullptr);
			return *Found;
		};

		// Lanes are authored ONCE and mirrored here, so every fixture in this
		// file is symmetric by construction — asymmetry is EclipseStrategyTests'
		// subject, not this file's, and a fixture that is quietly invalid would
		// make every result below meaningless.
		auto Link = [&Find](FName A, FName B, int32 Days, int32 Risk,
			EEclipseLaneStatus Status = EEclipseLaneStatus::Open, FName Gate = NAME_None,
			int32 SmugglerDelay = 1, int32 SmugglerRisk = 10)
		{
			auto Half = [&](FName From, FName To)
			{
				FEclipseLaneDefinition& Lane = Find(From).Lanes.AddDefaulted_GetRef();
				Lane.NeighborRegionId = To;
				Lane.TravelDays = Days;
				Lane.Risk = Risk;
				Lane.Status = Status;
				Lane.GateRegionId = Gate;
				Lane.SmugglerDelayDays = SmugglerDelay;
				Lane.SmugglerRiskPenalty = SmugglerRisk;
			};
			Half(A, B);
			Half(B, A);
		};

		Link(TEXT("Home"), TEXT("SpireBeta"), 1, 2);
		Link(TEXT("Home"), TEXT("Relay"), 1, 3);
		Link(TEXT("Relay"), TEXT("Target"), 1, 4, EEclipseLaneStatus::SpireGated, TEXT("SpireBeta"), 2, 20);
		Link(TEXT("Home"), TEXT("Waypoint"), 2, 1);
		Link(TEXT("Waypoint"), TEXT("Depot"), 2, 1);
		Link(TEXT("Depot"), TEXT("Target"), 2, 1);
		Link(TEXT("Waypoint"), TEXT("Quarry"), 1, 1);
		Link(TEXT("Relay"), TEXT("Quarry"), 2, 40);
		Link(TEXT("Home"), TEXT("Vault"), 1, 5, EEclipseLaneStatus::SmugglerOnly, NAME_None, 1, 15);
		Link(TEXT("Marooned"), TEXT("Hermit"), 1, 0);

		return Board;
	}

	/** Everything Dominion-held except Home. SpireBeta starts hostile — the shut gate. */
	FEclipseCampaignState MakeState(const TArray<FEclipseRegionDefinition>& Board)
	{
		FEclipseCampaignState State;
		for (const FEclipseRegionDefinition& Definition : Board)
		{
			FEclipseRegionState& Region = State.Regions.AddDefaulted_GetRef();
			Region.RegionId = Definition.RegionId;
			Region.Owner = Definition.RegionId == FName(TEXT("Home"))
				? EEclipseRegionOwner::Player
				: EEclipseRegionOwner::Dominion;
		}
		return State;
	}

	void SetOwner(FEclipseCampaignState& State, FName RegionId, EEclipseRegionOwner Owner)
	{
		FEclipseRegionState* Region = State.Regions.FindByPredicate(
			[RegionId](const FEclipseRegionState& R) { return R.RegionId == RegionId; });
		check(Region != nullptr);
		Region->Owner = Owner;
	}

	FEclipseRoute Route(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Board,
		FName From, FName To, EEclipseTransitMode Mode = EEclipseTransitMode::Military,
		int32 MaxRisk = MAX_int32)
	{
		FEclipseRouteQuery Query;
		Query.StartRegionId = From;
		Query.GoalRegionId = To;
		Query.Mode = Mode;
		Query.MaxAcceptableRisk = MaxRisk;
		return EclipseStrategyLogic::FindRoute(State, Board, Query);
	}

	FString PathToString(const FEclipseRoute& InRoute)
	{
		TArray<FString> Parts;
		for (const FName& Id : InRoute.RegionPath)
		{
			Parts.Add(Id.ToString());
		}
		return FString::Join(Parts, TEXT(" -> "));
	}
}

/**
 * FALSIFICATION 1 — a hostile Spire demonstrably changes the path.
 *
 * Same start, same goal, one ownership flip. If lane status did nothing, both
 * halves of this test would produce the identical route and the equality
 * assert at the end would fail.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLaneSpireChangesTheRouteTest,
	"Eclipse.Strategy.Lanes.AHostileSpireChangesTheMilitaryRoute",
	EclipseLaneTest::TestFlags)

bool FEclipseLaneSpireChangesTheRouteTest::RunTest(const FString& Parameters)
{
	using namespace EclipseLaneTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();

	// The board itself must be valid, or nothing measured on it means anything.
	TArray<FString> GraphErrors;
	if (!TestTrue(TEXT("PRECONDITION: the routing board is a valid graph"),
		EclipseStrategyLogic::ValidateGraph(Board, GraphErrors)))
	{
		for (const FString& GraphError : GraphErrors)
		{
			AddError(GraphError);
		}
		return false;
	}

	// --- Spire Beta hostile: the short road through Relay is shut to columns.
	FEclipseCampaignState Shut = MakeState(Board);
	const FEclipseRoute Detour = Route(Shut, Board, TEXT("Home"), TEXT("Target"));

	TestTrue(TEXT("A military route still exists — the board is not cut, only gated"), Detour.bValid);
	AddInfo(FString::Printf(TEXT("GEMETEN  spire hostile: %s (%d dagen, risico %d)"),
		*PathToString(Detour), Detour.TotalTravelDays, Detour.TotalRisk));
	TestEqual(TEXT("It takes the long way round"), PathToString(Detour), FString(TEXT("Home -> Waypoint -> Depot -> Target")));
	TestEqual(TEXT("Six days the long way"), Detour.TotalTravelDays, 6);
	TestFalse(TEXT("And it is a military route, not a smuggler run"), Detour.bUsesSmugglerLeg);

	// --- One flip. Nothing else about the board or the query changes.
	FEclipseCampaignState Open = MakeState(Board);
	SetOwner(Open, TEXT("SpireBeta"), EEclipseRegionOwner::Player);
	const FEclipseRoute Direct = Route(Open, Board, TEXT("Home"), TEXT("Target"));

	TestTrue(TEXT("With the spire taken, the military route is valid"), Direct.bValid);
	AddInfo(FString::Printf(TEXT("GEMETEN  spire taken:   %s (%d dagen, risico %d)"),
		*PathToString(Direct), Direct.TotalTravelDays, Direct.TotalRisk));
	TestEqual(TEXT("It now goes straight through Relay"), PathToString(Direct), FString(TEXT("Home -> Relay -> Target")));
	TestEqual(TEXT("Two days the short way"), Direct.TotalTravelDays, 2);

	// The load-bearing assert: the two answers are not the same answer.
	TestNotEqual(TEXT("THE POINT: one spire flip yields a different path"), PathToString(Direct), PathToString(Detour));
	TestTrue(TEXT("...and a shorter one"), Direct.TotalTravelDays < Detour.TotalTravelDays);

	// A contested gate is the third reading of the same lane: columns pass, but
	// through a firefight — GDD 6.3.2's trichotomy applied to movement.
	FEclipseCampaignState Contested = MakeState(Board);
	SetOwner(Contested, TEXT("SpireBeta"), EEclipseRegionOwner::Contested);
	const FEclipseRoute Gauntlet = Route(Contested, Board, TEXT("Home"), TEXT("Target"));
	TestTrue(TEXT("A contested gate lets columns through"), Gauntlet.bValid);
	TestEqual(TEXT("...on the same short road"), PathToString(Gauntlet), FString(TEXT("Home -> Relay -> Target")));
	TestTrue(TEXT("...but it costs more risk than a friendly gate"), Gauntlet.TotalRisk > Direct.TotalRisk);
	TestEqual(TEXT("...exactly the contested-gate penalty"),
		Gauntlet.TotalRisk - Direct.TotalRisk, FEclipseLaneTuning().ContestedGateRiskPenalty);

	// And the same flip moves supply, which is GDD 3.1 rule 4 in one assert:
	// six days out is beyond the supply horizon, two days is not.
	const FEclipseLaneTuning Tuning;
	TestFalse(TEXT("Behind a shut gate, Target is beyond supply"),
		EclipseStrategyLogic::IsRegionSupplied(Shut, Board, TEXT("Target"), Tuning));
	TestTrue(TEXT("With the gate open, Target is supplied"),
		EclipseStrategyLogic::IsRegionSupplied(Open, Board, TEXT("Target"), Tuning));

	return true;
}

/**
 * FALSIFICATION 3 — smuggler-passable is a THIRD outcome, measurably unlike
 * both "open" and "shut".
 *
 * Three goals, one query shape, three different answers. If the code collapsed
 * smuggler routes into either of the other two, two of these three blocks would
 * fail.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLaneSmugglerIsAThirdOutcomeTest,
	"Eclipse.Strategy.Lanes.SmugglerRouteIsAThirdOutcomeNotASecond",
	EclipseLaneTest::TestFlags)

bool FEclipseLaneSmugglerIsAThirdOutcomeTest::RunTest(const FString& Parameters)
{
	using namespace EclipseLaneTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();
	const FEclipseCampaignState State = MakeState(Board); // SpireBeta hostile

	// --- OUTCOME A: open. A military route exists and no smuggler was needed.
	const FEclipseRoute OpenMilitary = Route(State, Board, TEXT("Home"), TEXT("Waypoint"));
	TestTrue(TEXT("A: open lane — military route exists"), OpenMilitary.bValid);
	TestFalse(TEXT("A: and it used no smuggler leg"), OpenMilitary.bUsesSmugglerLeg);

	// --- OUTCOME B: shut to columns, open to smugglers. Vault hangs off a
	// SmugglerOnly lane, so this is unconditional — no gate to take.
	const FEclipseRoute VaultMilitary = Route(State, Board, TEXT("Home"), TEXT("Vault"));
	const FEclipseRoute VaultSmuggler = Route(State, Board, TEXT("Home"), TEXT("Vault"), EEclipseTransitMode::Smuggler);
	TestFalse(TEXT("B: no military route to the Vault"), VaultMilitary.bValid);
	TestTrue(TEXT("B: but smugglers get there"), VaultSmuggler.bValid);
	TestTrue(TEXT("B: and the route knows it was a smuggler run"), VaultSmuggler.bUsesSmugglerLeg);

	// --- OUTCOME C: shut to everyone. Hermit is on its own island.
	const FEclipseRoute HermitMilitary = Route(State, Board, TEXT("Home"), TEXT("Hermit"));
	const FEclipseRoute HermitSmuggler = Route(State, Board, TEXT("Home"), TEXT("Hermit"), EEclipseTransitMode::Smuggler);
	TestFalse(TEXT("C: no military route to Hermit"), HermitMilitary.bValid);
	TestFalse(TEXT("C: and no smuggler route either — this is what 'shut' means"), HermitSmuggler.bValid);

	// The separation, stated as the property that matters: B and C are both
	// "military: no", and they are NOT the same state of the world.
	TestNotEqual(TEXT("THE POINT: B and C differ in smuggler reachability"),
		VaultSmuggler.bValid, HermitSmuggler.bValid);
	TestNotEqual(TEXT("...and the two refusals do not read the same"),
		VaultMilitary.FailureReason, HermitMilitary.FailureReason);
	AddInfo(FString::Printf(TEXT("GEMETEN  B weigert met: %s"), *VaultMilitary.FailureReason));
	AddInfo(FString::Printf(TEXT("GEMETEN  C weigert met: %s"), *HermitMilitary.FailureReason));

	// Smuggling is not free: "at cost and risk" is two numbers, not a mood.
	const FEclipseLaneDefinition* VaultLane = Board.FindByPredicate(
		[](const FEclipseRegionDefinition& D) { return D.RegionId == FName(TEXT("Home")); })->FindLane(TEXT("Vault"));
	if (TestNotNull(TEXT("Vault lane exists"), VaultLane))
	{
		TestEqual(TEXT("The smuggler run costs the lane's delay on top of its days"),
			VaultSmuggler.TotalTravelDays, VaultLane->TravelDays + VaultLane->SmugglerDelayDays);
		TestEqual(TEXT("...and the lane's risk penalty on top of its risk"),
			VaultSmuggler.TotalRisk, VaultLane->Risk + VaultLane->SmugglerRiskPenalty);
	}

	/**
	 * SpireGated vs SmugglerOnly: conditional against unconditional. This is
	 * the assert that stops the two from being one status with two names.
	 */
	FEclipseCampaignState EverythingTaken = MakeState(Board);
	for (FEclipseRegionState& Region : EverythingTaken.Regions)
	{
		Region.Owner = EEclipseRegionOwner::Player;
	}
	TestTrue(TEXT("SpireGated is CONDITIONAL: take the gate and columns pass"),
		Route(EverythingTaken, Board, TEXT("Relay"), TEXT("Target")).bValid);
	TestFalse(TEXT("SmugglerOnly is UNCONDITIONAL: owning the whole board changes nothing"),
		Route(EverythingTaken, Board, TEXT("Home"), TEXT("Vault")).bValid);
	TestTrue(TEXT("...and smugglers still run it, exactly as before"),
		Route(EverythingTaken, Board, TEXT("Home"), TEXT("Vault"), EEclipseTransitMode::Smuggler).bValid);

	/**
	 * The gated LANE taken as a smuggler run while the gate is hostile: GDD 3.1
	 * rule 2 as an assert.
	 *
	 * Deliberately measured on the lane and not on a Relay->Target ROUTE. A
	 * route would be green for the wrong reason: this board has a detour
	 * (Relay -> Home -> Waypoint -> Depot -> Target), so "a military route
	 * exists" says nothing about whether the gated lane is shut. That mistake
	 * cost this test one red bar, and the fix is to ask the question the claim
	 * is actually about.
	 */
	const FEclipseRegionDefinition* RelayDefinition = Board.FindByPredicate(
		[](const FEclipseRegionDefinition& D) { return D.RegionId == FName(TEXT("Relay")); });
	const FEclipseLaneDefinition* GatedLane = RelayDefinition != nullptr ? RelayDefinition->FindLane(TEXT("Target")) : nullptr;
	if (TestNotNull(TEXT("The gated lane exists"), GatedLane))
	{
		const FEclipseLaneTuning Tuning;
		const EclipseStrategyLogic::FEclipseLaneTransit AsColumn =
			EclipseStrategyLogic::ResolveLaneTransit(State, *GatedLane, EEclipseTransitMode::Military, Tuning);
		const EclipseStrategyLogic::FEclipseLaneTransit AsSmuggler =
			EclipseStrategyLogic::ResolveLaneTransit(State, *GatedLane, EEclipseTransitMode::Smuggler, Tuning);

		TestFalse(TEXT("A hostile gate shuts the lane to columns"), AsColumn.bPassable);
		TestTrue(TEXT("...and names the Spire doing it"), AsColumn.BlockedReason.Contains(TEXT("SpireBeta")));
		TestTrue(TEXT("Smugglers still run that same lane"), AsSmuggler.bPassable);
		TestTrue(TEXT("...and pay for it"), AsSmuggler.bSmugglerLeg);
		TestTrue(TEXT("...in days"), AsSmuggler.TravelDays > GatedLane->TravelDays);
		TestTrue(TEXT("...and in risk"), AsSmuggler.Risk > GatedLane->Risk);
	}

	// And the route-level consequence of that shut lane: the military answer to
	// Relay -> Target is the DETOUR, not the one-day hop next door.
	const FEclipseRoute RelayMilitary = Route(State, Board, TEXT("Relay"), TEXT("Target"));
	TestTrue(TEXT("A military route to Target still exists — round the back"), RelayMilitary.bValid);
	TestTrue(TEXT("...and it does not use the gated lane"), RelayMilitary.Steps.Num() > 1);
	const FEclipseRoute RelaySmuggler = Route(State, Board, TEXT("Relay"), TEXT("Target"), EEclipseTransitMode::Smuggler);
	TestTrue(TEXT("Smugglers take the one hop"), RelaySmuggler.bValid);
	TestEqual(TEXT("...in a single leg"), RelaySmuggler.Steps.Num(), 1);
	TestTrue(TEXT("THE POINT: same start, same goal, the two modes take different roads"),
		RelaySmuggler.Steps.Num() != RelayMilitary.Steps.Num());

	// Mission legality follows the same rule: a lane nobody can cross is not a
	// border, but a smuggler lane still is — and the approach says which.
	FString Reason;
	TestTrue(TEXT("The Vault is a legal target — smuggling is movement too"),
		EclipseStrategyLogic::IsMissionTargetLegal(State, Board, TEXT("Vault"), Reason));
	EclipseStrategyLogic::EEclipseTransitMode Approach = EEclipseTransitMode::Military;
	TestTrue(TEXT("...and it classifies"),
		EclipseStrategyLogic::ClassifyMissionApproach(State, Board, TEXT("Vault"), FEclipseLaneTuning(), Approach));
	TestEqual(TEXT("...as a smuggler approach, not a military one"),
		static_cast<int32>(Approach), static_cast<int32>(EEclipseTransitMode::Smuggler));

	TestTrue(TEXT("CONTROL: an open border classifies as military"),
		EclipseStrategyLogic::ClassifyMissionApproach(State, Board, TEXT("Relay"), FEclipseLaneTuning(), Approach));
	TestEqual(TEXT("...military"), static_cast<int32>(Approach), static_cast<int32>(EEclipseTransitMode::Military));

	return true;
}

/**
 * Risk is a field something reads (21_quality_mandate.md): it breaks ties
 * between equal-length routes, and it can put a route out of reach entirely.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLaneRiskIsReadTest,
	"Eclipse.Strategy.Lanes.RiskDecidesBetweenEqualLengthRoutes",
	EclipseLaneTest::TestFlags)

bool FEclipseLaneRiskIsReadTest::RunTest(const FString& Parameters)
{
	using namespace EclipseLaneTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();
	const FEclipseCampaignState State = MakeState(Board);

	// Two ways to Quarry, both exactly three days:
	//   Home -> Waypoint (2d, r1) -> Quarry (1d, r1)  = 3d, risk  2
	//   Home -> Relay    (1d, r3) -> Quarry (2d, r40) = 3d, risk 43
	const FEclipseRoute Chosen = Route(State, Board, TEXT("Home"), TEXT("Quarry"));
	TestTrue(TEXT("A route to Quarry exists"), Chosen.bValid);
	TestEqual(TEXT("Both candidates are three days"), Chosen.TotalTravelDays, 3);
	AddInfo(FString::Printf(TEXT("GEMETEN  gekozen: %s (risico %d)"), *PathToString(Chosen), Chosen.TotalRisk));
	TestEqual(TEXT("THE POINT: the safe one wins the tie"),
		PathToString(Chosen), FString(TEXT("Home -> Waypoint -> Quarry")));
	TestEqual(TEXT("...at the risk the safe legs actually carry"), Chosen.TotalRisk, 2);

	// Risk tolerance: the same route, refused.
	const FEclipseRoute Refused = Route(State, Board, TEXT("Home"), TEXT("Quarry"), EEclipseTransitMode::Military, /*MaxRisk*/ 1);
	TestFalse(TEXT("A commander who will not spend 2 risk gets no route"), Refused.bValid);
	TestTrue(TEXT("...and is told it was the risk, not the geography"), Refused.FailureReason.Contains(TEXT("risk")));
	TestTrue(TEXT("CONTROL: one point more tolerance and the same route is fine"),
		Route(State, Board, TEXT("Home"), TEXT("Quarry"), EEclipseTransitMode::Military, /*MaxRisk*/ 2).bValid);

	return true;
}

/**
 * The Dominion Response Tier is state that CHANGES ROUTING, not a label. Same
 * board, same query, different temperature.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseResponseTierChangesRoutingTest,
	"Eclipse.Strategy.ResponseTier.TemperatureChangesWhatARouteCosts",
	EclipseLaneTest::TestFlags)

bool FEclipseResponseTierChangesRoutingTest::RunTest(const FString& Parameters)
{
	using namespace EclipseLaneTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();
	const FEclipseLaneTuning Tuning;

	FEclipseCampaignState Calm = MakeState(Board);
	Calm.ResponseTier = EEclipseDominionResponseTier::Indifference;
	const FEclipseRoute Quiet = Route(Calm, Board, TEXT("Home"), TEXT("Target"));

	FEclipseCampaignState Hunted = MakeState(Board);
	Hunted.ResponseTier = EEclipseDominionResponseTier::Rebellion; // tier 3
	const FEclipseRoute Watched = Route(Hunted, Board, TEXT("Home"), TEXT("Target"));

	TestTrue(TEXT("Both routes exist"), Quiet.bValid && Watched.bValid);
	TestEqual(TEXT("The road is the same road"), PathToString(Watched), PathToString(Quiet));
	TestEqual(TEXT("...and takes the same time"), Watched.TotalTravelDays, Quiet.TotalTravelDays);

	// Three legs, tier 3, four risk per tier: +36.
	const int32 Legs = Quiet.Steps.Num();
	TestEqual(TEXT("Sanity: three legs"), Legs, 3);
	TestEqual(TEXT("THE POINT: the same road costs more risk at tier 3"),
		Watched.TotalRisk - Quiet.TotalRisk,
		Legs * 3 * Tuning.RiskPerResponseTier);
	AddInfo(FString::Printf(TEXT("GEMETEN  tier 0: risico %d · tier 3: risico %d"), Quiet.TotalRisk, Watched.TotalRisk));

	// And the temperature can close a route the commander would otherwise take.
	const int32 Tolerance = Quiet.TotalRisk + 1;
	TestTrue(TEXT("CONTROL: at tier 0 this tolerance accepts the route"),
		Route(Calm, Board, TEXT("Home"), TEXT("Target"), EEclipseTransitMode::Military, Tolerance).bValid);
	TestFalse(TEXT("At tier 3 the same tolerance refuses it"),
		Route(Hunted, Board, TEXT("Home"), TEXT("Target"), EEclipseTransitMode::Military, Tolerance).bValid);

	return true;
}

/** Tier derivation reads campaign facts, and never walks back down. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseResponseTierDerivationTest,
	"Eclipse.Strategy.ResponseTier.DerivesFromFactsAndNeverDescends",
	EclipseLaneTest::TestFlags)

bool FEclipseResponseTierDerivationTest::RunTest(const FString& Parameters)
{
	FEclipseCampaignState State;
	auto AddRegion = [&State](FName Id, EEclipseRegionOwner Owner)
	{
		FEclipseRegionState& Region = State.Regions.AddDefaulted_GetRef();
		Region.RegionId = Id;
		Region.Owner = Owner;
	};

	// Nothing has happened.
	AddRegion(TEXT("Home"), EEclipseRegionOwner::Player);
	AddRegion(TEXT("A"), EEclipseRegionOwner::Dominion);
	AddRegion(TEXT("B"), EEclipseRegionOwner::Dominion);
	TestEqual(TEXT("A fresh campaign is beneath notice"),
		EclipseStrategyLogic::DeriveResponseTier(State), EEclipseDominionResponseTier::Indifference);

	// The story begins.
	State.StoryFlags.Add(FGameplayTag::RequestGameplayTag(TEXT("Story.Beat.M11.ThirteenBullets"), false));
	TestEqual(TEXT("One committed beat makes you a nuisance"),
		EclipseStrategyLogic::DeriveResponseTier(State), EEclipseDominionResponseTier::Nuisance);

	// You attacked and did not finish the job.
	State.Regions[1].Owner = EEclipseRegionOwner::Contested;
	TestEqual(TEXT("A contested region reads as insurgency"),
		EclipseStrategyLogic::DeriveResponseTier(State), EEclipseDominionResponseTier::Insurgency);

	// You took ground.
	State.Regions[1].Owner = EEclipseRegionOwner::Player;
	TestEqual(TEXT("A second held region is the first liberation — rebellion"),
		EclipseStrategyLogic::DeriveResponseTier(State), EEclipseDominionResponseTier::Rebellion);

	// Giving it back does not calm the empire down.
	State.ResponseTier = EEclipseDominionResponseTier::Rebellion;
	State.Regions[1].Owner = EEclipseRegionOwner::Dominion;
	State.StoryFlags.Reset();
	TestEqual(TEXT("THE POINT: losing ground does not lower the tier"),
		EclipseStrategyLogic::DeriveResponseTier(State), EEclipseDominionResponseTier::Rebellion);

	// 4 and 5 are authored-only, and derivation says so by never reaching them.
	for (FEclipseRegionState& Region : State.Regions)
	{
		Region.Owner = EEclipseRegionOwner::Player;
	}
	State.ResponseTier = EEclipseDominionResponseTier::Indifference;
	TestEqual(TEXT("Owning the whole district still tops out at Rebellion — War is multi-planet"),
		EclipseStrategyLogic::DeriveResponseTier(State), EEclipseDominionResponseTier::Rebellion);

	// But an authored tier is honoured and never derived away.
	State.ResponseTier = EEclipseDominionResponseTier::Existential;
	TestEqual(TEXT("An authored Existential survives derivation"),
		EclipseStrategyLogic::DeriveResponseTier(State), EEclipseDominionResponseTier::Existential);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
