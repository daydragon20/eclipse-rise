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
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "StructUtils/InstancedStruct.h"

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

	FEclipseMissionRewards Rewards;
	Rewards.Credits = 60;
	Rewards.Intel = 4;

	FEclipseMissionOutcome Outcome;
	Outcome.RegionId = TEXT("Region_Target");
	Outcome.bSuccess = true;

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
		Home.ConnectedRegionIds = { TEXT("Checkpoint") };

		FEclipseRegionDefinition& Target = Graph->Regions.AddDefaulted_GetRef();
		Target.RegionId = TEXT("Checkpoint");
		Target.RegionType = EEclipseRegionType::Checkpoint;
		Target.StartingOwner = EEclipseRegionOwner::Dominion;
		Target.ConnectedRegionIds = { TEXT("Underworks") };

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

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 2;
	Setup->RegionGraph = Graph;
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
	TestEqual(TEXT("Mission.Completed broadcast once"), CompletedEvents, 1);
	TestTrue(TEXT("Runtime finished"), Mission->GetPhase() == EEclipseMissionPhase::Finished);

	// 5. Second loop must be startable (the gate question is "loop #2").
	TestTrue(TEXT("Board offers a next target"), Strategy->GetAvailableOffers().Num() > 0);

	Bus->Unsubscribe(CompletedHandle);
	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
