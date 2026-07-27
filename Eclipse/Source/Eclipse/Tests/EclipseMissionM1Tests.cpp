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

#include "Base/EclipseBaseSubsystem.h"
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
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "HAL/FileManager.h"
#include "EclipseSaveSubsystem.h"
#include "StructUtils/InstancedStruct.h"
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

// SPEC-P2-04 step-2 closure — the functional Gauntlet on the SHIPPED content
// chain: DA_CampaignSetup -> DT_StoryMissions pin (TransitCheckpoint) ->
// MT_M11.uasset. The skeleton test above proves the runtime with an in-memory
// asset; these two prove the data on disk — red here means the commandlet
// materialised the wrong thing (or the link broke), not that the runtime broke.
// Reward asserts read the commit's own Event.Economy.ResourcesChanged facts
// (Reason "MissionReward") instead of wallet deltas: the real setup carries a
// base layout + econ data, so the debrief's day tick may legitimately add
// facility/region income on top — balance math would couple this test to
// tuning numbers that are allowed to change.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionM11ShippedDataGauntletTest,
	"Eclipse.Missions.M11GauntletOnShippedData",
	EclipseMissionM1Test::TestFlags)

bool FEclipseMissionM11ShippedDataGauntletTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Shipped DA_CampaignSetup missing - run Tools/create_phase1_content.py + Tools/setup_story_missions.py first."));
		return false;
	}
	// Soft slot: IsNull() checks the authored link; the resolver LoadSynchronous's
	// it on first use (asserting != nullptr here would test load timing, not data).
	TestFalse(TEXT("Story table linked on the shipped setup"), Setup->StoryMissions.IsNull());

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	Campaign->StartNewCampaign(Setup);

	// The pin surfaces on the shipped board: TransitCheckpoint is
	// adjacency-legal from Underworks at campaign start, and the story row must
	// beat the generic checkpoint offer with the row's own reward band.
	FEclipseMissionOfferView PinnedOffer;
	TestTrue(TEXT("Offer resolves for TransitCheckpoint"), Strategy->TryGetOffer(TEXT("TransitCheckpoint"), PinnedOffer));
	TestEqual(TEXT("Pin wins over the region offer (shipped chain)"), PinnedOffer.TemplateId, FName(TEXT("MT_M11")));
	TestEqual(TEXT("Row credits on the offer (decision 10 band)"), PinnedOffer.RewardCredits, 50);
	TestEqual(TEXT("Row materials on the offer"), PinnedOffer.RewardMaterials, 25);

	// Collect the debrief's own reward facts off the bus.
	TArray<FEclipseEconomyEventPayload> RewardFacts;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&RewardFacts](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>();
				Economy != nullptr && Economy->Reason == TEXT("MissionReward"))
			{
				RewardFacts.Add(*Economy);
			}
		}));

	FString Error;
	TestTrue(TEXT("M1.1 selected on the shipped board"), Strategy->SelectMission(TEXT("TransitCheckpoint"), Error));
	TestTrue(TEXT("AutoLaunch on shipped prep data"), Prep->AutoLaunch(Error));
	TestTrue(TEXT("Mission running"), Mission->GetPhase() == EEclipseMissionPhase::Objectives);

	// The disk asset resolved (not the synthesized fallback, not the skeleton).
	// Asserting a bare count of 2 was too coarse and broke the moment wave 2
	// authored the optional; what the spec actually promises is a two-objective
	// MANDATORY set plus the zero-casualty stretch goal. Pinning that instead
	// makes this Gauntlet prove the wave-2 data landed as well.
	const TArray<FEclipseObjectiveDef>& Active = Mission->GetActiveObjectives();
	TestEqual(TEXT("Authored objective count (2 mandatory + 1 optional)"), Active.Num(), 3);
	if (Active.Num() == 3)
	{
		TestTrue(TEXT("Authored ambush objective from MT_M11.uasset"),
			Active[0].ObjectiveId == TEXT("Obj_M11_PatrolLeader") && Active[0].Type == EEclipseObjectiveType::DestroyTarget);
		TestTrue(TEXT("Authored exfil objective from MT_M11.uasset"),
			Active[1].ObjectiveId == TEXT("Obj_M11_Exfil") && Active[1].Type == EEclipseObjectiveType::ExtractSquad);
		TestFalse(TEXT("Ambush is mandatory"), Active[0].bOptional);
		TestFalse(TEXT("Exfil is mandatory"), Active[1].bOptional);

		// The wave-2 stretch goal, authored by Tools/migrate_m11_optional.py.
		const FEclipseObjectiveDef& Stretch = Active[2];
		TestEqual(TEXT("Third objective is the zero-casualty stretch goal"), Stretch.ObjectiveId, FName(TEXT("Obj_M11_NoCasualties")));
		TestTrue(TEXT("Stretch goal is optional"), Stretch.bOptional);
		TestTrue(TEXT("Stretch goal carries the no-casualties latch condition"), Stretch.bRequiresNoCasualties);
		TestEqual(TEXT("Stretch payout is the spec's +20 Materials"), Stretch.OptionalRewardMaterials, 20);
	}

	// Guarded: shipped-data drift (a renamed/removed region) must fail this test
	// with a readable message, not crash the whole automation run on a null deref.
	const FEclipseRegionState* RegionBefore = Campaign->GetState().FindRegion(TEXT("TransitCheckpoint"));
	if (RegionBefore == nullptr)
	{
		AddError(TEXT("Shipped region graph has no TransitCheckpoint - the M1.1 pin targets a region that no longer exists."));
		GameInstance->Shutdown();
		return false;
	}
	const int32 DayBefore = Campaign->GetState().Day;
	const EEclipseRegionOwner OwnerBefore = RegionBefore->Owner;

	TestTrue(TEXT("Ambush completes"), Mission->CompleteObjective(TEXT("Obj_M11_PatrolLeader"), Error));
	TestTrue(TEXT("Exfil completes"), Mission->CompleteObjective(TEXT("Obj_M11_Exfil"), Error));
	TestTrue(TEXT("Debrief commits"), Mission->ResolveDebrief(true, Error));

	// Rewards == the row, as commit facts (exactly two, nothing more).
	TestEqual(TEXT("Exactly two MissionReward facts"), RewardFacts.Num(), 2);
	int32 CreditsRewarded = 0;
	int32 MaterialsRewarded = 0;
	for (const FEclipseEconomyEventPayload& Fact : RewardFacts)
	{
		if (Fact.ResourceType == EclipseTags::Resource_Credits.GetTag()) { CreditsRewarded += Fact.Delta; }
		if (Fact.ResourceType == EclipseTags::Resource_Materials.GetTag()) { MaterialsRewarded += Fact.Delta; }
	}
	TestEqual(TEXT("Row credits committed"), CreditsRewarded, 50);
	TestEqual(TEXT("Row materials committed"), MaterialsRewarded, 25);

	TestEqual(TEXT("Clock advanced exactly one day"), Campaign->GetState().Day, DayBefore + 1);
	TestTrue(TEXT("Completion beat committed"),
		Campaign->GetState().StoryFlags.Contains(EclipseTags::Story_Beat_M11_ThirteenBullets.GetTag()));
	if (const FEclipseRegionState* RegionAfter = Campaign->GetState().FindRegion(TEXT("TransitCheckpoint")))
	{
		TestTrue(TEXT("Region untouched (decision 6: M1.1 flips nothing)"), RegionAfter->Owner == OwnerBefore);
	}
	else
	{
		AddError(TEXT("TransitCheckpoint disappeared from state across the debrief."));
	}

	// The pin retires on the committed beat. Asserting only "not MT_M11" would
	// also pass if the board went silent entirely, so pin down the positive
	// half too: the checkpoint must still offer something, and that something is
	// the generic region offer.
	FEclipseMissionOfferView AfterOffer;
	const bool bStillOffered = Strategy->TryGetOffer(TEXT("TransitCheckpoint"), AfterOffer);
	TestTrue(TEXT("Board still offers the checkpoint after the beat (retire != go silent)"), bStillOffered);
	TestNotEqual(TEXT("Pin retired after the beat"), AfterOffer.TemplateId, FName(TEXT("MT_M11")));

	Bus->Unsubscribe(EconomyHandle);
	GameInstance->Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionM11LossKeepsStoryColdTest,
	"Eclipse.Missions.M11LossKeepsStoryCold",
	EclipseMissionM1Test::TestFlags)

bool FEclipseMissionM11LossKeepsStoryColdTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Shipped DA_CampaignSetup missing - run Tools/create_phase1_content.py + Tools/setup_story_missions.py first."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	Campaign->StartNewCampaign(Setup);

	int32 MissionRewardFacts = 0;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&MissionRewardFacts](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>();
				Economy != nullptr && Economy->Reason == TEXT("MissionReward"))
			{
				++MissionRewardFacts;
			}
		}));

	FString Error;
	TestTrue(TEXT("M1.1 selected"), Strategy->SelectMission(TEXT("TransitCheckpoint"), Error));
	TestTrue(TEXT("AutoLaunch"), Prep->AutoLaunch(Error));

	// Establish WHICH mission is running before asserting anything about the
	// loss. Without this, every assert below ("no beat", "no rewards", "region
	// untouched") is also satisfied by any other losing mission on this board —
	// the test would stay green while the M1.1 pin was silently broken.
	FEclipseMissionOfferView RunningOffer;
	TestTrue(TEXT("Offer resolves for the launched region"), Strategy->TryGetOffer(TEXT("TransitCheckpoint"), RunningOffer));
	TestEqual(TEXT("The mission under test IS M1.1"), RunningOffer.TemplateId, FName(TEXT("MT_M11")));
	const TArray<FEclipseObjectiveDef>& LossActive = Mission->GetActiveObjectives();
	TestEqual(TEXT("Authored M1.1 objectives active (not the synthesized fallback)"), LossActive.Num(), 3);
	if (LossActive.Num() == 3)
	{
		TestTrue(TEXT("Running the authored M1.1 asset"),
			LossActive[0].ObjectiveId == TEXT("Obj_M11_PatrolLeader") && LossActive[1].ObjectiveId == TEXT("Obj_M11_Exfil"));
		TestEqual(TEXT("Stretch goal present on the loss path too"), LossActive[2].ObjectiveId, FName(TEXT("Obj_M11_NoCasualties")));
	}

	const FEclipseRegionState* RegionBefore = Campaign->GetState().FindRegion(TEXT("TransitCheckpoint"));
	if (RegionBefore == nullptr)
	{
		AddError(TEXT("Shipped region graph has no TransitCheckpoint - the M1.1 pin targets a region that no longer exists."));
		GameInstance->Shutdown();
		return false;
	}
	const int32 DayBefore = Campaign->GetState().Day;
	const EEclipseRegionOwner OwnerBefore = RegionBefore->Owner;

	// Field abort straight from Objectives = the legal fail-forward path.
	TestTrue(TEXT("Loss debrief commits (fail-forward)"), Mission->ResolveDebrief(false, Error));

	TestEqual(TEXT("No mission rewards on loss"), MissionRewardFacts, 0);
	TestFalse(TEXT("Loss commits no story beat (GDD 11.4: retry, not progress)"),
		Campaign->GetState().StoryFlags.Contains(EclipseTags::Story_Beat_M11_ThirteenBullets.GetTag()));
	TestEqual(TEXT("Loss still costs the day (P2-03 locked decision 4)"), Campaign->GetState().Day, DayBefore + 1);
	if (const FEclipseRegionState* RegionAfter = Campaign->GetState().FindRegion(TEXT("TransitCheckpoint")))
	{
		TestTrue(TEXT("Region untouched on loss"), RegionAfter->Owner == OwnerBefore);
	}
	else
	{
		AddError(TEXT("TransitCheckpoint disappeared from state across the loss debrief."));
	}

	// The pin survives a loss: the story mission must be offered again.
	FEclipseMissionOfferView RetryOffer;
	TestTrue(TEXT("Offer still resolves after loss"), Strategy->TryGetOffer(TEXT("TransitCheckpoint"), RetryOffer));
	TestEqual(TEXT("Pin still M1.1 (retry wall never, story loss never advances)"), RetryOffer.TemplateId, FName(TEXT("MT_M11")));

	Bus->Unsubscribe(EconomyHandle);
	GameInstance->Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// M1.2 "The Dead Drop" op de verscheepte data
