// SPEC-P2-04 build step 1 — the R7 falsification: does the quest runtime AS
// BUILT carry an *authored* mission asset end to end? M1.1's skeleton (the
// real objective shape, no story layer: hardcoded offer pinning per the spec's
// step-1 note) runs spawn -> complete-by-script -> debrief, and the asserts
// are the risk verdict: authored objectives active (not the synthesized
// fallback), mandatory set enforced, rewards committed, the clock advanced by
// exactly one day, and the region UNTOUCHED (bProgressRegionOnSuccess=false —
// SPEC-P2-04 decision 6: M1.3 via the P2-05 seam is the only world-state
// change). Green here authorizes M1.2-M1.4 authoring; red means the runtime
// is amended first, never worked around per-mission (decision 2).

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipsePrepSubsystem.h"
#include "Base/EclipsePrepTypes.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Quests/EclipseMissionTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "UObject/Package.h"

namespace EclipseMissionM1Test
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionM11SkeletonTest,
	"Eclipse.Missions.M11SkeletonCarriesAuthoredAsset",
	EclipseMissionM1Test::TestFlags)

bool FEclipseMissionM11SkeletonTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	// The authored asset, registered in-memory at the exact soft path
	// ResolveMissionSpec loads from — TryLoad finds live objects before disk,
	// so the runtime code stays untouched (the skeleton tests the runtime,
	// not the content pipeline).
	UPackage* MissionPackage = CreatePackage(TEXT("/Game/Data/Missions/MT_M11Skeleton"));
	UEclipseMissionAsset* M11 = NewObject<UEclipseMissionAsset>(MissionPackage, TEXT("MT_M11Skeleton"), RF_Public | RF_Standalone);
	M11->TemplateId = TEXT("MT_M11Skeleton");
	M11->DisplayName = FText::FromString(TEXT("Thirteen Bullets (skeleton)"));
	M11->bProgressRegionOnSuccess = false;
	{
		FEclipseObjectiveDef& Ambush = M11->Objectives.AddDefaulted_GetRef();
		Ambush.ObjectiveId = TEXT("Obj_M11_PatrolLeader");
		Ambush.Type = EEclipseObjectiveType::DestroyTarget;
		Ambush.Description = FText::FromString(TEXT("Spring the ambush: take out the patrol leader"));
		Ambush.TargetId = TEXT("Site_M11_Overlook");

		FEclipseObjectiveDef& Exfil = M11->Objectives.AddDefaulted_GetRef();
		Exfil.ObjectiveId = TEXT("Obj_M11_Exfil");
		Exfil.Type = EEclipseObjectiveType::ExtractSquad;
		Exfil.Description = FText::FromString(TEXT("Extract the squad"));
		Exfil.TargetId = TEXT("Site_M11_Extraction");
	}

	// Board + hardcoded pinning stand-in: one offer whose TemplateId is the
	// authored asset; DT_StoryMissions pinning replaces this in build step 2.
	UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
	{
		FEclipseRegionDefinition& Home = Graph->Regions.AddDefaulted_GetRef();
		Home.RegionId = TEXT("Underworks");
		Home.StartingOwner = EEclipseRegionOwner::Player;
		Home.ConnectedRegionIds = { TEXT("AmbushBlock") };
		FEclipseRegionDefinition& Target = Graph->Regions.AddDefaulted_GetRef();
		Target.RegionId = TEXT("AmbushBlock");
		Target.RegionType = EEclipseRegionType::Checkpoint;
		Target.StartingOwner = EEclipseRegionOwner::Dominion;
		Target.ConnectedRegionIds = { TEXT("Underworks") };

		UDataTable* Offers = NewObject<UDataTable>();
		Offers->RowStruct = FEclipseMissionOfferRow::StaticStruct();
		FEclipseMissionOfferRow Offer;
		Offer.RegionType = EEclipseRegionType::Checkpoint;
		Offer.TemplateId = TEXT("MT_M11Skeleton");
		Offer.RewardMaterials = 25; // M1.1 band (SPEC-P2-04 decision 10)
		Offer.RewardCredits = 50;
		Offers->AddRow(TEXT("Offer_M11"), Offer);
		Graph->MissionOffers = Offers;
	}

	UEclipsePrepTuningAsset* PrepTuning = NewObject<UEclipsePrepTuningAsset>();
	PrepTuning->SquadSize = 2;
	{
		UDataTable* Loadouts = NewObject<UDataTable>();
		Loadouts->RowStruct = FEclipseLoadoutOptionRow::StaticStruct();
		FEclipseLoadoutOptionRow Scavenged;
		Scavenged.DisplayName = FText::FromString(TEXT("Scavenged arms"));
		Scavenged.LoadoutTag = EclipseTags::Resource_Credits.GetTag();
		Loadouts->AddRow(TEXT("Loadout_Scavenged"), Scavenged);
		PrepTuning->LoadoutOptions = Loadouts;
	}

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 3;
	Setup->RegionGraph = Graph;
	Setup->PrepTuning = PrepTuning;
	Campaign->StartNewCampaign(Setup);

	FString Error;
	TestTrue(TEXT("M1.1 offer selected"), Strategy->SelectMission(TEXT("AmbushBlock"), Error));

	TArray<FGuid> Squad;
	Squad.Add(Campaign->GetState().Roster[0].SoldierId);
	Squad.Add(Campaign->GetState().Roster[1].SoldierId);
	TestTrue(TEXT("Launch accepted"), Prep->LaunchMission(Squad, EclipseTags::Resource_Credits.GetTag(), TEXT("Entry_Main"), Error));
	TestTrue(TEXT("Mission running"), Mission->GetPhase() == EEclipseMissionPhase::Objectives);

	// THE R7 CORE: the runtime resolved the authored asset, not the
	// synthesized fallback (which would be Obj_Primary/Obj_Exfil).
	const TArray<FEclipseObjectiveDef>& Active = Mission->GetActiveObjectives();
	TestEqual(TEXT("Authored objective count active"), Active.Num(), 2);
	TestTrue(TEXT("Authored ambush objective active (no synthesized fallback)"),
		Active.Num() == 2 && Active[0].ObjectiveId == TEXT("Obj_M11_PatrolLeader") && Active[0].Type == EEclipseObjectiveType::DestroyTarget);

	// Mandatory-set enforcement: extraction still open -> success must reject.
	TestTrue(TEXT("Ambush objective completes by script"), Mission->CompleteObjective(TEXT("Obj_M11_PatrolLeader"), Error));
	TestFalse(TEXT("Debrief success rejected while extraction is open"), Mission->ResolveDebrief(true, Error));

	TestTrue(TEXT("Extraction completes by script"), Mission->CompleteObjective(TEXT("Obj_M11_Exfil"), Error));
	TestTrue(TEXT("Debrief commits"), Mission->ResolveDebrief(true, Error));

	// Consequence asserts — the risk verdict.
	TestEqual(TEXT("M1.1 materials committed"), Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag()), 25);
	TestEqual(TEXT("M1.1 credits committed"), Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()), 50);
	TestEqual(TEXT("Clock advanced exactly one day"), Campaign->GetState().Day, 2);
	TestTrue(TEXT("Region untouched (M1.1 flips nothing — decision 6)"),
		Campaign->GetState().FindRegion(TEXT("AmbushBlock"))->Owner == EEclipseRegionOwner::Dominion);

	// GC hygiene (review): drop the keep-alive flags on object AND package so
	// the transient asset is collectable; the unique MT_M11Skeleton id also
	// guarantees the real MT_M11.uasset (build step 2) can never be shadowed
	// or replaced in-place by this test.
	M11->ClearFlags(RF_Public | RF_Standalone);
	MissionPackage->ClearFlags(RF_Public | RF_Standalone);
	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
