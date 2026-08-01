#pragma once

#include "CoreMinimal.h"

class UButton;
class UOverlay;
class UTextBlock;
class UWidget;
class UWidgetTree;

/**
 * DE PLAAT ONDER TEKST, op één plek voor de hele schermlaag.
 *
 * WAAROM DIT EEN EIGEN BESTAND IS. Op 01-08 zijn drie bevindingen met dezelfde
 * oorzaak gemeten (`phase0/SHOT_FINDINGS.md`, blok bovenaan): er ligt nergens
 * een plaat onder tekst, dus de leesbaarheid hangt af van wat er toevallig
 * achter staat. De HUD-hoogte is toen gerepareerd (commit `d766712`) — met de
 * kleuren en de maten hard in dat bestand. Bij de tweede hoogte (basis + kaart)
 * zouden diezelfde getallen opnieuw ergens moeten staan, en dan zijn het twee
 * getallen die uit elkaar kunnen lopen. Dat is precies wat de één-stijl-wet
 * (15.5) verbiedt.
 *
 * DE METING DIE DIT AFDWONG, op `Eclipse/Saved/Screenshots/HUD_volledig/HUD_hub_kaart.png`
 * met `Tools/measure_text_contrast.py` (halo-uitsluiting 2 px):
 *
 *   bordregels           1,21 : 1   ondergrond L 0,0010-0,6267, spreiding 0,0425
 *   aanbodknoppen        1,36 : 1   ondergrond L 0,4969-0,7231  <- de knoppen die je moet AANKLIKKEN
 *
 * Ter vergelijking, op hetzelfde moment, het enige element dat toen al een
 * eigen plaat had: de munitiehoek op 18,02 : 1. Dat verschil is het bewijs dat
 * dit een ontbrekend ONDERDEEL is en geen smaakvraag.
 *
 * Let op: 4,5 : 1 is de WCAG-drempel en niet de projectnorm. De projecteis is
 * `REFERENTIE_HUD_BORDERLANDS.md` r49 — afleesbaar binnen een halve seconde,
 * getoetst op een echt frame — en de harde eis van deze ronde is dat het getal
 * op ELK frame hetzelfde is, want dát is wat "hangt niet meer af van de wereld"
 * betekent.
 */
namespace EclipseScreenPlate
{
	/**
	 * De twee kleuren, letterlijk die van de HUD-plaat (`FColor(5,5,7)` inkt en
	 * `FColor(18,19,24,209)` vulling). Een derde waarde zou een ongemeten getal
	 * in dezelfde schermlaag zetten.
	 */
	ECLIPSE_API FLinearColor InkColor();
	ECLIPSE_API FLinearColor FillColor();

	/** Warme bot-inkt voor gewone schermtekst — één palet voor alle drie de hoogtes. */
	ECLIPSE_API FLinearColor BoneColor();

	/**
	 * Zet `Content` in een overlay met inkt + vulling eronder en geeft die
	 * overlay terug. De aanroeper hangt de overlay op de plek waar `Content`
	 * stond.
	 *
	 * Een OVERLAY en geen canvas: de inhoud groeit en krimpt (regio's, orders,
	 * debriefregels), en een plaat met een vaste maat is daarna of te klein of
	 * te groot. In een overlay bepaalt het grootste kind de maat en volgen de
	 * twee plaatafbeeldingen op Fill — de plaat kan de tekst dus niet mislopen.
	 */
	ECLIPSE_API UOverlay* Wrap(UWidgetTree& Tree, UWidget& Content, FName Name);

	/**
	 * De plaat volgt wat eronder ligt.
	 *
	 * VERBORGEN IS NIET AFWEZIG, en dat is gemeten geweest: de eerste HUD-plaat
	 * werd onvoorwaardelijk gebouwd en in de reviewronde op `Collapsed` gezet, en
	 * er stond nog steeds een vierkant van 21x21 px op het frame — de eigen
	 * borstelmaat van een overlay zonder zichtbare inhoud. Een plaat zonder
	 * inhoud hoort geen enkele pixel te claimen, en deze aanroep hoort dus ná
	 * het vullen te staan, niet ervoor.
	 */
	ECLIPSE_API void ApplyVisibility(UOverlay* Plate, bool bHasContent);

	/**
	 * Een knop is zijn eigen plaat. De standaard UMG-knop is lichtgrijs, en
	 * botkleurige tekst daarop mat 1,36 : 1 — de slechtste waarde op het hele
	 * scherm, op het enige element dat je moet aanklikken.
	 */
	ECLIPSE_API void StyleButton(UButton& Button, UTextBlock& Label, int32 FontSize = 14);

	/**
	 * De inktrand om een regel (15.5): een harde zwarte verschuiving van 1 px.
	 * Hetzelfde gebaar als de outline in de wereld — één visuele taal, niet een
	 * aparte voor de schermlaag. Staat náást de plaat en niet in plaats ervan:
	 * een contour houdt de letter leesbaar waar hij over zijn eigen plaat heen
	 * steekt, de plaat doet de rest.
	 */
	ECLIPSE_API void StyleLine(UTextBlock& Line, const FLinearColor& Color, int32 FontSize, bool bBold);
}
