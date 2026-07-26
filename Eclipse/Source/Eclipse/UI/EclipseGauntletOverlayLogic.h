#pragma once

#include "CoreMinimal.h"

/**
 * Pure core of the feel-gauntlet overlay (phase0/FEEL_GAUNTLET_P2-02.md; GDD
 * 14.5 step 4 debug tier, 14.3.2 headless rule). No engine-actor headers: this
 * file turns the five R3 criteria into the owner's copy-paste verdict block and
 * holds the control table the overview panel draws. The HUD widget owns pixels
 * and input; every pass/fail/not-measured *decision* lives here so the headless
 * tier can sweep it without a world.
 *
 * Deliberate asymmetry: an unmeasured criterion is never a pass. 14.5's rule
 * ("polish may only make a true better, never fabricate a verdict") applies to
 * missing telemetry too — a blank row must read as blank, not as green.
 */
namespace EclipseGauntletOverlay
{
	/** Manual answer for criteria 2-4 and the 13.2 playtest rows. */
	enum class EEclipseGauntletAnswer : uint8
	{
		Unanswered,
		Good,
		Bad
	};

	/** Per-criterion verdict; Open = not measured yet (neither pass nor fail). */
	enum class EEclipseGauntletStatus : uint8
	{
		Open,
		Pass,
		Fail
	};

	/** The five R3 criteria of the Stage A gauntlet, in the owner's order. */
	inline constexpr int32 CriterionCount = 5;

	/** Title + one row per criterion + tally. Fixed so the panel pre-builds its rows and then only ever SetText (no per-fact widget churn in a firefight). */
	inline constexpr int32 VerdictLineCount = CriterionCount + 2;

	/** The 13.2 gate questions (playtest panel), gate question last. */
	inline constexpr int32 PlaytestQuestionCount = 5;

	/**
	 * Everything the five criteria are judged on, gathered from the layers that
	 * own the facts: criteria 1 from the squad layer (order -> ack/refusal),
	 * criterion 5 from the Command Mode component (ModeEntered per encounter
	 * beat), criteria 2-4 from the tester's keys. The overlay only copies.
	 */
	struct FEclipseGauntletCriteria
	{
		/** Criterion 1 — wall-clock order round trips recorded by UEclipseSquadSubsystem. */
		int32 RoundTripSamples = 0;
		int32 RoundTripWithinBar = 0;
		double RoundTripWorstSeconds = 0.0;
		double RoundTripBarSeconds = 1.0;

		/** Sample count the criterion asks for before it may pass ("10 orders achter elkaar"). */
		int32 RoundTripRequiredSamples = 10;

		/** Criterion 2 — targeting readability, counted by hand (clean pick vs mis-pick). */
		int32 CleanPicks = 0;
		int32 MisPicks = 0;
		int32 TargetingRequiredAttempts = 10;
		int32 TargetingRequiredClean = 9;

		/** Criteria 3 and 4 — dilation comfort and trust under dilation. */
		EEclipseGauntletAnswer Comfort = EEclipseGauntletAnswer::Unanswered;
		EEclipseGauntletAnswer Confidence = EEclipseGauntletAnswer::Unanswered;

		/** Criterion 5 — usage pull, from the component's encounter-beat tally. */
		int32 EntriesThisBeat = 0;
		int32 ClosedBeatCount = 0;
		float AverageEntriesPerBeat = 0.0f;
	};

	/** The composed block plus the machine-readable status the panel colours rows by. */
	struct FEclipseGauntletVerdict
	{
		/** Exactly VerdictLineCount lines, ready to paste into the owner's verdict message. */
		TArray<FString> Lines;

		/** CriterionCount entries, index = criterion number - 1. */
		TArray<EEclipseGauntletStatus> Statuses;

		int32 FailedCount = 0;
		int32 OpenCount = 0;
	};

	/**
	 * Judge one criterion (index 0-4). Rules, in the strict reading of the
	 * gauntlet draaiboek:
	 *  1 round trip — Fail the moment any answer misses the wall-clock bar;
	 *    Pass only once the asked-for sample count is in and all of it is inside.
	 *  2 targeting — Fail as soon as more mis-picks happened than the target
	 *    tolerates (the verdict is already false, no need to finish the ten);
	 *    Pass at the full attempt count with enough clean picks.
	 *  3/4 comfort + trust — the tester's word: Good passes, Bad fails.
	 *  5 usage pull — judged on closed beats only: an unused mode is a failed
	 *    mode, so an average below one entry per beat fails.
	 */
	ECLIPSE_API EEclipseGauntletStatus EvaluateCriterion(int32 CriterionIndex, const FEclipseGauntletCriteria& Criteria);

