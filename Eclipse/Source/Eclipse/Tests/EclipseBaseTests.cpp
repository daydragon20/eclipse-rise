// Unit tests for SPEC-P2-03 steps 1-2 (GDD 14.4, blocking): build-order
// validation on the slot-graph (occupied slot, unknown facility, authored
// placement, insufficient funds - never a crash), the construction day-tick
// (multi-day, crew -1 with floor 1, staff release), rush (cost = 60 x remaining
// days, instant completion), facility yields incl. the IC analyst bonus, and
// the seeded spec start state (Command Center pre-built at L1). Pure logic:
// deterministic, no engine actors, tables built in-memory like EclipseClassTests.

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipseBaseLogic.h"
#include "Base/EclipseBaseTypes.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/DataTable.h"
#include "Misc/AutomationTest.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
