#include "UI/EclipseTestGuideLogic.h"
#include "Eclipse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace EclipseTestGuide
{
	namespace
	{

		/**
		 * WAT ER VERANDERD IS, per landing opgeschreven op het moment van landen —
		 * niet achteraf gereconstrueerd, want dan staat er wat ik me herinner in
		 * plaats van wat er gebeurde.
		 *
		 * Alleen dingen die de owner NIET uit een meting kan halen: een knop die
		 * iets anders betekent dan hij gewend was, of een verschil dat op papier
		 * niets zegt. Wat de suite bewijst, hoort hier niet.
		 */
		struct FGuideChange
		{
			const TCHAR* Date;   // ISO, zodat sorteren en vergelijken tekst blijft
			const TCHAR* What;
		};

		const FGuideChange GuideChanges[] = {
			{ TEXT("2026-07-26"), TEXT("RB buiten Command Mode is nu WAPENWISSEL (was camerastandpunt). Het camerastandpunt houdt C op het toetsenbord — een bumper is beter voor iets wat je middenin een vuurgevecht doet") },
			{ TEXT("2026-07-26"), TEXT("X en R zijn HERLADEN buiten Command Mode. Zit je magazijn vol, dan valt dezelfde knop terug op de snelle hergroepeer-order") },
			{ TEXT("2026-07-26"), TEXT("Y tijdens Command Mode cycelt door VIER DOCTRINES — recon, ready, overwatch, aggressive — en die gelden meteen voor de hele squad. Y deed tot vandaag niets") },
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
			{ TEXT("Order-reactie voelt direct"),
			  TEXT("10x een order 1-4"),
			  TEXT("10x D-pad"),
			  TEXT("de METING zegt altijd ~0 s (vraag en antwoord vallen in hetzelfde frame), dus die bewijst niets. Wat JIJ beoordeelt: valt het antwoord op? Er is sinds 26-07 een ingesproken zin per order, met een rem van 2 s per soldaat") },
			{ TEXT("De demper: is 1200 tegen 5000 cm te voelen?"),
			  TEXT("wissel naar de sidearm (RB) en schiet vanaf een afstand"),
			  TEXT("RB, dan RT"),
			  TEXT("je sidearm is gedempt en alarmeert tot 12 m in plaats van 50. Op papier zegt dat niets — de vraag is of je MERKT dat je met het pistool wegkomt waar de AR je verraadt. Zo nee, dan is de demper een timbre en geen keuze") }
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
		// De order-reactie is de EERSTE systeemstap; zijn index hangt af van hoeveel
		// wijzigingen er boven staan, dus hij wordt berekend in plaats van vastgezet.

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

		/** Het woord dat in het archief belandt. */
		const TCHAR* DescribeGuideState(EEclipseGuidePart Part, EEclipseGuideStepState State)
		{
			switch (State)
			{
			case EEclipseGuideStepState::Confirmed:
				return Part == EEclipseGuidePart::Controls ? TEXT("gelezen")
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
			case EEclipseGuideStepState::Confirmed: return TEXT("[v]");
			case EEclipseGuideStepState::Skipped:   return TEXT("[-]");
			case EEclipseGuideStepState::Rejected:  return TEXT("[x]");
			default:                                return TEXT("[ ]");
			}
		}
	}

	FString FindLastSessionDate()
	{
		// Het eindrapport heet EclipseGauntletR3_JJJJMMDD_UUMMSS.txt en gaat naar
		// Saved/Logs. Het NIEUWSTE bestand is dus de vorige sessie, en zijn naam
		// draagt de datum al — geen bestand hoeven openen.
		TArray<FString> Reports;
		IFileManager::Get().FindFiles(Reports, *(FPaths::ProjectLogDir() / TEXT("EclipseGauntletR3_*.txt")), true, false);
		if (Reports.Num() == 0)
		{
			return FString(); // eerste sessie: alles tonen
		}
		Reports.Sort();
		const FString Newest = Reports.Last();

		// "EclipseGauntletR3_20260726_174501.txt" -> "2026-07-26"
		int32 Underscore = INDEX_NONE;
		if (!Newest.FindChar(TEXT('_'), Underscore) || Newest.Len() < Underscore + 9)
		{
			return FString();
		}
		const FString Stamp = Newest.Mid(Underscore + 1, 8);
		if (Stamp.Len() != 8 || !Stamp.IsNumeric())
		{
			return FString();
		}
		const FString Date = FString::Printf(TEXT("%s-%s-%s"), *Stamp.Left(4), *Stamp.Mid(4, 2), *Stamp.Right(2));

		// Eén keer per sessie hardop zeggen waarop er gefilterd wordt. GetGuideSteps
		// draait bij elke verversing van het paneel, dus zonder deze vlag zou het een
		// regel per frame worden — en dan is het geen diagnose meer maar ruis.
		static bool bAnnounced = false;
		if (!bAnnounced)
		{
			bAnnounced = true;
			UE_LOG(LogEclipse, Display,
				TEXT("Gids: vorige sessie was %s (%s) — alleen wijzigingen daarna staan in deel 1."),
				*Date, *Newest);
		}
		return Date;
	}

	int32 GetChangeStepCount()
	{
		// Alles wat NA het nieuwste eindrapport geland is. De datums in de tabel
		// zijn ISO, dus een tekstvergelijking IS een datumvergelijking — dat is de
		// hele reden dat ze zo geschreven staan.
		//
		// Deze functie stond hier tot vlak na het bouwen als "return
		// UE_ARRAY_COUNT(GuideChanges)", met een comment erboven die beloofde dat
		// hij op het rapport filterde. Dat is precies het soort belofte-in-een-
		// comment waar ik vandaag drie keer over gestruikeld ben, dus nu doet hij
		// het echt.
		const FString Since = FindLastSessionDate();
		if (Since.IsEmpty())
		{
			return UE_ARRAY_COUNT(GuideChanges);
		}
		int32 Count = 0;
		for (const FGuideChange& Change : GuideChanges)
		{
			if (FString(Change.Date) > Since)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 GetGuideStepCount()
	{
		return FMath::Max(GetChangeStepCount(), 1) + SystemStepCount + QuestionStepCount;
	}

	TArray<FEclipseGuideStep> GetGuideSteps()
	{
		TArray<FEclipseGuideStep> Steps;
		Steps.Reserve(GetGuideStepCount());

		// ---- deel 1: wat er veranderd is sinds je vorige sessie ---------------
		const FString Since = FindLastSessionDate();
		for (const FGuideChange& Change : GuideChanges)
		{
			if (!Since.IsEmpty() && FString(Change.Date) <= Since)
			{
				continue; // die kende hij al toen hij vorige keer speelde
			}
			FEclipseGuideStep& Step = Steps.AddDefaulted_GetRef();
			Step.Part = EEclipseGuidePart::Controls;
			Step.Label = TEXT("VERANDERD");
			Step.MouseKeyboard = Change.Date;
			Step.Controller = Change.Date;
			Step.Expectation = Change.What;
		}
		if (GetChangeStepCount() == 0)
		{
			// "Er is niets voor jou" hoort er te STAAN. Een lege lijst leest als een
			// fout; deze regel leest als een antwoord.
			FEclipseGuideStep& Step = Steps.AddDefaulted_GetRef();
			Step.Part = EEclipseGuidePart::Controls;
			Step.Label = TEXT("VERANDERD");
			Step.MouseKeyboard = TEXT("—");
			Step.Controller = TEXT("—");
			Step.Expectation = TEXT("niets sinds je vorige sessie: geen knop betekent iets anders");
		}

		// ---- deel 2: alleen wat een meting niet kan beantwoorden --------------
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


	EEclipseGuidePart GetPartOfStep(int32 StepIndex)
	{
		const int32 Changes = FMath::Max(GetChangeStepCount(), 1);
		if (StepIndex < Changes)
		{
			return EEclipseGuidePart::Controls;
		}
		return StepIndex < Changes + SystemStepCount
			? EEclipseGuidePart::Systems
			: EEclipseGuidePart::Questions;
	}

	int32 GetIndexWithinPart(int32 StepIndex)
	{
		switch (GetPartOfStep(StepIndex))
		{
		case EEclipseGuidePart::Controls: return StepIndex;
		case EEclipseGuidePart::Systems:  return StepIndex - FMath::Max(GetChangeStepCount(), 1);
		default:                          return StepIndex - FMath::Max(GetChangeStepCount(), 1) - SystemStepCount;
		}
	}

	int32 GetPartStepCount(EEclipseGuidePart Part)
	{
		switch (Part)
		{
		case EEclipseGuidePart::Controls: return FMath::Max(GetChangeStepCount(), 1);
		case EEclipseGuidePart::Systems:  return SystemStepCount;
		default:                          return QuestionStepCount;
		}
	}

	int32 GetPlaytestQuestionIndex(int32 StepIndex)
	{
		const int32 Index = StepIndex - FMath::Max(GetChangeStepCount(), 1) - SystemStepCount;
		return Index >= 0 && Index < QuestionStepCount ? Index : INDEX_NONE;
	}

	FEclipseGuideProgress::FEclipseGuideProgress()
	{
		Reset();
	}

	void FEclipseGuideProgress::Reset()
	{
		States.Init(EEclipseGuideStepState::Pending, GetGuideStepCount());
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
				GetGuideStepCount()));
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
				if (GetPartOfStep(Index) == EEclipseGuidePart::Systems && GetIndexWithinPart(Index) == 0)
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

		Lines.Add(FString::Printf(TEXT("beoordeeld %d · overgeslagen %d · niet goed %d · nog open %d"),
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
		Lines.Reserve(GetGuideStepCount() + 5);
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
		Lines.Add(FString::Printf(TEXT("-> %d beoordeeld | %d overgeslagen | %d niet goed | %d nog open"),
			Progress.CountInState(EEclipseGuideStepState::Confirmed),
			Progress.CountInState(EEclipseGuideStepState::Skipped),
			Progress.CountInState(EEclipseGuideStepState::Rejected),
			Progress.CountInState(EEclipseGuideStepState::Pending)));
		return Lines;
	}
}
