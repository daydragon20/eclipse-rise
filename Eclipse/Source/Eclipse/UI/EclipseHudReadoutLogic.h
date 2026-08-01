#pragma once

#include "CoreMinimal.h"
// EEclipseStance — pure data (een enum uit de payload-header, geen engine-actor).
// Bewust de ECHTE enum en geen eigen kopie: twee enums die hetzelfde bedoelen
// lopen uit elkaar zodra er een houding bij komt, en dan zegt het scherm iets
// anders dan de feed. Zie de toelichting bij EEclipseStance zelf.
#include "Core/EclipseEventPayloads.h"

/**
 * Pure core van de SPELERLAAG van de HUD (GDD 14.5 stap 2, 14.3.2 headless-regel).
 *
 * Geen engine-actor-headers en geen widgets: dit bestand zet ruwe wapen- en
 * schermfeiten om in de tekst en de maten die op het scherm horen te komen. De
 * widget bezit pixels; élke beslissing over WAT er staat leeft hier, zodat de
 * headless testlaag hem kan afvegen zonder wereld.
 *
 * Waarom dat hier scherper telt dan elders: de drie defecten die de
 * schermbeoordeling vond, waren alledrie beslissingen die in een opmaakfunctie
 * verstopt zaten — "toon HERLADEN in plaats van de kogels", "plak de rijnaam op het
 * scherm", "zet hem 48 px van de rand". Geen daarvan was toetsbaar zonder de game
 * te starten, en dus is geen daarvan ooit getoetst.
 *
 * BRONTAAL IS ENGELS. `13_roadmap.md` r45 zet NL juist bij de DOELtalen van de
 * lokalisatie (FIGS+NL+PL+BR+RU+ZH) naast "EN VO" — de brontekst van het spel is
 * dus Engels, en `HERLADEN` stond hier als Nederlandse string in een
 * NSLOCTEXT-brontekst. Alles wat de speler leest gaat door FText/NSLOCTEXT, zodat
 * de Nederlandse versie later een VERTALING is en geen tweede brontekst.
 */
namespace EclipseHudReadout
{
	// ---------------------------------------------------------------------------
	// TITEL-VEILIGE MARGE (defect 4)
	// ---------------------------------------------------------------------------
	//
	// GEMETEN op HUD_wapen_E_na_wissel.png (1280x720, 31-07 20:36): de wapenregel
	// rechtsonder raakt beeldkolom 1278 van 1279 — marge 1 px — en de "12 / 12"
	// erachter staat helemaal buiten beeld. De regel linksboven begint op kolom 9,
	// dat is 0,7 % van de breedte.
	//
	// De norm die de industrie hiervoor heeft, en die niet uit mijn duim komt:
	//   action-safe  ~3,5 % van elke rand  (alles wat MEEDOET moet hierbinnen)
	//   title-safe   ~5,0 % van elke rand  (alle TEKST hierbinnen)
	// Dat is de EBU/SMPTE-conventie die ook Unreal's eigen "TV Safe Zone"-debug
	// tekent. Wij zetten TEKST neer, dus de strengste van de twee geldt.
	//
	// Waarom een FRACTIE en niet een vast aantal pixels: 48 px is 3,8 % op 1280
	// breed en 1,7 % op 2560. Een vaste marge verhuist dus mee met de resolutie en
	// is op een groot scherm geen marge meer. Dat is precies waarom er twee
	// verschillende getallen in dit bestand stonden (12 px linksboven, 48/32 px
	// rechtsonder) zonder dat iemand kon zeggen welke de juiste was.
	inline constexpr float TitleSafeFraction = 0.05f;
	inline constexpr float ActionSafeFraction = 0.035f;

	/** Onder- en bovengrens van de marge in pixels; zie TitleSafeMarginPx. */
	inline constexpr float MinSafeMarginPx = 16.0f;
	inline constexpr float MaxSafeMarginPx = 72.0f;

