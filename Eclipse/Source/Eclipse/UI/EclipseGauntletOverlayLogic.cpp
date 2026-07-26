#include "UI/EclipseGauntletOverlayLogic.h"

namespace EclipseGauntletOverlay
{
	namespace
	{
		/** Row labels of the verdict block, padded to one width so the block reads as a table in plain text. */
		const TCHAR* CriterionLabels[CriterionCount] = {
			TEXT("order-round-trip"),
			TEXT("targeting"),
			TEXT("dilatatie-comfort"),
			TEXT("vertrouwen"),
			TEXT("gebruiks-trek")
		};

		/** The measured/answered value of one criterion, without its pass-fail mark. */
		FString DescribeCriterion(int32 CriterionIndex, const FEclipseGauntletCriteria& Criteria)
		{
			switch (CriterionIndex)
			{
			case 0:
				return Criteria.RoundTripSamples <= 0
					? FString(TEXT("geen orders gemeten"))
					: FString::Printf(TEXT("%d/%d antwoorden <= %.2f s echte tijd (slechtste %.3f s)"),
						Criteria.RoundTripWithinBar, Criteria.RoundTripSamples, Criteria.RoundTripBarSeconds, Criteria.RoundTripWorstSeconds);

			case 1:
			{
				const int32 Attempts = Criteria.CleanPicks + Criteria.MisPicks;
				return Attempts <= 0
					? FString(TEXT("nog geen picks geteld"))
					: FString::Printf(TEXT("%d schoon van %d pogingen (%d mis-picks)"), Criteria.CleanPicks, Attempts, Criteria.MisPicks);
			}

			case 2:
				return Criteria.Comfort == EEclipseGauntletAnswer::Good ? FString(TEXT("goed: sneller denken"))
					: Criteria.Comfort == EEclipseGauntletAnswer::Bad ? FString(TEXT("niet goed: als een menu openen"))
					: FString(TEXT("-"));

			case 3:
				return Criteria.Confidence == EEclipseGauntletAnswer::Good ? FString(TEXT("geen stille orderfouten"))
					: Criteria.Confidence == EEclipseGauntletAnswer::Bad ? FString(TEXT("wel een stille orderfout"))
					: FString(TEXT("-"));

			case 4:
				return FString::Printf(TEXT("%d entries deze beat, gem. %.1f over %d beats"),
					Criteria.EntriesThisBeat, Criteria.AverageEntriesPerBeat, Criteria.ClosedBeatCount);

			default:
				return FString(TEXT("-"));
			}
		}

		FString DescribeAnswer(EEclipseGauntletAnswer Answer)
		{
			switch (Answer)
			{
			case EEclipseGauntletAnswer::Good: return FString(TEXT("goed  "));
			case EEclipseGauntletAnswer::Bad:  return FString(TEXT("slecht"));
			default:                           return FString(TEXT("  ?   "));
			}
		}
	}

	EEclipseGauntletStatus EvaluateCriterion(int32 CriterionIndex, const FEclipseGauntletCriteria& Criteria)
	{
		switch (CriterionIndex)
		{
		case 0:
			// One late answer already falsifies the criterion (draaiboek: "streng
			// zijn"); a short run is simply not measured yet.
			if (Criteria.RoundTripSamples <= 0)
			{
				return EEclipseGauntletStatus::Open;
			}
			if (Criteria.RoundTripWithinBar < Criteria.RoundTripSamples)
			{
				return EEclipseGauntletStatus::Fail;
			}
			return Criteria.RoundTripSamples >= Criteria.RoundTripRequiredSamples
				? EEclipseGauntletStatus::Pass
				: EEclipseGauntletStatus::Open;

		case 1:
		{
			// Target is >= 9/10, so exactly one mis-pick is tolerated; the second
			// one settles the verdict before the tenth attempt is even taken.
			const int32 Attempts = Criteria.CleanPicks + Criteria.MisPicks;
			const int32 Tolerated = FMath::Max(0, Criteria.TargetingRequiredAttempts - Criteria.TargetingRequiredClean);
			if (Criteria.MisPicks > Tolerated)
			{
				return EEclipseGauntletStatus::Fail;
			}
			if (Attempts < Criteria.TargetingRequiredAttempts)
			{
				return EEclipseGauntletStatus::Open;
			}
			return Criteria.CleanPicks >= Criteria.TargetingRequiredClean
				? EEclipseGauntletStatus::Pass
				: EEclipseGauntletStatus::Fail;
		}

		case 2:
			return Criteria.Comfort == EEclipseGauntletAnswer::Good ? EEclipseGauntletStatus::Pass
				: Criteria.Comfort == EEclipseGauntletAnswer::Bad ? EEclipseGauntletStatus::Fail
				: EEclipseGauntletStatus::Open;

		case 3:
			return Criteria.Confidence == EEclipseGauntletAnswer::Good ? EEclipseGauntletStatus::Pass
				: Criteria.Confidence == EEclipseGauntletAnswer::Bad ? EEclipseGauntletStatus::Fail
				: EEclipseGauntletStatus::Open;

		case 4:
			// Closed beats only: the beat in progress may legitimately still be
			// empty. "An unused mode is a failed mode" = below one entry per beat.
			if (Criteria.ClosedBeatCount <= 0)
			{
				return EEclipseGauntletStatus::Open;
			}
			return Criteria.AverageEntriesPerBeat >= 1.0f
				? EEclipseGauntletStatus::Pass
				: EEclipseGauntletStatus::Fail;

		default:
			return EEclipseGauntletStatus::Open;
		}
	}

