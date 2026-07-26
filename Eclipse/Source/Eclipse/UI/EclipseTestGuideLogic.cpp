#include "UI/EclipseTestGuideLogic.h"

namespace EclipseTestGuide
{
	namespace
	{
		/**
		 * Deel 1, index-aligned with EclipseGauntletOverlay::GetControlRows(): that
		 * table already mirrors the live bindings, so the guide adds only the two
		 * things it needs on top of a key name — what proves it worked, and which
		 * existing input action announces it. Two readers, one control table; the
		 * test asserts the counts still match.
		 */
		struct FGuideControlDetail
		{
			EEclipseGuideSignal Signal;
			const TCHAR* Expectation;
		};

		const FGuideControlDetail GuideControlDetails[ControlStepCount] = {
			{ EEclipseGuideSignal::Move,
			  TEXT("je pion loopt en de camera volgt; de HUD-kop staat op MISSION ACTIVE. Met een stick is halve uitslag halve snelheid; op toetsenbord ren je altijd 420 (wandelen kan daar niet)") },
			{ EEclipseGuideSignal::Look,
			  TEXT("het beeld draait mee; stick-kijken is nog framerate-gebonden (PLACEHOLDER GDD 8.1)") },
			{ EEclipseGuideSignal::Fire,
			  TEXT("een hitscan-schot vertrekt langs je camera; richt op een muur en kijk of de treffer klopt") },
			{ EEclipseGuideSignal::Sprint,
			  TEXT("je moet merkbaar sneller gaan: 420 -> 650 cm/s. LET OP, dit verschilt per apparaat sinds 26-07: Shift is een HOLD (terug bij loslaten), L3 is een TOGGLE — één klik start hem, en hij blijft aan tot je ophoudt met vooruit duwen, mikt, vuurt of nog eens L3 drukt") },
			{ EEclipseGuideSignal::Crouch,
			  TEXT("je zakt 52 cm (176 -> 124 cm) en gaat naar 150 cm/s. Beide zijn recent gerepareerd — de toets was dood en je zakte daarna te diep. Gebeurt er niets of voelt de hoogte raar: zeg het") },
			{ EEclipseGuideSignal::Jump,
			  TEXT("je komt van de grond. Deze actie bestond tot vandaag helemaal niet — er was geen JumpAction in het project") },
			{ EEclipseGuideSignal::Aim,
			  TEXT("de camera trekt in en de FOV versmalt zolang je hem vasthoudt. LET OP: op de controller mikt LT alleen BUITEN Command Mode; tijdens de Q/LB-hold is LT 'vorige soldaat'") },
			{ EEclipseGuideSignal::ToggleView,
			  TEXT("de camera schuift in ~0,1 s naar je ogen en terug. In 1e persoon verdwijnt alleen je HOOFD — armen, wapen en benen blijven zichtbaar. Zie je je hoofd nog: zeg het (dan mist het skelet zijn head-bone)") },
			{ EEclipseGuideSignal::CommandMode,
			  TEXT("de wereld vertraagt naar 30%; de HUD toont 'COMMAND MODE  x0.30' en loslaten zet hem exact terug op 1.0") },
			{ EEclipseGuideSignal::SelectNext,
			  TEXT("HOUD Command Mode vast (LB / Q) — daarbuiten doet deze knop niets. De regel 'target:' springt dan van ALL naar een soldaat-id, en bij elke druk naar de volgende") },
			{ EEclipseGuideSignal::SelectPrev,
			  TEXT("ook met Command Mode vast: 'target:' loopt de andere kant op — door de squad lopen moet in beide richtingen werken") },
			{ EEclipseGuideSignal::DirectPick,
			  TEXT("ook met Command Mode vast: de soldaat onder je kruis wordt 'target:'; wijs je niemand aan, dan blijft de vorige selectie staan") },
			{ EEclipseGuideSignal::Order,
			  TEXT("onder '-- squad orders --' springt de order om: 1 MoveTo, 2 FocusTarget, 3 Hold, 4 Regroup") },
			{ EEclipseGuideSignal::Stance,
			  TEXT("HOUD Command Mode vast (LB / Q) — daarbuiten doet Y niets. 'stance:' wisselt ready <-> aggressive. LET OP: dat is vandaag alleen een REGEL die omschakelt; je squad vecht er nog niet anders door. Op toetsenbord: druk J") }
		};

		/** One judgement row: no detection, the tester's word settles it. */
		struct FGuideJudgementStep
		{
			const TCHAR* Label;
			const TCHAR* MouseKeyboard;
			const TCHAR* Controller;
			const TCHAR* Expectation;
		};

