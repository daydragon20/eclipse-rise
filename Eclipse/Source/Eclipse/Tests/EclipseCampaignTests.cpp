// Unit + save-integrity tests for SPEC-P1-02 (GDD 14.4: unit layer + save
// round-trip per merge). Fixtures run against fully initialized GameInstances
// so subsystem wiring (bus, save provider registration) is the real path.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "EclipseSaveSubsystem.h"
#include "EclipseSaveSystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"

namespace EclipseCampaignTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	struct FFixture
	{
		UGameInstance* GameInstance = nullptr;
		UEclipseCampaignSubsystem* Campaign = nullptr;
		UEclipseEventBusSubsystem* Bus = nullptr;
		UEclipseSaveSubsystem* Save = nullptr;

		static FFixture Make()
		{
			FFixture Fixture;
			Fixture.GameInstance = NewObject<UGameInstance>(GEngine);
			Fixture.GameInstance->InitializeStandalone();
			Fixture.Campaign = Fixture.GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
			Fixture.Bus = Fixture.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
			Fixture.Save = Fixture.GameInstance->GetSubsystem<UEclipseSaveSubsystem>();
			return Fixture;
		}

		void Shutdown()
		{
			if (GameInstance != nullptr)
			{
				GameInstance->Shutdown();
				GameInstance = nullptr;
			}
		}
	};

	FGameplayTag CreditsTag()
	{
		return EclipseTags::Resource_Credits.GetTag();
	}

	FEclipseCampaignMutation MakeAdjustResource(const FGameplayTag& Tag, int32 Amount, FName Reason)
	{
		FEclipseCampaignMutation Mutation;
		Mutation.Type = EEclipseCampaignMutationType::AdjustResource;
		Mutation.ResourceType = Tag;
		Mutation.Amount = Amount;
		Mutation.Reason = Reason;
		return Mutation;
	}

	FEclipseCampaignMutation MakeAddSoldier(const FGuid& Id, const FString& Name)
	{
		FEclipseCampaignMutation Mutation;
		Mutation.Type = EEclipseCampaignMutationType::AddSoldier;
		Mutation.SoldierRecord.SoldierId = Id;
		Mutation.SoldierRecord.Name = Name;
		Mutation.SoldierRecord.OriginId = TEXT("Kessara");
		return Mutation;
	}

	/** Scripted sequence used by the determinism and round-trip tests (SPEC-P1-02 DoD). */
	bool RunScriptedSequence(UEclipseCampaignSubsystem& Campaign, FString& OutError)
	{
		const FGameplayTag Resource = CreditsTag();
		const FGuid SoldierA(1, 2, 3, 4);
		const FGuid SoldierB(5, 6, 7, 8);

		FEclipseCampaignTransaction Setup;
		Setup.Source = TEXT("TestSetup");
		Setup.Mutations.Add(MakeAdjustResource(Resource, 100, TEXT("TestGrant")));
		Setup.Mutations.Add(MakeAddSoldier(SoldierA, TEXT("Vara Chen")));
		Setup.Mutations.Add(MakeAddSoldier(SoldierB, TEXT("Oscar Line")));
		if (!Campaign.CommitTransaction(Setup, OutError))
		{
			return false;
		}

		FEclipseCampaignTransaction Loop;
		Loop.Source = TEXT("TestLoop");

		FEclipseCampaignMutation Kill;
		Kill.Type = EEclipseCampaignMutationType::KillSoldier;
		Kill.SoldierId = SoldierA;
		Kill.Cause = TEXT("TestExplosion");
		Loop.Mutations.Add(Kill);

		FEclipseCampaignMutation Memorial;
		Memorial.Type = EEclipseCampaignMutationType::AddMemorialEntry;
		Memorial.MemorialEntry.SoldierId = SoldierA;
		Memorial.MemorialEntry.Name = TEXT("Vara Chen");
		Memorial.MemorialEntry.Cause = TEXT("TestExplosion");
		Memorial.MemorialEntry.Day = 0;
		Loop.Mutations.Add(Memorial);

		FEclipseCampaignMutation Production;
		Production.Type = EEclipseCampaignMutationType::QueueProduction;
		Production.ProductionItemId = TEXT("Item_RiflePlatform");
		Production.EtaDays = 2;
		Loop.Mutations.Add(Production);

		FEclipseCampaignMutation Advance;
		Advance.Type = EEclipseCampaignMutationType::AdvanceDay;
		Loop.Mutations.Add(Advance);
		Loop.Mutations.Add(MakeAdjustResource(Resource, -40, TEXT("TestSpend")));

		return Campaign.CommitTransaction(Loop, OutError);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignAtomicityTest,
	"Eclipse.Strategy.Campaign.TransactionAtomicity",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignAtomicityTest::RunTest(const FString& Parameters)
{
	EclipseCampaignTest::FFixture Fixture = EclipseCampaignTest::FFixture::Make();
	if (!TestNotNull(TEXT("Campaign subsystem exists"), Fixture.Campaign))
	{
		return false;
	}

	const FGameplayTag Resource = EclipseCampaignTest::CreditsTag();
	FString Error;

	FEclipseCampaignTransaction Grant;
	Grant.Source = TEXT("Test");
	Grant.Mutations.Add(EclipseCampaignTest::MakeAdjustResource(Resource, 50, TEXT("Seed")));
	TestTrue(TEXT("Seed grant commits"), Fixture.Campaign->CommitTransaction(Grant, Error));

	const uint32 HashBefore = Fixture.Campaign->GetState().ComputeStateHash();

	int32 EventCount = 0;
	FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct&) { ++EventCount; }));

	// Second mutation overdraws: the whole transaction must reject, first
	// mutation included (SPEC-P1-02 atomicity).
	FEclipseCampaignTransaction Mixed;
	Mixed.Source = TEXT("Test");
	Mixed.Mutations.Add(EclipseCampaignTest::MakeAdjustResource(Resource, -30, TEXT("Valid")));
	Mixed.Mutations.Add(EclipseCampaignTest::MakeAdjustResource(Resource, -40, TEXT("Overdraw")));

	TestFalse(TEXT("Mixed transaction rejected"), Fixture.Campaign->CommitTransaction(Mixed, Error));
	TestTrue(TEXT("Rejection names the offending mutation"), Error.Contains(TEXT("Mutation 1")));
	TestEqual(TEXT("State untouched after rejection"), Fixture.Campaign->GetState().ComputeStateHash(), HashBefore);
	TestEqual(TEXT("Balance unchanged"), Fixture.Campaign->GetState().GetBalance(Resource), 50);
	TestEqual(TEXT("No events leaked from a rejected transaction"), EventCount, 0);

	Fixture.Bus->Unsubscribe(Handle);
	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignDeterministicHashTest,
	"Eclipse.Strategy.Campaign.DeterministicStateHash",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignDeterministicHashTest::RunTest(const FString& Parameters)
{
	EclipseCampaignTest::FFixture FixtureA = EclipseCampaignTest::FFixture::Make();
	EclipseCampaignTest::FFixture FixtureB = EclipseCampaignTest::FFixture::Make();

	FString Error;
	TestTrue(TEXT("Sequence A commits"), EclipseCampaignTest::RunScriptedSequence(*FixtureA.Campaign, Error));
	TestTrue(TEXT("Sequence B commits"), EclipseCampaignTest::RunScriptedSequence(*FixtureB.Campaign, Error));

	TestEqual(TEXT("Identical sequences produce identical hashes"),
		FixtureA.Campaign->GetState().ComputeStateHash(),
		FixtureB.Campaign->GetState().ComputeStateHash());

	FEclipseCampaignTransaction Extra;
	Extra.Source = TEXT("Test");
	FEclipseCampaignMutation Advance;
	Advance.Type = EEclipseCampaignMutationType::AdvanceDay;
	Extra.Mutations.Add(Advance);
	TestTrue(TEXT("Extra mutation commits"), FixtureB.Campaign->CommitTransaction(Extra, Error));

	TestNotEqual(TEXT("Diverged states produce different hashes"),
		FixtureA.Campaign->GetState().ComputeStateHash(),
		FixtureB.Campaign->GetState().ComputeStateHash());

	FixtureA.Shutdown();
	FixtureB.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignCommitEventsTest,
	"Eclipse.Strategy.Campaign.CommitEmitsEvents",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignCommitEventsTest::RunTest(const FString& Parameters)
{
	EclipseCampaignTest::FFixture Fixture = EclipseCampaignTest::FFixture::Make();

	TArray<FGameplayTag> Received;
	const FGameplayTag EventRoot = FGameplayTag::RequestGameplayTag(TEXT("Event"));
	FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
		EventRoot,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag Tag, const FInstancedStruct&) { Received.Add(Tag); }));

	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Fixture.Campaign, Error));

	auto CountOf = [&Received](const FGameplayTag& Tag)
	{
		int32 Count = 0;
		for (const FGameplayTag& ReceivedTag : Received)
		{
			if (ReceivedTag == Tag)
			{
				++Count;
			}
		}
		return Count;
	};

	TestEqual(TEXT("ResourcesChanged emitted per adjust"), CountOf(EclipseTags::Event_Economy_ResourcesChanged), 2);
	TestEqual(TEXT("SoldierAdded emitted per recruit"), CountOf(EclipseTags::Event_Roster_SoldierAdded), 2);
	TestEqual(TEXT("SoldierDied emitted once"), CountOf(EclipseTags::Event_Roster_SoldierDied), 1);
	TestEqual(TEXT("Memorial entry emitted once"), CountOf(EclipseTags::Event_Memorial_EntryAdded), 1);
	TestEqual(TEXT("ProductionQueued emitted once"), CountOf(EclipseTags::Event_Economy_ProductionQueued), 1);
	TestEqual(TEXT("DayAdvanced emitted once"), CountOf(EclipseTags::Event_Campaign_DayAdvanced), 1);

	Fixture.Bus->Unsubscribe(Handle);
	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignSaveRoundTripTest,
	"Eclipse.Strategy.Campaign.SaveRoundTrip",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignSaveRoundTripTest::RunTest(const FString& Parameters)
{
	const FString SlotName = TEXT("AutomationRoundTrip");
	IFileManager::Get().Delete(*UEclipseSaveSubsystem::GetSlotFilePath(SlotName), false, true, true);

	EclipseCampaignTest::FFixture Source = EclipseCampaignTest::FFixture::Make();

	// Fixture includes regions (via a transient graph asset) and a death — the
	// dead-stay-dead assertion is the SPEC-P1-07 CI fixture requirement, wired
	// in from day one.
	UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
	FEclipseRegionDefinition Region;
	Region.RegionId = TEXT("Region_Foundry");
	Region.StartingOwner = EEclipseRegionOwner::Dominion;
	Region.StartingUnrest = 25;
	Region.StartingGarrison = 3;
	Graph->Regions.Add(Region);

	UEclipseCampaignSetupAsset* SetupAsset = NewObject<UEclipseCampaignSetupAsset>();
	SetupAsset->StartingDay = 1;
	SetupAsset->StartingRosterSize = 2;
	SetupAsset->RegionGraph = Graph;
	Source.Campaign->StartNewCampaign(SetupAsset);

	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Source.Campaign, Error));

	FEclipseCampaignTransaction Flip;
	Flip.Source = TEXT("Test");
	FEclipseCampaignMutation Owner;
	Owner.Type = EEclipseCampaignMutationType::SetRegionOwner;
	Owner.RegionId = TEXT("Region_Foundry");
	Owner.NewOwner = EEclipseRegionOwner::Player;
	Flip.Mutations.Add(Owner);
	TestTrue(TEXT("Region flip commits"), Source.Campaign->CommitTransaction(Flip, Error));

	// A MANNED facility in the fixture (SPEC-P2-03 review R6/a): the staff-guid
	// serialization loops must be exercised by the hash-equality assert below —
	// an unmanned base would leave them dead code in this round trip.
	FEclipseCampaignTransaction BaseOrders;
	BaseOrders.Source = TEXT("Test");
	FEclipseCampaignMutation Build;
	Build.Type = EEclipseCampaignMutationType::StartConstruction;
	Build.SlotId = TEXT("Slot_D");
	Build.FacilityId = TEXT("IntelligenceCenter");
	Build.EtaDays = 4;
	BaseOrders.Mutations.Add(Build);
	FEclipseCampaignMutation Staff;
	Staff.Type = EEclipseCampaignMutationType::AssignStaff;
	Staff.SlotId = TEXT("Slot_D");
	Staff.SoldierId = FGuid(5, 6, 7, 8); // Oscar Line, alive and Available
	Staff.StaffRoleTag = EclipseTags::Base_Staff_Crew.GetTag();
	BaseOrders.Mutations.Add(Staff);
	TestTrue(TEXT("Base build + staff commits"), Source.Campaign->CommitTransaction(BaseOrders, Error));

	const uint32 SourceHash = Source.Campaign->GetState().ComputeStateHash();
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("Load succeeds"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("Round-tripped state hash matches"), Target.Campaign->GetState().ComputeStateHash(), SourceHash);
	TestEqual(TEXT("No migration steps for a current-version file"), Target.Save->GetLastLoadMigrationStepCount(), 0);

	const FEclipseSoldierRecord* DeadSoldier = Target.Campaign->GetState().FindSoldier(FGuid(1, 2, 3, 4));
	TestNotNull(TEXT("Dead soldier still on the roster record"), DeadSoldier);
	if (DeadSoldier != nullptr)
	{
		TestTrue(TEXT("Dead soldier stays dead across save/load"), DeadSoldier->Status == EEclipseSoldierStatus::Dead);
	}
	TestEqual(TEXT("Memorial survives save/load"), Target.Campaign->GetState().Memorial.Num(), 1);

	// The manned facility survives byte-for-byte (R6/a): construction state and
	// the assigned soldier guid both round-trip.
	const FEclipseFacilityState* MannedSite = Target.Campaign->GetState().BaseState.FindBySlot(TEXT("Slot_D"));
	TestNotNull(TEXT("Manned facility survives save/load"), MannedSite);
	if (MannedSite != nullptr)
	{
		TestEqual(TEXT("Facility id round-trips"), MannedSite->FacilityId, FName(TEXT("IntelligenceCenter")));
		TestEqual(TEXT("Construction days round-trip"), MannedSite->DaysRemaining, 4);
		TestEqual(TEXT("Exactly one staff assignment"), MannedSite->AssignedSoldierIds.Num(), 1);
		TestTrue(TEXT("Staff guid round-trips"), MannedSite->AssignedSoldierIds.Contains(FGuid(5, 6, 7, 8)));
	}

	IFileManager::Get().Delete(*UEclipseSaveSubsystem::GetSlotFilePath(SlotName), false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignSaveMigrationTest,
	"Eclipse.Strategy.Campaign.SaveMigrationPipeline",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignSaveMigrationTest::RunTest(const FString& Parameters)
{
	const FString SlotName = TEXT("AutomationMigration");
	const FString SlotPath = UEclipseSaveSubsystem::GetSlotFilePath(SlotName);
	IFileManager::Get().Delete(*SlotPath, false, true, true);

	EclipseCampaignTest::FFixture Source = EclipseCampaignTest::FFixture::Make();
	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Source.Campaign, Error));
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	// Rewrite the header's schema version to 0: the whole chain (0->1 no-op,
	// 1->2 loadout-unlock append, 2->3 ClassId append, 3->4 base-tail append,
	// 4->5 story-tail append, 5->6 response-tier byte)
	// must run and the load must still succeed (SPEC-P1-02: migration scaffold
	// proven by CI, not by hope). The tail-appends are harmless on this
	// current-shaped block: readers stop at the original trailing counts and
	// ignore appended bytes.
	TArray<uint8> FileBytes;
	TestTrue(TEXT("Save file readable"), FFileHelper::LoadFileToArray(FileBytes, *SlotPath));
	const int32 VersionOffset = sizeof(uint32);
	*reinterpret_cast<int32*>(FileBytes.GetData() + VersionOffset) = 0;
	TestTrue(TEXT("Patched file written"), FFileHelper::SaveArrayToFile(FileBytes, *SlotPath));

	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("Load of v0 file succeeds via migration"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("All migration steps ran (0->1, 1->2, 2->3, 3->4, 4->5, 5->6, 6->7)"), Target.Save->GetLastLoadMigrationStepCount(), 7);
	TestEqual(TEXT("Migrated state matches source"),
		Target.Campaign->GetState().ComputeStateHash(),
		Source.Campaign->GetState().ComputeStateHash());

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignBaseCommitEventsTest,
	"Eclipse.Strategy.Campaign.BaseCommitEmitsEvents",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignBaseCommitEventsTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-03 events contract: all four Event.Base.* facts leave the
	// CampaignState commit and only the commit — build order, staffing (with
	// the positional role), day-tick completion incl. the staff release, and
	// rush completing in its own commit (spec lines 100-105).
	EclipseCampaignTest::FFixture Fixture = EclipseCampaignTest::FFixture::Make();

	TArray<TPair<FGameplayTag, FEclipseBaseEventPayload>> BaseEvents;
	const FGameplayTag BaseFamily = EclipseTags::Event_Base_FacilityBuilt.GetTag().RequestDirectParent();
	FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
		BaseFamily,
		FEclipseEventNativeDelegate::CreateLambda([&BaseEvents](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			if (const FEclipseBaseEventPayload* Base = Payload.GetPtr<FEclipseBaseEventPayload>())
			{
				BaseEvents.Emplace(Tag, *Base);
			}
		}));

	auto CountOf = [&BaseEvents](const FGameplayTag& Tag)
	{
		int32 Count = 0;
		for (const TPair<FGameplayTag, FEclipseBaseEventPayload>& Event : BaseEvents)
		{
			if (Event.Key == Tag)
			{
				++Count;
			}
		}
		return Count;
	};

	const FGuid Crew(11, 12, 13, 14);
	FString Error;

	// Seed: wallet + one Available soldier for staffing.
	FEclipseCampaignTransaction Seed;
	Seed.Source = TEXT("Test");
	Seed.Mutations.Add(EclipseCampaignTest::MakeAdjustResource(EclipseTags::Resource_Materials.GetTag(), 500, TEXT("Seed")));
	Seed.Mutations.Add(EclipseCampaignTest::MakeAdjustResource(EclipseTags::Resource_Credits.GetTag(), 500, TEXT("Seed")));
	Seed.Mutations.Add(EclipseCampaignTest::MakeAddSoldier(Crew, TEXT("Mira Voss")));
	TestTrue(TEXT("Seed commits"), Fixture.Campaign->CommitTransaction(Seed, Error));
	TestEqual(TEXT("Seed emits no base events"), BaseEvents.Num(), 0);

	// Build order: spend + start atomically -> exactly one ConstructionStarted.
	FEclipseCampaignTransaction BuildOrder;
	BuildOrder.Source = TEXT("Test");
	BuildOrder.Mutations.Add(EclipseCampaignTest::MakeAdjustResource(EclipseTags::Resource_Materials.GetTag(), -240, TEXT("Build_IntelligenceCenter")));
	{
		FEclipseCampaignMutation& Start = BuildOrder.Mutations.AddDefaulted_GetRef();
		Start.Type = EEclipseCampaignMutationType::StartConstruction;
		Start.SlotId = TEXT("Slot_D");
		Start.FacilityId = TEXT("IntelligenceCenter");
		Start.EtaDays = 4;
	}
	TestTrue(TEXT("Build order commits"), Fixture.Campaign->CommitTransaction(BuildOrder, Error));
	TestEqual(TEXT("ConstructionStarted emitted once"), CountOf(EclipseTags::Event_Base_ConstructionStarted), 1);
	if (BaseEvents.Num() == 1)
	{
		const FEclipseBaseEventPayload& Started = BaseEvents[0].Value;
		TestEqual(TEXT("Payload names the slot"), Started.SlotId, FName(TEXT("Slot_D")));
		TestEqual(TEXT("Payload names the facility"), Started.FacilityId, FName(TEXT("IntelligenceCenter")));
		TestEqual(TEXT("Target level is 1"), Started.Level, 1);
		TestEqual(TEXT("ETA = day 0 + 4 uncrewed days"), Started.EtaDay, 4);
	}

	// Staffing: the fact carries the positional role - a building site takes a crew.
	FEclipseCampaignTransaction StaffOrder;
	StaffOrder.Source = TEXT("Test");
	{
		FEclipseCampaignMutation& Assign = StaffOrder.Mutations.AddDefaulted_GetRef();
		Assign.Type = EEclipseCampaignMutationType::AssignStaff;
		Assign.SlotId = TEXT("Slot_D");
		Assign.SoldierId = Crew;
		Assign.StaffRoleTag = EclipseTags::Base_Staff_Crew.GetTag();
	}
	TestTrue(TEXT("Staff order commits"), Fixture.Campaign->CommitTransaction(StaffOrder, Error));
	TestEqual(TEXT("StaffAssigned emitted once"), CountOf(EclipseTags::Event_Base_StaffAssigned), 1);
	if (BaseEvents.Num() == 2)
	{
		TestEqual(TEXT("Assigned soldier in payload"), BaseEvents[1].Value.SoldierId, Crew);
		TestEqual(TEXT("Role is crew (positional: site under construction)"), BaseEvents[1].Value.RoleTag, EclipseTags::Base_Staff_Crew.GetTag());
	}

	// Day ticks: crewed 4-day build completes on day 3 - FacilityBuilt and the
	// staff release (StaffAssigned, empty role) leave that same AdvanceDay commit.
	auto AdvanceOneDay = [&Fixture, &Error]()
	{
		FEclipseCampaignTransaction Advance;
		Advance.Source = TEXT("Test");
		FEclipseCampaignMutation& Mutation = Advance.Mutations.AddDefaulted_GetRef();
		Mutation.Type = EEclipseCampaignMutationType::AdvanceDay;
		return Fixture.Campaign->CommitTransaction(Advance, Error);
	};
	TestTrue(TEXT("Day 1 commits"), AdvanceOneDay());
	TestTrue(TEXT("Day 2 commits"), AdvanceOneDay());
	TestEqual(TEXT("Nothing built after 2 days"), CountOf(EclipseTags::Event_Base_FacilityBuilt), 0);
	TestTrue(TEXT("Day 3 commits"), AdvanceOneDay());
	TestEqual(TEXT("FacilityBuilt emitted on the completing tick"), CountOf(EclipseTags::Event_Base_FacilityBuilt), 1);
	TestEqual(TEXT("Completion releases the crew (second StaffAssigned)"), CountOf(EclipseTags::Event_Base_StaffAssigned), 2);
	if (BaseEvents.Num() == 4)
	{
		TestEqual(TEXT("Built at L1"), BaseEvents[2].Value.Level, 1);
		TestEqual(TEXT("Release names the crew"), BaseEvents[3].Value.SoldierId, Crew);
		TestFalse(TEXT("Release role is empty (none = unassigned)"), BaseEvents[3].Value.RoleTag.IsValid());
	}
	const FEclipseFacilityState* Site = Fixture.Campaign->GetState().BaseState.FindBySlot(TEXT("Slot_D"));
	TestNotNull(TEXT("Site exists"), Site);
	if (Site != nullptr)
	{
		TestEqual(TEXT("Site operational at L1"), Site->Level, 1);
		TestEqual(TEXT("Crew released in state"), Site->AssignedSoldierIds.Num(), 0);
	}

	// Upgrade order + rush: FacilityUpgraded fires in the SAME commit as the
	// rush mutation (SPEC-P2-03 clock rules), never a tick later.
	FEclipseCampaignTransaction UpgradeOrder;
	UpgradeOrder.Source = TEXT("Test");
	{
		FEclipseCampaignMutation& Start = UpgradeOrder.Mutations.AddDefaulted_GetRef();
		Start.Type = EEclipseCampaignMutationType::StartConstruction;
		Start.SlotId = TEXT("Slot_D");
		Start.FacilityId = TEXT("IntelligenceCenter");
		Start.EtaDays = 2;
	}
	TestTrue(TEXT("Upgrade order commits"), Fixture.Campaign->CommitTransaction(UpgradeOrder, Error));
	TestEqual(TEXT("Second ConstructionStarted"), CountOf(EclipseTags::Event_Base_ConstructionStarted), 2);
	if (BaseEvents.Num() == 5)
	{
		TestEqual(TEXT("Upgrade order targets L2"), BaseEvents[4].Value.Level, 2);
		TestEqual(TEXT("Upgrade ETA = day 3 + 2"), BaseEvents[4].Value.EtaDay, 5);
	}

	FEclipseCampaignTransaction RushOrder;
	RushOrder.Source = TEXT("Test");
	RushOrder.Mutations.Add(EclipseCampaignTest::MakeAdjustResource(EclipseTags::Resource_Credits.GetTag(), -120, TEXT("Rush_IntelligenceCenter")));
	{
		FEclipseCampaignMutation& Rush = RushOrder.Mutations.AddDefaulted_GetRef();
		Rush.Type = EEclipseCampaignMutationType::RushConstruction;
		Rush.SlotId = TEXT("Slot_D");
	}
	TestTrue(TEXT("Rush commits"), Fixture.Campaign->CommitTransaction(RushOrder, Error));
	TestEqual(TEXT("FacilityUpgraded in the rush commit itself"), CountOf(EclipseTags::Event_Base_FacilityUpgraded), 1);
	TestEqual(TEXT("Rush never reads as Built"), CountOf(EclipseTags::Event_Base_FacilityBuilt), 1);
	Site = Fixture.Campaign->GetState().BaseState.FindBySlot(TEXT("Slot_D"));
	if (Site != nullptr)
	{
		TestEqual(TEXT("Site operational at L2"), Site->Level, 2);
	}

	// Atomicity: an invalid base mutation rejects the whole transaction and
	// leaks no facts (the P1-02 contract holds for the new mutation types).
	const int32 EventsBefore = BaseEvents.Num();
	const uint32 HashBefore = Fixture.Campaign->GetState().ComputeStateHash();
	FEclipseCampaignTransaction BadRush;
	BadRush.Source = TEXT("Test");
	{
		FEclipseCampaignMutation& Rush = BadRush.Mutations.AddDefaulted_GetRef();
		Rush.Type = EEclipseCampaignMutationType::RushConstruction;
		Rush.SlotId = TEXT("Slot_D"); // operational: nothing to rush
	}
	TestFalse(TEXT("Rushing an operational site rejects"), Fixture.Campaign->CommitTransaction(BadRush, Error));
	TestEqual(TEXT("No events leaked from the rejected commit"), BaseEvents.Num(), EventsBefore);
	TestEqual(TEXT("State untouched"), Fixture.Campaign->GetState().ComputeStateHash(), HashBefore);

	Fixture.Bus->Unsubscribe(Handle);
	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignBaseStateMigrationTest,
	"Eclipse.Strategy.Campaign.SaveMigrationV3RecordsWithoutBaseState",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignBaseStateMigrationTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-03 / GDD 14.3.6: a byte-faithful v3 file (no base tail) must land
	// deterministically on the spec start state - Command Center pre-built at
	// L1, slots B-D unbuilt (locked decision 2) - via exactly the 3->4 step.
	const FString SlotName = TEXT("AutomationBaseMigration");
	const FString SlotPath = UEclipseSaveSubsystem::GetSlotFilePath(SlotName);
	IFileManager::Get().Delete(*SlotPath, false, true, true);

	EclipseCampaignTest::FFixture Source = EclipseCampaignTest::FFixture::Make();
	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Source.Campaign, Error));
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	// Reconstruct a v3 file from the v6 save: strip the trailing base tail
	// (count + per-facility: SlotId/FacilityId as ANSI FStrings, Level,
	// DaysRemaining, staff count + FGuids) from the Campaign block - the only
	// registered block, so it ends the file - then rewrite the file header
	// version, the block size, and the block's leading state version.
	// Layout (save container contract): [u32 Magic][i32 Ver][i32 Blocks]
	// [FString "Campaign" = i32 9 + 9 bytes][i64 BlockSize][block bytes...].
	TArray<uint8> FileBytes;
	TestTrue(TEXT("Save file readable"), FFileHelper::LoadFileToArray(FileBytes, *SlotPath));

	const int32 HeaderVersionOffset = sizeof(uint32);
	const int32 BlockSizeOffset = 12 + (4 + 9);
	const int32 BlockStartOffset = BlockSizeOffset + sizeof(int64);
	TestEqual(TEXT("Sanity: file header is v7"), *reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset), 7);
	TestEqual(TEXT("Sanity: block leads with state schema v7"), *reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset), 7);

	int32 BaseTailSize = sizeof(int32);
	for (const FEclipseFacilityState& Facility : Source.Campaign->GetState().BaseState.Facilities)
	{
		BaseTailSize += sizeof(int32) + Facility.SlotId.ToString().Len() + 1; // ANSI FString: len-with-null + bytes
		BaseTailSize += sizeof(int32) + Facility.FacilityId.ToString().Len() + 1;
		BaseTailSize += 3 * sizeof(int32); // Level, DaysRemaining, staff count
		BaseTailSize += Facility.AssignedSoldierIds.Num() * sizeof(FGuid);
	}
	// The v5 story tail sits after the base tail and must strip too — a
	// byte-faithful v3 file predates both (SPEC-P2-04).
	int32 StripSize = BaseTailSize + sizeof(int32);
	for (const FGameplayTag& Flag : Source.Campaign->GetState().StoryFlags)
	{
		StripSize += sizeof(int32) + Flag.GetTagName().ToString().Len() + 1;
	}
	StripSize += sizeof(uint8); // the v6 response-tier byte (GDD 9.4) — a v3 file predates it too
	// En de v7-schadestaart erachter (GDD 5.4): telling + een byte per faciliteit.
	StripSize += sizeof(int32) + Source.Campaign->GetState().BaseState.Facilities.Num() * sizeof(uint8);

	*reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset) = 3;
	*reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset) = 3;
	*reinterpret_cast<int64*>(FileBytes.GetData() + BlockSizeOffset) -= StripSize;
	FileBytes.SetNum(FileBytes.Num() - StripSize);
	TestTrue(TEXT("v3-shaped file written"), FFileHelper::SaveArrayToFile(FileBytes, *SlotPath));

	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("v3 file loads via migration"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("Exactly the 3->4, 4->5, 5->6 and 6->7 steps ran"), Target.Save->GetLastLoadMigrationStepCount(), 4);

	const FEclipseBaseState& Base = Target.Campaign->GetState().BaseState;
	TestEqual(TEXT("Spec start state: exactly one facility"), Base.Facilities.Num(), 1);
	const FEclipseFacilityState* CommandCenter = Base.FindBySlot(EclipseBaseDefaults::CommandSlotId);
	TestNotNull(TEXT("Command Center sits at slot A"), CommandCenter);
	if (CommandCenter != nullptr)
	{
		TestEqual(TEXT("Pre-built facility is the Command Center"), CommandCenter->FacilityId, EclipseBaseDefaults::CommandCenterFacilityId);
		TestEqual(TEXT("Command Center is L1 (free, pre-built - locked decision 2)"), CommandCenter->Level, 1);
		TestEqual(TEXT("Command Center is operational"), CommandCenter->DaysRemaining, 0);
		TestEqual(TEXT("No staff assigned at migration"), CommandCenter->AssignedSoldierIds.Num(), 0);
	}
	TestEqual(TEXT("Migrated state matches source (base included in hash)"),
		Target.Campaign->GetState().ComputeStateHash(),
		Source.Campaign->GetState().ComputeStateHash());

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignStoryFlagMigrationTest,
	"Eclipse.Strategy.Campaign.SaveMigrationV4RecordsWithoutStoryFlags",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignStoryFlagMigrationTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-04 / GDD 14.3.6 (third R6 exercise): a byte-faithful v4 file (no
	// story tail) must land on empty StoryFlags — the campaign simply hasn't
	// reached a beat yet — via exactly the 4->5 step.
	const FString SlotName = TEXT("AutomationStoryMigration");
	const FString SlotPath = UEclipseSaveSubsystem::GetSlotFilePath(SlotName);
	IFileManager::Get().Delete(*SlotPath, false, true, true);

	EclipseCampaignTest::FFixture Source = EclipseCampaignTest::FFixture::Make();
	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Source.Campaign, Error));
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	// Reconstruct a v4 file from the v6 save: strip the story tail (for a
	// beat-less campaign exactly one int32 count) AND the v6 tier byte behind it.
	// Same container math as the v3 reconstruction above.
	TArray<uint8> FileBytes;
	TestTrue(TEXT("Save file readable"), FFileHelper::LoadFileToArray(FileBytes, *SlotPath));

	const int32 HeaderVersionOffset = sizeof(uint32);
	const int32 BlockSizeOffset = 12 + (4 + 9);
	const int32 BlockStartOffset = BlockSizeOffset + sizeof(int64);
	TestEqual(TEXT("Sanity: file header is v7"), *reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset), 7);
	TestEqual(TEXT("Sanity: block leads with state schema v7"), *reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset), 7);
	TestEqual(TEXT("Sanity: the scripted campaign has no beats yet"), Source.Campaign->GetState().StoryFlags.Num(), 0);

	int32 StoryTailSize = sizeof(int32);
	for (const FGameplayTag& Flag : Source.Campaign->GetState().StoryFlags)
	{
		StoryTailSize += sizeof(int32) + Flag.GetTagName().ToString().Len() + 1; // ANSI FName-as-FString
	}
	StoryTailSize += sizeof(uint8); // and the v6 tier byte sitting behind it (GDD 9.4)
	// ...en de v7-schadestaart daar weer achter (GDD 5.4).
	StoryTailSize += sizeof(int32) + Source.Campaign->GetState().BaseState.Facilities.Num() * sizeof(uint8);

	*reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset) = 4;
	*reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset) = 4;
	*reinterpret_cast<int64*>(FileBytes.GetData() + BlockSizeOffset) -= StoryTailSize;
	FileBytes.SetNum(FileBytes.Num() - StoryTailSize);
	TestTrue(TEXT("v4-shaped file written"), FFileHelper::SaveArrayToFile(FileBytes, *SlotPath));

	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("v4 file loads via migration"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("Exactly the 4->5, 5->6 and 6->7 steps ran"), Target.Save->GetLastLoadMigrationStepCount(), 3);
	TestEqual(TEXT("Pre-story campaign lands on empty flags"), Target.Campaign->GetState().StoryFlags.Num(), 0);
	TestEqual(TEXT("Migrated state matches source (flags included in hash)"),
		Target.Campaign->GetState().ComputeStateHash(),
		Source.Campaign->GetState().ComputeStateHash());

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignResponseTierMigrationTest,
	"Eclipse.Strategy.Campaign.SaveMigrationV5RecordsWithoutResponseTier",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignResponseTierMigrationTest::RunTest(const FString& Parameters)
{
	// GDD 9.4 / 14.3.6, fourth exercise of the same ladder: a byte-faithful v5
	// file (no tier byte) must land on Indifference — a campaign that predates
	// the strategic opponent has not been noticed yet — via exactly the 5->6 step.
	const FString SlotName = TEXT("AutomationTierMigration");
	const FString SlotPath = UEclipseSaveSubsystem::GetSlotFilePath(SlotName);
	IFileManager::Get().Delete(*SlotPath, false, true, true);

	EclipseCampaignTest::FFixture Source = EclipseCampaignTest::FFixture::Make();
	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Source.Campaign, Error));

	// Escalate BEFORE saving, so the stripped byte is a non-zero one: a
	// migration test whose only expected value is 0 cannot tell "migrated to
	// zero" apart from "never read anything".
	FEclipseCampaignTransaction Escalate;
	Escalate.Source = TEXT("Test");
	{
		FEclipseCampaignMutation& Tier = Escalate.Mutations.AddDefaulted_GetRef();
		Tier.Type = EEclipseCampaignMutationType::SetResponseTier;
		Tier.ResponseTier = EEclipseDominionResponseTier::Insurgency;
		Tier.Reason = TEXT("Test_Escalation");
	}
	TestTrue(TEXT("Escalation commits"), Source.Campaign->CommitTransaction(Escalate, Error));
	TestEqual(TEXT("Sanity: the source campaign is at Insurgency"),
		Source.Campaign->GetState().ResponseTier, EEclipseDominionResponseTier::Insurgency);
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	// Reconstruct a v5 file from the v7 save: the tier byte sits behind the
	// story tail, and de v7-schadestaart weer achter de tierbyte. Same container
	// math as the v3/v4 reconstructions.
	TArray<uint8> FileBytes;
	TestTrue(TEXT("Save file readable"), FFileHelper::LoadFileToArray(FileBytes, *SlotPath));

	const int32 HeaderVersionOffset = sizeof(uint32);
	const int32 BlockSizeOffset = 12 + (4 + 9);
	const int32 BlockStartOffset = BlockSizeOffset + sizeof(int64);
	TestEqual(TEXT("Sanity: file header is v7"), *reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset), 7);
	TestEqual(TEXT("Sanity: block leads with state schema v7"), *reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset), 7);

	// De schadestaart (v7) staat ACHTER de tierbyte, dus die gaat er in dezelfde
	// knip af — een v5-bestand kent geen van beide.
	const int32 DamageTailSize = sizeof(int32)
		+ Source.Campaign->GetState().BaseState.Facilities.Num() * sizeof(uint8);
	const int32 TierTailSize = sizeof(uint8) + DamageTailSize;

	// DE TIERBYTE IS NIET MEER DE LAATSTE BYTE, en deze regel is de reden dat
	// die controle hier blijft staan in plaats van te verdwijnen: hij pint vast
	// WAAR de byte staat die we zo gaan wegknippen. Zou de staartvolgorde ooit
	// veranderen, dan zakt deze test op de sanity-regel in plaats van op een
	// raadselachtige migratie-uitkomst dertig regels verderop.
	TestEqual(TEXT("Sanity: the tier we committed sits just before the v7 damage tail"),
		static_cast<int32>(FileBytes[FileBytes.Num() - 1 - DamageTailSize]),
		static_cast<int32>(EEclipseDominionResponseTier::Insurgency));
	*reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset) = 5;
	*reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset) = 5;
	*reinterpret_cast<int64*>(FileBytes.GetData() + BlockSizeOffset) -= TierTailSize;
	FileBytes.SetNum(FileBytes.Num() - TierTailSize);
	TestTrue(TEXT("v5-shaped file written"), FFileHelper::SaveArrayToFile(FileBytes, *SlotPath));

	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("v5 file loads via migration"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("Exactly the 5->6 and 6->7 steps ran"), Target.Save->GetLastLoadMigrationStepCount(), 2);
	TestEqual(TEXT("A pre-tier campaign comes home unnoticed"),
		Target.Campaign->GetState().ResponseTier, EEclipseDominionResponseTier::Indifference);
	TestNotEqual(TEXT("...which is NOT what the source held — the byte really was stripped"),
		Target.Campaign->GetState().ResponseTier, Source.Campaign->GetState().ResponseTier);

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignDamageMigrationTest,
	"Eclipse.Strategy.Campaign.SaveMigrationV6RecordsWithoutFacilityDamage",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignDamageMigrationTest::RunTest(const FString& Parameters)
{
	// GDD 5.4 / 14.3.6 (vijfde R6-oefening): een byte-getrouw v6-bestand kent de
	// schadestaart niet en moet thuiskomen met ALLES HEEL — een campagne van
	// vóór het schadesysteem heeft nooit een aanval op de basis meegemaakt.
	//
	// EN DE CONTROLEPROEF ZIT ERIN: de broncampagne zet één faciliteit op
	// beschadigd en bewijst dat die vlag de save-ronde OVERLEEFT. Zonder die
	// helft bewijst "alles komt heel thuis" niets — dat zou ook waar zijn als de
	// vlag helemaal niet werd weggeschreven.
	const FString SlotName = TEXT("AutomationDamageMigration");
	const FString SlotPath = UEclipseSaveSubsystem::GetSlotFilePath(SlotName);
	IFileManager::Get().Delete(*SlotPath, false, true, true);

	EclipseCampaignTest::FFixture Source = EclipseCampaignTest::FFixture::Make();
	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Source.Campaign, Error));
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	const int32 FacilityCount = Source.Campaign->GetState().BaseState.Facilities.Num();
	TestTrue(TEXT("Fixture has at least one facility"), FacilityCount > 0);

	const int32 HeaderVersionOffset = sizeof(uint32);
	const int32 BlockSizeOffset = 12 + (4 + 9);
	const int32 BlockStartOffset = BlockSizeOffset + sizeof(int64);
	const int32 DamageTailSize = sizeof(int32) + FacilityCount * sizeof(uint8);

	TArray<uint8> Pristine;
	TestTrue(TEXT("Save file readable"), FFileHelper::LoadFileToArray(Pristine, *SlotPath));
	TestEqual(TEXT("Sanity: file header is v7"), *reinterpret_cast<int32*>(Pristine.GetData() + HeaderVersionOffset), 7);
	TestEqual(TEXT("Sanity: block leads with state schema v7"), *reinterpret_cast<int32*>(Pristine.GetData() + BlockStartOffset), 7);
	TestEqual(TEXT("Sanity: the tail counts exactly the facilities we have"),
		*reinterpret_cast<int32*>(Pristine.GetData() + Pristine.Num() - DamageTailSize), FacilityCount);

	// --- CONTROLEPROEF: de staart wordt ECHT gelezen ----------------------
	//
	// De schadevlag wordt vandaag door geen enkele mutatie gezet (het
	// aanvalssysteem is GDD 11 en niet van deze ronde), dus zetten we hem waar
	// hij leeft: in de bytes. Zonder deze helft zou de migratietest hieronder
	// even groen zijn als de staart nooit was gelezen — "alles komt heel thuis"
	// is een uitspraak over niets zolang niet vaststaat dat kapot ook aankomt.
	{
		TArray<uint8> Damaged = Pristine;
		Damaged.Last() = 1; // de vlag van de LAATSTE faciliteit
		TestTrue(TEXT("Control: damaged-shaped file written"), FFileHelper::SaveArrayToFile(Damaged, *SlotPath));

		EclipseCampaignTest::FFixture RoundTrip = EclipseCampaignTest::FFixture::Make();
		TestTrue(TEXT("Control: the v7 file loads"), RoundTrip.Save->LoadFromSlot(SlotName, Error));
		TestEqual(TEXT("Control: no migration step was needed"), RoundTrip.Save->GetLastLoadMigrationStepCount(), 0);

		const TArray<FEclipseFacilityState>& Back = RoundTrip.Campaign->GetState().BaseState.Facilities;
		TestEqual(TEXT("Control: same number of facilities"), Back.Num(), FacilityCount);
		if (Back.Num() > 0)
		{
			TestTrue(TEXT("Control: the damage flag really arrives"), Back.Last().bDamaged);
		}
		if (Back.Num() > 1)
		{
			// Alleen zinvol bij meer dan één faciliteit: met één post is "de
			// juiste" en "allemaal" hetzelfde ding, en dan meet deze regel niets.
			// (Eerste versie schreef dit als één expressie met `|| Num()==1`
			// erin en zakte precies daarop — de vacuüme helft maakte hem waar.)
			TestFalse(TEXT("Control: and it lands on the RIGHT facility, not all of them"), Back[0].bDamaged);
		}
		RoundTrip.Shutdown();
	}

	// --- CONTROLEPROEF 2: DE VERSIEPOORT KAN ROOD WORDEN -----------------
	//
	// `MigrateBlocks` loopt versie voor versie omhoog en WEIGERT te laden zodra
	// er voor een stap geen migratie geregistreerd staat. Dat is precies waarom
	// de v6->v7-registratie erbij hoort en niet optioneel is: zonder die entry
	// laadt géén enkele bestaande save meer.
	//
	// Die poort hier één keer rood maken is de reden dat het groen hieronder
	// iets betekent. Een bestand dat zegt v8 te zijn kan deze build niet kennen,
	// en dan hoort het te WEIGEREN met een leesbare fout in plaats van stil de
	// helft te lezen.
	{
		TArray<uint8> FromTheFuture = Pristine;
		*reinterpret_cast<int32*>(FromTheFuture.GetData() + HeaderVersionOffset) = 8;
		TestTrue(TEXT("Control: future-shaped file written"), FFileHelper::SaveArrayToFile(FromTheFuture, *SlotPath));

		EclipseCampaignTest::FFixture Future = EclipseCampaignTest::FFixture::Make();
		FString FutureError;
		TestFalse(TEXT("Control: a save newer than this build is REFUSED, not half-read"),
			Future.Save->LoadFromSlot(SlotName, FutureError));
		TestFalse(TEXT("Control: and the refusal says why"), FutureError.IsEmpty());
		Future.Shutdown();
	}

	// --- DE MUTATIE: knip de schadestaart eraf en noem het bestand v6 -----
	TArray<uint8> FileBytes = Pristine;
	FileBytes.Last() = 1; // dezelfde schade als in de controleproef...
	*reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset) = 6;
	*reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset) = 6;
	*reinterpret_cast<int64*>(FileBytes.GetData() + BlockSizeOffset) -= DamageTailSize;
	FileBytes.SetNum(FileBytes.Num() - DamageTailSize); // ...maar de staart gaat eraf
	TestTrue(TEXT("v6-shaped file written"), FFileHelper::SaveArrayToFile(FileBytes, *SlotPath));

	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("v6 file loads via migration"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("Exactly the 6->7 step ran"), Target.Save->GetLastLoadMigrationStepCount(), 1);

	const TArray<FEclipseFacilityState>& Migrated = Target.Campaign->GetState().BaseState.Facilities;
	TestEqual(TEXT("Every facility came home"), Migrated.Num(), FacilityCount);
	for (const FEclipseFacilityState& Facility : Migrated)
	{
		// EN DIT IS AANTOONBAAR NIET GRATIS: exact dezelfde byte stond in de
		// controleproef hierboven op 1 en kwam daar als schade aan.
		TestFalse(TEXT("A pre-damage campaign comes home intact"), Facility.bDamaged);
	}

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignResponseTierCommitTest,
	"Eclipse.Strategy.Campaign.ResponseTierEscalatesOnlyUpwardAndEmitsTheStep",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignResponseTierCommitTest::RunTest(const FString& Parameters)
{
	// GDD 9.4: the tier is committed state with one writer, it only climbs, and
	// the fact it emits carries the STEP so a diegetic reaction can pick a tone.
	EclipseCampaignTest::FFixture Fixture = EclipseCampaignTest::FFixture::Make();

	TArray<FEclipseResponseTierEventPayload> Facts;
	FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
		EclipseTags::Event_Strategy_ResponseTierChanged,
		FEclipseEventNativeDelegate::CreateLambda([&Facts](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseResponseTierEventPayload* Fact = Payload.GetPtr<FEclipseResponseTierEventPayload>())
			{
				Facts.Add(*Fact);
			}
		}));

	FString Error;
	auto Escalate = [&Fixture, &Error](EEclipseDominionResponseTier To, FName Reason)
	{
		FEclipseCampaignTransaction Transaction;
		Transaction.Source = TEXT("Test");
		FEclipseCampaignMutation& Tier = Transaction.Mutations.AddDefaulted_GetRef();
		Tier.Type = EEclipseCampaignMutationType::SetResponseTier;
		Tier.ResponseTier = To;
		Tier.Reason = Reason;
		return Fixture.Campaign->CommitTransaction(Transaction, Error);
	};

	TestEqual(TEXT("A new campaign starts beneath notice"),
		Fixture.Campaign->GetState().ResponseTier, EEclipseDominionResponseTier::Indifference);

	TestTrue(TEXT("Escalating to Insurgency commits"), Escalate(EEclipseDominionResponseTier::Insurgency, TEXT("RegionalAttack")));
	TestEqual(TEXT("Exactly one fact"), Facts.Num(), 1);
	if (Facts.Num() == 1)
	{
		TestEqual(TEXT("The fact carries where it came FROM"), Facts[0].OldTier, EEclipseDominionResponseTier::Indifference);
		TestEqual(TEXT("...and where it went"), Facts[0].NewTier, EEclipseDominionResponseTier::Insurgency);
		TestEqual(TEXT("...and why"), Facts[0].Reason, FName(TEXT("RegionalAttack")));
	}

	// Skipping rungs is legal — an authored beat may jump the ladder.
	TestTrue(TEXT("Jumping two rungs commits"), Escalate(EEclipseDominionResponseTier::Existential, TEXT("Act4")));
	TestEqual(TEXT("Two facts"), Facts.Num(), 2);
	if (Facts.Num() == 2)
	{
		TestEqual(TEXT("The step is 2 -> 5, not 4 -> 5"), Facts[1].OldTier, EEclipseDominionResponseTier::Insurgency);
	}

	// Down and sideways are both refused, and refused LOUDLY: a silent no-op
	// would let a caller believe the empire calmed down.
	const uint32 HashBefore = Fixture.Campaign->GetState().ComputeStateHash();
	TestFalse(TEXT("Lowering the tier is rejected"), Escalate(EEclipseDominionResponseTier::Nuisance, TEXT("Wishful")));
	TestTrue(TEXT("...with a reason that names the rule"), Error.Contains(TEXT("escalate")));
	TestFalse(TEXT("Re-committing the same tier is rejected"), Escalate(EEclipseDominionResponseTier::Existential, TEXT("Again")));
	TestEqual(TEXT("No facts leaked from the rejected commits"), Facts.Num(), 2);
	TestEqual(TEXT("State untouched"), Fixture.Campaign->GetState().ComputeStateHash(), HashBefore);

	// The tier is part of campaign identity: two campaigns that differ only in
	// temperature must not hash the same (it is what routing risk reads).
	FEclipseCampaignState Cool = Fixture.Campaign->GetState();
	Cool.ResponseTier = EEclipseDominionResponseTier::Indifference;
	TestNotEqual(TEXT("The tier is inside the state hash"), Cool.ComputeStateHash(), HashBefore);

	Fixture.Bus->Unsubscribe(Handle);
	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCampaignStoryFlagCommitTest,
	"Eclipse.Strategy.Campaign.StoryFlagCommitEmitsAndRefusesDuplicates",
	EclipseCampaignTest::TestFlags)

bool FEclipseCampaignStoryFlagCommitTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-04: beats are set-only facts — one commit sets the flag and emits
	// Event.Story.BeatReached exactly once; a duplicate reaching the API is a
	// caller bug the transaction rejects (composers filter via ShouldCommitBeat).
	const FString SlotName = TEXT("AutomationStoryCommit");
	IFileManager::Get().Delete(*UEclipseSaveSubsystem::GetSlotFilePath(SlotName), false, true, true);

	EclipseCampaignTest::FFixture Fixture = EclipseCampaignTest::FFixture::Make();
	FString Error;
	TestTrue(TEXT("Scripted sequence commits"), EclipseCampaignTest::RunScriptedSequence(*Fixture.Campaign, Error));

	// A registered tag stands in for the beat identity; the layer never
	// inspects families, only exact membership (same stand-in the story
	// sequencing tests use).
	const FGameplayTag Beat = EclipseTags::Class_Verb_Momentum.GetTag();

	int32 BeatEvents = 0;
	FGameplayTag LastBeat;
	UEclipseEventBusSubsystem* Bus = Fixture.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		EclipseTags::Event_Story_BeatReached,
		FEclipseEventNativeDelegate::CreateLambda([&BeatEvents, &LastBeat](FGameplayTag, const FInstancedStruct& Payload)
		{
			++BeatEvents;
			if (const FEclipseStoryEventPayload* Story = Payload.GetPtr<FEclipseStoryEventPayload>())
			{
				LastBeat = Story->BeatTag;
			}
		}));

	const uint32 HashBefore = Fixture.Campaign->GetState().ComputeStateHash();

	FEclipseCampaignTransaction SetBeat;
	SetBeat.Source = TEXT("Test");
	FEclipseCampaignMutation& Mutation = SetBeat.Mutations.AddDefaulted_GetRef();
	Mutation.Type = EEclipseCampaignMutationType::SetStoryFlag;
	Mutation.StoryFlagTag = Beat;
	TestTrue(TEXT("Beat commit accepted"), Fixture.Campaign->CommitTransaction(SetBeat, Error));
	TestTrue(TEXT("Flag set in state"), Fixture.Campaign->GetState().StoryFlags.Contains(Beat));
	TestEqual(TEXT("BeatReached emitted exactly once"), BeatEvents, 1);
	TestEqual(TEXT("Payload names the beat"), LastBeat, Beat);
	TestTrue(TEXT("Hash covers story flags"), Fixture.Campaign->GetState().ComputeStateHash() != HashBefore);

	TestFalse(TEXT("Duplicate beat rejected (set-only idempotence)"), Fixture.Campaign->CommitTransaction(SetBeat, Error));
	TestEqual(TEXT("Rejection emitted nothing"), BeatEvents, 1);

	// Two identical beats inside ONE transaction must also fail as a whole —
	// the scratch-copy validation sees the first mutation's write (review pin).
	const FGameplayTag SecondBeat = EclipseTags::Class_Verb_Stabilize.GetTag();
	FEclipseCampaignTransaction DoubleBeat;
	DoubleBeat.Source = TEXT("Test");
	for (int32 Index = 0; Index < 2; ++Index)
	{
		FEclipseCampaignMutation& Dup = DoubleBeat.Mutations.AddDefaulted_GetRef();
		Dup.Type = EEclipseCampaignMutationType::SetStoryFlag;
		Dup.StoryFlagTag = SecondBeat;
	}
	TestFalse(TEXT("Intra-transaction duplicate rejects atomically"), Fixture.Campaign->CommitTransaction(DoubleBeat, Error));
	TestFalse(TEXT("Atomic rejection committed neither"), Fixture.Campaign->GetState().StoryFlags.Contains(SecondBeat));
	TestEqual(TEXT("Atomic rejection emitted nothing"), BeatEvents, 1);

	// Round-trip: the flag survives save/load on the v5 tail, hash included.
	TestTrue(TEXT("Save succeeds"), Fixture.Save->SaveToSlot(SlotName, Error));
	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("Load succeeds"), Target.Save->LoadFromSlot(SlotName, Error));
	TestTrue(TEXT("Flag survives the round-trip"), Target.Campaign->GetState().StoryFlags.Contains(Beat));
	TestEqual(TEXT("Non-empty flags round-trip in the hash"),
		Target.Campaign->GetState().ComputeStateHash(),
		Fixture.Campaign->GetState().ComputeStateHash());

	Bus->Unsubscribe(Handle);
	IFileManager::Get().Delete(*UEclipseSaveSubsystem::GetSlotFilePath(SlotName), false, true, true);
	Fixture.Shutdown();
	Target.Shutdown();
	return true;
}


