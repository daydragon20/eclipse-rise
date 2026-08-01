// De pure kern van de SPELERLAAG van de HUD (GDD 14.4/14.5 stap 2).
//
// Elke test hieronder hoort bij een defect dat de schermbeoordeling van 31-07 op
// een FRAME vond. Dat is met opzet: een defect dat alleen op een screenshot te zien
// was, is een defect dat geen enkele test kon vinden — en dan is de eerste
// reparatie niet de code maar de meetbaarheid.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/EclipseHudReadoutLogic.h"

namespace EclipseHudReadoutTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::ProductFilter;

	EclipseHudReadout::FEclipseWeaponReadoutFacts MakeFacts()
	{
		EclipseHudReadout::FEclipseWeaponReadoutFacts Facts;
		Facts.DisplayName = FText::FromString(TEXT("Scrap Sidearm"));
		Facts.RowName = TEXT("Sidearm_Scrap");
		Facts.AmmoInMagazine = 7;
		Facts.MagazineSize = 12;
		Facts.SlotCount = 2;
		return Facts;
	}
}

/**
 * DEFECT 1: de munitieteller mag tijdens het herladen NIET verdwijnen.
 *
 * GEZIEN op HUD_wapen_E_na_wissel.png: één tekstveld droeg zowel het aantal kogels
 * als het woord HERLADEN, en tijdens de beurt won HERLADEN. Precies dan wil je
 * weten hoeveel er straks in zit.
 *
 * ROOD = het ene veld is terug, of de teller wordt tijdens het herladen leeg.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudAmmoStaysVisibleWhileReloadingTest,
	"Eclipse.UI.HudReadout.AmmoStaysReadableWhileReloading",
	EclipseHudReadoutTest::TestFlags)

bool FEclipseHudAmmoStaysVisibleWhileReloadingTest::RunTest(const FString&)
{
	using namespace EclipseHudReadout;

	// CONTROLEPROEF EERST: laat zien dat de twee toestanden verschillen, anders zegt
	// "de tekst is er nog" niets — een functie die altijd hetzelfde teruggeeft
	// haalt deze test ook.
	FEclipseWeaponReadoutFacts Rustig = EclipseHudReadoutTest::MakeFacts();
	const FEclipseAmmoReadout Uit = ComposeAmmoReadout(Rustig);
	TestTrue(TEXT("CONTROLEPROEF: buiten een herlaadbeurt staat er geen herlaadtekst"), Uit.ReloadText.IsEmpty());

	FEclipseWeaponReadoutFacts Bezig = Rustig;
	Bezig.bReloading = true;
	Bezig.ReloadProgress = 0.4f;
	const FEclipseAmmoReadout Aan = ComposeAmmoReadout(Bezig);
	TestFalse(TEXT("CONTROLEPROEF: tijdens de beurt staat er wél herlaadtekst"), Aan.ReloadText.IsEmpty());

	// En dan de eis zelf.
	TestEqual(TEXT("de kogels staan er tijdens het herladen NOG STEEDS"), Aan.AmmoText, FString(TEXT("7 / 12")));
	TestEqual(TEXT("de kogels zijn identiek aan de rustige stand — het herladen overschrijft niets"),
		Aan.AmmoText, Uit.AmmoText);
	TestFalse(TEXT("de teller wordt tijdens het herladen niet verborgen"), Aan.bHidden);
	TestEqual(TEXT("de voortgang komt door"), Aan.ReloadProgress, 0.4f);

	// De brontaal is Engels (13_roadmap.md r45 zet NL bij de DOELtalen) — defect 6.
	TestEqual(TEXT("de herlaadtekst staat in de brontaal Engels, niet in het Nederlands"),
		Aan.ReloadText.ToString(), FString(TEXT("RELOADING")));

	// Voortgang buiten de bandbreedte hoort geklemd te worden en niet doorgegeven:
	// een balk van 340 % tekent buiten zijn eigen vak.
	Bezig.ReloadProgress = 3.4f;
	TestEqual(TEXT("voortgang wordt geklemd op 1"), ComposeAmmoReadout(Bezig).ReloadProgress, 1.0f);
	return true;
}

/**
 * DEFECT 3: er staat een LEESBARE naam op het scherm, geen rijnaam.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudWeaponNameIsReadableTest,
	"Eclipse.UI.HudReadout.WeaponNameIsReadableNotARowKey",
	EclipseHudReadoutTest::TestFlags)

bool FEclipseHudWeaponNameIsReadableTest::RunTest(const FString&)
{
	using namespace EclipseHudReadout;

	FEclipseWeaponReadoutFacts Facts = EclipseHudReadoutTest::MakeFacts();
	const FEclipseAmmoReadout Goed = ComposeAmmoReadout(Facts);
	TestEqual(TEXT("de naam uit de data komt op het scherm"), Goed.WeaponText.ToString(), FString(TEXT("Scrap Sidearm")));
	TestTrue(TEXT("en die komt aantoonbaar uit de data"), DisplayNameCameFromData(Facts));

	// DE DEGRADATIE. Een rij zonder DisplayName mag geen leeg vakje geven en al
	// helemaal geen sleutel; hij geeft de opgepoetste rijnaam. Dat is een NOODVERBAND
	// en geen ontwerp — de validator hoort de lege rij te vinden, zie
	// EclipseValidateDataCommandlet.
	FEclipseWeaponReadoutFacts Leeg = Facts;
	Leeg.DisplayName = FText::GetEmpty();
	const FEclipseAmmoReadout Terugval = ComposeAmmoReadout(Leeg);
	TestFalse(TEXT("zonder data meldt de kern dat er opgepoetst is"), DisplayNameCameFromData(Leeg));
	TestFalse(TEXT("de terugval is niet leeg"), Terugval.WeaponText.IsEmpty());
	TestFalse(TEXT("er staat GEEN underscore op het scherm"), Terugval.WeaponText.ToString().Contains(TEXT("_")));
	TestEqual(TEXT("de terugval is de opgepoetste rijnaam"), Terugval.WeaponText.ToString(), FString(TEXT("Sidearm Scrap")));

	// EEN WAPEN, GEEN NAAM. Met niets om naar te wisselen is de naam ruis: je weet
	// wat je vasthoudt want er is niets anders.
	FEclipseWeaponReadoutFacts Enkel = Facts;
	Enkel.SlotCount = 1;
	TestTrue(TEXT("met één wapen staat er geen naam"), ComposeAmmoReadout(Enkel).WeaponText.IsEmpty());

	TestEqual(TEXT("HumaniseRowName laat een naam zonder scheidingsteken met rust"),
		HumaniseRowName(TEXT("Longsight")), FString(TEXT("Longsight")));
	TestEqual(TEXT("HumaniseRowName op niets geeft niets"), HumaniseRowName(NAME_None), FString());
	return true;
}

/**
 * DEFECT 1 (de andere helft) + de leestoestanden van de teller.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudAmmoStatesTest,
	"Eclipse.UI.HudReadout.AmmoStatesLowEmptyAndNoMagazine",
	EclipseHudReadoutTest::TestFlags)

bool FEclipseHudAmmoStatesTest::RunTest(const FString&)
{
	using namespace EclipseHudReadout;

	FEclipseWeaponReadoutFacts Facts = EclipseHudReadoutTest::MakeFacts();

	// Vol: niet laag, niet leeg.
	Facts.AmmoInMagazine = 12;
	const FEclipseAmmoReadout Vol = ComposeAmmoReadout(Facts);
	TestFalse(TEXT("vol is niet laag"), Vol.bLow);
	TestFalse(TEXT("vol is niet leeg"), Vol.bEmpty);

	// Precies op een derde: nog nét laag — de grens hoort bij de waarschuwing, want
	// twijfel op een grens moet de kant van "kijk uit" op vallen.
	Facts.AmmoInMagazine = 4;
	TestTrue(TEXT("een derde van het magazijn telt als laag"), ComposeAmmoReadout(Facts).bLow);
	Facts.AmmoInMagazine = 5;
	TestFalse(TEXT("net boven een derde is niet laag"), ComposeAmmoReadout(Facts).bLow);

	// Leeg.
	Facts.AmmoInMagazine = 0;
	const FEclipseAmmoReadout Leeg = ComposeAmmoReadout(Facts);
	TestTrue(TEXT("nul kogels is leeg"), Leeg.bEmpty);
	TestTrue(TEXT("nul kogels is ook laag"), Leeg.bLow);
	TestEqual(TEXT("leeg toont nog steeds een getal en geen leegte"), Leeg.AmmoText, FString(TEXT("0 / 12")));

	// Negatief kan echt voorkomen: ApplyWeaponRow zet -1 tot er een rij is.
	Facts.AmmoInMagazine = -1;
	TestEqual(TEXT("een negatief magazijn wordt niet als '-1' getoond"), ComposeAmmoReadout(Facts).AmmoText, FString(TEXT("0 / 12")));

	// Geen magazijn = geen teller. "0 / 0" zou liegen over een wapen dat nooit leeg raakt.
	Facts.MagazineSize = 0;
	TestTrue(TEXT("een wapen zonder magazijn heeft geen teller"), ComposeAmmoReadout(Facts).bHidden);
	return true;
}

/**
 * DEFECT 4: er is één marge, en die geldt overal.
 *
 * GEMETEN op HUD_wapen_E_na_wissel.png (1280x720): de wapenregel raakte kolom 1278
 * van 1279 (marge 1 px) en de regel linksboven begon op kolom 9 (0,7 %). Twee
 * verschillende getallen in dezelfde HUD, geen van beide gekozen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudTitleSafeMarginTest,
	"Eclipse.UI.HudReadout.TitleSafeMarginIsOneNumberEverywhere",
	EclipseHudReadoutTest::TestFlags)

bool FEclipseHudTitleSafeMarginTest::RunTest(const FString&)
{
	using namespace EclipseHudReadout;

	struct FCase { FVector2D Size; const TCHAR* Wat; };
	const FCase Cases[] = {
		{ FVector2D(1280.0, 720.0),  TEXT("720p — het formaat van de opnameronde") },
		{ FVector2D(1920.0, 1080.0), TEXT("1080p") },
		{ FVector2D(2560.0, 1440.0), TEXT("1440p") },
		{ FVector2D(3840.0, 2160.0), TEXT("4K") },
		{ FVector2D(640.0, 360.0),   TEXT("een klein debugvenster") },
		{ FVector2D(2560.0, 1080.0), TEXT("ultrawide 21:9") },
	};

	for (const FCase& Case : Cases)
	{
		const FVector2D Margin = TitleSafeMarginPx(Case.Size);
		AddInfo(FString::Printf(TEXT("GEMETEN %s (%.0fx%.0f): marge %.1f x %.1f px"),
			Case.Wat, Case.Size.X, Case.Size.Y, Margin.X, Margin.Y));

		// De oude waarde van 1 px moet per constructie onmogelijk zijn geworden.
		TestTrue(FString::Printf(TEXT("%s: de marge is minstens %.0f px"), Case.Wat, MinSafeMarginPx),
			Margin.X >= MinSafeMarginPx && Margin.Y >= MinSafeMarginPx);

		// En nooit zo ruim dat het beeld leegloopt: een marge van een tiende van het
		// scherm is geen marge meer maar een lijst.
		TestTrue(FString::Printf(TEXT("%s: de marge blijft onder een tiende van de kortste as"), Case.Wat),
			Margin.X <= FMath::Min(Case.Size.X, Case.Size.Y) * 0.1);

		TestEqual(FString::Printf(TEXT("%s: horizontaal en verticaal even ruim"), Case.Wat), Margin.X, Margin.Y);
	}

	// 720p is de maat waarop de defecten gemeten zijn; die pin ik vast, zodat een
	// latere wijziging aan de fractie hier zichtbaar wordt in plaats van stil.
	TestEqual(TEXT("op 720p is de marge 36 px (5 % van de kortste as)"), TitleSafeMarginPx(FVector2D(1280.0, 720.0)).X, 36.0);
	return true;
}

/**
 * DEFECT 5: het richtkruis is groot genoeg om te vinden, en zegt wat je vasthoudt.
 *
 * GEMETEN op alle elf frames van 31-07: 7x9 px, 17 pixels inkt. Dat is het defect —
 * niet het contrast (de donkere rand meet 0,0 en zit er dus echt op), maar de MAAT.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudCrosshairTest,
	"Eclipse.UI.HudReadout.CrosshairIsBigEnoughAndSaysWhatYouHold",
	EclipseHudReadoutTest::TestFlags)

bool FEclipseHudCrosshairTest::RunTest(const FString&)
{
	using namespace EclipseHudReadout;

	constexpr float Height720 = 720.0f;
	constexpr float Fov = 64.0f; // de mikstand van de spelercamera

	// ---- de vorm zegt wat je vasthoudt --------------------------------------
	const FEclipseCrosshairLayout Ar = ComposeCrosshair(2.5f, 1, 0.6f, Height720, Fov);
	const FEclipseCrosshairLayout Dmr = ComposeCrosshair(5.0f, 1, 0.15f, Height720, Fov);
	const FEclipseCrosshairLayout Hagel = ComposeCrosshair(6.0f, 8, 3.0f, Height720, Fov);

	TestTrue(TEXT("de AR krijgt een gewoon kruis"), Ar.Shape == EEclipseCrosshairShape::Cross);
	TestTrue(TEXT("de DMR krijgt de precisievorm"), Dmr.Shape == EEclipseCrosshairShape::Precision);
	TestTrue(TEXT("een hagelwapen krijgt haken"), Hagel.Shape == EEclipseCrosshairShape::Brackets);

	// ---- de MAAT: de meting van 31-07 mag niet terug kunnen komen -----------
	//
	// Het oude kruis was 9 px in totaal (7x9 bbox, 17 px inkt). De eis hieronder is
	// dat één arm alleen al langer is dan dat hele oude kruis breed was.
	for (const FEclipseCrosshairLayout& Layout : { Ar, Dmr, Hagel })
	{
		const float TotalSpanPx = 2.0f * (Layout.GapPx + Layout.ArmLengthPx);
		AddInfo(FString::Printf(TEXT("GEMETEN kruis: arm %.1f px, dikte %.1f px, gat %.1f px, totale spanwijdte %.1f px"),
			Layout.ArmLengthPx, Layout.ThicknessPx, Layout.GapPx, TotalSpanPx));
		TestTrue(TEXT("één arm is langer dan het hele oude kruis breed was (7 px)"), Layout.ArmLengthPx > 7.0f);
		TestTrue(TEXT("de balk is dikker dan één pixel — anders verdwijnt hij in de ruis"), Layout.ThicknessPx >= 2.0f);
		TestTrue(TEXT("er is een echt gat in het midden, zodat het doelwit vrij blijft"), Layout.GapPx >= 4.0f);
	}

	// ---- het kruis REAGEERT op de toestand ----------------------------------
	//
	// CONTROLEPROEF: kan het gat überhaupt veranderen? Zonder dit bewijst "het gat
	// groeit bij meer spreiding" niets als beide waarden toevallig geklemd worden.
	const float NauwGat = ComposeCrosshair(0.6f, 1, 0.6f, Height720, Fov).GapPx;
	const float WijdGat = ComposeCrosshair(6.0f, 1, 0.6f, Height720, Fov).GapPx;
	AddInfo(FString::Printf(TEXT("GEMETEN gat bij 0,6 graden spreiding: %.1f px; bij 6,0 graden: %.1f px"), NauwGat, WijdGat));
	TestTrue(TEXT("CONTROLEPROEF: het gat kán verschillen tussen twee spreidingen"), !FMath::IsNearlyEqual(NauwGat, WijdGat));
	TestTrue(TEXT("meer spreiding = een wijder kruis"), WijdGat > NauwGat);

	// ---- schalen met de resolutie -------------------------------------------
	const FEclipseCrosshairLayout Op1440 = ComposeCrosshair(2.5f, 1, 0.6f, 1440.0f, Fov);
	AddInfo(FString::Printf(TEXT("GEMETEN op 1440p: arm %.1f px tegen %.1f px op 720p"), Op1440.ArmLengthPx, Ar.ArmLengthPx));
	TestTrue(TEXT("op een dubbel zo hoog scherm is het kruis ook groter"), Op1440.ArmLengthPx > Ar.ArmLengthPx);

	// ---- onzin-invoer mag niets laten ontploffen ----------------------------
	const FEclipseCrosshairLayout Kapot = ComposeCrosshair(-3.0f, 0, 0.0f, 0.0f, 0.0f);
	TestTrue(TEXT("een FOV van 0 levert nog steeds een eindig gat"), FMath::IsFinite(Kapot.GapPx) && Kapot.GapPx > 0.0f);
	TestTrue(TEXT("een schermhoogte van 0 levert nog steeds een eindige arm"), FMath::IsFinite(Kapot.ArmLengthPx) && Kapot.ArmLengthPx > 0.0f);
	return true;
}

// ---------------------------------------------------------------------------
// N-c — DE GEZONDHEIDSUITLEZING, de pure helft.
//
// De nulmeting: op alle tien geopende HUD-frames van 01-08 staat linksonder NIETS.
// `REFERENTIE_HUD_BORDERLANDS.md` r36-37 wijst die hoek aan gezondheid toe. Wat
// hieronder staat is elke BESLISSING over wat daar komt — het getal, het woord, de
// toestand — zodat ze te toetsen zijn zonder een game te starten.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudVitalsReadoutTest,
	"Eclipse.UI.HudReadout.VitalsSaysWhatTheBodyIsDoing",
	EclipseHudReadoutTest::TestFlags)

bool FEclipseHudVitalsReadoutTest::RunTest(const FString&)
{
	using namespace EclipseHudReadout;

	// ---- CONTROLEPROEF EERST: nog geen feit is iets ANDERS dan nul ----------
	//
	// Zonder deze proef zou "de hoek is leeg" niet te onderscheiden zijn van "de hoek
	// staat op nul", en dat zijn twee heel verschillende schermen: het eerste zegt
	// niets, het tweede meldt je dood.
	FEclipseVitalsReadoutFacts Leeg;
	Leeg.bHasFact = false;
	const FEclipseVitalsReadout Niets = ComposeVitalsReadout(Leeg);
	TestTrue(TEXT("CONTROLEPROEF: zonder feit staat er niets"), Niets.bHidden);
	TestTrue(TEXT("...en er is geen getal om per ongeluk te tonen"), Niets.HealthText.IsEmpty());

	FEclipseVitalsReadoutFacts Vol;
	Vol.bHasFact = true;
	Vol.Health = 100.0f;
	Vol.MaxHealth = 100.0f;
	Vol.PreviousHealth = 100.0f;
	const FEclipseVitalsReadout Gezond = ComposeVitalsReadout(Vol);
	TestFalse(TEXT("CONTROLEPROEF: mét feit staat er wél iets"), Gezond.bHidden);
	TestEqual(TEXT("het getal is de gezondheid"), Gezond.HealthText, FString(TEXT("100")));
	TestEqual(TEXT("het maximum staat er apart naast"), Gezond.CapacityText, FString(TEXT("/ 100")));
	TestEqual(TEXT("de balk staat vol"), Gezond.HealthFraction, 1.0f);
	TestFalse(TEXT("vol leven kleurt niet"), Gezond.bLow);

	// ---- 100 -> 60: het getal volgt, en de balk volgt hetzelfde getal -------
	FEclipseVitalsReadoutFacts Geraakt = Vol;
	Geraakt.Health = 60.0f;
	Geraakt.PreviousHealth = 100.0f;
	const FEclipseVitalsReadout Gewond = ComposeVitalsReadout(Geraakt);
	TestEqual(TEXT("60 van de 100 leest als 60"), Gewond.HealthText, FString(TEXT("60")));
	TestEqual(TEXT("...en de balk staat op 0,6 — één bron voor getal en balk"), Gewond.HealthFraction, 0.6f);
	TestFalse(TEXT("60 % is nog niet 'laag'"), Gewond.bLow);

	FEclipseVitalsReadoutFacts Kritiek = Vol;
	Kritiek.Health = 30.0f;
	TestTrue(TEXT("onder een derde kleurt hij — zelfde grens als het magazijn"),
		ComposeVitalsReadout(Kritiek).bLow);

	// ---- afronden naar boven, en dat is geen kosmetiek ----------------------
	//
	// Bij 0,4 hp zou rekenkundig afronden "0" tonen terwijl je nog leeft. Dat is een
	// leugen in precies de richting waar hij het duurst is.
	FEclipseVitalsReadoutFacts Sliver = Vol;
	Sliver.Health = 0.4f;
	const FEclipseVitalsReadout Rest = ComposeVitalsReadout(Sliver);
	AddInfo(FString::Printf(TEXT("GEMETEN 0,4 hp op het scherm: '%s'"), *Rest.HealthText));
	TestEqual(TEXT("0,4 hp is niet nul — je leeft nog"), Rest.HealthText, FString(TEXT("1")));

	FEclipseVitalsReadoutFacts Nul = Vol;
	Nul.Health = 0.0f;
	TestEqual(TEXT("echt nul is wél nul"), ComposeVitalsReadout(Nul).HealthText, FString(TEXT("0")));

	// ---- DE HOUDING, in de brontaal Engels ---------------------------------
	//
	// Er heeft ooit NSLOCTEXT("Eclipse","Reloading","HERLADEN") in de schermlaag
	// gestaan; deze vier regels zijn de reden dat dat hier niet stil kan terugkomen.
	FEclipseVitalsReadoutFacts Houding = Vol;
	Houding.Stance = EEclipseStance::Standing;
	TestEqual(TEXT("staand"), ComposeVitalsReadout(Houding).StanceText.ToString(), FString(TEXT("STANDING")));
	Houding.Stance = EEclipseStance::Crouched;
	TestEqual(TEXT("hurken"), ComposeVitalsReadout(Houding).StanceText.ToString(), FString(TEXT("CROUCHED")));
	Houding.Stance = EEclipseStance::Sprinting;
	TestEqual(TEXT("sprint"), ComposeVitalsReadout(Houding).StanceText.ToString(), FString(TEXT("SPRINTING")));
	Houding.Stance = EEclipseStance::InCover;
	TestEqual(TEXT("dekking"), ComposeVitalsReadout(Houding).StanceText.ToString(), FString(TEXT("IN COVER")));

	// ---- NEER EN GESTABILISEERD ZIJN TWEE TOESTANDEN, geen één "gewond" -----
	//
	// Dit is de scherpste eis van N-c. Allebei gaan ze gepaard met een lage
	// gezondheid, dus met een balk alleen zijn ze niet te scheiden — terwijl "lig ik
	// of sta ik weer" het enige is waar je op dat moment naar handelt.
	FEclipseVitalsReadoutFacts Neer = Vol;
	Neer.Health = 0.0f;
	Neer.PreviousHealth = 12.0f;
	Neer.bDowned = true;
	Neer.bDownedChanged = true;
	const FEclipseVitalsReadout Liggend = ComposeVitalsReadout(Neer);

	FEclipseVitalsReadoutFacts Terug = Vol;
	Terug.Health = 100.0f;
	Terug.PreviousHealth = 0.0f;
	Terug.bDowned = false;
	Terug.bDownedChanged = true;
	const FEclipseVitalsReadout Overeind = ComposeVitalsReadout(Terug);

	AddInfo(FString::Printf(TEXT("GEMETEN toestanden: neer='%s' · gestabiliseerd='%s'"),
		*Liggend.StatusText.ToString(), *Overeind.StatusText.ToString()));
	TestEqual(TEXT("neergaan heet DOWN"), Liggend.StatusText.ToString(), FString(TEXT("DOWN")));
	TestEqual(TEXT("gestabiliseerd worden heet STABILIZED"), Overeind.StatusText.ToString(), FString(TEXT("STABILIZED")));
	TestNotEqual(TEXT("en de twee zijn op het scherm ONDERSCHEIDBAAR"),
		Liggend.StatusText.ToString(), Overeind.StatusText.ToString());
	TestTrue(TEXT("neer zegt ook los van de tekst dat hij neer is"), Liggend.bDowned);
	TestTrue(TEXT("gestabiliseerd is een eigen vlag, niet 'niet neer'"), Overeind.bStabilized);
	TestFalse(TEXT("...en gestabiliseerd is dus niet neer"), Overeind.bDowned);

	// Terwijl je ligt is er geen houding: de bewegingslaag staat uit, dus
	// "STANDING · DOWN" zou het scherm zichzelf laten tegenspreken.
	TestTrue(TEXT("terwijl je ligt staat er geen houding"), Liggend.StanceText.IsEmpty());
	TestFalse(TEXT("...en zodra je weer staat wel"), Overeind.StanceText.IsEmpty());

	// EN DE DERDE TOESTAND: gewoon gewond. Zonder deze regel zou "twee
	// onderscheidbare toestanden" ook waar zijn voor een uitlezing die ALTIJD iets
	// roept — en dan staat er permanent een melding waar niets aan de hand is.
	TestTrue(TEXT("gewoon gewond zijn is geen meldingswaardige toestand"), Gewond.StatusText.IsEmpty());

	// ---- een lichaam zonder maximum toont niets ----------------------------
	FEclipseVitalsReadoutFacts ZonderMax;
	ZonderMax.bHasFact = true;
	ZonderMax.MaxHealth = 0.0f;
	TestTrue(TEXT("zonder maximum valt er niets af te zetten, dus staat er niets"),
		ComposeVitalsReadout(ZonderMax).bHidden);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
