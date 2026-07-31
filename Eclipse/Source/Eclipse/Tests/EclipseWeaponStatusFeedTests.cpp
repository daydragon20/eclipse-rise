// De datalaag ONDER de schermlaag voor het WAPEN: Event.Player.WeaponStatusChanged
// (N-a; bouwvolgorde GDD 14.5 — schema, pure logica + tests, bedrading + events,
// pas daarna iets zichtbaars). De tweede helft van dezelfde laag als
// EclipseVitalsFeedTests, en met opzet in dezelfde vorm.
//
// De vier vragen uit het plan, en de vierde is de belangrijkste:
//
//   1. 30 schoten = 30 feiten, met een AFLOPEND magazijn.
//   2. Herladen = start + MONOTONE voortgang + eind.
//   3. Een wapenwissel = PRECIES ÉÉN feit, met de nieuwe naam erin.
//   4. Een frame zonder verandering = NIETS.
//
// Nummer 4 is de makkelijkste om per ongeluk te laten slagen: een feed die
// helemaal niets doet haalt hem ook. Daarom staat bij ELKE nul-meting hieronder
// eerst de CONTROLEPROEF die bewijst dat de teller op diezelfde opstelling wél kan
// bewegen. Een teller van 0 kan "kapot" en "nooit geprobeerd" niet scheiden, en
// dat onderscheid is in dit project al vaker geld gaan kosten dan wat ook.
//
// En daarom staat er onderaan een test op het feel-harnas. De vier hierboven
// kunnen alle vier groen zijn terwijl er in het spel niets gebeurt: pure logica
// die niemand aanroept is perfecte logica. Die laatste draait op een echte wereld,
// een echte AEclipseCharacter met een echte AEclipsePlayerController, een echt
// UEclipseHitscanWeaponComponent en de echte UEclipseEventBusSubsystem — en telt
// wat er werkelijk op de bus landt.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Combat/EclipseWeaponStatusFeed.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Misc/AutomationTest.h"
#include "Tests/EclipseFeelHarness.h"

namespace EclipseWeaponStatusFeedTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/**
	 * Het profiel waarop gemeten wordt — de AR-rij uit DT_Weapons.
	 *
	 * De getallen zijn overgenomen uit `Tools/create_phase1_content.py` (magazijn 30,
	 * herladen 2,2 s, vuurinterval 0,15 s) en niet verzonnen: de falsificatie zegt
	 * "30 schoten", en dat getal is alleen betekenisvol als het het magazijn van een
	 * wapen is dat werkelijk in het spel zit.
	 */
	EclipseWeaponStatusFeed::FEclipseWeaponSnapshot FullMagazine()
	{
		EclipseWeaponStatusFeed::FEclipseWeaponSnapshot Snapshot;
		Snapshot.AmmoInMagazine = 30;
		Snapshot.MagazineSize = 30;
		Snapshot.ReloadSecondsTotal = 2.2f;
		Snapshot.WeaponRowName = FName(TEXT("AR_Foundry"));
		Snapshot.WeaponDisplayName = FText::FromString(TEXT("Foundry AR"));
		Snapshot.SlotCount = 2;
		return Snapshot;
	}

	FEclipseWeaponRow TestRow(int32 MagazineSize)
	{
		FEclipseWeaponRow Row;
		Row.MagazineSize = MagazineSize;
		Row.ReloadSeconds = 2.2f;
		Row.FireInterval = 0.15f;
		// Nul: de handling-tijd is een echt kenmerk, maar in deze meting zou hij
		// alleen betekenen dat de eerste schoten na een wissel geweigerd worden en
		// dat de test iets anders telt dan hij denkt te tellen.
		Row.ReadySeconds = 0.0f;
		Row.RangeCm = 5000.0f;
		return Row;
	}
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 4 (eerst, want het is de zwakste claim) — 60 tikken zonder
// verandering = 0 uitzendingen.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponStatusFeedSilentWhenUnchangedTest,
	"Eclipse.Combat.WeaponStatusFeed.SilentWhenUnchanged",
	EclipseWeaponStatusFeedTest::TestFlags)