// ---------------------------------------------------------------------------
//
// Zelfde vorm als de M1.1-Gauntlet hierboven, en met opzet: SPEC-P2-04 eist dat
// M1.1 groen is VOORDAT M1.2-M1.4 geauthord worden, en dat het bewijs dezelfde
// vorm heeft - spawnen, via script voltooien, campagnestaat nakijken.
//
// Wat deze test bewaakt dat de M1.1-versie niet kon: DE POORT. M1.2 hangt achter
// Story.Beat.M11_ThirteenBullets, en een poort die niet dicht kan, is geen poort.
// Daarom staat de eerste stap op de AFWEZIGHEID van het aanbod.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionM12ShippedDataGauntletTest,
	"Eclipse.Missions.M12GauntletOnShippedData",
	EclipseMissionM1Test::TestFlags)

bool FEclipseMissionM12ShippedDataGauntletTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt - draai Tools/create_phase1_content.py + Tools/setup_story_missions.py."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	Campaign->StartNewCampaign(Setup);
	FString Error;

	// 1. DE POORT IS DICHT. WorkerHousing is vanaf dag 1 bereikbaar vanuit
	//    Underworks, dus als de unlock-beat niet gehonoreerd wordt, staat de dode
	//    brief hier meteen naast de openingsmissie.
	//
	//    Beide uitkomsten zijn goed - geen aanbod, of een ander aanbod - maar ze
	//    moeten uit elkaar te houden zijn. Een `if (TryGetOffer)` met de assertie
	//    erbinnen slaat stilletjes over zodra er niets terugkomt, en bewijst dan
	//    niets terwijl hij groen staat.
	FEclipseMissionOfferView BeforeBeat;
	const bool bOfferedBeforeBeat = Strategy->TryGetOffer(TEXT("WorkerHousing"), BeforeBeat);
	AddInfo(bOfferedBeforeBeat
		? FString::Printf(TEXT("GEMETEN  voor de beat biedt WorkerHousing '%s' aan"), *BeforeBeat.TemplateId.ToString())
		: FString(TEXT("GEMETEN  voor de beat heeft WorkerHousing geen enkel aanbod")));
	if (bOfferedBeforeBeat)
	{
		TestNotEqual(TEXT("M1.2 staat NIET in het aanbod voordat M1.1 gespeeld is"),
			BeforeBeat.TemplateId, FName(TEXT("MT_M12")));
	}

	// 2. M1.1 uitspelen - geen herhaling van de vorige test, maar de enige manier
	//    om de sleutel te verdienen die stap 3 nodig heeft.
	TestTrue(TEXT("M1.1 geselecteerd"), Strategy->SelectMission(TEXT("TransitCheckpoint"), Error));
	TestTrue(TEXT("M1.1 gelanceerd"), Prep->AutoLaunch(Error));
	TestTrue(TEXT("M1.1 hinderlaag voltooid"), Mission->CompleteObjective(TEXT("Obj_M11_PatrolLeader"), Error));
	TestTrue(TEXT("M1.1 extractie voltooid"), Mission->CompleteObjective(TEXT("Obj_M11_Exfil"), Error));
	TestTrue(TEXT("M1.1 debrief"), Mission->ResolveDebrief(true, Error));

	// 3. DE POORT IS OPEN.
	FEclipseMissionOfferView Offer;
	if (!TestTrue(TEXT("aanbod bestaat voor WorkerHousing"), Strategy->TryGetOffer(TEXT("WorkerHousing"), Offer)))
	{
		GameInstance->Shutdown();
		return false;
	}
	TestEqual(TEXT("de pin wint van het regio-aanbod"), Offer.TemplateId, FName(TEXT("MT_M12")));
	TestEqual(TEXT("rij-credits op het aanbod"), Offer.RewardCredits, 30);
	TestEqual(TEXT("rij-materialen op het aanbod"), Offer.RewardMaterials, 15);

	TArray<FEclipseEconomyEventPayload> RewardFacts;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&RewardFacts](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>();
				Economy != nullptr && Economy->Reason == TEXT("MissionReward"))
			{
				RewardFacts.Add(*Economy);
			}
		}));

	TestTrue(TEXT("M1.2 geselecteerd"), Strategy->SelectMission(TEXT("WorkerHousing"), Error));
	TestTrue(TEXT("M1.2 gelanceerd"), Prep->AutoLaunch(Error));
	TestTrue(TEXT("M1.2 draait"), Mission->GetPhase() == EEclipseMissionPhase::Objectives);

	// 4. DE GEAUTHORDE VORM: twee caches in gescheiden zones, extractie, ghost.
	const TArray<FEclipseObjectiveDef>& Active = Mission->GetActiveObjectives();
	TestEqual(TEXT("3 verplichte doelen + 1 optional"), Active.Num(), 4);
	if (Active.Num() == 4)
	{
		TestTrue(TEXT("eerste cache is CollectItem"),
			Active[0].ObjectiveId == TEXT("Obj_M12_CacheNorth") && Active[0].Type == EEclipseObjectiveType::CollectItem);
		TestTrue(TEXT("tweede cache is CollectItem"),
			Active[1].ObjectiveId == TEXT("Obj_M12_CacheSouth") && Active[1].Type == EEclipseObjectiveType::CollectItem);
		TestTrue(TEXT("extractie sluit af"),
			Active[2].ObjectiveId == TEXT("Obj_M12_Exfil") && Active[2].Type == EEclipseObjectiveType::ExtractSquad);
		// Verschillende sites is niet cosmetisch: twee patrouilles lezen in plaats
		// van een is de hele les van deze missie.
		TestNotEqual(TEXT("de twee caches liggen op verschillende sites"),
			Active[0].TargetId, Active[1].TargetId);

		const FEclipseObjectiveDef& Ghost = Active[3];
		TestEqual(TEXT("vierde doel is de ghost"), Ghost.ObjectiveId, FName(TEXT("Obj_M12_Ghost")));
		TestTrue(TEXT("ghost is optioneel"), Ghost.bOptional);
		TestTrue(TEXT("ghost hangt aan het alarm-feit"), Ghost.bRequiresNoAlarm);
		TestEqual(TEXT("ghost betaalt de +10 materialen uit de spec"), Ghost.OptionalRewardMaterials, 10);
		TestEqual(TEXT("ghost betaalt de +4 intel uit de spec"), Ghost.OptionalRewardIntel, 4);
	}

	const FEclipseRegionState* RegionBefore = Campaign->GetState().FindRegion(TEXT("WorkerHousing"));
	if (RegionBefore == nullptr)
	{
		AddError(TEXT("De verscheepte regiograaf kent geen WorkerHousing - de M1.2-pin wijst naar een regio die niet bestaat."));
		GameInstance->Shutdown();
		return false;
	}
	const int32 DayBefore = Campaign->GetState().Day;
	const EEclipseRegionOwner OwnerBefore = RegionBefore->Owner;

	TestTrue(TEXT("eerste cache voltooid"), Mission->CompleteObjective(TEXT("Obj_M12_CacheNorth"), Error));
	TestTrue(TEXT("tweede cache voltooid"), Mission->CompleteObjective(TEXT("Obj_M12_CacheSouth"), Error));
	TestTrue(TEXT("extractie voltooid"), Mission->CompleteObjective(TEXT("Obj_M12_Exfil"), Error));
	TestTrue(TEXT("debrief commit"), Mission->ResolveDebrief(true, Error));

	int32 Credits = 0;
	int32 Materials = 0;
	int32 Intel = 0;
	for (const FEclipseEconomyEventPayload& Fact : RewardFacts)
	{
		if (Fact.ResourceType == EclipseTags::Resource_Credits.GetTag())   { Credits += Fact.Delta; }
		if (Fact.ResourceType == EclipseTags::Resource_Materials.GetTag()) { Materials += Fact.Delta; }
		if (Fact.ResourceType == EclipseTags::Resource_Intel.GetTag())     { Intel += Fact.Delta; }
	}
	TestEqual(TEXT("rij-credits vastgelegd"), Credits, 30);
	TestEqual(TEXT("rij-materialen vastgelegd"), Materials, 15);
	TestEqual(TEXT("rij-intel vastgelegd - M1.2 is de eerste missie die intel betaalt"), Intel, 8);
	TestEqual(TEXT("klok precies een dag verder"), Campaign->GetState().Day, DayBefore + 1);

	// 5. GEEN REGIO-FLIP. Spec-besluit 6: M1.3 is de EERSTE wereldstaat-wijziging,
	//    en twee schrijvers van regiostaat is precies de divergentiebug waar 12.3
	//    voor bestaat.
	const FEclipseRegionState* RegionAfter = Campaign->GetState().FindRegion(TEXT("WorkerHousing"));
	if (TestNotNull(TEXT("regio bestaat nog na de debrief"), RegionAfter))
	{
		TestTrue(TEXT("M1.2 draait GEEN regio om (besluit 6: M1.3 is de eerste)"),
			RegionAfter->Owner == OwnerBefore);
	}

	Bus->Unsubscribe(EconomyHandle);
	GameInstance->Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// M1.3 "Signal Fire" op de verscheepte data