// Eclipse.Save.Report meldt een ontbrekende setup, want DAT is de lege kaart.
//
// Deze test bestaat omdat de regel die hij bewaakt in een gezonde run nooit
// gedraaid wordt: bij een normale start staat er "campagne-setup
// DA_CampaignSetup" en klaar. De tak die ertoe doet is de andere, en die zag ik
// op 27-07 pas nadat de kaart al leeg was.
//
// Beide richtingen, want een controle die altijd rood staat is net zo waardeloos
// als een die nooit rood staat.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSaveReportNamesTheMissingSetup,
	"Eclipse.Campaign.SaveReportNamesTheMissingSetup", EclipseCampaignTest::TestFlags)
bool FEclipseSaveReportNamesTheMissingSetup::RunTest(const FString&)
{
	FEclipseCampaignState State;
	State.Day = 4;

	// Zonder setup: het rapport moet het WOORD zeggen, niet een lege naam of een
	// nette nul. "campagne-setup " met niets erachter leest als "in orde".
	TArray<FString> Zonder;
	EclipseSaveReport::BuildStateLines(nullptr, State, Zonder);
	if (TestEqual(TEXT("rapport zonder setup: twee regels"), Zonder.Num(), 2))
	{
		TestTrue(TEXT("rapport zonder setup: noemt ONTBREEKT"), Zonder[0].Contains(TEXT("ONTBREEKT")));
		TestTrue(TEXT("rapport zonder setup: zegt ook WAT dat betekent"),
			Zonder[0].Contains(TEXT("bord blijft leeg")));
		TestTrue(TEXT("rapport zonder setup: de staat staat er nog steeds bij"),
			Zonder[1].Contains(TEXT("dag=4")));
	}

	// Met setup: geen vals alarm. Zonder deze helft zou een rapport dat ALTIJD
	// ONTBREEKT roept ook groen zijn.
	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>(GetTransientPackage(), TEXT("DA_TestSetup"));
	TArray<FString> Met;
	EclipseSaveReport::BuildStateLines(Setup, State, Met);
	if (TestEqual(TEXT("rapport met setup: twee regels"), Met.Num(), 2))
	{
		TestFalse(TEXT("rapport met setup: geen vals ONTBREEKT"), Met[0].Contains(TEXT("ONTBREEKT")));
		TestTrue(TEXT("rapport met setup: noemt het asset bij naam"), Met[0].Contains(TEXT("DA_TestSetup")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