bool FEclipseWeaponStatusFeedSilentWhenUnchangedTest::RunTest(const FString& Parameters)
{
	EclipseWeaponStatusFeed::FEclipseWeaponStatusTracker Tracker;
	const EclipseWeaponStatusFeed::FEclipseWeaponSnapshot Snapshot = EclipseWeaponStatusFeedTest::FullMagazine();

	// CONTROLEPROEF EERST. Zonder deze regel zou "0 uitzendingen" ook waar zijn voor
	// een tracker die helemaal niets doet, en die twee verklaringen kan een teller
	// van 0 niet scheiden. Het eerste monster MOET vertrekken: de teller heeft een
	// beginwaarde nodig, anders staat er "0 / 0" tot het eerste schot valt.
	const EclipseWeaponStatusFeed::FEclipseWeaponStatusDecision First = Tracker.Submit(Snapshot);
	TestTrue(TEXT("Controleproef: het EERSTE monster gaat wel degelijk de deur uit"), First.bShouldBroadcast);
	TestTrue(TEXT("Het eerste feit is gemarkeerd als beginwaarde, niet als verandering"), First.Payload.bInitial);
	TestFalse(TEXT("Een beginwaarde is geen schot"), First.Payload.bAmmoChanged);
	TestFalse(TEXT("Een beginwaarde is geen wapenwissel"), First.Payload.bWeaponChanged);
	TestFalse(TEXT("Een beginwaarde is geen herlaadbeurt"), First.Payload.bReloadStateChanged);
	TestEqual(TEXT("Een vol magazijn draagt zijn eigen getal als 'vorige'"), First.Payload.PreviousAmmoInMagazine, 30);
	TestEqual(TEXT("Na de beginwaarde: 1 uitzending"), Tracker.GetBroadcastCount(), 1);

	// En dan zestig tikken lang exact hetzelfde wapen aanbieden.
	for (int32 Tick = 0; Tick < 60; ++Tick)
	{
		const EclipseWeaponStatusFeed::FEclipseWeaponStatusDecision Decision = Tracker.Submit(Snapshot);
		if (Decision.bShouldBroadcast)
		{
			AddError(FString::Printf(TEXT("Tik %d zond een feit uit terwijl er niets veranderde — de feed spamt de bus."), Tick));
			break;
		}
	}
	TestEqual(TEXT("60 tikken zonder verandering voegen NIETS toe aan de teller"), Tracker.GetBroadcastCount(), 1);

	// Tweede controleproef, ná de stilte: de tracker is niet dichtgeslagen maar
	// gewoon stil. Een feed die na 60 tikken permanent zwijgt zou het bovenstaande
	// ook halen, en dat is precies de vorm van vals groen waar dit tegen bedoeld is.
	EclipseWeaponStatusFeed::FEclipseWeaponSnapshot OneShotFired = Snapshot;
	OneShotFired.AmmoInMagazine = 29;
	TestTrue(TEXT("Controleproef: na de stilte komt een echt schot er nog steeds door"), Tracker.Submit(OneShotFired).bShouldBroadcast);
	TestEqual(TEXT("En dan pas staat de teller op 2"), Tracker.GetBroadcastCount(), 2);

	return true;
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 1 — 30 schoten = 30 feiten, met een aflopend magazijn.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponStatusFeedThirtyShotsThirtyFactsTest,
	"Eclipse.Combat.WeaponStatusFeed.ThirtyShotsThirtyFacts",
	EclipseWeaponStatusFeedTest::TestFlags)

bool FEclipseWeaponStatusFeedThirtyShotsThirtyFactsTest::RunTest(const FString& Parameters)
{
	EclipseWeaponStatusFeed::FEclipseWeaponStatusTracker Tracker;
	EclipseWeaponStatusFeed::FEclipseWeaponSnapshot Snapshot = EclipseWeaponStatusFeedTest::FullMagazine();
	Tracker.Submit(Snapshot); // de beginwaarde; die telt niet als schot
	const int32 CountAfterSeed = Tracker.GetBroadcastCount();

	TArray<int32> Magazines;
	TArray<int32> PreviousMagazines;
	int32 EmptyFlagged = 0;
	for (int32 Shot = 0; Shot < 30; ++Shot)
	{
		--Snapshot.AmmoInMagazine;
		const EclipseWeaponStatusFeed::FEclipseWeaponStatusDecision Decision = Tracker.Submit(Snapshot);
		if (!Decision.bShouldBroadcast)
		{
			AddError(FString::Printf(TEXT("Schot %d leverde GEEN feit op — de teller op het scherm zou blijven staan."), Shot + 1));
			continue;
		}
		Magazines.Add(Decision.Payload.AmmoInMagazine);
		PreviousMagazines.Add(Decision.Payload.PreviousAmmoInMagazine);
		if (Decision.Payload.bEmpty)
		{
			++EmptyFlagged;
		}
		if (!Decision.Payload.bAmmoChanged)
		{
			AddError(FString::Printf(TEXT("Schot %d was niet als munitieverandering gemarkeerd."), Shot + 1));
		}
	}

	EclipseFeelHarness::Report(*this, TEXT("feiten uit 30 schoten"), Magazines.Num(), TEXT("feiten"));
	TestEqual(TEXT("30 schoten leveren precies 30 feiten op"), Tracker.GetBroadcastCount() - CountAfterSeed, 30);
	if (Magazines.Num() != 30)
	{
		return false;
	}

	// AFLOPEND, en niet alleen "dertig stuks". Dertig feiten in de verkeerde
	// volgorde zouden dezelfde teller opleveren en een scherm dat springt.
	TestEqual(TEXT("Feit 1 meldt 29 kogels"), Magazines[0], 29);
	TestEqual(TEXT("...en weet dat het er 30 waren"), PreviousMagazines[0], 30);
	TestEqual(TEXT("Feit 30 meldt 0 kogels"), Magazines[29], 0);
	TestEqual(TEXT("...en weet dat het er 1 was"), PreviousMagazines[29], 1);
	for (int32 Index = 1; Index < Magazines.Num(); ++Index)
	{
		if (Magazines[Index] != Magazines[Index - 1] - 1)
		{
			AddError(FString::Printf(TEXT("Het magazijn liep niet af: feit %d meldt %d na %d."),
				Index + 1, Magazines[Index], Magazines[Index - 1]));
			break;
		}
		if (PreviousMagazines[Index] != Magazines[Index - 1])
		{
			AddError(FString::Printf(TEXT("De ketting brak: feit %d zegt 'vorige = %d' terwijl feit %d %d meldde."),
				Index + 1, PreviousMagazines[Index], Index, Magazines[Index - 1]));
			break;
		}
	}

	// LEEG IS PRECIES ÉÉN VAN DE DERTIG. De vlag hoort bij de OMSLAG te horen, niet
	// bij elke lage stand — anders zou de HUD hem nooit als gebeurtenis kunnen
	// gebruiken.
	TestEqual(TEXT("Precies één van de dertig feiten meldt een leeg magazijn"), EmptyFlagged, 1);

	// En het 31e schot bestaat niet: het magazijn is op, er verandert niets meer, en
	// dus zwijgt de feed. Dit is de silence-claim op de plek waar hij het makkelijkst
	// per ongeluk zou breken.
	const EclipseWeaponStatusFeed::FEclipseWeaponStatusDecision NoShot = Tracker.Submit(Snapshot);
	TestFalse(TEXT("Een leeg magazijn nogmaals aanbieden zendt niets"), NoShot.bShouldBroadcast);

	return true;
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 2 — herladen = start + MONOTONE voortgang + eind.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponStatusFeedReloadStartProgressEndTest,
	"Eclipse.Combat.WeaponStatusFeed.ReloadStartProgressEnd",
	EclipseWeaponStatusFeedTest::TestFlags)