		/**
		 * Deel 2 (INGAME_TESTGIDS §3). The expected values are quoted from
		 * phase0/TESTROUTE_OBJECTIVES.md and nowhere else — those numbers are
		 * asserted by Eclipse.Missions.M11GauntletOnShippedData, so the guide can
		 * claim nothing the suite does not also check.
		 */
		const FGuideJudgementStep GuideSystemSteps[SystemStepCount] = {
			{ TEXT("Squad doet wat je vroeg"),
			  TEXT("1-4, dan kijken"),
			  TEXT("D-pad, dan kijken"),
			  TEXT("iedere soldaat pakt de order op en beweegt ernaar; Eclipse.Squad.DumpOrders moet hetzelfde zeggen als de HUD") },
			{ TEXT("Order-reactie voelt direct"),
			  TEXT("10x een order 1-4"),
			  TEXT("10x D-pad"),
			  TEXT("elk antwoord binnen 1 s echte tijd (R3-criterium 1)") },
			{ TEXT("Objective vinkt af"),
			  TEXT("console ~: Eclipse.Mission.CompleteObjective Obj_M11_PatrolLeader"),
			  TEXT("toetsenbord nodig (console)"),
			  TEXT("het eerste doel vinkt af in de HUD; daarna Obj_M11_Exfil zet de fase op Extraction") },
			{ TEXT("Debrief betaalt en faalt"),
			  TEXT("console ~: Eclipse.Mission.ForceEnd win"),
			  TEXT("toetsenbord nodig (console)"),
			  TEXT("+50 credits en +25 materiaal, +20 materiaal extra als niemand neerging, dag 1 -> dag 2, regio wisselt NIET van eigenaar (Eclipse.Economy.Report)") }
		};

		/** Deel 3: the questions themselves come from the 13.2 checklist; only the "how do I answer this honestly" line is new. */
		const TCHAR* GuideQuestionExpectations[QuestionStepCount] = {
			TEXT("je eigen oordeel over de hele ronde — geen groene meting kan dit overrulen"),
			TEXT("denk aan de traagste order en het rafeligste moment dat je zag"),
			TEXT("zonder deze gids en zonder het GDD: wist je het uit het scherm?"),
			TEXT("hetzelfde oordeel als deel 2, nu over de hele ronde in plaats van een enkele order"),
			TEXT("de poort van 13.2: een 'nee' hier weegt zwaarder dan elke groene meting eronder")
		};

		/** Deel 2's responsiveness row is the one place a live measurement belongs. */
		constexpr int32 ResponsivenessStepIndex = ControlStepCount + 1;

		const TCHAR* GuidePartLabel(EEclipseGuidePart Part)
		{
			switch (Part)
			{
			case EEclipseGuidePart::Controls:  return TEXT("deel 1: controls");
			case EEclipseGuidePart::Systems:   return TEXT("deel 2: systemen");
			default:                           return TEXT("deel 3: 13.2-vragen");
			}
		}

		/** What J and N mean in this part — the header says it, so neither key is ever a guess. */
		const TCHAR* GuideConfirmWord(EEclipseGuidePart Part)
		{
			switch (Part)
			{
			case EEclipseGuidePart::Controls:  return TEXT("gehaald");
			case EEclipseGuidePart::Systems:   return TEXT("goed");
			default:                           return TEXT("ja");
			}
		}

		const TCHAR* GuideRejectWord(EEclipseGuidePart Part)
		{
			switch (Part)
			{
			case EEclipseGuidePart::Controls:  return TEXT("sla over");
			case EEclipseGuidePart::Systems:   return TEXT("niet goed");
			default:                           return TEXT("nee");
			}
		}

		/** The word that ends up in the archive; "handmatig bevestigd" is a finding, not a synonym of "gedetecteerd". */
		const TCHAR* DescribeGuideState(EEclipseGuidePart Part, EEclipseGuideStepState State)
		{
			switch (State)
			{
			case EEclipseGuideStepState::Detected:
				return TEXT("gedetecteerd");

			case EEclipseGuideStepState::Confirmed:
				return Part == EEclipseGuidePart::Controls ? TEXT("handmatig bevestigd")
					: Part == EEclipseGuidePart::Systems ? TEXT("goed")
					: TEXT("ja");

			case EEclipseGuideStepState::Skipped:
				return TEXT("overgeslagen");

			case EEclipseGuideStepState::Rejected:
				return Part == EEclipseGuidePart::Questions ? TEXT("nee") : TEXT("niet goed");

			default:
				return TEXT("nog open");
			}
		}

