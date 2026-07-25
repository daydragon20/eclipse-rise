// Unit tests for SPEC-P2-03 steps 1-2 (GDD 14.4, blocking): build-order
// validation on the slot-graph (occupied slot, unknown facility, authored
// placement, insufficient funds - never a crash), the construction day-tick
// (multi-day, crew -1 with floor 1, staff release), rush (cost = 60 x remaining
// days, instant completion), facility yields incl. the IC analyst bonus, and
// the seeded spec start state (Command Center pre-built at L1). Pure logic:
// deterministic, no engine actors, tables built in-memory like EclipseClassTests.
// Step 4-5 (anchored step-3 review findings) adds the casualty staff-release
// (no ghost analyst), the mutation-layer staff cap, and the day-N-completion-
// yields-on-day-N pin the nightly soak leans on.
//
// The walkable-vault parity Gauntlet (SPEC-P2-03 step 4-5, blocking the P1-08
// menu-hub retirement) lives at the bottom of this file: it spawns the vault
// into a headless world and holds four contracts - PARITY (what the vault
// physically places == what the state says == what the hub renders),
// DETERMINISM (two builds on one state are byte-identical), DEGRADATION (a
// missing layout is a warning + an empty vault, never a crash - GDD 14.3.5) and
// VISIBILITY (a state row without a layout slot is shown and tagged, never
// silently hidden), plus the bus wiring: the standing vault re-renders on the
// existing Event.Base.* facts alone (no tick, GDD 12.4).

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipseBaseLogic.h"
#include "Base/EclipseBaseSubsystem.h"
#include "Base/EclipseBaseTypes.h"
#include "Base/EclipseVaultBuilder.h"
#include "Core/EclipseGameplayTags.h"
#include "Economy/EclipseEconomyLogic.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseCampaignTransaction.h"
#include "Strategy/EclipseCampaignTypes.h"

