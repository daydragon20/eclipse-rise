// Feel-gauntlet overlay, headless tier (SPEC-P2-02 R3; phase0/FEEL_GAUNTLET_P2-02.md).
// The overlay itself is pixels, but every number it shows and every verdict it
// renders is pure logic and is swept here: the wall-clock order round trip (the
// one measurement time dilation could falsify), the Command Mode usage-pull
// tally, and the R3-VERDICT INPUT block the owner copies out of it.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Squad/EclipseCommandLogic.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "UI/EclipseGauntletOverlayLogic.h"

namespace EclipseGauntletTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** A criteria set where all five criteria pass — the baseline each case degrades from. */
	EclipseGauntletOverlay::FEclipseGauntletCriteria MakePassingCriteria()
	{
		using namespace EclipseGauntletOverlay;
		FEclipseGauntletCriteria Criteria;
		Criteria.RoundTripSamples = 10;
		Criteria.RoundTripWithinBar = 10;
		Criteria.RoundTripWorstSeconds = 0.004;
		Criteria.CleanPicks = 10;
		Criteria.MisPicks = 0;
		Criteria.Comfort = EEclipseGauntletAnswer::Good;
		Criteria.Confidence = EEclipseGauntletAnswer::Good;
		Criteria.EntriesThisBeat = 2;
		Criteria.ClosedBeatCount = 3;
		Criteria.AverageEntriesPerBeat = 1.7f;
		return Criteria;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGauntletRoundTripMeterTest,
	"Eclipse.Gauntlet.OrderRoundTripMeterJudgesTheAnswerBar",
	EclipseGauntletTest::TestFlags)

bool FEclipseGauntletRoundTripMeterTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	// The bar is the locked R3 number; if this ever drifts, the verdict the owner
	// rendered was rendered on a different game.
	TestEqual(TEXT("Answer bar is 1 second of real time"), FEclipseOrderRoundTripStats::BarSeconds, 1.0);

	FEclipseOrderRoundTripStats Stats;
	TestEqual(TEXT("Fresh meter has no samples"), Stats.SampleCount, 0);
	TestEqual(TEXT("Fresh meter averages 0"), Stats.GetAverageSeconds(), 0.0);

	Stats.NoteRoundTrip(0.25);
	Stats.NoteRoundTrip(0.50);
	TestEqual(TEXT("Two answers recorded"), Stats.SampleCount, 2);
	TestEqual(TEXT("Both inside the bar"), Stats.WithinBarCount, 2);
	TestEqual(TEXT("Worst is the slowest of the two"), Stats.WorstSeconds, 0.50);
	TestEqual(TEXT("Average of the two"), Stats.GetAverageSeconds(), 0.375);

	// Exactly on the bar still counts as answered in time (the criterion says
	// "within 1 second", not "under").
	Stats.NoteRoundTrip(1.0);
	TestEqual(TEXT("A round trip exactly on the bar is inside it"), Stats.WithinBarCount, 3);

	// One late answer is enough to falsify criterion 1 — the count must show it.
	Stats.NoteRoundTrip(1.2);
	TestEqual(TEXT("Four samples"), Stats.SampleCount, 4);
	TestEqual(TEXT("The late answer is not inside the bar"), Stats.WithinBarCount, 3);
	TestEqual(TEXT("Worst tracks the late answer"), Stats.WorstSeconds, 1.2);

	// A backwards clock (NTP step, thread migration) must not enter the tally as
	// a spectacular 0 s success in the other direction.
	Stats.NoteRoundTrip(-5.0);
	TestEqual(TEXT("Negative interval clamps to zero"), Stats.WorstSeconds, 1.2);
	TestEqual(TEXT("Clamped sample still counts as answered in time"), Stats.WithinBarCount, 4);

	Stats.Reset();
	TestEqual(TEXT("Reset clears the run"), Stats.SampleCount, 0);
	TestEqual(TEXT("Reset clears the worst case"), Stats.WorstSeconds, 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGauntletWallClockUnderDilationTest,
	"Eclipse.Gauntlet.RoundTripClockSurvivesTimeDilation",
	EclipseGauntletTest::TestFlags)

bool FEclipseGauntletWallClockUnderDilationTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	// The whole point of criterion 1: Command Mode dilates the world to 0.30, so
	// a round trip timed on game time would read 3.3x too fast and could hand the
	// owner a "true" he never earned. This test dilates a real world and then
	// measures with the exact seam UEclipseSquadSubsystem::IssueOrder samples.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	if (!TestNotNull(TEXT("Standalone world"), World))
	{
		GameInstance->Shutdown();
		return false;
	}

	if (const UEclipseSquadSubsystem* Squad = World->GetSubsystem<UEclipseSquadSubsystem>())
	{
		TestEqual(TEXT("A fresh run starts with an empty round-trip tally"), Squad->GetOrderRoundTripStats().SampleCount, 0);
	}
	else
	{
		AddInfo(TEXT("No squad subsystem in this world type — clock proof below still runs."));
	}

	// 0.30 = the locked Command Mode rate, 0.05 = an extreme the fallback ladder
	// could reach. The wall clock must be indifferent to both.
	constexpr double MeasuredWindowSeconds = 0.03;
	for (const float Dilation : { 0.30f, 0.05f })
	{
		UGameplayStatics::SetGlobalTimeDilation(World, Dilation);
		TestEqual(TEXT("The world really is dilated while we measure"), UGameplayStatics::GetGlobalTimeDilation(World), Dilation);

		const double Start = NowWallSeconds();
		FPlatformProcess::Sleep(static_cast<float>(MeasuredWindowSeconds));
		const double Elapsed = NowWallSeconds() - Start;

		// Sleep is real time. Read on the dilated clock the same window would be
		// 0.009 s (x0.30) or 0.0015 s (x0.05); two thirds of the real window is a
		// bound no dilated reading can reach and no scheduler jitter can miss.
		TestTrue(TEXT("Round-trip clock advances at wall-clock rate under dilation"),
			Elapsed >= MeasuredWindowSeconds * 0.66);

		FEclipseOrderRoundTripStats Stats;
		Stats.NoteRoundTrip(Elapsed);
		TestEqual(TEXT("The measured wait is one sample"), Stats.SampleCount, 1);
		TestEqual(TEXT("A 30 ms answer is comfortably inside the 1 s bar"), Stats.WithinBarCount, 1);
		TestTrue(TEXT("The worst case is the real wait, not the dilated one"), Stats.WorstSeconds >= MeasuredWindowSeconds * 0.66);
	}

	UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	GameInstance->Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGauntletBeatTallyTest,
	"Eclipse.Gauntlet.EncounterBeatTallyCountsUsagePull",
	EclipseGauntletTest::TestFlags)