	/**
	 * De titel-veilige marge in pixels voor een scherm van deze maat.
	 *
	 * Onder- en bovengrens, want beide uitersten zijn echt: op een klein
	 * debugvenster van 640x360 zou 5 % nog geen 18 px zijn (dan raakt een letter met
	 * rand de rand alsnog), en op 4K zou hij 108 px worden en het beeld leegtrekken.
	 */
	ECLIPSE_API FVector2D TitleSafeMarginPx(const FVector2D& ViewportSizePx);

	// ---------------------------------------------------------------------------
	// DE MUNITIETELLER (defect 1 en 3)
	// ---------------------------------------------------------------------------

	/** Wat er van het wapen bekend is op het moment dat de teller getekend wordt. */
	struct FEclipseWeaponReadoutFacts
	{
		/** De LEESBARE naam uit de data (DT_Weapons.DisplayName), niet de rijnaam. */
		FText DisplayName;

		/** De rijnaam, als terugval en voor de diagnose. */
		FName RowName;

		int32 AmmoInMagazine = 0;
		int32 MagazineSize = 0;

		bool bReloading = false;

		/** 0..1 door de herlaadbeurt heen; buiten een beurt betekenisloos. */
		float ReloadProgress = 0.0f;

		/** Hoeveel wapens de speler bij zich draagt; 1 = geen wisselknop. */
		int32 SlotCount = 1;
	};

	/** Wat de velden van de teller moeten zeggen. Leeg = dat veld hoort weg. */
	struct FEclipseAmmoReadout
	{
		/**
		 * "23 / 30" — VERDWIJNT NOOIT, ook niet tijdens het herladen.
		 * FString en geen FText: hier staan alleen cijfers en een schuine streep, er
		 * is niets te vertalen, en FText::AsNumber zou er op sommige locales een
		 * duizendtalscheiding in zetten.
		 */
		FString AmmoText;

		/** Alleen het magazijngetal, zodat de vormlaag het groot kan zetten. */
		FString MagazineText;

		/** Alleen de magazijnmaat ("/ 30"), zodat het kleiner mag dan het getal ervoor. */
		FString CapacityText;

		/** De leesbare wapennaam; leeg als er maar één wapen te dragen valt. */
		FText WeaponText;

		/** "RELOADING", of leeg buiten een herlaadbeurt. */
		FText ReloadText;

		/** Hoort de hele teller weg? (geen wapen, of een wapen zonder magazijn) */
		bool bHidden = false;

		/** Onder een derde van het magazijn: kleuren, want dan moet je BESLUITEN. */
		bool bLow = false;

		/** Leeg magazijn: de scherpste toestand, en de enige waarin de trekker niets doet. */
		bool bEmpty = false;

		/** 0..1 voor de voortgangsbalk; 0 buiten een beurt. */
		float ReloadProgress = 0.0f;
	};

	/**
	 * De teller samenstellen.
	 *
	 * APARTE VELDEN EN NIET ÉÉN, en dat is de reparatie van defect 1. Er stond één
	 * tekstblok dat tijdens het herladen volledig werd overschreven met "HERLADEN";
	 * daarmee is precies op het moment dat je wilt weten hoeveel er in zit, niet te
	 * zien hoeveel er in zit. Eén veld voor twee feiten is geen opmaakkeuze maar een
	 * ontwerpfout: de twee feiten gelden tegelijk.
	 *
	 * DE NAAM KOMT UIT DE DATA, en dat is de reparatie van defect 3. Stond hier de
	 * rijnaam, dan leest de speler "Sidearm_Scrap" — een interne sleutel, mét
	 * underscore. Ontbreekt de leesbare naam, dan valt deze functie terug op een
	 * OPGEPOETSTE rijnaam (underscores eruit) in plaats van op de ruwe: onbekende
	 * data hoort te degraderen naar iets leesbaars, nooit naar een sleutel (14.3.5).
	 * Of de data ontbrak, is aan `DisplayNameCameFromData` te vragen — dat is wat de
	 * validator en het log kunnen aangrijpen.
	 */
	ECLIPSE_API FEclipseAmmoReadout ComposeAmmoReadout(const FEclipseWeaponReadoutFacts& Facts);

	/** True zodra de leesbare naam echt uit de data komt; false = er is opgepoetst. */
	ECLIPSE_API bool DisplayNameCameFromData(const FEclipseWeaponReadoutFacts& Facts);