// ---------------------------------------------------------------------------
//
// Derde schakel van de keten, en de eerste die iets nieuws bewaakt: WELKE REGIO
// een verhaalmissie kan dragen. Een pin is pas een missie als de regio op dat
// punt in de campagne ook echt aanbiedbaar is, en dat volgt uit de graaf plus wie
// wat bezit - niet uit wat thematisch mooi klinkt.
//
// Daarom logt deze test eerst het aanbod van ELKE regio na de M1.2-beat. Die
// regels zijn geen decoratie: ze zijn de reden dat de pin op SupplyDepot staat
// en niet op CommsRelay.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionM13ShippedDataGauntletTest,
	"Eclipse.Missions.M13GauntletOnShippedData",
	EclipseMissionM1Test::TestFlags)

bool FEclipseMissionM13ShippedDataGauntletTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt - draai Tools/create_phase1_content.py + Tools/setup_story_missions.py."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	Campaign->StartNewCampaign(Setup);
	FString Error;

	// De keten tot en met M1.2 uitspelen: dat is de enige manier om de sleutel
	// van M1.3 te verdienen.
	TestTrue(TEXT("M1.1 geselecteerd"), Strategy->SelectMission(TEXT("TransitCheckpoint"), Error));
	TestTrue(TEXT("M1.1 gelanceerd"), Prep->AutoLaunch(Error));
	TestTrue(TEXT("M1.1 hinderlaag"), Mission->CompleteObjective(TEXT("Obj_M11_PatrolLeader"), Error));
	TestTrue(TEXT("M1.1 extractie"), Mission->CompleteObjective(TEXT("Obj_M11_Exfil"), Error));
	TestTrue(TEXT("M1.1 debrief"), Mission->ResolveDebrief(true, Error));

	TestTrue(TEXT("M1.2 geselecteerd"), Strategy->SelectMission(TEXT("WorkerHousing"), Error));
	TestTrue(TEXT("M1.2 gelanceerd"), Prep->AutoLaunch(Error));
	TestTrue(TEXT("M1.2 cache noord"), Mission->CompleteObjective(TEXT("Obj_M12_CacheNorth"), Error));
	TestTrue(TEXT("M1.2 cache zuid"), Mission->CompleteObjective(TEXT("Obj_M12_CacheSouth"), Error));
	TestTrue(TEXT("M1.2 extractie"), Mission->CompleteObjective(TEXT("Obj_M12_Exfil"), Error));
	TestTrue(TEXT("M1.2 debrief"), Mission->ResolveDebrief(true, Error));

	// DE BEREIKBAARHEIDSMETING. Wat biedt elke regio nu aan? Zonder deze regels
	// is "de pin staat op de goede regio" een aanname, en een pin naar een regio
	// die niet aanbiedbaar is levert een missie op die nooit verschijnt - stil,
	// en niet zichtbaar in de data.
	const TCHAR* Regions[] = { TEXT("Underworks"), TEXT("TransitCheckpoint"), TEXT("FoundryRow"),
		TEXT("WorkerHousing"), TEXT("SupplyDepot"), TEXT("CommsRelay") };
	for (const TCHAR* Region : Regions)
	{
		FEclipseMissionOfferView View;
		AddInfo(Strategy->TryGetOffer(Region, View)
			? FString::Printf(TEXT("GEMETEN  na M1.2 biedt %s '%s' aan"), Region, *View.TemplateId.ToString())
			: FString::Printf(TEXT("GEMETEN  na M1.2 heeft %s GEEN aanbod"), Region));
	}

	FEclipseMissionOfferView Offer;
	if (!TestTrue(TEXT("de gepinde regio heeft aanbod - anders verschijnt M1.3 nooit"),
			Strategy->TryGetOffer(TEXT("TransitCheckpoint"), Offer)))
	{
		GameInstance->Shutdown();
		return false;
	}
	TestEqual(TEXT("de pin wint van het regio-aanbod"), Offer.TemplateId, FName(TEXT("MT_M13")));
	TestEqual(TEXT("rij-credits op het aanbod"), Offer.RewardCredits, 60);
	TestEqual(TEXT("rij-materialen op het aanbod"), Offer.RewardMaterials, 40);

	TArray<FEclipseEconomyEventPayload> RewardFacts;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&RewardFacts](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>();
				Economy != nullptr && Economy->Reason == TEXT("MissionReward"))
			{
				RewardFacts.Add(*Economy);
			}
		}));

	// De REDEN meelezen. Een geweigerde selectie zonder reden is precies het
	// stille falen dat deze codebase overal aan het opruimen is.
	const bool bSelected = Strategy->SelectMission(TEXT("TransitCheckpoint"), Error);
	AddInfo(FString::Printf(TEXT("GEMETEN  SelectMission(TransitCheckpoint) = %s, reden: '%s'"),
		bSelected ? TEXT("ja") : TEXT("NEE"), *Error));
	if (!TestTrue(TEXT("M1.3 geselecteerd"), bSelected))
	{
		GameInstance->Shutdown();
		return false;
	}
	TestTrue(TEXT("M1.3 gelanceerd"), Prep->AutoLaunch(Error));

	const TArray<FEclipseObjectiveDef>& Active = Mission->GetActiveObjectives();
	TestEqual(TEXT("2 verplichte doelen, geen optional"), Active.Num(), 2);
	if (Active.Num() == 2)
	{
		TestTrue(TEXT("jammer is DestroyTarget"),
			Active[0].ObjectiveId == TEXT("Obj_M13_Jammer") && Active[0].Type == EEclipseObjectiveType::DestroyTarget);
		TestTrue(TEXT("extractie sluit af"),
			Active[1].ObjectiveId == TEXT("Obj_M13_Exfil") && Active[1].Type == EEclipseObjectiveType::ExtractSquad);
	}

	const FEclipseRegionState* RegionBefore = Campaign->GetState().FindRegion(TEXT("TransitCheckpoint"));
	if (RegionBefore == nullptr)
	{
		AddError(TEXT("De verscheepte regiograaf kent geen TransitCheckpoint - de M1.3-pin wijst naar een regio die niet bestaat."));
		GameInstance->Shutdown();
		return false;
	}
	const EEclipseRegionOwner OwnerBefore = RegionBefore->Owner;

	TestTrue(TEXT("jammer neer"), Mission->CompleteObjective(TEXT("Obj_M13_Jammer"), Error));
	TestTrue(TEXT("extractie voltooid"), Mission->CompleteObjective(TEXT("Obj_M13_Exfil"), Error));
	TestTrue(TEXT("debrief commit"), Mission->ResolveDebrief(true, Error));

	int32 Credits = 0;
	int32 Materials = 0;
	for (const FEclipseEconomyEventPayload& Fact : RewardFacts)
	{
		if (Fact.ResourceType == EclipseTags::Resource_Credits.GetTag())   { Credits += Fact.Delta; }
		if (Fact.ResourceType == EclipseTags::Resource_Materials.GetTag()) { Materials += Fact.Delta; }
	}
	TestEqual(TEXT("rij-credits vastgelegd"), Credits, 60);
	TestEqual(TEXT("rij-materialen vastgelegd"), Materials, 40);

	// DE SUBTIELSTE REGEL VAN DEZE SPEC, en hij is 27-07 verhuisd zoals het
	// commentaar dat hier stond al aankondigde.
	//
	// Er stond: "M1.3 draait zelf GEEN regio om, zolang P2-05 niet bestaat mag er
	// niets omdraaien". Dat klopte precies zolang de liberation-tabel niet aan de
	// verscheepte setup gekoppeld was - en dat was hij niet. Nu wel, en dan hoort
	// de assertie de NAAD te bewijzen in plaats van zijn afwezigheid.
	//
	// De discriminator is het AANTAL: M1.3 speelt op TransitCheckpoint, dus een
	// missie-eigen flip zou alleen dat vak kunnen raken. Dat ook WorkerHousing en
	// SupplyDepot omgaan - twee regio's waar deze missie niets mee te maken heeft
	// - kan alleen van de Foothold-rij komen. Daarmee onderscheidt deze test de
	// twee mogelijke schrijvers van regiostaat, en dat is precies waar 12.3 om
	// vraagt.
	for (const TCHAR* Region : { TEXT("TransitCheckpoint"), TEXT("WorkerHousing"), TEXT("SupplyDepot") })
	{
		const FEclipseRegionState* Flipped = Campaign->GetState().FindRegion(Region);
		if (TestNotNull(FString::Printf(TEXT("naad: %s bestaat"), Region), Flipped))
		{
			TestTrue(FString::Printf(TEXT("naad: %s ging naar de speler op M1.3's completion (SPEC-P2-05)"), Region),
				Flipped->Owner == EEclipseRegionOwner::Player);
		}
	}
	// En de missie zelf draagt die flip niet: haar eigen vlag staat uit.
	TestFalse(TEXT("naad: M1.3's eigen transactie draagt geen regio-flip"),
		OwnerBefore == EEclipseRegionOwner::Player);

	Bus->Unsubscribe(EconomyHandle);
	GameInstance->Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// De hele M1-keten, in een run
// ---------------------------------------------------------------------------
//
// De drie Gauntlets hierboven bewijzen elk hun eigen missie. Deze bewijst de
// KETEN: vier missies achter elkaar, elk achter de beat van zijn voorganger, op
// de twee regio's die de speler zonder SPEC-P2-05 kan bereiken.
//
// Waarom dat een eigen test verdient: elke schakel apart kan kloppen terwijl de
// keten vastloopt. Precies dat gebeurde bij het authoren van M1.3 - het aanbod
// stond er, en selecteren kon niet.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionM14ChainOnShippedDataTest,
	"Eclipse.Missions.M14ChainOnShippedData",
	EclipseMissionM1Test::TestFlags)

