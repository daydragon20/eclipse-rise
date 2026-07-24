// Unit tests for SPEC-P1-03 (GDD 14.4): the ledger core is pure logic, so most
// of this file needs no engine fixtures at all — states and params are built
// by hand, outcomes are exact.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/EclipseGameplayTags.h"
#include "Economy/EclipseEconomyLogic.h"
#include "Misc/AutomationTest.h"
#include "Strategy/EclipseCampaignTransaction.h"

namespace EclipseEconomyTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** Baseline params mirroring the GDD 6 prototype numbers (tests pin the math, data assets pin the game). */
	FEclipseEconomyTickParams MakeParams()
	{
		FEclipseEconomyTickParams Params;
		Params.WagePerSoldierPerDay = 15;
		Params.IntelDecayPerWeek = 0.05f;
		Params.IntelDecayIntervalDays = 7;
		Params.ContestedYieldFactor = 0.4f;
		Params.CreditsTag = EclipseTags::Resource_Credits.GetTag();
		Params.IntelTag = EclipseTags::Resource_Intel.GetTag();

		FEclipseRegionYieldParams& PlayerRegion = Params.Regions.AddDefaulted_GetRef();
		PlayerRegion.RegionId = TEXT("Region_Player");
		PlayerRegion.BaseYieldPerDay.Add(EclipseTags::Resource_Materials.GetTag(), 20);
		PlayerRegion.BaseYieldPerDay.Add(EclipseTags::Resource_Credits.GetTag(), 50);

		FEclipseRegionYieldParams& ContestedRegion = Params.Regions.AddDefaulted_GetRef();
		ContestedRegion.RegionId = TEXT("Region_Contested");
		ContestedRegion.BaseYieldPerDay.Add(EclipseTags::Resource_Materials.GetTag(), 10);

		FEclipseRegionYieldParams& DominionRegion = Params.Regions.AddDefaulted_GetRef();
		DominionRegion.RegionId = TEXT("Region_Dominion");
		DominionRegion.BaseYieldPerDay.Add(EclipseTags::Resource_Materials.GetTag(), 100);

		FEclipseProductionItemParams& Rifle = Params.ProductionItems.AddDefaulted_GetRef();
		Rifle.ItemId = TEXT("Item_RiflePlatform");
		Rifle.CostMaterials = 40;
		Rifle.TimeDays = 1;
		Rifle.ResultLoadoutTag = EclipseTags::Resource_Materials.GetTag(); // any valid tag works as an unlock marker in tests

		FEclipseProductionItemParams& Armor = Params.ProductionItems.AddDefaulted_GetRef();
		Armor.ItemId = TEXT("Item_ArmorSet");
		Armor.CostMaterials = 60;
		Armor.TimeDays = 2;
		return Params;
	}

	FEclipseCampaignState MakeState()
	{
		FEclipseCampaignState State;
		State.Day = 1;
		State.Wallet.Add(EclipseTags::Resource_Credits.GetTag(), 100);
		State.Wallet.Add(EclipseTags::Resource_Materials.GetTag(), 80);
		State.Wallet.Add(EclipseTags::Resource_Intel.GetTag(), 40);

		FEclipseRegionState& PlayerRegion = State.Regions.AddDefaulted_GetRef();
		PlayerRegion.RegionId = TEXT("Region_Player");
		PlayerRegion.Owner = EEclipseRegionOwner::Player;

		FEclipseRegionState& ContestedRegion = State.Regions.AddDefaulted_GetRef();
		ContestedRegion.RegionId = TEXT("Region_Contested");
		ContestedRegion.Owner = EEclipseRegionOwner::Contested;

		FEclipseRegionState& DominionRegion = State.Regions.AddDefaulted_GetRef();
		DominionRegion.RegionId = TEXT("Region_Dominion");
		DominionRegion.Owner = EEclipseRegionOwner::Dominion;

		for (int32 Index = 0; Index < 2; ++Index)
		{
			FEclipseSoldierRecord& Soldier = State.Roster.AddDefaulted_GetRef();
			Soldier.SoldierId = FGuid(7, 7, 7, Index + 1);
			Soldier.Name = FString::Printf(TEXT("Econ Tester %d"), Index + 1);
		}
		return State;
	}

	int32 SumAdjustments(const FEclipseCampaignTransaction& Transaction, const FGameplayTag& Resource, FName ReasonFilter = NAME_None)
	{
		int32 Total = 0;
		for (const FEclipseCampaignMutation& Mutation : Transaction.Mutations)
		{
			if (Mutation.Type == EEclipseCampaignMutationType::AdjustResource
				&& Mutation.ResourceType == Resource
				&& (ReasonFilter.IsNone() || Mutation.Reason == ReasonFilter))
			{
				Total += Mutation.Amount;
			}
		}
		return Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEconomyDayTickMathTest,
	"Eclipse.Economy.Ledger.DayTickMath",
	EclipseEconomyTest::TestFlags)

