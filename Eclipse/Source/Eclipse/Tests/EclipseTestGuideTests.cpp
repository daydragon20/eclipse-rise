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
	TestEqual(TEXT("Every step in the list is accounted for"), Steps.Num(), GetGuideStepCount());

	// GEEN VAST TOTAAL MEER. Hier stond "de lijst is 23 stappen lang". Dat getal is
	// sinds 26-07 avond geen constante: deel 1 toont wat er VERANDERD is sinds je
	// vorige sessie, en dat zijn er soms nul. Wat wél vaststaat is de ondergrens —
	// één regel over wijzigingen (ook als die zegt dat er niets is), plus de twee
	// oordelen en de vijf 13.2-vragen.
	TestTrue(FString::Printf(TEXT("De gids is kort gebleven (%d stappen)"), Steps.Num()),
		Steps.Num() >= 1 + SystemStepCount + QuestionStepCount && Steps.Num() <= 10);

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

	// NIETS IS NOG DETECTEERBAAR, en dat is de kern van de herbouw. Deel 1 vinkte
	// veertien controls af via Enhanced Input; die stappen waren allemaal meetbaar
	// en horen daarom niet in een gids die alleen mag bevatten wat ik NIET kan
	// meten. Een stap die zichzelf afvinkt terwijl de owner hem niet beoordeelde,
	// is precies de vorm van "te veel naar hem doorschuiven" die de regel verbood.
	// De detectie is helemaal weg: er valt niets meer af te vinken zonder dat de
	// owner er zelf J of N op geeft. Dat is precies wat "alleen wat ik niet kan
	// meten" oplevert — een oordeel geeft geen invoergebeurtenis af.

	// Deel 1 zegt ALTIJD iets, ook als er niets veranderd is — "er is niets voor
	// jou" hoort er te staan, want een lege lijst leest als een fout.
	TestTrue(TEXT("Deel 1 heeft minstens één regel"), GetPartStepCount(EEclipseGuidePart::Controls) >= 1);
	TestTrue(TEXT("De eerste stap hoort bij deel 1"), Steps[0].Part == EEclipseGuidePart::Controls);

	// Deel 3 IS the 13.2 checklist — same statements, gate question last — so the
	// guide cannot open a second set of books on the owner's gate.
	const TArray<FString> Questions = EclipseGauntletOverlay::GetPlaytestQuestions();
	TestEqual(TEXT("Deel 3 is as long as the 13.2 checklist"), QuestionStepCount, Questions.Num());
	const int32 FirstQuestion = Steps.Num() - QuestionStepCount;
	for (int32 Index = 0; Index < QuestionStepCount; ++Index)
	{
		const int32 StepIndex = FirstQuestion + Index;
		TestEqual(TEXT("Deel 3 quotes the 13.2 statement verbatim"), Steps[StepIndex].Label, Questions[Index]);
		TestEqual(TEXT("Deel 3 maps back onto its playtest row"), GetPlaytestQuestionIndex(StepIndex), Index);
	}
	TestTrue(TEXT("The gate question is the last step of the guide"), Steps.Last().Label.Contains(TEXT("GATE")));
	TestEqual(TEXT("A change line feeds no playtest row"), GetPlaytestQuestionIndex(0), (int32)INDEX_NONE);

	// Part bookkeeping: the panel counts "stap 3/11" off these three helpers.
	const int32 Changes = GetPartStepCount(EEclipseGuidePart::Controls);
	TestTrue(TEXT("Step 1 is deel 1"), GetPartOfStep(0) == EEclipseGuidePart::Controls);
	TestTrue(TEXT("De eerste stap na de wijzigingen is deel 2"), GetPartOfStep(Changes) == EEclipseGuidePart::Systems);
	TestTrue(TEXT("En daarna deel 3"), GetPartOfStep(Changes + SystemStepCount) == EEclipseGuidePart::Questions);
	TestEqual(TEXT("De laatste wijzigingsregel is laatste van zijn deel"),
		GetIndexWithinPart(Changes - 1), Changes - 1);
	TestEqual(TEXT("De eerste systeemstap herstart de telling"), GetIndexWithinPart(Changes), 0);
	TestEqual(TEXT("De eerste vraag herstart de telling"), GetIndexWithinPart(Changes + SystemStepCount), 0);
	TestEqual(TEXT("Deel 2 telt twee oordelen"), GetPartStepCount(EEclipseGuidePart::Systems), SystemStepCount);
	TestEqual(TEXT("Deel 3 telt de 13.2-vragen"), GetPartStepCount(EEclipseGuidePart::Questions), QuestionStepCount);

	// DE DEBRIEF-ASSERT IS WEG, en dat is met opzet. Hier stond dat de debrief-stap
	// alleen getallen mocht citeren die TESTROUTE_OBJECTIVES.md noemt. Die stap
	// bestaat niet meer: uitbetaling en dagovergang staan als assert in
	// M11GauntletOnShippedData, dus de owner hoeft ze niet na te tellen — precies
	// de regel waar deze herbouw op staat.
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
	TestEqual(TEXT("Everything starts pending"), Progress.CountInState(EEclipseGuideStepState::Pending), GetGuideStepCount());

	// J bevestigt de actieve stap; het onderscheid tussen "gelezen" en "beoordeeld"
	// blijft in de toestand bewaard, want dat is wat het archief moet tonen.
	TestTrue(TEXT("J settles the active step"), Progress.ConfirmActive());
	TestTrue(TEXT("Een bevestigde stap leest als bevestigd"),
		Progress.GetStates()[0] == EEclipseGuideStepState::Confirmed);
	TestEqual(TEXT("De gids schuift op"), Progress.GetActiveIndex(), 1);

	// N is een ECHT antwoord op een oordeel, geen schouderophalen — en op een
	// wijzigingsregel betekent het "gelezen en verder".
	TestTrue(TEXT("N beantwoordt de actieve stap"), Progress.RejectActive());
	TestTrue(TEXT("Een afgewezen stap leest als afgewezen of overgeslagen"),
		Progress.GetStates()[1] == EEclipseGuideStepState::Rejected
		|| Progress.GetStates()[1] == EEclipseGuideStepState::Skipped);
	TestEqual(TEXT("Twee stappen afgehandeld"), Progress.GetActiveIndex(), 2);

	// Walk the rest out and check the end state is a real end state.
	EclipseTestGuideTest::ConfirmSteps(Progress, GetGuideStepCount());
	TestTrue(TEXT("The guide finishes"), Progress.IsComplete());
	TestEqual(TEXT("A finished guide has no active step"), Progress.GetActiveIndex(), (int32)INDEX_NONE);
	TestEqual(TEXT("Nothing is left pending"), Progress.CountInState(EEclipseGuideStepState::Pending), 0);
	TestFalse(TEXT("A finished guide settles nothing further"), Progress.ConfirmActive());
	TestFalse(TEXT("A finished guide rejects nothing further"), Progress.RejectActive());

	// The tally the panel and the archive both print.
	// Eén N is er gegeven; of die als "overgeslagen" of als "niet goed" wordt
	// geboekt hangt af van het deel waar hij viel, en beide zijn een echt antwoord.
	TestEqual(TEXT("Eén stap kreeg een N"),
		Progress.CountInState(EEclipseGuideStepState::Skipped)
		+ Progress.CountInState(EEclipseGuideStepState::Rejected), 1);
	TestEqual(TEXT("De rest kreeg een J"),
		Progress.CountInState(EEclipseGuideStepState::Confirmed), GetGuideStepCount() - 1);

	Progress.Reset();
	TestEqual(TEXT("Reset returns to step 1"), Progress.GetActiveIndex(), 0);
	TestFalse(TEXT("Reset clears the archive-worthy state"), Progress.HasAnyProgress());
	TestEqual(TEXT("Reset clears every step"), Progress.CountInState(EEclipseGuideStepState::Pending), GetGuideStepCount());
	return true;
}