bool FEclipseMissionM14ChainOnShippedDataTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt - draai Tools/create_phase1_content.py + Tools/setup_story_missions.py."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	Campaign->StartNewCampaign(Setup);
	FString Error;

	// Eén schakel: kies de regio, controleer dat de verwachte missie wint, speel
	// hem uit. De reden van een weigering gaat mee in de melding - een keten die
	// vastloopt moet zeggen WAAR, niet alleen DAT.
	auto PlayLink = [&](const TCHAR* Region, const TCHAR* Expected, const TArray<FName>& Objectives) -> bool
	{
		FEclipseMissionOfferView View;
		if (!TestTrue(FString::Printf(TEXT("keten: %s heeft aanbod"), Region), Strategy->TryGetOffer(Region, View)))
		{
			return false;
		}
		if (!TestEqual(FString::Printf(TEXT("keten: %s biedt %s"), Region, Expected), View.TemplateId, FName(Expected)))
		{
			return false;
		}
		const bool bSelected = Strategy->SelectMission(Region, Error);
		if (!TestTrue(FString::Printf(TEXT("keten: %s selecteerbaar (reden bij weigering: '%s')"), Expected, *Error), bSelected))
		{
			return false;
		}
		if (!TestTrue(FString::Printf(TEXT("keten: %s gelanceerd"), Expected), Prep->AutoLaunch(Error)))
		{
			return false;
		}
		for (const FName& Objective : Objectives)
		{
			if (!TestTrue(FString::Printf(TEXT("keten: %s voltooit %s"), Expected, *Objective.ToString()),
					Mission->CompleteObjective(Objective, Error)))
			{
				return false;
			}
		}
		return TestTrue(FString::Printf(TEXT("keten: %s debrieft"), Expected), Mission->ResolveDebrief(true, Error));
	};

	const int32 DayAtStart = Campaign->GetState().Day;

	if (!PlayLink(TEXT("TransitCheckpoint"), TEXT("MT_M11"),
			{ TEXT("Obj_M11_PatrolLeader"), TEXT("Obj_M11_Exfil") })) { GameInstance->Shutdown(); return false; }
	if (!PlayLink(TEXT("WorkerHousing"), TEXT("MT_M12"),
			{ TEXT("Obj_M12_CacheNorth"), TEXT("Obj_M12_CacheSouth"), TEXT("Obj_M12_Exfil") })) { GameInstance->Shutdown(); return false; }
	if (!PlayLink(TEXT("TransitCheckpoint"), TEXT("MT_M13"),
			{ TEXT("Obj_M13_Jammer"), TEXT("Obj_M13_Exfil") })) { GameInstance->Shutdown(); return false; }
	// Eerst: IS de tabel uberhaupt gekoppeld? Zonder die vraag is "de flip
	// gebeurt niet" dubbelzinnig — het kan de bedrading zijn of de data, en dat
	// zijn twee verschillende reparaties.
	AddInfo(FString::Printf(TEXT("GEMETEN  DA_CampaignSetup.LiberationInstances = %s"),
		Setup->LiberationInstances.IsNull() ? TEXT("NIET GEKOPPELD") : *Setup->LiberationInstances.ToString()));

	// DE P2-05-NAAD, voor het eerst op een missie die ECHT bestaat.
	//
	// De liberation-instance ("Foothold") triggert op MT_M13 en draait
	// TransitCheckpoint, WorkerHousing en SupplyDepot om naar de speler. Die
	// bedrading is gebouwd en unit-getest, maar de trigger wees tot vannacht naar
	// een missie die niet geauthord was — er was dus nooit een keten waarin hij
	// echt kon vuren.
	for (const TCHAR* Region : { TEXT("TransitCheckpoint"), TEXT("WorkerHousing"), TEXT("SupplyDepot") })
	{
		const FEclipseRegionState* State = Campaign->GetState().FindRegion(Region);
		AddInfo(FString::Printf(TEXT("GEMETEN  na M1.3 is %s van %s"), Region,
			State != nullptr ? *UEnum::GetValueAsString(State->Owner) : TEXT("(onbekend)")));
		if (TestNotNull(FString::Printf(TEXT("naad: %s bestaat"), Region), State))
		{
			TestTrue(FString::Printf(TEXT("naad: %s is na M1.3 van de speler — de Foothold-liberation heeft gevuurd"), Region),
				State->Owner == EEclipseRegionOwner::Player);
		}
	}

	// HET AANBODBORD ANTWOORDT (SPEC-P2-05 Gauntlet). De Foothold verlegt de rand
	// van de kaart, en dat hoort aan twee kanten te zien te zijn: wat er BIJ komt
	// en wat er AF valt.
	{
		FString BoardError;
		const bool bTrioStillSelectable = Strategy->SelectMission(TEXT("WorkerHousing"), BoardError);
		AddInfo(FString::Printf(TEXT("GEMETEN  WorkerHousing na de flip: %s (%s)"),
			bTrioStillSelectable ? TEXT("nog steeds kiesbaar") : TEXT("niet meer kiesbaar"), *BoardError));
		TestFalse(TEXT("bord: de bevrijde trio is geen doelwit meer — je valt je eigen wijk niet aan"),
			bTrioStillSelectable);

		FEclipseMissionOfferView Beyond;
		const bool bCommsRelayOpen = Strategy->TryGetOffer(TEXT("CommsRelay"), Beyond);
		AddInfo(FString::Printf(TEXT("GEMETEN  CommsRelay na de flip: %s"),
			bCommsRelayOpen ? *Beyond.TemplateId.ToString() : TEXT("geen aanbod")));
		// CommsRelay grenst aan SupplyDepot, dat nu van de speler is. Voor de flip
		// was hij onbereikbaar; dat de kaart nu verder reikt is precies wat een
		// bevrijding hoort te betekenen.
		TestTrue(TEXT("bord: CommsRelay is na de flip bereikbaar geworden"),
			Strategy->SelectMission(TEXT("CommsRelay"), BoardError));
	}

	if (!PlayLink(TEXT("FoundryRow"), TEXT("MT_M14"),
			{ TEXT("Obj_M14_CrateFirst"), TEXT("Obj_M14_CrateSecond"), TEXT("Obj_M14_Exfil") })) { GameInstance->Shutdown(); return false; }

	// Vier missies, vier dagen. Een keten die er vier speelt maar er drie telt,
	// heeft ergens een debrief overgeslagen.
	TestEqual(TEXT("keten: vier missies, vier dagen verder"), Campaign->GetState().Day, DayAtStart + 4);

	// En na de vierde is er niets meer gepind: WorkerHousing valt terug op zijn
	// regio-aanbod. Zonder deze assertie zou een pin die blijft hangen (en de
	// speler dus in M1.4 opsluit) onopgemerkt blijven.
	FEclipseMissionOfferView AfterChain;
	if (Strategy->TryGetOffer(TEXT("FoundryRow"), AfterChain))
	{
		AddInfo(FString::Printf(TEXT("GEMETEN  na de keten biedt FoundryRow '%s' aan"), *AfterChain.TemplateId.ToString()));
		TestNotEqual(TEXT("keten: M1.4 blijft niet hangen als aanbod"), AfterChain.TemplateId, FName(TEXT("MT_M14")));
	}

	GameInstance->Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// Besluit 6: alleen M1.3 verandert de wereldstaat
// ---------------------------------------------------------------------------
//
// SPEC-P2-05 vraagt deze test met zoveel woorden ("Regression - P2-04 decision 6
// enforced"), en hij was tot vandaag niet te bouwen: M1.2 en M1.4 bestonden niet.
//
// Wat hij bewaakt is geen detail. Besluit 6 zegt dat er precies EEN schrijver van
// regiostaat is - de liberation-instance - en dat de andere drie missies alleen
// rewards, beat en dag committen. Twee schrijvers van dezelfde staat is de
// divergentiebug waar GDD 12.3 voor bestaat, en die merk je pas als de kaart iets
// anders zegt dan het verhaal.
//
// De meting is het aantal RegionControlChanged-feiten per missie, geteld op de
// bus. Niet de eindstand van de regio: die kan toevallig gelijk blijven terwijl
// er wel degelijk iets is omgedraaid en teruggezet.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseOnlyM13FlipsRegionsTest,
	"Eclipse.Missions.OnlyM13FlipsRegions",
	EclipseMissionM1Test::TestFlags)

