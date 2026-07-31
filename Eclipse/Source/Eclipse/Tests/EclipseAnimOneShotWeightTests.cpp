// DOSSIER "TRILLEN BIJ HET SCHIETEN" — DE METING, NIET DE REPARATIE.
//
// DEBUG_DISCIPLINE §4.2 wijst oorzaak 1 aan als hoofdverdachte ("blendgewicht
// oscilleert — bovenlichaamslaag en aim-offset vechten om dezelfde bones") en
// schrijft voor: Rewind Debugger op de AnimBP, gewicht per frame aflezen.
//
// DIE ROUTE BESTAAT VOOR DE SPELER NIET. AEclipseCharacter::ApplyBodyDefAnimation roept
// SetAnimInstanceClass(UEclipseAnimInstance::StaticClass()) aan — een C++-proxy die
// gewogen poses optelt. Er is geen Layered-Blend-Per-Bone-node, geen aim-offset-node
// en geen graaf om terug te spoelen. §4.2 oorzaak 1 wijst naar een graaf die voor
// de speler niet bestaat.
//
// Dat is winst, geen tegenslag: als functie van zijn argumenten is het gewicht
// volledig headless te bemonsteren. Geen editor, geen skelet, geen owner-klik.
//
// WAT HIER GEMETEN WORDT. EclipseLocomotion::OneShotEnvelope is de curve die ELKE
// eenmalige pose volgt: 0 -> piek -> 0 over de posetijd. Elk schot roept
// PlayOneShotPose aan en die zet de klok terug op nul (EclipseAnimInstance.cpp,
// FEclipseLocomotionProxy::PlayOneShot: OneShotTime = 0.0f). De vraag van dit
// dossier is dus precies: hoe vaak keert dat gewicht van richting om, en loopt dat
// mee met het aantal schoten?
//
// Beide kopieën van de curve — de proxy die de speler ziet en de spiegel op de game
// thread die de testlaag uitleest — roepen sinds de extractie DEZELFDE functie aan.
// Wat hier gemeten wordt, is dus wat het spel doet, en niet een derde curve die
// alleen in een test bestaat.
//
// ER STAAT HIER GEEN FIX. Twee eerdere reparaties zijn teruggedraaid omdat ze vóór
// de meting kwamen (EclipseCharacter.cpp::PlayShootPose). Dit bestand meet.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseLocomotionTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	/** De geleverde spelerconfig: EclipseCharacter.cpp:320 en FEclipseWeaponRow. */
	constexpr float ShootPoseDuration = 0.12f;
	constexpr float ShootPosePeak = 0.85f;
	constexpr float PlayerFireInterval = 0.15f;

	/**
	 * Onder deze verandering noemen we het gewicht "vlak". Ruim boven float-ruis op
	 * waarden van orde 0,85, en ruim onder de kleinste echte stap in de curve — een
	 * drempel die het verschil niet haalt, telt zijn eigen afrondingsfouten mee.
	 */
	constexpr float FlatEpsilon = 1.0e-4f;

	struct FEnvelopeTrace
	{
		TArray<float> Weights;
		int32 ShotsFired = 0;
		int32 LastShotFrame = 0;
	};

	/**
	 * Eén vuurreeks bemonsterd, in exact de volgorde die het spel aanhoudt:
	 * eerst kan er een schot vallen (dat zet de klok op nul), dan loopt de klok een
	 * frame door, dan wordt het gewicht gelezen. Zo doet de proxy het (Update telt
	 * OneShotTime op, Evaluate leest) en zo doet de game thread het
	 * (NativeUpdateAnimation telt OneShotElapsed op en leest daarna).
	 */
	FEnvelopeTrace SampleShotTrain(int32 ShotCount, float FireInterval, float PoseDuration,
		float PeakWeight, float SampleHz)
	{
		FEnvelopeTrace Trace;
		const float Dt = 1.0f / SampleHz;

		// Tot voorbij de staart van de laatste puls, zodat een pose die na het laatste
		// schot nog uitloopt volledig in beeld komt.
		const float TotalTime = (ShotCount - 1) * FireInterval + FMath::Max(PoseDuration, FireInterval) + 3.0f * Dt;
		const int32 FrameCount = FMath::CeilToInt(TotalTime / Dt) + 1;

		// Geen pose actief: een tijd ver voorbij elke duur, zodat de envelope 0 geeft.
		float Elapsed = 1.0e6f;

		for (int32 Frame = 0; Frame < FrameCount; ++Frame)
		{
			const float Time = Frame * Dt;

			// Het schot op het dichtstbijzijnde frame, niet op het eerstvolgende frame
			// ná zijn tijdstip: anders schuift de reeks systematisch op met de
			// bemonsteringsstap en meet je je raster in plaats van het signaal.
			if (Trace.ShotsFired < ShotCount && Time >= Trace.ShotsFired * FireInterval - Dt * 0.5f)
			{
				Elapsed = 0.0f;
				++Trace.ShotsFired;
				Trace.LastShotFrame = Frame;
			}

			Elapsed += Dt;
			Trace.Weights.Add(EclipseLocomotion::OneShotEnvelope(Elapsed, PoseDuration, PeakWeight));
		}

		return Trace;
	}

	/**
	 * Een OMKERING is een wissel van stijgen naar dalen of andersom.
	 *
	 * Vlakke stukken tellen niet mee EN breken de reeks niet: een gewicht dat drie
	 * frames op nul blijft staan is niet drie keer van richting veranderd, en het is
	 * ook niet "opnieuw begonnen" als het daarna stijgt. Zou een vlak stuk de reeks
	 * breken, dan telde deze functie bij een hogere bemonstering vanzelf meer
	 * omkeringen — en dan meet hij het raster.
	 */
	int32 CountReversals(const TArray<float>& Weights)
	{
		int32 Reversals = 0;
		int32 LastSign = 0;
		for (int32 Index = 1; Index < Weights.Num(); ++Index)
		{
			const float Delta = Weights[Index] - Weights[Index - 1];
			if (FMath::Abs(Delta) <= FlatEpsilon)
			{
				continue;
			}
			const int32 Sign = Delta > 0.0f ? 1 : -1;
			if (LastSign != 0 && Sign != LastSign)
			{
				++Reversals;
			}
			LastSign = Sign;
		}
		return Reversals;
	}

	/** Alleen de toppen: stijgen dat omslaat in dalen. Eén per voltooide puls. */
	int32 CountPeaks(const TArray<float>& Weights)
	{
		int32 Peaks = 0;
		int32 LastSign = 0;
		for (int32 Index = 1; Index < Weights.Num(); ++Index)
		{
			const float Delta = Weights[Index] - Weights[Index - 1];
			if (FMath::Abs(Delta) <= FlatEpsilon)
			{
				continue;
			}
			const int32 Sign = Delta > 0.0f ? 1 : -1;
			if (LastSign == 1 && Sign == -1)
			{
				++Peaks;
			}
			LastSign = Sign;
		}
		return Peaks;
	}

	/** Grootste val tussen twee opeenvolgende frames. LET OP: rasterafhankelijk. */
	float LargestDrop(const TArray<float>& Weights)
	{
		float Drop = 0.0f;
		for (int32 Index = 1; Index < Weights.Num(); ++Index)
		{
			Drop = FMath::Max(Drop, Weights[Index - 1] - Weights[Index]);
		}
		return Drop;
	}

	/** Frames waarin het gewicht exact 0 is, vóór het laatste schot. */
	int32 SilentFramesBeforeLastShot(const FEnvelopeTrace& Trace)
	{
		int32 Silent = 0;
		for (int32 Index = 0; Index < Trace.LastShotFrame && Index < Trace.Weights.Num(); ++Index)
		{
			if (Trace.Weights[Index] == 0.0f)
			{
				++Silent;
			}
		}
		return Silent;
	}
}