		const TCHAR* GuideStateMarker(EEclipseGuideStepState State)
		{
			switch (State)
			{
			case EEclipseGuideStepState::Detected:
			case EEclipseGuideStepState::Confirmed: return TEXT("[v]");
			case EEclipseGuideStepState::Skipped:   return TEXT("[-]");
			case EEclipseGuideStepState::Rejected:  return TEXT("[x]");
			default:                                return TEXT("[ ]");
			}
		}
	}

	TArray<FEclipseGuideStep> GetGuideSteps()
	{
		TArray<FEclipseGuideStep> Steps;
		Steps.Reserve(GuideStepCount);

		// ---- deel 1: the eleven controls, straight off the live control table ----
		const TArray<EclipseGauntletOverlay::FEclipseControlRow> ControlRows = EclipseGauntletOverlay::GetControlRows();
		for (int32 Index = 0; Index < ControlStepCount; ++Index)
		{
			FEclipseGuideStep& Step = Steps.AddDefaulted_GetRef();
			Step.Part = EEclipseGuidePart::Controls;

			// A control table that grew past this list degrades to a hand-settled
			// step with an honest expectation rather than to a silent lie; the panel
			// content test fails loudly on the missing detail (14.3.5 spirit).
			if (ControlRows.IsValidIndex(Index))
			{
				Step.Label = ControlRows[Index].Action;
				Step.MouseKeyboard = ControlRows[Index].MouseKeyboard;
				Step.Controller = ControlRows[Index].Controller;
			}
			else
			{
				Step.Label = FString::Printf(TEXT("control %d"), Index + 1);
				Step.MouseKeyboard = TEXT("(geen besturingsregel)");
				Step.Controller = TEXT("(geen besturingsregel)");
			}
			Step.Signal = GuideControlDetails[Index].Signal;
			Step.Expectation = GuideControlDetails[Index].Expectation;
		}

		// ---- deel 2: the four system judgements -------------------------------
		for (const FGuideJudgementStep& Source : GuideSystemSteps)
		{
			FEclipseGuideStep& Step = Steps.AddDefaulted_GetRef();
			Step.Part = EEclipseGuidePart::Systems;
			Step.Label = Source.Label;
			Step.MouseKeyboard = Source.MouseKeyboard;
			Step.Controller = Source.Controller;
			Step.Expectation = Source.Expectation;
		}

		// ---- deel 3: the 13.2 checklist, gate question last --------------------
		const TArray<FString> Questions = EclipseGauntletOverlay::GetPlaytestQuestions();
		for (int32 Index = 0; Index < QuestionStepCount; ++Index)
		{
			FEclipseGuideStep& Step = Steps.AddDefaulted_GetRef();
			Step.Part = EEclipseGuidePart::Questions;
			Step.Label = Questions.IsValidIndex(Index) ? Questions[Index] : FString::Printf(TEXT("13.2-vraag %d"), Index + 1);
			Step.MouseKeyboard = TEXT("J = ja · N = nee");
			Step.Controller = TEXT("toetsenbord (J / N)");
			Step.Expectation = GuideQuestionExpectations[Index];
		}

		return Steps;
	}

	EEclipseGuideSignal GetStepSignal(int32 StepIndex)
	{
		// Allocation-free: this is the one function the input path calls, and it
		// runs on every Started event of every bound action while the panel is open.
		return StepIndex >= 0 && StepIndex < ControlStepCount
			? GuideControlDetails[StepIndex].Signal
			: EEclipseGuideSignal::None;
	}

	EEclipseGuidePart GetPartOfStep(int32 StepIndex)
	{
		if (StepIndex < ControlStepCount)
		{
			return EEclipseGuidePart::Controls;
		}
		return StepIndex < ControlStepCount + SystemStepCount
			? EEclipseGuidePart::Systems
			: EEclipseGuidePart::Questions;
	}

	int32 GetIndexWithinPart(int32 StepIndex)
	{
		switch (GetPartOfStep(StepIndex))
		{
		case EEclipseGuidePart::Controls: return StepIndex;
		case EEclipseGuidePart::Systems:  return StepIndex - ControlStepCount;
		default:                          return StepIndex - ControlStepCount - SystemStepCount;
		}
	}

	int32 GetPartStepCount(EEclipseGuidePart Part)
	{
		switch (Part)
		{
		case EEclipseGuidePart::Controls: return ControlStepCount;
		case EEclipseGuidePart::Systems:  return SystemStepCount;
		default:                          return QuestionStepCount;
		}
	}

