// Tests for SPEC-P1-05 (GDD 14.4): pure phase/consequence logic + the canonical
// "spawn -> complete-by-script -> verify consequences committed" functional test
// in unit form (the Gauntlet level variant lands with the graybox district).

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Quests/EclipseMissionLogic.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Quests/EclipseStoryTypes.h"
#include "Squad/EclipseRosterTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Package.h"

namespace EclipseMissionTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	TArray<FEclipseObjectiveDef> MakeObjectives()
	{
		TArray<FEclipseObjectiveDef> Objectives;
		FEclipseObjectiveDef& Primary = Objectives.AddDefaulted_GetRef();
		Primary.ObjectiveId = TEXT("Obj_Primary");
		FEclipseObjectiveDef& Optional = Objectives.AddDefaulted_GetRef();
		Optional.ObjectiveId = TEXT("Obj_NoAlarms");
		Optional.bOptional = true;
		FEclipseObjectiveDef& Exfil = Objectives.AddDefaulted_GetRef();
		Exfil.ObjectiveId = TEXT("Obj_Exfil");
		return Objectives;
	}

	/**
	 * In-memory mission asset at the exact /Game path ResolveMissionSpec loads,
	 * so subsystem tests can author optionals (conditions/rewards) without a
	 * cooked asset — TryLoad finds the in-memory object first. Reuses the
	 * object across reruns in one session (RF_Standalone outlives the test).
	 */
	UEclipseMissionAsset* MakeMissionAsset(const TCHAR* TemplateId)
	{
		// Drill ids only. These packages live at real content paths with
		// RF_Standalone, so passing a SHIPPED id here would shadow the authored
		// .uasset for the rest of the process — the M1.1 Gauntlet would then
		// silently grade this fixture instead of the data on disk. Today's ids
		// (MT_AlarmDrill/MT_MedicDrill) are safe; this check keeps them that way.
		checkf(!FString(TemplateId).StartsWith(TEXT("MT_M1")) || FString(TemplateId).EndsWith(TEXT("Drill")),
			TEXT("MakeMissionAsset: '%s' collides with an authored mission id — pick a *Drill id so the shipped asset stays the thing under test."), TemplateId);

		const FString PackagePath = FString::Printf(TEXT("/Game/Data/Missions/%s"), TemplateId);
		UPackage* Package = CreatePackage(*PackagePath);
		UEclipseMissionAsset* Asset = FindObject<UEclipseMissionAsset>(Package, TemplateId);
		if (Asset == nullptr)
		{
			Asset = NewObject<UEclipseMissionAsset>(Package, FName(TemplateId), RF_Public | RF_Standalone);
		}
		Asset->TemplateId = FName(TemplateId);
		Asset->Objectives.Reset();
		Asset->bProgressRegionOnSuccess = true;
		return Asset;
	}

	/** Two-region drill board: player home "Underworks" + one Dominion Checkpoint whose offer launches TemplateId. */
	UEclipseCampaignSetupAsset* MakeDrillSetup(const TCHAR* RegionId, const TCHAR* TemplateId, int32 RewardCredits)
	{
		UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
		FEclipseRegionDefinition& Home = Graph->Regions.AddDefaulted_GetRef();
		Home.RegionId = TEXT("Underworks");
		Home.RegionType = EEclipseRegionType::Industrial;
		Home.StartingOwner = EEclipseRegionOwner::Player;
		Home.Lanes = { FName(RegionId) };

		FEclipseRegionDefinition& Target = Graph->Regions.AddDefaulted_GetRef();
		Target.RegionId = FName(RegionId);
		Target.RegionType = EEclipseRegionType::Checkpoint;
		Target.StartingOwner = EEclipseRegionOwner::Dominion;
		Target.Lanes = { TEXT("Underworks") };

		UDataTable* Offers = NewObject<UDataTable>();
		Offers->RowStruct = FEclipseMissionOfferRow::StaticStruct();
		FEclipseMissionOfferRow OfferRow;
		OfferRow.RegionType = EEclipseRegionType::Checkpoint;
		OfferRow.TemplateId = FName(TemplateId);
		OfferRow.RewardCredits = RewardCredits;
		Offers->AddRow(TEXT("Offer_Drill"), OfferRow);
		Graph->MissionOffers = Offers;

		UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
		Setup->StartingDay = 1;
		Setup->StartingRosterSize = 2;
		Setup->RegionGraph = Graph;
		return Setup;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionPhaseMachineTest,
	"Eclipse.Quests.Mission.PhaseMachine",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionPhaseMachineTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMissionLogic;

	TestTrue(TEXT("None -> Insertion"), CanAdvancePhase(EEclipseMissionPhase::None, EEclipseMissionPhase::Insertion));
	TestTrue(TEXT("Insertion -> Objectives"), CanAdvancePhase(EEclipseMissionPhase::Insertion, EEclipseMissionPhase::Objectives));
	TestTrue(TEXT("Objectives -> Extraction"), CanAdvancePhase(EEclipseMissionPhase::Objectives, EEclipseMissionPhase::Extraction));
	TestTrue(TEXT("Objectives -> Debrief (field abort = fail-forward)"), CanAdvancePhase(EEclipseMissionPhase::Objectives, EEclipseMissionPhase::Debrief));
	TestTrue(TEXT("Extraction -> Debrief"), CanAdvancePhase(EEclipseMissionPhase::Extraction, EEclipseMissionPhase::Debrief));
	TestFalse(TEXT("No skipping insertion"), CanAdvancePhase(EEclipseMissionPhase::None, EEclipseMissionPhase::Objectives));
	TestFalse(TEXT("No going back"), CanAdvancePhase(EEclipseMissionPhase::Extraction, EEclipseMissionPhase::Objectives));
	TestFalse(TEXT("Finished is terminal"), CanAdvancePhase(EEclipseMissionPhase::Finished, EEclipseMissionPhase::Insertion));

	const TArray<FEclipseObjectiveDef> Objectives = EclipseMissionTest::MakeObjectives();
	TestFalse(TEXT("Mandatory incomplete"), AreMandatoryObjectivesComplete(Objectives, { TEXT("Obj_Primary") }));
	TestTrue(TEXT("Optionals never gate success (GDD 11.4)"),
		AreMandatoryObjectivesComplete(Objectives, { TEXT("Obj_Primary"), TEXT("Obj_Exfil") }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionConsequenceCompositionTest,
	"Eclipse.Quests.Mission.ConsequenceComposition",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionConsequenceCompositionTest::RunTest(const FString& Parameters)
{
	FEclipseCampaignState State;
	State.Day = 3;
	State.Wallet.Add(EclipseTags::Resource_Intel.GetTag(), 10);
	FEclipseRegionState& Region = State.Regions.AddDefaulted_GetRef();
	Region.RegionId = TEXT("Region_Target");
	Region.Owner = EEclipseRegionOwner::Dominion;
	FEclipseSoldierRecord& Soldier = State.Roster.AddDefaulted_GetRef();
	Soldier.SoldierId = FGuid(9, 9, 9, 1);
	Soldier.Name = TEXT("Vara Chen");
	Soldier.MissionsServed = 4;
	FEclipseSoldierRecord& Survivor = State.Roster.AddDefaulted_GetRef();
	Survivor.SoldierId = FGuid(9, 9, 9, 2);
	Survivor.Name = TEXT("Oram Bex");
	Survivor.MissionsServed = 2;

	FEclipseMissionRewards Rewards;
	Rewards.Credits = 60;
	Rewards.Intel = 4;

	FEclipseMissionOutcome Outcome;
	Outcome.RegionId = TEXT("Region_Target");
	Outcome.bSuccess = true;
	Outcome.DeployedSoldierIds = { Soldier.SoldierId, Survivor.SoldierId };

	FEclipseResolvedCasualty Casualty;
	Casualty.SoldierId = Soldier.SoldierId;
	Casualty.Cause = TEXT("Gunfire");
	Casualty.bDead = true;

	// Win: rewards + one-step flip + death recorded atomically with its memorial.
	FEclipseCampaignTransaction Transaction = EclipseMissionLogic::ComposeConsequences(
		Outcome, Rewards, { Casualty }, State,
		EclipseTags::Resource_Credits.GetTag(), EclipseTags::Resource_Materials.GetTag(), EclipseTags::Resource_Intel.GetTag(),
		/*bProgressRegionOnSuccess*/ true);

	TArray<FEclipseAppliedMutation> Applied;
	FString Error;
	TestTrue(TEXT("Win consequences commit"), EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, Error));
	TestEqual(TEXT("Credits rewarded"), State.GetBalance(EclipseTags::Resource_Credits.GetTag()), 60);
	TestTrue(TEXT("Dominion region progressed to Contested"), State.FindRegion(TEXT("Region_Target"))->Owner == EEclipseRegionOwner::Contested);
	TestTrue(TEXT("Casualty is dead"), State.FindSoldier(Soldier.SoldierId)->Status == EEclipseSoldierStatus::Dead);
	TestEqual(TEXT("Memorial written in the same transaction"), State.Memorial.Num(), 1);
	TestEqual(TEXT("Memorial counts the fatal mission"), State.Memorial[0].MissionsServed, 5);
	TestEqual(TEXT("Fallen soldier's roster row counts the fatal mission"), State.FindSoldier(Soldier.SoldierId)->MissionsServed, 5);
	TestEqual(TEXT("Survivor's missions-served incremented (SPEC-P1-07)"), State.FindSoldier(Survivor.SoldierId)->MissionsServed, 3);

	// Lose (fail-forward): half intel salvage, no flip. Fresh Dominion-held
	// region (the win branch above mutated the shared state's region).
	FEclipseCampaignState LoseState;
	FEclipseRegionState& LoseRegion = LoseState.Regions.AddDefaulted_GetRef();
	LoseRegion.RegionId = TEXT("Region_Target");
	LoseRegion.Owner = EEclipseRegionOwner::Dominion;
	FEclipseMissionOutcome LoseOutcome;
	LoseOutcome.RegionId = TEXT("Region_Target");
	LoseOutcome.bSuccess = false;

	FEclipseCampaignTransaction LoseTransaction = EclipseMissionLogic::ComposeConsequences(
		LoseOutcome, Rewards, {}, LoseState,
		EclipseTags::Resource_Credits.GetTag(), EclipseTags::Resource_Materials.GetTag(), EclipseTags::Resource_Intel.GetTag(), true);

	TestTrue(TEXT("Loss consequences commit (fail-forward, GDD 11.4)"), EclipseCampaignLogic::CommitTransaction(LoseState, LoseTransaction, Applied, Error));
	TestEqual(TEXT("Salvaged intel = half"), LoseState.GetBalance(EclipseTags::Resource_Intel.GetTag()), 2);
	TestEqual(TEXT("No credits on failure"), LoseState.GetBalance(EclipseTags::Resource_Credits.GetTag()), 0);
	TestTrue(TEXT("No region flip on failure"), LoseState.FindRegion(TEXT("Region_Target"))->Owner == EEclipseRegionOwner::Dominion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionFullLoopTest,
	"Eclipse.Quests.Mission.FullLoopConsequencesCommitted",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionFullLoopTest::RunTest(const FString& Parameters)
{
	// The canonical GDD 14.4 functional contract, headless: select on the map ->
	// launch -> complete by script -> verify CampaignState committed.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	// Board: player home + adjacent Dominion target with an offer.
	UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
	{
		FEclipseRegionDefinition& Home = Graph->Regions.AddDefaulted_GetRef();
		Home.RegionId = TEXT("Underworks");
		Home.RegionType = EEclipseRegionType::Industrial;
		Home.StartingOwner = EEclipseRegionOwner::Player;
		Home.Lanes = { TEXT("Checkpoint") };

		FEclipseRegionDefinition& Target = Graph->Regions.AddDefaulted_GetRef();
		Target.RegionId = TEXT("Checkpoint");
		Target.RegionType = EEclipseRegionType::Checkpoint;
		Target.StartingOwner = EEclipseRegionOwner::Dominion;
		Target.Lanes = { TEXT("Underworks") };

		UDataTable* Offers = NewObject<UDataTable>();
		Offers->RowStruct = FEclipseMissionOfferRow::StaticStruct();
		FEclipseMissionOfferRow OfferRow;
		OfferRow.RegionType = EEclipseRegionType::Checkpoint;
		OfferRow.TemplateId = TEXT("MT_Assault");
		OfferRow.RewardCredits = 60;
		OfferRow.RewardIntel = 4;
		Offers->AddRow(TEXT("Offer_Checkpoint"), OfferRow);
		Graph->MissionOffers = Offers;
	}

	// Story layer on the same board: the launched template carries a completion
	// beat, so this loop proves the debrief's real table-lookup path AND that
	// beat + rewards + casualty + day tick land as ONE atomic commit
	// (SPEC-P2-04 decision 12). Row rewards mirror the generic offer so the
	// reward asserts below stay meaningful for both resolution paths.
	UDataTable* StoryTable = NewObject<UDataTable>();
	StoryTable->RowStruct = FEclipseStoryMissionRow::StaticStruct();
	{
		FEclipseStoryMissionRow StoryRow;
		StoryRow.MissionId = TEXT("MT_Assault");
		StoryRow.PinnedRegionId = TEXT("Checkpoint");
		StoryRow.CompletionBeatTag = EclipseTags::Story_Beat_M11_ThirteenBullets.GetTag();
		StoryRow.RewardCredits = 60;
		StoryRow.RewardIntel = 4;
		StoryTable->AddRow(TEXT("Story_Assault"), StoryRow);
	}

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 2;
	Setup->RegionGraph = Graph;
	Setup->StoryMissions = StoryTable;
	Campaign->StartNewCampaign(Setup);

	int32 CompletedEvents = 0;
	FEclipseEventSubscriptionHandle CompletedHandle = Bus->Subscribe(
		EclipseTags::Event_Mission_Completed,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct&) { ++CompletedEvents; }));

	// 1. Pick on the map (adjacency-legal target).
	FString Error;
	TestTrue(TEXT("Mission selected on the board"), Strategy->SelectMission(TEXT("Checkpoint"), Error));

	// 2. Launch with the two starting recruits (prep flow arrives with SPEC-P1-08;
	//    the launch API is the same seam).
	TArray<FGuid> Squad;
	for (const FEclipseSoldierRecord& Soldier : Campaign->GetState().Roster)
	{
		Squad.Add(Soldier.SoldierId);
	}
	TestTrue(TEXT("Mission starts"), Mission->StartMission(Squad, Error));
	TestTrue(TEXT("Objectives phase active"), Mission->GetPhase() == EEclipseMissionPhase::Objectives);

	// 3. Complete by script (synthesized default spec: Obj_Primary + Obj_Exfil),
	//    with one soldier downed on the way.
	Mission->NotifySoldierDowned(Squad[0], TEXT("Gunfire"));
	TestTrue(TEXT("Primary completes"), Mission->CompleteObjective(TEXT("Obj_Primary"), Error));
	TestTrue(TEXT("Exfil completes"), Mission->CompleteObjective(TEXT("Obj_Exfil"), Error));
	TestTrue(TEXT("Extraction opened after mandatory set"), Mission->GetPhase() == EEclipseMissionPhase::Extraction);

	// 4. Debrief commits consequences.
	const uint32 HashBefore = Campaign->GetState().ComputeStateHash();
	TestTrue(TEXT("Debrief resolves"), Mission->ResolveDebrief(true, Error));
	TestTrue(TEXT("State changed"), Campaign->GetState().ComputeStateHash() != HashBefore);

	TestEqual(TEXT("Reward credits committed"), Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()), 60);
	TestTrue(TEXT("Region visibly progressed (the map noticed)"),
		Campaign->GetState().FindRegion(TEXT("Checkpoint"))->Owner == EEclipseRegionOwner::Contested);
	TestTrue(TEXT("Downed soldier resolved and recorded"),
		Campaign->GetState().FindSoldier(Squad[0])->Status == EEclipseSoldierStatus::Dead);
	TestEqual(TEXT("Memorial entry present"), Campaign->GetState().Memorial.Num(), 1);
	TestEqual(TEXT("Fallen soldier served the fatal mission"), Campaign->GetState().FindSoldier(Squad[0])->MissionsServed, 1);
	TestEqual(TEXT("Survivor served the mission too"), Campaign->GetState().FindSoldier(Squad[1])->MissionsServed, 1);
	TestEqual(TEXT("Mission.Completed broadcast once"), CompletedEvents, 1);
	TestTrue(TEXT("Completion beat committed in the same debrief transaction"),
		Campaign->GetState().StoryFlags.Contains(EclipseTags::Story_Beat_M11_ThirteenBullets.GetTag()));
	TestEqual(TEXT("Debrief cost the day (P2-03 locked decision 4)"), Campaign->GetState().Day, 2);
	TestTrue(TEXT("Runtime finished"), Mission->GetPhase() == EEclipseMissionPhase::Finished);

	// 5. Second loop must be startable (the gate question is "loop #2").
	TestTrue(TEXT("Board offers a next target"), Strategy->GetAvailableOffers().Num() > 0);

	Bus->Unsubscribe(CompletedHandle);
	GameInstance->Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionDebriefBeatTest,
	"Eclipse.Quests.Mission.DebriefCommitsCompletionBeat",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionDebriefBeatTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-04 decision 12: the completion beat is one atomic fact with the
	// debrief. Pure composition proves the four-way contract: win commits the
	// beat, a re-completed mission skips it (SetStoryFlag's duplicate-reject
	// would otherwise drop the whole debrief), loss never commits story
	// progress, and a beatless (generic) mission composes nothing.
	const FGameplayTag Beat = EclipseTags::Story_Beat_M11_ThirteenBullets.GetTag();

	auto CountBeatMutations = [&Beat](const FEclipseCampaignTransaction& Transaction)
	{
		int32 Count = 0;
		for (const FEclipseCampaignMutation& Mutation : Transaction.Mutations)
		{
			if (Mutation.Type == EEclipseCampaignMutationType::SetStoryFlag && Mutation.StoryFlagTag == Beat)
			{
				++Count;
			}
		}
		return Count;
	};

	FEclipseCampaignState State;
	FEclipseMissionOutcome Outcome;
	Outcome.bSuccess = true;

	FEclipseCampaignTransaction Win = EclipseMissionLogic::ComposeConsequences(
		Outcome, FEclipseMissionRewards(), {}, State,
		EclipseTags::Resource_Credits.GetTag(), EclipseTags::Resource_Materials.GetTag(), EclipseTags::Resource_Intel.GetTag(),
		/*bProgressRegionOnSuccess*/ false, Beat);
	TestEqual(TEXT("Win commits the completion beat"), CountBeatMutations(Win), 1);

	TArray<FEclipseAppliedMutation> Applied;
	FString Error;
	TestTrue(TEXT("Beat debrief commits"), EclipseCampaignLogic::CommitTransaction(State, Win, Applied, Error));
	TestTrue(TEXT("StoryFlags carry the beat"), State.StoryFlags.Contains(Beat));

	// Re-completion against the committed state: no beat mutation, and the
	// debrief still commits whole — the duplicate-reject is never hit.
	FEclipseCampaignTransaction Again = EclipseMissionLogic::ComposeConsequences(
		Outcome, FEclipseMissionRewards(), {}, State,
		EclipseTags::Resource_Credits.GetTag(), EclipseTags::Resource_Materials.GetTag(), EclipseTags::Resource_Intel.GetTag(),
		false, Beat);
	TestEqual(TEXT("Re-completion skips the already-set beat"), CountBeatMutations(Again), 0);
	TestTrue(TEXT("Second debrief still commits whole"), EclipseCampaignLogic::CommitTransaction(State, Again, Applied, Error));

	Outcome.bSuccess = false;
	FEclipseCampaignTransaction Loss = EclipseMissionLogic::ComposeConsequences(
		Outcome, FEclipseMissionRewards(), {}, State,
		EclipseTags::Resource_Credits.GetTag(), EclipseTags::Resource_Materials.GetTag(), EclipseTags::Resource_Intel.GetTag(),
		false, Beat);
	TestEqual(TEXT("Loss never composes story progress (GDD 11.4)"), CountBeatMutations(Loss), 0);

	Outcome.bSuccess = true;
	FEclipseCampaignTransaction Generic = EclipseMissionLogic::ComposeConsequences(
		Outcome, FEclipseMissionRewards(), {}, State,
		EclipseTags::Resource_Credits.GetTag(), EclipseTags::Resource_Materials.GetTag(), EclipseTags::Resource_Intel.GetTag(),
		false);
	TestEqual(TEXT("A beatless mission composes no beat"), CountBeatMutations(Generic), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionOptionalTruthTableTest,
	"Eclipse.Quests.Mission.OptionalObjectiveTruthTable",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionOptionalTruthTableTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-04 ghost/zero-casualty truth table over the pure evaluation:
	// alarm-latch x casualty-latch against every condition shape. Latch
	// semantics are the contract — "did it EVER happen this run".
	TArray<FEclipseObjectiveDef> Objectives;
	auto AddOptional = [&Objectives](const TCHAR* Id, bool bNoAlarm, bool bNoCasualties)
	{
		FEclipseObjectiveDef& Objective = Objectives.AddDefaulted_GetRef();
		Objective.ObjectiveId = Id;
		Objective.bOptional = true;
		Objective.bRequiresNoAlarm = bNoAlarm;
		Objective.bRequiresNoCasualties = bNoCasualties;
		Objective.OptionalRewardCredits = 10;
	};
	AddOptional(TEXT("Opt_Ghost"), true, false);
	AddOptional(TEXT("Opt_NoCas"), false, true);
	AddOptional(TEXT("Opt_Both"), true, true);
	AddOptional(TEXT("Opt_Plain"), false, false);
	AddOptional(TEXT("Opt_NotDone"), false, false);

	// Mandatory decoy with conditions/rewards set: never read (bOptional gate),
	// never paid, never "missed" — and alarm never fails the mission (GDD 11.4).
	FEclipseObjectiveDef& Mandatory = Objectives.AddDefaulted_GetRef();
	Mandatory.ObjectiveId = TEXT("Obj_Primary");
	Mandatory.bRequiresNoAlarm = true;
	Mandatory.OptionalRewardCredits = 99;

	const TArray<FName> Completed = { TEXT("Opt_Ghost"), TEXT("Opt_NoCas"), TEXT("Opt_Both"), TEXT("Opt_Plain"), TEXT("Obj_Primary") };

	auto PaidNames = [](const TArray<FEclipseObjectiveDef>& Paid)
	{
		TArray<FName> Names;
		for (const FEclipseObjectiveDef& Objective : Paid)
		{
			Names.Add(Objective.ObjectiveId);
		}
		return Names;
	};

	TArray<FEclipseObjectiveDef> Paid;
	TArray<FName> Missed;
	using namespace EclipseMissionLogic;

	EvaluateOptionalObjectives(Objectives, Completed, /*alarm*/ false, /*downed*/ false, Paid, Missed);
	TestEqual(TEXT("Quiet+clean pays every completed optional"), Paid.Num(), 4);
	TestEqual(TEXT("Quiet+clean misses nothing"), Missed.Num(), 0);

	EvaluateOptionalObjectives(Objectives, Completed, /*alarm*/ true, /*downed*/ false, Paid, Missed);
	TestEqual(TEXT("Alarm voids exactly the no-alarm conditions"), Missed, TArray<FName>({ TEXT("Opt_Ghost"), TEXT("Opt_Both") }));
	TestEqual(TEXT("Alarm still pays the rest"), PaidNames(Paid), TArray<FName>({ TEXT("Opt_NoCas"), TEXT("Opt_Plain") }));

	EvaluateOptionalObjectives(Objectives, Completed, /*alarm*/ false, /*downed*/ true, Paid, Missed);
	TestEqual(TEXT("A down voids exactly the zero-casualty conditions"), Missed, TArray<FName>({ TEXT("Opt_NoCas"), TEXT("Opt_Both") }));
	TestEqual(TEXT("A down still pays the rest"), PaidNames(Paid), TArray<FName>({ TEXT("Opt_Ghost"), TEXT("Opt_Plain") }));

	EvaluateOptionalObjectives(Objectives, Completed, /*alarm*/ true, /*downed*/ true, Paid, Missed);
	TestEqual(TEXT("Both latches leave only the unconditional optional"), PaidNames(Paid), TArray<FName>({ TEXT("Opt_Plain") }));
	TestEqual(TEXT("Both latches miss all three conditioned optionals"), Missed.Num(), 3);

	TestFalse(TEXT("An uncompleted optional is absent, not 'missed'"), Missed.Contains(FName(TEXT("Opt_NotDone"))));
	TestFalse(TEXT("A mandatory objective never pays optional rewards"), PaidNames(Paid).Contains(FName(TEXT("Obj_Primary"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionOptionalRewardCompositionTest,
	"Eclipse.Quests.Mission.OptionalRewardsAtomicComposition",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionOptionalRewardCompositionTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-04: optional payouts are AdjustResource mutations (Reason
	// "OptionalObjective") inside the SAME debrief transaction — one commit
	// moves base reward and stretch bonus together, or neither.
	TArray<FEclipseObjectiveDef> Objectives;
	FEclipseObjectiveDef& Ghost = Objectives.AddDefaulted_GetRef();
	Ghost.ObjectiveId = TEXT("Opt_Ghost");
	Ghost.bOptional = true;
	Ghost.bRequiresNoAlarm = true;
	Ghost.OptionalRewardCredits = 25;
	FEclipseObjectiveDef& NoCas = Objectives.AddDefaulted_GetRef();
	NoCas.ObjectiveId = TEXT("Opt_NoCas");
	NoCas.bOptional = true;
	NoCas.bRequiresNoCasualties = true;
	NoCas.OptionalRewardIntel = 10;
	FEclipseObjectiveDef& Plain = Objectives.AddDefaulted_GetRef();
	Plain.ObjectiveId = TEXT("Opt_Plain");
	Plain.bOptional = true;
	Plain.OptionalRewardMaterials = 20;
	FEclipseObjectiveDef& Primary = Objectives.AddDefaulted_GetRef();
	Primary.ObjectiveId = TEXT("Obj_Primary");

	FEclipseMissionRewards Rewards;
	Rewards.Credits = 50;

	auto CountOptionalMutations = [](const FEclipseCampaignTransaction& Transaction)
	{
		int32 Count = 0;
		for (const FEclipseCampaignMutation& Mutation : Transaction.Mutations)
		{
			if (Mutation.Type == EEclipseCampaignMutationType::AdjustResource && Mutation.Reason == FName(TEXT("OptionalObjective")))
			{
				++Count;
			}
		}
		return Count;
	};

	auto Compose = [&](const FEclipseMissionOutcome& Outcome, const TArray<FEclipseResolvedCasualty>& Casualties, const FEclipseCampaignState& State)
	{
		return EclipseMissionLogic::ComposeConsequences(
			Outcome, Rewards, Casualties, State,
			EclipseTags::Resource_Credits.GetTag(), EclipseTags::Resource_Materials.GetTag(), EclipseTags::Resource_Intel.GetTag(),
			/*bProgressRegionOnSuccess*/ false, FGameplayTag(), Objectives);
	};

	FEclipseMissionOutcome Outcome;
	Outcome.bSuccess = true;
	Outcome.CompletedObjectiveIds = { TEXT("Obj_Primary"), TEXT("Opt_Ghost"), TEXT("Opt_NoCas"), TEXT("Opt_Plain") };

	// Quiet + clean win: all three optionals pay, atomically with the base reward.
	{
		FEclipseCampaignState State;
		FEclipseCampaignTransaction Transaction = Compose(Outcome, {}, State);
		TestEqual(TEXT("Three optional payouts composed"), CountOptionalMutations(Transaction), 3);
		TArray<FEclipseAppliedMutation> Applied;
		FString Error;
		TestTrue(TEXT("One commit carries base + optionals"), EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, Error));
		TestEqual(TEXT("Credits = base + ghost bonus"), State.GetBalance(EclipseTags::Resource_Credits.GetTag()), 75);
		TestEqual(TEXT("Materials = unconditional bonus"), State.GetBalance(EclipseTags::Resource_Materials.GetTag()), 20);
		TestEqual(TEXT("Intel = zero-casualty bonus"), State.GetBalance(EclipseTags::Resource_Intel.GetTag()), 10);
	}

	// Alarm latch: the ghost bonus vanishes from the wallet, the rest stands.
	{
		FEclipseCampaignState State;
		FEclipseMissionOutcome Loud = Outcome;
		Loud.bAlarmRaised = true;
		FEclipseCampaignTransaction Transaction = Compose(Loud, {}, State);
		TestEqual(TEXT("Alarm drops one payout"), CountOptionalMutations(Transaction), 2);
		TArray<FEclipseAppliedMutation> Applied;
		FString Error;
		TestTrue(TEXT("Loud debrief commits"), EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, Error));
		TestEqual(TEXT("Credits = base only (ghost voided)"), State.GetBalance(EclipseTags::Resource_Credits.GetTag()), 50);
		TestEqual(TEXT("Materials unaffected by alarm"), State.GetBalance(EclipseTags::Resource_Materials.GetTag()), 20);
	}

	// Downed -> stabilized (the amendment's edge): the soldier comes home
	// WOUNDED, but DownedSoldierIds still lists them — zero-casualty is lost,
	// the ghost bonus is not.
	{
		FEclipseCampaignState State;
		FEclipseSoldierRecord& Soldier = State.Roster.AddDefaulted_GetRef();
		Soldier.SoldierId = FGuid(7, 7, 7, 1);
		Soldier.Name = TEXT("Juno Hale");

		FEclipseMissionOutcome Bloodied = Outcome;
		Bloodied.DownedSoldierIds = { Soldier.SoldierId };
		FEclipseResolvedCasualty Stabilized;
		Stabilized.SoldierId = Soldier.SoldierId;
		Stabilized.Cause = TEXT("Gunfire");
		Stabilized.bDead = false;
		Stabilized.DaysOut = 5;

		FEclipseCampaignTransaction Transaction = Compose(Bloodied, { Stabilized }, State);
		TestEqual(TEXT("Stabilized down still drops the zero-casualty payout"), CountOptionalMutations(Transaction), 2);
		TArray<FEclipseAppliedMutation> Applied;
		FString Error;
		TestTrue(TEXT("Bloodied debrief commits"), EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, Error));
		TestEqual(TEXT("Intel = 0: 'ever went down' is the latch, the save changes only the resolution"), State.GetBalance(EclipseTags::Resource_Intel.GetTag()), 0);
		TestEqual(TEXT("Ghost bonus survives a quiet-but-bloody run"), State.GetBalance(EclipseTags::Resource_Credits.GetTag()), 75);
		TestTrue(TEXT("The soldier lives, wounded"), State.FindSoldier(Soldier.SoldierId)->Status == EEclipseSoldierStatus::Wounded);
	}

	// Loss: stretch bonuses never pay on fail-forward, even quiet and clean.
	{
		FEclipseCampaignState State;
		FEclipseMissionOutcome Loss = Outcome;
		Loss.bSuccess = false;
		FEclipseCampaignTransaction Transaction = Compose(Loss, {}, State);
		TestEqual(TEXT("Loss composes zero optional payouts (GDD 11.4: salvage, not bonuses)"), CountOptionalMutations(Transaction), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionAlarmPhaseFlowTest,
	"Eclipse.Quests.Mission.AlarmLatchAndPhaseChangedFlow",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionAlarmPhaseFlowTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-04 emission path: every outer transition broadcasts
	// Event.Mission.PhaseChanged(bAuthoredSubPhase=false); the alarm travels as
	// the named sub-phase — idempotent, latched, reset by StartMission.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	UEclipseMissionAsset* Asset = EclipseMissionTest::MakeMissionAsset(TEXT("MT_AlarmDrill"));
	{
		FEclipseObjectiveDef& Primary = Asset->Objectives.AddDefaulted_GetRef();
		Primary.ObjectiveId = TEXT("Obj_Primary");
		FEclipseObjectiveDef& Ghost = Asset->Objectives.AddDefaulted_GetRef();
		Ghost.ObjectiveId = TEXT("Obj_Ghost");
		Ghost.bOptional = true;
		Ghost.bRequiresNoAlarm = true;
		Ghost.OptionalRewardCredits = 25;
	}
	Campaign->StartNewCampaign(EclipseMissionTest::MakeDrillSetup(TEXT("DrillYard"), TEXT("MT_AlarmDrill"), 50));

	struct FPhaseRecord
	{
		FName PhaseName;
		bool bAuthoredSubPhase = false;
	};
	TArray<FPhaseRecord> PhaseEvents;
	FEclipseEventSubscriptionHandle PhaseHandle = Bus->Subscribe(
		EclipseTags::Event_Mission_PhaseChanged,
		FEclipseEventNativeDelegate::CreateLambda([&PhaseEvents](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseMissionEventPayload* MissionPayload = Payload.GetPtr<FEclipseMissionEventPayload>())
			{
				PhaseEvents.Add({ MissionPayload->PhaseName, MissionPayload->bAuthoredSubPhase });
			}
		}));

	// Alarm with no run: no latch, no broadcast (alert sources outlive missions).
	Mission->NotifyAlarmRaised();
	TestFalse(TEXT("No latch outside a run"), Mission->IsAlarmRaised());
	TestEqual(TEXT("No broadcast outside a run"), PhaseEvents.Num(), 0);

	FString Error;
	TestTrue(TEXT("Mission selected"), Strategy->SelectMission(TEXT("DrillYard"), Error));
	TArray<FGuid> Squad;
	for (const FEclipseSoldierRecord& Soldier : Campaign->GetState().Roster)
	{
		Squad.Add(Soldier.SoldierId);
	}
	TestTrue(TEXT("Mission starts"), Mission->StartMission(Squad, Error));

	TestEqual(TEXT("Start broadcasts the two opening phases"), PhaseEvents.Num(), 2);
	if (PhaseEvents.Num() == 2)
	{
		TestEqual(TEXT("First phase is Insertion"), PhaseEvents[0].PhaseName, FName(TEXT("Insertion")));
		TestFalse(TEXT("Insertion is an outer phase"), PhaseEvents[0].bAuthoredSubPhase);
		TestEqual(TEXT("Second phase is Objectives"), PhaseEvents[1].PhaseName, FName(TEXT("Objectives")));
	}

	// First alarm: latch + the named sub-phase. Second alarm: silence.
	Mission->NotifyAlarmRaised();
	TestTrue(TEXT("Alarm latched"), Mission->IsAlarmRaised());
	TestEqual(TEXT("Alarm broadcast as a sub-phase"), PhaseEvents.Num(), 3);
	if (PhaseEvents.Num() == 3)
	{
		TestEqual(TEXT("Alarm rides the canonical name"), PhaseEvents[2].PhaseName, EclipseMissionLogic::AlarmSubPhaseName);
		TestTrue(TEXT("Alarm is an authored sub-phase"), PhaseEvents[2].bAuthoredSubPhase);
	}
	Mission->NotifyAlarmRaised();
	TestEqual(TEXT("Second alarm never re-broadcasts (idempotent)"), PhaseEvents.Num(), 3);

	// Complete the ghost optional under alarm (field completion stands; the
	// debrief verdict is where it is lost), then the mandatory set.
	TestTrue(TEXT("Ghost optional completes in the field"), Mission->CompleteObjective(TEXT("Obj_Ghost"), Error));
	TestEqual(TEXT("An optional completion changes no phase"), PhaseEvents.Num(), 3);
	TestTrue(TEXT("Primary completes"), Mission->CompleteObjective(TEXT("Obj_Primary"), Error));
	TestEqual(TEXT("Mandatory set opens Extraction"), PhaseEvents.Num(), 4);
	if (PhaseEvents.Num() == 4)
	{
		TestEqual(TEXT("Extraction broadcast"), PhaseEvents[3].PhaseName, FName(TEXT("Extraction")));
	}

	TestTrue(TEXT("Debrief resolves"), Mission->ResolveDebrief(true, Error));
	TestEqual(TEXT("Debrief broadcast"), PhaseEvents.Num(), 5);
	if (PhaseEvents.Num() == 5)
	{
		TestEqual(TEXT("Debrief is the last outer phase"), PhaseEvents[4].PhaseName, FName(TEXT("Debrief")));
		TestFalse(TEXT("Debrief is an outer phase"), PhaseEvents[4].bAuthoredSubPhase);
	}

	// The ghost optional was completed but the alarm voids it: base reward
	// only, and the outcome shows the miss.
	TestEqual(TEXT("Wallet holds base reward only (ghost voided)"), Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()), 50);
	TestTrue(TEXT("Outcome carries the alarm latch"), Mission->GetLastOutcome().bAlarmRaised);
	TestEqual(TEXT("Voided optional is visible as missed"), Mission->GetLastOutcome().MissedOptionalObjectiveIds, TArray<FName>({ TEXT("Obj_Ghost") }));
	TestTrue(TEXT("Field completion still on record"), Mission->GetLastOutcome().CompletedObjectiveIds.Contains(FName(TEXT("Obj_Ghost"))));

	// StartMission resets the latch (run-scoped, never campaign-scoped).
	TestTrue(TEXT("Second selection"), Strategy->SelectMission(TEXT("DrillYard"), Error));
	TestTrue(TEXT("Second mission starts"), Mission->StartMission(Squad, Error));
	TestFalse(TEXT("Alarm latch reset by StartMission"), Mission->IsAlarmRaised());
	TestEqual(TEXT("Second start broadcasts its own opening phases"), PhaseEvents.Num(), 7);

	Bus->Unsubscribe(PhaseHandle);
	GameInstance->Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionZeroCasualtyLatchTest,
	"Eclipse.Quests.Mission.ZeroCasualtyLatchSurvivesStabilize",
	EclipseMissionTest::TestFlags)

bool FEclipseMissionZeroCasualtyLatchTest::RunTest(const FString& Parameters)
{
	// The amendment's decisive case end-to-end (SPEC-P2-04): a soldier goes
	// down, the Medic window saves them — they come home Wounded, but the
	// zero-casualty optional is still lost, because DownedSoldierIds retains
	// stabilized soldiers ("niemand ging OOIT neer"). A clean second run pays.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	UEclipseMissionAsset* Asset = EclipseMissionTest::MakeMissionAsset(TEXT("MT_MedicDrill"));
	{
		FEclipseObjectiveDef& Primary = Asset->Objectives.AddDefaulted_GetRef();
		Primary.ObjectiveId = TEXT("Obj_Primary");
		FEclipseObjectiveDef& Untouched = Asset->Objectives.AddDefaulted_GetRef();
		Untouched.ObjectiveId = TEXT("Obj_Untouchable");
		Untouched.bOptional = true;
		Untouched.bRequiresNoCasualties = true;
		Untouched.OptionalRewardMaterials = 20;
	}

	UEclipseRosterTuningAsset* RosterTuning = NewObject<UEclipseRosterTuningAsset>();
	RosterTuning->WoundedDaysOut = 5;
	UEclipseCampaignSetupAsset* Setup = EclipseMissionTest::MakeDrillSetup(TEXT("MedYard"), TEXT("MT_MedicDrill"), 40);
	Setup->RosterTuning = RosterTuning;
	Campaign->StartNewCampaign(Setup);

	FString Error;
	TestTrue(TEXT("Mission selected"), Strategy->SelectMission(TEXT("MedYard"), Error));
	TArray<FGuid> Squad;
	for (const FEclipseSoldierRecord& Soldier : Campaign->GetState().Roster)
	{
		Squad.Add(Soldier.SoldierId);
	}
	TestTrue(TEXT("Mission starts"), Mission->StartMission(Squad, Error));

	// Down at t=10, stabilized at t=12 inside an 8s window: the save lands.
	Mission->NotifySoldierDownedAt(Squad[0], TEXT("Gunfire"), 10.0);
	TestTrue(TEXT("Stabilize inside the window succeeds"), Mission->TryStabilizeSoldier(Squad[0], 8.0f, 12.0));

	TestTrue(TEXT("Zero-casualty optional completes in the field"), Mission->CompleteObjective(TEXT("Obj_Untouchable"), Error));
	TestTrue(TEXT("Primary completes"), Mission->CompleteObjective(TEXT("Obj_Primary"), Error));
	TestTrue(TEXT("Debrief resolves"), Mission->ResolveDebrief(true, Error));

	TestTrue(TEXT("Stabilized soldier comes home Wounded, not dead"),
		Campaign->GetState().FindSoldier(Squad[0])->Status == EEclipseSoldierStatus::Wounded);
	TestTrue(TEXT("DownedSoldierIds retains the stabilized soldier (the as-built latch)"),
		Mission->GetLastOutcome().DownedSoldierIds.Contains(Squad[0]));
	TestEqual(TEXT("Zero-casualty bonus withheld: he WENT DOWN, the save is not an eraser"),
		Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag()), 0);
	TestEqual(TEXT("Voided optional visible as missed"),
		Mission->GetLastOutcome().MissedOptionalObjectiveIds, TArray<FName>({ TEXT("Obj_Untouchable") }));
	TestEqual(TEXT("Base reward unaffected"), Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()), 40);

	// Clean second run: the latch was run-scoped, the bonus pays.
	TestTrue(TEXT("Second selection"), Strategy->SelectMission(TEXT("MedYard"), Error));
	TestTrue(TEXT("Second mission starts"), Mission->StartMission(Squad, Error));
	TestTrue(TEXT("Optional completes again"), Mission->CompleteObjective(TEXT("Obj_Untouchable"), Error));
	TestTrue(TEXT("Primary completes again"), Mission->CompleteObjective(TEXT("Obj_Primary"), Error));
	TestTrue(TEXT("Second debrief resolves"), Mission->ResolveDebrief(true, Error));
	TestEqual(TEXT("Clean run pays the zero-casualty bonus"), Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag()), 20);
	TestEqual(TEXT("No misses on the clean run"), Mission->GetLastOutcome().MissedOptionalObjectiveIds.Num(), 0);

	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
