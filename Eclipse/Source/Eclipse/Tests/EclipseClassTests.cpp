// Tests for SPEC-P2-01 "Squad of 4 & Classes" (GDD 14.4, blocking): class-kit
// resolution incl. the classless fallback, stabilize-window edge math, the
// per-class order modulation, save round-trip + v2 migration for ClassId, the
// "Medic stabilizes downed Assault under fire" scenario at the mission seam,
// and the 4-deployed x 3-mission soak with roster-consistency assertions.

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipsePrepSubsystem.h"
#include "Base/EclipsePrepTypes.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipseClassLogic.h"
#include "Core/EclipseGameplayTags.h"
#include "EclipseSaveSubsystem.h"
#include "EclipseSaveSystem.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Squad/EclipseRosterLogic.h"
#include "Squad/EclipseRosterTypes.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Squad/EclipseSquadTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"

namespace EclipseClassTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** The three slice classes as an in-memory DT_ClassDefs (SPEC-P2-01 locked decision 1). */
	UDataTable* MakeClassDefsTable()
	{
		UDataTable* Table = NewObject<UDataTable>();
		Table->RowStruct = FEclipseClassDefRow::StaticStruct();

		FEclipseClassDefRow Assault;
		Assault.DisplayName = FText::FromString(TEXT("Assault"));
		Assault.WeaponRow = TEXT("AR_Foundry");
		Assault.BodyDefOverride = TEXT("Rebel_A");
		Assault.SignatureVerb = EclipseTags::Class_Verb_Momentum.GetTag();
		Assault.BarkSet = TEXT("Barks_Assault");
		Assault.OrderPushDistanceCm = 300.0f;
		Table->AddRow(TEXT("Assault"), Assault);

		FEclipseClassDefRow Medic;
		Medic.DisplayName = FText::FromString(TEXT("Medic"));
		Medic.WeaponRow = TEXT("SMG_Patch");
		Medic.BodyDefOverride = TEXT("Rebel_B");
		Medic.SignatureVerb = EclipseTags::Class_Verb_Stabilize.GetTag();
		Medic.StabilizeWindowSeconds = 30.0f; // the GDD 4.2.5 window
		Medic.BarkSet = TEXT("Barks_Medic");
		Medic.bAutoTriage = true;
		Table->AddRow(TEXT("Medic"), Medic);

		FEclipseClassDefRow Sniper;
		Sniper.DisplayName = FText::FromString(TEXT("Sniper"));
		Sniper.WeaponRow = TEXT("DMR_Longsight");
		Sniper.BodyDefOverride = TEXT("Rebel_C");
		Sniper.SignatureVerb = EclipseTags::Class_Verb_Killzone.GetTag();
		Sniper.KillzoneRangeCm = 6000.0f;
		Sniper.BarkSet = TEXT("Barks_Sniper");
		Sniper.CoverLaneBias = 1.0f;
		Table->AddRow(TEXT("Sniper"), Sniper);

		return Table;
	}

	struct FFixture
	{
		UGameInstance* GameInstance = nullptr;
		UEclipseCampaignSubsystem* Campaign = nullptr;
		UEclipseStrategySubsystem* Strategy = nullptr;
		UEclipseMissionSubsystem* Mission = nullptr;
		UEclipsePrepSubsystem* Prep = nullptr;
		UEclipseSaveSubsystem* Save = nullptr;

		static FFixture Make()
		{
			FFixture Fixture;
			Fixture.GameInstance = NewObject<UGameInstance>(GEngine);
			Fixture.GameInstance->InitializeStandalone();
			Fixture.Campaign = Fixture.GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
			Fixture.Strategy = Fixture.GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
			Fixture.Mission = Fixture.GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
			Fixture.Prep = Fixture.GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
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

	/** Small board with two Dominion targets, so a 3-mission soak never runs out of legal offers. */
	UEclipseRegionGraphAsset* MakeBoard()
	{
		UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
		FEclipseRegionDefinition& Home = Graph->Regions.AddDefaulted_GetRef();
		Home.RegionId = TEXT("Underworks");
		Home.StartingOwner = EEclipseRegionOwner::Player;
		Home.ConnectedRegionIds = { TEXT("Checkpoint"), TEXT("Depot") };
		FEclipseRegionDefinition& Target = Graph->Regions.AddDefaulted_GetRef();
		Target.RegionId = TEXT("Checkpoint");
		Target.RegionType = EEclipseRegionType::Checkpoint;
		Target.StartingOwner = EEclipseRegionOwner::Dominion;
		Target.ConnectedRegionIds = { TEXT("Underworks") };
		FEclipseRegionDefinition& Second = Graph->Regions.AddDefaulted_GetRef();
		Second.RegionId = TEXT("Depot");
		Second.RegionType = EEclipseRegionType::Checkpoint;
		Second.StartingOwner = EEclipseRegionOwner::Dominion;
		Second.ConnectedRegionIds = { TEXT("Underworks") };

		UDataTable* Offers = NewObject<UDataTable>();
		Offers->RowStruct = FEclipseMissionOfferRow::StaticStruct();
		FEclipseMissionOfferRow Offer;
		Offer.RegionType = EEclipseRegionType::Checkpoint;
		Offer.TemplateId = TEXT("MT_Assault");
		Offer.RewardCredits = 60;
		Offers->AddRow(TEXT("Offer_Checkpoint"), Offer);
		Graph->MissionOffers = Offers;
		return Graph;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassKitResolutionTest,
	"Eclipse.Characters.Class.KitResolutionAndClasslessFallback",
	EclipseClassTest::TestFlags)

bool FEclipseClassKitResolutionTest::RunTest(const FString& Parameters)
{
	UDataTable* ClassDefs = EclipseClassTest::MakeClassDefsTable();

	FEclipseSoldierRecord Medic;
	Medic.SoldierId = FGuid(1, 2, 3, 4);
	Medic.Name = TEXT("Vara Chen");
	Medic.ClassId = TEXT("Medic");

	// Full kit from the row: everything a class may change, nothing else.
	const FEclipseClassDefRow* MedicRow = ClassDefs->FindRow<FEclipseClassDefRow>(TEXT("Medic"), TEXT("Test"));
	const EclipseClassLogic::FEclipseResolvedClassKit Kit = EclipseClassLogic::ResolveClassKit(Medic, MedicRow);
	TestEqual(TEXT("Kit carries the class id"), Kit.ClassId, FName(TEXT("Medic")));
	TestEqual(TEXT("Weapon row from data"), Kit.WeaponRow, FName(TEXT("SMG_Patch")));
	TestEqual(TEXT("Body override from data"), Kit.BodyDefOverride, FName(TEXT("Rebel_B")));
	TestTrue(TEXT("Signature verb resolved"), Kit.SignatureVerb == EclipseTags::Class_Verb_Stabilize.GetTag());
	TestEqual(TEXT("Stabilize window 30 s (GDD 4.2.5)"), Kit.StabilizeWindowSeconds, 30.0f);
	TestEqual(TEXT("Bark set from data"), Kit.BarkSet, FName(TEXT("Barks_Medic")));
	TestTrue(TEXT("Medic auto-triages"), Kit.bAutoTriage);

	const FEclipseClassDefRow* SniperRow = ClassDefs->FindRow<FEclipseClassDefRow>(TEXT("Sniper"), TEXT("Test"));
	FEclipseSoldierRecord Sniper = Medic;
	Sniper.ClassId = TEXT("Sniper");
	const EclipseClassLogic::FEclipseResolvedClassKit SniperKit = EclipseClassLogic::ResolveClassKit(Sniper, SniperRow);
	TestEqual(TEXT("Killzone range 6000 cm from data"), SniperKit.KillzoneRangeCm, 6000.0f);
	TestEqual(TEXT("Sniper lane bias from data"), SniperKit.CoverLaneBias, 1.0f);
	TestEqual(TEXT("Sniper cannot stabilize (data, not class checks)"), SniperKit.StabilizeWindowSeconds, 0.0f);

	// Missing row -> classless fallback, id preserved for display, never a crash.
	FEclipseSoldierRecord Orphan;
	Orphan.ClassId = TEXT("Heavy"); // Phase 3 class — no row in the slice table
	const EclipseClassLogic::FEclipseResolvedClassKit Fallback = EclipseClassLogic::ResolveClassKit(Orphan, nullptr);
	TestEqual(TEXT("Fallback keeps the unresolved id"), Fallback.ClassId, FName(TEXT("Heavy")));
	TestEqual(TEXT("Fallback has no weapon override"), Fallback.WeaponRow, FName(NAME_None));
	TestEqual(TEXT("Fallback has no body override"), Fallback.BodyDefOverride, FName(NAME_None));
	TestFalse(TEXT("Fallback has no verb"), Fallback.SignatureVerb.IsValid());
	TestEqual(TEXT("Fallback cannot stabilize"), Fallback.StabilizeWindowSeconds, 0.0f);
	TestFalse(TEXT("Fallback does not triage"), Fallback.bAutoTriage);

	// Classless record (NAME_None) resolves the same shape.
	const EclipseClassLogic::FEclipseResolvedClassKit Recruit = EclipseClassLogic::ResolveClassKit(FEclipseSoldierRecord(), nullptr);
	TestEqual(TEXT("Recruit kit is classless"), Recruit.ClassId, FName(NAME_None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassStabilizeWindowTest,
	"Eclipse.Characters.Class.StabilizeWindowMath",
	EclipseClassTest::TestFlags)

bool FEclipseClassStabilizeWindowTest::RunTest(const FString& Parameters)
{
	using namespace EclipseClassLogic;

	TestTrue(TEXT("Inside the window"), IsStabilizeWindowOpen(100.0, 115.0, 30.0f));
	TestTrue(TEXT("Down at the window edge still saves (inclusive)"), IsStabilizeWindowOpen(100.0, 130.0, 30.0f));
	TestFalse(TEXT("Just past the edge is a death"), IsStabilizeWindowOpen(100.0, 130.0001, 30.0f));
	TestTrue(TEXT("Immediate save"), IsStabilizeWindowOpen(100.0, 100.0, 30.0f));
	TestTrue(TEXT("Clock skew reads as elapsed zero"), IsStabilizeWindowOpen(100.0, 99.9, 30.0f));
	TestFalse(TEXT("Window 0 = this soldier cannot stabilize"), IsStabilizeWindowOpen(100.0, 100.0, 0.0f));
	TestFalse(TEXT("Negative window = cannot stabilize"), IsStabilizeWindowOpen(100.0, 100.0, -5.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassOrderModulationTest,
	"Eclipse.Squad.Class.OrderModulationInData",
	EclipseClassTest::TestFlags)

bool FEclipseClassOrderModulationTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	// Assault push: past the ordered point along the approach, planar.
	const FVector Soldier(0.0f, 0.0f, 0.0f);
	const FVector Ordered(1000.0f, 0.0f, 0.0f);
	TestEqual(TEXT("Zero push obeys the letter of the order"), ComputePushedOrderPoint(Soldier, Ordered, 0.0f), Ordered);
	const FVector Pushed = ComputePushedOrderPoint(Soldier, Ordered, 300.0f);
	TestTrue(TEXT("Assault pushes 300 cm past the point"), Pushed.Equals(FVector(1300.0f, 0.0f, 0.0f), 0.1f));
	TestEqual(TEXT("Degenerate direction returns the order unchanged"), ComputePushedOrderPoint(Ordered, Ordered, 300.0f), Ordered);

	// Cover scorer: cover always dominates; bias only re-ranks covered samples.
	const float CoveredNear = ScoreCoverSample(true, /*ToOrder*/ 100.0f, /*ToThreat*/ 900.0f, /*Bias*/ 0.0f);
	const float CoveredFar = ScoreCoverSample(true, 300.0f, 1400.0f, 0.0f);
	const float OpenFar = ScoreCoverSample(false, 50.0f, 2000.0f, 0.0f);
	TestTrue(TEXT("Bias 0: nearest covered sample wins (SPEC-P1-06 behavior kept)"), CoveredNear > CoveredFar);
	TestTrue(TEXT("Cover beats open ground"), CoveredFar > OpenFar);

	const float SniperNear = ScoreCoverSample(true, 100.0f, 900.0f, 1.0f);
	const float SniperFar = ScoreCoverSample(true, 300.0f, 1400.0f, 1.0f);
	TestTrue(TEXT("Sniper bias prefers the longer covered lane"), SniperFar > SniperNear);
	const float SniperOpen = ScoreCoverSample(false, 100.0f, 99000.0f, 1.0f);
	TestTrue(TEXT("No bias talks a soldier out of cover (GDD 9.5 bug bar)"), SniperNear > SniperOpen);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassSaveRoundTripTest,
	"Eclipse.Strategy.Campaign.SaveRoundTripWithClassIds",
	EclipseClassTest::TestFlags)

bool FEclipseClassSaveRoundTripTest::RunTest(const FString& Parameters)
{
	const FString SlotName = TEXT("AutomationClassRoundTrip");
	IFileManager::Get().Delete(*UEclipseSaveSubsystem::GetSlotFilePath(SlotName), false, true, true);

	EclipseClassTest::FFixture Source = EclipseClassTest::FFixture::Make();

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 5;
	Setup->ClassDefs = EclipseClassTest::MakeClassDefsTable();
	Source.Campaign->StartNewCampaign(Setup);

	// Pre-classed authored recruits (SPEC-P2-01 decision 2): table order cycles.
	const TArray<FEclipseSoldierRecord>& Roster = Source.Campaign->GetState().Roster;
	TestEqual(TEXT("Five recruits mustered"), Roster.Num(), 5);
	TestEqual(TEXT("Recruit 1 is Assault"), Roster[0].ClassId, FName(TEXT("Assault")));
	TestEqual(TEXT("Recruit 2 is Medic"), Roster[1].ClassId, FName(TEXT("Medic")));
	TestEqual(TEXT("Recruit 3 is Sniper"), Roster[2].ClassId, FName(TEXT("Sniper")));
	TestEqual(TEXT("Recruit 4 cycles back to Assault"), Roster[3].ClassId, FName(TEXT("Assault")));

	FString Error;
	const uint32 SourceHash = Source.Campaign->GetState().ComputeStateHash();
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	EclipseClassTest::FFixture Target = EclipseClassTest::FFixture::Make();
	TestTrue(TEXT("Load succeeds"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("Current-version file needs no migration"), Target.Save->GetLastLoadMigrationStepCount(), 0);
	TestEqual(TEXT("Round-tripped hash matches (ClassId is hashed)"), Target.Campaign->GetState().ComputeStateHash(), SourceHash);

	const TArray<FEclipseSoldierRecord>& Loaded = Target.Campaign->GetState().Roster;
	TestEqual(TEXT("Roster size survives"), Loaded.Num(), 5);
	for (int32 Index = 0; Index < Loaded.Num() && Index < Roster.Num(); ++Index)
	{
		TestEqual(TEXT("ClassId survives the round trip"), Loaded[Index].ClassId, Roster[Index].ClassId);
	}

	IFileManager::Get().Delete(*UEclipseSaveSubsystem::GetSlotFilePath(SlotName), false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassSaveMigrationTest,
	"Eclipse.Strategy.Campaign.SaveMigrationV2RecordsWithoutClassId",
	EclipseClassTest::TestFlags)

bool FEclipseClassSaveMigrationTest::RunTest(const FString& Parameters)
{
	const FString SlotName = TEXT("AutomationClassMigration");
	const FString SlotPath = UEclipseSaveSubsystem::GetSlotFilePath(SlotName);
	IFileManager::Get().Delete(*SlotPath, false, true, true);

	EclipseClassTest::FFixture Source = EclipseClassTest::FFixture::Make();

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 4;
	Setup->ClassDefs = EclipseClassTest::MakeClassDefsTable();
	Source.Campaign->StartNewCampaign(Setup);

	FString Error;
	TestTrue(TEXT("Save succeeds"), Source.Save->SaveToSlot(SlotName, Error));

	// Reconstruct a byte-faithful v2 file from the v3 save: strip the trailing
	// ClassId tail (count + FNames-as-ANSI-strings) from the Campaign block —
	// the only registered block, so it ends the file — then rewrite the file
	// header version, the block size, and the block's leading state version.
	// Layout (save container contract): [u32 Magic][i32 Ver][i32 Blocks]
	// [FString "Campaign" = i32 9 + 9 bytes][i64 BlockSize][block bytes...].
	TArray<uint8> FileBytes;
	TestTrue(TEXT("Save file readable"), FFileHelper::LoadFileToArray(FileBytes, *SlotPath));

	const int32 HeaderVersionOffset = sizeof(uint32);
	const int32 BlockSizeOffset = 12 + (4 + 9);
	const int32 BlockStartOffset = BlockSizeOffset + sizeof(int64);
	TestEqual(TEXT("Sanity: file header is v3"), *reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset), 3);
	TestEqual(TEXT("Sanity: block leads with state schema v3"), *reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset), 3);

	int32 TailSize = sizeof(int32);
	for (const FEclipseSoldierRecord& Soldier : Source.Campaign->GetState().Roster)
	{
		TailSize += sizeof(int32) + Soldier.ClassId.ToString().Len() + 1; // ANSI FString: len-with-null + bytes
	}

	*reinterpret_cast<int32*>(FileBytes.GetData() + HeaderVersionOffset) = 2;
	*reinterpret_cast<int32*>(FileBytes.GetData() + BlockStartOffset) = 2;
	*reinterpret_cast<int64*>(FileBytes.GetData() + BlockSizeOffset) -= TailSize;
	FileBytes.SetNum(FileBytes.Num() - TailSize);
	TestTrue(TEXT("v2-shaped file written"), FFileHelper::SaveArrayToFile(FileBytes, *SlotPath));

	EclipseClassTest::FFixture Target = EclipseClassTest::FFixture::Make();
	TestTrue(TEXT("v2 file loads via migration"), Target.Save->LoadFromSlot(SlotName, Error));
	TestEqual(TEXT("Exactly the 2->3 step ran"), Target.Save->GetLastLoadMigrationStepCount(), 1);

	const TArray<FEclipseSoldierRecord>& Loaded = Target.Campaign->GetState().Roster;
	TestEqual(TEXT("All soldiers came home"), Loaded.Num(), 4);
	for (int32 Index = 0; Index < Loaded.Num(); ++Index)
	{
		TestEqual(TEXT("v2 records migrate to classless Recruits (GDD 4.2.3 default)"), Loaded[Index].ClassId, FName(NAME_None));
		TestEqual(TEXT("Names survive the migration"), Loaded[Index].Name, Source.Campaign->GetState().Roster[Index].Name);
	}
	TestEqual(TEXT("Campaign day survives"), Target.Campaign->GetState().Day, Source.Campaign->GetState().Day);

	IFileManager::Get().Delete(*SlotPath, false, true, true);
	Source.Shutdown();
	Target.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassMedicStabilizeScenarioTest,
	"Eclipse.Squad.Class.MedicStabilizesDownedAssaultUnderFire",
	EclipseClassTest::TestFlags)

bool FEclipseClassMedicStabilizeScenarioTest::RunTest(const FString& Parameters)
{
	// The SPEC-P2-01 scenario at the mission seam, headless with a hand-driven
	// clock: Assault goes down under fire, the Medic's *data* window converts
	// the down into a survivable wound at a FAILED debrief — the exact case
	// that used to be permadeath (GDD 4.2.5).
	EclipseClassTest::FFixture Fixture = EclipseClassTest::FFixture::Make();

	UDataTable* ClassDefs = EclipseClassTest::MakeClassDefsTable();
	UEclipseRosterTuningAsset* RosterTuning = NewObject<UEclipseRosterTuningAsset>();
	RosterTuning->WoundedDaysOut = 5;

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 4;
	Setup->ClassDefs = ClassDefs;
	Setup->RosterTuning = RosterTuning;
	Setup->RegionGraph = EclipseClassTest::MakeBoard();
	Fixture.Campaign->StartNewCampaign(Setup);

	const TArray<FEclipseSoldierRecord>& Roster = Fixture.Campaign->GetState().Roster;
	const FGuid AssaultId = Roster[0].SoldierId; // pre-classed Assault
	const FGuid MedicId = Roster[1].SoldierId;   // pre-classed Medic
	const FGuid SniperId = Roster[2].SoldierId;  // pre-classed Sniper
	TestEqual(TEXT("Downed soldier is the Assault"), Roster[0].ClassId, FName(TEXT("Assault")));
	TestEqual(TEXT("Rescuer is the Medic"), Roster[1].ClassId, FName(TEXT("Medic")));

	// The window comes from the *rescuer's* kit — resolved data, no Medic branch.
	const EclipseClassLogic::FEclipseResolvedClassKit MedicKit = EclipseClassLogic::ResolveClassKit(
		Roster[1], ClassDefs->FindRow<FEclipseClassDefRow>(Roster[1].ClassId, TEXT("Test")));
	const EclipseClassLogic::FEclipseResolvedClassKit SniperKit = EclipseClassLogic::ResolveClassKit(
		Roster[2], ClassDefs->FindRow<FEclipseClassDefRow>(Roster[2].ClassId, TEXT("Test")));

	FString Error;
	TestTrue(TEXT("Mission selected"), Fixture.Strategy->SelectMission(TEXT("Checkpoint"), Error));
	TestTrue(TEXT("Mission starts with the squad of 4's three squadmates"),
		Fixture.Mission->StartMission({ AssaultId, MedicId, SniperId }, Error));

	// Assault down at t=100 under fire.
	Fixture.Mission->NotifySoldierDownedAt(AssaultId, TEXT("Gunfire"), 100.0);

	// A kit without a window cannot stabilize, however fast it arrives (data decides).
	TestFalse(TEXT("Sniper kit cannot stabilize"), Fixture.Mission->TryStabilizeSoldier(AssaultId, SniperKit.StabilizeWindowSeconds, 101.0));

	// The Medic arrives 29.5 s in — inside the 30 s data window.
	TestTrue(TEXT("Medic stabilize inside the window succeeds"),
		Fixture.Mission->TryStabilizeSoldier(AssaultId, MedicKit.StabilizeWindowSeconds, 129.5));
	TestFalse(TEXT("A down is saved once"), Fixture.Mission->TryStabilizeSoldier(AssaultId, MedicKit.StabilizeWindowSeconds, 129.9));
	TestTrue(TEXT("Stabilized set reports the save"), Fixture.Mission->GetStabilizedSoldierIds().Contains(AssaultId));

	// A second down whose window expires: the Medic is 31 s late.
	Fixture.Mission->NotifySoldierDownedAt(SniperId, TEXT("Gunfire"), 200.0);
	TestFalse(TEXT("Past the window the save fails"),
		Fixture.Mission->TryStabilizeSoldier(SniperId, MedicKit.StabilizeWindowSeconds, 231.0));

	// Failed extraction: the stabilized Assault survives as Wounded; the
	// unstabilized Sniper is the SPEC-P1-07 failure-branch death.
	TestTrue(TEXT("Failed debrief still commits (fail-forward)"), Fixture.Mission->ResolveDebrief(false, Error));
	const FEclipseCampaignState& State = Fixture.Campaign->GetState();
	TestTrue(TEXT("Stabilized Assault comes home Wounded, not dead (the Medic save)"),
		State.FindSoldier(AssaultId)->Status == EEclipseSoldierStatus::Wounded);
	TestEqual(TEXT("Wound duration from roster data"), State.FindSoldier(AssaultId)->WoundedUntilDay, State.Day + 5);
	TestTrue(TEXT("Unstabilized Sniper died on the failed mission"),
		State.FindSoldier(SniperId)->Status == EEclipseSoldierStatus::Dead);
	TestEqual(TEXT("Exactly one memorial entry — the wall got one name, not two"), State.Memorial.Num(), 1);
	TestEqual(TEXT("The memorial names the Sniper"), State.Memorial[0].SoldierId, SniperId);

	Fixture.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassSquadOfFourSoakTest,
	"Eclipse.Squad.Class.SquadOfFourSoakRosterConsistency",
	EclipseClassTest::TestFlags)

bool FEclipseClassSquadOfFourSoakTest::RunTest(const FString& Parameters)
{
	// Soak (SPEC-P2-01 test plan): 4-deployed (player + 3 picked) x 3 mission
	// loops through the real prep -> launch -> debrief pipeline, with roster
	// consistency asserted after every loop.
	EclipseClassTest::FFixture Fixture = EclipseClassTest::FFixture::Make();

	UEclipseSquadTuningAsset* SquadTuning = NewObject<UEclipseSquadTuningAsset>();
	SquadTuning->MaxDeployed = 4; // the SPEC-P2-01 number, in data

	UEclipsePrepTuningAsset* PrepTuning = NewObject<UEclipsePrepTuningAsset>();
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
	Setup->StartingRosterSize = 8;
	Setup->ClassDefs = EclipseClassTest::MakeClassDefsTable();
	Setup->SquadTuning = SquadTuning;
	Setup->PrepTuning = PrepTuning;
	Setup->RegionGraph = EclipseClassTest::MakeBoard();
	Fixture.Campaign->StartNewCampaign(Setup);

	TestEqual(TEXT("Prep derives 3 squadmates from MaxDeployed 4"), Fixture.Prep->ResolveSquadmateCount(), 3);

	// Snapshot identity for the consistency assertions.
	TMap<FGuid, FName> ClassById;
	for (const FEclipseSoldierRecord& Soldier : Fixture.Campaign->GetState().Roster)
	{
		ClassById.Add(Soldier.SoldierId, Soldier.ClassId);
	}
	TestEqual(TEXT("Eight recruits mustered"), ClassById.Num(), 8);

	FString Error;
	for (int32 Loop = 0; Loop < 3; ++Loop)
	{
		// Alternate targets: a win progresses a region one step, and a
		// player-held region legally stops offering missions (GDD 6.3.2).
		const FName TargetRegion = (Loop % 2 == 0) ? FName(TEXT("Checkpoint")) : FName(TEXT("Depot"));
		TestTrue(FString::Printf(TEXT("Loop %d: offer selected"), Loop), Fixture.Strategy->SelectMission(TargetRegion, Error));
		TestTrue(FString::Printf(TEXT("Loop %d: auto-launch (squad of 4 scale)"), Loop), Fixture.Prep->AutoLaunch(Error));
		TestEqual(FString::Printf(TEXT("Loop %d: three squadmates deployed beside the player"), Loop),
			Fixture.Mission->GetDeployedSoldierIds().Num(), 3);

		// No duplicate deployment inside one launch.
		TSet<FGuid> Unique(Fixture.Mission->GetDeployedSoldierIds());
		TestEqual(FString::Printf(TEXT("Loop %d: deployed ids unique"), Loop), Unique.Num(), 3);

		TestTrue(FString::Printf(TEXT("Loop %d: primary completes"), Loop), Fixture.Mission->CompleteObjective(TEXT("Obj_Primary"), Error));
		TestTrue(FString::Printf(TEXT("Loop %d: exfil completes"), Loop), Fixture.Mission->CompleteObjective(TEXT("Obj_Exfil"), Error));
		TestTrue(FString::Printf(TEXT("Loop %d: debrief commits"), Loop), Fixture.Mission->ResolveDebrief(true, Error));

		// Roster consistency after every loop (the soak's whole point).
		const FEclipseCampaignState& State = Fixture.Campaign->GetState();
		TestEqual(FString::Printf(TEXT("Loop %d: roster size stable"), Loop), State.Roster.Num(), 8);
		int32 ServedTotal = 0;
		for (const FEclipseSoldierRecord& Soldier : State.Roster)
		{
			const FName* OriginalClass = ClassById.Find(Soldier.SoldierId);
			TestNotNull(TEXT("No soldier appears from nowhere"), OriginalClass);
			if (OriginalClass != nullptr)
			{
				TestEqual(TEXT("ClassId never drifts across missions"), Soldier.ClassId, *OriginalClass);
			}
			TestTrue(TEXT("Nobody dies or wounds on clean wins"), Soldier.Status == EEclipseSoldierStatus::Available);
			ServedTotal += Soldier.MissionsServed;
		}
		TestEqual(FString::Printf(TEXT("Loop %d: served count = 3 per completed mission"), Loop), ServedTotal, (Loop + 1) * 3);
	}

	Fixture.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