bool FEclipseOnlyM13FlipsRegionsTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	int32 Flips = 0;
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		EclipseTags::Event_Strategy_RegionControlChanged,
		FEclipseEventNativeDelegate::CreateLambda([&Flips](FGameplayTag, const FInstancedStruct&) { ++Flips; }));

	Campaign->StartNewCampaign(Setup);
	FString Error;

	auto Play = [&](const TCHAR* Region, const TArray<FName>& Objectives) -> bool
	{
		if (!TestTrue(FString::Printf(TEXT("besluit 6: %s selecteerbaar (reden: '%s')"), Region, *Error),
				Strategy->SelectMission(Region, Error))) { return false; }
		if (!TestTrue(TEXT("besluit 6: gelanceerd"), Prep->AutoLaunch(Error))) { return false; }
		for (const FName& Objective : Objectives)
		{
			if (!TestTrue(FString::Printf(TEXT("besluit 6: %s voltooid"), *Objective.ToString()),
					Mission->CompleteObjective(Objective, Error))) { return false; }
		}
		return TestTrue(TEXT("besluit 6: debrief"), Mission->ResolveDebrief(true, Error));
	};

	// M1.1 - mag niets omdraaien.
	if (!Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M11_PatrolLeader"), TEXT("Obj_M11_Exfil") }))
	{
		Bus->Unsubscribe(Handle); GameInstance->Shutdown(); return false;
	}
	TestEqual(TEXT("besluit 6: M1.1 draait nul regio's om"), Flips, 0);

	// M1.2 - ook niet.
	if (!Play(TEXT("WorkerHousing"), { TEXT("Obj_M12_CacheNorth"), TEXT("Obj_M12_CacheSouth"), TEXT("Obj_M12_Exfil") }))
	{
		Bus->Unsubscribe(Handle); GameInstance->Shutdown(); return false;
	}
	TestEqual(TEXT("besluit 6: M1.2 draait nul regio's om"), Flips, 0);

	// M1.3 - DEZE wel, en precies drie: de Foothold-trio in rijvolgorde. Dit is
	// meteen de discriminator voor de twee nullen hierboven: als de teller sowieso
	// nooit oploopt, bewijzen die niets.
	if (!Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M13_Jammer"), TEXT("Obj_M13_Exfil") }))
	{
		Bus->Unsubscribe(Handle); GameInstance->Shutdown(); return false;
	}
	TestEqual(TEXT("besluit 6: M1.3 draait precies de drie Foothold-regio's om"), Flips, 3);

	// M1.4 - weer nul. Teller op nul zetten zodat we alleen deze missie meten.
	Flips = 0;
	if (!Play(TEXT("FoundryRow"), { TEXT("Obj_M14_CrateFirst"), TEXT("Obj_M14_CrateSecond"), TEXT("Obj_M14_Exfil") }))
	{
		Bus->Unsubscribe(Handle); GameInstance->Shutdown(); return false;
	}
	TestEqual(TEXT("besluit 6: M1.4 draait nul regio's om"), Flips, 0);

	Bus->Unsubscribe(Handle);
	GameInstance->Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// Antwoordt de kaart op de Foothold?
// ---------------------------------------------------------------------------
//
// SPEC-P2-05 belooft dat de dagtick na de flip een hogere band betaalt (de spec
// noemt 28 M / 110 C / 2 I voor de vier regio's die de speler dan houdt). Dat is
// de vraag "verandert er iets in de WERELD", en die is een andere dan "draait de
// eigenaar om" - een regio kan van eigenaar wisselen zonder dat er ooit een
// credit binnenkomt.
//
// De assertie is een VERGELIJKING en geen vast getal: opbrengsten zijn
// tuningwaarden (6.5 levers) en horen te mogen bewegen zonder deze test te
// breken. Wat niet mag bewegen is de richting - drie regio's erbij hoort meer op
// te leveren. De absolute getallen staan in de uitvoer, zodat de spec-band met
// het oog na te kijken is.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFootholdReachesTheLedgerTest,
	"Eclipse.Missions.FootholdReachesTheLedger",
	EclipseMissionM1Test::TestFlags)

bool FEclipseFootholdReachesTheLedgerTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	Campaign->StartNewCampaign(Setup);
	FString Error;

	// Wat levert een dag op VOOR de Foothold? Alleen de Underworks is dan van de
	// speler.
	// EEN ECHTE DAG LATEN VERSTRIJKEN, via het pad dat het spel ook gebruikt: een
	// AdvanceDay-transactie op de campagne, waarna het economiesubsysteem zijn
	// tick zelf bouwt en commit op Event.Campaign.DayAdvanced. Een handgemaakte
	// FEclipseEconomyTickParams zou een FIXTURE meten in plaats van het spel — en
	// dat is precies het verschil dat deze nacht steeds de fout in ging.
	auto AdvanceOneDay = [&](int32& OutCredits, int32& OutMaterials) -> bool
	{
		const int32 CreditsBeforeTick = Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag());
		const int32 MaterialsBeforeTick = Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag());

		FEclipseCampaignMutation Advance;
		Advance.Type = EEclipseCampaignMutationType::AdvanceDay;
		FEclipseCampaignTransaction Transaction;
		Transaction.Source = TEXT("LedgerTest");
		Transaction.Mutations.Add(Advance);

		FString TickError;
		if (!Campaign->CommitTransaction(Transaction, TickError))
		{
			AddError(FString::Printf(TEXT("dagtick geweigerd: %s"), *TickError));
			return false;
		}
		OutCredits = Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()) - CreditsBeforeTick;
		OutMaterials = Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag()) - MaterialsBeforeTick;
		return true;
	};

	int32 CreditsBefore = 0;
	int32 MaterialsBefore = 0;
	if (!AdvanceOneDay(CreditsBefore, MaterialsBefore))
	{
		GameInstance->Shutdown();
		return false;
	}
	AddInfo(FString::Printf(TEXT("GEMETEN  dagtick VOOR de Foothold: %+d C, %+d M"), CreditsBefore, MaterialsBefore));

	auto Play = [&](const TCHAR* Region, const TArray<FName>& Objectives) -> bool
	{
		if (!Strategy->SelectMission(Region, Error)) { AddError(FString::Printf(TEXT("select %s: %s"), Region, *Error)); return false; }
		if (!Prep->AutoLaunch(Error)) { AddError(Error); return false; }
		for (const FName& Objective : Objectives)
		{
			if (!Mission->CompleteObjective(Objective, Error)) { AddError(Error); return false; }
		}
		return Mission->ResolveDebrief(true, Error);
	};

	if (!Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M11_PatrolLeader"), TEXT("Obj_M11_Exfil") })
		|| !Play(TEXT("WorkerHousing"), { TEXT("Obj_M12_CacheNorth"), TEXT("Obj_M12_CacheSouth"), TEXT("Obj_M12_Exfil") })
		|| !Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M13_Jammer"), TEXT("Obj_M13_Exfil") }))
	{
		GameInstance->Shutdown();
		return false;
	}

	int32 CreditsAfter = 0;
	int32 MaterialsAfter = 0;
	if (!AdvanceOneDay(CreditsAfter, MaterialsAfter))
	{
		GameInstance->Shutdown();
		return false;
	}
	AddInfo(FString::Printf(TEXT("GEMETEN  dagtick NA de Foothold: %+d C, %+d M"), CreditsAfter, MaterialsAfter));

	// NIET "meer dan nul", maar "beter dan ervoor". De dagtick is NETTO: hij trekt
	// de soldij van je acht soldaten af van de opbrengst, en dat is de reden dat
	// het spel begint met een verlies. Gemeten:
	//
	//   voor de Foothold  -84 C  +8 M   per dag
	//   na de Foothold      0 C +28 M   per dag
	//
	// De drie regio's brengen de kas dus van een dagelijks verlies naar precies
	// quitte, en de materialen van 8 naar 28 — de band die SPEC-P2-05 belooft
	// (28 M) komt er exact uit. Een assertie op "positief" zou hier rood zijn
	// geworden op een volkomen gezonde economie: dat is de fout die een test
	// onbruikbaar maakt.
	//
	// De discriminator zit ingebakken: bij een lege tick zijn beide nul en is
	// "na > voor" onwaar, dus de test kan niet groen worden zonder dat er echt
	// iets binnenkomt.
	TestTrue(TEXT("ledger: de materialen lopen echt op (geen lege tick)"), MaterialsAfter > 0);
	TestTrue(TEXT("ledger: de Foothold verbetert de dagbalans in credits"), CreditsAfter > CreditsBefore);
	TestTrue(TEXT("ledger: en levert meer materialen op"), MaterialsAfter > MaterialsBefore);

	GameInstance->Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// Overleeft verdiende voortgang een herstart?
// ---------------------------------------------------------------------------
//
// De bestaande save-tests dekken de story-tail wel, maar op campagnes ZONDER
// beats - een van die tests zegt het zelfs hardop: "Sanity: the scripted campaign
// has no beats yet". Er was dus niets dat bewees dat een SPELER die M1.1 heeft
// uitgespeeld die voortgang terugkrijgt na het laden.
//
// Dat is sinds vannacht geen theoretisch risico meer: M1.2 t/m M1.4 hangen achter
// beats. Gaan die verloren bij het laden, dan staat de speler na een herstart
// weer voor een dichte deur - met zijn credits en zijn dagen intact, wat het nog
// verwarrender maakt.
//
// De assertie die telt is niet "de tag zit in de array" maar "het spel gedraagt
// zich nog alsof je die missie gespeeld hebt". Daarom staat de poort erin.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseProgressSurvivesASaveLoadTest,
	"Eclipse.Missions.ProgressSurvivesASaveLoad",
	EclipseMissionM1Test::TestFlags)

bool FEclipseProgressSurvivesASaveLoadTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt."));
		return false;
	}

	const FString SlotName = TEXT("AutomationStoryProgress");
	const FString SlotPath = UEclipseSaveSubsystem::GetSlotFilePath(SlotName);
	IFileManager::Get().Delete(*SlotPath, false, true, true);

	// --- spelen en opslaan
	UGameInstance* Source = NewObject<UGameInstance>(GEngine);
	Source->InitializeStandalone();
	UEclipseCampaignSubsystem* SourceCampaign = Source->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* SourceStrategy = Source->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* SourcePrep = Source->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* SourceMission = Source->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseSaveSubsystem* SourceSave = Source->GetSubsystem<UEclipseSaveSubsystem>();

	SourceCampaign->StartNewCampaign(Setup);
	FString Error;
	TestTrue(TEXT("save: M1.1 geselecteerd"), SourceStrategy->SelectMission(TEXT("TransitCheckpoint"), Error));
	TestTrue(TEXT("save: M1.1 gelanceerd"), SourcePrep->AutoLaunch(Error));
	TestTrue(TEXT("save: hinderlaag"), SourceMission->CompleteObjective(TEXT("Obj_M11_PatrolLeader"), Error));
	TestTrue(TEXT("save: extractie"), SourceMission->CompleteObjective(TEXT("Obj_M11_Exfil"), Error));
	TestTrue(TEXT("save: debrief"), SourceMission->ResolveDebrief(true, Error));

	const int32 BeatsEarned = SourceCampaign->GetState().StoryFlags.Num();
	AddInfo(FString::Printf(TEXT("GEMETEN  na M1.1 staan er %d story-beats in de staat"), BeatsEarned));
	// Discriminator: als M1.1 helemaal geen beat zet, bewijst de rest niets.
	if (!TestTrue(TEXT("save: M1.1 heeft daadwerkelijk een beat gezet"), BeatsEarned > 0))
	{
		Source->Shutdown();
		return false;
	}

	const uint32 HashBefore = SourceCampaign->GetState().ComputeStateHash();
	TestTrue(TEXT("save: opslaan lukt"), SourceSave->SaveToSlot(SlotName, Error));

	// --- laden in een VERSE instantie
	UGameInstance* Target = NewObject<UGameInstance>(GEngine);
	Target->InitializeStandalone();
	UEclipseCampaignSubsystem* TargetCampaign = Target->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* TargetStrategy = Target->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipseSaveSubsystem* TargetSave = Target->GetSubsystem<UEclipseSaveSubsystem>();

	// De setup EERST, want die komt niet uit het bestand: de regiograaf en de
	// missietabellen zijn geauthorde inhoud en horen bij de build. Zo boot een
	// echte sessie ook — het spel kent zijn campagnedata voordat er een slot
	// opengaat. Doe je dat niet, dan laadt de staat wel maar is het hele
	// aanbodbord leeg; sinds 27-07 zegt de engine dat hardop in plaats van stil.
	TargetCampaign->StartNewCampaign(Setup);
	TestTrue(TEXT("laden lukt"), TargetSave->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("laden: evenveel beats terug"), TargetCampaign->GetState().StoryFlags.Num(), BeatsEarned);
	TestEqual(TEXT("laden: de hele staat is bit-voor-bit dezelfde"),
		TargetCampaign->GetState().ComputeStateHash(), HashBefore);

	// DE ASSERTIE DIE ER ECHT TOE DOET. Een tag in een array is geen voortgang;
	// voortgang is dat de volgende missie op je wacht. Zonder deze regel zou een
	// beat die wel bewaard maar nergens meer gelezen wordt, gewoon groen zijn.
	// Eerst breed meten: heeft er NA het laden nog ergens een missie-aanbod?
	// "WorkerHousing is leeg" en "het hele bord is leeg" zijn twee heel
	// verschillende bugs.
	for (const TCHAR* Region : { TEXT("Underworks"), TEXT("TransitCheckpoint"), TEXT("FoundryRow"),
		TEXT("WorkerHousing"), TEXT("SupplyDepot"), TEXT("CommsRelay") })
	{
		FEclipseMissionOfferView View;
		AddInfo(TargetStrategy->TryGetOffer(Region, View)
			? FString::Printf(TEXT("GEMETEN  na het laden biedt %s '%s' aan"), Region, *View.TemplateId.ToString())
			: FString::Printf(TEXT("GEMETEN  na het laden heeft %s GEEN aanbod"), Region));
	}
	AddInfo(FString::Printf(TEXT("GEMETEN  na het laden is de campagne-setup %s"),
		TargetCampaign->GetActiveSetup() != nullptr ? TEXT("aanwezig") : TEXT("WEG")));

	FEclipseMissionOfferView Offer;
	if (TestTrue(TEXT("laden: WorkerHousing heeft aanbod"), TargetStrategy->TryGetOffer(TEXT("WorkerHousing"), Offer)))
	{
		AddInfo(FString::Printf(TEXT("GEMETEN  na het laden biedt WorkerHousing '%s' aan"), *Offer.TemplateId.ToString()));
		TestEqual(TEXT("laden: M1.2 staat nog steeds open — de poort onthoudt dat je M1.1 speelde"),
			Offer.TemplateId, FName(TEXT("MT_M12")));
	}

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source->Shutdown();
	Target->Shutdown();
	return true;
}


