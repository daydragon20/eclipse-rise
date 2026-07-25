// Liberation unit tier (SPEC-P2-05 build step 2, GDD 14.4): the resolution
// matrix over the pure core — trigger gate, monotone remainder, state-derived
// idempotence, 14.3.5 drop discipline and the "trio is done" fold — plus the
// load-bearing ValidateMutation contract: a no-op SetRegionOwner is rejected,
// so a naive always-three-flips proposal dies wholesale on the atomic commit
// (locked decision 3). Pure logic over the slice board — no fixtures, no bus.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/EclipseGameplayTags.h"
#include "Misc/AutomationTest.h"
#include "Strategy/EclipseCampaignTransaction.h"
#include "Strategy/EclipseLiberationLogic.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