	FEclipseGauntletVerdict ComposeVerdict(const FEclipseGauntletCriteria& Criteria)
	{
		FEclipseGauntletVerdict Verdict;
		Verdict.Lines.Reserve(VerdictLineCount);
		Verdict.Statuses.Reserve(CriterionCount);

		// De kop noemt zijn eigen toetsen. Drie van de vijf criteria vullen zich
		// niet vanzelf (targeting, comfort, vertrouwen), en zonder die aanwijzing
		// blijven ze op "nog niet gemeten" staan terwijl de tester zich afvraagt
		// wat er kapot is.
		Verdict.Lines.Add(TEXT("=== R3-VERDICT INPUT ===  [F4] pick schoon · [F5] mis-pick · [F6] comfort · [F7] vertrouwen · [F8] beat af"));

		for (int32 Index = 0; Index < CriterionCount; ++Index)
		{
			const EEclipseGauntletStatus Status = EvaluateCriterion(Index, Criteria);
			Verdict.Statuses.Add(Status);
			Verdict.FailedCount += Status == EEclipseGauntletStatus::Fail ? 1 : 0;
			Verdict.OpenCount += Status == EEclipseGauntletStatus::Open ? 1 : 0;

			const TCHAR* Mark = Status == EEclipseGauntletStatus::Fail ? TEXT("   << GEFAALD")
				: Status == EEclipseGauntletStatus::Open ? TEXT("   << NIET GEMETEN")
				: TEXT("");
			Verdict.Lines.Add(FString::Printf(TEXT("%d %-18s: %s%s"),
				Index + 1, CriterionLabels[Index], *DescribeCriterion(Index, Criteria), Mark));
		}

		FString Tally = FString::Printf(TEXT("-> %d %s gefaald"),
			Verdict.FailedCount, Verdict.FailedCount == 1 ? TEXT("criterium") : TEXT("criteria"));
		if (Verdict.OpenCount > 0)
		{
			// An unmeasured criterion is not a pass and not a failure — saying so
			// out loud keeps a half-played gauntlet from reading as a verdict.
			Tally += FString::Printf(TEXT("  (%d nog niet gemeten)"), Verdict.OpenCount);
		}
		Verdict.Lines.Add(Tally);

		return Verdict;
	}

