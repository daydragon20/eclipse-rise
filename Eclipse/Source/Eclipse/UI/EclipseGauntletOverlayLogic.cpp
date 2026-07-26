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
			{ TEXT("Mikken"),       TEXT("RMB"),               TEXT("LT (altijd)") },
			{ TEXT("1e/3e persoon"),TEXT("C"),                 TEXT("geen — RB draagt sinds 26-07 de wapenwissel") },
			{ TEXT("Command Mode"), TEXT("Q vasthouden"),      TEXT("LB vasthouden") },
			{ TEXT("Volgende"),     TEXT("Tab / scroll op (tijdens Command Mode)"), TEXT("RB (in CM; erbuiten: WAPENWISSEL)") },
			{ TEXT("Vorige"),       TEXT("scroll neer (tijdens Command Mode)"),     TEXT("geen — RB wrapt rond") },
			{ TEXT("Onder kruis"),  TEXT("E in CM; R/E erbuiten: HERLADEN"),        TEXT("X (in CM; erbuiten: HERLADEN)") },
			{ TEXT("Orders"),       TEXT("1 2 3 4"),           TEXT("D-pad") },
			// Elke rij noemt zijn CONTEXT, en die contexten zijn 26-07 veranderd.
			//
			// Tot vannacht deden vier controls buiten Command Mode stil niets. Drie
			// daarvan hebben er nu echt werk gekregen, en de tabel zegt wat: RB
			// wisselt buiten de modus van WAPEN (26-07 avond, punt 5: de owner vroeg
			// hem op RB; het genre legt hem meestal op Y/Driehoek, maar Y draagt
			// hier de stance — en een bumper is beter voor iets wat je middenin een
			// vuurgevecht doet, want je duim blijft op de rechterstick), X HERLAADT (26-07 avond:
			// X/Vierkant is herladen in Call of Duty, Battlefield, Gears, Division,
			// Borderlands en Destiny — een sterkere claim dan de hergroepeer-order
			// die ik er 's ochtends zelf op zette; die valt terug op X zodra er
			// niets te herladen valt), en LT is geen moduskeuze meer — die is altijd
			// mikken, want een moduskeuze op een analoge trigger druk je half per
			// ongeluk (Division en Gears houden de triggers daarom heilig).
			// "Vorige" heeft daardoor geen padknop meer: RB wrapt rond, en bij vier
			// soldaten is drie keer vooruit hetzelfde als één keer terug.
			//
			// STANCE is de enige die nog nergens buiten de modus werkt, en dat staat
			// er met opzet zo: stance verandert vandaag alleen de HUD-regel, dus er
			// iets anders op zetten zou verbergen dat de stance zelf nog niet af is.
			// Vastgepind door
			// Eclipse.Feel.Input.CommandModeControlsAreSilentOutsideTheMode, die
			// bewaakt dat geen van deze knoppen buiten de modus de Command
			// Mode-toestand aanraakt.
			{ TEXT("Doctrine"),     TEXT("Alt vast bij de order"), TEXT("Y: recon/ready/overwatch/aggressive") }
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

	FEclipseHitMarker MakeHitMarker(bool bHeadshot)
	{
		FEclipseHitMarker Marker;
		Marker.Glyph = bHeadshot ? TEXT("×") : TEXT("+");
		Marker.Colour = bHeadshot ? FLinearColor(1.0f, 0.35f, 0.15f) : FLinearColor::White;
		Marker.Seconds = 0.12f;
		return Marker;
	}
}
