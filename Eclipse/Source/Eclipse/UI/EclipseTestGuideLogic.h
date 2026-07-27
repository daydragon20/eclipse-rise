#pragma once

#include "CoreMinimal.h"
#include "UI/EclipseGauntletOverlayLogic.h"

/**
 * Pure core of the in-game test guide (phase0/INGAME_TESTGIDS.md; GDD 14.5 step 4
 * debug tier, 14.3.2 headless rule). The owner picked **variant A**: the guide
 * asks for one thing, notices via Enhanced Input that it happened, ticks it off
 * and moves on. It never locks a control, never consumes input and never edits a
 * binding — a guide built to *find* faults may not be able to trap you in your
 * own game when its own detection is the thing that broke.
 *
 * Three consequences of that decision live in this file:
 *  1. every step carries both device columns AND a visible expectation, so the
 *     tester knows what he should SEE, not only which key to press;
 *  2. every step can also be settled by hand (J = gehaald/goed/ja, N = sla over/
 *     niet goed/nee), so a failing detection costs one keypress, never a session;
 *  3. an unanswered step is never a pass — the same asymmetry the gauntlet
 *     verdict block enforces.
 *
 * No engine-actor headers: the widget owns pixels and input, every step, state
 * transition and summary line is decided here so the headless tier can sweep it
 * without a world. Deliberately NOT built: variant B (`Eclipse.Guide.Strict`,
 * input locking) — the BESLOTEN block forbids it until A proves its detection.
 */
namespace EclipseTestGuide
{
	/** The three parts of the guide, in the order the panel stacks them. */
	enum class EEclipseGuidePart : uint8
	{
		/**
		 * Deel 1 — WAT ER VERANDERD IS sinds de vorige sessie (26-07 avond).
		 *
		 * Hier stonden veertien controlstappen. Die zijn eruit langs de owner-regel:
		 * *de gids mag alleen bevatten wat ik NIET kan meten.* Dat een binding
		 * bestaat wordt getest, en het effect is gemeten — snelheden, springhoogte,
		 * FOV, coyote-venster. Zeventien van de drieëntwintig stappen waren dingen
		 * die ik zelf had moeten vaststellen.
		 *
		 * Wat blijft is niet "leer de besturing" maar "welke knop betekent sinds
		 * jouw laatste sessie iets anders". Dat is geen meting: ik kan meten dát RB
		 * van wapen wisselt, niet of de owner dat weet. Staat er niets in, dan is
		 * dat het antwoord — en dat hoort er ook te staan.
		 */
		Controls,

		/** Deel 2 — systems the tester judges himself (a measurement cannot answer these). */
		Systems,

		/** Deel 3 — the five GDD 13.2 questions, gate question last. */
		Questions
	};

	/**
	 * DE SIGNAAL-ENUM IS WEG (26-07 avond). Hij vertelde de gids welke invoeractie
	 * er binnenkwam, zodat een stap zichzelf kon afvinken. Sinds de gids alleen
	 * nog oordelen bevat, valt er niets meer te detecteren — een oordeel geeft geen
	 * invoergebeurtenis af.
	 */


	/** How a step was settled; Pending is neither pass nor fail (14.5's rule). */
	enum class EEclipseGuideStepState : uint8
	{
		Pending,

		/** The tester settled it himself: J — "gehaald" / "goed" / "ja". */
		Confirmed,

		/** N op een wijzigingsregel: gelezen en verder. */
		Skipped,

		/** N on a judgement step: a real negative answer — "niet goed" / "nee". */
		Rejected
	};

	/**
	 * Deel 1 mirrors EclipseGauntletOverlay::GetControlRows() one-for-one, index
	 * by index: one control table, two readers. GetGuideSteps() degrades loudly
	 * (empty expectation, no signal) if that table ever grows, and the test asserts
	 * the counts still match.
	 */
	/**
	 * Wijzigingen sinds de vorige sessie. Geen mirror van de controletabel meer —
	 * zie de toelichting bij EEclipseGuidePart::Controls.
	 */
	ECLIPSE_API int32 GetChangeStepCount();

	/**
	 * Deel 2 — alleen wat een meting NIET kan beantwoorden.
	 *
	 * Was vier. Drie eruit: "squad doet wat je vroeg" (Eclipse.Squad.* meet het
	 * antwoord én de beweging), "objective vinkt af" en "debrief betaalt" (allebei
	 * assert in M11GauntletOnShippedData). Wat blijft is de order-reactie, en
	 * juist omdat de meting daar altijd ~0 s zegt — vraag en antwoord vallen in
	 * hetzelfde frame — en daarmee niets bewijst.
	 *
	 * Sinds 26-07 avond een tweede: de demper. 1200 tegen 2500 cm is precies het
	 * soort verschil dat op papier niets zegt.
	 */
	inline constexpr int32 SystemStepCount = 2;

	/** Deel 3 is the 13.2 checklist itself, so it can never drift from the playtest block. */
	inline constexpr int32 QuestionStepCount = EclipseGauntletOverlay::PlaytestQuestionCount;

	/** Runtime, want het aantal wijzigingen hangt af van wanneer je voor het laatst speelde. */
	ECLIPSE_API int32 GetGuideStepCount();

