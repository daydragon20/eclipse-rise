// Tests for SPEC-P1-08 (GDD 14.4): launch-payload validation and the full-loop
// smoke — the gate question ("does a second loop start?") automated.

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipsePrepLogic.h"
#include "Base/EclipsePrepSubsystem.h"
#include "Base/EclipsePrepTypes.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"

namespace EclipsePrepTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	TArray<EclipsePrepLogic::FEclipseLoadoutOption> MakeLoadouts()
	{
		TArray<EclipsePrepLogic::FEclipseLoadoutOption> Options;
		EclipsePrepLogic::FEclipseLoadoutOption& Base = Options.AddDefaulted_GetRef();
		Base.OptionId = TEXT("Loadout_Scavenged");
		Base.LoadoutTag = EclipseTags::Resource_Credits.GetTag(); // any registered tag serves as identity in tests
		EclipsePrepLogic::FEclipseLoadoutOption& Gated = Options.AddDefaulted_GetRef();
		Gated.OptionId = TEXT("Loadout_Rifle");
		Gated.LoadoutTag = EclipseTags::Resource_Materials.GetTag();
		Gated.RequiredUnlockTag = EclipseTags::Resource_Intel.GetTag();
		return Options;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipsePrepValidationTest,
	"Eclipse.Base.Prep.LaunchPayloadValidation",
	EclipsePrepTest::TestFlags)

bool FEclipsePrepValidationTest::RunTest(const FString& Parameters)
{
	FEclipseCampaignState State;
	State.Day = 10;

	FEclipseSoldierRecord& Ready = State.Roster.AddDefaulted_GetRef();
	Ready.SoldierId = FGuid(1, 0, 0, 1);
	Ready.Name = TEXT("Ready Soldier");

	FEclipseSoldierRecord& Wounded = State.Roster.AddDefaulted_GetRef();
	Wounded.SoldierId = FGuid(1, 0, 0, 2);
	Wounded.Name = TEXT("Wounded Soldier");
	Wounded.Status = EEclipseSoldierStatus::Wounded;
	Wounded.WoundedUntilDay = 15;

	FEclipseSoldierRecord& Dead = State.Roster.AddDefaulted_GetRef();
	Dead.SoldierId = FGuid(1, 0, 0, 3);
	Dead.Name = TEXT("Dead Soldier");
	Dead.Status = EEclipseSoldierStatus::Dead;

	FEclipseSoldierRecord& Second = State.Roster.AddDefaulted_GetRef();
	Second.SoldierId = FGuid(1, 0, 0, 4);
	Second.Name = TEXT("Second Ready");

	FString Error;
	TestTrue(TEXT("Two available soldiers pass"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId, Second.SoldierId }, 2, Error));
	TestFalse(TEXT("Wounded soldier unpickable (SPEC-P1-08 DoD)"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId, Wounded.SoldierId }, 2, Error));
	TestTrue(TEXT("Rejection names the person"), Error.Contains(TEXT("Wounded Soldier")));
	TestFalse(TEXT("Dead soldier unpickable"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId, Dead.SoldierId }, 2, Error));
	TestFalse(TEXT("Duplicate pick rejected"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId, Ready.SoldierId }, 2, Error));
	TestFalse(TEXT("Wrong size rejected"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId }, 2, Error));

	// Wounded soldier recovers with the clock — pickable again at day 15.
	State.Day = 15;
	TestTrue(TEXT("Recovered soldier pickable"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId, Wounded.SoldierId }, 2, Error));

	// Undeployable while assigned (SPEC-P2-03 staffing v1, step-3 review
	// finding 2): a base post blocks the muster even though the roster row
	// stays Available — the gate reads base state, not soldier status.
	FEclipseFacilityState& Post = State.BaseState.Facilities.AddDefaulted_GetRef();
	Post.SlotId = TEXT("Slot_D");
	Post.FacilityId = TEXT("IntelligenceCenter");
	Post.Level = 1;
	Post.AssignedSoldierIds.Add(Second.SoldierId);
	TestFalse(TEXT("Assigned soldier unpickable (undeployable while assigned)"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId, Second.SoldierId }, 2, Error));
	TestTrue(TEXT("Rejection names the person"), Error.Contains(TEXT("Second Ready")));
	TestTrue(TEXT("Rejection names the post"), Error.Contains(TEXT("IntelligenceCenter")));

	// Released from the post: deployable again, no residue.
	Post.AssignedSoldierIds.Reset();
	TestTrue(TEXT("Released soldier pickable again"),
		EclipsePrepLogic::ValidateSquadPick(State, { Ready.SoldierId, Second.SoldierId }, 2, Error));

	// Loadout gating (SPEC-P1-03 production gate).
	const TArray<EclipsePrepLogic::FEclipseLoadoutOption> Loadouts = EclipsePrepTest::MakeLoadouts();
	TestTrue(TEXT("Base loadout always legal"),
		EclipsePrepLogic::ValidateLoadout(State, Loadouts, EclipseTags::Resource_Credits.GetTag(), Error));
	TestFalse(TEXT("Gated loadout illegal before production"),
		EclipsePrepLogic::ValidateLoadout(State, Loadouts, EclipseTags::Resource_Materials.GetTag(), Error));
	TestTrue(TEXT("Rejection names the Workshop"), Error.Contains(TEXT("Workshop")));

	State.UnlockedLoadoutTags.Add(EclipseTags::Resource_Intel.GetTag());
	TestTrue(TEXT("Gated loadout legal after production"),
		EclipsePrepLogic::ValidateLoadout(State, Loadouts, EclipseTags::Resource_Materials.GetTag(), Error));
	TestEqual(TEXT("Available list grows with the unlock"),
		EclipsePrepLogic::GetAvailableLoadouts(State, Loadouts).Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipsePrepFullCircleTest,
	"Eclipse.Base.Prep.FullCircleSmoke",
	EclipsePrepTest::TestFlags)