namespace EclipseTestGuideTest
{
	/**
	 * DE TELLERREGEL OP INHOUD, niet op positie.
	 *
	 * Hier stond driemaal `Lines.Last()`. Dat werkte zolang de stappen het paneel
	 * exact vulden — en ComposeGuidePanelLines vult sinds jaar en dag AAN tot
	 * GuidePanelLineCount met lege regels, dus zodra deel 1 krimpt is de laatste
	 * regel leeg en valt de assertie om op een paneel dat perfect klopt. Dat is
	 * precies de fout die deze test op 27-07 drie draaien rood hield: een toets die
	 * een TOEVALLIGE indeling vastlegt in plaats van een eigenschap.
	 *
	 * De teller is herkenbaar aan zijn eigen tekst, en dat is stabiel onder elke
	 * paneelhoogte.
	 */
	FString FindTallyLine(const TArray<FString>& Lines)
	{
		for (const FString& Line : Lines)
		{
			if (Line.Contains(TEXT("beoordeeld ")))
			{
				return Line;
			}
		}
		return FString();
	}
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
		// Kop + de look-waardenregel + elke stap + de teller. Het paneel bouwt zijn
		// rijen één keer, dus de bovengrens staat vast; het AANTAL stappen niet meer,
		// want deel 1 toont wat er veranderd is.
		// HERZIEN 27-07 NA EEN GEMETEN DIAGNOSE. Hier stond
		// `Lines.Num() == GetGuideStepCount() + 3`, en die assertie legde een
		// CONSTANTE naast een VARIABELE. ComposeGuidePanelLines eindigt met
		//   while (Lines.Num() < GuidePanelLineCount) { Lines.Add(FString()); }
		//   Lines.SetNum(GuidePanelLineCount);
		// dus het paneel geeft ALTIJD precies GuidePanelLineCount regels terug —
		// het vult aan en kapt af, want de widget bouwt zijn rijen één keer en doet
		// daarna alleen nog SetText. Die 'stappen + 3' klopte alleen zolang deel 1
		// vol zat; zodra er niets nieuws sinds de vorige sessie is (één placeholder
		// in plaats van drie wijzigingen) liep de verwachting weg en werd de bar
		// rood op code waar niemand aan had gezeten.
		//
		// Drie draaien lang rood, en drie verklaringen van mij die alle drie fout
		// waren (datum-race, teller-tegen-lus, en een rekensom). Wat het oploste
		// waren twee logregels: LUS en TELLER meldden allebei 8 stappen, dus de
		// bronnen waren het eens en zat het verschil in het paneel — precies waar
		// ik eerst keek en mezelf toen uit had gepraat.
		//
		// WAT ER NU GETOETST WORDT is het contract zelf: de vaste hoogte, én dat
		// alle stappen daar ook echt in PASSEN. Dat tweede is waar de assertie
		// tanden houdt — zonder die regel zou een gids die groeit tot voorbij zijn
		// paneel stilletjes stappen afkappen, en dat is de fout die de vaste hoogte
		// kan veroorzaken.
		TestEqual(TEXT("Het paneel houdt zijn vaste hoogte"), Lines.Num(), GuidePanelLineCount);
		TestTrue(FString::Printf(
				TEXT("Alle %d stappen passen in het paneel (%d regels, waarvan %d omlijsting)"),
				Steps.Num(), GuidePanelLineCount, 3),
			Steps.Num() + 3 <= GuidePanelLineCount);
		TestTrue(TEXT("En dat past binnen de vaste paneelhoogte"), Lines.Num() <= GuidePanelLineCount);

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