bool FEclipseEconomyDayTickMathTest::RunTest(const FString& Parameters)
{
	const FEclipseEconomyTickParams Params = EclipseEconomyTest::MakeParams();
	FEclipseCampaignState State = EclipseEconomyTest::MakeState();

	FEclipseCampaignTransaction Tick;
	TestTrue(TEXT("Tick produced"), EclipseEconomyLogic::BuildDayTick(State, Params, Tick));

	// Yields: player region 100% (20 M + 50 C), contested 40% of 10 M = 4 M,
	// Dominion region nothing. Wages: 2 soldiers x 15 C.
	TestEqual(TEXT("Materials yield 20 + 4"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Materials.GetTag(), TEXT("RegionYield")), 24);
	TestEqual(TEXT("Credits yield 50"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Credits.GetTag(), TEXT("RegionYield")), 50);
	TestEqual(TEXT("Wages -30"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Credits.GetTag(), TEXT("Wages")), -30);
	TestEqual(TEXT("No intel decay on day 1"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Intel.GetTag()), 0);

	// Applying against real state must succeed (the core validates its own output).
	TArray<FEclipseAppliedMutation> Applied;
	FString Error;
	TestTrue(TEXT("Tick commits"), EclipseCampaignLogic::CommitTransaction(State, Tick, Applied, Error));
	TestEqual(TEXT("Credits: 100 + 50 - 30"), State.GetBalance(EclipseTags::Resource_Credits.GetTag()), 120);
	TestEqual(TEXT("Materials: 80 + 24"), State.GetBalance(EclipseTags::Resource_Materials.GetTag()), 104);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEconomyDeterministicTickTest,
	"Eclipse.Economy.Ledger.DeterministicTick",
	EclipseEconomyTest::TestFlags)