namespace EclipseBaseTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** The slice DT_Facilities in-memory: x0.8 Act-1 cost table (SPEC-P2-03 locked decision 3). */
	UDataTable* MakeFacilitiesTable()
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FEclipseFacilityRow::StaticStruct();

		FEclipseFacilityRow CommandCenter;
		CommandCenter.DisplayName = FText::FromString(TEXT("Command Center"));
		CommandCenter.Levels.AddDefaulted(); // free, pre-built (locked decision 2); the row exists for display/data completeness
		Table->AddRow(EclipseBaseDefaults::CommandCenterFacilityId, CommandCenter);

		FEclipseFacilityRow Barracks;
		Barracks.DisplayName = FText::FromString(TEXT("Barracks"));
		FEclipseFacilityLevelData& BarracksL1 = Barracks.Levels.AddDefaulted_GetRef();
		BarracksL1.CostMaterials = 120;
		BarracksL1.BuildDays = 2;
		Table->AddRow(TEXT("Barracks"), Barracks);

		FEclipseFacilityRow Workshop;
		Workshop.DisplayName = FText::FromString(TEXT("Workshop"));
		FEclipseFacilityLevelData& WorkshopL1 = Workshop.Levels.AddDefaulted_GetRef();
		WorkshopL1.CostMaterials = 160;
		WorkshopL1.BuildDays = 3;
		FEclipseFacilityLevelData& WorkshopL2 = Workshop.Levels.AddDefaulted_GetRef();
		WorkshopL2.CostMaterials = 200; // slice-native L2 price (SPEC-P2-03 cost table)
		WorkshopL2.BuildDays = 3;
		Table->AddRow(TEXT("Workshop"), Workshop);

		FEclipseFacilityRow IntelCenter;
		IntelCenter.DisplayName = FText::FromString(TEXT("Intelligence Center"));
		FEclipseFacilityLevelData& IntelL1 = IntelCenter.Levels.AddDefaulted_GetRef();
		IntelL1.CostMaterials = 240;
		IntelL1.BuildDays = 4;
		IntelL1.YieldPerDay.Add(EclipseTags::Resource_Intel.GetTag(), 2); // +2 Intel/day (locked decision 1)
		Table->AddRow(TEXT("IntelligenceCenter"), IntelCenter);

		return Table;
	}

	/** The 4-slot Act 1 vault slot-graph (SPEC-P2-03): every slot hangs off the Spine; placement is authored per slot. */
	TArray<FEclipseBaseSlotDef> MakeSlots()
	{
		TArray<FEclipseBaseSlotDef> Slots;

		FEclipseBaseSlotDef& SlotA = Slots.AddDefaulted_GetRef();
		SlotA.SlotId = EclipseBaseDefaults::CommandSlotId;
		SlotA.AllowedFacilityRows = { EclipseBaseDefaults::CommandCenterFacilityId };
		SlotA.AdjacentSlotIds = { TEXT("Spine") };

		FEclipseBaseSlotDef& SlotB = Slots.AddDefaulted_GetRef();
		SlotB.SlotId = TEXT("Slot_B");
		SlotB.AllowedFacilityRows = { TEXT("Barracks") };
		SlotB.AdjacentSlotIds = { TEXT("Spine"), TEXT("MemorialAlcove") };
		SlotB.EmptyStateStreamingId = TEXT("HP_SlotB_BunkCamp"); // the 5.2 bunk camp, not raw rock

		FEclipseBaseSlotDef& SlotC = Slots.AddDefaulted_GetRef();
		SlotC.SlotId = TEXT("Slot_C");
		SlotC.AllowedFacilityRows = { TEXT("Workshop") };
		SlotC.AdjacentSlotIds = { TEXT("Spine"), TEXT("GeneratorRoom") };

		FEclipseBaseSlotDef& SlotD = Slots.AddDefaulted_GetRef();
		SlotD.SlotId = TEXT("Slot_D");
		SlotD.AllowedFacilityRows = { TEXT("IntelligenceCenter") };
		SlotD.AdjacentSlotIds = { TEXT("Spine") };

		return Slots;
	}

	const FEclipseFacilityRow* FindRow(const UDataTable* Table, FName FacilityId)
	{
		return Table->FindRow<FEclipseFacilityRow>(FacilityId, TEXT("EclipseBaseTest"), /*bWarnIfMissing*/ false);
	}

	EclipseBaseLogic::FEclipseBaseTuningParams MakeTuning()
	{
		EclipseBaseLogic::FEclipseBaseTuningParams Tuning; // the DA_BaseTuning spec values are the struct defaults
		Tuning.AnalystBonusResource = EclipseTags::Resource_Intel.GetTag();
		return Tuning;
	}

	/**
	 * A throwaway game world for the vault probes: actors only, never ticked -
	 * which is itself the GDD 12.4 contract under test (the builder does all its
	 * work in one call and leaves nothing behind that needs a frame).
	 */
	struct FScopedTestWorld
	{
		FScopedTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		}

		~FScopedTestWorld()
		{
			if (World != nullptr)
			{
				World->DestroyWorld(/*bBroadcastWorldDestroyedEvent*/ false);
			}
		}

		UWorld* World = nullptr;
	};

	/**
	 * A base state that exercises every row of the SPEC-P2-03 visible-growth
	 * table at once: Slot_A = the pre-built operational Command Center,
	 * Slot_B = Barracks under construction with a crew on site (and the authored
	 * bunk-camp empty state underneath), Slot_C = Workshop operational at L2 with
	 * an analyst, Slot_D = untouched (no state row at all - not even an empty one).
	 */
	FEclipseBaseState MakeGrowthSpreadState()
	{
		using namespace EclipseBaseLogic;
		FEclipseBaseState Base; // seeded: CC L1 at Slot_A (locked decision 2)

		StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), /*BuildDays*/ 2);
		Base.FindBySlot(TEXT("Slot_B"))->AssignedSoldierIds.Add(FGuid(5, 0, 0, 1));

		// Re-fetch after every structural change: the facility array reallocates.
		StartConstruction(Base, TEXT("Slot_C"), TEXT("Workshop"), /*BuildDays*/ 3);
		ApplyRush(*Base.FindBySlot(TEXT("Slot_C"))); // operational L1
		StartConstruction(Base, TEXT("Slot_C"), TEXT("Workshop"), /*BuildDays*/ 3);
		ApplyRush(*Base.FindBySlot(TEXT("Slot_C"))); // operational L2 (the only slice upgrade)
		Base.FindBySlot(TEXT("Slot_C"))->AssignedSoldierIds.Add(FGuid(5, 0, 0, 2)); // rush releases staff: assign after

		return Base;
	}

	/** The placed marker for a slot, or null when the vault never placed one. */
	const EclipseVault::FEclipseVaultSlotView* FindView(const TArray<EclipseVault::FEclipseVaultSlotView>& Views, FName SlotId)
	{
		return Views.FindByPredicate([SlotId](const EclipseVault::FEclipseVaultSlotView& View) { return View.SlotId == SlotId; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseSpecStartStateTest,
	"Eclipse.Base.SpecStartState",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseSpecStartStateTest::RunTest(const FString& Parameters)
{
	// Locked decision 2 as a type invariant: every campaign state is born with
	// the pre-built L1 Command Center at slot A - new campaigns, migrated saves
	// and asset-less fallbacks all agree (GDD 14.3.5).
	const FEclipseCampaignState State;
	TestEqual(TEXT("Fresh state has exactly one facility"), State.BaseState.Facilities.Num(), 1);

	const FEclipseFacilityState* CommandCenter = State.BaseState.FindBySlot(EclipseBaseDefaults::CommandSlotId);
	TestNotNull(TEXT("Command Center sits at slot A"), CommandCenter);
	if (CommandCenter != nullptr)
	{
		TestEqual(TEXT("It is the Command Center"), CommandCenter->FacilityId, EclipseBaseDefaults::CommandCenterFacilityId);
		TestEqual(TEXT("Pre-built at L1"), CommandCenter->Level, 1);
		TestEqual(TEXT("Operational from day 0"), CommandCenter->DaysRemaining, 0);
		TestEqual(TEXT("Unstaffed"), CommandCenter->AssignedSoldierIds.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseBuildValidationTest,
	"Eclipse.Base.BuildOrderValidation",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseBuildValidationTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	UDataTable* Facilities = EclipseBaseTest::MakeFacilitiesTable();
	const TArray<FEclipseBaseSlotDef> Slots = EclipseBaseTest::MakeSlots();
	FEclipseBaseState Base; // seeded: CC at slot A

	const FEclipseFacilityRow* BarracksRow = EclipseBaseTest::FindRow(Facilities, TEXT("Barracks"));
	const FEclipseFacilityRow* WorkshopRow = EclipseBaseTest::FindRow(Facilities, TEXT("Workshop"));
	int32 TargetLevel = 0;
	FString Error;

	// Happy path: Barracks at the bunk camp, funded (locked decision 3 costs).
	TestTrue(TEXT("Barracks at Slot_B validates"),
		ValidateBuildOrder(Base, Slots, TEXT("Slot_B"), TEXT("Barracks"), BarracksRow, 120, 0, TargetLevel, Error));
	TestEqual(TEXT("New build targets L1"), TargetLevel, 1);

	// Unknown slot: rejected, never a crash (GDD 14.3.5).
	TestFalse(TEXT("Unknown slot rejects"),
		ValidateBuildOrder(Base, Slots, TEXT("Slot_Z"), TEXT("Barracks"), BarracksRow, 999, 999, TargetLevel, Error));
	TestTrue(TEXT("Error names the slot"), Error.Contains(TEXT("unknown slot")));

	// Unknown facility (null row - e.g. a Phase 3 Medbay id): rejected.
	TestFalse(TEXT("Null facility row rejects"),
		ValidateBuildOrder(Base, Slots, TEXT("Slot_B"), TEXT("Medbay"), nullptr, 999, 999, TargetLevel, Error));
	TestTrue(TEXT("Error names the facility"), Error.Contains(TEXT("unknown facility")));

	// Authored placement (locked decision 2): the Workshop does not fit the bunk room.
	TestFalse(TEXT("Workshop at Slot_B rejects"),
		ValidateBuildOrder(Base, Slots, TEXT("Slot_B"), TEXT("Workshop"), WorkshopRow, 999, 999, TargetLevel, Error));
	TestTrue(TEXT("Error says not allowed"), Error.Contains(TEXT("not allowed")));

	// Insufficient materials: the P1-03 rejection reused (119 < 120).
	TestFalse(TEXT("Underfunded build rejects"),
		ValidateBuildOrder(Base, Slots, TEXT("Slot_B"), TEXT("Barracks"), BarracksRow, 119, 0, TargetLevel, Error));
	TestTrue(TEXT("Error names materials"), Error.Contains(TEXT("insufficient materials")));

	// Under construction: no double orders on one slot.
	const FEclipseFacilityLevelData* WorkshopL1 = GetLevelData(WorkshopRow, 1);
	TestNotNull(TEXT("Workshop L1 data exists"), WorkshopL1);
	StartConstruction(Base, TEXT("Slot_C"), TEXT("Workshop"), *WorkshopL1);
	TestFalse(TEXT("Building slot rejects a second order"),
		ValidateBuildOrder(Base, Slots, TEXT("Slot_C"), TEXT("Workshop"), WorkshopRow, 999, 999, TargetLevel, Error));
	TestTrue(TEXT("Error says under construction"), Error.Contains(TEXT("under construction")));

	// Occupied by another facility (needs a slot whose data allows two rows).
	TArray<FEclipseBaseSlotDef> SlotsWithShared = Slots;
	FEclipseBaseSlotDef& Shared = SlotsWithShared.AddDefaulted_GetRef();
	Shared.SlotId = TEXT("Slot_Shared");
	Shared.AllowedFacilityRows = { TEXT("Barracks"), TEXT("Workshop") };
	const FEclipseFacilityLevelData* BarracksL1 = GetLevelData(BarracksRow, 1);
	FEclipseFacilityState& SharedSite = StartConstruction(Base, TEXT("Slot_Shared"), TEXT("Barracks"), *BarracksL1);
	ApplyRush(SharedSite); // operational Barracks
	TestFalse(TEXT("Occupied slot rejects a different facility"),
		ValidateBuildOrder(Base, SlotsWithShared, TEXT("Slot_Shared"), TEXT("Workshop"), WorkshopRow, 999, 999, TargetLevel, Error));
	TestTrue(TEXT("Error says occupied"), Error.Contains(TEXT("occupied")));

	// Upgrade path: the operational Workshop targets L2 at the slice-native price.
	FEclipseFacilityState* WorkshopSite = Base.FindBySlot(TEXT("Slot_C"));
	ApplyRush(*WorkshopSite);
	TestTrue(TEXT("Workshop upgrade validates"),
		ValidateBuildOrder(Base, Slots, TEXT("Slot_C"), TEXT("Workshop"), WorkshopRow, 200, 0, TargetLevel, Error));
	TestEqual(TEXT("Upgrade targets L2"), TargetLevel, 2);

	// No L2 for anything but the Workshop - data decides, not code.
	TestFalse(TEXT("Barracks upgrade rejects (no L2 data)"),
		ValidateBuildOrder(Base, SlotsWithShared, TEXT("Slot_Shared"), TEXT("Barracks"), BarracksRow, 999, 999, TargetLevel, Error));
	TestTrue(TEXT("Error names the missing level"), Error.Contains(TEXT("no level 2 data")));

	// Slot A is taken by the mandatory core: rebuilding the CC rejects on level data (row has only L1).
	const FEclipseFacilityRow* CommandRow = EclipseBaseTest::FindRow(Facilities, EclipseBaseDefaults::CommandCenterFacilityId);
	TestFalse(TEXT("Slot A rejects a rebuild"),
		ValidateBuildOrder(Base, Slots, EclipseBaseDefaults::CommandSlotId, EclipseBaseDefaults::CommandCenterFacilityId, CommandRow, 999, 999, TargetLevel, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseConstructionTickTest,
	"Eclipse.Base.ConstructionDayTick",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseConstructionTickTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	UDataTable* Facilities = EclipseBaseTest::MakeFacilitiesTable();
	const FEclipseBaseTuningParams Tuning = EclipseBaseTest::MakeTuning();
	const FEclipseFacilityLevelData* IntelL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("IntelligenceCenter")), 1);
	const FEclipseFacilityLevelData* BarracksL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("Barracks")), 1);

	// Uncrewed IC: the full 4 days, completion on the 4th tick only.
	{
		FEclipseBaseState Base;
		StartConstruction(Base, TEXT("Slot_D"), TEXT("IntelligenceCenter"), *IntelL1);
		for (int32 Day = 1; Day <= 3; ++Day)
		{
			TestEqual(FString::Printf(TEXT("Day %d: nothing completes"), Day), TickConstructionDay(Base, Tuning).Num(), 0);
		}
		TArray<FEclipseFacilityCompletion> Completions = TickConstructionDay(Base, Tuning);
		TestEqual(TEXT("IC completes on day 4"), Completions.Num(), 1);
		if (Completions.Num() == 1)
		{
			TestEqual(TEXT("Completion names the slot"), Completions[0].SlotId, FName(TEXT("Slot_D")));
			TestEqual(TEXT("Completion is L1 (Built, not Upgraded)"), Completions[0].NewLevel, 1);
		}
		TestEqual(TEXT("Site is operational at L1"), Base.FindBySlot(TEXT("Slot_D"))->Level, 1);
		TestEqual(TEXT("No days remaining"), Base.FindBySlot(TEXT("Slot_D"))->DaysRemaining, 0);
	}

	// Crewed IC: 4 -> 3 days (crew -1, locked decision 6), and the crew is released on completion.
	{
		FEclipseBaseState Base;
		FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_D"), TEXT("IntelligenceCenter"), *IntelL1);
		Site.AssignedSoldierIds.Add(FGuid(1, 2, 3, 4));
		TestEqual(TEXT("Day 1: building"), TickConstructionDay(Base, Tuning).Num(), 0);
		TestEqual(TEXT("Day 2: building"), TickConstructionDay(Base, Tuning).Num(), 0);
		TestEqual(TEXT("Crewed IC completes on day 3"), TickConstructionDay(Base, Tuning).Num(), 1);
		TestEqual(TEXT("Crew released on completion"), Base.FindBySlot(TEXT("Slot_D"))->AssignedSoldierIds.Num(), 0);
	}

	// Crewed Barracks: 2 -> 1 day; a 1-day-equivalent build still takes 1 day (floor, never 0).
	{
		FEclipseBaseState Base;
		FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), *BarracksL1);
		Site.AssignedSoldierIds.Add(FGuid(1, 2, 3, 4));
		TestEqual(TEXT("Crewed Barracks completes on day 1 (2 - 1)"), TickConstructionDay(Base, Tuning).Num(), 1);
	}
	{
		FEclipseFacilityLevelData OneDay;
		OneDay.BuildDays = 1;
		FEclipseBaseState Base;
		FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), OneDay);
		Site.AssignedSoldierIds.Add(FGuid(1, 2, 3, 4));
		TestEqual(TEXT("Crew never makes a build free: min 1 day"), Site.DaysRemaining, 1);
		TestEqual(TEXT("1-day crewed build completes on day 1, not day 0"), TickConstructionDay(Base, Tuning).Num(), 1);
	}

	// CrewDayReduction = 0 in data: a crew changes nothing (tunable, not code - GDD 14.2).
	{
		FEclipseBaseTuningParams NoCrewBonus = Tuning;
		NoCrewBonus.CrewDayReduction = 0;
		FEclipseBaseState Base;
		FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), *BarracksL1);
		Site.AssignedSoldierIds.Add(FGuid(1, 2, 3, 4));
		TestEqual(TEXT("Day 1: still building"), TickConstructionDay(Base, NoCrewBonus).Num(), 0);
		TestEqual(TEXT("Completes on the full day 2"), TickConstructionDay(Base, NoCrewBonus).Num(), 1);
	}

	// Two sites on one strategic day: both tick (construction progresses while you fight - locked decision 4).
	{
		FEclipseBaseState Base;
		StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), *BarracksL1);
		StartConstruction(Base, TEXT("Slot_D"), TEXT("IntelligenceCenter"), *IntelL1);
		TickConstructionDay(Base, Tuning);
		TestEqual(TEXT("Barracks ticked"), Base.FindBySlot(TEXT("Slot_B"))->DaysRemaining, 1);
		TestEqual(TEXT("IC ticked"), Base.FindBySlot(TEXT("Slot_D"))->DaysRemaining, 3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseRushTest,
	"Eclipse.Base.RushCostAndCompletion",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseRushTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	UDataTable* Facilities = EclipseBaseTest::MakeFacilitiesTable();
	const FEclipseBaseTuningParams Tuning = EclipseBaseTest::MakeTuning();
	const FEclipseFacilityLevelData* WorkshopL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("Workshop")), 1);

	FEclipseBaseState Base;
	FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_C"), TEXT("Workshop"), *WorkshopL1);
	FString Error;

	// 60 C x remaining days (DA_BaseTuning values): 3 days = 180, then 120 after a tick.
	TestEqual(TEXT("Rush cost at 3 days"), ComputeRushCost(&Site, Tuning), 180);
	TickConstructionDay(Base, Tuning);
	TestEqual(TEXT("Rush cost at 2 days"), ComputeRushCost(&Site, Tuning), 120);

	// Payroll-clamped wallets feel this: 119 C is not enough (available, never comfortable - 5.4).
	TestFalse(TEXT("Underfunded rush rejects"), ValidateRush(&Site, Tuning, 119, Error));
	TestTrue(TEXT("Error names credits"), Error.Contains(TEXT("insufficient credits")));
	TestTrue(TEXT("Funded rush validates"), ValidateRush(&Site, Tuning, 120, Error));

	// The rush commit completes instantly in that same commit (clock rules).
	const FEclipseFacilityCompletion Completion = ApplyRush(Site);
	TestEqual(TEXT("Rush completes to L1"), Completion.NewLevel, 1);
	TestEqual(TEXT("Site operational"), Site.DaysRemaining, 0);
	TestEqual(TEXT("Site at L1"), Site.Level, 1);

	// Nothing to rush anymore: reject, and the cost reads 0.
	TestFalse(TEXT("Rushing an operational site rejects"), ValidateRush(&Site, Tuning, 9999, Error));
	TestEqual(TEXT("Operational rush cost is 0"), ComputeRushCost(&Site, Tuning), 0);
	TestFalse(TEXT("Null site rejects, never crashes"), ValidateRush(nullptr, Tuning, 9999, Error));

	// Guard (review R6/e): ApplyRush on an operational site is a no-op — a
	// double-apply must never double-bump the level.
	const FEclipseFacilityCompletion Guarded = ApplyRush(Site);
	TestEqual(TEXT("Guarded double-rush completes nothing (NewLevel 0)"), Guarded.NewLevel, 0);
	TestEqual(TEXT("Level not double-bumped"), Site.Level, 1);
	TestEqual(TEXT("Site stays operational"), Site.DaysRemaining, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseUpgradeCompletionTest,
	"Eclipse.Base.UpgradeCompletionYieldRetentionAndAnalystSuspension",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseUpgradeCompletionTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	// Review R6/b: tick a yielding facility from L1 to L2 and pin three
	// contracts at once — the completion reads Upgraded (NewLevel == 2, not
	// Built), the L1 yield keeps flowing during the upgrade, and the analyst
	// bonus is suspended while Level >= 1 && DaysRemaining > 0 (staff on a
	// building site is positionally the crew). Rows are test-local data:
	// which facility has an L2 is data, never code.
	const FGameplayTag Intel = EclipseTags::Resource_Intel.GetTag();
	const FEclipseBaseTuningParams Tuning = EclipseBaseTest::MakeTuning();
	const FName SlotId(TEXT("Slot_D"));
	const FName FacilityId(TEXT("IntelligenceCenter"));
	const FGuid Analyst(21, 22, 23, 24);

	FEclipseFacilityRow YieldingRow;
	FEclipseFacilityLevelData& L1 = YieldingRow.Levels.AddDefaulted_GetRef();
	L1.BuildDays = 4;
	L1.YieldPerDay.Add(Intel, 2);
	FEclipseFacilityLevelData& L2 = YieldingRow.Levels.AddDefaulted_GetRef();
	L2.BuildDays = 3;
	L2.YieldPerDay.Add(Intel, 4);
	const auto Resolver = [&YieldingRow, FacilityId](FName Id) -> const FEclipseFacilityRow* { return Id == FacilityId ? &YieldingRow : nullptr; };

	FEclipseBaseState Base;
	FEclipseFacilityState& Site = StartConstruction(Base, SlotId, FacilityId, *GetLevelData(&YieldingRow, 1));
	ApplyRush(Site);
	Site.AssignedSoldierIds.Add(Analyst);
	TestEqual(TEXT("Operational + analyst: 2 + 1 Intel"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 3);

	// The upgrade order targets L2 through the shared state validator.
	int32 TargetLevel = 0;
	FString Error;
	TestTrue(TEXT("Upgrade order validates on state"), ValidateStartConstructionState(Base, SlotId, FacilityId, TargetLevel, Error));
	TestEqual(TEXT("Order targets L2"), TargetLevel, 2);
	StartConstruction(Base, SlotId, FacilityId, *GetLevelData(&YieldingRow, 2));
	TestEqual(TEXT("Level stays 1 during the upgrade"), Site.Level, 1);
	TestEqual(TEXT("Upgrade takes the L2 build days"), Site.DaysRemaining, 3);

	// Yield retention + analyst suspension: L1 keeps producing, the staffed
	// soldier now positionally works construction — no bonus.
	TestEqual(TEXT("During upgrade: L1 yield retained, bonus suspended (2, not 3 or 4)"),
		ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 2);

	// Crewed 3-day upgrade completes on tick 2 (crew -1).
	TestEqual(TEXT("Upgrade day 1: still building"), TickConstructionDay(Base, Tuning).Num(), 0);
	TestEqual(TEXT("Yield still retained on day 1"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 2);
	TArray<FEclipseFacilityCompletion> Completions = TickConstructionDay(Base, Tuning);
	TestEqual(TEXT("Upgrade completes on day 2 (crewed)"), Completions.Num(), 1);
	if (Completions.Num() == 1)
	{
		TestEqual(TEXT("Completion is L2 — Upgraded, not Built"), Completions[0].NewLevel, 2);
		TestEqual(TEXT("Completion names the slot"), Completions[0].SlotId, SlotId);
		TestEqual(TEXT("Released staff captured before the reset (R6/d)"), Completions[0].ReleasedSoldierIds.Num(), 1);
		TestTrue(TEXT("Released id is the assigned soldier"), Completions[0].ReleasedSoldierIds.Contains(Analyst));
	}
	TestEqual(TEXT("Site operational at L2"), Site.Level, 2);
	TestEqual(TEXT("Staff released on completion"), Site.AssignedSoldierIds.Num(), 0);
	TestEqual(TEXT("Post-upgrade: L2 yield, no analyst (released)"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseCrewTimingRuleTest,
	"Eclipse.Base.CrewTimingDataRule",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseCrewTimingRuleTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	// Review R6/c, decided semantics (see TickConstructionDay header): a crew
	// present on the finishing day earns the full -1, even when assigned
	// last-minute. The coarse 1-day clock makes that the whole contract; the
	// optimizer still pays one soldier-day of undeployability per day saved,
	// and accumulating crew-days would push hidden fractional state into the
	// save schema. This test pins the rule so a future change is a decision.
	const FEclipseBaseTuningParams Tuning = EclipseBaseTest::MakeTuning();
	FEclipseFacilityLevelData FourDays;
	FourDays.BuildDays = 4;

	// Control: continuously crewed from day 1 completes on day 3 (4 - 1).
	int32 ControlCompletionDay = 0;
	{
		FEclipseBaseState Base;
		FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), FourDays);
		Site.AssignedSoldierIds.Add(FGuid(1, 1, 1, 1));
		for (int32 Day = 1; Day <= 4 && ControlCompletionDay == 0; ++Day)
		{
			if (TickConstructionDay(Base, Tuning).Num() > 0)
			{
				ControlCompletionDay = Day;
			}
		}
		TestEqual(TEXT("Continuous crew: completes day 3"), ControlCompletionDay, 3);
	}

	// Last-minute crew: uncrewed days 1-2, assigned before day 3 — same day 3.
	{
		FEclipseBaseState Base;
		FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), FourDays);
		TestEqual(TEXT("Day 1 uncrewed: building"), TickConstructionDay(Base, Tuning).Num(), 0);
		TestEqual(TEXT("Day 2 uncrewed: building"), TickConstructionDay(Base, Tuning).Num(), 0);
		Site.AssignedSoldierIds.Add(FGuid(2, 2, 2, 2));
		TArray<FEclipseFacilityCompletion> Completions = TickConstructionDay(Base, Tuning);
		TestEqual(TEXT("Last-minute crew: completes day 3 — the full -1 (data rule)"), Completions.Num(), 1);
		if (Completions.Num() == 1)
		{
			TestEqual(TEXT("Crew released"), Completions[0].ReleasedSoldierIds.Num(), 1);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseWrapperOrderFlowTest,
	"Eclipse.Base.WrapperOrderFlow",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseWrapperOrderFlowTest::RunTest(const FString& Parameters)
{
	// The step-3 wrapper end-to-end against in-memory data: validated orders
	// become atomic campaign transactions (GDD 14.3.3), the ledger stays the
	// hard funds gate, and staffing respects the DA_BaseTuning cap.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseBaseSubsystem* BaseSubsystem = GameInstance->GetSubsystem<UEclipseBaseSubsystem>();
	if (!TestNotNull(TEXT("Base subsystem exists"), BaseSubsystem))
	{
		GameInstance->Shutdown();
		return false;
	}

	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = EclipseBaseTest::MakeSlots();
	UEclipseBaseTuningAsset* Tuning = NewObject<UEclipseBaseTuningAsset>();
	Tuning->AnalystBonusResource = EclipseTags::Resource_Intel.GetTag();

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingRosterSize = 2;
	Setup->StartingResources.Add(EclipseTags::Resource_Materials.GetTag(), 300);
	Setup->StartingResources.Add(EclipseTags::Resource_Credits.GetTag(), 200);
	Setup->BaseLayout = Layout;
	Setup->BaseTuning = Tuning;
	Setup->Facilities = EclipseBaseTest::MakeFacilitiesTable();
	Campaign->StartNewCampaign(Setup);

	FString Error;

	// Build order by facility id: the wrapper finds the authored slot (B),
	// spends 120 M and starts the 2-day build in one commit.
	TestTrue(TEXT("Barracks order accepted"), BaseSubsystem->TryStartConstruction(TEXT("Barracks"), Error));
	const FEclipseFacilityState* BarracksSite = Campaign->GetState().BaseState.FindBySlot(TEXT("Slot_B"));
	TestNotNull(TEXT("Barracks state exists at Slot_B"), BarracksSite);
	if (BarracksSite != nullptr)
	{
		TestEqual(TEXT("2 build days queued"), BarracksSite->DaysRemaining, 2);
	}
	TestEqual(TEXT("Materials spent (300 - 120)"), Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag()), 180);

	// Double order on the building slot rejects; an unaffordable IC rejects on
	// the funds pre-check (the P1-03 rejection reused).
	TestFalse(TEXT("Second Barracks order rejects"), BaseSubsystem->TryStartConstruction(TEXT("Barracks"), Error));
	TestTrue(TEXT("Error says under construction"), Error.Contains(TEXT("under construction")));
	TestFalse(TEXT("IC at 180 M rejects (needs 240)"), BaseSubsystem->TryStartConstruction(TEXT("IntelligenceCenter"), Error));
	TestTrue(TEXT("Error names materials"), Error.Contains(TEXT("insufficient materials")));

	// Rush: 60 C x 2 days = 120 C, completes in that same commit.
	// (Re-fetch after every commit: state pointers are reads, not handles.)
	TestTrue(TEXT("Rush accepted"), BaseSubsystem->TryRushConstruction(TEXT("Slot_B"), Error));
	BarracksSite = Campaign->GetState().BaseState.FindBySlot(TEXT("Slot_B"));
	if (BarracksSite != nullptr)
	{
		TestEqual(TEXT("Barracks operational at L1"), BarracksSite->Level, 1);
		TestEqual(TEXT("No days remaining"), BarracksSite->DaysRemaining, 0);
	}
	TestEqual(TEXT("Credits spent (200 - 120)"), Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()), 80);

	// Staffing: cap = MaxCrewPerSite (1), one job per person, unassign works.
	const FGuid SoldierA = Campaign->GetState().Roster[0].SoldierId;
	const FGuid SoldierB = Campaign->GetState().Roster[1].SoldierId;
	TestTrue(TEXT("Assign staff to Slot_B"), BaseSubsystem->TryAssignStaff(TEXT("Slot_B"), SoldierA, Error));
	BarracksSite = Campaign->GetState().BaseState.FindBySlot(TEXT("Slot_B"));
	if (BarracksSite != nullptr)
	{
		TestEqual(TEXT("One soldier assigned"), BarracksSite->AssignedSoldierIds.Num(), 1);
	}
	TestFalse(TEXT("Second assignment rejects (cap 1)"), BaseSubsystem->TryAssignStaff(TEXT("Slot_B"), SoldierB, Error));
	TestTrue(TEXT("Error says fully staffed"), Error.Contains(TEXT("fully staffed")));
	TestFalse(TEXT("Empty slot rejects staff"), BaseSubsystem->TryAssignStaff(TEXT("Slot_C"), SoldierB, Error));
	TestTrue(TEXT("Unassign works"), BaseSubsystem->TryUnassignStaff(TEXT("Slot_B"), SoldierA, Error));
	TestFalse(TEXT("Double unassign rejects"), BaseSubsystem->TryUnassignStaff(TEXT("Slot_B"), SoldierA, Error));

	GameInstance->Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseFacilityYieldTest,
	"Eclipse.Base.FacilityYieldsAndAnalystBonus",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseFacilityYieldTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	UDataTable* Facilities = EclipseBaseTest::MakeFacilitiesTable();
	const FEclipseBaseTuningParams Tuning = EclipseBaseTest::MakeTuning();
	const FGameplayTag Intel = EclipseTags::Resource_Intel.GetTag();
	const FEclipseFacilityLevelData* IntelL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("IntelligenceCenter")), 1);
	const auto Resolver = [Facilities](FName FacilityId) { return EclipseBaseTest::FindRow(Facilities, FacilityId); };

	FEclipseBaseState Base;

	// The fresh base (CC only) yields nothing.
	TestEqual(TEXT("Command Center alone yields nothing"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.Num(), 0);

	// Under construction: no yield yet (facilities add capability when built).
	FEclipseFacilityState& Site = StartConstruction(Base, TEXT("Slot_D"), TEXT("IntelligenceCenter"), *IntelL1);
	TestEqual(TEXT("Building IC yields nothing"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.Num(), 0);

	// Operational IC: +2 Intel/day from data (locked decision 1).
	ApplyRush(Site);
	TestEqual(TEXT("Operational IC yields +2 Intel"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 2);

	// Staffed analyst: +1 (DA_BaseTuning), capped at MaxCrewPerSite even if over-assigned.
	Site.AssignedSoldierIds.Add(FGuid(1, 2, 3, 4));
	TestEqual(TEXT("Analyst adds +1 Intel"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 3);
	Site.AssignedSoldierIds.Add(FGuid(5, 6, 7, 8));
	TestEqual(TEXT("Staff cap holds: still +3 with two assigned"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 3);

	// No bonus resource configured: the bonus disappears, the base yield stays (graceful - GDD 14.3.5).
	FEclipseBaseTuningParams NoBonus = Tuning;
	NoBonus.AnalystBonusResource = FGameplayTag();
	TestEqual(TEXT("Unconfigured bonus tag: base yield only"), ComputeFacilityYields(Base, NoBonus, Resolver).YieldPerDay.FindRef(Intel), 2);

	// Analysts only boost facilities that yield the resource themselves: a staffed
	// Barracks never produces Intel (the IC rule lives in data, not in a branch).
	const FEclipseFacilityLevelData* BarracksL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("Barracks")), 1);
	FEclipseFacilityState& BarracksSite = StartConstruction(Base, TEXT("Slot_B"), TEXT("Barracks"), *BarracksL1);
	ApplyRush(BarracksSite);
	BarracksSite.AssignedSoldierIds.Add(FGuid(9, 9, 9, 9));
	TestEqual(TEXT("Staffed Barracks adds no Intel"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 3);

	// Unknown facility id in state (data removed after a save): skipped, never a crash.
	FEclipseFacilityState& Orphan = Base.Facilities.AddDefaulted_GetRef();
	Orphan.SlotId = TEXT("Slot_C");
	Orphan.FacilityId = TEXT("Medbay"); // the rejected variant - no row in the slice table
	Orphan.Level = 1;
	TestEqual(TEXT("Orphaned facility is skipped gracefully"), ComputeFacilityYields(Base, Tuning, Resolver).YieldPerDay.FindRef(Intel), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseCasualtyReleasesPostTest,
	"Eclipse.Base.CasualtyReleasesBasePost",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseCasualtyReleasesPostTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	// Step-3 review finding 1 (the ghost-analyst yield gap): KillSoldier and
	// WoundSoldier strip the casualty from AssignedSoldierIds inside the same
	// apply, so a dead analyst can never keep earning the IC bonus and the
	// vacated post is immediately re-staffable. Headless through the real
	// transaction API - the muster board and yields read the state this commits.
	UDataTable* Facilities = EclipseBaseTest::MakeFacilitiesTable();
	const FEclipseBaseTuningParams Tuning = EclipseBaseTest::MakeTuning();
	const FGameplayTag Intel = EclipseTags::Resource_Intel.GetTag();
	const auto Resolver = [Facilities](FName FacilityId) { return EclipseBaseTest::FindRow(Facilities, FacilityId); };

	FEclipseCampaignState State;
	FEclipseSoldierRecord& Analyst = State.Roster.AddDefaulted_GetRef();
	Analyst.SoldierId = FGuid(3, 0, 0, 1);
	Analyst.Name = TEXT("Analyst Reyes");
	FEclipseSoldierRecord& Crew = State.Roster.AddDefaulted_GetRef();
	Crew.SoldierId = FGuid(3, 0, 0, 2);
	Crew.Name = TEXT("Crew Dalen");
	// Guid value copies: the roster array may reallocate on any later add —
	// the facility-array lesson below applies to the roster references too.
	const FGuid AnalystId = Analyst.SoldierId;
	const FGuid CrewId = Crew.SoldierId;

	// Operational IC with the analyst on post; Barracks under construction with
	// the crew on site. (Re-fetch by slot after every structural change - the
	// facility array may reallocate.)
	const FEclipseFacilityLevelData* IntelL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("IntelligenceCenter")), 1);
	ApplyRush(StartConstruction(State.BaseState, TEXT("Slot_D"), TEXT("IntelligenceCenter"), *IntelL1));
	const FEclipseFacilityLevelData* BarracksL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("Barracks")), 1);
	StartConstruction(State.BaseState, TEXT("Slot_B"), TEXT("Barracks"), *BarracksL1);
	State.BaseState.FindBySlot(TEXT("Slot_D"))->AssignedSoldierIds.Add(AnalystId);
	State.BaseState.FindBySlot(TEXT("Slot_B"))->AssignedSoldierIds.Add(CrewId);

	TestEqual(TEXT("Live analyst: 2 + 1 Intel"), ComputeFacilityYields(State.BaseState, Tuning, Resolver).YieldPerDay.FindRef(Intel), 3);

	// KIA: the commit strips the analyst and records the release for the
	// StaffAssigned(none) fact.
	FEclipseCampaignTransaction Kill;
	Kill.Source = TEXT("Test");
	FEclipseCampaignMutation& KillMutation = Kill.Mutations.AddDefaulted_GetRef();
	KillMutation.Type = EEclipseCampaignMutationType::KillSoldier;
	KillMutation.SoldierId = AnalystId;
	KillMutation.Cause = TEXT("TestKIA");

	TArray<FEclipseAppliedMutation> Applied;
	FString Error;
	TestTrue(TEXT("Kill commits"), EclipseCampaignLogic::CommitTransaction(State, Kill, Applied, Error));
	TestEqual(TEXT("IC post vacated"), State.BaseState.FindBySlot(TEXT("Slot_D"))->AssignedSoldierIds.Num(), 0);
	TestEqual(TEXT("One release recorded"), Applied.Num() == 1 ? Applied[0].StaffReleases.Num() : -1, 1);
	if (Applied.Num() == 1 && Applied[0].StaffReleases.Num() == 1)
	{
		TestEqual(TEXT("Release names the slot"), Applied[0].StaffReleases[0].SlotId, FName(TEXT("Slot_D")));
		TestEqual(TEXT("Release names the facility"), Applied[0].StaffReleases[0].FacilityId, FName(TEXT("IntelligenceCenter")));
	}

	// The gap this test exists for: a dead analyst yields no IC bonus.
	TestEqual(TEXT("Dead analyst earns nothing: back to +2"), ComputeFacilityYields(State.BaseState, Tuning, Resolver).YieldPerDay.FindRef(Intel), 2);

	// Wounded crew: same strip, same release record.
	FEclipseCampaignTransaction Wound;
	Wound.Source = TEXT("Test");
	FEclipseCampaignMutation& WoundMutation = Wound.Mutations.AddDefaulted_GetRef();
	WoundMutation.Type = EEclipseCampaignMutationType::WoundSoldier;
	WoundMutation.SoldierId = CrewId;
	WoundMutation.Cause = TEXT("TestShrapnel");
	WoundMutation.EtaDays = 5;

	TestTrue(TEXT("Wound commits"), EclipseCampaignLogic::CommitTransaction(State, Wound, Applied, Error));
	TestEqual(TEXT("Building site vacated"), State.BaseState.FindBySlot(TEXT("Slot_B"))->AssignedSoldierIds.Num(), 0);
	TestEqual(TEXT("Wound release recorded"), Applied.Num() == 1 ? Applied[0].StaffReleases.Num() : -1, 1);

	// An unassigned casualty releases nothing (no spurious facts).
	FEclipseSoldierRecord& Bystander = State.Roster.AddDefaulted_GetRef();
	Bystander.SoldierId = FGuid(3, 0, 0, 3);
	Bystander.Name = TEXT("Bystander Voss");
	FEclipseCampaignTransaction KillBystander;
	KillBystander.Source = TEXT("Test");
	FEclipseCampaignMutation& KillBystanderMutation = KillBystander.Mutations.AddDefaulted_GetRef();
	KillBystanderMutation.Type = EEclipseCampaignMutationType::KillSoldier;
	KillBystanderMutation.SoldierId = Bystander.SoldierId;
	TestTrue(TEXT("Unassigned kill commits"), EclipseCampaignLogic::CommitTransaction(State, KillBystander, Applied, Error));
	TestEqual(TEXT("No release recorded"), Applied.Num() == 1 ? Applied[0].StaffReleases.Num() : -1, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseStaffCapMutationLayerTest,
	"Eclipse.Base.StaffCapInMutationLayer",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseStaffCapMutationLayerTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	// Step-3 review finding 3: the staff cap holds in the mutation layer itself
	// (MaxStaffPerSite rides the mutation; default = the SPEC-P2-03 spec value 1,
	// stamped from DA_BaseTuning by the subsystem), not only in the wrapper's
	// pre-check. An over-cap transaction rejects as a full no-op.
	UDataTable* Facilities = EclipseBaseTest::MakeFacilitiesTable();

	FEclipseCampaignState State;
	FEclipseSoldierRecord& SoldierA = State.Roster.AddDefaulted_GetRef();
	SoldierA.SoldierId = FGuid(4, 0, 0, 1);
	SoldierA.Name = TEXT("First Pick");
	FEclipseSoldierRecord& SoldierB = State.Roster.AddDefaulted_GetRef();
	SoldierB.SoldierId = FGuid(4, 0, 0, 2);
	SoldierB.Name = TEXT("Second Pick");
	// Guid value copies — roster references dangle on a later reallocation.
	const FGuid SoldierAId = SoldierA.SoldierId;
	const FGuid SoldierBId = SoldierB.SoldierId;

	const FEclipseFacilityLevelData* BarracksL1 = GetLevelData(EclipseBaseTest::FindRow(Facilities, TEXT("Barracks")), 1);
	StartConstruction(State.BaseState, TEXT("Slot_B"), TEXT("Barracks"), *BarracksL1);

	const auto MakeAssign = [](const FGuid& SoldierId)
	{
		FEclipseCampaignMutation Mutation;
		Mutation.Type = EEclipseCampaignMutationType::AssignStaff;
		Mutation.SlotId = TEXT("Slot_B");
		Mutation.SoldierId = SoldierId;
		Mutation.StaffRoleTag = EclipseTags::Base_Staff_Crew.GetTag();
		return Mutation;
	};

	TArray<FEclipseAppliedMutation> Applied;
	FString Error;

	// First assignment fills the site to the spec cap of 1.
	FEclipseCampaignTransaction AssignFirst;
	AssignFirst.Source = TEXT("Test");
	AssignFirst.Mutations.Add(MakeAssign(SoldierAId));
	TestTrue(TEXT("First assignment commits"), EclipseCampaignLogic::CommitTransaction(State, AssignFirst, Applied, Error));
	TestEqual(TEXT("Site staffed to cap"), State.BaseState.FindBySlot(TEXT("Slot_B"))->AssignedSoldierIds.Num(), 1);

	// Second assignment: the mutation layer rejects, state untouched (no-op).
	const uint32 HashBefore = State.ComputeStateHash();
	FEclipseCampaignTransaction AssignSecond;
	AssignSecond.Source = TEXT("Test");
	AssignSecond.Mutations.Add(MakeAssign(SoldierBId));
	TestFalse(TEXT("Over-cap assignment rejects in the mutation layer"), EclipseCampaignLogic::CommitTransaction(State, AssignSecond, Applied, Error));
	TestTrue(TEXT("Error says fully staffed"), Error.Contains(TEXT("fully staffed")));
	TestEqual(TEXT("Staff count unchanged"), State.BaseState.FindBySlot(TEXT("Slot_B"))->AssignedSoldierIds.Num(), 1);
	TestEqual(TEXT("Rejection was a full no-op (hash)"), State.ComputeStateHash(), HashBefore);

	// Atomicity: two assigns in ONE transaction also reject whole - the second
	// mutation validates against the scratch state the first already filled.
	FEclipseCampaignState FreshState;
	FreshState.Roster = State.Roster;
	StartConstruction(FreshState.BaseState, TEXT("Slot_B"), TEXT("Barracks"), *BarracksL1);
	FEclipseCampaignTransaction AssignBoth;
	AssignBoth.Source = TEXT("Test");
	AssignBoth.Mutations.Add(MakeAssign(SoldierAId));
	AssignBoth.Mutations.Add(MakeAssign(SoldierBId));
	TestFalse(TEXT("Double assign in one transaction rejects atomically"), EclipseCampaignLogic::CommitTransaction(FreshState, AssignBoth, Applied, Error));
	TestEqual(TEXT("No staff landed at all"), FreshState.BaseState.FindBySlot(TEXT("Slot_B"))->AssignedSoldierIds.Num(), 0);

	// The cap is data: stamped at 2, the same second assignment passes.
	FEclipseCampaignMutation RaisedCap = MakeAssign(SoldierBId);
	RaisedCap.MaxStaffPerSite = 2;
	FEclipseCampaignTransaction AssignRaised;
	AssignRaised.Source = TEXT("Test");
	AssignRaised.Mutations.Add(RaisedCap);
	TestTrue(TEXT("Raised cap admits the second soldier (data, not code)"), EclipseCampaignLogic::CommitTransaction(State, AssignRaised, Applied, Error));
	TestEqual(TEXT("Two staffed at cap 2"), State.BaseState.FindBySlot(TEXT("Slot_B"))->AssignedSoldierIds.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseCompletionYieldDayTest,
	"Eclipse.Base.CompletionYieldsOnCompletionDay",
	EclipseBaseTest::TestFlags)

bool FEclipseBaseCompletionYieldDayTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseLogic;

	// Step-3 review finding 4, pinned (the nightly soak's econ paths lean on
	// it): a facility completing on day N yields on day N. Mechanism under
	// test: the AdvanceDay apply ticks construction inside the commit, and the
	// economy tick runs downstream of the DayAdvanced fact, resolving facility
	// yields from the post-commit state (EclipseEconomySubsystem::
	// ResolveTickParams documents the seam). If completion ever slid to the
	// tick after the yield resolution, the IC's first day of income - which the
	// Intel-opening path prices into its build days - would arrive a day late.
	UDataTable* Facilities = EclipseBaseTest::MakeFacilitiesTable();
	const FEclipseBaseTuningParams Tuning = EclipseBaseTest::MakeTuning();
	const FGameplayTag Intel = EclipseTags::Resource_Intel.GetTag();
	const auto Resolver = [Facilities](FName FacilityId) { return EclipseBaseTest::FindRow(Facilities, FacilityId); };

	FEclipseCampaignState State;
	State.Day = 11;
	StartConstruction(State.BaseState, TEXT("Slot_D"), TEXT("IntelligenceCenter"), /*BuildDays*/ 1);

	// Day 11, still building: the pre-commit state yields nothing.
	TestEqual(TEXT("Building IC yields nothing pre-completion"), ComputeFacilityYields(State.BaseState, Tuning, Resolver).YieldPerDay.Num(), 0);

	FEclipseCampaignTransaction Advance;
	Advance.Source = TEXT("Test");
	Advance.Mutations.AddDefaulted_GetRef().Type = EEclipseCampaignMutationType::AdvanceDay;

	TArray<FEclipseAppliedMutation> Applied;
	FString Error;
	TestTrue(TEXT("AdvanceDay commits"), EclipseCampaignLogic::CommitTransaction(State, Advance, Applied, Error));
	TestEqual(TEXT("Clock reads day 12"), State.Day, 12);
	TestEqual(TEXT("Completion happened inside the day-12 commit"), Applied.Num() == 1 ? Applied[0].FacilityCompletions.Num() : -1, 1);
	if (Applied.Num() == 1 && Applied[0].FacilityCompletions.Num() == 1)
	{
		TestEqual(TEXT("Completed to L1"), Applied[0].FacilityCompletions[0].NewLevel, 1);
		TestEqual(TEXT("Completion day is the advanced day (day N)"), Applied[0].DayAfter, 12);
	}

	// What the economy tick sees on day 12 (it runs after this commit): the IC
	// is operational and yields TODAY.
	const FEclipseFacilityYieldParams DayNYields = ComputeFacilityYields(State.BaseState, Tuning, Resolver);
	TestEqual(TEXT("Day-N completion yields on day N (+2 Intel)"), DayNYields.YieldPerDay.FindRef(Intel), 2);

	// And the composed day tick carries it as the transparent FacilityYield
	// ledger line (GDD 7.6) - the full seam, headless.
	FEclipseEconomyTickParams TickParams;
	TickParams.CreditsTag = EclipseTags::Resource_Credits.GetTag();
	TickParams.IntelTag = Intel;
	TickParams.FacilityYields = DayNYields;
	FEclipseCampaignTransaction Tick;
	TestTrue(TEXT("Day tick has work"), EclipseEconomyLogic::BuildDayTick(State, TickParams, Tick));
	int32 FacilityIntel = 0;
	for (const FEclipseCampaignMutation& Mutation : Tick.Mutations)
	{
		if (Mutation.Type == EEclipseCampaignMutationType::AdjustResource && Mutation.ResourceType == Intel && Mutation.Reason == FName(TEXT("FacilityYield")))
		{
			FacilityIntel += Mutation.Amount;
		}
	}
	TestEqual(TEXT("FacilityYield ledger line lands the same day"), FacilityIntel, 2);
	return true;
}

// ---------------------------------------------------------------------------
// The walkable-vault parity Gauntlet (SPEC-P2-03 step 4-5; GDD 5.4, 12.4, 14.3.5)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultStateParityTest,
	"Eclipse.Base.VaultStateParity",
	EclipseBaseTest::TestFlags)

bool FEclipseVaultStateParityTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;

	// The Gauntlet's core claim (SPEC-P2-03 Integration, the gate in front of the
	// P1-08 menu-hub retirement): what the vault PHYSICALLY places equals what
	// FEclipseBaseState says - per slot, field for field. The menu hub and
	// Eclipse.Base.Report derive their rows from exactly those state fields
	// (FacilityId / Level + DaysRemaining / AssignedSoldierIds), so state-vault
	// parity IS hub-vault parity: one truth, three surfaces.
	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = EclipseBaseTest::MakeSlots();
	const FEclipseBaseState Base = EclipseBaseTest::MakeGrowthSpreadState();

	EclipseBaseTest::FScopedTestWorld Scoped;
	if (!TestNotNull(TEXT("Headless test world"), Scoped.World))
	{
		return false;
	}

	BuildVault(*Scoped.World, Layout, Base);
	TestTrue(TEXT("The vault stands (anchor placed)"), IsVaultPresent(*Scoped.World));

	const TArray<FEclipseVaultSlotView> Views = ReadSlotMarkers(*Scoped.World);
	TestEqual(TEXT("Exactly one slot marker per layout slot"), Views.Num(), Layout->Slots.Num());

	for (const FEclipseBaseSlotDef& Slot : Layout->Slots)
	{
		const FEclipseVaultSlotView* View = EclipseBaseTest::FindView(Views, Slot.SlotId);
		if (!TestNotNull(FString::Printf(TEXT("Slot %s is physically placed"), *Slot.SlotId.ToString()), View))
		{
			continue;
		}

		// The state side of the comparison, derived from state alone.
		const FEclipseFacilityState* Facility = Base.FindBySlot(Slot.SlotId);
		const FName StateFacility = Facility != nullptr ? Facility->FacilityId : FName(NAME_None);
		const int32 StateLevel = Facility != nullptr ? Facility->Level : 0;
		const int32 StateStaff = Facility != nullptr ? Facility->AssignedSoldierIds.Num() : 0;
		const TCHAR* StateVisual =
			Facility == nullptr ? TEXT("Empty")
			: (Facility->DaysRemaining > 0 ? TEXT("Constructing")
			: (Facility->Level >= 1 ? TEXT("Built") : TEXT("Empty")));

		const FString Where = Slot.SlotId.ToString();
		TestEqual(*FString::Printf(TEXT("%s: placed facility == state facility"), *Where), View->FacilityId, StateFacility);
		TestEqual(*FString::Printf(TEXT("%s: placed level == state level"), *Where), View->Level, StateLevel);
		TestEqual(*FString::Printf(TEXT("%s: placed staff == state staff"), *Where), View->StaffCount, StateStaff);
		TestEqual(*FString::Printf(TEXT("%s: placed read == state read"), *Where), VisualToString(View->Visual), StateVisual);
		TestFalse(*FString::Printf(TEXT("%s: a layout slot is never an orphan"), *Where), View->bOrphan);
	}

	// The reads pinned literally against the SPEC-P2-03 visible-growth table, so
	// a change in the derivation above cannot move both sides of the comparison
	// at once and still pass.
	const FEclipseVaultSlotView* SlotA = EclipseBaseTest::FindView(Views, EclipseBaseDefaults::CommandSlotId);
	const FEclipseVaultSlotView* SlotB = EclipseBaseTest::FindView(Views, TEXT("Slot_B"));
	const FEclipseVaultSlotView* SlotC = EclipseBaseTest::FindView(Views, TEXT("Slot_C"));
	const FEclipseVaultSlotView* SlotD = EclipseBaseTest::FindView(Views, TEXT("Slot_D"));
	if (SlotA != nullptr && SlotB != nullptr && SlotC != nullptr && SlotD != nullptr)
	{
		TestEqual(TEXT("Slot A: the pre-built Command Center reads Built"), VisualToString(SlotA->Visual), TEXT("Built"));
		TestEqual(TEXT("Slot A: at L1"), SlotA->Level, 1);
		TestEqual(TEXT("Slot B: a building site reads Constructing, not Built"), VisualToString(SlotB->Visual), TEXT("Constructing"));
		TestEqual(TEXT("Slot B: no level completed yet"), SlotB->Level, 0);
		TestEqual(TEXT("Slot B: the crew on site is placed"), SlotB->StaffCount, 1);
		TestEqual(TEXT("Slot C: the Workshop reads Built"), VisualToString(SlotC->Visual), TEXT("Built"));
		TestEqual(TEXT("Slot C: at L2 - the only slice upgrade"), SlotC->Level, 2);
		TestEqual(TEXT("Slot D: an untouched slot reads Empty"), VisualToString(SlotD->Visual), TEXT("Empty"));
		TestEqual(TEXT("Slot D: no facility"), SlotD->FacilityId, FName(NAME_None));
	}

	// Double-ended parity: the pure decision (PlanSlots) and the physical result
	// agree too, so a divergence can only come from the plan, never from the
	// placement code drifting away from it.
	const TArray<FEclipseVaultSlotPlan> Plans = PlanSlots(Layout->Slots, Base);
	TestEqual(TEXT("One marker per planned slot"), Views.Num(), Plans.Num());
	for (const FEclipseVaultSlotPlan& Plan : Plans)
	{
		const FEclipseVaultSlotView* View = EclipseBaseTest::FindView(Views, Plan.SlotId);
		if (View == nullptr)
		{
			AddError(FString::Printf(TEXT("Planned slot %s was never placed"), *Plan.SlotId.ToString()));
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s: plan facility == placed facility"), *Plan.SlotId.ToString()), View->FacilityId, Plan.FacilityId);
		TestEqual(*FString::Printf(TEXT("%s: plan level == placed level"), *Plan.SlotId.ToString()), View->Level, Plan.Level);
		TestEqual(*FString::Printf(TEXT("%s: plan read == placed read"), *Plan.SlotId.ToString()), VisualToString(View->Visual), VisualToString(Plan.Visual));
	}

	// Over-staffed state (a raised DA_BaseTuning cap, or Phase 3 roles): the
	// idler geometry caps at four chunky stand-ins, but the marker still reports
	// the TRUE count - parity is about the state, not about how many figures fit.
	FEclipseBaseState Crowded = Base;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Crowded.FindBySlot(TEXT("Slot_C"))->AssignedSoldierIds.Add(FGuid(6, 0, 0, Index));
	}
	RebuildVault(*Scoped.World, Layout, Crowded);
	const TArray<FEclipseVaultSlotView> CrowdedViews = ReadSlotMarkers(*Scoped.World);
	const FEclipseVaultSlotView* CrowdedView = EclipseBaseTest::FindView(CrowdedViews, TEXT("Slot_C"));
	if (TestNotNull(TEXT("Slot C still placed after the re-render"), CrowdedView))
	{
		TestEqual(TEXT("Marker reports the true staff count, capped geometry or not"), CrowdedView->StaffCount, 7);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultDeterminismTest,
	"Eclipse.Base.VaultBuildDeterminism",
	EclipseBaseTest::TestFlags)

bool FEclipseVaultDeterminismTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;

	// Determinism is what makes the vault reviewable in diffs and re-renderable
	// on every Event.Base.* fact without visual drift (the same regime as the
	// district graybox): two builds on one state must produce an identical
	// content signature - every marker tag + location and every mesh instance
	// transform. Actor names are engine counters and stay out of it by design.
	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = EclipseBaseTest::MakeSlots();
	const FEclipseBaseState Base = EclipseBaseTest::MakeGrowthSpreadState();

	EclipseBaseTest::FScopedTestWorld Scoped;
	if (!TestNotNull(TEXT("Headless test world"), Scoped.World))
	{
		return false;
	}

	BuildVault(*Scoped.World, Layout, Base);
	const TArray<FString> First = ComputeVaultSignature(*Scoped.World);
	TestTrue(TEXT("The signature has real content"), First.Num() > 40);

	// Instanced geometry must be part of the signature (GDD 12.4 says the shell
	// is instanced; if the ISM components ever stopped being reachable from the
	// anchor, the signature would silently degrade to marker-only).
	int32 InstanceLines = 0;
	for (const FString& Line : First)
	{
		if (Line.StartsWith(TEXT("ISM ")))
		{
			++InstanceLines;
		}
	}
	TestTrue(TEXT("Instanced mesh transforms are signed too"), InstanceLines > 30);

	RebuildVault(*Scoped.World, Layout, Base);
	const TArray<FString> Second = ComputeVaultSignature(*Scoped.World);
	TestEqual(TEXT("A rebuild signs the same number of items"), Second.Num(), First.Num());

	int32 FirstMismatch = INDEX_NONE;
	for (int32 Index = 0; Index < FMath::Min(First.Num(), Second.Num()); ++Index)
	{
		if (First[Index] != Second[Index])
		{
			FirstMismatch = Index;
			AddError(FString::Printf(TEXT("Vault build is not deterministic at line %d: '%s' vs '%s'"),
				Index, *First[Index], *Second[Index]));
			break;
		}
	}
	TestEqual(TEXT("Two builds on one state are identical"), FirstMismatch, INDEX_NONE);

	// And the signature is not vacuous: one completed build step changes it.
	FEclipseBaseState Advanced = Base;
	EclipseBaseLogic::ApplyRush(*Advanced.FindBySlot(TEXT("Slot_B"))); // Barracks completes
	RebuildVault(*Scoped.World, Layout, Advanced);
	TestFalse(TEXT("A completed facility changes the signature"), ComputeVaultSignature(*Scoped.World) == Second);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultMissingLayoutTest,
	"Eclipse.Base.VaultDegradesWithoutLayout",
	EclipseBaseTest::TestFlags)

bool FEclipseVaultMissingLayoutTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;

	// GDD 14.3.5 on the presentation layer: a missing DA_BaseLayout is a loud
	// warning plus an empty vault (anchor only) - never a crash, never a silent
	// pass. Both the build and the re-render path degrade the same way, and
	// neither stacks a second anchor.
	AddExpectedMessage(TEXT("no DA_BaseLayout linked"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

	const FEclipseBaseState Base = EclipseBaseTest::MakeGrowthSpreadState();

	EclipseBaseTest::FScopedTestWorld Scoped;
	if (!TestNotNull(TEXT("Headless test world"), Scoped.World))
	{
		return false;
	}

	BuildVault(*Scoped.World, /*Layout*/ nullptr, Base);
	TestTrue(TEXT("The anchor still stands (nothing crashed)"), IsVaultPresent(*Scoped.World));
	TestEqual(TEXT("No slot chamber is placed"), ReadSlotMarkers(*Scoped.World).Num(), 0);
	TestEqual(TEXT("The empty vault signs exactly one item: the anchor"), ComputeVaultSignature(*Scoped.World).Num(), 1);

	RebuildVault(*Scoped.World, /*Layout*/ nullptr, Base);
	TestTrue(TEXT("The re-render path degrades identically"), IsVaultPresent(*Scoped.World));
	TestEqual(TEXT("Still exactly one anchor - no stacking"), ComputeVaultSignature(*Scoped.World).Num(), 1);

	// The empty vault is not amnesia: the pure planner still accounts for every
	// state row (as orphans) and Eclipse.Base.Report keeps printing them. Loud
	// degradation means "show less", never "lie".
	const TArray<FEclipseBaseSlotDef> NoSlots;
	const TArray<FEclipseVaultSlotPlan> LayoutlessPlans = PlanSlots(NoSlots, Base);
	TestEqual(TEXT("Every state row is still accounted for"), LayoutlessPlans.Num(), Base.Facilities.Num());
	for (const FEclipseVaultSlotPlan& Plan : LayoutlessPlans)
	{
		TestTrue(*FString::Printf(TEXT("%s is tagged as an orphan without a layout"), *Plan.SlotId.ToString()), Plan.bOrphan);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultOrphanRowTest,
	"Eclipse.Base.VaultShowsOrphanStateRow",
	EclipseBaseTest::TestFlags)

bool FEclipseVaultOrphanRowTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;

	// A stale save (or a trimmed layout) can hold a facility at a slot the layout
	// no longer declares. The Eclipse.Base.Report rule applies to the vault: such
	// a row is SHOWN and tagged as an orphan, never silently hidden - hidden
	// state is the bug class that makes a save look healthy while it is not.
	// The facility id here is the rejected Medbay variant, so this also covers an
	// unknown presentation binding: neutral blockout, no site marker, no crash.
	FEclipseBaseState Base; // CC L1 at Slot_A
	EclipseBaseLogic::StartConstruction(Base, TEXT("Slot_Ghost"), TEXT("Medbay"), /*BuildDays*/ 3);
	EclipseBaseLogic::ApplyRush(*Base.FindBySlot(TEXT("Slot_Ghost"))); // operational L1 in the save

	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = EclipseBaseTest::MakeSlots(); // four slots, none of them Slot_Ghost

	EclipseBaseTest::FScopedTestWorld Scoped;
	if (!TestNotNull(TEXT("Headless test world"), Scoped.World))
	{
		return false;
	}

	BuildVault(*Scoped.World, Layout, Base);
	const TArray<FEclipseVaultSlotView> Views = ReadSlotMarkers(*Scoped.World);
	TestEqual(TEXT("Layout slots plus the orphan are all placed"), Views.Num(), Layout->Slots.Num() + 1);

	const FEclipseVaultSlotView* Ghost = EclipseBaseTest::FindView(Views, TEXT("Slot_Ghost"));
	if (TestNotNull(TEXT("The orphaned state row is physically shown"), Ghost))
	{
		TestTrue(TEXT("It is tagged as an orphan"), Ghost->bOrphan);
		TestEqual(TEXT("It names its facility"), Ghost->FacilityId, FName(TEXT("Medbay")));
		TestEqual(TEXT("It keeps its level"), Ghost->Level, 1);
		TestEqual(TEXT("It reads Built like the state says"), VisualToString(Ghost->Visual), TEXT("Built"));
	}
	for (const FEclipseBaseSlotDef& Slot : Layout->Slots)
	{
		const FEclipseVaultSlotView* View = EclipseBaseTest::FindView(Views, Slot.SlotId);
		if (View != nullptr)
		{
			TestFalse(*FString::Printf(TEXT("%s is not tagged as an orphan"), *Slot.SlotId.ToString()), View->bOrphan);
		}
	}

	// The chamber sits inside the shell, not in the void: the orphan gets a real
	// place in the vault (the plan positions it off the Spine like any slot).
	const TArray<FEclipseVaultSlotPlan> Plans = PlanSlots(Layout->Slots, Base);
	const FEclipseVaultSlotPlan* GhostPlan = Plans.FindByPredicate([](const FEclipseVaultSlotPlan& Plan) { return Plan.SlotId == FName(TEXT("Slot_Ghost")); });
	if (TestNotNull(TEXT("The orphan is planned, not appended nowhere"), GhostPlan))
	{
		TestTrue(TEXT("The orphan chamber is the last one (authored slots keep their order)"), Plans.Last().SlotId == GhostPlan->SlotId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultPlanHashTest,
	"Eclipse.Base.VaultPlanHashCoalescer",
	EclipseBaseTest::TestFlags)

bool FEclipseVaultPlanHashTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;

	// The re-render coalescer's contract (UEclipseBaseSubsystem::RefreshVault):
	// one campaign commit emits up to four Event.Base.* facts against the SAME
	// post-commit state, so the first fact renders the final vault and the rest
	// must hash identically and cost nothing. Equally important: every parity
	// field has to move the hash, or a real change would be swallowed.
	const TArray<FEclipseBaseSlotDef> Slots = EclipseBaseTest::MakeSlots();
	const FEclipseBaseState Base = EclipseBaseTest::MakeGrowthSpreadState();
	const uint32 Hash = ComputePlanHash(PlanSlots(Slots, Base));

	TestTrue(TEXT("Planning is pure: the same state hashes the same twice"),
		ComputePlanHash(PlanSlots(Slots, Base)) == Hash);

	FEclipseBaseState Staffed = Base;
	Staffed.FindBySlot(EclipseBaseDefaults::CommandSlotId)->AssignedSoldierIds.Add(FGuid(9, 0, 0, 1));
	TestFalse(TEXT("A staff change moves the hash (idlers are a visible read)"),
		ComputePlanHash(PlanSlots(Slots, Staffed)) == Hash);

	FEclipseBaseState Completed = Base;
	EclipseBaseLogic::ApplyRush(*Completed.FindBySlot(TEXT("Slot_B"))); // Constructing -> Built
	TestFalse(TEXT("A completed build moves the hash"),
		ComputePlanHash(PlanSlots(Slots, Completed)) == Hash);

	FEclipseBaseState Orphaned = Base;
	EclipseBaseLogic::StartConstruction(Orphaned, TEXT("Slot_Ghost"), TEXT("Workshop"), /*BuildDays*/ 2);
	TestFalse(TEXT("An appearing orphan row moves the hash"),
		ComputePlanHash(PlanSlots(Slots, Orphaned)) == Hash);

	TArray<FEclipseBaseSlotDef> Trimmed = Slots;
	Trimmed.Pop();
	TestFalse(TEXT("A trimmed layout moves the hash (chamber count and centering)"),
		ComputePlanHash(PlanSlots(Trimmed, Base)) == Hash);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultEventRerenderTest,
	"Eclipse.Base.VaultRerendersOnBaseFacts",
	EclipseBaseTest::TestFlags)

bool FEclipseVaultEventRerenderTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;

	// The wiring, end to end and headless: nobody calls the builder after the
	// first render. Orders go through the subsystem API, the CampaignState commit
	// emits the existing Event.Base.* facts, and the standing vault picks them up
	// off the bus (GDD 14.3.3 both ways: orders down the API, facts up the bus).
	// No tick, no polling - the assertions below run without a single frame.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseBaseSubsystem* BaseSubsystem = GameInstance->GetSubsystem<UEclipseBaseSubsystem>();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Base subsystem exists"), BaseSubsystem) || !TestNotNull(TEXT("The game instance has a world"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = EclipseBaseTest::MakeSlots();
	UEclipseBaseTuningAsset* Tuning = NewObject<UEclipseBaseTuningAsset>();
	Tuning->AnalystBonusResource = EclipseTags::Resource_Intel.GetTag();

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingRosterSize = 2;
	Setup->StartingResources.Add(EclipseTags::Resource_Materials.GetTag(), 300);
	Setup->StartingResources.Add(EclipseTags::Resource_Credits.GetTag(), 200);
	Setup->BaseLayout = Layout;
	Setup->BaseTuning = Tuning;
	Setup->Facilities = EclipseBaseTest::MakeFacilitiesTable();
	Campaign->StartNewCampaign(Setup);

	// Snapshot read of one slot marker: the marker array is a fresh snapshot per
	// call, so this hands back a value - never a pointer into a temporary.
	FEclipseVaultSlotView SlotB;
	const auto ReadSlot = [World, &SlotB](FName SlotId) -> bool
	{
		const TArray<FEclipseVaultSlotView> Snapshot = ReadSlotMarkers(*World);
		if (const FEclipseVaultSlotView* Found = EclipseBaseTest::FindView(Snapshot, SlotId))
		{
			SlotB = *Found;
			return true;
		}
		return false;
	};

	// A Base fact before anything stands is a strict no-op: the bus is never an
	// invitation to spawn a vault into a mission or menu world.
	FString Error;
	TestTrue(TEXT("A build order commits without a vault present"), BaseSubsystem->TryStartConstruction(TEXT("Barracks"), Error));
	TestFalse(TEXT("No vault was spawned by the fact alone"), IsVaultPresent(*World));

	// The level/entry seam renders it once; from here the bus keeps it current.
	BaseSubsystem->EnsureVaultPresent();
	TestTrue(TEXT("The vault stands after the entry render"), IsVaultPresent(*World));
	if (TestTrue(TEXT("Slot_B is placed"), ReadSlot(TEXT("Slot_B"))))
	{
		TestEqual(TEXT("It renders the in-flight order: Constructing"), VisualToString(SlotB.Visual), TEXT("Constructing"));
		TestEqual(TEXT("No level completed yet"), SlotB.Level, 0);
	}

	// Event.Base.StaffAssigned: the crew appears in the chamber, nothing else moved.
	const FGuid SoldierA = Campaign->GetState().Roster[0].SoldierId;
	TestTrue(TEXT("Crew assigned to the building site"), BaseSubsystem->TryAssignStaff(TEXT("Slot_B"), SoldierA, Error));
	if (TestTrue(TEXT("Slot_B still placed"), ReadSlot(TEXT("Slot_B"))))
	{
		TestEqual(TEXT("The staff fact re-rendered the crew idler"), SlotB.StaffCount, 1);
		TestEqual(TEXT("Still under construction"), VisualToString(SlotB.Visual), TEXT("Constructing"));
	}

	// Event.Base.FacilityBuilt (+ the completion's StaffAssigned release, two
	// facts from ONE commit): the chamber flips to Built and the crew is gone.
	TestTrue(TEXT("Rush commits"), BaseSubsystem->TryRushConstruction(TEXT("Slot_B"), Error));
	if (TestTrue(TEXT("Slot_B still placed"), ReadSlot(TEXT("Slot_B"))))
	{
		TestEqual(TEXT("The completion fact re-rendered the built facility"), VisualToString(SlotB.Visual), TEXT("Built"));
		TestEqual(TEXT("Operational at L1"), SlotB.Level, 1);
		TestEqual(TEXT("The released crew left the chamber (no ghost idler)"), SlotB.StaffCount, 0);
	}

	// Full-vault parity after the whole event chain: every marker still matches
	// the committed campaign state - the re-renders never drifted from it.
	const FEclipseBaseState& CommittedState = Campaign->GetState().BaseState;
	const TArray<FEclipseVaultSlotView> Views = ReadSlotMarkers(*World);
	TestEqual(TEXT("One marker per layout slot after the chain"), Views.Num(), Layout->Slots.Num());
	for (const FEclipseVaultSlotPlan& Plan : PlanSlots(Layout->Slots, CommittedState))
	{
		const FEclipseVaultSlotView* View = EclipseBaseTest::FindView(Views, Plan.SlotId);
		if (View == nullptr)
		{
			AddError(FString::Printf(TEXT("Slot %s vanished from the vault during the event chain"), *Plan.SlotId.ToString()));
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s: still parity on facility"), *Plan.SlotId.ToString()), View->FacilityId, Plan.FacilityId);
		TestEqual(*FString::Printf(TEXT("%s: still parity on level"), *Plan.SlotId.ToString()), View->Level, Plan.Level);
		TestEqual(*FString::Printf(TEXT("%s: still parity on staff"), *Plan.SlotId.ToString()), View->StaffCount, Plan.StaffCount);
		TestEqual(*FString::Printf(TEXT("%s: still parity on the read"), *Plan.SlotId.ToString()), VisualToString(View->Visual), VisualToString(Plan.Visual));
	}

	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