bool FEclipseWeaponStatusFeedReloadStartProgressEndTest::RunTest(const FString& Parameters)
{
	using namespace EclipseWeaponStatusFeed;

	FEclipseWeaponStatusTracker Tracker;
	FEclipseWeaponSnapshot Snapshot = EclipseWeaponStatusFeedTest::FullMagazine();
	Snapshot.AmmoInMagazine = 3; // een halfleeg wapen; vol herladen mag niet eens
	Tracker.Submit(Snapshot);

	// --- START -----------------------------------------------------------------
	constexpr float ReloadSeconds = 2.2f;
	Snapshot.bReloading = true;
	Snapshot.ReloadProgress = 0.0f;
	Snapshot.ReloadSecondsRemaining = ReloadSeconds;
	const FEclipseWeaponStatusDecision Start = Tracker.Submit(Snapshot);
	TestTrue(TEXT("De START van een beurt is een feit"), Start.bShouldBroadcast);
	TestTrue(TEXT("...gemarkeerd als toestandsomslag"), Start.Payload.bReloadStateChanged);
	TestTrue(TEXT("...met bReloading aan, zodat 'start' en 'eind' uit elkaar te houden zijn"), Start.Payload.bReloading);
	TestFalse(TEXT("...en NIET als munitieverandering: herladen begint, er valt nog niets in"), Start.Payload.bAmmoChanged);
	TestEqual(TEXT("De volle duur reist mee, zodat de schermlaag kan interpoleren"), Start.Payload.ReloadSecondsTotal, ReloadSeconds);

	// --- VOORTGANG, op een klok van 60 fps -------------------------------------
	// 132 monsters over 2,2 s. De vraag is dubbel: komt er genoeg door om een balk
	// te laten lopen, en blijft het ver onder één-per-frame?
	TArray<float> Progress;
	int32 SilentSamples = 0;
	constexpr int32 SampleCount = 132;
	for (int32 Sample = 1; Sample <= SampleCount; ++Sample)
	{
		const float Elapsed = (Sample / static_cast<float>(SampleCount)) * ReloadSeconds;
		Snapshot.ReloadProgress = Elapsed / ReloadSeconds;
		Snapshot.ReloadSecondsRemaining = ReloadSeconds - Elapsed;
		const FEclipseWeaponStatusDecision Decision = Tracker.Submit(Snapshot);
		if (!Decision.bShouldBroadcast)
		{
			++SilentSamples;
			continue;
		}
		if (!Decision.Payload.bReloadProgressed)
		{
			AddError(FString::Printf(TEXT("Monster %d leverde een feit dat NIET als voortgang gemarkeerd was."), Sample));
		}
		Progress.Add(Decision.Payload.ReloadProgress);
	}

	// De bovengrens is AFGELEID en niet gegokt: over een hele beurt (1,0) past er bij
	// een drempel van ReloadProgressEpsilon hooguit 1/epsilon stappen in. Een grens
	// uit de duim meet niets — die zegt alleen iets over mijn rekenwerk.
	const int32 TheoreticalMax = FMath::CeilToInt(1.0f / ReloadProgressEpsilon);
	EclipseFeelHarness::Report(*this, TEXT("voortgangsfeiten uit 132 monsters"), Progress.Num(), TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("monsters die NIETS opleverden"), SilentSamples, TEXT("monsters"));
	TestTrue(TEXT("Er komt voortgang door — de balk staat niet stil"), Progress.Num() > 0);
	TestTrue(FString::Printf(TEXT("...en nooit meer dan 1/drempel (%d)"), TheoreticalMax), Progress.Num() <= TheoreticalMax);
	TestTrue(TEXT("...en het overgrote deel van de frames levert NIETS op (geen per-frame verkeer)"),
		SilentSamples > SampleCount / 2);

	// MONOTOON. Een balk die terugspringt is erger dan geen balk: hij zegt dat je
	// verder van schieten af bent dan een moment eerder.
	for (int32 Index = 1; Index < Progress.Num(); ++Index)
	{
		if (Progress[Index] <= Progress[Index - 1])
		{
			AddError(FString::Printf(TEXT("De voortgang liep niet monotoon op: %.3f na %.3f (feit %d)."),
				Progress[Index], Progress[Index - 1], Index + 1));
			break;
		}
	}

	// --- EIND ------------------------------------------------------------------
	Snapshot.bReloading = false;
	Snapshot.ReloadProgress = 0.0f;
	Snapshot.ReloadSecondsRemaining = 0.0f;
	Snapshot.AmmoInMagazine = Snapshot.MagazineSize;
	const FEclipseWeaponStatusDecision End = Tracker.Submit(Snapshot);
	TestTrue(TEXT("Het EIND van de beurt is een feit"), End.bShouldBroadcast);
	TestTrue(TEXT("...gemarkeerd als toestandsomslag"), End.Payload.bReloadStateChanged);
	TestFalse(TEXT("...met bReloading uit"), End.Payload.bReloading);
	TestFalse(TEXT("...en NIET als 'voortgang': de balk verdwijnt, hij springt niet terug"), End.Payload.bReloadProgressed);
	TestEqual(TEXT("De voortgang staat op 0 buiten een beurt, niet op de laatst gemeten 0,97"), End.Payload.ReloadProgress, 0.0f);
	TestTrue(TEXT("Het volle magazijn reist mee in hetzelfde feit"), End.Payload.bAmmoChanged);
	TestEqual(TEXT("...en staat op 30"), End.Payload.AmmoInMagazine, 30);
	TestEqual(TEXT("...met 3 als vorige waarde: het teken van dat verschil is 'gevuld', niet 'geschoten'"),
		End.Payload.PreviousAmmoInMagazine, 3);

	// Ná de beurt is het weer stil. Zonder deze regel zou een feed die na het eind
	// blijft napraten hier nog steeds groen staan.
	TestFalse(TEXT("Na het eind zwijgt de feed weer"), Tracker.Submit(Snapshot).bShouldBroadcast);

	// EN DE LEEG-OMSLAG DIE ERBIJ HOORT. Een beurt die vanaf 0 begint, hoort het
	// scherm ook te vertellen dat het wapen niet meer leeg is.
	FEclipseWeaponStatusTracker EmptyTracker;
	FEclipseWeaponSnapshot Dry = EclipseWeaponStatusFeedTest::FullMagazine();
	Dry.AmmoInMagazine = 0;
	const FEclipseWeaponStatusDecision DryInitial = EmptyTracker.Submit(Dry);
	TestTrue(TEXT("Een leeg wapen meldt zich als leeg"), DryInitial.Payload.bEmpty);
	Dry.AmmoInMagazine = Dry.MagazineSize;
	const FEclipseWeaponStatusDecision Refilled = EmptyTracker.Submit(Dry);
	TestFalse(TEXT("Na het vullen is hij niet meer leeg"), Refilled.Payload.bEmpty);
	TestTrue(TEXT("...en dát is als omslag gemarkeerd"), Refilled.Payload.bEmptyChanged);

	// Een wapen ZONDER magazijn is niet leeg maar telt niet mee — anders zou de
	// teller permanent op de scherpste stand staan (zie IsEmpty).
	FEclipseWeaponSnapshot Unlimited;
	Unlimited.MagazineSize = 0;
	Unlimited.AmmoInMagazine = 0;
	TestFalse(TEXT("Een wapen zonder magazijn is niet 'leeg', het telt niet"), IsEmpty(Unlimited));

	return true;
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 3 — een wapenwissel = precies ÉÉN feit, met de nieuwe naam.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponStatusFeedSwapIsOneFactTest,
	"Eclipse.Combat.WeaponStatusFeed.SwapIsExactlyOneFact",
	EclipseWeaponStatusFeedTest::TestFlags)

