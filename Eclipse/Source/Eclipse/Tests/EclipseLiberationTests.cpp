// Liberation unit tier (SPEC-P2-05 build step 2, GDD 14.4): the resolution
// matrix over the pure core — trigger gate, monotone remainder, state-derived
// idempotence, 14.3.5 drop discipline and the "trio is done" fold — plus the
// load-bearing ValidateMutation contract: a no-op SetRegionOwner is rejected,
// so a naive always-three-flips proposal dies wholesale on the atomic commit
// (locked decision 3). Pure logic over the slice board — no fixtures, no bus.
//
// Wiring tier (build step 3, the F1 falsification): the StrategySubsystem
// consumes Event.Mission.Completed on real GameInstance fixtures — the
// re-entrancy Gauntlet (a nested completion mid-commit must not double-commit),
// the idempotence Gauntlet (a second completion commits nothing, state-derived),
// the M1.1 regression (no liberation row = board untouched, P2-04 decision 6
// upheld from the liberation side) and the 14.3.5 degradation ladder (missing
// table warns once; a typo id drops with a warning while the rest still flips).

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipsePrepSubsystem.h"
#include "Base/EclipsePrepTypes.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Quests/EclipseMissionTypes.h"
#include "Quests/EclipseStoryTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseCampaignTransaction.h"
#include "Strategy/EclipseLiberationLogic.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Package.h"