	/**
	 * Compose the R3-VERDICT INPUT block: title, five criterion rows (failures
	 * marked, unmeasured rows marked as such), and the "-> N criteria gefaald"
	 * tally. Same text for the on-screen panel, the log and the Saved/Logs file,
	 * so what the owner reads is exactly what gets archived.
	 */
	ECLIPSE_API FEclipseGauntletVerdict ComposeVerdict(const FEclipseGauntletCriteria& Criteria);

	/**
	 * IN WELKE MODUS EEN KNOP IETS BETEKENT (owner-punt 7).
	 *
	 * De controletabel hieronder heeft één rij per handeling, en de modus zit
	 * daar als PROZA in de cel verstopt: "RB (in CM; erbuiten: WAPENWISSEL)".
	 * Zo'n cel leest prima en is niet te controleren — en juist dáár ging het
	 * mis. LT was buiten Command Mode mikken en erbinnen "vorige soldaat", en
	 * niets kon dat zien behalve een mens die de zin las.
	 *
	 * Eén knop mag twee dingen doen; dat is hier bewust ontwerp, want de speler
	 * houdt LB ingedrukt op het moment dat hij orders geeft en weet dus in welke
	 * van de twee hij zit. Wat NIET mag is twee betekenissen BINNEN dezelfde
	 * modus, want dan bestaat er geen moment waarop de speler weet welke geldt.
	 *
	 * Die regel is pas te bewaken als de modus een veld is in plaats van een zin.
	 */
	enum class EEclipseControlMode : uint8
	{
		/** Gewoon lopen en vechten — Command Mode NIET ingedrukt. */
		OnFoot,
		/** Zolang Q/LB ingedrukt blijft. */
		CommandMode,
	};

	/** Eén knop in één modus. Een leeg apparaatveld betekent: die knop bestaat hier niet. */
	struct FEclipseBinding
	{
		EEclipseControlMode Mode = EEclipseControlMode::OnFoot;
		const TCHAR* Action = nullptr;
		const TCHAR* Key = nullptr;
		const TCHAR* Pad = nullptr;
	};

	/**
	 * Het knoppenschema per modus. Bron voor de controle hieronder, en bedoeld om
	 * de bron te worden van BESTURING.md, de F3-gids en de startbat — die vier
	 * liepen 26-07 vier keer op één dag uit elkaar.
	 */
	ECLIPSE_API TArray<FEclipseBinding> GetBindings();

	/** Leesbare naam van een modus, voor testmeldingen en documentatie. */
	ECLIPSE_API const TCHAR* ModeName(EEclipseControlMode Mode);

	/** One row of the control overview: the action and its two device cells. */
	struct FEclipseControlRow
	{
		const TCHAR* Action = nullptr;
		const TCHAR* MouseKeyboard = nullptr;
		const TCHAR* Controller = nullptr;
	};

	/**
	 * The control overview (F2). Mirrors the live bindings in
	 * AEclipsePlayerController::SetupInputComponent — when a binding moves, this
	 * table moves with it, and the test asserts no cell is ever left blank.
	 */
	ECLIPSE_API TArray<FEclipseControlRow> GetControlRows();

	/** The five 13.2 playtest statements (H panel); the gate question is last. */
	ECLIPSE_API TArray<FString> GetPlaytestQuestions();

	/**
	 * Playtest answers as an archive section appended below the verdict block —
	 * labelled separately so the copy-paste block above it stays untouched.
	 */
	ECLIPSE_API TArray<FString> ComposePlaytestBlock(const TArray<EEclipseGauntletAnswer>& Answers);

	/** Unanswered -> Good -> Bad -> Unanswered: one key per row, no text entry (owner request). */
	ECLIPSE_API EEclipseGauntletAnswer CycleAnswer(EEclipseGauntletAnswer Answer);

	/**
	 * Hoe de trefferbevestiging eruitziet (owner-opdracht 26-07 avond, punt 2).
	 *
	 * Puur, zodat het toetsbaar is zonder viewport: de widget-laag die dit tekent
	 * heeft geen enkele test, en juist dáár zaten vandaag twee bugs. Wat hier
	 * beslist wordt — welk teken, welke kleur, hoe lang — is het deel dat stil kan
	 * veranderen; het tekenen zelf is één SetText.
	 */
	struct FEclipseHitMarker
	{
		FString Glyph;
		FLinearColor Colour = FLinearColor::White;
		float Seconds = 0.0f;
	};

	/**
	 * VORM én kleur verschillen bij een kopschot, niet alleen kleur: kleur alleen
	 * is voor een deel van de spelers geen onderscheid, en een × leest ook in je
	 * ooghoek anders dan een +.
	 *
	 * De duur is korter dan het vuurinterval (0,15 s), zodat aanhoudend vuur een
	 * KNIPPERENDE marker geeft. Een marker die blijft staan meldt dat er ooit een
	 * treffer was; een die knippert telt ze.
	 */
	ECLIPSE_API FEclipseHitMarker MakeHitMarker(bool bHeadshot);
}