bool FEclipseGauntletBeatTallyTest::RunTest(const FString& Parameters)
{
	using namespace EclipseCommandLogic;
	constexpr double IdleGap = 45.0;

	FEclipseEncounterBeatTally Tally;
	TestEqual(TEXT("Fresh tally has no entries"), Tally.EntriesThisBeat, 0);
	TestEqual(TEXT("Fresh tally has no closed beats"), Tally.ClosedBeatCount, 0);
	TestEqual(TEXT("Fresh tally averages 0"), Tally.GetAverageEntriesPerBeat(), 0.0f);

	// Two entries ten seconds apart are the same fight.
	Tally.NoteModeEntered(100.0, IdleGap);
	Tally.NoteModeEntered(110.0, IdleGap);
	TestEqual(TEXT("Both entries land in the running beat"), Tally.EntriesThisBeat, 2);
	TestEqual(TEXT("Nothing closed yet"), Tally.ClosedBeatCount, 0);

	// Ninety seconds of quiet: the next entry belongs to a new beat, and the old
	// one closes with the two entries it had.
	Tally.NoteModeEntered(200.0, IdleGap);
	TestEqual(TEXT("Idle gap closed the previous beat"), Tally.ClosedBeatCount, 1);
	TestEqual(TEXT("Closed beat kept its two entries"), Tally.GetAverageEntriesPerBeat(), 2.0f);
	TestEqual(TEXT("The new beat holds the entry that opened it"), Tally.EntriesThisBeat, 1);

	Tally.BeginNextBeat(/*bCountEmptyBeat*/ false);
	TestEqual(TEXT("Explicit close records the second beat"), Tally.ClosedBeatCount, 2);
	TestEqual(TEXT("Average over two beats"), Tally.GetAverageEntriesPerBeat(), 1.5f);

	// A phase boundary with nothing behind it is not evidence a fight happened.
	Tally.BeginNextBeat(/*bCountEmptyBeat*/ false);
	TestEqual(TEXT("An empty automatic close is a no-op"), Tally.ClosedBeatCount, 2);

	// The tester marking an empty beat IS evidence: he says a fight went by that
	// he never opened the mode in. That is the zero criterion 5 exists to catch.
	Tally.BeginNextBeat(/*bCountEmptyBeat*/ true);
	TestEqual(TEXT("A marked empty beat counts"), Tally.ClosedBeatCount, 3);
	TestEqual(TEXT("Three entries over three beats"), Tally.GetAverageEntriesPerBeat(), 1.0f);

	// Cross-check with the verdict core: exactly one entry per beat still passes,
	// the next unused beat fails it.
	EclipseGauntletOverlay::FEclipseGauntletCriteria Criteria;
	Criteria.ClosedBeatCount = Tally.ClosedBeatCount;
	Criteria.AverageEntriesPerBeat = Tally.GetAverageEntriesPerBeat();
	TestTrue(TEXT("One entry per beat passes criterion 5"),
		EclipseGauntletOverlay::EvaluateCriterion(4, Criteria) == EclipseGauntletOverlay::EEclipseGauntletStatus::Pass);

	Tally.BeginNextBeat(/*bCountEmptyBeat*/ true);
	Criteria.ClosedBeatCount = Tally.ClosedBeatCount;
	Criteria.AverageEntriesPerBeat = Tally.GetAverageEntriesPerBeat();
	TestEqual(TEXT("Four beats, three entries"), Tally.GetAverageEntriesPerBeat(), 0.75f);
	TestTrue(TEXT("An unused beat fails criterion 5 (an unused mode is a failed mode)"),
		EclipseGauntletOverlay::EvaluateCriterion(4, Criteria) == EclipseGauntletOverlay::EEclipseGauntletStatus::Fail);

	// Gap 0 disables the automatic split: only explicit marks segment then.
	FEclipseEncounterBeatTally Manual;
	Manual.NoteModeEntered(0.0, 0.0);
	Manual.NoteModeEntered(10000.0, 0.0);
	TestEqual(TEXT("Disabled idle gap keeps one running beat"), Manual.EntriesThisBeat, 2);
	TestEqual(TEXT("Disabled idle gap closes nothing"), Manual.ClosedBeatCount, 0);

	Manual.Reset();
	TestEqual(TEXT("Reset clears the entries"), Manual.EntriesThisBeat, 0);
	TestEqual(TEXT("Reset clears the beats"), Manual.ClosedBeatCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGauntletVerdictBlockTest,
	"Eclipse.Gauntlet.VerdictBlockMarksFailuresAndCounts",
	EclipseGauntletTest::TestFlags)

bool FEclipseGauntletVerdictBlockTest::RunTest(const FString& Parameters)
{
	using namespace EclipseGauntletOverlay;

	// Nothing played yet: five open criteria, zero failures, and a block that says
	// so — an unmeasured gauntlet may never read as a verdict.
	{
		const FEclipseGauntletVerdict Verdict = ComposeVerdict(FEclipseGauntletCriteria());
		TestEqual(TEXT("Block has title + five criteria + tally"), Verdict.Lines.Num(), VerdictLineCount);
		TestEqual(TEXT("One status per criterion"), Verdict.Statuses.Num(), CriterionCount);
		TestEqual(TEXT("Nothing failed"), Verdict.FailedCount, 0);
		TestEqual(TEXT("Everything is open"), Verdict.OpenCount, CriterionCount);
		TestTrue(TEXT("Tally counts zero failures"), Verdict.Lines.Last().Contains(TEXT("0 criteria gefaald")));
		TestTrue(TEXT("Tally admits the missing measurements"), Verdict.Lines.Last().Contains(TEXT("5 nog niet gemeten")));
		for (const EEclipseGauntletStatus Status : Verdict.Statuses)
		{
			TestTrue(TEXT("An unmeasured criterion never passes"), Status == EEclipseGauntletStatus::Open);
		}
	}

	// A clean gauntlet: five passes, no marks, tally reads zero.
	{
		const FEclipseGauntletVerdict Verdict = ComposeVerdict(EclipseGauntletTest::MakePassingCriteria());
		TestEqual(TEXT("Clean run fails nothing"), Verdict.FailedCount, 0);
		TestEqual(TEXT("Clean run leaves nothing open"), Verdict.OpenCount, 0);
		TestEqual(TEXT("Clean tally"), Verdict.Lines.Last(), FString(TEXT("-> 0 criteria gefaald")));
		for (int32 Line = 1; Line <= CriterionCount; ++Line)
		{
			TestFalse(TEXT("A passing row carries no mark"), Verdict.Lines[Line].Contains(TEXT("<<")));
		}
		TestTrue(TEXT("The round-trip row reports the worst case"), Verdict.Lines[1].Contains(TEXT("0.004")));
	}

	// One late answer out of ten: criterion 1 falls, and the tally uses the Dutch
	// singular the owner's format asks for.
	{
		FEclipseGauntletCriteria Criteria = EclipseGauntletTest::MakePassingCriteria();
		Criteria.RoundTripWithinBar = 9;
		Criteria.RoundTripWorstSeconds = 1.4;
		const FEclipseGauntletVerdict Verdict = ComposeVerdict(Criteria);
		TestTrue(TEXT("Criterion 1 failed"), Verdict.Statuses[0] == EEclipseGauntletStatus::Fail);
		TestTrue(TEXT("The failed row is marked"), Verdict.Lines[1].Contains(TEXT("<< GEFAALD")));
		TestEqual(TEXT("Singular tally"), Verdict.Lines.Last(), FString(TEXT("-> 1 criterium gefaald")));
	}

	// Nine answers all inside the bar is not yet a pass: the criterion asks for ten.
	{
		FEclipseGauntletCriteria Criteria = EclipseGauntletTest::MakePassingCriteria();
		Criteria.RoundTripSamples = 9;
		Criteria.RoundTripWithinBar = 9;
		TestTrue(TEXT("A short run stays open"), EvaluateCriterion(0, Criteria) == EEclipseGauntletStatus::Open);
	}

	// Targeting tolerates exactly one mis-pick (>= 9/10); the second settles the
	// verdict before the tenth attempt is taken.
	{
		FEclipseGauntletCriteria Criteria;
		Criteria.CleanPicks = 3;
		Criteria.MisPicks = 1;
		TestTrue(TEXT("One mis-pick early is still open"), EvaluateCriterion(1, Criteria) == EEclipseGauntletStatus::Open);
		Criteria.MisPicks = 2;
		TestTrue(TEXT("Two mis-picks fail criterion 2 immediately"), EvaluateCriterion(1, Criteria) == EEclipseGauntletStatus::Fail);

		FEclipseGauntletCriteria Full;
		Full.CleanPicks = 9;
		Full.MisPicks = 1;
		TestTrue(TEXT("9/10 clean passes"), EvaluateCriterion(1, Full) == EEclipseGauntletStatus::Pass);
	}

	// The tester's own answers: bad is a failure, unanswered never is.
	{
		FEclipseGauntletCriteria Criteria = EclipseGauntletTest::MakePassingCriteria();
		Criteria.Comfort = EEclipseGauntletAnswer::Bad;
		Criteria.Confidence = EEclipseGauntletAnswer::Bad;
		Criteria.AverageEntriesPerBeat = 0.5f;
		const FEclipseGauntletVerdict Verdict = ComposeVerdict(Criteria);
		TestEqual(TEXT("Three criteria failed"), Verdict.FailedCount, 3);
		TestEqual(TEXT("Plural tally"), Verdict.Lines.Last(), FString(TEXT("-> 3 criteria gefaald")));
		TestTrue(TEXT("Comfort row marked"), Verdict.Lines[3].Contains(TEXT("<< GEFAALD")));
		TestTrue(TEXT("Trust row marked"), Verdict.Lines[4].Contains(TEXT("<< GEFAALD")));
		TestTrue(TEXT("Usage-pull row marked"), Verdict.Lines[5].Contains(TEXT("<< GEFAALD")));
	}

	// Out-of-range indices answer Open rather than inventing a verdict.
	TestTrue(TEXT("Unknown criterion index is open"), EvaluateCriterion(9, FEclipseGauntletCriteria()) == EEclipseGauntletStatus::Open);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGauntletPanelContentTest,
	"Eclipse.Gauntlet.ControlTableAndPlaytestRowsAreComplete",
	EclipseGauntletTest::TestFlags)

bool FEclipseGauntletPanelContentTest::RunTest(const FString& Parameters)
{
	using namespace EclipseGauntletOverlay;

	// The control overview is the owner's answer to "what does this key do"; a
	// blank cell is worse than no table (the row that lies is the row nobody
	// checks against the bindings).
	const TArray<FEclipseControlRow> Rows = GetControlRows();
	TestEqual(TEXT("Eleven documented actions"), Rows.Num(), 11);
	for (const FEclipseControlRow& Row : Rows)
	{
		TestTrue(TEXT("Row names its action"), Row.Action != nullptr && FCString::Strlen(Row.Action) > 0);
		TestTrue(TEXT("Row has a mouse+keyboard cell"), Row.MouseKeyboard != nullptr && FCString::Strlen(Row.MouseKeyboard) > 0);
		TestTrue(TEXT("Row has a controller cell"), Row.Controller != nullptr && FCString::Strlen(Row.Controller) > 0);
	}

	// Spot-check the two rows that changed most recently in the real bindings
	// (SPEC-P2-02 moved the hold to Q/LB and stance to Alt/Y).
	const FEclipseControlRow* CommandRow = Rows.FindByPredicate(
		[](const FEclipseControlRow& Row) { return FCString::Strcmp(Row.Action, TEXT("Command Mode")) == 0; });
	TestNotNull(TEXT("Command Mode row present"), CommandRow);
	if (CommandRow != nullptr)
	{
		TestEqual(TEXT("Command Mode is hold Q"), FString(CommandRow->MouseKeyboard), FString(TEXT("Q vasthouden")));
		TestEqual(TEXT("Command Mode is hold LB"), FString(CommandRow->Controller), FString(TEXT("LB vasthouden")));
	}

	// The 13.2 checklist: five statements, gate question last, one key per row.
	const TArray<FString> Questions = GetPlaytestQuestions();
	TestEqual(TEXT("Five playtest statements"), Questions.Num(), PlaytestQuestionCount);
	TestTrue(TEXT("The gate question is the last row"), Questions.Last().Contains(TEXT("GATE")));

	TArray<EEclipseGauntletAnswer> Answers;
	Answers.Init(EEclipseGauntletAnswer::Unanswered, PlaytestQuestionCount);
	Answers[0] = EEclipseGauntletAnswer::Good;
	Answers[1] = EEclipseGauntletAnswer::Bad;
	Answers[PlaytestQuestionCount - 1] = EEclipseGauntletAnswer::Good;

	const TArray<FString> Block = ComposePlaytestBlock(Answers);
	TestEqual(TEXT("Header + five rows + gate line"), Block.Num(), PlaytestQuestionCount + 2);
	TestTrue(TEXT("Answered good is visible"), Block[1].Contains(TEXT("goed")));
	TestTrue(TEXT("Answered bad is visible"), Block[2].Contains(TEXT("slecht")));
	TestEqual(TEXT("Gate line reads JA"), Block.Last(), FString(TEXT("-> gate: JA")));

	// A short answer array is a half-filled checklist, not a crash (14.3.5 spirit).
	const TArray<FString> Partial = ComposePlaytestBlock(TArray<EEclipseGauntletAnswer>());
	TestEqual(TEXT("Empty answers still compose a block"), Partial.Num(), PlaytestQuestionCount + 2);
	TestEqual(TEXT("Unanswered gate says so"), Partial.Last(), FString(TEXT("-> gate: niet beantwoord")));

	// One key per row cycles unanswered -> good -> bad -> unanswered.
	EEclipseGauntletAnswer Answer = EEclipseGauntletAnswer::Unanswered;
	Answer = CycleAnswer(Answer);
	TestTrue(TEXT("First press answers good"), Answer == EEclipseGauntletAnswer::Good);
	Answer = CycleAnswer(Answer);
	TestTrue(TEXT("Second press answers bad"), Answer == EEclipseGauntletAnswer::Bad);
	Answer = CycleAnswer(Answer);
	TestTrue(TEXT("Third press clears the row"), Answer == EEclipseGauntletAnswer::Unanswered);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