namespace EclipseLiberationTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/**
	 * The slice board at campaign start (create_phase1_content.py owners):
	 * Underworks Player; the Foothold trio TransitCheckpoint(D) /
	 * WorkerHousing(C) / SupplyDepot(D) — the spec's Dominion/Contested/
	 * Dominion start set; FoundryRow and CommsRelay Dominion beyond it.
	 */
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
		AddRegion(TEXT("TransitCheckpoint"), EEclipseRegionOwner::Dominion);
		AddRegion(TEXT("FoundryRow"), EEclipseRegionOwner::Dominion);
		AddRegion(TEXT("WorkerHousing"), EEclipseRegionOwner::Contested);
		AddRegion(TEXT("SupplyDepot"), EEclipseRegionOwner::Dominion);
		AddRegion(TEXT("CommsRelay"), EEclipseRegionOwner::Dominion);
		return State;
	}

	/** The slice's one row: Foothold — the trio to Player, gated on the M1.3 beat (locked decision 1). */
	FEclipseLiberationRow MakeFootholdRow()
	{
		FEclipseLiberationRow Row;
		Row.TriggerMissionId = TEXT("MT_M13");
		Row.RegionIds = { TEXT("TransitCheckpoint"), TEXT("WorkerHousing"), TEXT("SupplyDepot") };
		Row.RequiredBeatTag = EclipseTags::Story_Beat_M13_SignalFire.GetTag();
		return Row; // NewOwner defaults to Player
	}

	FEclipseRegionState* FindRegionMutable(FEclipseCampaignState& State, FName RegionId)
	{
		return State.Regions.FindByPredicate([RegionId](const FEclipseRegionState& R) { return R.RegionId == RegionId; });
	}

	/**
	 * Wiring fixture (build step 3): a real GameInstance so the strategy
	 * subsystem's Event.Mission.Completed subscription is the path under test.
	 * The board mirrors MakeState through the region graph; the liberation
	 * table is in-memory data — no content dependency (the authored
	 * DT_LiberationInstances is Tools/setup_liberation_data.py's).
	 */
	struct FWiringFixture
	{
		UGameInstance* GameInstance = nullptr;
		UEclipseCampaignSubsystem* Campaign = nullptr;
		UEclipseEventBusSubsystem* Bus = nullptr;

		static FWiringFixture Make(UDataTable* LiberationTable)
		{
			FWiringFixture Fixture;
			Fixture.GameInstance = NewObject<UGameInstance>(GEngine);
			Fixture.GameInstance->InitializeStandalone();
			Fixture.Campaign = Fixture.GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
			Fixture.Bus = Fixture.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
			// The consumer under test — resolved so its subscription is live.
			Fixture.GameInstance->GetSubsystem<UEclipseStrategySubsystem>();

			UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
			auto AddRegion = [Graph](FName Id, EEclipseRegionOwner Owner)
			{
				FEclipseRegionDefinition& Definition = Graph->Regions.AddDefaulted_GetRef();
				Definition.RegionId = Id;
				Definition.StartingOwner = Owner;
			};
			AddRegion(TEXT("Underworks"), EEclipseRegionOwner::Player);
			AddRegion(TEXT("TransitCheckpoint"), EEclipseRegionOwner::Dominion);
			AddRegion(TEXT("FoundryRow"), EEclipseRegionOwner::Dominion);
			AddRegion(TEXT("WorkerHousing"), EEclipseRegionOwner::Contested);
			AddRegion(TEXT("SupplyDepot"), EEclipseRegionOwner::Dominion);
			AddRegion(TEXT("CommsRelay"), EEclipseRegionOwner::Dominion);

			UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
			Setup->StartingRosterSize = 0; // the board is the subject; no roster noise
			Setup->RegionGraph = Graph;
			if (LiberationTable != nullptr)
			{
				Setup->LiberationInstances = LiberationTable;
			}
			Fixture.Campaign->StartNewCampaign(Setup);
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

	UDataTable* MakeLiberationTable(const FEclipseLiberationRow& Row)
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FEclipseLiberationRow::StaticStruct();
		Table->AddRow(TEXT("Foothold"), Row);
		return Table;
	}

	/** Commits the M1.3 beat exactly as the debrief does — BEFORE the Completed broadcast (the as-built order the spec calls load-bearing). */
	bool CommitM13Beat(UEclipseCampaignSubsystem& Campaign)
	{
		FEclipseCampaignTransaction Transaction;
		Transaction.Source = TEXT("TestDebrief");
		FEclipseCampaignMutation& Beat = Transaction.Mutations.AddDefaulted_GetRef();
		Beat.Type = EEclipseCampaignMutationType::SetStoryFlag;
		Beat.StoryFlagTag = EclipseTags::Story_Beat_M13_SignalFire.GetTag();
		FString Error;
		return Campaign.CommitTransaction(Transaction, Error);
	}

	/** Scripted completion fact on the P2-04 seam (Completed fires on success only). */
	void BroadcastCompleted(UEclipseEventBusSubsystem& Bus, FName MissionId)
	{
		FEclipseMissionEventPayload Payload;
		Payload.MissionId = MissionId;
		Payload.bSuccess = true;
		Bus.Broadcast(EclipseTags::Event_Mission_Completed, FInstancedStruct::Make(Payload));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationResolveFullTrioTest,
	"Eclipse.Strategy.Liberation.ResolveFullTrio",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationResolveFullTrioTest::RunTest(const FString& Parameters)
{
	FEclipseCampaignState State = EclipseLiberationTest::MakeState();
	const FEclipseLiberationRow Row = EclipseLiberationTest::MakeFootholdRow();

	TestFalse(TEXT("Sanity: campaign start is not liberated"),
		EclipseLiberationLogic::IsLiberationComplete(State, Row));

	TArray<FName> Dropped;
	const FEclipseCampaignTransaction Transaction = EclipseLiberationLogic::ResolveLiberationTransaction(State, Row, Dropped);

	TestEqual(TEXT("Full trio resolves to exactly 3 mutations"), Transaction.Mutations.Num(), 3);
	TestEqual(TEXT("Nothing dropped on a clean row"), Dropped.Num(), 0);
	TestTrue(TEXT("Audit source names the liberation instance (decision 2)"),
		Transaction.Source == EclipseLiberationLogic::LiberationTransactionSource);

	// Row order = commit order = event order (deterministic for tests and the map).
	for (int32 Index = 0; Index < Transaction.Mutations.Num(); ++Index)
	{
		const FEclipseCampaignMutation& Mutation = Transaction.Mutations[Index];
		TestTrue(TEXT("Only SetRegionOwner mutations (decision 6: owner-only)"),
			Mutation.Type == EEclipseCampaignMutationType::SetRegionOwner);
		TestTrue(TEXT("Mutations follow row order"),
			Row.RegionIds.IsValidIndex(Index) && Mutation.RegionId == Row.RegionIds[Index]);
		TestTrue(TEXT("Every flip targets the row owner"),
			Mutation.NewOwner == EEclipseRegionOwner::Player);

		// The spec's contract line: the resolved transaction always passes
		// ValidateMutation for every mutation. (Hoisted before the assert so a
		// failure message carries the actual error, not an unevaluated string.)
		FString Error;
		const bool bMutationValid = EclipseCampaignLogic::ValidateMutation(State, Mutation, Error);
		TestTrue(FString::Printf(TEXT("Mutation %d passes ValidateMutation (%s)"), Index, *Error), bMutationValid);
	}

	// The one transaction commits atomically on the as-built API and the board answers.
	TArray<FEclipseAppliedMutation> Applied;
	FString CommitError;
	TestTrue(TEXT("Liberation transaction commits green"),
		EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, CommitError));
	TestEqual(TEXT("Three applied mutations for the map facts"), Applied.Num(), 3);
	if (Applied.Num() == 3)
	{
		TestTrue(TEXT("Old owner recorded for the RegionControlChanged fact"),
			Applied[0].OldOwner == EEclipseRegionOwner::Dominion);
	}
	for (const FName& RegionId : Row.RegionIds)
	{
		const FEclipseRegionState* Region = State.FindRegion(RegionId);
		TestTrue(FString::Printf(TEXT("Region '%s' is player-held after the flip"), *RegionId.ToString()),
			Region != nullptr && Region->Owner == EEclipseRegionOwner::Player);
	}
	TestTrue(TEXT("The trio is done after the commit (the decision-4 fold)"),
		EclipseLiberationLogic::IsLiberationComplete(State, Row));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationResolveRemainderTest,
	"Eclipse.Strategy.Liberation.ResolveMonotoneRemainder",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationResolveRemainderTest::RunTest(const FString& Parameters)
{
	// A player who already liberated part of the trio through generic offers
	// gets exactly the remainder (decision 3: monotone, never un-flips,
	// never double-pays).
	FEclipseCampaignState State = EclipseLiberationTest::MakeState();
	EclipseLiberationTest::FindRegionMutable(State, TEXT("TransitCheckpoint"))->Owner = EEclipseRegionOwner::Player;
	const FEclipseLiberationRow Row = EclipseLiberationTest::MakeFootholdRow();

	TArray<FName> Dropped;
	const FEclipseCampaignTransaction Transaction = EclipseLiberationLogic::ResolveLiberationTransaction(State, Row, Dropped);

	TestEqual(TEXT("Pre-flipped trio resolves to exactly the remainder"), Transaction.Mutations.Num(), 2);
	TestEqual(TEXT("Pre-flipped region is excluded, not dropped"), Dropped.Num(), 0);
	if (Transaction.Mutations.Num() == 2)
	{
		TestTrue(TEXT("Remainder keeps row order: WorkerHousing first"),
			Transaction.Mutations[0].RegionId == FName(TEXT("WorkerHousing")));
		TestTrue(TEXT("Remainder keeps row order: SupplyDepot second"),
			Transaction.Mutations[1].RegionId == FName(TEXT("SupplyDepot")));
	}

	// The load-bearing contract behind the exclusion (decision 3): a no-op
	// SetRegionOwner is REJECTED by ValidateMutation...
	FEclipseCampaignMutation NoOp;
	NoOp.Type = EEclipseCampaignMutationType::SetRegionOwner;
	NoOp.RegionId = TEXT("TransitCheckpoint");
	NoOp.NewOwner = EEclipseRegionOwner::Player;
	FString Error;
	TestFalse(TEXT("No-op SetRegionOwner is rejected by ValidateMutation"),
		EclipseCampaignLogic::ValidateMutation(State, NoOp, Error));
	TestTrue(TEXT("Rejection names the no-op"), Error.Contains(TEXT("already")));

	// ...so a naive always-three-flips proposal dies wholesale on the atomic
	// commit, leaving state untouched — the bug the state-derived resolution
	// exists to prevent.
	FEclipseCampaignTransaction Naive;
	Naive.Source = EclipseLiberationLogic::LiberationTransactionSource;
	for (const FName& RegionId : Row.RegionIds)
	{
		FEclipseCampaignMutation& Mutation = Naive.Mutations.AddDefaulted_GetRef();
		Mutation.Type = EEclipseCampaignMutationType::SetRegionOwner;
		Mutation.RegionId = RegionId;
		Mutation.NewOwner = EEclipseRegionOwner::Player;
	}
	const uint32 HashBefore = State.ComputeStateHash();
	TArray<FEclipseAppliedMutation> Applied;
	TestFalse(TEXT("Naive always-three transaction is rejected wholesale"),
		EclipseCampaignLogic::CommitTransaction(State, Naive, Applied, Error));
	TestEqual(TEXT("Atomic rejection leaves state untouched"), State.ComputeStateHash(), HashBefore);

	// The resolved remainder, by contrast, commits green.
	TestTrue(TEXT("Resolved remainder commits green"),
		EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, Error));
	TestTrue(TEXT("Trio complete after the remainder lands"),
		EclipseLiberationLogic::IsLiberationComplete(State, Row));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationResolveIdempotentTest,
	"Eclipse.Strategy.Liberation.ResolveIdempotentWhenComplete",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationResolveIdempotentTest::RunTest(const FString& Parameters)
{
	// State-derived idempotence (decision 3, no stored marker): everything at
	// target resolves to an empty transaction — no commit, no events; a
	// hypothetical second M1.3 completion commits nothing.
	FEclipseCampaignState State = EclipseLiberationTest::MakeState();
	const FEclipseLiberationRow Row = EclipseLiberationTest::MakeFootholdRow();
	for (const FName& RegionId : Row.RegionIds)
	{
		EclipseLiberationTest::FindRegionMutable(State, RegionId)->Owner = EEclipseRegionOwner::Player;
	}

	TArray<FName> Dropped;
	const FEclipseCampaignTransaction Transaction = EclipseLiberationLogic::ResolveLiberationTransaction(State, Row, Dropped);

	TestEqual(TEXT("Nothing to commit when all regions are at target"), Transaction.Mutations.Num(), 0);
	TestEqual(TEXT("Nothing dropped either — at-target is legitimate, not damage"), Dropped.Num(), 0);
	TestTrue(TEXT("Complete-fold agrees"), EclipseLiberationLogic::IsLiberationComplete(State, Row));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationDropUnknownIdTest,
	"Eclipse.Strategy.Liberation.DropUnknownAndDuplicateIds",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationDropUnknownIdTest::RunTest(const FString& Parameters)
{
	// 14.3.5: a typo must not reject the whole liberation — the atomic
	// transaction would. Unknown ids drop with a report; the rest still flips.
	FEclipseCampaignState State = EclipseLiberationTest::MakeState();
	FEclipseLiberationRow Row = EclipseLiberationTest::MakeFootholdRow();
	Row.RegionIds = { TEXT("TransitCheckpoint"), TEXT("GhostBlock"), TEXT("SupplyDepot") };

	TArray<FName> Dropped;
	FEclipseCampaignTransaction Transaction = EclipseLiberationLogic::ResolveLiberationTransaction(State, Row, Dropped);

	TestEqual(TEXT("Known regions still resolve"), Transaction.Mutations.Num(), 2);
	TestEqual(TEXT("Exactly the unknown id is dropped"), Dropped.Num(), 1);
	TestTrue(TEXT("Dropped report names the typo"), Dropped.Contains(FName(TEXT("GhostBlock"))));
	if (Transaction.Mutations.Num() == 2)
	{
		TestTrue(TEXT("Survivors keep row order: TransitCheckpoint first"),
			Transaction.Mutations[0].RegionId == FName(TEXT("TransitCheckpoint")));
		TestTrue(TEXT("Survivors keep row order: SupplyDepot second"),
			Transaction.Mutations[1].RegionId == FName(TEXT("SupplyDepot")));
	}

	TArray<FEclipseAppliedMutation> Applied;
	FString Error;
	TestTrue(TEXT("Damaged-row resolution still commits green"),
		EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, Error));

	// A duplicate id is data damage too: its second flip would be a no-op
	// mid-transaction and the atomic commit would reject everything — so the
	// resolution drops the repeat and reports it.
	FEclipseCampaignState DupState = EclipseLiberationTest::MakeState();
	Row = EclipseLiberationTest::MakeFootholdRow();
	Row.RegionIds = { TEXT("TransitCheckpoint"), TEXT("TransitCheckpoint"), TEXT("SupplyDepot") };
	Transaction = EclipseLiberationLogic::ResolveLiberationTransaction(DupState, Row, Dropped);

	TestEqual(TEXT("Duplicate resolves each region once"), Transaction.Mutations.Num(), 2);
	TestEqual(TEXT("The repeat is dropped and reported"), Dropped.Num(), 1);
	TestTrue(TEXT("Dropped report names the duplicate"), Dropped.Contains(FName(TEXT("TransitCheckpoint"))));
	TestTrue(TEXT("Duplicate-row resolution still commits green"),
		EclipseCampaignLogic::CommitTransaction(DupState, Transaction, Applied, Error));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationTriggerGateTest,
	"Eclipse.Strategy.Liberation.TriggerBeatGate",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationTriggerGateTest::RunTest(const FString& Parameters)
{
	FEclipseCampaignState State = EclipseLiberationTest::MakeState();
	const FEclipseLiberationRow Row = EclipseLiberationTest::MakeFootholdRow();
	const FGameplayTag Beat = EclipseTags::Story_Beat_M13_SignalFire.GetTag();
	if (!TestTrue(TEXT("Native M1.3 beat tag available"), Beat.IsValid()))
	{
		return true;
	}

	// Id mismatch never triggers, gated or not.
	TestFalse(TEXT("Foreign mission id does not trigger"),
		EclipseLiberationLogic::IsLiberationTriggered(State, Row, TEXT("MT_M11")));

	// Gated row + flag unset: not triggered (the audit gate against real StoryFlags).
	TestFalse(TEXT("Beat gate holds while the flag is unset"),
		EclipseLiberationLogic::IsLiberationTriggered(State, Row, TEXT("MT_M13")));

	// Flag committed (as the debrief's SetStoryFlag would): triggered.
	State.StoryFlags.Add(Beat);
	TestTrue(TEXT("Id match + committed beat triggers"),
		EclipseLiberationLogic::IsLiberationTriggered(State, Row, TEXT("MT_M13")));

	// Empty gate = ungated: id match alone fires, even on a flagless campaign.
	FEclipseLiberationRow Ungated = EclipseLiberationTest::MakeFootholdRow();
	Ungated.RequiredBeatTag = FGameplayTag();
	const FEclipseCampaignState FreshState = EclipseLiberationTest::MakeState();
	TestTrue(TEXT("Empty RequiredBeatTag skips the gate (14.3.5)"),
		EclipseLiberationLogic::IsLiberationTriggered(FreshState, Ungated, TEXT("MT_M13")));

	// A row without identity is damaged data, never a wildcard.
	FEclipseLiberationRow Nameless = EclipseLiberationTest::MakeFootholdRow();
	Nameless.TriggerMissionId = NAME_None;
	Nameless.RequiredBeatTag = FGameplayTag();
	TestFalse(TEXT("Row without TriggerMissionId never triggers"),
		EclipseLiberationLogic::IsLiberationTriggered(FreshState, Nameless, NAME_None));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationCompleteCheckTest,
	"Eclipse.Strategy.Liberation.CompleteCheck",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationCompleteCheckTest::RunTest(const FString& Parameters)
{
	FEclipseCampaignState State = EclipseLiberationTest::MakeState();
	const FEclipseLiberationRow Row = EclipseLiberationTest::MakeFootholdRow();

	TestFalse(TEXT("Campaign start: not complete"), EclipseLiberationLogic::IsLiberationComplete(State, Row));

	EclipseLiberationTest::FindRegionMutable(State, TEXT("TransitCheckpoint"))->Owner = EEclipseRegionOwner::Player;
	EclipseLiberationTest::FindRegionMutable(State, TEXT("WorkerHousing"))->Owner = EEclipseRegionOwner::Player;
	TestFalse(TEXT("Two of three: not complete"), EclipseLiberationLogic::IsLiberationComplete(State, Row));

	EclipseLiberationTest::FindRegionMutable(State, TEXT("SupplyDepot"))->Owner = EEclipseRegionOwner::Player;
	TestTrue(TEXT("All three at target: complete"), EclipseLiberationLogic::IsLiberationComplete(State, Row));

	// Unknown ids fold out exactly as resolution drops them: a typo makes the
	// row degrade, not wedge — the remaining regions decide.
	FEclipseLiberationRow TypoRow = EclipseLiberationTest::MakeFootholdRow();
	TypoRow.RegionIds.Add(TEXT("GhostBlock"));
	TestTrue(TEXT("Typo id does not wedge completeness (14.3.5)"),
		EclipseLiberationLogic::IsLiberationComplete(State, TypoRow));

	// A row that names no existing region is damaged data, never a finished
	// liberation.
	FEclipseLiberationRow EmptyRow = EclipseLiberationTest::MakeFootholdRow();
	EmptyRow.RegionIds.Empty();
	TestFalse(TEXT("Empty region set is never complete"), EclipseLiberationLogic::IsLiberationComplete(State, EmptyRow));
	FEclipseLiberationRow AllUnknownRow = EclipseLiberationTest::MakeFootholdRow();
	AllUnknownRow.RegionIds = { TEXT("GhostBlock"), TEXT("PhantomRow") };
	TestFalse(TEXT("All-unknown region set is never complete"), EclipseLiberationLogic::IsLiberationComplete(State, AllUnknownRow));

	// Phase 3 shape honesty: NewOwner is data — a Contested step reads
	// completeness against ITS target, not Player.
	FEclipseLiberationRow ContestedRow = EclipseLiberationTest::MakeFootholdRow();
	ContestedRow.NewOwner = EEclipseRegionOwner::Contested;
	TestFalse(TEXT("Player-held trio is not 'complete' for a Contested-step row"),
		EclipseLiberationLogic::IsLiberationComplete(State, ContestedRow));

	return true;
}

// ---------------------------------------------------------------------------
// Wiring tier (SPEC-P2-05 build step 3) — the F1 Gauntlets over the real bus.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationWiringReentrancyTest,
	"Eclipse.Strategy.Liberation.WiringReentrantTriggerCommitsOnce",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationWiringReentrancyTest::RunTest(const FString& Parameters)
{
	// F1: the liberation commit broadcasts RegionControlChanged from INSIDE the
	// Mission.Completed dispatch. A consumer of those facts that re-broadcasts
	// the completion mid-commit (the nastiest legal bus shape) must not produce
	// a second commit: idempotence is state-derived — by the time any nested
	// handler runs, the atomic commit has already applied all three flips.
	EclipseLiberationTest::FWiringFixture Fixture = EclipseLiberationTest::FWiringFixture::Make(
		EclipseLiberationTest::MakeLiberationTable(EclipseLiberationTest::MakeFootholdRow()));
	if (!TestNotNull(TEXT("Campaign subsystem exists"), Fixture.Campaign))
	{
		return false;
	}
	TestTrue(TEXT("Debrief beat commits first (as-built order)"), EclipseLiberationTest::CommitM13Beat(*Fixture.Campaign));

	// The nested no-op resolution logs its one line — asserted, not tolerated.
	AddExpectedMessage(TEXT("has nothing to commit"), ELogVerbosity::Display, EAutomationExpectedMessageFlags::Contains, 1);

	TArray<FName> FlippedRegionIds;
	bool bRetriggered = false;
	FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
		EclipseTags::Event_Strategy_RegionControlChanged,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseStrategyEventPayload* Fact = Payload.GetPtr<FEclipseStrategyEventPayload>())
			{
				FlippedRegionIds.Add(Fact->RegionId);
			}
			if (!bRetriggered)
			{
				bRetriggered = true;
				EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));
			}
		}),
		FEclipseStrategyEventPayload::StaticStruct());

	EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));

	TestTrue(TEXT("The re-entrant completion actually fired mid-commit"), bRetriggered);
	TestEqual(TEXT("Exactly three RegionControlChanged facts — no double commit"), FlippedRegionIds.Num(), 3);
	if (FlippedRegionIds.Num() == 3)
	{
		TestTrue(TEXT("Facts arrive in row order: TransitCheckpoint first"), FlippedRegionIds[0] == FName(TEXT("TransitCheckpoint")));
		TestTrue(TEXT("Facts arrive in row order: WorkerHousing second"), FlippedRegionIds[1] == FName(TEXT("WorkerHousing")));
		TestTrue(TEXT("Facts arrive in row order: SupplyDepot third"), FlippedRegionIds[2] == FName(TEXT("SupplyDepot")));
	}
	for (const FName& RegionId : EclipseLiberationTest::MakeFootholdRow().RegionIds)
	{
		const FEclipseRegionState* Region = Fixture.Campaign->GetState().FindRegion(RegionId);
		TestTrue(FString::Printf(TEXT("Region '%s' is player-held"), *RegionId.ToString()),
			Region != nullptr && Region->Owner == EEclipseRegionOwner::Player);
	}

	Fixture.Bus->Unsubscribe(Handle);
	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationWiringIdempotentTest,
	"Eclipse.Strategy.Liberation.WiringSecondCompletionCommitsNothing",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationWiringIdempotentTest::RunTest(const FString& Parameters)
{
	// The exactly-once contract, state-derived (locked decision 3): a forced
	// second Completed for the same id = no commit, no events, state hash
	// unchanged, one log line.
	EclipseLiberationTest::FWiringFixture Fixture = EclipseLiberationTest::FWiringFixture::Make(
		EclipseLiberationTest::MakeLiberationTable(EclipseLiberationTest::MakeFootholdRow()));
	if (!TestNotNull(TEXT("Campaign subsystem exists"), Fixture.Campaign))
	{
		return false;
	}
	TestTrue(TEXT("Debrief beat commits first"), EclipseLiberationTest::CommitM13Beat(*Fixture.Campaign));

	int32 RegionEventCount = 0;
	FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
		EclipseTags::Event_Strategy_RegionControlChanged,
		FEclipseEventNativeDelegate::CreateLambda([&RegionEventCount](FGameplayTag, const FInstancedStruct&) { ++RegionEventCount; }));

	EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));
	TestEqual(TEXT("First completion flips the trio"), RegionEventCount, 3);
	const uint32 HashAfterFirst = Fixture.Campaign->GetState().ComputeStateHash();

	AddExpectedMessage(TEXT("has nothing to commit"), ELogVerbosity::Display, EAutomationExpectedMessageFlags::Contains, 1);
	EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));

	TestEqual(TEXT("Second completion emits zero region facts"), RegionEventCount, 3);
	TestEqual(TEXT("Second completion leaves the state hash unchanged"),
		Fixture.Campaign->GetState().ComputeStateHash(), HashAfterFirst);

	Fixture.Bus->Unsubscribe(Handle);
	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationWiringM11RegressionTest,
	"Eclipse.Strategy.Liberation.WiringM11CompletionFlipsNothing",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationWiringM11RegressionTest::RunTest(const FString& Parameters)
{
	// Regression (P2-04 decision 6, liberation side): M1.1 has
	// bProgressRegionOnSuccess=false (asserted in EclipseMissionM1Tests) AND no
	// liberation row — its completion must leave the board bit-identical. The
	// no-row warning fires once per mission id, not per completion.
	EclipseLiberationTest::FWiringFixture Fixture = EclipseLiberationTest::FWiringFixture::Make(
		EclipseLiberationTest::MakeLiberationTable(EclipseLiberationTest::MakeFootholdRow()));
	if (!TestNotNull(TEXT("Campaign subsystem exists"), Fixture.Campaign))
	{
		return false;
	}
	const uint32 HashBefore = Fixture.Campaign->GetState().ComputeStateHash();

	int32 RegionEventCount = 0;
	FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
		EclipseTags::Event_Strategy_RegionControlChanged,
		FEclipseEventNativeDelegate::CreateLambda([&RegionEventCount](FGameplayTag, const FInstancedStruct&) { ++RegionEventCount; }));

	// Two completions, ONE warning: the 14.3.5 breadcrumb is loud but bounded.
	AddExpectedMessage(TEXT("no liberation row triggered for completed mission 'MT_M11'"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M11"));
	EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M11"));

	TestEqual(TEXT("Zero RegionControlChanged on M1.1 completion"), RegionEventCount, 0);
	TestEqual(TEXT("Board bit-identical after M1.1 completion"),
		Fixture.Campaign->GetState().ComputeStateHash(), HashBefore);
	TestTrue(TEXT("Trio start owners stand: TransitCheckpoint Dominion"),
		Fixture.Campaign->GetState().FindRegion(TEXT("TransitCheckpoint"))->Owner == EEclipseRegionOwner::Dominion);
	TestTrue(TEXT("Trio start owners stand: WorkerHousing Contested"),
		Fixture.Campaign->GetState().FindRegion(TEXT("WorkerHousing"))->Owner == EEclipseRegionOwner::Contested);
	TestTrue(TEXT("Trio start owners stand: SupplyDepot Dominion"),
		Fixture.Campaign->GetState().FindRegion(TEXT("SupplyDepot"))->Owner == EEclipseRegionOwner::Dominion);

	Fixture.Bus->Unsubscribe(Handle);
	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationWiringDegradationTest,
	"Eclipse.Strategy.Liberation.WiringDegradationNeverThrows",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationWiringDegradationTest::RunTest(const FString& Parameters)
{
	// The 14.3.5 ladder on the wiring (header-doc obligations, MANDATORY):
	// missing table = no flip + ONE warning across repeated completions, never
	// a throw; a typo region id drops with one warning while the rest still
	// flips — the campaign runs on in both cases.
	AddExpectedMessage(TEXT("no LiberationInstances table"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessage(TEXT("dropped 1 region id"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	// Phase A: no table wired at all.
	{
		EclipseLiberationTest::FWiringFixture Fixture = EclipseLiberationTest::FWiringFixture::Make(nullptr);
		if (!TestNotNull(TEXT("Campaign subsystem exists"), Fixture.Campaign))
		{
			return false;
		}
		TestTrue(TEXT("Beat commits without a liberation table"), EclipseLiberationTest::CommitM13Beat(*Fixture.Campaign));
		const uint32 HashBefore = Fixture.Campaign->GetState().ComputeStateHash();

		EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));
		EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));

		TestEqual(TEXT("Missing table: board untouched, campaign runs on"),
			Fixture.Campaign->GetState().ComputeStateHash(), HashBefore);
		Fixture.Shutdown();
	}

	// Phase B: a typo in the authored row — WorkerHousing misspelled. The known
	// remainder still flips (a typo must not reject the whole liberation).
	{
		FEclipseLiberationRow TypoRow = EclipseLiberationTest::MakeFootholdRow();
		TypoRow.RegionIds = { TEXT("TransitCheckpoint"), TEXT("GhostBlock"), TEXT("SupplyDepot") };
		EclipseLiberationTest::FWiringFixture Fixture = EclipseLiberationTest::FWiringFixture::Make(
			EclipseLiberationTest::MakeLiberationTable(TypoRow));
		TestTrue(TEXT("Beat commits"), EclipseLiberationTest::CommitM13Beat(*Fixture.Campaign));

		int32 RegionEventCount = 0;
		FEclipseEventSubscriptionHandle Handle = Fixture.Bus->Subscribe(
			EclipseTags::Event_Strategy_RegionControlChanged,
			FEclipseEventNativeDelegate::CreateLambda([&RegionEventCount](FGameplayTag, const FInstancedStruct&) { ++RegionEventCount; }));

		EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));

		TestEqual(TEXT("Typo row: the two known regions still flip"), RegionEventCount, 2);
		TestTrue(TEXT("TransitCheckpoint flipped despite the typo"),
			Fixture.Campaign->GetState().FindRegion(TEXT("TransitCheckpoint"))->Owner == EEclipseRegionOwner::Player);
		TestTrue(TEXT("SupplyDepot flipped despite the typo"),
			Fixture.Campaign->GetState().FindRegion(TEXT("SupplyDepot"))->Owner == EEclipseRegionOwner::Player);
		TestTrue(TEXT("WorkerHousing untouched — the typo names nothing"),
			Fixture.Campaign->GetState().FindRegion(TEXT("WorkerHousing"))->Owner == EEclipseRegionOwner::Contested);

		Fixture.Bus->Unsubscribe(Handle);
		Fixture.Shutdown();
	}

	return true;
}

