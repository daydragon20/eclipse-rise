// Story-sequencing unit tier (SPEC-P2-04 build step 2/3). The pinned-offer
// resolution and beat idempotence are pure — the matrix here is the "each
// beat unlocks exactly its successor" contract from the spec's test list,
// exercised over a three-row M1 chain (M1.4's pattern is structurally
// identical to M1.3: unlock-by-predecessor on its own region) without any
// table asset or campaign state.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Quests/EclipseStoryLogic.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
