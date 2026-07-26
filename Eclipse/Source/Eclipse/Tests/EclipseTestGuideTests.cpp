// In-game test guide, headless tier (phase0/INGAME_TESTGIDS.md; GDD 14.5 step 4).
// The guide is pixels plus one detection hook, but everything it *claims* is pure:
// the stappenlijst itself (does every step name both devices and say what you must
// see?), the tick-off logic (does a signal settle only the step that asked for it?)
// and the summary the owner reads back out of Saved/Logs. Variant A is the whole
// point of these tests: no step may ever be unreachable, so every path out of a
// step — detected, confirmed by hand, skipped, answered negatively — is swept here.

#if WITH_DEV_AUTOMATION_TESTS

#include "Containers/Set.h"
#include "Misc/AutomationTest.h"
#include "UI/EclipseGauntletOverlayLogic.h"
#include "UI/EclipseTestGuideLogic.h"

namespace EclipseTestGuideTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** Settle the first N steps positively, so a test can stand on any step it wants to inspect. */
	void ConfirmSteps(EclipseTestGuide::FEclipseGuideProgress& Progress, int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Progress.ConfirmActive();
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGuideStepListTest,
	"Eclipse.Guide.StepListNamesBothDevicesAndAnExpectation",
	EclipseTestGuideTest::TestFlags)