// The seam Gauntlet (review finding, confirmed): every other wiring test above
// hand-builds the two facts production is supposed to emit — the beat commit and
// the Completed broadcast — in the order the test itself picks. That leaves the
// real ordering unpinned: swap the commit (EclipseMissionSubsystem ResolveDebrief)
// with its Completed broadcast and the liberation would silently die while all of
// them stayed green, because they commit the beat themselves first. This test
// drives the actual debrief instead, so ONE test pins three couplings at once:
// commit-before-broadcast, the story row's CompletionBeatTag == the liberation
// row's RequiredBeatTag, and the story row's MissionId == TriggerMissionId.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationDebriefSeamTest,
	"Eclipse.Strategy.Liberation.WiringDebriefSeamFlipsTrio",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationDebriefSeamTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	// The authored M1.3 asset, in memory at the path ResolveMissionSpec loads
	// from (TryLoad finds live objects before disk — the skeleton test's proven
	// trick). Unique id so it can never shadow a future MT_M13.uasset.
	UPackage* MissionPackage = CreatePackage(TEXT("/Game/Data/Missions/MT_M13Seam"));
	UEclipseMissionAsset* M13 = NewObject<UEclipseMissionAsset>(MissionPackage, TEXT("MT_M13Seam"), RF_Public | RF_Standalone);
	M13->TemplateId = TEXT("MT_M13Seam");
	M13->DisplayName = FText::FromString(TEXT("Signal Fire (seam)"));
	// The mission's own region must NOT move, so the only owner changes in this
	// test are the liberation's (decision 6 keeps story missions world-neutral).
	M13->bProgressRegionOnSuccess = false;
	{
		FEclipseObjectiveDef& Jammer = M13->Objectives.AddDefaulted_GetRef();
		Jammer.ObjectiveId = TEXT("Obj_M13_Jammer");
		Jammer.Type = EEclipseObjectiveType::DestroyTarget;
		Jammer.TargetId = TEXT("Site_M13_Jammer");
	}
	M13->InsertionPointIds = { TEXT("Entry_Main") };

	// Board: home + the mission site (outside the trio) + the trio itself.
	UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
	{
		auto AddRegion = [Graph](FName Id, EEclipseRegionOwner Owner, const TArray<FName>& Links)
		{
			FEclipseRegionDefinition& Definition = Graph->Regions.AddDefaulted_GetRef();
			Definition.RegionId = Id;
			Definition.RegionType = EEclipseRegionType::Industrial;
			Definition.StartingOwner = Owner;
			Definition.Lanes = EclipseRegionGraph::OpenLanes(Links);
		};
		AddRegion(TEXT("Underworks"), EEclipseRegionOwner::Player, { TEXT("SignalSite") });
		AddRegion(TEXT("SignalSite"), EEclipseRegionOwner::Dominion, { TEXT("Underworks") });
		AddRegion(TEXT("TransitCheckpoint"), EEclipseRegionOwner::Dominion, {});
		AddRegion(TEXT("WorkerHousing"), EEclipseRegionOwner::Contested, {});
		AddRegion(TEXT("SupplyDepot"), EEclipseRegionOwner::Dominion, {});
	}

	// Story row: the pin that makes MT_M13Seam launchable AND the producer of the
	// beat the liberation row gates on. Both tables agree by construction here —
	// that agreement is exactly what this test exists to keep true.
	UDataTable* StoryTable = NewObject<UDataTable>();
	StoryTable->RowStruct = FEclipseStoryMissionRow::StaticStruct();
	{
		FEclipseStoryMissionRow Row;
		Row.MissionId = TEXT("MT_M13Seam");
		Row.PinnedRegionId = TEXT("SignalSite");
		Row.CompletionBeatTag = EclipseTags::Story_Beat_M13_SignalFire.GetTag();
		StoryTable->AddRow(TEXT("Story_M13Seam"), Row);
	}

	FEclipseLiberationRow LiberationRow = EclipseLiberationTest::MakeFootholdRow();
	LiberationRow.TriggerMissionId = TEXT("MT_M13Seam");
	UDataTable* LiberationTable = EclipseLiberationTest::MakeLiberationTable(LiberationRow);

	UEclipsePrepTuningAsset* PrepTuning = NewObject<UEclipsePrepTuningAsset>();
	PrepTuning->SquadSize = 1;
	{
		UDataTable* Loadouts = NewObject<UDataTable>();
		Loadouts->RowStruct = FEclipseLoadoutOptionRow::StaticStruct();
		FEclipseLoadoutOptionRow Scavenged;
		Scavenged.LoadoutTag = EclipseTags::Resource_Credits.GetTag();
		Loadouts->AddRow(TEXT("Loadout_Scavenged"), Scavenged);
		PrepTuning->LoadoutOptions = Loadouts;
	}

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 2;
	Setup->RegionGraph = Graph;
	Setup->StoryMissions = StoryTable;
	Setup->LiberationInstances = LiberationTable;
	Setup->PrepTuning = PrepTuning;
	Campaign->StartNewCampaign(Setup);

	FString Error;
	TestTrue(TEXT("M1.3 seam mission selected on the pin"), Strategy->SelectMission(TEXT("SignalSite"), Error));

	TArray<FGuid> Squad;
	Squad.Add(Campaign->GetState().Roster[0].SoldierId);
	TestTrue(TEXT("Launch accepted"), Prep->LaunchMission(Squad, EclipseTags::Resource_Credits.GetTag(), TEXT("Entry_Main"), Error));

	// Pre-state: the trio is untouched and the gate beat is NOT set, so nothing
	// below can be a leftover from setup.
	TestFalse(TEXT("Gate beat unset before the debrief"),
		Campaign->GetState().StoryFlags.Contains(EclipseTags::Story_Beat_M13_SignalFire.GetTag()));

	TestTrue(TEXT("Objective completes"), Mission->CompleteObjective(TEXT("Obj_M13_Jammer"), Error));

	// THE SEAM: one real debrief. It commits the beat and then broadcasts
	// Completed; the strategy subsystem's subscription must see the beat already
	// in state and flip the trio inside its own single transaction.
	TestTrue(TEXT("Real debrief commits"), Mission->ResolveDebrief(true, Error));

	TestTrue(TEXT("Debrief committed the gate beat"),
		Campaign->GetState().StoryFlags.Contains(EclipseTags::Story_Beat_M13_SignalFire.GetTag()));

	auto OwnerOf = [Campaign](FName RegionId)
	{
		const FEclipseRegionState* Region = Campaign->GetState().FindRegion(RegionId);
		return Region != nullptr ? Region->Owner : EEclipseRegionOwner::Dominion;
	};
	TestTrue(TEXT("TransitCheckpoint liberated through the real seam"), OwnerOf(TEXT("TransitCheckpoint")) == EEclipseRegionOwner::Player);
	TestTrue(TEXT("WorkerHousing liberated through the real seam"), OwnerOf(TEXT("WorkerHousing")) == EEclipseRegionOwner::Player);
	TestTrue(TEXT("SupplyDepot liberated through the real seam"), OwnerOf(TEXT("SupplyDepot")) == EEclipseRegionOwner::Player);
	TestTrue(TEXT("The mission's own region stayed Dominion (decision 6)"), OwnerOf(TEXT("SignalSite")) == EEclipseRegionOwner::Dominion);

	M13->ClearFlags(RF_Public | RF_Standalone);
	MissionPackage->ClearFlags(RF_Public | RF_Standalone);
	GameInstance->Shutdown();
	return true;
}


