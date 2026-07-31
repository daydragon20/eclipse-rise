#pragma once

#include "CoreMinimal.h"

class AStaticMeshActor;
class UWorld;

/**
 * HET INSLAGSPOOR — ÉÉN RECEPT, ÉÉN PLEK.
 *
 * WAAROM DIT BESTAND BESTAAT. Tot 31-07 stonden er TWEE onafhankelijke bouwers
 * van hetzelfde spoor in de code: `UEclipseHitscanWeaponComponent::SpawnImpactMark`
 * (gemaskerd, met de rotatie uit de oppervlaknormaal) en
 * `AEclipseGameMode::OnWorldImpact` (ander materiaal, geen rotatie). Ze vuurden
 * allebei op elke wereldtreffer, dus er stonden 22 objecten bij 11 treffers, exact
 * samenvallend.
 *
 * DAT IS TWEE FOUTEN IN ÉÉN. Het is een duplicaat in VERSCHEEPT gedrag — de
 * game-mode-versie draagt geen normaal (de payload heeft er geen), dus op een muur
 * legt hij zijn spoor plat in wereld-Z in plaats van tegen het oppervlak. En het
 * is een SABOTAGE VAN DE DIAGNOSE: de meetmethode van dossier 4.3 verbergt precies
 * één object en telt de veranderde pixels. Bij een exacte tweeling vult de tweede
 * het gat, en elk differentieel leest nul — ongeacht hoe groot of hoe fel het spoor
 * is. Zolang die twee er allebei stonden was geen enkele maatwijziging meetbaar.
 *
 * De winnaar is de gemaskerde met rotatie, en die woont nu hier: één recept dat
 * het spel én het meetharnas gebruiken. Dat laatste is niet netheid maar de kern
 * van de meting — een harnas dat zijn eigen kopie van het recept bouwt, meet een
 * object dat niemand ooit ziet (`meten-voor-je-concludeert`: authored ≠ verscheept).
 */
namespace EclipseImpactMark
{
	/**
	 * De maten van één spoor. Een struct en geen losse constanten, zodat de
	 * NULMETING en het VERSCHEEPTE spoor door exact dezelfde code lopen en het
	 * enige verschil tussen die twee de getallen zijn.
	 */
	struct FRecipe
	{
		/** Naam in het log, zodat een meting nooit los van zijn recept staat. */
		const TCHAR* Naam = TEXT("verscheept");

		/**
		 * Breedte van het VLAK waar de inktring op staat, in cm.
		 *
		 * LET OP HET VERSCHIL TUSSEN DEZE MAAT EN WAT JE ZIET. Het masker bepaalt
		 * welk deel van het vlak dekt: T_impact_ring_mask dekt van r 0,79 tot 0,91,
		 * dus de zichtbare ring is 0,91 van dit getal breed. Die twee door elkaar
		 * halen kostte de eerste meetronde — "vulling 15 cm" was in werkelijkheid
		 * 8 cm en er zat een doorzichtig gat van 6 cm omheen.
		 */
		float FootprintCm = 28.0f;

		/** Breedte van het vlak van de hete vulling, in cm. Masker dekt 0,81 daarvan. 0 = geen vulling. */
		float FillCm = 26.0f;

		/** Breedte van de opstaande kern, in cm. 0 = geen kern (volledig plat). */
		float CoreCm = 18.0f;

		/** Hoe ver de kern BOVEN het oppervlak uitsteekt, in cm. */
		float CoreRiseCm = 11.0f;

		/**
		 * Vermenigvuldiger op het masker vóór de saturate in M_EclipseToonDecal.
		 * Boven 1 wordt de zachte flank van een masker afgekapt en dus HARD; dat
		 * is een echte knop op randscherpte en niet alleen op dekking.
		 */
		float OpacityScale = 3.0f;

		/** Pad naar het masker van het buitenvlak (de ring, of bij de nulmeting de blob). */
		const TCHAR* RingMask = TEXT("/Game/Art/Decals/T_impact_ring_mask.T_impact_ring_mask");

		/** Pad naar het masker van de vulling. Genegeerd als FillCm 0 is. */
		const TCHAR* FillMask = TEXT("/Game/Art/Decals/T_impact_core_mask.T_impact_core_mask");

		/** Kleur van het buitenvlak. Donker = inktlijn (15.5), niet contrast. */
		FLinearColor RingColor = FLinearColor(0.045f, 0.030f, 0.022f, 1.0f);

		/** Kleur van vulling en kern. Heet, want de wijk staat unlit. */
		FLinearColor HotColor = FLinearColor(1.00f, 0.62f, 0.22f, 1.0f);

		/**
		 * Emissie van de hete delen. De ring krijgt altijd 1,0 — die moet donker blijven.
		 *
		 * 20 en niet 10, en dat is geen smaak maar een randmaatregel. Op 15 m was de
		 * sterkste pixelverandering 57/255 tegen 195 op 8 m: op afstand bestaat het
		 * spoor bijna alleen nog uit GEDEELTELIJK bedekte pixels, en die mengen met
		 * het wegdek. Een fellere bron tilt precies die randpixels boven de drempel
		 * uit — en doet voor het oog hetzelfde. Meer emissie koopt hier dus
		 * leesbaarheid zonder ook maar één centimeter extra grond te beplakken, en
		 * dat is de goedkoopste van de drie knoppen.
		 */
		float HotEmissive = 20.0f;

		float LifeSeconds = 2.5f;
	};

	/** Het spoor dat de speler krijgt. */
	const FRecipe& Verscheept();

	/**
	 * HET SPOOR ZOALS HET WAS: 9 cm, plat, blob-masker, geen kern.
	 *
	 * Bestaat uitsluitend voor de CONTROLEPROEF. Een meting die na een wijziging
	 * alles groen noemt, meet niets meer — dus moet dezelfde ladder een spoor dat
	 * aantoonbaar TE KLEIN is nog steeds als te klein aanwijzen. Dit is dat spoor,
	 * en het is bovendien de banked nulmeting (14 px op 1038 cm), dus als de ladder
	 * dat getal terugvindt is de ladder zelf geijkt.
	 */
	const FRecipe& Nulmeting();

	/**
	 * Zet één spoor neer op `ImpactPoint`, uitgelijnd op `Normal`.
	 *
	 * De actor-oorsprong landt op `ImpactPoint + Normal * 1 cm` — die lift staat er
	 * tegen z-fighting en `Eclipse.Combat.ImpactMarkLandsOnTheHitAndNotOnTheShooter`
	 * meet erop. Geeft nullptr en logt een reden als er iets niet te laden was;
	 * een ontbrekende asset mag nooit een crash worden.
	 */
	AStaticMeshActor* Spawn(UWorld& World, const FVector& ImpactPoint, const FVector& Normal,
		const FRecipe& Recipe);
}