// ---------------------------------------------------------------------------
// De curve zelf, zodat de extractie vastligt
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseOneShotEnvelopeShapeTest,
	"Eclipse.Characters.Locomotion.OneShotEnvelopeShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseOneShotEnvelopeShapeTest::RunTest(const FString& Parameters)
{
	// Een pose die niet loopt, weegt niets — anders zou een lichaam zonder actieve
	// pose stilletjes een halve schietpose meedragen.
	TestEqual(TEXT("duur 0 weegt niets"), EclipseLocomotion::OneShotEnvelope(0.05f, 0.0f, 0.85f), 0.0f);
	TestEqual(TEXT("negatieve duur weegt niets"), EclipseLocomotion::OneShotEnvelope(0.05f, -1.0f, 0.85f), 0.0f);
	TestEqual(TEXT("afgelopen pose weegt niets"), EclipseLocomotion::OneShotEnvelope(0.12f, 0.12f, 0.85f), 0.0f);
	TestEqual(TEXT("ruim afgelopen pose weegt niets"), EclipseLocomotion::OneShotEnvelope(5.0f, 0.12f, 0.85f), 0.0f);

	// Begin en piek. De piek ligt halverwege de duur, niet op een derde — de
	// commentaarregel in de proxy bewéért een derde, de sinus doet halverwege.
	TestEqual(TEXT("bij t=0 begint de pose op nul"),
		EclipseLocomotion::OneShotEnvelope(0.0f, ShootPoseDuration, ShootPosePeak), 0.0f, 0.001f);
	TestEqual(TEXT("halverwege staat de pose op zijn piek"),
		EclipseLocomotion::OneShotEnvelope(ShootPoseDuration * 0.5f, ShootPoseDuration, ShootPosePeak),
		ShootPosePeak, 0.001f);

	// Symmetrie: even ver voor en na de piek weegt hetzelfde.
	TestEqual(TEXT("de curve is symmetrisch om zijn piek"),
		EclipseLocomotion::OneShotEnvelope(ShootPoseDuration * 0.25f, ShootPoseDuration, ShootPosePeak),
		EclipseLocomotion::OneShotEnvelope(ShootPoseDuration * 0.75f, ShootPoseDuration, ShootPosePeak), 0.001f);

	// De piekklem is een no-op voor beide aanroepers (die klemmen al bij het zetten),
	// maar de functie moet totaal zijn: de testlaag mag hem met alles voeden.
	TestEqual(TEXT("een piek boven 1 wordt geklemd"),
		EclipseLocomotion::OneShotEnvelope(ShootPoseDuration * 0.5f, ShootPoseDuration, 5.0f), 1.0f, 0.001f);
	TestEqual(TEXT("een negatieve piek weegt niets"),
		EclipseLocomotion::OneShotEnvelope(ShootPoseDuration * 0.5f, ShootPoseDuration, -1.0f), 0.0f, 0.001f);

	return true;
}

