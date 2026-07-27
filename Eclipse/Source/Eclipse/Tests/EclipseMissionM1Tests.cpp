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

	// DE SUBTIELSTE REGEL VAN DEZE SPEC. M1.3 IS de eerste wereldstaat-wijziging,
	// maar hij voert hem NIET ZELF uit: zijn completion is de naad die SPEC-P2-05
	// consumeert, en die commit de Foothold-flips. Zolang P2-05 niet bestaat mag
	// er dus niets omdraaien - en als P2-05 er is, hoort deze assertie te
	// verhuizen in plaats van stilletjes te blijven kloppen.
	const FEclipseRegionState* RegionAfter = Campaign->GetState().FindRegion(TEXT("TransitCheckpoint"));
	if (TestNotNull(TEXT("regio bestaat nog na de debrief"), RegionAfter))
	{
		TestTrue(TEXT("M1.3 draait zelf GEEN regio om - dat is de P2-05-naad"),
			RegionAfter->Owner == OwnerBefore);
	}

	Bus->Unsubscribe(EconomyHandle);
	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