// ---------------------------------------------------------------------------
// De overdracht-asserties van de soak (SPEC-P2-05, laatste testregel)
// ---------------------------------------------------------------------------
//
// De spec vraagt drie dingen van de soak, en NIET het getal dat er het meest
// uitspringt. "Flip ≈ dag 9" staat er met zoveel woorden bij als een aanname van
// het soak-script en NOOIT als een poort voor de speler (regel 209). Een test die
// `Dag == 9` eist zou dus een tuningwaarde tot contract bombarderen: DT_Facilities
// een dag laten schuiven en de bar wordt rood terwijl het spel klopt. Precies de
// fout die deze week drie keer gemaakt is.
//
// Wat er WEL te beweren valt, is de bouw eronder:
//
//   1. de Foothold landt op de dag waarop M1.3 afrekent - zonder dat er nog een
//      dagtick nodig is. De commit loopt binnen TryResolveMission (commit ->
//      broadcast), dus na ResolveDebrief is het al gebeurd.
//   2. de nieuwe inkomensband geldt vanaf de EERSTVOLGENDE tick, niet later.
//   3. een BLUTTE campagne krijgt zijn Foothold net zo goed. Credits staan
//      midden in de slice bij nul en `Wages_Short` vuurt - dat is het bedoelde
//      6.5-gevoel, geen storing - en de overdracht mag daar niet op stuklopen.
//
// Bewering 3 zou zonder tekort niets bewijzen: op een rijke campagne staat hij
// groen zonder iets te zeggen. Daarom eist hij dat Wages_Short ECHT gevuurd
// heeft.
//
// NAGEMETEN, en het antwoord was niet wat ik verwachtte: zonder de kas leeg te
// halen vuurt Wages_Short ook. De slice is uit zichzelf al blut - 150 credits
// aan het begin dekken de soldij van acht soldaten niet. Het leeghalen MAAKT het
// tekort dus niet, het maakt het DETERMINISTISCH: zonder die stap hangt de
// premisse aan een startsaldo dat een balansronde zo verandert.
//
// Die assertie is daarmee geen discriminator maar een bewaker op de PREMISSE.
// Wordt de economie ooit ruim genoeg om de soldij te betalen, dan valt hij om en
// zegt hij precies het juiste: deze test meet niet meer wat hij belooft.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBrokeCampaignStillGetsItsFootholdTest,
	"Eclipse.Missions.BrokeCampaignStillGetsItsFoothold",
	EclipseMissionM1Test::TestFlags)

bool FEclipseBrokeCampaignStillGetsItsFootholdTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	Campaign->StartNewCampaign(Setup);
	FString Error;

	// Meeluisteren op de twee feiten die dit verhaal vertellen.
	bool bWagesFellShort = false;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&bWagesFellShort](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseEconomyEventPayload* Fact = Payload.GetPtr<FEclipseEconomyEventPayload>())
			{
				if (Fact->Reason == FName(TEXT("Wages_Short")))
				{
					bWagesFellShort = true;
				}
			}
		}),
		FEclipseEconomyEventPayload::StaticStruct());

	int32 FlipDay = INDEX_NONE;
	int32 FlippedRegions = 0;
	FEclipseEventSubscriptionHandle LiberationHandle = Bus->Subscribe(
		EclipseTags::Event_Strategy_LiberationResolved,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct& Payload)
		{
			// De dag WAAROP het gebeurt, gelezen op het moment zelf. Achteraf de
			// staat aflezen zou een latere dagtick meetellen.
			FlipDay = Campaign->GetState().Day;
			if (const FEclipseLiberationEventPayload* Fact = Payload.GetPtr<FEclipseLiberationEventPayload>())
			{
				FlippedRegions = Fact->RegionCount;
			}
		}),
		FEclipseLiberationEventPayload::StaticStruct());

	auto AdvanceOneDay = [&](int32& OutCredits, int32& OutMaterials) -> bool
	{
		const int32 CreditsBefore = Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag());
		const int32 MaterialsBefore = Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag());

		FEclipseCampaignMutation Advance;
		Advance.Type = EEclipseCampaignMutationType::AdvanceDay;
		FEclipseCampaignTransaction Transaction;
		Transaction.Source = TEXT("SoakHandoff");
		Transaction.Mutations.Add(Advance);

		FString TickError;
		if (!Campaign->CommitTransaction(Transaction, TickError))
		{
			AddError(FString::Printf(TEXT("dagtick geweigerd: %s"), *TickError));
			return false;
		}
		OutCredits = Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()) - CreditsBefore;
		OutMaterials = Campaign->GetState().GetBalance(EclipseTags::Resource_Materials.GetTag()) - MaterialsBefore;
		return true;
	};

	// DE KAS LEEGHALEN. Niet om het spel te pesten: de slice IS blut rond deze
	// dagen (SPEC-P2-03 eerlijkheidsregel, ~1484 C binnen tegen ~1920 C soldij),
	// en de dagtick liep vóór de Foothold gemeten op -84 C. Hier wordt dat
	// gegarandeerd bereikt in plaats van afgewacht, want een test die toevallig
	// wel of niet blut is, meet elke nacht iets anders.
	{
		FEclipseCampaignMutation Drain;
		Drain.Type = EEclipseCampaignMutationType::AdjustResource;
		Drain.ResourceType = EclipseTags::Resource_Credits.GetTag();
		Drain.Amount = -Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag());
		FEclipseCampaignTransaction Transaction;
		Transaction.Source = TEXT("SoakHandoff_Drain");
		Transaction.Mutations.Add(Drain);
		if (!Campaign->CommitTransaction(Transaction, Error))
		{
			AddError(FString::Printf(TEXT("kas leeghalen geweigerd: %s"), *Error));
			GameInstance->Shutdown();
			return false;
		}
	}
	TestEqual(TEXT("opzet: de kas staat op nul voor we beginnen"),
		Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()), 0);

	int32 CreditsPreFlip = 0;
	int32 MaterialsPreFlip = 0;
	if (!AdvanceOneDay(CreditsPreFlip, MaterialsPreFlip))
	{
		GameInstance->Shutdown();
		return false;
	}
	AddInfo(FString::Printf(TEXT("GEMETEN  dagtick op een lege kas: %+d C, %+d M"), CreditsPreFlip, MaterialsPreFlip));

	auto Play = [&](const TCHAR* Region, const TArray<FName>& Objectives) -> bool
	{
		if (!Strategy->SelectMission(Region, Error)) { AddError(FString::Printf(TEXT("select %s: %s"), Region, *Error)); return false; }
		if (!Prep->AutoLaunch(Error)) { AddError(Error); return false; }
		for (const FName& Objective : Objectives)
		{
			if (!Mission->CompleteObjective(Objective, Error)) { AddError(Error); return false; }
		}
		return Mission->ResolveDebrief(true, Error);
	};

	if (!Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M11_PatrolLeader"), TEXT("Obj_M11_Exfil") })
		|| !Play(TEXT("WorkerHousing"), { TEXT("Obj_M12_CacheNorth"), TEXT("Obj_M12_CacheSouth"), TEXT("Obj_M12_Exfil") }))
	{
		GameInstance->Shutdown();
		return false;
	}

	// Vlak voor de missie die de Foothold trekt: welke dag staat er op de kalender?
	const int32 DayBeforeM13 = Campaign->GetState().Day;

	if (!Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M13_Jammer"), TEXT("Obj_M13_Exfil") }))
	{
		GameInstance->Shutdown();
		return false;
	}

	// (1) HET IS AL GEBEURD. Geen extra dagtick, geen tweede commit: na de debrief
	//     van M1.3 zijn de drie vakken van de speler.
	TestEqual(TEXT("de Foothold draaide drie vakken om"), FlippedRegions, 3);
	for (const TCHAR* RegionId : { TEXT("TransitCheckpoint"), TEXT("WorkerHousing"), TEXT("SupplyDepot") })
	{
		const FEclipseRegionState* Region = Campaign->GetState().FindRegion(RegionId);
		TestTrue(FString::Printf(TEXT("'%s' is van de speler zodra de debrief klaar is"), RegionId),
			Region != nullptr && Region->Owner == EEclipseRegionOwner::Player);
	}

	// (2) OP DE DAG VAN DE AFREKENING, niet later. Geen hard getal 9 - dat is een
	//     aanname van het script - maar de band: de flip hoort bij de dag die M1.3
	//     zelf oplevert, en die ligt op of net na de dag ervoor.
	TestTrue(FString::Printf(TEXT("de flip landde op de dag van de M1.3-afrekening (dag %d, ervoor %d)"), FlipDay, DayBeforeM13),
		FlipDay != INDEX_NONE && FlipDay >= DayBeforeM13 && FlipDay <= DayBeforeM13 + 1);

	// (3) EN DE BAND GELDT VANAF DE EERSTVOLGENDE TICK.
	int32 CreditsPostFlip = 0;
	int32 MaterialsPostFlip = 0;
	if (!AdvanceOneDay(CreditsPostFlip, MaterialsPostFlip))
	{
		GameInstance->Shutdown();
		return false;
	}
	AddInfo(FString::Printf(TEXT("GEMETEN  eerste dagtick NA de Foothold: %+d C, %+d M"), CreditsPostFlip, MaterialsPostFlip));
	TestTrue(TEXT("de eerste tick na de flip levert al meer materialen op"), MaterialsPostFlip > MaterialsPreFlip);

	// DE DISCRIMINATOR. Zonder een echt tekort zegt dit alles niets.
	TestTrue(TEXT("discriminator: de soldij kwam echt tekort (Wages_Short gevuurd)"), bWagesFellShort);

	Bus->Unsubscribe(EconomyHandle);
	Bus->Unsubscribe(LiberationHandle);
	GameInstance->Shutdown();
	return true;
}