bool FEclipsePrepFullCircleTest::RunTest(const FString& Parameters)
{
	// SPEC-P1-08's functional contract: advance day -> select -> prep (intel
	// spend) -> launch -> force-win -> consequences visible -> second loop
	// launches. All through subsystem APIs — exactly what the hub's buttons call.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	// Campaign data in-memory: two regions, one offer, one base loadout, prep tuning.
	UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
	{
		FEclipseRegionDefinition& Home = Graph->Regions.AddDefaulted_GetRef();
		Home.RegionId = TEXT("Underworks");
		Home.StartingOwner = EEclipseRegionOwner::Player;
		Home.ConnectedRegionIds = { TEXT("Checkpoint") };
		FEclipseRegionDefinition& Target = Graph->Regions.AddDefaulted_GetRef();
		Target.RegionId = TEXT("Checkpoint");
		Target.RegionType = EEclipseRegionType::Checkpoint;
		Target.StartingOwner = EEclipseRegionOwner::Dominion;
		Target.ConnectedRegionIds = { TEXT("Underworks") };

		UDataTable* Offers = NewObject<UDataTable>();
		Offers->RowStruct = FEclipseMissionOfferRow::StaticStruct();
		FEclipseMissionOfferRow Offer;
		Offer.RegionType = EEclipseRegionType::Checkpoint;
		Offer.TemplateId = TEXT("MT_Assault");
		Offer.RewardCredits = 60;
		Offers->AddRow(TEXT("Offer_Checkpoint"), Offer);
		Graph->MissionOffers = Offers;
	}

	UEclipsePrepTuningAsset* PrepTuning = NewObject<UEclipsePrepTuningAsset>();
	PrepTuning->IntelRevealCost = 5;
	PrepTuning->SquadSize = 2;
	{
		UDataTable* Loadouts = NewObject<UDataTable>();
		Loadouts->RowStruct = FEclipseLoadoutOptionRow::StaticStruct();
		FEclipseLoadoutOptionRow BaseRow;
		BaseRow.DisplayName = FText::FromString(TEXT("Scavenged arms"));
		BaseRow.LoadoutTag = EclipseTags::Resource_Credits.GetTag();
		Loadouts->AddRow(TEXT("Loadout_Scavenged"), BaseRow);
		PrepTuning->LoadoutOptions = Loadouts;
	}

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 3;
	Setup->StartingResources.Add(EclipseTags::Resource_Intel.GetTag(), 10);
	Setup->RegionGraph = Graph;
	Setup->PrepTuning = PrepTuning;
	Campaign->StartNewCampaign(Setup);

	FString Error;

	// Loop #1.
	TestTrue(TEXT("Offer selected"), Strategy->SelectMission(TEXT("Checkpoint"), Error));
	TestTrue(TEXT("Intel spend commits"), Prep->SpendIntelForReveal(Error));
	TestEqual(TEXT("Intel balance reduced by the reveal"), Campaign->GetState().GetBalance(EclipseTags::Resource_Intel.GetTag()), 5);
	TestTrue(TEXT("Briefing revealed"), Prep->WasIntelRevealed());

	TArray<FGuid> Squad;
	Squad.Add(Campaign->GetState().Roster[0].SoldierId);
	Squad.Add(Campaign->GetState().Roster[1].SoldierId);
	TestTrue(TEXT("Launch accepted"), Prep->LaunchMission(Squad, EclipseTags::Resource_Credits.GetTag(), TEXT("Entry_Sewer"), Error));
	TestTrue(TEXT("Mission running (launch event consumed)"), Mission->GetPhase() == EEclipseMissionPhase::Objectives);

	TestTrue(TEXT("Objectives scripted"), Mission->CompleteObjective(TEXT("Obj_Primary"), Error) && Mission->CompleteObjective(TEXT("Obj_Exfil"), Error));
	TestTrue(TEXT("Debrief commits"), Mission->ResolveDebrief(true, Error));
	TestEqual(TEXT("Debrief advanced the clock by exactly one day (SPEC-P2-03 locked decision 4)"), Campaign->GetState().Day, 2);
	TestEqual(TEXT("Reward visible in wallet"), Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()), 60);
	TestTrue(TEXT("Board changed"), Campaign->GetState().FindRegion(TEXT("Checkpoint"))->Owner == EEclipseRegionOwner::Contested);

	// Loop #2 — the gate question.
	TestTrue(TEXT("Second offer selectable"), Strategy->SelectMission(TEXT("Checkpoint"), Error));
	TestTrue(TEXT("Second launch accepted"), Prep->LaunchMission(Squad, EclipseTags::Resource_Credits.GetTag(), TEXT("Entry_Main"), Error));
	TestTrue(TEXT("Second mission running — the loop closes"), Mission->GetPhase() == EEclipseMissionPhase::Objectives);

	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