	/**
	 * Rijnaam -> leesbare tekst: "Sidearm_Scrap" wordt "Sidearm Scrap".
	 *
	 * Bewust alleen scheidingstekens en niets slimmers. Een functie die probeert te
	 * RADEN hoe een wapen zou moeten heten, verzint content — en dan staat de echte
	 * naam nergens in de data en kan hij ook nooit vertaald worden.
	 */
	ECLIPSE_API FString HumaniseRowName(FName RowName);

	// ---------------------------------------------------------------------------
	// DE GEZONDHEIDSUITLEZING (N-c)
	// ---------------------------------------------------------------------------
	//
	// GEMETEN op 01-08 in alle tien geopende HUD-frames van 23:34: linksonder staat
	// NIETS. Geen balk, geen getal, geen schade-indicatie. `REFERENTIE_HUD_BORDERLANDS.md`
	// r36-37 wijst die hoek expliciet aan gezondheid toe.
	//
	// Het was geen datagat: `Event.Player.VitalsChanged` bestaat sinds `4747010` met
	// payload én tests. De feed was er, de consument niet — precies de vorm die de
	// munitiehoek had. Dit is de pure helft van die consument: alles wat een BESLISSING
	// is over wat er komt te staan, zodat het toetsbaar is zonder game.

	/** Onder deze fractie kleurt de uitlezing: hier moet je BESLUITEN (zelfde grens als het magazijn). */
	inline constexpr float LowHealthFraction = 1.0f / 3.0f;

	/** Wat er van het spelerslichaam bekend is; één op één de velden van FEclipsePlayerVitalsPayload. */
	struct FEclipseVitalsReadoutFacts
	{
		/**
		 * Is er ooit een feit binnengekomen? Zonder feit hoort er NIETS te staan.
		 *
		 * Apart veld en niet "MaxHealth > 0" als vervanger: die twee zijn verschillende
		 * vragen, en de HUD moet ze kunnen scheiden. Een lichaam waarvan het maximum nog
		 * niet is toegekend geeft de bus nog niets — een balk die dan alvast "0 / 0"
		 * toont, meldt "je bent dood" op het moment dat er alleen nog niets bekend is.
		 */
		bool bHasFact = false;

		float Health = 0.0f;
		float MaxHealth = 0.0f;

		/** Waar de gezondheid vandaan komt; het TEKEN van het verschil scheidt de klap van het verband. */
		float PreviousHealth = 0.0f;

		EEclipseStance Stance = EEclipseStance::Standing;

		bool bDowned = false;

		/** Dít feit is de overgang zelf — de enige manier om "neer gegaan" van "ligt al" te scheiden. */
		bool bDownedChanged = false;
	};

	/** Wat de velden linksonder moeten zeggen. Leeg = dat veld hoort weg. */
	struct FEclipseVitalsReadout
	{
		/** "62" — het grote getal. Geen FText: het zijn cijfers, en AsNumber zet er op sommige locales een scheiding in. */
		FString HealthText;

		/** "/ 100" — staat vast, dus kleiner en gedimd ernaast. */
		FString CapacityText;

		/** 0..1 voor de balk. Het getal en de balk komen uit dezelfde bron en kunnen dus niet uiteenlopen. */
		float HealthFraction = 0.0f;

		/** "STANDING" / "CROUCHED" / "SPRINTING" / "IN COVER"; LEEG zolang het lichaam neer ligt. */
		FText StanceText;

		/**
		 * "DOWN" of "STABILIZED", of leeg.
		 *
		 * TWEE ONDERSCHEIDBARE TOESTANDEN EN NIET ÉÉN "GEWOND", en dat is de harde eis
		 * van deze stap. Neergaan en gestabiliseerd worden hebben allebei een lage
		 * gezondheid en zijn met een balk alleen niet uit elkaar te houden — terwijl het
		 * verschil tussen "ik lig" en "ik sta weer" het enige is waar je op dat moment
		 * naar handelt.
		 */
		FText StatusText;