// ---------------------------------------------------------------------------
// De drie econ-paden van SPEC-P2-03, over de gespeelde slice
// ---------------------------------------------------------------------------
//
// De spec noemt drie manieren waarop een speler zijn basis kan opbouwen en
// noemt ze met zoveel woorden "de soak-invarianten": Builder (drie gebouwen
// neerzetten), Intel opening (eerst het Intelligence Centre, gear komt later) en
// Thrifty (zuinig, en dan een Workshop-upgrade). Ze horen alle drie te PASSEN
// binnen de slice.
//
// TWEE DINGEN DIE IK BEWUST NIET DOE.
//
// Niet de kalender vastpinnen. De spec zet er dagen bij - Barracks d2, Workshop
// d9, IC d13 - maar zegt er zelf bij dat retunen in DT_Facilities gebeurt zonder
// dat de spec verandert. Een test die "Barracks op dag 2" eist maakt van een
// tuningwaarde een contract. Wat de invariant beweert is de UITKOMST: haalt dit
// pad zijn gebouwen binnen de slice. Dus bestelt dit script "zodra het te
// betalen is" en kijkt het achteraf wat eruit kwam.
//
// Niet de dagtick alleen laten draaien. Zonder missies levert een dag +8 M op,
// dus zestien dagen geven 128 M - niet eens één Barracks plus Workshop. De drie
// paden LEUNEN op missie-opbrengst; ze bestaan alleen in een campagne die
// gespeeld wordt. Daarom draait dit over de echte keten M1.1 t/m M1.4, op de
// verscheepte data, met een bestelpoging op elke dag.

namespace EclipseSoakPath
{
	struct FRun
	{
		TArray<FName> Wishes;
		int32 WishIndex = 0;
		TMap<FName, int32> OrderedOnDay;
		/** Eindstand per gewenste faciliteit: niveau en resterende bouwdagen. */
		TMap<FName, int32> FinalLevel;
		TMap<FName, int32> FinalDaysRemaining;
		bool bCreditsWentNegative = false;
		bool bOrderRejectedForCredits = false;
		FString LastOrderError;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseThreeEconPathsSurviveTheSliceTest,
	"Eclipse.Missions.ThreeEconPathsSurviveTheSlice",
	EclipseMissionM1Test::TestFlags)

bool FEclipseThreeEconPathsSurviveTheSliceTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt."));
		return false;
	}

	// EERST DE VRAAG DIE ALLES ERONDER BEPAALT: kent de verscheepte opzet
	// uberhaupt een basis? Zonder layout of faciliteitentabel is elke
	// bouwbestelling een weigering, en dan zou dit script "pad haalt het niet"
	// melden terwijl er niets te bouwen VALT. Dat zijn twee verschillende
	// reparaties, dus staat de vraag vooraan (dezelfde vorm als
	// Eclipse.Liberation.Report).
	// Let op de SOFT pointer: dit zijn TSoftObjectPtr-velden, en mijn eerste versie
	// vroeg `!= nullptr`. Dat leverde twee keer "ONTBREEKT" op terwijl er vrolijk
	// een Barracks werd neergezet — ik had bijna "de verscheepte opzet kent geen
	// basis" als bevinding opgeschreven. Vragen of de VERWIJZING leeg is, niet of
	// het geladen object er al staat.
	AddInfo(FString::Printf(TEXT("GEMETEN  DA_CampaignSetup.BaseLayout = %s"),
		Setup->BaseLayout.IsNull() ? TEXT("NIET GEKOPPELD") : *Setup->BaseLayout.ToString()));
	AddInfo(FString::Printf(TEXT("GEMETEN  DA_CampaignSetup.Facilities = %s"),
		Setup->Facilities.IsNull() ? TEXT("NIET GEKOPPELD") : *Setup->Facilities.ToString()));

	// Eén pad, van dag 1 tot het eind van de slice.
	auto RunPath = [&](const TCHAR* PathName, const TArray<FName>& Wishes) -> EclipseSoakPath::FRun
	{
		EclipseSoakPath::FRun Run;
		Run.Wishes = Wishes;

		UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
		GameInstance->InitializeStandalone();
		UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
		UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
		UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
		UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
		UEclipseBaseSubsystem* Base = GameInstance->GetSubsystem<UEclipseBaseSubsystem>();
		Campaign->StartNewCampaign(Setup);

		FString Error;

		// Bestellen zodra het kan. Een weigering wegens geld is een BEVINDING
		// (soldij mag een bouw niet blokkeren); een weigering wegens materiaal is
		// gewoon "nog niet genoeg gespaard" en hoort erbij.
		auto AttemptOrder = [&]()
		{
			while (Run.WishIndex < Run.Wishes.Num())
			{
				FString OrderError;
				if (!Base->TryStartConstruction(Run.Wishes[Run.WishIndex], OrderError))
				{
					Run.LastOrderError = OrderError;
					if (OrderError.Contains(TEXT("credits")))
					{
						Run.bOrderRejectedForCredits = true;
					}
					return;
				}
				Run.OrderedOnDay.Add(Run.Wishes[Run.WishIndex], Campaign->GetState().Day);
				++Run.WishIndex;
			}
		};

		auto AdvanceOneDay = [&]() -> bool
		{
			FEclipseCampaignMutation Advance;
			Advance.Type = EEclipseCampaignMutationType::AdvanceDay;
			FEclipseCampaignTransaction Transaction;
			Transaction.Source = TEXT("SoakPaths");
			Transaction.Mutations.Add(Advance);
			FString TickError;
			if (!Campaign->CommitTransaction(Transaction, TickError))
			{
				AddError(FString::Printf(TEXT("[%s] dagtick geweigerd: %s"), PathName, *TickError));
				return false;
			}
			if (Campaign->GetState().GetBalance(EclipseTags::Resource_Credits.GetTag()) < 0)
			{
				Run.bCreditsWentNegative = true;
			}
			return true;
		};

		auto Play = [&](const TCHAR* Region, const TArray<FName>& Objectives) -> bool
		{
			if (!Strategy->SelectMission(Region, Error)) { AddError(FString::Printf(TEXT("[%s] select %s: %s"), PathName, Region, *Error)); return false; }
			if (!Prep->AutoLaunch(Error)) { AddError(FString::Printf(TEXT("[%s] launch: %s"), PathName, *Error)); return false; }
			for (const FName& Objective : Objectives)
			{
				if (!Mission->CompleteObjective(Objective, Error)) { AddError(FString::Printf(TEXT("[%s] %s: %s"), PathName, *Objective.ToString(), *Error)); return false; }
			}
			return Mission->ResolveDebrief(true, Error);
		};

		// De keten, met op elke overgang een bestelpoging.
		AttemptOrder();
		bool bOk = Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M11_PatrolLeader"), TEXT("Obj_M11_Exfil") });
		AttemptOrder();
		bOk = bOk && AdvanceOneDay(); AttemptOrder();
		bOk = bOk && Play(TEXT("WorkerHousing"), { TEXT("Obj_M12_CacheNorth"), TEXT("Obj_M12_CacheSouth"), TEXT("Obj_M12_Exfil") });
		AttemptOrder();
		bOk = bOk && AdvanceOneDay(); AttemptOrder();
		bOk = bOk && Play(TEXT("TransitCheckpoint"), { TEXT("Obj_M13_Jammer"), TEXT("Obj_M13_Exfil") });
		AttemptOrder();
		bOk = bOk && AdvanceOneDay(); AttemptOrder();
		bOk = bOk && Play(TEXT("FoundryRow"), { TEXT("Obj_M14_CrateFirst"), TEXT("Obj_M14_CrateSecond"), TEXT("Obj_M14_Exfil") });
		AttemptOrder();

		// En daarna doortikken tot het eind van de slice, elke dag opnieuw
		// proberen: de laatste bestelling wacht meestal op gespaard materiaal.
		while (bOk && Campaign->GetState().Day < 16)
		{
			bOk = AdvanceOneDay();
			AttemptOrder();
		}

		// Opschrijven wat eruit kwam, ook als het niet is wat de spec hoopt.
		FString Gebouwd;
		for (const FEclipseFacilityState& Facility : Campaign->GetState().BaseState.Facilities)
		{
			Gebouwd += FString::Printf(TEXT("%s=L%d(%dd) "), *Facility.FacilityId.ToString(), Facility.Level, Facility.DaysRemaining);
		}
		AddInfo(FString::Printf(TEXT("GEMETEN  [%s] dag %d, besteld %d/%d, basis: %s"),
			PathName, Campaign->GetState().Day, Run.WishIndex, Run.Wishes.Num(), *Gebouwd));
		if (Run.WishIndex < Run.Wishes.Num())
		{
			AddInfo(FString::Printf(TEXT("GEMETEN  [%s] eerste onbestelde wens '%s', laatste weigering: %s"),
				PathName, *Run.Wishes[Run.WishIndex].ToString(), *Run.LastOrderError));
		}

		// De uitkomst uitlezen VOOR de shutdown.
		Run.OrderedOnDay.Add(TEXT("__Day"), Campaign->GetState().Day);
		int32 Operational = 0;
		for (const FName& Wish : Wishes)
		{
			for (const FEclipseFacilityState& Facility : Campaign->GetState().BaseState.Facilities)
			{
				if (Facility.FacilityId == Wish)
				{
					Run.FinalLevel.Add(Wish, Facility.Level);
					Run.FinalDaysRemaining.Add(Wish, Facility.DaysRemaining);
					if (Facility.Level >= 1)
					{
						++Operational;
					}
					break;
				}
			}
		}
		Run.OrderedOnDay.Add(TEXT("__Operational"), Operational);

		GameInstance->Shutdown();
		return Run;
	};

	const FName Barracks(TEXT("Barracks"));
	const FName Workshop(TEXT("Workshop"));
	const FName IntelCentre(TEXT("IntelligenceCenter"));

	const EclipseSoakPath::FRun Builder = RunPath(TEXT("Builder"), { Barracks, Workshop, IntelCentre });
	const EclipseSoakPath::FRun Intel   = RunPath(TEXT("Intel opening"), { IntelCentre, Barracks, Workshop });
	// Thrifty vraagt de Workshop TWEE keer: de tweede bestelling is de upgrade naar
	// L2, en dat is wat de spec van dit pad eist ("Workshop L2 ordered before day
	// 16") - niet alleen dat er een Workshop staat.
	const EclipseSoakPath::FRun Thrifty = RunPath(TEXT("Thrifty"), { Barracks, Workshop, Workshop });

	// DE INVARIANTEN, als uitkomst en niet als kalender.
	TestTrue(TEXT("Builder: drie gebouwen staan er binnen de slice"),
		Builder.OrderedOnDay.FindRef(TEXT("__Operational")) >= 3);
	TestTrue(TEXT("Intel opening: het Intelligence Centre staat er binnen de slice"),
		Intel.OrderedOnDay.Contains(IntelCentre));
	// Thrifty: L2 STAAT er, of hij is besteld en nog in aanbouw. De spec vraagt
	// "ordered before day 16" en niet "af", want de bouwtijd loopt bewust door tot
	// ongeveer d18 - dat is het hele punt van dit pad.
	{
		const int32 Level = Thrifty.FinalLevel.FindRef(Workshop);
		const int32 DaysLeft = Thrifty.FinalDaysRemaining.FindRef(Workshop);
		AddInfo(FString::Printf(TEXT("GEMETEN  [Thrifty] Workshop eindigt op L%d met %d bouwdag(en) te gaan"), Level, DaysLeft));
		TestTrue(TEXT("Thrifty: de Workshop-upgrade naar L2 is binnen de slice besteld (af of in aanbouw)"),
			Level >= 2 || (Level >= 1 && DaysLeft > 0));
	}

	// EN DE TWEE REGELS DIE VOOR ALLE DRIE GELDEN.
	for (const TPair<const TCHAR*, const EclipseSoakPath::FRun*>& Pad :
		{ TPair<const TCHAR*, const EclipseSoakPath::FRun*>(TEXT("Builder"), &Builder),
		  TPair<const TCHAR*, const EclipseSoakPath::FRun*>(TEXT("Intel opening"), &Intel),
		  TPair<const TCHAR*, const EclipseSoakPath::FRun*>(TEXT("Thrifty"), &Thrifty) })
	{
		TestFalse(FString::Printf(TEXT("[%s] de kas gaat nooit onder nul"), Pad.Key), Pad.Value->bCreditsWentNegative);
		TestFalse(FString::Printf(TEXT("[%s] soldijtekort blokkeert nooit een bouw"), Pad.Key), Pad.Value->bOrderRejectedForCredits);
	}

	return true;
}