// De bevrijdingszin komt EEN keer aan, na de vakken.
//
// Twee beweringen, en de eerste is de reden dat dit feit een eigen struct heeft:
// de Foothold draait DRIE vakken om met EEN zin. Had die zin in het vak-feit
// gezeten, dan stond hij drie keer op het debriefscherm - een fout die alleen
// zichtbaar wordt als je telt, want drie keer dezelfde zin leest als een zin.
//
// De tweede is de volgorde. Het feit vuurt NA de commit en dus na de vak-feiten,
// omdat een uitleg vooraf een uitkomst zou aankondigen die nog kon worden
// afgewezen. Je leest dus eerst wat er kantelde en dan waarom.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLiberationLineReachesTheDebriefTest,
	"Eclipse.Strategy.Liberation.LineReachesTheDebrief",
	EclipseLiberationTest::TestFlags)

bool FEclipseLiberationLineReachesTheDebriefTest::RunTest(const FString& Parameters)
{
	const FText Zin = FText::FromString(TEXT("De doorgang is van ons; de wijk kan ademen."));

	FEclipseLiberationRow Row = EclipseLiberationTest::MakeFootholdRow();
	Row.ContextLine = Zin;

	EclipseLiberationTest::FWiringFixture Fixture = EclipseLiberationTest::FWiringFixture::Make(
		EclipseLiberationTest::MakeLiberationTable(Row));
	if (!TestNotNull(TEXT("Campaign subsystem exists"), Fixture.Campaign))
	{
		return false;
	}
	TestTrue(TEXT("Debrief beat commits first (as-built order)"), EclipseLiberationTest::CommitM13Beat(*Fixture.Campaign));

	// Een gedeelde volgorde-lijst: zonder die ene lijst kun je wel tellen maar
	// niet zien wat er eerst kwam, en de volgorde is hier de halve bewering.
	TArray<FString> Volgorde;
	TArray<FText> Zinnen;
	int32 GemeldeVakken = 0;

	FEclipseEventSubscriptionHandle VakHandle = Fixture.Bus->Subscribe(
		EclipseTags::Event_Strategy_RegionControlChanged,
		FEclipseEventNativeDelegate::CreateLambda([&Volgorde](FGameplayTag, const FInstancedStruct&)
		{
			Volgorde.Add(TEXT("vak"));
		}),
		FEclipseStrategyEventPayload::StaticStruct());

	FEclipseEventSubscriptionHandle ZinHandle = Fixture.Bus->Subscribe(
		EclipseTags::Event_Strategy_LiberationResolved,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct& Payload)
		{
			Volgorde.Add(TEXT("zin"));
			if (const FEclipseLiberationEventPayload* Fact = Payload.GetPtr<FEclipseLiberationEventPayload>())
			{
				Zinnen.Add(Fact->ContextLine);
				GemeldeVakken = Fact->RegionCount;
			}
		}),
		FEclipseLiberationEventPayload::StaticStruct());

	EclipseLiberationTest::BroadcastCompleted(*Fixture.Bus, TEXT("MT_M13"));

	// EEN zin, niet drie. Dit is de assertie waar het ontwerp op staat.
	TestEqual(TEXT("Precies EEN bevrijdingszin, ook al kantelden er drie vakken"), Zinnen.Num(), 1);
	if (Zinnen.Num() == 1)
	{
		TestTrue(TEXT("Het is de geauthorde zin en niet een samengestelde tekst"), Zinnen[0].EqualTo(Zin));
	}
	TestEqual(TEXT("Het feit meldt hoeveel vakken er ECHT omdraaiden"), GemeldeVakken, 3);

	// En de volgorde: drie vakken, dan de uitleg.
	TestEqual(TEXT("Vier feiten in totaal"), Volgorde.Num(), 4);
	if (Volgorde.Num() == 4)
	{
		TestTrue(TEXT("De uitleg komt NA de vakken, niet ervoor"),
			Volgorde[0] == TEXT("vak") && Volgorde[1] == TEXT("vak")
			&& Volgorde[2] == TEXT("vak") && Volgorde[3] == TEXT("zin"));
	}

	Fixture.Bus->Unsubscribe(VakHandle);
	Fixture.Bus->Unsubscribe(ZinHandle);
	Fixture.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