bool FEclipseWeaponStatusFeedSwapIsOneFactTest::RunTest(const FString& Parameters)
{
	using namespace EclipseWeaponStatusFeed;

	FEclipseWeaponStatusTracker Tracker;
	FEclipseWeaponSnapshot Primary = EclipseWeaponStatusFeedTest::FullMagazine();
	Primary.AmmoInMagazine = 17; // halfleeg weggestopt; dat is wat wisselen tactisch maakt
	Tracker.Submit(Primary);
	const int32 CountAfterSeed = Tracker.GetBroadcastCount();

	FEclipseWeaponSnapshot Sidearm;
	Sidearm.WeaponRowName = FName(TEXT("Sidearm_Scrap"));
	Sidearm.WeaponDisplayName = FText::FromString(TEXT("Scrap Sidearm"));
	Sidearm.AmmoInMagazine = 12;
	Sidearm.MagazineSize = 12;
	Sidearm.ReloadSecondsTotal = 1.4f;
	Sidearm.ActiveSlot = 1;
	Sidearm.SlotCount = 2;

	const FEclipseWeaponStatusDecision Swap = Tracker.Submit(Sidearm);
	TestEqual(TEXT("Een wissel levert PRECIES één feit op"), Tracker.GetBroadcastCount() - CountAfterSeed, 1);
	TestTrue(TEXT("...en dat feit vertrekt echt"), Swap.bShouldBroadcast);
	TestTrue(TEXT("...gemarkeerd als wapenwissel"), Swap.Payload.bWeaponChanged);
	TestEqual(TEXT("...met de NIEUWE rijnaam erin"), Swap.Payload.WeaponRowName, FName(TEXT("Sidearm_Scrap")));
	TestEqual(TEXT("...en de leesbare naam ernaast, zodat de HUD geen sleutel hoeft te tonen"),
		Swap.Payload.WeaponDisplayName.ToString(), FString(TEXT("Scrap Sidearm")));
	TestEqual(TEXT("...het magazijn van het NIEUWE wapen"), Swap.Payload.AmmoInMagazine, 12);
	TestEqual(TEXT("...met zijn eigen maat"), Swap.Payload.MagazineSize, 12);
	TestEqual(TEXT("...en het actieve slot"), Swap.Payload.ActiveSlot, 1);

	// Terugwisselen naar een halfleeg primair wapen is óók één feit, en het magazijn
	// dat terugkomt is het magazijn dat je wegstopte.
	const FEclipseWeaponStatusDecision Back = Tracker.Submit(Primary);
	TestEqual(TEXT("Terugwisselen is ook precies één feit"), Tracker.GetBroadcastCount() - CountAfterSeed, 2);
	TestTrue(TEXT("...gemarkeerd als wapenwissel"), Back.Payload.bWeaponChanged);
	TestEqual(TEXT("...en het halflege magazijn komt halfleeg terug"), Back.Payload.AmmoInMagazine, 17);

	// TWEE KEER DEZELFDE RIJ IS EEN GELDIGE LOADOUT, en dan is de wissel alleen aan
	// het slotnummer te zien. Zonder die vergelijking zou zo'n wissel STIL zijn en
	// zou de HUD het magazijn van het verkeerde wapen blijven tonen.
	FEclipseWeaponStatusTracker TwinTracker;
	FEclipseWeaponSnapshot TwinA = EclipseWeaponStatusFeedTest::FullMagazine();
	TwinA.AmmoInMagazine = 30;
	TwinA.ActiveSlot = 0;
	TwinTracker.Submit(TwinA);
	FEclipseWeaponSnapshot TwinB = TwinA;
	TwinB.ActiveSlot = 1;
	// Zelfde rij, zelfde magazijnstand: ALLEEN het slot verschilt.
	const FEclipseWeaponStatusDecision TwinSwap = TwinTracker.Submit(TwinB);
	TestTrue(TEXT("Wisselen naar een IDENTIEK tweede wapen is nog steeds een feit"), TwinSwap.bShouldBroadcast);
	TestTrue(TEXT("...gemarkeerd als wapenwissel"), TwinSwap.Payload.bWeaponChanged);

	// DE LEESTEKST BESLIST NIET MEE. Een lokalisatiewissel is geen wapenwissel; zou
	// hij dat wel zijn, dan zou een taalwissel de hele HUD laten flitsen.
	FEclipseWeaponStatusTracker TextTracker;
	FEclipseWeaponSnapshot Dutch = EclipseWeaponStatusFeedTest::FullMagazine();
	TextTracker.Submit(Dutch);
	Dutch.WeaponDisplayName = FText::FromString(TEXT("Foundry-aanvalsgeweer"));
	TestFalse(TEXT("Alleen de leestekst veranderen is GEEN feit — de rijnaam is de sleutel"),
		TextTracker.Submit(Dutch).bShouldBroadcast);

	return true;
}

