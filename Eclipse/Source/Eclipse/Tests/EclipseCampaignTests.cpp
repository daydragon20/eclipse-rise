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
	// 1->2 loadout-unlock append, 2->3 ClassId append) must run and the load
	// must still succeed (SPEC-P1-02: migration scaffold proven by CI, not by
	// hope). The tail-appends are harmless on this current-shaped block:
	// readers stop at the original trailing counts and ignore appended bytes.
	TArray<uint8> FileBytes;
	TestTrue(TEXT("Save file readable"), FFileHelper::LoadFileToArray(FileBytes, *SlotPath));
	const int32 VersionOffset = sizeof(uint32);
	*reinterpret_cast<int32*>(FileBytes.GetData() + VersionOffset) = 0;
	TestTrue(TEXT("Patched file written"), FFileHelper::SaveArrayToFile(FileBytes, *SlotPath));

	EclipseCampaignTest::FFixture Target = EclipseCampaignTest::FFixture::Make();
	TestTrue(TEXT("Load of v0 file succeeds via migration"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("All migration steps ran (0->1, 1->2, 2->3)"), Target.Save->GetLastLoadMigrationStepCount(), 3);
	TestEqual(TEXT("Migrated state matches source"),
		Target.Campaign->GetState().ComputeStateHash(),
		Source.Campaign->GetState().ComputeStateHash());

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