bool FEclipseEconomyDeterministicTickTest::RunTest(const FString& Parameters)
{
	const FEclipseEconomyTickParams Params = EclipseEconomyTest::MakeParams();
	const FEclipseCampaignState State = EclipseEconomyTest::MakeState();

	FEclipseCampaignTransaction TickA;
	FEclipseCampaignTransaction TickB;
	EclipseEconomyLogic::BuildDayTick(State, Params, TickA);
	EclipseEconomyLogic::BuildDayTick(State, Params, TickB);

	TestEqual(TEXT("Same state + params -> same mutation count"), TickA.Mutations.Num(), TickB.Mutations.Num());
	for (int32 Index = 0; Index < FMath::Min(TickA.Mutations.Num(), TickB.Mutations.Num()); ++Index)
	{
		TestTrue(TEXT("Mutation order and content identical"),
			TickA.Mutations[Index].Type == TickB.Mutations[Index].Type
			&& TickA.Mutations[Index].ResourceType == TickB.Mutations[Index].ResourceType
			&& TickA.Mutations[Index].Amount == TickB.Mutations[Index].Amount
			&& TickA.Mutations[Index].Reason == TickB.Mutations[Index].Reason);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEconomyFacilityYieldTickTest,
	"Eclipse.Economy.Ledger.FacilityYieldEntersDayTick",
	EclipseEconomyTest::TestFlags)

bool FEclipseEconomyFacilityYieldTickTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-03: facility output (the IC's intel) rides the SAME deterministic
	// day tick as region yields — its own ledger lines (GDD 7.6), and facility
	// credits count toward the wage clamp like region credits do.
	FEclipseEconomyTickParams Params = EclipseEconomyTest::MakeParams();
	Params.Regions.Reset(); // isolate the facility stream
	Params.FacilityYields.YieldPerDay.Add(EclipseTags::Resource_Intel.GetTag(), 3);
	Params.FacilityYields.YieldPerDay.Add(EclipseTags::Resource_Credits.GetTag(), 10);

	FEclipseCampaignState State = EclipseEconomyTest::MakeState();
	State.Wallet.Add(EclipseTags::Resource_Credits.GetTag(), 0); // broke: the 6.5 Act 1 feeling

	FEclipseCampaignTransaction Tick;
	TestTrue(TEXT("Tick produced"), EclipseEconomyLogic::BuildDayTick(State, Params, Tick));

	TestEqual(TEXT("Facility intel enters the tick (+3, reason FacilityYield)"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Intel.GetTag(), TEXT("FacilityYield")), 3);
	TestEqual(TEXT("Facility credits enter the tick (+10)"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Credits.GetTag(), TEXT("FacilityYield")), 10);

	// Wages due 2 x 15 = 30; only the facility's 10 C exist -> clamped short pay.
	TestEqual(TEXT("Wage clamp counts facility credits (Wages_Short -10)"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Credits.GetTag(), TEXT("Wages_Short")), -10);

	// The composed tick must commit against real state (never overdraw).
	TArray<FEclipseAppliedMutation> Applied;
	FString Error;
	TestTrue(TEXT("Tick commits"), EclipseCampaignLogic::CommitTransaction(State, Tick, Applied, Error));
	TestEqual(TEXT("Credits end at 0 (10 in, 10 out)"), State.GetBalance(EclipseTags::Resource_Credits.GetTag()), 0);
	TestEqual(TEXT("Intel: 40 + 3"), State.GetBalance(EclipseTags::Resource_Intel.GetTag()), 43);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEconomyIntelDecayTest,
	"Eclipse.Economy.Ledger.IntelDecay",
	EclipseEconomyTest::TestFlags)

bool FEclipseEconomyIntelDecayTest::RunTest(const FString& Parameters)
{
	const FEclipseEconomyTickParams Params = EclipseEconomyTest::MakeParams();
	FEclipseCampaignState State = EclipseEconomyTest::MakeState();
	State.Day = 7;

	FEclipseCampaignTransaction Tick;
	EclipseEconomyLogic::BuildDayTick(State, Params, Tick);

	// 5% of 40 intel = 2, floored.
	TestEqual(TEXT("Intel decays on day 7"),
		EclipseEconomyTest::SumAdjustments(Tick, EclipseTags::Resource_Intel.GetTag(), TEXT("IntelDecay")), -2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEconomyProductionScheduleTest,
	"Eclipse.Economy.Ledger.ProductionSchedule",
	EclipseEconomyTest::TestFlags)

bool FEclipseEconomyProductionScheduleTest::RunTest(const FString& Parameters)
{
	const FEclipseEconomyTickParams Params = EclipseEconomyTest::MakeParams();
	FEclipseCampaignState State = EclipseEconomyTest::MakeState();

	// Order the rifle (costs 40 M of the available 80).
	FEclipseCampaignTransaction Order;
	FString Error;
	TestTrue(TEXT("Order builds"), EclipseEconomyLogic::BuildProductionOrder(
		State, Params, TEXT("Item_RiflePlatform"), EclipseTags::Resource_Materials.GetTag(), Order, Error));

	TArray<FEclipseAppliedMutation> Applied;
	TestTrue(TEXT("Order commits"), EclipseCampaignLogic::CommitTransaction(State, Order, Applied, Error));
	TestEqual(TEXT("Materials spent"), State.GetBalance(EclipseTags::Resource_Materials.GetTag()), 40);
	TestEqual(TEXT("Queue holds the order"), State.ProductionQueue.Num(), 1);
	TestEqual(TEXT("Completes on day 2 (day 1 + 1)"), State.ProductionQueue[0].CompletesOnDay, 2);

	// Not done on day 1's tick...
	FEclipseCampaignTransaction TickDay1;
	EclipseEconomyLogic::BuildDayTick(State, Params, TickDay1);
	const bool CompletedDay1 = TickDay1.Mutations.ContainsByPredicate(
		[](const FEclipseCampaignMutation& M) { return M.Type == EEclipseCampaignMutationType::CompleteProduction; });
	TestFalse(TEXT("No completion before schedule"), CompletedDay1);

	// ...done on day 2's.
	State.Day = 2;
	FEclipseCampaignTransaction TickDay2;
	EclipseEconomyLogic::BuildDayTick(State, Params, TickDay2);
	const FEclipseCampaignMutation* Completion = TickDay2.Mutations.FindByPredicate(
		[](const FEclipseCampaignMutation& M) { return M.Type == EEclipseCampaignMutationType::CompleteProduction; });
	TestNotNull(TEXT("Completion mutation on schedule"), Completion);

	TestTrue(TEXT("Completion commits"), EclipseCampaignLogic::CommitTransaction(State, TickDay2, Applied, Error));
	TestEqual(TEXT("Queue empty after completion"), State.ProductionQueue.Num(), 0);
	TestEqual(TEXT("Loadout unlocked"), State.UnlockedLoadoutTags.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEconomyInsufficientFundsTest,
	"Eclipse.Economy.Ledger.InsufficientFundsRejection",
	EclipseEconomyTest::TestFlags)

bool FEclipseEconomyInsufficientFundsTest::RunTest(const FString& Parameters)
{
	const FEclipseEconomyTickParams Params = EclipseEconomyTest::MakeParams();
	FEclipseCampaignState State = EclipseEconomyTest::MakeState();
	State.Wallet.Add(EclipseTags::Resource_Materials.GetTag(), 30); // below the armor's 60

	FEclipseCampaignTransaction Order;
	FString Error;
	TestTrue(TEXT("Order builds (affordability is the transaction's job)"), EclipseEconomyLogic::BuildProductionOrder(
		State, Params, TEXT("Item_ArmorSet"), EclipseTags::Resource_Materials.GetTag(), Order, Error));

	const uint32 HashBefore = State.ComputeStateHash();
	TArray<FEclipseAppliedMutation> Applied;
	TestFalse(TEXT("Unaffordable order rejected atomically"), EclipseCampaignLogic::CommitTransaction(State, Order, Applied, Error));
	TestEqual(TEXT("State untouched"), State.ComputeStateHash(), HashBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEconomyThirtyDaySoakTest,
	"Eclipse.Economy.Ledger.ThirtyDaySoak",
	EclipseEconomyTest::TestFlags)

bool FEclipseEconomyThirtyDaySoakTest::RunTest(const FString& Parameters)
{
	// SPEC-P1-03: 30 scripted days stay within hand-set prototype bands — the
	// precursor of the GDD 6.5 band assertion. Bands are deliberately generous;
	// they catch runaway math, not tuning drift.
	const FEclipseEconomyTickParams Params = EclipseEconomyTest::MakeParams();
	FEclipseCampaignState State = EclipseEconomyTest::MakeState();

	FString Error;
	for (int32 DayIndex = 0; DayIndex < 30; ++DayIndex)
	{
		FEclipseCampaignMutation Advance;
		Advance.Type = EEclipseCampaignMutationType::AdvanceDay;
		FEclipseCampaignTransaction AdvanceTransaction;
		AdvanceTransaction.Source = TEXT("SoakTest");
		AdvanceTransaction.Mutations.Add(Advance);

		TArray<FEclipseAppliedMutation> Applied;
		if (!EclipseCampaignLogic::CommitTransaction(State, AdvanceTransaction, Applied, Error))
		{
			return TestTrue(FString::Printf(TEXT("AdvanceDay failed: %s"), *Error), false);
		}

		FEclipseCampaignTransaction Tick;
		if (EclipseEconomyLogic::BuildDayTick(State, Params, Tick))
		{
			if (!EclipseCampaignLogic::CommitTransaction(State, Tick, Applied, Error))
			{
				return TestTrue(FString::Printf(TEXT("Day %d tick rejected: %s"), State.Day, *Error), false);
			}
		}
	}

	const int32 Credits = State.GetBalance(EclipseTags::Resource_Credits.GetTag());
	const int32 Materials = State.GetBalance(EclipseTags::Resource_Materials.GetTag());
	const int32 Intel = State.GetBalance(EclipseTags::Resource_Intel.GetTag());

	// Hand-set prototype bands: income (+50C/+24M, wages -30C) over 30 days from 100C/80M.
	TestTrue(TEXT("Credits within band [400, 900]"), Credits >= 400 && Credits <= 900);
	TestTrue(TEXT("Materials within band [600, 1000]"), Materials >= 600 && Materials <= 1000);
	TestTrue(TEXT("Intel decayed but not zeroed"), Intel > 0 && Intel < 40);
	TestEqual(TEXT("Clock at day 31"), State.Day, 31);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