	/** Header + one row per step + tally. Fixed, so the panel builds its rows once and afterwards only ever SetText (GDD 12.4: no widget churn in a firefight). */
	/**
	 * Hoeveel wijzigingen deel 1 hoogstens TOONT — niet hoeveel de tabel er mag
	 * bevatten.
	 *
	 * Bij de vorige regel stond "drie wijzigingen is het meeste wat één sessie
	 * oplevert", en ik heb die aanname op 27-07 weersproken omdat die dag er vijf
	 * opleverde. DAT WAS EEN VERKEERDE LEZING VAN MIJN EIGEN GETAL: de 3 is geen
	 * gok over hoeveel een dag brengt maar het GEVOLG van de korte-gids-regel van
	 * de owner. Twee systeemstappen plus vijf 13.2-vragen is zeven, en de gids mag
	 * er tien; dan blijven er drie over. De suite bewaakt dat ook echt — mijn
	 * poging om er vijf bij te zetten viel meteen rood met "de gids is kort
	 * gebleven (15 stappen)". Precies waarvoor die test bestaat.
	 *
	 * De tabel groeit dus door als geschiedenis, en deel 1 toont de NIEUWSTE drie
	 * die de speler nog niet gezien heeft. Wat afvalt is niet weg, alleen niet in
	 * beeld — en het staat met de hand in het kliklijstje.
	 */
	inline constexpr int32 MaxChangeStepCount = 3;

	/** Kop + de look-waardenregel + elke stap + de tellerregel. */
	inline constexpr int32 GuidePanelLineCount = MaxChangeStepCount + SystemStepCount + EclipseGauntletOverlay::PlaytestQuestionCount + 3;

	/** One step: what to do, on both devices, and what you must SEE if it works. */
	struct FEclipseGuideStep
	{
		EEclipseGuidePart Part = EEclipseGuidePart::Controls;

		FString Label;

		/** Mouse + keyboard cell — never empty (the test enforces it). */
		FString MouseKeyboard;

		/** Controller cell — never empty; says "toetsenbord nodig" where the pad genuinely cannot do it. */
		FString Controller;

		/** The visible expectation: what proves it worked. Never empty. */
		FString Expectation;

	};

	/**
	 * The whole stappenlijst. Allocates, so the panel calls it on a refresh and
	 * never on an input event.
	 */
	ECLIPSE_API TArray<FEclipseGuideStep> GetGuideSteps();

	/** Detection signal of one step; None for out-of-range and for the judgement steps. */

	ECLIPSE_API EEclipseGuidePart GetPartOfStep(int32 StepIndex);

	/** 0-based position of the step inside its own part (the panel counts "stap 3/11"). */
	ECLIPSE_API int32 GetIndexWithinPart(int32 StepIndex);

	ECLIPSE_API int32 GetPartStepCount(EEclipseGuidePart Part);

	/**
	 * The 13.2 answer row a deel-3 step feeds, or INDEX_NONE. Answering a guide
	 * question and answering the playtest panel are the same act — the guide may
	 * not open a second set of books on the owner's gate question.
	 */
	ECLIPSE_API int32 GetPlaytestQuestionIndex(int32 StepIndex);

	/**
	 * Live position in the list. Strictly sequential: a signal only ever settles
	 * the ACTIVE step. The guide asks one thing at a time, so a player sprinting
	 * past step 1 may not silently evaporate steps 3-11 he never looked at.
	 */
	struct ECLIPSE_API FEclipseGuideProgress
	{
		FEclipseGuideProgress();

		/** Back to step 1, everything Pending (a fresh mount is a fresh run). */
		void Reset();

		const TArray<EEclipseGuideStepState>& GetStates() const { return States; }

		/** First unsettled step, or INDEX_NONE when the guide is done. */
		int32 GetActiveIndex() const { return ActiveIndex; }

		bool IsComplete() const { return ActiveIndex == INDEX_NONE; }

		/** True once anything at all has been settled — "nothing done" writes no summary. */
		bool HasAnyProgress() const;

		/** Enhanced Input reported an action. Returns true only when it settled the active step. */

		/** J — gehaald / goed / ja. Returns true when a step moved. */
		bool ConfirmActive();

		/** N — sla over (control) / niet goed / nee (judgement). Returns true when a step moved. */
		bool RejectActive();

		int32 CountInState(EEclipseGuideStepState State) const;

	private:
		void AdvanceActive();

		TArray<EEclipseGuideStepState> States;
		int32 ActiveIndex = 0;
	};

	/**
	 * The panel, exactly GuidePanelLineCount lines (padded), so the widget's
	 * pre-built rows map 1:1 and a refresh is pure SetText. Settled steps collapse
	 * to a marker + label; the active step shows both device cells AND its
	 * expectation, which is the whole point of §2 of the spec.
	 *
	 * OrderRoundTripFact is the gauntlet's wall-clock measurement, already
	 * formatted ("8/10 binnen 1.00 s") — deel 2's responsiveness row shows it next
	 * to the tester's judgement. Empty = not measured yet, and it then says so
	 * rather than implying a number.
	 */
	/** LookSummary is the tuned look values, already formatted. Owner request: with
	 *  them on screen he can say "te snel" or "te traag" about a specific number
	 *  instead of "voelt raar". Empty = the line stays blank, never invented. */
	ECLIPSE_API TArray<FString> ComposeGuidePanelLines(const FEclipseGuideProgress& Progress, const FString& OrderRoundTripFact = FString(), const FString& LookSummary = FString());

	/**
	 * The archive section, appended under the R3 verdict block in the SAME
	 * Saved/Logs file the gauntlet already writes — one summary mechanism, one
	 * artefact. Every step appears with how it was settled, so "detected" and
	 * "the tester had to confirm it by hand" stay distinguishable: the second one
	 * is a detection bug worth reading back.
	 */
	ECLIPSE_API TArray<FString> ComposeGuideSummaryBlock(const FEclipseGuideProgress& Progress);
}