bool FEclipseGuideStepListTest::RunTest(const FString& Parameters)
{
	using namespace EclipseTestGuide;

	const TArray<FEclipseGuideStep> Steps = GetGuideSteps();
	// Geen aantallen in de tekst van deze assert: de lijst groeide vannacht van elf
	// naar veertien controls en de omschrijving liep er twee keer op achter — ook
	// in het kliklijstje van de owner, dat "20 stappen" beloofde terwijl het er 23
	// waren. De constanten zijn de bron; de assert eronder pint het totaal.
	TestEqual(TEXT("Every step in the list is accounted for"), Steps.Num(), GuideStepCount);
	// Het totaal is de som van de drie deel-constanten; verandert er een, dan hoort
	// dit getal mee te veranderen en niet stilletjes te blijven staan.
	TestEqual(TEXT("The list is twenty-three steps long"), GuideStepCount, 23);

	// The guide's promise is not "press this key" but "press this key AND here is
	// what proves it worked". A step missing either device column or its
	// expectation is a step the tester cannot judge, which is worse than absent.
	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		const FEclipseGuideStep& Step = Steps[Index];
		TestFalse(FString::Printf(TEXT("Step %d names itself"), Index + 1), Step.Label.IsEmpty());
		TestFalse(FString::Printf(TEXT("Step %d has a mouse+keyboard cell"), Index + 1), Step.MouseKeyboard.IsEmpty());
		TestFalse(FString::Printf(TEXT("Step %d has a controller cell"), Index + 1), Step.Controller.IsEmpty());
		TestFalse(FString::Printf(TEXT("Step %d says what you must see"), Index + 1), Step.Expectation.IsEmpty());
	}

	// Deel 1 rides on the live control table, index by index — one table, two
	// readers, so a moved binding can never leave the guide teaching an old key.
	const TArray<EclipseGauntletOverlay::FEclipseControlRow> Rows = EclipseGauntletOverlay::GetControlRows();
	TestEqual(TEXT("The control table still has exactly the eleven rows deel 1 expects"), Rows.Num(), ControlStepCount);
	for (int32 Index = 0; Index < ControlStepCount && Index < Rows.Num(); ++Index)
	{
		TestEqual(TEXT("Control step mirrors the control table's action"), Steps[Index].Label, FString(Rows[Index].Action));
		TestEqual(TEXT("Control step mirrors the mouse+keyboard cell"), Steps[Index].MouseKeyboard, FString(Rows[Index].MouseKeyboard));
		TestEqual(TEXT("Control step mirrors the controller cell"), Steps[Index].Controller, FString(Rows[Index].Controller));
		TestTrue(TEXT("Control step belongs to deel 1"), Steps[Index].Part == EEclipseGuidePart::Controls);
	}

	// Every control is detectable, and no two controls share a signal — a shared
	// signal would tick off a step the tester never performed.
	TSet<EEclipseGuideSignal> Signals;
	for (int32 Index = 0; Index < ControlStepCount; ++Index)
	{
		TestTrue(TEXT("A control step carries a detection signal"), Steps[Index].Signal != EEclipseGuideSignal::None);
		TestTrue(TEXT("GetStepSignal agrees with the step list"), GetStepSignal(Index) == Steps[Index].Signal);
		Signals.Add(Steps[Index].Signal);
	}
	TestEqual(TEXT("Each control has its own signal"), Signals.Num(), ControlStepCount);

	// Deel 2 and 3 are judgements: nothing in the game can detect them, and
	// pretending otherwise would tick them off behind the tester's back.
	for (int32 Index = ControlStepCount; Index < Steps.Num(); ++Index)
	{
		TestTrue(TEXT("A judgement step has nothing to detect"), Steps[Index].Signal == EEclipseGuideSignal::None);
		TestTrue(TEXT("GetStepSignal answers None outside deel 1"), GetStepSignal(Index) == EEclipseGuideSignal::None);
	}
	TestTrue(TEXT("Out-of-range signal lookup is None, not a read past the table"), GetStepSignal(-1) == EEclipseGuideSignal::None);
	TestTrue(TEXT("Past-the-end signal lookup is None"), GetStepSignal(GuideStepCount + 5) == EEclipseGuideSignal::None);

	// Deel 3 IS the 13.2 checklist — same statements, gate question last — so the
	// guide cannot open a second set of books on the owner's gate.
	const TArray<FString> Questions = EclipseGauntletOverlay::GetPlaytestQuestions();
	TestEqual(TEXT("Deel 3 is as long as the 13.2 checklist"), QuestionStepCount, Questions.Num());
	for (int32 Index = 0; Index < QuestionStepCount; ++Index)
	{
		const int32 StepIndex = ControlStepCount + SystemStepCount + Index;
		TestEqual(TEXT("Deel 3 quotes the 13.2 statement verbatim"), Steps[StepIndex].Label, Questions[Index]);
		TestEqual(TEXT("Deel 3 maps back onto its playtest row"), GetPlaytestQuestionIndex(StepIndex), Index);
	}
	TestTrue(TEXT("The gate question is the last step of the guide"), Steps.Last().Label.Contains(TEXT("GATE")));
	TestEqual(TEXT("A control step feeds no playtest row"), GetPlaytestQuestionIndex(0), (int32)INDEX_NONE);
	TestEqual(TEXT("A system step feeds no playtest row"), GetPlaytestQuestionIndex(ControlStepCount), (int32)INDEX_NONE);

	// Part bookkeeping: the panel counts "stap 3/11" off these three helpers.
	TestTrue(TEXT("Step 1 is deel 1"), GetPartOfStep(0) == EEclipseGuidePart::Controls);
	TestTrue(TEXT("Step 12 is deel 2"), GetPartOfStep(ControlStepCount) == EEclipseGuidePart::Systems);
	TestTrue(TEXT("Step 16 is deel 3"), GetPartOfStep(ControlStepCount + SystemStepCount) == EEclipseGuidePart::Questions);
	TestEqual(TEXT("The last control is last of its part"), GetIndexWithinPart(ControlStepCount - 1), ControlStepCount - 1);
	TestEqual(TEXT("The first system step restarts the count"), GetIndexWithinPart(ControlStepCount), 0);
	TestEqual(TEXT("The first question restarts the count"), GetIndexWithinPart(ControlStepCount + SystemStepCount), 0);
	TestEqual(TEXT("Deel 1 counts eleven"), GetPartStepCount(EEclipseGuidePart::Controls), ControlStepCount);
	TestEqual(TEXT("Deel 2 counts four"), GetPartStepCount(EEclipseGuidePart::Systems), SystemStepCount);
	TestEqual(TEXT("Deel 3 counts five"), GetPartStepCount(EEclipseGuidePart::Questions), QuestionStepCount);

	// The debrief step may only quote numbers phase0/TESTROUTE_OBJECTIVES.md
	// states (and the M1.1 gauntlet asserts): the guide never invents a value.
	const FEclipseGuideStep& Debrief = Steps[ControlStepCount + SystemStepCount - 1];
	TestTrue(TEXT("Debrief step quotes the documented base reward"), Debrief.Expectation.Contains(TEXT("+50 credits")));
	// Deze assert eiste tot 26-07 dat de stap de +20-bonus BELOOFT. Gemeten betaalt
	// die bonus niet uit: hij hangt aan een optioneel objective dat voltooid moet
	// worden, en niets voltooit het (er is geen vak en geen doelwit — het is een
	// voorwaarde, geen taak). De speelronde bevestigt het: nul gewonden, 25
	// materiaal in plaats van 45.
	//
	// De assert bewaakte dus een belofte die het spel niet kan waarmaken. Nu eist
	// hij het omgekeerde: de stap moet zeggen dat de bonus NIET komt, zodat de
	// tester niet gaat zoeken naar een fout die er niet is. Zodra de bonus wél
	// uitbetaalt, valt deze regel om en hoort de tekst mee te veranderen.
	TestTrue(TEXT("Debrief step is eerlijk over de niet-uitbetaalde stretch-bonus"),
		Debrief.Expectation.Contains(TEXT("+20 stretch-bonus")) && Debrief.Expectation.Contains(TEXT("komt NIET")));
	TestTrue(TEXT("Debrief step states the region does not flip"), Debrief.Expectation.Contains(TEXT("NIET van eigenaar")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGuideProgressTest,
	"Eclipse.Guide.ProgressTicksOffOnlyTheStepItAskedFor",
	EclipseTestGuideTest::TestFlags)

bool FEclipseGuideProgressTest::RunTest(const FString& Parameters)
{
	using namespace EclipseTestGuide;

	FEclipseGuideProgress Progress;
	TestEqual(TEXT("A fresh guide stands on step 1"), Progress.GetActiveIndex(), 0);
	TestFalse(TEXT("A fresh guide is not complete"), Progress.IsComplete());
	TestFalse(TEXT("A fresh guide has nothing to archive"), Progress.HasAnyProgress());
	TestEqual(TEXT("Everything starts pending"), Progress.CountInState(EEclipseGuideStepState::Pending), GuideStepCount);

	// Step 1 asks for movement. Firing, looking or anything else must not settle
	// it: the guide asks one thing at a time, and a step nobody looked at may not
	// evaporate because the player happened to touch another key.
	TestFalse(TEXT("The wrong action does not tick the active step"), Progress.NoteSignal(EEclipseGuideSignal::Fire));
	TestFalse(TEXT("A later step is not settled from a distance"), Progress.NoteSignal(EEclipseGuideSignal::Stance));
	TestFalse(TEXT("A signal-less report settles nothing"), Progress.NoteSignal(EEclipseGuideSignal::None));
	TestEqual(TEXT("Still on step 1"), Progress.GetActiveIndex(), 0);

	TestTrue(TEXT("The asked-for action settles the step"), Progress.NoteSignal(EEclipseGuideSignal::Move));
	TestTrue(TEXT("A detected step reads as detected"), Progress.GetStates()[0] == EEclipseGuideStepState::Detected);
	TestEqual(TEXT("The guide moved to step 2"), Progress.GetActiveIndex(), 1);
	TestTrue(TEXT("There is now something to archive"), Progress.HasAnyProgress());

	// Repeating the previous action does nothing: step 2 asks for looking.
	TestFalse(TEXT("Re-doing the settled step does not skip ahead"), Progress.NoteSignal(EEclipseGuideSignal::Move));
	TestEqual(TEXT("Still on step 2"), Progress.GetActiveIndex(), 1);

	// Variant A's escape hatch: when detection fails you confirm by hand, and the
	// difference is preserved — "the tester had to confirm it" is a finding.
	TestTrue(TEXT("J settles the active step"), Progress.ConfirmActive());
	TestTrue(TEXT("A hand-settled step is distinguishable from a detected one"),
		Progress.GetStates()[1] == EEclipseGuideStepState::Confirmed);

	// N on a control step is "sla over": you can always move on, which is the
	// entire reason variant A was chosen over locking the input.
	TestTrue(TEXT("N moves past a control step"), Progress.RejectActive());
	TestTrue(TEXT("A control step passed over reads as skipped"), Progress.GetStates()[2] == EEclipseGuideStepState::Skipped);
	TestEqual(TEXT("Three steps settled"), Progress.GetActiveIndex(), 3);

	// N on a judgement step is a real answer, not a shrug.
	EclipseTestGuideTest::ConfirmSteps(Progress, ControlStepCount - 3);
	TestEqual(TEXT("Deel 2 starts after the eleven controls"), Progress.GetActiveIndex(), ControlStepCount);
	TestTrue(TEXT("N answers a system step"), Progress.RejectActive());
	TestTrue(TEXT("A system judged badly reads as rejected, not skipped"),
		Progress.GetStates()[ControlStepCount] == EEclipseGuideStepState::Rejected);

	// Detection is dead outside deel 1: nothing in the game can observe an opinion.
	TestFalse(TEXT("No signal can settle a judgement step"), Progress.NoteSignal(EEclipseGuideSignal::Move));
	TestFalse(TEXT("Not even a matching-looking one"), Progress.NoteSignal(EEclipseGuideSignal::Order));

	// Walk the rest out and check the end state is a real end state.
	EclipseTestGuideTest::ConfirmSteps(Progress, GuideStepCount);
	TestTrue(TEXT("The guide finishes"), Progress.IsComplete());
	TestEqual(TEXT("A finished guide has no active step"), Progress.GetActiveIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("Nothing is left pending"), Progress.CountInState(EEclipseGuideStepState::Pending), 0);
	TestFalse(TEXT("A finished guide settles nothing further"), Progress.ConfirmActive());
	TestFalse(TEXT("A finished guide rejects nothing further"), Progress.RejectActive());
	TestFalse(TEXT("A finished guide ignores late input"), Progress.NoteSignal(EEclipseGuideSignal::Move));

	// The tally the panel and the archive both print.
	TestEqual(TEXT("One step was detected"), Progress.CountInState(EEclipseGuideStepState::Detected), 1);
	TestEqual(TEXT("One step was skipped"), Progress.CountInState(EEclipseGuideStepState::Skipped), 1);
	TestEqual(TEXT("One step was rejected"), Progress.CountInState(EEclipseGuideStepState::Rejected), 1);
	TestEqual(TEXT("The rest were confirmed by hand"),
		Progress.CountInState(EEclipseGuideStepState::Confirmed), GuideStepCount - 3);

	Progress.Reset();
	TestEqual(TEXT("Reset returns to step 1"), Progress.GetActiveIndex(), 0);
	TestFalse(TEXT("Reset clears the archive-worthy state"), Progress.HasAnyProgress());
	TestEqual(TEXT("Reset clears every step"), Progress.CountInState(EEclipseGuideStepState::Pending), GuideStepCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGuidePanelTest,
	"Eclipse.Guide.PanelShowsTheActiveExpectationAndCollapsesTheRest",
	EclipseTestGuideTest::TestFlags)

bool FEclipseGuidePanelTest::RunTest(const FString& Parameters)
{
	using namespace EclipseTestGuide;

	const TArray<FEclipseGuideStep> Steps = GetGuideSteps();
	FEclipseGuideProgress Progress;

	{
		const TArray<FString> Lines = ComposeGuidePanelLines(Progress);
		// Fixed height: the widget pre-builds these rows once and only SetTexts
		// them afterwards, so a drifting line count would write into thin air.
		TestEqual(TEXT("Header + twenty steps + tally"), Lines.Num(), GuidePanelLineCount);

		// BREEDTE, en niet alleen hoogte. Het paneel rendert de actieve stap op één
		// regel; een lange verwachting loopt dus van het scherm af in plaats van te
		// wrappen. Dat is 26-07 gebeurd: ik breidde vier stapteksten uit met wat ik
		// gemeten had en de langste kwam op 435 tekens. Onzichtbaar in elke test,
		// want de hoogte klopte nog.
		//
		// De grens is gemeten en niet verzonnen: de langste samengestelde regel is
		// na het inkorten ~200 tekens, en 260 laat ruimte voor een langere stapnaam
		// zonder dat er iets buiten beeld valt. Wie meer wil vertellen, zet het in
		// BESTURING.md — de gids is een LIVE hulpmiddel, geen naslagwerk.
		constexpr int32 MaxPanelLineChars = 260;
		for (const FString& Line : Lines)
		{
			TestTrue(*FString::Printf(TEXT("Paneelregel past op het scherm (%d tekens, max %d): '%s'"),
					Line.Len(), MaxPanelLineChars, *Line.Left(60)),
				Line.Len() <= MaxPanelLineChars);
		}

		TestTrue(TEXT("The header counts within its part"), Lines[0].Contains(TEXT("stap 1/14")));
		TestTrue(TEXT("The header names the part"), Lines[0].Contains(TEXT("deel 1: controls")));
		TestTrue(TEXT("The header names the hide key"), Lines[0].Contains(TEXT("[F3]")));
		TestTrue(TEXT("In deel 1, N is a skip"), Lines[0].Contains(TEXT("[N] sla over")));
		TestTrue(TEXT("In deel 1, J is a pass"), Lines[0].Contains(TEXT("[J] gehaald")));

		// The active step is the one that carries its expectation — knowing the key
		// without knowing what proves it worked is half an instruction (spec §2).
		TestTrue(TEXT("The active step is marked"), Lines[2].StartsWith(TEXT(">>")));
		TestTrue(TEXT("The active step names the keyboard control"), Lines[2].Contains(Steps[0].MouseKeyboard));
		TestTrue(TEXT("The active step names the controller control"), Lines[2].Contains(Steps[0].Controller));
		TestTrue(TEXT("The active step states its expectation"), Lines[2].Contains(Steps[0].Expectation));

		// Steps still to come show their keys but not their expectations, so the
		// panel stays readable without scrolling.
		TestTrue(TEXT("A future step still shows both device cells"),
			Lines[3].Contains(Steps[1].MouseKeyboard) && Lines[3].Contains(Steps[1].Controller));
		TestFalse(TEXT("A future step does not shout its expectation"), Lines[3].Contains(Steps[1].Expectation));
		TestFalse(TEXT("A future step is not marked active"), Lines[3].StartsWith(TEXT(">>")));

		TestTrue(TEXT("The tally line closes the panel"), Lines.Last().Contains(TEXT("nog open 23")));
	}

	// A settled step collapses to marker + label + how it was settled.
	Progress.NoteSignal(EEclipseGuideSignal::Move);
	{
		const TArray<FString> Lines = ComposeGuidePanelLines(Progress);
		TestTrue(TEXT("A settled step is ticked"), Lines[2].StartsWith(TEXT("[v]")));
		TestTrue(TEXT("A settled step says it was detected"), Lines[2].Contains(TEXT("gedetecteerd")));
		TestFalse(TEXT("A settled step folds its expectation away"), Lines[2].Contains(Steps[0].Expectation));
		TestTrue(TEXT("The next step became active"), Lines[3].StartsWith(TEXT(">>")));
		TestTrue(TEXT("The header follows along"), Lines[0].Contains(TEXT("stap 2/14")));
		TestTrue(TEXT("The tally counts the detection"), Lines.Last().Contains(TEXT("gedetecteerd 1")));
	}

	// Deel 2's responsiveness row is the one place a live measurement belongs, and
	// "not measured" must read as not measured — never as a silent zero.
	// Step 1 was detected above, so eleven more settlements land on the twelfth
	// step: deel 2's "Order-reactie voelt direct".
	EclipseTestGuideTest::ConfirmSteps(Progress, ControlStepCount);
	{
		// header + the look-values line + every control + the first system step
		const int32 ResponsivenessLine = ControlStepCount + 3;
		const TArray<FString> Blank = ComposeGuidePanelLines(Progress);
		TestTrue(TEXT("The responsiveness row is the active one"), Blank[ResponsivenessLine].StartsWith(TEXT(">>")));
		TestTrue(TEXT("An unmeasured meter says so"), Blank[ResponsivenessLine].Contains(TEXT("nog geen orders gemeten")));

		const TArray<FString> Measured = ComposeGuidePanelLines(Progress, TEXT("8/10 binnen 1.00 s"));
		TestTrue(TEXT("A measured meter is quoted next to the judgement"),
			Measured[ResponsivenessLine].Contains(TEXT("meter: 8/10 binnen 1.00 s")));
		TestTrue(TEXT("Deel 2's header offers a verdict, not a skip"), Measured[0].Contains(TEXT("[N] niet goed")));
		TestTrue(TEXT("Deel 2 counts its own four steps"), Measured[0].Contains(TEXT("stap 2/4")));
	}

	// Deel 3 asks yes/no, and says so.
	EclipseTestGuideTest::ConfirmSteps(Progress, SystemStepCount - 1);
	{
		const TArray<FString> Lines = ComposeGuidePanelLines(Progress);
		TestTrue(TEXT("Deel 3 announces itself"), Lines[0].Contains(TEXT("deel 3: 13.2-vragen")));
		TestTrue(TEXT("Deel 3 answers with ja"), Lines[0].Contains(TEXT("[J] ja")));
		TestTrue(TEXT("Deel 3 answers with nee"), Lines[0].Contains(TEXT("[N] nee")));
		TestTrue(TEXT("Deel 3 counts its own five steps"), Lines[0].Contains(TEXT("stap 1/5")));
	}

	// Finished: no active row anywhere, and the header says where the record went.
	EclipseTestGuideTest::ConfirmSteps(Progress, QuestionStepCount);
	{
		const TArray<FString> Lines = ComposeGuidePanelLines(Progress);
		TestEqual(TEXT("A finished panel is still the same height"), Lines.Num(), GuidePanelLineCount);
		TestTrue(TEXT("The header reports completion"), Lines[0].Contains(TEXT("klaar")));
		TestTrue(TEXT("The header points at the archive"), Lines[0].Contains(TEXT("Saved/Logs")));
		for (int32 Line = 1; Line <= GuideStepCount; ++Line)
		{
			TestFalse(TEXT("No step is active once the guide is done"), Lines[Line].StartsWith(TEXT(">>")));
		}
		TestTrue(TEXT("Nothing is left open"), Lines.Last().Contains(TEXT("nog open 0")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGuideSummaryTest,
	"Eclipse.Guide.SummaryRecordsHowEveryStepWasSettled",
	EclipseTestGuideTest::TestFlags)

bool FEclipseGuideSummaryTest::RunTest(const FString& Parameters)
{
	using namespace EclipseTestGuide;

	// An untouched guide still composes a complete, honest block: twenty open
	// steps. A half-walked guide may never read back as a verdict (14.5).
	{
		const FEclipseGuideProgress Fresh;
		const TArray<FString> Block = ComposeGuideSummaryBlock(Fresh);
		TestEqual(TEXT("Title + three part headers + twenty steps + tally"), Block.Num(), GuideStepCount + 5);
		TestTrue(TEXT("The block names the variant it was built as"), Block[0].Contains(TEXT("variant A")));
		TestTrue(TEXT("Everything is open"), Block.Last().Contains(TEXT("23 nog open")));
		TestTrue(TEXT("An untouched step claims nothing"), Block[2].Contains(TEXT("nog open")));
	}

	// One of every outcome, so the archive can be read back without guessing.
	FEclipseGuideProgress Progress;
	Progress.NoteSignal(EEclipseGuideSignal::Move);   // step 1  — detected
	Progress.ConfirmActive();                          // step 2  — confirmed by hand
	Progress.RejectActive();                           // step 3  — skipped
	EclipseTestGuideTest::ConfirmSteps(Progress, ControlStepCount - 3);
	Progress.RejectActive();                           // step 12 — system judged badly
	EclipseTestGuideTest::ConfirmSteps(Progress, SystemStepCount - 1);
	Progress.RejectActive();                           // step 16 — gate-style "nee"

	const TArray<FString> Block = ComposeGuideSummaryBlock(Progress);
	TestEqual(TEXT("The block keeps its fixed shape"), Block.Num(), GuideStepCount + 5);

	// Layout: title, [deel 1], 11 controls, [deel 2], 4 systems, [deel 3], 5 questions, tally.
	TestTrue(TEXT("Deel 1 is labelled"), Block[1].Contains(TEXT("deel 1: controls")));
	TestTrue(TEXT("Deel 2 is labelled"), Block[ControlStepCount + 2].Contains(TEXT("deel 2: systemen")));
	TestTrue(TEXT("Deel 3 is labelled"), Block[ControlStepCount + SystemStepCount + 3].Contains(TEXT("deel 3: 13.2-vragen")));

	// The distinction that matters most: a control the game DETECTED versus one the
	// tester had to confirm by hand. The second is a detection bug in the making,
	// and flattening both to "done" would hide exactly what variant A exists to find.
	TestTrue(TEXT("A detected control is archived as detected"), Block[2].Contains(TEXT("gedetecteerd")));
	TestTrue(TEXT("A hand-confirmed control is archived as such"), Block[3].Contains(TEXT("handmatig bevestigd")));
	TestTrue(TEXT("A skipped control is archived as skipped"), Block[4].Contains(TEXT("overgeslagen")));
	TestTrue(TEXT("A badly judged system is archived as niet goed"), Block[ControlStepCount + 3].Contains(TEXT("niet goed")));
	TestTrue(TEXT("A positively judged system is archived as goed"), Block[ControlStepCount + 4].Contains(TEXT("goed")));
	TestTrue(TEXT("A question answered no is archived as nee"), Block[ControlStepCount + SystemStepCount + 4].Contains(TEXT("nee")));
	TestTrue(TEXT("An unanswered question stays open"), Block.Last(1).Contains(TEXT("nog open")));

	// Every step is named in the archive, not just the settled ones.
	const TArray<FEclipseGuideStep> Steps = GetGuideSteps();
	const FString Joined = FString::Join(Block, TEXT("\n"));
	for (const FEclipseGuideStep& Step : Steps)
	{
		TestTrue(FString::Printf(TEXT("The archive names '%s'"), *Step.Label), Joined.Contains(Step.Label));
	}

	TestTrue(TEXT("The tally counts the detection"), Block.Last().Contains(TEXT("1 gedetecteerd")));
	TestTrue(TEXT("The tally counts the skip"), Block.Last().Contains(TEXT("1 overgeslagen")));
	TestTrue(TEXT("The tally counts the two negative answers"), Block.Last().Contains(TEXT("2 niet goed")));
	TestTrue(TEXT("The tally admits what was never reached"), Block.Last().Contains(TEXT("4 nog open")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