// ---------------------------------------------------------------------------
// De meting
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseOneShotWeightReversalTest,
	"Eclipse.Characters.Locomotion.OneShotWeightReversalsTrackShotCount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseOneShotWeightReversalTest::RunTest(const FString& Parameters)
{
	// --- 0. CONTROLEPROEF EERST -------------------------------------------------
	//
	// Bewijs dat deze teller iets ANDERS dan "het aantal schoten" kan zeggen, vóór
	// hij het aantal schoten zegt. Een teller die altijd meeloopt met wat je erin
	// stopt, meet niets — en een test die nooit rood kan worden, verbergt evenveel
	// als een bar die altijd rood staat.

	{
		// Vlak signaal: geen pose, geen omkeringen.
		const TArray<float> Flat = []{ TArray<float> A; A.Init(0.0f, 500); return A; }();
		TestEqual(TEXT("controleproef: een vlak gewicht geeft 0 omkeringen"), CountReversals(Flat), 0);
		TestEqual(TEXT("controleproef: een vlak gewicht geeft 0 pieken"), CountPeaks(Flat), 0);
	}

	{
		// ÉÉN doorlopende pose over hetzelfde venster als 28 schoten (27 * 0,15 s).
		// Dit is de vorm die een DOORLOPENDE envelope zou hebben. Als de teller ook
		// hier 27 zou zeggen, meet hij het venster en niet het signaal.
		const float LongWindow = 27.0f * PlayerFireInterval;
		for (const float Hz : { 120.0f, 60.0f })
		{
			const FEnvelopeTrace One = SampleShotTrain(1, LongWindow, LongWindow, ShootPosePeak, Hz);
			AddInfo(FString::Printf(TEXT("controleproef %.0f Hz: EEN pose van %.2f s -> %d pieken, %d omkeringen"),
				Hz, LongWindow, CountPeaks(One.Weights), CountReversals(One.Weights)));
			TestEqual(FString::Printf(TEXT("controleproef %.0f Hz: een doorlopende pose heeft 1 piek"), Hz),
				CountPeaks(One.Weights), 1);
			TestEqual(FString::Printf(TEXT("controleproef %.0f Hz: een doorlopende pose heeft 1 omkering"), Hz),
				CountReversals(One.Weights), 1);
		}
	}

	// --- 1. DE FALSIFICATIE -----------------------------------------------------
	//
	// Loopt het aantal omkeringen 1:1 met het aantal schoten, en blijft dat staan
	// als de bemonstering halveert?
	//
	// TWEE TELLINGEN, want ze zeggen niet hetzelfde:
	//   pieken       stijgen dat omslaat in dalen — één per puls
	//   omkeringen   ELKE richtingwissel, dus ook het dal tussen twee pulsen
	// Een puls die begint en eindigt levert één top; tussen N pulsen liggen N-1
	// dalen. De verwachting is dus pieken = N en omkeringen = 2N-1, en beide moeten
	// onafhankelijk zijn van de bemonstering.

	const int32 ShotCounts[] = { 10, 20, 27 };

	// 77 Hz staat er met opzet naast 120 en 60: die twee zijn allebei een geheel
	// veelvoud van het vuurinterval van 0,15 s (18 respectievelijk 9 frames per
	// schot), dus ze vallen allebei precies op de schoten. Een raster dat NIET op de
	// schoten valt, scheidt "het signaal keert om" van "mijn raster valt samen met
	// de schoten" — zonder die derde meting kunnen de eerste twee het samen eens
	// zijn over een artefact.
	const float SampleRates[] = { 120.0f, 60.0f, 77.0f };

	for (const float Hz : SampleRates)
	{
		for (const int32 Shots : ShotCounts)
		{
			const FEnvelopeTrace Trace = SampleShotTrain(Shots, PlayerFireInterval, ShootPoseDuration,
				ShootPosePeak, Hz);

			const int32 Peaks = CountPeaks(Trace.Weights);
			const int32 Reversals = CountReversals(Trace.Weights);

			AddInfo(FString::Printf(
				TEXT("%.0f Hz: %d schoten -> %d pieken, %d omkeringen (%d frames, grootste val %.3f)"),
				Hz, Trace.ShotsFired, Peaks, Reversals, Trace.Weights.Num(), LargestDrop(Trace.Weights)));

			// De reeks moet ook echt gevallen zijn: een bemonstering die schoten
			// overslaat zou een lage telling geven die als "geen defect" leest.
			TestEqual(FString::Printf(TEXT("%.0f Hz: alle %d schoten zijn gevallen"), Hz, Shots),
				Trace.ShotsFired, Shots);

			TestEqual(FString::Printf(TEXT("%.0f Hz: %d schoten geven %d pieken"), Hz, Shots, Shots),
				Peaks, Shots);

			TestEqual(FString::Printf(TEXT("%.0f Hz: %d schoten geven %d omkeringen (2N-1)"),
					Hz, Shots, 2 * Shots - 1),
				Reversals, 2 * Shots - 1);
		}
	}

	// --- 2. WAT DE HYPOTHESE NIET IS --------------------------------------------
	//
	// De verdenking luidde: "bij een vuurinterval korter dan de posetijd klapt het
	// gewicht bij elke trigger terug naar 0 — een zaagtand". De geleverde
	// spelerconfig is dat regime NIET: de pose duurt 0,12 s en het vuurinterval is
	// 0,15 s, dus elke puls loopt af vóór de volgende begint. Er is een STILTE tussen
	// de schoten, geen afgekapte curve.
	//
	// Dat wordt hier gemeten in plaats van beweerd, en het snelvuur-regime ernaast,
	// zodat de twee verklaringen gescheiden zijn.

	{
		const FEnvelopeTrace Geleverd = SampleShotTrain(27, PlayerFireInterval, ShootPoseDuration,
			ShootPosePeak, 120.0f);
		const int32 Stil = SilentFramesBeforeLastShot(Geleverd);
		AddInfo(FString::Printf(
			TEXT("geleverde config (interval %.2f > posetijd %.2f): %d stille frames voor het laatste schot"),
			PlayerFireInterval, ShootPoseDuration, Stil));
		TestTrue(TEXT("geleverde config: er zitten stille frames tussen de schoten (de puls loopt af)"),
			Stil > 0);

		// Snelvuur: het laagst toegestane vuurinterval in FEclipseWeaponRow
		// (meta ClampMin = 0.05) ligt ruim onder de posetijd. Daar wordt de curve wel
		// afgekapt en is er geen stilte meer.
		const FEnvelopeTrace Snelvuur = SampleShotTrain(27, 0.08f, ShootPoseDuration, ShootPosePeak, 120.0f);
		const int32 StilSnel = SilentFramesBeforeLastShot(Snelvuur);
		AddInfo(FString::Printf(
			TEXT("snelvuur (interval 0.08 < posetijd %.2f): %d stille frames, %d pieken"),
			ShootPoseDuration, StilSnel, CountPeaks(Snelvuur.Weights)));
		TestEqual(TEXT("snelvuur: geen stille frames — de curve wordt midden in de puls afgekapt"),
			StilSnel, 0);

		// EN DIT IS DE KERN: het aantal pieken is in BEIDE regimes gelijk aan het
		// aantal schoten. Het afkappen is dus niet wat de omkeringen maakt — de
		// herstart per schot is dat, en die gebeurt in allebei.
		TestEqual(TEXT("snelvuur geeft evenveel pieken als schoten"), CountPeaks(Snelvuur.Weights), 27);
		TestEqual(TEXT("snelvuur geeft evenveel omkeringen als de geleverde config"),
			CountReversals(Snelvuur.Weights), CountReversals(Geleverd.Weights));
	}

	// --- 3. WAT HIER NIET GEMETEN IS --------------------------------------------
	//
	// De grootste val per frame is GEEN bewijs en staat daarom alleen in de
	// logregels hierboven: hij hangt af van waar het raster toevallig op de steile
	// staart van de sinus valt (gemeten 0,182 bij 120 Hz tegen 0,351 bij 60 Hz voor
	// exact hetzelfde signaal). Een grootheid die met je bemonstering meebeweegt,
	// beschrijft je bemonstering.

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
