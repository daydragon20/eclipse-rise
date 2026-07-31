// Story-sequencing unit tier (SPEC-P2-04 build step 2/3). The pinned-offer
// resolution and beat idempotence are pure — the matrix here is the "each
// beat unlocks exactly its successor" contract from the spec's test list,
// exercised over a three-row M1 chain (M1.4's pattern is structurally
// identical to M1.3: unlock-by-predecessor on its own region) without any
// table asset or campaign state.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/EclipseGameplayTags.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Quests/EclipseStoryLogic.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategySubsystem.h"

namespace EclipseStoryTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	FGameplayTag Beat(const TCHAR* Name)
	{
		// Test-local tags: AddNativeGameplayTag is closed by now, so request
		// registered leaves only. We reuse registered Class.Verb tags as
		// arbitrary distinct beat identities — the logic never inspects
		// families, only exact membership.
		return FGameplayTag::RequestGameplayTag(Name, /*ErrorIfNotFound*/ false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStorySequencingTest,
	"Eclipse.Story.PinnedOfferSequencing",
	EclipseStoryTest::TestFlags)

bool FEclipseStorySequencingTest::RunTest(const FString& Parameters)
{
	using namespace EclipseStoryLogic;

	// Three distinct registered tags stand in for beat identities.
	const FGameplayTag BeatM11 = EclipseStoryTest::Beat(TEXT("Class.Verb.Momentum"));
	const FGameplayTag BeatM12 = EclipseStoryTest::Beat(TEXT("Class.Verb.Stabilize"));
	const FGameplayTag BeatM13 = EclipseStoryTest::Beat(TEXT("Class.Verb.Killzone"));
	if (!BeatM11.IsValid() || !BeatM12.IsValid() || !BeatM13.IsValid())
	{
		return TestTrue(TEXT("Native tags available for the beat matrix"), false);
	}

	// M1.1 (no unlock) -> M1.2 (needs M11 beat) -> M1.3 (needs M12 beat),
	// all pinned to the same district region like the slice authors them.
	FEclipseStoryMissionRow M11;
	M11.MissionId = TEXT("MT_M11");
	M11.PinnedRegionId = TEXT("AmbushBlock");
	M11.CompletionBeatTag = BeatM11;

	FEclipseStoryMissionRow M12;
	M12.MissionId = TEXT("MT_M12");
	M12.PinnedRegionId = TEXT("AmbushBlock");
	M12.UnlockBeatTag = BeatM11;
	M12.CompletionBeatTag = BeatM12;

	FEclipseStoryMissionRow M13;
	M13.MissionId = TEXT("MT_M13");
	M13.PinnedRegionId = TEXT("JammerBlock");
	M13.UnlockBeatTag = BeatM12;
	M13.CompletionBeatTag = BeatM13;

	const TArray<const FEclipseStoryMissionRow*> Rows = { &M11, &M12, &M13 };

	// Fresh campaign: only M1.1 pins; the locked region pins nothing.
	FGameplayTagContainer Flags;
	const FEclipseStoryMissionRow* Pinned = ResolvePinnedMission(Rows, Flags, TEXT("AmbushBlock"));
	TestTrue(TEXT("Campaign start pins M1.1"), Pinned != nullptr && Pinned->MissionId == TEXT("MT_M11"));
	TestTrue(TEXT("M1.3's region pins nothing before its unlock"), ResolvePinnedMission(Rows, Flags, TEXT("JammerBlock")) == nullptr);

	// M1.1 done: its beat unlocks exactly its successor, and the done mission
	// never re-pins.
	Flags.AddTag(BeatM11);
	Pinned = ResolvePinnedMission(Rows, Flags, TEXT("AmbushBlock"));
	TestTrue(TEXT("M1.1's beat pins M1.2 (done mission never re-pins)"), Pinned != nullptr && Pinned->MissionId == TEXT("MT_M12"));
	TestTrue(TEXT("M1.3 still locked by its own unlock beat"), ResolvePinnedMission(Rows, Flags, TEXT("JammerBlock")) == nullptr);

	// M1.2 done: M1.3 unlocks on its own region; the first region is exhausted.
	Flags.AddTag(BeatM12);
	TestTrue(TEXT("Ambush region exhausted after both beats"), ResolvePinnedMission(Rows, Flags, TEXT("AmbushBlock")) == nullptr);
	Pinned = ResolvePinnedMission(Rows, Flags, TEXT("JammerBlock"));
	TestTrue(TEXT("M1.2's beat pins M1.3"), Pinned != nullptr && Pinned->MissionId == TEXT("MT_M13"));

	// Unknown region and damaged (null) rows degrade to no pin, never a crash.
	TestTrue(TEXT("Unknown region pins nothing"), ResolvePinnedMission(Rows, Flags, TEXT("Nowhere")) == nullptr);
	const TArray<const FEclipseStoryMissionRow*> Damaged = { nullptr, &M13 };
	TestTrue(TEXT("Null rows are skipped, later rows still resolve"),
		ResolvePinnedMission(Damaged, Flags, TEXT("JammerBlock")) == &M13);

	// Beat idempotence: a set beat never commits twice; an invalid tag never commits.
	TestTrue(TEXT("Unset beat commits"), ShouldCommitBeat(Flags, BeatM13));
	Flags.AddTag(BeatM13);
	TestFalse(TEXT("Set beat never commits twice (one Brick, ever)"), ShouldCommitBeat(Flags, BeatM13));
	TestFalse(TEXT("Invalid beat never commits"), ShouldCommitBeat(Flags, FGameplayTag()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStoryOfferPrecedenceTest,
	"Eclipse.Story.PinnedOfferBeatsRegionOffer",
	EclipseStoryTest::TestFlags)

bool FEclipseStoryOfferPrecedenceTest::RunTest(const FString& Parameters)
{
	// SPEC-P2-04 locked decision 4, wired: a story-pinned row wins the offer
	// query over the region-type offer, and retires to the generic offer the
	// moment its completion beat commits — through the real subsystems.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();

	// The loud half of the 14.3.5 ladder is part of the contract under test:
	// the orphan row below must warn twice (empty beat + unknown region), the
	// malformed-table swap once, and the pin-only region once after it retires.
	// Exact counts — the per-table validation must not re-fire across queries
	// or a second campaign on the same table.
	AddExpectedMessage(TEXT("has no CompletionBeatTag"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessage(TEXT("pins unknown region"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessage(TEXT("wrong row struct"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessage(TEXT("has no offer row for its type"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	UEclipseRegionGraphAsset* Graph = NewObject<UEclipseRegionGraphAsset>();
	{
		FEclipseRegionDefinition& Home = Graph->Regions.AddDefaulted_GetRef();
		Home.RegionId = TEXT("Underworks");
		Home.StartingOwner = EEclipseRegionOwner::Player;
		Home.Lanes = { TEXT("AmbushBlock"), TEXT("OldQuarter") };
		FEclipseRegionDefinition& Target = Graph->Regions.AddDefaulted_GetRef();
		Target.RegionId = TEXT("AmbushBlock");
		Target.RegionType = EEclipseRegionType::Checkpoint;
		Target.StartingOwner = EEclipseRegionOwner::Dominion;
		Target.Lanes = { TEXT("Underworks") };
		// Industrial has no generic offer row: the asymmetric path — this
		// region only ever surfaces through its story pin.
		FEclipseRegionDefinition& PinOnly = Graph->Regions.AddDefaulted_GetRef();
		PinOnly.RegionId = TEXT("OldQuarter");
		PinOnly.RegionType = EEclipseRegionType::Industrial;
		PinOnly.StartingOwner = EEclipseRegionOwner::Dominion;
		PinOnly.Lanes = { TEXT("Underworks") };

		UDataTable* Offers = NewObject<UDataTable>();
		Offers->RowStruct = FEclipseMissionOfferRow::StaticStruct();
		FEclipseMissionOfferRow Generic;
		Generic.RegionType = EEclipseRegionType::Checkpoint;
		Generic.TemplateId = TEXT("MT_Assault");
		Generic.RewardCredits = 60;
		Offers->AddRow(TEXT("Offer_Generic"), Generic);
		Graph->MissionOffers = Offers;
	}

	UDataTable* StoryTable = NewObject<UDataTable>();
	StoryTable->RowStruct = FEclipseStoryMissionRow::StaticStruct();
	{
		FEclipseStoryMissionRow M11;
		M11.MissionId = TEXT("MT_M11");
		M11.PinnedRegionId = TEXT("AmbushBlock");
		M11.CompletionBeatTag = EclipseTags::Story_Beat_M11_ThirteenBullets.GetTag();
		M11.RewardMaterials = 25;
		M11.RewardCredits = 50;
		StoryTable->AddRow(TEXT("M11"), M11);

		FEclipseStoryMissionRow M12;
		M12.MissionId = TEXT("MT_M12");
		M12.PinnedRegionId = TEXT("OldQuarter");
		M12.CompletionBeatTag = EclipseTags::Story_Beat_M12_DeadDrop.GetTag();
		M12.RewardIntel = 8;
		StoryTable->AddRow(TEXT("M12"), M12);

		// Both loud-validation defects on one row: no beat, unknown region.
		FEclipseStoryMissionRow Orphan;
		Orphan.MissionId = TEXT("MT_Orphan");
		Orphan.PinnedRegionId = TEXT("Nowhere");
		StoryTable->AddRow(TEXT("M_Orphan"), Orphan);
	}

	UEclipseCampaignSetupAsset* Setup = NewObject<UEclipseCampaignSetupAsset>();
	Setup->StartingDay = 1;
	Setup->StartingRosterSize = 2;
	Setup->RegionGraph = Graph;
	Setup->StoryMissions = StoryTable;
	Campaign->StartNewCampaign(Setup);

	FEclipseMissionOfferView Offer;
	TestTrue(TEXT("Offer resolves"), Strategy->TryGetOffer(TEXT("AmbushBlock"), Offer));
	TestEqual(TEXT("Story pin wins the offer query"), Offer.TemplateId, FName(TEXT("MT_M11")));
	TestEqual(TEXT("Authored reward band surfaces"), Offer.RewardMaterials, 25);
	TestTrue(TEXT("Pin surfaces a region with no generic offer"), Strategy->TryGetOffer(TEXT("OldQuarter"), Offer));
	TestEqual(TEXT("Pin-only region carries the story template"), Offer.TemplateId, FName(TEXT("MT_M12")));
	TestEqual(TEXT("Both pinned regions are on the board"), Strategy->GetAvailableOffers().Num(), 2);

	// Degradation while the pins are still live (order matters: after a retire
	// this assert could not tell the guard from the pin being done): a wrong-
	// shaped story table disables pins loudly-but-safely, the region offer
	// stands, and the resolver recovers the moment the table is fixed. NOTE:
	// resolution reads the setup's soft ptr live per query — if rows ever get
	// cached per campaign, this swap stops meaning what it tests.
	Setup->StoryMissions = Graph->MissionOffers.Get();
	TestTrue(TEXT("Offer resolves with a malformed story table"), Strategy->TryGetOffer(TEXT("AmbushBlock"), Offer));
	TestEqual(TEXT("Malformed story table degrades to the region offer"), Offer.TemplateId, FName(TEXT("MT_Assault")));
	TestFalse(TEXT("Pin-only region drops with pins disabled"), Strategy->TryGetOffer(TEXT("OldQuarter"), Offer));
	Setup->StoryMissions = StoryTable;
	TestTrue(TEXT("Offer resolves after the table is restored"), Strategy->TryGetOffer(TEXT("AmbushBlock"), Offer));
	TestEqual(TEXT("Restored table re-surfaces the pin"), Offer.TemplateId, FName(TEXT("MT_M11")));

	// Commit the completion beats: each pin retires the moment its beat lands.
	FString Error;
	FEclipseCampaignTransaction Done;
	Done.Source = TEXT("Test");
	FEclipseCampaignMutation& Beat = Done.Mutations.AddDefaulted_GetRef();
	Beat.Type = EEclipseCampaignMutationType::SetStoryFlag;
	Beat.StoryFlagTag = EclipseTags::Story_Beat_M11_ThirteenBullets.GetTag();
	TestTrue(TEXT("Completion beat commits"), Campaign->CommitTransaction(Done, Error));

	TestTrue(TEXT("Offer still resolves after the beat"), Strategy->TryGetOffer(TEXT("AmbushBlock"), Offer));
	TestEqual(TEXT("Done mission retires to the generic offer"), Offer.TemplateId, FName(TEXT("MT_Assault")));

	FEclipseCampaignTransaction Done12;
	Done12.Source = TEXT("Test");
	FEclipseCampaignMutation& Beat12 = Done12.Mutations.AddDefaulted_GetRef();
	Beat12.Type = EEclipseCampaignMutationType::SetStoryFlag;
	Beat12.StoryFlagTag = EclipseTags::Story_Beat_M12_DeadDrop.GetTag();
	TestTrue(TEXT("Second completion beat commits"), Campaign->CommitTransaction(Done12, Error));
	TestFalse(TEXT("Retired pin-only region has no offer left"), Strategy->TryGetOffer(TEXT("OldQuarter"), Offer));
	// The pin-only region now falls off the board with the skip-warning.
	TestEqual(TEXT("Board shrinks to the generic offer"), Strategy->GetAvailableOffers().Num(), 1);

	// A fresh campaign resets StoryFlags: the pins re-arm — and the already-
	// validated table must not warn again (the exact counts above prove it).
	Campaign->StartNewCampaign(Setup);
	TestTrue(TEXT("Offer resolves in the second campaign"), Strategy->TryGetOffer(TEXT("AmbushBlock"), Offer));
	TestEqual(TEXT("A new campaign re-arms the pin"), Offer.TemplateId, FName(TEXT("MT_M11")));

	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