		/** Hoort de hele uitlezing weg? (nog geen feit, of een lichaam zonder maximum) */
		bool bHidden = false;

		/** Onder LowHealthFraction: kleuren. */
		bool bLow = false;

		bool bDowned = false;

		/** Dit feit IS de herstelmelding; de vormlaag mag hem anders kleuren dan "neer". */
		bool bStabilized = false;
	};

	/**
	 * De gezondheidsuitlezing samenstellen.
	 *
	 * NAAR BOVEN AFGEROND EN NIET REKENKUNDIG, en dat is geen kosmetiek: bij 0,4 hp zou
	 * afronden "0" op het scherm zetten terwijl je nog leeft, en dat is een leugen in
	 * precies de richting waarin hij het duurst is. Naar boven afronden liegt de andere
	 * kant op — hooguit één punt te gunstig — en houdt "0" gereserveerd voor werkelijk nul.
	 */
	ECLIPSE_API FEclipseVitalsReadout ComposeVitalsReadout(const FEclipseVitalsReadoutFacts& Facts);

	/** Houding -> het woord op het scherm. Brontaal Engels, net als de rest van de spelerlaag. */
	ECLIPSE_API FText StanceLabel(EEclipseStance Stance);

	// ---------------------------------------------------------------------------
	// HET RICHTKRUIS (defect 5)
	// ---------------------------------------------------------------------------
	//
	// GEMETEN op alle elf HUD-frames van 31-07 20:36: het kruis is een tekstglyph
	// van 7x9 px met 17 pixels boven luminantie 200 — 0,0018 % van een beeld van
	// 1280x720 — en zijn inkt staat 1 px BOVEN het echte schermmidden, want een
	// '+'-glyph is niet gecentreerd op zijn eigen vak. Het is dus niet primair een
	// contrastprobleem (de donkere rand is er en meet 0,0) maar een MAAT-probleem.
	//
	// Vandaar: geen glyph meer maar geometrie. Vier balken rond een gat in het
	// midden, elk met een donkere rand — de vorm die Borderlands, Destiny en Halo
	// alledrie gebruiken, en om dezelfde reden: het gat houdt je doelwit vrij
	// terwijl de balken in je ooghoek blijven staan.

	/** De vorm van het kruis, per wapentype (REFERENTIE_HUD_BORDERLANDS.md §2). */
	enum class EEclipseCrosshairShape : uint8
	{
		/** Rechtopstaand kruis: enkelschot, burst, automatisch. */
		Cross,
		/** Wijde haken: hagel en alles met veel spreiding. */
		Brackets,
		/** Fijne punt met lange armen: precisie op afstand. */
		Precision
	};

	/** Alles wat de tekenlaag nodig heeft om het kruis te leggen; puur uit feiten afgeleid. */
	struct FEclipseCrosshairLayout
	{
		EEclipseCrosshairShape Shape = EEclipseCrosshairShape::Cross;

		/** Halve lengte van één balk, in pixels. */
		float ArmLengthPx = 7.0f;

		/** Dikte van een balk, in pixels. Altijd oneven-vriendelijk t.o.v. het midden. */
		float ThicknessPx = 2.0f;

		/** Afstand van het schermmidden tot waar een balk begint — het gat. */
		float GapPx = 5.0f;
	};

	/**
	 * De spreiding van het wapen in graden -> het gat in pixels.
	 *
	 * Het kruis moet ZEGGEN hoe zuiver het volgende schot is; dat is wat een
	 * state-reactive vizier betekent. De omrekening is de standaard kegelprojectie:
	 * een halve spreidingshoek van A graden dekt op schermafstand een straal van
	 * tan(A) * (halve schermhoogte / tan(halve FOV)).
	 */
	ECLIPSE_API float SpreadDegreesToGapPx(float SpreadDegrees, float ViewportHeightPx, float VerticalFovDegrees);

	/** Wapenprofiel -> vorm en maten van het kruis. */
	ECLIPSE_API FEclipseCrosshairLayout ComposeCrosshair(
		float SpreadDegrees, int32 PelletsPerShot, float AimSpreadDegrees,
		float ViewportHeightPx, float VerticalFovDegrees);
}