		TestTrue(TEXT("The header counts within its part"),
			Lines[0].Contains(FString::Printf(TEXT("stap 1/%d"), GetPartStepCount(EEclipseGuidePart::Controls))));
		TestTrue(TEXT("The header names the part"), Lines[0].Contains(TEXT("deel 1")));
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

		TestTrue(TEXT("De tellerregel staat in het paneel en telt alles nog open"),
			EclipseTestGuideTest::FindTallyLine(Lines).Contains(
				FString::Printf(TEXT("nog open %d"), GetGuideStepCount())));
	}

	// A settled step collapses to marker + label + how it was settled.
	{
		// De stap moest vroeger door detectie afgevinkt worden; sinds de herbouw
		// beoordeelt de owner hem zelf, dus hier ook.
		Progress.ConfirmActive();
		const TArray<FString> Lines = ComposeGuidePanelLines(Progress);
		TestTrue(TEXT("A settled step is ticked"), Lines[2].StartsWith(TEXT("[v]")));
		TestFalse(TEXT("A settled step folds its expectation away"), Lines[2].Contains(Steps[0].Expectation));
		TestTrue(TEXT("The next step became active"), Lines[3].StartsWith(TEXT(">>")));
		// DE KOP VOLGT DE ACTIEVE STAP, en welke dat is wordt AFGELEID. Hier stond
		// "stap 2/<aantal in deel 1>", en dat gaat ervan uit dat deel 1 minstens
		// twee stappen heeft. Is er niets nieuws sinds de vorige sessie, dan telt
		// deel 1 er één (de placeholder) en staat stap 2 in een ander deel — dan
		// faalt deze regel op een gids die precies doet wat hij hoort te doen.
		const EEclipseGuidePart ActivePart = GetPartOfStep(1);
		TestTrue(TEXT("The header follows along"),
			Lines[0].Contains(FString::Printf(TEXT("stap %d/%d"),
				GetIndexWithinPart(1) + 1, GetPartStepCount(ActivePart))));
		TestTrue(TEXT("De teller telt de beoordeling"),
			EclipseTestGuideTest::FindTallyLine(Lines).Contains(TEXT("beoordeeld 1")));
	}

	// Deel 2's responsiveness row is the one place a live measurement belongs, and
	// "not measured" must read as not measured — never as a silent zero.
	// Doorlopen tot de eerste systeemstap: dat is de "Order-reactie"-regel. Vers
	// beginnen, want hierboven is er al één stap afgehandeld.
	Progress.Reset();
	const int32 Changes = GetPartStepCount(EEclipseGuidePart::Controls);
	EclipseTestGuideTest::ConfirmSteps(Progress, Changes);
	{
		// De ACTIEVE regel opzoeken in plaats van uitrekenen. De oude versie telde
		// hem uit de deel-groottes (Changes + 3) en dat brak zodra deel 1 van vaste
		// veertien naar "het aantal wijzigingen" ging. Een index die je zoekt kan
		// niet verschuiven; een index die je uitrekent wel.
		const TArray<FString> Blank = ComposeGuidePanelLines(Progress);
		const int32 ResponsivenessLine = Blank.IndexOfByPredicate(
			[](const FString& Line) { return Line.StartsWith(TEXT(">>")); });
		TestTrue(TEXT("The responsiveness row is the active one"), Blank[ResponsivenessLine].StartsWith(TEXT(">>")));
		TestTrue(TEXT("An unmeasured meter says so"), Blank[ResponsivenessLine].Contains(TEXT("nog geen orders gemeten")));

		const TArray<FString> Measured = ComposeGuidePanelLines(Progress, TEXT("8/10 binnen 1.00 s"));
		TestTrue(TEXT("A measured meter is quoted next to the judgement"),
			Measured[ResponsivenessLine].Contains(TEXT("meter: 8/10 binnen 1.00 s")));
		TestTrue(TEXT("Deel 2's header offers a verdict, not a skip"), Measured[0].Contains(TEXT("[N] niet goed")));
		TestTrue(TEXT("Deel 2 telt zijn eigen stappen"),
			Measured[0].Contains(FString::Printf(TEXT("stap 1/%d"), SystemStepCount)));
	}

	// Deel 3 asks yes/no, and says so.
	EclipseTestGuideTest::ConfirmSteps(Progress, SystemStepCount);
	{
		const TArray<FString> Lines = ComposeGuidePanelLines(Progress);
		TestTrue(TEXT("Deel 3 announces itself"), Lines[0].Contains(TEXT("deel 3: 13.2-vragen")));
		TestTrue(TEXT("Deel 3 answers with ja"), Lines[0].Contains(TEXT("[J] ja")));
		TestTrue(TEXT("Deel 3 answers with nee"), Lines[0].Contains(TEXT("[N] nee")));
		TestTrue(TEXT("Deel 3 telt zijn eigen stappen"),
			Lines[0].Contains(FString::Printf(TEXT("stap 1/%d"), QuestionStepCount)));
	}

	// Finished: no active row anywhere, and the header says where the record went.
	EclipseTestGuideTest::ConfirmSteps(Progress, QuestionStepCount);
	{
		const TArray<FString> Lines = ComposeGuidePanelLines(Progress);
		TestEqual(TEXT("A finished panel is still the same height"), Lines.Num(), GuidePanelLineCount);
		TestTrue(TEXT("The header reports completion"), Lines[0].Contains(TEXT("klaar")));
		TestTrue(TEXT("The header points at the archive"), Lines[0].Contains(TEXT("Saved/Logs")));
		for (int32 Line = 1; Line <= GetGuideStepCount(); ++Line)
		{
			TestFalse(TEXT("No step is active once the guide is done"), Lines[Line].StartsWith(TEXT(">>")));
		}
		TestTrue(TEXT("Nothing is left open"),
			EclipseTestGuideTest::FindTallyLine(Lines).Contains(TEXT("nog open 0")));
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
		TestEqual(TEXT("Title + three part headers + twenty steps + tally"), Block.Num(), GetGuideStepCount() + 5);
		TestTrue(TEXT("The block names the variant it was built as"), Block[0].Contains(TEXT("variant A")));
		TestTrue(TEXT("Everything is open"),
			Block.Last().Contains(FString::Printf(TEXT("%d nog open"), GetGuideStepCount())));
		TestTrue(TEXT("An untouched step claims nothing"), Block[2].Contains(TEXT("nog open")));
	}

	// One of every outcome, so the archive can be read back without guessing.
	// Eén van elke uitkomst, zodat het archief terug te lezen is zonder te raden:
	// de wijzigingsregels doorlopen (één met een N), dan één systeemoordeel "niet
	// goed", en dan één 13.2-vraag "nee". De rest blijft open.
	const int32 ChangeLines = GetPartStepCount(EEclipseGuidePart::Controls);
	FEclipseGuideProgress Progress;
	Progress.RejectActive();                           // een wijzigingsregel: overgeslagen
	EclipseTestGuideTest::ConfirmSteps(Progress, ChangeLines - 1);
	Progress.RejectActive();                           // een systeemoordeel: "niet goed"
	EclipseTestGuideTest::ConfirmSteps(Progress, SystemStepCount - 1);
	Progress.RejectActive();                           // een 13.2-vraag: "nee"

	const TArray<FString> Block = ComposeGuideSummaryBlock(Progress);
	TestEqual(TEXT("The block keeps its fixed shape"), Block.Num(), GetGuideStepCount() + 5);

	// Layout: titel, [deel 1], de wijzigingsregels, [deel 2], de oordelen,
	// [deel 3], de 13.2-vragen, teller.
	const int32 Changed = GetPartStepCount(EEclipseGuidePart::Controls);
	TestTrue(TEXT("Deel 1 is labelled"), Block[1].Contains(TEXT("deel 1")));
	TestTrue(TEXT("Deel 2 is labelled"), Block[Changed + 2].Contains(TEXT("deel 2")));
	TestTrue(TEXT("Deel 3 is labelled"), Block[Changed + SystemStepCount + 3].Contains(TEXT("deel 3")));

	// Het onderscheid dat telt is niet meer "gedetecteerd tegen handmatig" — er
	// valt niets meer te detecteren — maar wat de owner ERVAN VOND. Een oordeel
	// dat als "goed" of "niet goed" in het archief staat, is het hele product van
	// een speelronde.
	TestTrue(TEXT("Een niet goed beoordeeld systeem staat als niet goed in het archief"),
		Block[Changed + 3].Contains(TEXT("niet goed")));
	TestTrue(TEXT("Een goed beoordeeld systeem staat als goed"), Block[Changed + 4].Contains(TEXT("goed")));
	TestTrue(TEXT("Een vraag met nee staat als nee"),
		Block[Changed + SystemStepCount + 4].Contains(TEXT("nee")));
	TestTrue(TEXT("An unanswered question stays open"), Block.Last(1).Contains(TEXT("nog open")));

	// Every step is named in the archive, not just the settled ones.
	const TArray<FEclipseGuideStep> Steps = GetGuideSteps();
	const FString Joined = FString::Join(Block, TEXT("\n"));
	for (const FEclipseGuideStep& Step : Steps)
	{
		TestTrue(FString::Printf(TEXT("The archive names '%s'"), *Step.Label), Joined.Contains(Step.Label));
	}

	TestTrue(TEXT("De teller telt de overgeslagen regel"), Block.Last().Contains(TEXT("1 overgeslagen")));
	TestTrue(TEXT("De teller telt de twee negatieve antwoorden"), Block.Last().Contains(TEXT("2 niet goed")));
	TestTrue(TEXT("De teller geeft toe wat nooit bereikt is"),
		Block.Last().Contains(FString::Printf(TEXT("%d nog open"), QuestionStepCount - 1)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