// ---------------------------------------------------------------------------
// De twee velden die vandaag GEEN producent hebben, en die daarom het makkelijkst
// stilletjes kapot kunnen staan: vuurmodus en magazijnen-over.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponStatusFeedFireModeAndSparesTest,
	"Eclipse.Combat.WeaponStatusFeed.FireModeAndSpareMagazines",
	EclipseWeaponStatusFeedTest::TestFlags)

bool FEclipseWeaponStatusFeedFireModeAndSparesTest::RunTest(const FString& Parameters)
{
	using namespace EclipseWeaponStatusFeed;

	FEclipseWeaponStatusTracker Tracker;
	FEclipseWeaponSnapshot Snapshot = EclipseWeaponStatusFeedTest::FullMagazine();
	const FEclipseWeaponStatusDecision Initial = Tracker.Submit(Snapshot);

	// DT_Weapons draagt de kolom vandaag niet; dat is GEMETEN en hier vastgepind, in
	// plaats van dat de HUD straks stilletjes "SINGLE" zou tonen omdat dat toevallig
	// de eerste enumwaarde is.
	TestEqual(TEXT("Zonder geauthorde vuurmodus staat er Unspecified — de HUD hoort dan te zwijgen"),
		static_cast<int32>(Initial.Payload.FireMode), static_cast<int32>(EEclipseWeaponFireMode::Unspecified));
	TestEqual(TEXT("Magazijnen over = -1: onbeperkt bij ontwerp, niet 'op'"), Initial.Payload.SpareMagazines, -1);

	// De bedrading eronder is compleet: zodra iemand die kolom invult, is het één
	// feit en geen regel code. Deze test is het bewijs dat dat pad bestaat.
	Snapshot.FireMode = EEclipseWeaponFireMode::Automatic;
	const FEclipseWeaponStatusDecision Switched = Tracker.Submit(Snapshot);
	TestTrue(TEXT("Een andere vuurmodus is een feit"), Switched.bShouldBroadcast);
	TestTrue(TEXT("...en is als zodanig gemarkeerd"), Switched.Payload.bFireModeChanged);
	TestEqual(TEXT("...met de nieuwe modus erin"),
		static_cast<int32>(Switched.Payload.FireMode), static_cast<int32>(EEclipseWeaponFireMode::Automatic));
	TestFalse(TEXT("Dezelfde modus nogmaals aanbieden zendt niets"), Tracker.Submit(Snapshot).bShouldBroadcast);

	// En de dag dat er wél voorraad geteld wordt, is een verschuiving daarin nieuws.
	Snapshot.SpareMagazines = 4;
	const FEclipseWeaponStatusDecision Spares = Tracker.Submit(Snapshot);
	TestTrue(TEXT("Een verschuivende voorraad is een feit"), Spares.bShouldBroadcast);
	TestEqual(TEXT("...met het getal erin"), Spares.Payload.SpareMagazines, 4);

	return true;
}