	int32 GetPlaytestQuestionIndex(int32 StepIndex)
	{
		const int32 Index = StepIndex - ControlStepCount - SystemStepCount;
		return Index >= 0 && Index < QuestionStepCount ? Index : INDEX_NONE;
	}

	FEclipseGuideProgress::FEclipseGuideProgress()
	{
		Reset();
	}

	void FEclipseGuideProgress::Reset()
	{
		States.Init(EEclipseGuideStepState::Pending, GuideStepCount);
		ActiveIndex = 0;
	}

	bool FEclipseGuideProgress::HasAnyProgress() const
	{
		return States.ContainsByPredicate([](EEclipseGuideStepState State) { return State != EEclipseGuideStepState::Pending; });
	}

	void FEclipseGuideProgress::AdvanceActive()
	{
		for (int32 Index = FMath::Max(ActiveIndex, 0); Index < States.Num(); ++Index)
		{
			if (States[Index] == EEclipseGuideStepState::Pending)
			{
				ActiveIndex = Index;
				return;
			}
		}
		ActiveIndex = INDEX_NONE;
	}

	bool FEclipseGuideProgress::NoteSignal(EEclipseGuideSignal Signal)
	{
		// Strictly the active step. The guide asks one thing at a time, so a player
		// who walks, looks and fires in the first two seconds may not have half the
		// list evaporate behind him — he never saw what he was supposed to check.
		if (Signal == EEclipseGuideSignal::None || !States.IsValidIndex(ActiveIndex))
		{
			return false;
		}
		if (GetStepSignal(ActiveIndex) != Signal)
		{
			return false;
		}
		States[ActiveIndex] = EEclipseGuideStepState::Detected;
		AdvanceActive();
		return true;
	}

	bool FEclipseGuideProgress::ConfirmActive()
	{
		if (!States.IsValidIndex(ActiveIndex))
		{
			return false;
		}
		States[ActiveIndex] = EEclipseGuideStepState::Confirmed;
		AdvanceActive();
		return true;
	}

	bool FEclipseGuideProgress::RejectActive()
	{
		if (!States.IsValidIndex(ActiveIndex))
		{
			return false;
		}
		// On a control step N is "sla over" — variant A's escape hatch. On a
		// judgement step it is a real answer ("niet goed" / "nee"), and losing that
		// distinction in the archive would turn a finding into a shrug.
		States[ActiveIndex] = GetPartOfStep(ActiveIndex) == EEclipseGuidePart::Controls
			? EEclipseGuideStepState::Skipped
			: EEclipseGuideStepState::Rejected;
		AdvanceActive();
		return true;
	}

	int32 FEclipseGuideProgress::CountInState(EEclipseGuideStepState State) const
	{
		int32 Count = 0;
		for (const EEclipseGuideStepState Entry : States)
		{
			Count += Entry == State ? 1 : 0;
		}
		return Count;
	}