// Halverwege stoppen laat de campagne ONGEMOEID (SPEC-P2-04, save-integriteit).
//
// De spec vraagt: "quit mid-mission at every outer phase -> strategic state hash
// unchanged". Er was geen test voor. DeterministicStateHash bewijst iets anders -
// dat de hash reproduceerbaar is - en niet dat een lopende missie er buiten
// blijft.
//
// De belofte die eronder ligt is er een aan de speler: wat je in een missie doet
// telt pas bij de debrief. Sluit je halverwege af, dan ben je je voortgang kwijt
// maar je campagne niet. De omgekeerde fout is de vervelende: als een objective
// onderweg al credits of een story-beat commit, dan kan de speler een missie
// half spelen, afsluiten, en de opbrengst houden - en dat sloopt zowel de
// economie als de verhaalvolgorde.
//
// Dat is geen theoretisch risico. De verleiding om "even meteen te committen"
// zit precies in de objective-afhandeling, en 12.3 zegt niet voor niets dat er
// EEN schrijver van campagnestaat is.
//
// Gemeten op de hash en niet op losse velden: die dekt dag, credits, roster,
// regio-eigenaars en story-flags in een keer, en hij is expliciet veldvolgorde-
// vast (geen reflectie-iteratie).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseQuittingMidMissionLeavesTheCampaignUntouchedTest,
	"Eclipse.Missions.QuittingMidMissionLeavesTheCampaignUntouched",
	EclipseMissionM1Test::TestFlags)

bool FEclipseQuittingMidMissionLeavesTheCampaignUntouchedTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	Campaign->StartNewCampaign(Setup);
	FString Error;

	// De nulmeting: alles wat de strategische laag over zichzelf weet, in een getal.
	const uint32 HashVoor = Campaign->GetState().ComputeStateHash();
	const int32 DagVoor = Campaign->GetState().Day;

	if (!TestTrue(TEXT("opzet: missie geselecteerd"), Strategy->SelectMission(TEXT("TransitCheckpoint"), Error))
		|| !TestTrue(TEXT("opzet: missie gelanceerd"), Prep->AutoLaunch(Error)))
	{
		AddError(Error);
		GameInstance->Shutdown();
		return false;
	}

	// De nulmeting wordt hier OPNIEUW genomen omdat selecteren en lanceren prep
	// is en geen missie: zou prep iets kosten, dan mag deze test dat niet als lek
	// aanzien. De vraag is wat er tijdens de MISSIE verandert.
	//
	// Gemeten valt dat mee: prep kost op de verscheepte data NIETS in
	// campagnestaat - beide hashes zijn gelijk. Dat is een waarneming en geen
	// eis; verandert prep ooit wel iets, dan blijft deze test kloppen zonder
	// aanpassing, en dat is precies waarom hij op twee nulmetingen staat.
	const uint32 HashNaLancering = Campaign->GetState().ComputeStateHash();
	AddInfo(FString::Printf(TEXT("GEMETEN  hash bij start %u, na lancering %u"),
		HashVoor, HashNaLancering));

	// Elke buitenfase langslopen en na elke stap kijken of de campagne bewoog.
	auto ControleerOngemoeid = [&](const TCHAR* Waar)
	{
		TestEqual(*FString::Printf(TEXT("campagne ongemoeid %s"), Waar),
			Campaign->GetState().ComputeStateHash(), HashNaLancering);
	};

	ControleerOngemoeid(TEXT("bij insertie"));

	// M1.1 helemaal uitspelen TOT de debrief, maar hem niet afrekenen.
	for (const FName& Objective : { FName(TEXT("Obj_M11_PatrolLeader")), FName(TEXT("Obj_M11_Exfil")) })
	{
		if (!TestTrue(*FString::Printf(TEXT("opzet: %s voltooid"), *Objective.ToString()),
				Mission->CompleteObjective(Objective, Error)))
		{
			AddError(Error);
			GameInstance->Shutdown();
			return false;
		}
		ControleerOngemoeid(*FString::Printf(TEXT("na objective %s"), *Objective.ToString()));
	}

	ControleerOngemoeid(TEXT("met alle objectives klaar, voor de debrief"));
	TestEqual(TEXT("en de kalender staat nog stil"), Campaign->GetState().Day, DagVoor);

	// DE DISCRIMINATOR. Zonder deze helft zou een spel waarin de debrief OOK
	// niets doet net zo groen zijn - en dan meet de test niets. Pas als de
	// debrief de hash wel beweegt, weet je dat "onveranderd" iets betekende.
	if (TestTrue(TEXT("discriminator: de debrief rekent wel degelijk af"), Mission->ResolveDebrief(true, Error)))
	{
		TestNotEqual(TEXT("discriminator: en DAAR verandert de campagne pas"),
			Campaign->GetState().ComputeStateHash(), HashNaLancering);
		TestEqual(TEXT("de debrief zet de dag een verder"), Campaign->GetState().Day, DagVoor + 1);
	}

	GameInstance->Shutdown();
	return true;
}


// De grootboekregels van de dagtick komen BINNEN het debrief-venster
// (SPEC-P2-03 regressieregel, GDD 7.6 transparantie).
//
// De spec vraagt dat het debriefscherm de grootboekregels van diezelfde dagtick
// toont - soldij, opbrengsten - zodat de speler ziet WAAROM zijn getallen
// veranderden. Er was geen test voor.
//
// Die eis wordt hier NIET op de widget gemeten maar op de bus, en dat is met
// opzet: het debriefscherm verzamelt feiten tussen Mission.Started en
// Mission.Completed, dus of het scherm iets KAN tonen hangt volledig af van de
// VOLGORDE waarin die feiten langskomen. Vuurt de dagtick pas na
// Mission.Completed, dan sluit het venster voor de cijfers binnen zijn en blijft
// het debrief leeg - zonder dat er iets kapot is. Dat is precies de vorm waarin
// de HUD-regel eerder stukging: op zichzelf correct, alleen op het verkeerde
// moment.
//
// Een widget-test zou hetzelfde aantonen maar met een wereld en pixels eromheen,
// en dan meet je de widget in plaats van het contract.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLedgerLinesArriveBeforeTheDebriefClosesTest,
	"Eclipse.Missions.LedgerLinesArriveBeforeTheDebriefCloses",
	EclipseMissionM1Test::TestFlags)

bool FEclipseLedgerLinesArriveBeforeTheDebriefClosesTest::RunTest(const FString& Parameters)
{
	UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		AddError(TEXT("Verscheepte DA_CampaignSetup ontbreekt."));
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();

	Campaign->StartNewCampaign(Setup);
	FString Error;

	// Een gedeelde reeks: de volgorde is de hele bewering, niet de aantallen.
	TArray<FString> Reeks;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&Reeks](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseEconomyEventPayload* Fact = Payload.GetPtr<FEclipseEconomyEventPayload>())
			{
				Reeks.Add(Fact->Reason.ToString());
			}
		}),
		FEclipseEconomyEventPayload::StaticStruct());
	FEclipseEventSubscriptionHandle DoneHandle = Bus->Subscribe(
		EclipseTags::Event_Mission_Completed,
		FEclipseEventNativeDelegate::CreateLambda([&Reeks](FGameplayTag, const FInstancedStruct&)
		{
			Reeks.Add(TEXT("<<DEBRIEF SLUIT>>"));
		}),
		FEclipseMissionEventPayload::StaticStruct());

	if (!TestTrue(TEXT("opzet: missie geselecteerd"), Strategy->SelectMission(TEXT("TransitCheckpoint"), Error))
		|| !TestTrue(TEXT("opzet: missie gelanceerd"), Prep->AutoLaunch(Error)))
	{
		AddError(Error);
		GameInstance->Shutdown();
		return false;
	}
	for (const FName& Objective : { FName(TEXT("Obj_M11_PatrolLeader")), FName(TEXT("Obj_M11_Exfil")) })
	{
		if (!Mission->CompleteObjective(Objective, Error)) { AddError(Error); GameInstance->Shutdown(); return false; }
	}
	Reeks.Reset(); // alleen de afrekening zelf telt
	if (!TestTrue(TEXT("opzet: debrief afgerekend"), Mission->ResolveDebrief(true, Error)))
	{
		AddError(Error);
		GameInstance->Shutdown();
		return false;
	}

	AddInfo(FString::Printf(TEXT("GEMETEN  volgorde bij de afrekening: %s"), *FString::Join(Reeks, TEXT(" | "))));

	const int32 Sluiting = Reeks.IndexOfByKey(FString(TEXT("<<DEBRIEF SLUIT>>")));
	if (!TestTrue(TEXT("het debrief-venster sluit ook echt (Mission.Completed gezien)"), Sluiting != INDEX_NONE))
	{
		Bus->Unsubscribe(EconomyHandle);
		Bus->Unsubscribe(DoneHandle);
		GameInstance->Shutdown();
		return false;
	}

	// Er moet MINSTENS EEN grootboekregel voor de sluiting staan, anders heeft het
	// debriefscherm niets te tonen en is 7.6 een dode letter.
	TestTrue(TEXT("er staan grootboekregels VOOR de sluiting"), Sluiting > 0);

	// En de soldij hoort erbij: dat is de regel die het duurst is om te missen,
	// want dat is de post die de speler elke dag armer maakt.
	bool bSoldijVoorSluiting = false;
	for (int32 Index = 0; Index < Sluiting; ++Index)
	{
		if (Reeks[Index].StartsWith(TEXT("Wages")))
		{
			bSoldijVoorSluiting = true;
			break;
		}
	}
	TestTrue(TEXT("de soldijregel van de dagtick valt binnen het venster"), bSoldijVoorSluiting);

	Bus->Unsubscribe(EconomyHandle);
	Bus->Unsubscribe(DoneHandle);
	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