// ---------------------------------------------------------------------------
// DE BELANGRIJKSTE — komt het feit ook AAN, op de echte bus, uit het echte wapen?
// Pure logica die niemand aanroept is perfecte logica.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponStatusWiredToBusTest,
	"Eclipse.Combat.WeaponStatus.WiredToBus",
	EclipseWeaponStatusFeedTest::TestFlags)

bool FEclipseWeaponStatusWiredToBusTest::RunTest(const FString& Parameters)
{
	using namespace EclipseWeaponStatusFeedTest;

	EclipseFeelHarness::FHarness Harness;
	EclipseFeelHarness::FHarness::FOptions Options;
	// 1/60 in plaats van 1/120: deze meting legt zeven seconden speeltijd af en
	// heeft geen 8 ms-resolutie nodig. De herlaadvoortgang wordt op deze stap
	// gemeten, en dat is precies de framestap waar het spel op draait.
	Options.StepSeconds = 1.0f / 60.0f;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UEclipseEventBusSubsystem* Bus = Harness.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestNotNull(TEXT("De echte event-bus staat in deze wereld"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	TArray<FEclipseWeaponStatusPayload> Received;
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		EclipseTags::Event_Player_WeaponStatusChanged,
		FEclipseEventNativeDelegate::CreateLambda([&Received](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			if (const FEclipseWeaponStatusPayload* Status = Payload.GetPtr<FEclipseWeaponStatusPayload>())
			{
				Received.Add(*Status);
			}
		}),
		FEclipseWeaponStatusPayload::StaticStruct());

	// Het wapen zoals de game mode het geeft: twee slots uit een loadout. Hier met
	// de hand, want dit harnas draait bewust ZONDER game mode (een vlakke, bekende
	// vloer); het component en het pad zijn wel de verscheepte.
	UEclipseHitscanWeaponComponent* Weapon = NewObject<UEclipseHitscanWeaponComponent>(Harness.Body);
	Harness.Body->AddOwnedComponent(Weapon);
	Weapon->RegisterComponent();

	Received.Reset();
	Weapon->ApplyLoadout(TestRow(30), FName(TEXT("AR_Foundry")), TestRow(12), FName(TEXT("Sidearm_Scrap")));

	// CONTROLEPROEF 0: er komt op DEZE opstelling überhaupt iets aan. Zonder deze
	// regel bewijst elke latere nul niets — dan kan "stil" ook "nooit aangesloten"
	// betekenen, en dat is deze week vaker de verklaring geweest dan een echte bug.
	EclipseFeelHarness::Report(*this, TEXT("feiten bij het uitrusten"), Received.Num(), TEXT("feiten"));
	if (!TestTrue(TEXT("CONTROLEPROEF: het uitrusten bereikt de bus"), Received.Num() >= 1))
	{
		Bus->Unsubscribe(Handle);
		Harness.Shutdown();
		return false;
	}
	TestEqual(TEXT("Het laatste feit draagt de rijnaam van het primaire wapen"),
		Received.Last().WeaponRowName, FName(TEXT("AR_Foundry")));
	TestEqual(TEXT("...en een vol magazijn van 30"), Received.Last().AmmoInMagazine, 30);
	TestEqual(TEXT("...en meldt twee slots, dus een wisselknop"), Received.Last().SlotCount, 2);

	// --- FALSIFICATIE 4 op een ECHT tickende wereld ----------------------------
	// Sterker dan de pure variant: hier draaien camera-blends, bewegingsupdates en
	// animatie mee, en de component-tick staat aan als iemand hem vergat uit te
	// zetten. Zat er ergens een per-frame-schrijver, dan ziet hij dat hier.
	Received.Reset();
	Harness.Idle(60.0f / 60.0f);
	EclipseFeelHarness::Report(*this, TEXT("feiten uit 60 getickte frames zonder handeling"), Received.Num(), TEXT("feiten"));
	TestEqual(TEXT("60 getickte frames zonder gebeurtenis = 0 feiten op de bus"), Received.Num(), 0);

	// --- FALSIFICATIE 1: 30 schoten = 30 feiten --------------------------------
	// SCHUIN OMHOOG en niet in de vloer: een wereldtreffer spawnt een inslagspoor,
	// en dertig daarvan zouden deze meting met ander werk vermengen. Wat hier
	// gemeten wordt is de TREKKER, niet de trefkans.
	const FVector Muzzle = Harness.Body->GetActorLocation();
	const FVector SkyWard = FVector(1.0f, 0.0f, 0.35f).GetSafeNormal();

	Received.Reset();
	const int32 ShotsBefore = Weapon->GetShotsFired();
	const int32 StatusBefore = Weapon->GetWeaponStatusBroadcastCount();
	TArray<int32> Magazines;
	for (int32 Shot = 0; Shot < 30; ++Shot)
	{
		// De cadanspoort staat op 0,15 s; wie sneller aanbiedt, meet de poort en
		// niet de feed.
		Harness.Idle(0.16f);
		Weapon->Fire(Muzzle, SkyWard, TEXT("StatusMeting"));
	}
	for (const FEclipseWeaponStatusPayload& Fact : Received)
	{
		Magazines.Add(Fact.AmmoInMagazine);
	}

	// ELKE KETENSTAP ZIJN EIGEN GETAL. Een "0 feiten" heeft hier vier verklaringen —
	// de trekker kwam niet door de poort, de poort liet het lichaam niet door, de
	// feed vond het geen verandering, of de bus leverde niet af — en een kale teller
	// scheidt die vier niet.
	EclipseFeelHarness::Report(*this, TEXT("ketenstap 1: schoten door de cadanspoort"),
		Weapon->GetShotsFired() - ShotsBefore, TEXT("schoten"));
	EclipseFeelHarness::Report(*this, TEXT("ketenstap 2: uitzendingen geteld op het component"),
		Weapon->GetWeaponStatusBroadcastCount() - StatusBefore, TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("ketenstap 3: op de BUS aangekomen"), Received.Num(), TEXT("feiten"));

	TestEqual(TEXT("Ketenstap 1: er gingen werkelijk 30 schoten af"), Weapon->GetShotsFired() - ShotsBefore, 30);
	TestEqual(TEXT("Ketenstap 2: de feed telde 30 uitzendingen"), Weapon->GetWeaponStatusBroadcastCount() - StatusBefore, 30);
	TestEqual(TEXT("Ketenstap 3: 30 schoten = 30 feiten op de bus"), Received.Num(), 30);

	if (Magazines.Num() == 30)
	{
		TestEqual(TEXT("Het eerste feit meldt 29"), Magazines[0], 29);
		TestEqual(TEXT("Het dertigste feit meldt 0"), Magazines[29], 0);
		TestTrue(TEXT("Het laatste feit meldt een LEEG magazijn"), Received.Last().bEmpty);
		for (int32 Index = 1; Index < Magazines.Num(); ++Index)
		{
			if (Magazines[Index] != Magazines[Index - 1] - 1)
			{
				AddError(FString::Printf(TEXT("Het magazijn liep op de bus niet af: %d na %d (feit %d)."),
					Magazines[Index], Magazines[Index - 1], Index + 1));
				break;
			}
		}
	}

	// --- FALSIFICATIE 2 op de echte klok: start + voortgang + eind -------------
	// Het magazijn is leeg, dus dit schot start automatisch een herlaadbeurt (dat is
	// het verscheepte gedrag: een dode trekker leest als een defect).
	Received.Reset();
	Harness.Idle(0.16f);
	Weapon->Fire(Muzzle, SkyWard, TEXT("StatusMeting"));
	TestTrue(TEXT("Een leeg magazijn start uit zichzelf een herlaadbeurt"), Weapon->IsReloading());

	int32 StartFacts = 0;
	int32 EndFacts = 0;
	TArray<float> Progress;
	// 3,0 s: ruim voorbij de beurt van 2,2 s, zodat het EIND ook echt in het venster
	// valt. Zou de meting op 2,2 stoppen, dan zou een beurt die één frame te laat
	// afsluit als "geen eind" lezen.
	const int32 Frames = FMath::RoundToInt(3.0f / Options.StepSeconds);
	int32 SilentFrames = 0;
	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		const int32 Before = Received.Num();
		Harness.Step();
		if (Received.Num() == Before)
		{
			++SilentFrames;
		}
	}
	for (const FEclipseWeaponStatusPayload& Fact : Received)
	{
		if (Fact.bReloadStateChanged)
		{
			if (Fact.bReloading) { ++StartFacts; } else { ++EndFacts; }
		}
		else if (Fact.bReloadProgressed)
		{
			Progress.Add(Fact.ReloadProgress);
		}
	}

	EclipseFeelHarness::Report(*this, TEXT("voortgangsfeiten in één echte herlaadbeurt"), Progress.Num(), TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("frames die NIETS opleverden"), SilentFrames, TEXT("frames"));
	TestEqual(TEXT("Precies één EIND-feit"), EndFacts, 1);
	TestTrue(TEXT("Er komt echte voortgang door"), Progress.Num() > 0);
	TestTrue(TEXT("...en nooit meer dan 1/drempel"),
		Progress.Num() <= FMath::CeilToInt(1.0f / EclipseWeaponStatusFeed::ReloadProgressEpsilon));
	TestTrue(TEXT("...en de meeste frames zwijgen: dit is geen tick-stroom"), SilentFrames > Frames / 2);
	for (int32 Index = 1; Index < Progress.Num(); ++Index)
	{
		if (Progress[Index] <= Progress[Index - 1])
		{
			AddError(FString::Printf(TEXT("De voortgang liep op de bus niet monotoon op: %.3f na %.3f."),
				Progress[Index], Progress[Index - 1]));
			break;
		}
	}
	TestEqual(TEXT("De beurt is voorbij en het magazijn is vol"), Weapon->GetAmmoInMagazine(), 30);

	// EN DE TICK IS UIT. Een venster dat open blijft staan is exact de vorm van
	// defect 2 (een toestand die wacht op een handeling die misschien nooit komt),
	// alleen dan met framekosten erbij.
	TestFalse(TEXT("Na de beurt tikt het component niet meer"), Weapon->IsComponentTickEnabled());

	// Stilte-controle op de plek waar het het spannendst is: net ná een beurt.
	Received.Reset();
	Harness.Idle(1.0f);
	TestEqual(TEXT("Een seconde stilte ná een herlaadbeurt levert 0 feiten op"), Received.Num(), 0);

	// --- FALSIFICATIE 3: de wissel op de echte bus -----------------------------
	Received.Reset();
	TestTrue(TEXT("De wissel lukt"), Weapon->SwapWeapon());
	EclipseFeelHarness::Report(*this, TEXT("feiten uit één wapenwissel"), Received.Num(), TEXT("feiten"));
	TestEqual(TEXT("Een wapenwissel = precies één feit"), Received.Num(), 1);
	if (Received.Num() == 1)
	{
		TestEqual(TEXT("...met de NIEUWE rijnaam"), Received[0].WeaponRowName, FName(TEXT("Sidearm_Scrap")));
		TestTrue(TEXT("...gemarkeerd als wapenwissel"), Received[0].bWeaponChanged);
		TestEqual(TEXT("...en het magazijn van de sidearm"), Received[0].AmmoInMagazine, 12);
		TestEqual(TEXT("...op slot 1"), Received[0].ActiveSlot, 1);
	}

	// --- DE POORT: een lichaam ZONDER speler hoort hier niets te doen -----------
	// Zonder deze meting zou "de bus is rustig" ook kunnen betekenen "er staat geen
	// vijand te schieten"; met een echte vijand die echt vuurt scheidt hij de twee.
	Received.Reset();
	AEclipseCharacter* Hostile = Harness.World->SpawnActor<AEclipseCharacter>(FVector(800.0f, 0.0f, 120.0f), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Vijandelijk lichaam gespawnd"), Hostile))
	{
		UEclipseHitscanWeaponComponent* HostileWeapon = NewObject<UEclipseHitscanWeaponComponent>(Hostile);
		Hostile->AddOwnedComponent(HostileWeapon);
		HostileWeapon->RegisterComponent();
		HostileWeapon->ApplyWeaponRow(TestRow(30));
		Harness.Idle(0.16f);
		HostileWeapon->Fire(Hostile->GetActorLocation(), SkyWard, TEXT("VijandMeting"));

		TestEqual(TEXT("Een AI-wapen zet zijn magazijn NIET op de spelersbus"), Received.Num(), 0);
		TestEqual(TEXT("...en zijn eigen teller staat ook op nul: de poort zit vóór de tracker"),
			HostileWeapon->GetWeaponStatusBroadcastCount(), 0);
		TestTrue(TEXT("Controleproef: dat schot ging wél echt af"), HostileWeapon->GetShotsFired() >= 1);
		TestEqual(TEXT("...en zijn magazijn liep wel degelijk af"), HostileWeapon->GetAmmoInMagazine(), 29);
	}

	Bus->Unsubscribe(Handle);
	Harness.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