	TArray<FEclipseControlRow> GetControlRows()
	{
		// Mirrors AEclipsePlayerController::SetupInputComponent one-for-one
		// (SPEC-P2-02 provisional debug bindings; the context stack is P2-07).
		return {
			{ TEXT("Lopen"),        TEXT("WASD"),              TEXT("linkerstick") },
			{ TEXT("Rondkijken"),   TEXT("muis"),              TEXT("rechterstick") },
			{ TEXT("Vuren"),        TEXT("LMB"),               TEXT("RT") },
			{ TEXT("Sprint"),       TEXT("Shift (vasthouden)"), TEXT("L3 (togglen)") },
			{ TEXT("Hurken"),       TEXT("Ctrl"),              TEXT("B") },
			{ TEXT("Springen"),     TEXT("Spatie"),            TEXT("A") },
			{ TEXT("Mikken"),       TEXT("RMB"),               TEXT("LT (buiten Command Mode)") },
			{ TEXT("1e/3e persoon"),TEXT("C"),                 TEXT("geen (R3 ging per ongeluk af)") },
			{ TEXT("Command Mode"), TEXT("Q vasthouden"),      TEXT("LB vasthouden") },
			{ TEXT("Volgende"),     TEXT("Tab / scroll op (tijdens Command Mode)"), TEXT("RB (tijdens Command Mode)") },
			{ TEXT("Vorige"),       TEXT("scroll neer (tijdens Command Mode)"),     TEXT("LT (tijdens Command Mode)") },
			{ TEXT("Onder kruis"),  TEXT("E (tijdens Command Mode)"),               TEXT("X (tijdens Command Mode)") },
			{ TEXT("Orders"),       TEXT("1 2 3 4"),           TEXT("D-pad") },
			// VIER rijen noemen hun context, want alle vier gaan door een handler die
			// meteen terugkeert als Command Mode niet vast staat (`if (!State.bHeld)
			// return;`): stance, volgende, vorige en onder-kruis. Tot vannacht stond
			// die voorwaarde bij precies één kolom van één rij, dus je drukte RB of X
			// in het veld en er gebeurde stil niets. Vastgepind door
			// Eclipse.Feel.Input.CommandModeControlsAreSilentOutsideTheMode.
			//
			// Beide kolommen noemen hun CONTEXT, want stance werkt nergens anders:
			// ToggleHeldStance() keert meteen terug als Command Mode niet vast staat,
			// en op toetsenbord wordt LeftAlt gepollt op het moment van de order zelf.
			// Zonder die toevoeging drukt de speler Y in het veld en gebeurt er stil
			// niets.
			{ TEXT("Stance"),       TEXT("Alt vast bij de order"), TEXT("Y (tijdens Command Mode)") }
		};
	}

	TArray<FString> GetPlaytestQuestions()
	{
		// GDD 13.2's questions as statements so one key per row settles them; the
		// gate question ("do testers voluntarily play a second loop?") stays last.
		return {
			TEXT("Het spel voelde goed"),
			TEXT("Niets voelde slecht of traag"),
			TEXT("Ik wist wat ik moest doen"),
			TEXT("Mijn squad deed wat ik vroeg"),
			TEXT("GATE: ik wil vrijwillig een tweede ronde")
		};
	}

	TArray<FString> ComposePlaytestBlock(const TArray<EEclipseGauntletAnswer>& Answers)
	{
		const TArray<FString> Questions = GetPlaytestQuestions();

		TArray<FString> Lines;
		Lines.Reserve(Questions.Num() + 2);
		Lines.Add(TEXT("--- 13.2 PLAYTEST (gate: speel ik vrijwillig een tweede ronde?) ---  [6]-[0] antwoorden ---"));
		for (int32 Index = 0; Index < Questions.Num(); ++Index)
		{
			const EEclipseGauntletAnswer Answer = Answers.IsValidIndex(Index) ? Answers[Index] : EEclipseGauntletAnswer::Unanswered;
			// De TOETS erbij, want het paneel toonde de vragen zonder te zeggen hoe je
			// ze beantwoordt. De owner moest weten dat 6 t/m 0 op de vijf regels
			// slaan; dat stond nergens, ook niet in BESTURING.md, en een vraag die
			// je niet kunt beantwoorden leest als een vraag die kapot is.
			Lines.Add(FString::Printf(TEXT("[%d] [%s] %s"), Index + 6 <= 9 ? Index + 6 : 0,
				*DescribeAnswer(Answer), *Questions[Index]));
		}

		const EEclipseGauntletAnswer Gate = Answers.IsValidIndex(PlaytestQuestionCount - 1)
			? Answers[PlaytestQuestionCount - 1]
			: EEclipseGauntletAnswer::Unanswered;
		Lines.Add(FString::Printf(TEXT("-> gate: %s"),
			Gate == EEclipseGauntletAnswer::Good ? TEXT("JA")
			: Gate == EEclipseGauntletAnswer::Bad ? TEXT("NEE")
			: TEXT("niet beantwoord")));
		return Lines;
	}

	EEclipseGauntletAnswer CycleAnswer(EEclipseGauntletAnswer Answer)
	{
		switch (Answer)
		{
		case EEclipseGauntletAnswer::Unanswered: return EEclipseGauntletAnswer::Good;
		case EEclipseGauntletAnswer::Good:       return EEclipseGauntletAnswer::Bad;
		default:                                 return EEclipseGauntletAnswer::Unanswered;
		}
	}
}