	TArray<FString> ComposeGuidePanelLines(const FEclipseGuideProgress& Progress, const FString& OrderRoundTripFact, const FString& LookSummary)
	{
		const TArray<FEclipseGuideStep> Steps = GetGuideSteps();
		const TArray<EEclipseGuideStepState>& States = Progress.GetStates();
		const int32 Active = Progress.GetActiveIndex();

		TArray<FString> Lines;
		Lines.Reserve(GuidePanelLineCount);

		// ---- header: which step, in which part, and what J and N mean here ------
		if (Active == INDEX_NONE)
		{
			Lines.Add(FString::Printf(
				TEXT("== TESTGIDS ==  klaar · alle %d stappen afgehandeld · samenvatting staat in Saved/Logs      [F3] verberg"),
				GuideStepCount));
		}
		else
		{
			const EEclipseGuidePart Part = GetPartOfStep(Active);
			Lines.Add(FString::Printf(TEXT("== TESTGIDS ==  stap %d/%d · %s      [F3] verberg · [J] %s · [N] %s"),
				GetIndexWithinPart(Active) + 1, GetPartStepCount(Part), GuidePartLabel(Part),
				GuideConfirmWord(Part), GuideRejectWord(Part)));
		}

		// The tuned look values, on screen while you judge them (owner request):
		// with the numbers visible you can say "yaw te snel" instead of "voelt
		// raar", and the next tuning round starts from a value instead of a mood.
		// Always a line, blank when nothing was passed — the panel keeps a fixed
		// height so it never reflows under the tester's eyes.
		Lines.Add(LookSummary);

		for (int32 Index = 0; Index < Steps.Num(); ++Index)
		{
			const EEclipseGuideStepState State = States.IsValidIndex(Index) ? States[Index] : EEclipseGuideStepState::Pending;
			const FEclipseGuideStep& Step = Steps[Index];

			if (Index == Active)
			{
				// The active step is the only one that shows both device columns AND
				// its expectation: knowing which key to press is half an instruction
				// without knowing what proves it worked (spec §2).
				FString Expectation = Step.Expectation;
				if (Index == ResponsivenessStepIndex)
				{
					Expectation += OrderRoundTripFact.IsEmpty()
						? FString(TEXT("  ·  meter: nog geen orders gemeten"))
						: FString::Printf(TEXT("  ·  meter: %s"), *OrderRoundTripFact);
				}
				Lines.Add(FString::Printf(TEXT(">>  %2d  %-24s  %s / %s   —   %s"),
					Index + 1, *Step.Label, *Step.MouseKeyboard, *Step.Controller, *Expectation));
			}
			else if (State == EEclipseGuideStepState::Pending)
			{
				Lines.Add(FString::Printf(TEXT("    %2d  %-24s  %s / %s"),
					Index + 1, *Step.Label, *Step.MouseKeyboard, *Step.Controller));
			}
			else
			{
				// Settled rows collapse to marker + label + how it was settled: the
				// overview survives without scrolling (spec §2).
				Lines.Add(FString::Printf(TEXT("%s %2d  %-24s  %s"),
					GuideStateMarker(State), Index + 1, *Step.Label, DescribeGuideState(Step.Part, State)));
			}
		}

		Lines.Add(FString::Printf(TEXT("gedetecteerd %d · handmatig %d · overgeslagen %d · niet goed %d · nog open %d"),
			Progress.CountInState(EEclipseGuideStepState::Detected),
			Progress.CountInState(EEclipseGuideStepState::Confirmed),
			Progress.CountInState(EEclipseGuideStepState::Skipped),
			Progress.CountInState(EEclipseGuideStepState::Rejected),
			Progress.CountInState(EEclipseGuideStepState::Pending)));

		// Fixed height contract: the widget pre-builds exactly this many rows and
		// afterwards only ever SetText on them.
		while (Lines.Num() < GuidePanelLineCount)
		{
			Lines.Add(FString());
		}
		Lines.SetNum(GuidePanelLineCount);
		return Lines;
	}

	TArray<FString> ComposeGuideSummaryBlock(const FEclipseGuideProgress& Progress)
	{
		const TArray<FEclipseGuideStep> Steps = GetGuideSteps();
		const TArray<EEclipseGuideStepState>& States = Progress.GetStates();

		TArray<FString> Lines;
		Lines.Reserve(GuideStepCount + 5);
		Lines.Add(TEXT("--- IN-GAME TESTGIDS (variant A: detecteren en aftikken, geen vergrendeling) ---"));

		EEclipseGuidePart CurrentPart = EEclipseGuidePart::Controls;
		for (int32 Index = 0; Index < Steps.Num(); ++Index)
		{
			const FEclipseGuideStep& Step = Steps[Index];
			if (Index == 0 || Step.Part != CurrentPart)
			{
				CurrentPart = Step.Part;
				Lines.Add(FString::Printf(TEXT("[%s]"), GuidePartLabel(CurrentPart)));
			}

			const EEclipseGuideStepState State = States.IsValidIndex(Index) ? States[Index] : EEclipseGuideStepState::Pending;
			Lines.Add(FString::Printf(TEXT("%s %2d %-24s %s"),
				GuideStateMarker(State), Index + 1, *Step.Label, DescribeGuideState(Step.Part, State)));
		}

		// Plain ASCII on purpose: FFileHelper picks UTF-16 for the whole archive the
		// moment one line leaves 7-bit, and the gauntlet block above this one is
		// ASCII today. The panel may use nicer separators; the log file should not
		// change encoding because a guide section joined it.
		Lines.Add(FString::Printf(TEXT("-> %d gedetecteerd | %d handmatig | %d overgeslagen | %d niet goed | %d nog open"),
			Progress.CountInState(EEclipseGuideStepState::Detected),
			Progress.CountInState(EEclipseGuideStepState::Confirmed),
			Progress.CountInState(EEclipseGuideStepState::Skipped),
			Progress.CountInState(EEclipseGuideStepState::Rejected),
			Progress.CountInState(EEclipseGuideStepState::Pending)));
		return Lines;
	}
}
