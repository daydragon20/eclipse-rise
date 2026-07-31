#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * MEETGEREEDSCHAP VOOR HET DOSSIER "INSLAGSPOOR RENDERT NIET" (DEBUG_DISCIPLINE 4.3).
 * Dit bestand REPAREERT NIETS. Het meet twee dingen die het dossier zelf als open
 * benoemt en die nog nooit gemeten zijn.
 *
 * A. VRIJ ZICHT — `TraceFreeSight`. De regel [PLAYSHOT n SPOREN] meldt `inbeeld=1`,
 *    en dat zegt alleen dat een punt binnen de kijkkegel valt: niets over wat er
 *    vóór dat punt staat. Deze functie maakt daar een ja/nee van, mét de naam van
 *    de blokkeerder.
 *
 * B. RENDERT DE GRONDVLAK-LAAG — `ArmGroundProof`. Van de grondvlakken die
 *    EclipseGrayboxBuilder neerzet is nooit vastgesteld DÁT ze renderen. "Ik zie
 *    iets grijs" is geen meting: grijs asfalt onder een grijs vlak ziet er
 *    hetzelfde uit als grijs asfalt zonder vlak.
 *
 *    DE METING SCHEIDT DIE TWEE DOOR VERSCHIL. Twee opnames van exact hetzelfde
 *    beeld, waarvan er tussenin PRECIES ÉÉN ding verandert: het vlak wordt
 *    verborgen. Elke pixel die daardoor verandert is per constructie veroorzaakt
 *    door dat vlak — achtergrond die toevallig grijs is kan niet meebewegen met
 *    een schakelaar die hij niet kent. Nul veranderde pixels betekent dat het vlak
 *    niets aan het beeld bijdraagt.
 *
 *    EN DE CONTROLEPROEF DRAAIT EERST, in drie vormen, want een verschilmeting die
 *    altijd nul zegt meet niets:
 *      - ruisvloer: twee opnames zónder wijziging. Moet ~0 zijn, anders is elk
 *        ander getal ruis.
 *      - zichtbaar vlak: een onmiskenbaar vlak op exact dezelfde plek en maat.
 *        Moet ≫ ruisvloer zijn, anders kan deze methode een zichtbaar vlak niet
 *        aanwijzen en is een nul op de verdachte betekenisloos.
 *      - buiten beeld: hetzelfde vlak achter de camera. Moet ~ruisvloer zijn,
 *        anders reageert de methode op iets anders dan wat ze verbergt.
 */
namespace EclipseRenderProof
{
	/**
	 * Vrij zicht van `From` naar het middelpunt van `Target`?
	 *
	 * Geeft true als er niets tussen staat. Bij false vullen `OutBlocker` (naam +
	 * eerste tag van de blokkeerder) en `OutBlockDistance` (cm vanaf `From`) wat
	 * het zicht wegneemt. `Ignore` gaat mee in de trace-uitsluiting — de pion en de
	 * camera-actor horen daar altijd in, anders blokkeert de kijker zichzelf.
	 */
	bool TraceFreeSight(const UWorld& World, const FVector& From, const AActor& Target,
		const TArray<const AActor*>& Ignore, FString& OutBlocker, float& OutBlockDistance);

	/**
	 * Wapent de grondvlak-renderproef als `-EclipseGrondProef` op de commandline
	 * staat. Zonder die vlag gebeurt er niets — dit mag nooit spelgedrag raken.
	 */
	void ArmGroundProof(UWorld& World);

	/**
	 * C. DE MAATLADDER — `ArmMarkLadder`, achter `-EclipseSpoorLadder`.
	 *
	 * Dossier 4.3 is met A en B beslecht: het spoor rendert, het staat goed, en het
	 * is vanaf de spelercamera 14 pixels groot. De vraag die overblijft is een MAAT,
	 * en die is met het harnas hierboven niet te beantwoorden — dat meet één spoor
	 * op de afstand waar het toevallig landde. "Leest op 8 m maar weg op 15 m" is
	 * dan onzichtbaar, en dat is geen oplossing maar een verplaatsing.
	 *
	 * Deze ladder zet hetzelfde VERSCHEEPTE spoor (EclipseImpactMark::Spawn, niet
	 * een nabouw) op 8, 10 en 15 m van het echte oog van de speler en telt de
	 * pixels met exact dezelfde differentiële methode.
	 *
	 * EN OP ELKE SPORT DRAAIT DE CONTROLEPROEF MEE: het OUDE spoor van 9 cm, uit
	 * dezelfde code met andere getallen, vanuit dezelfde camerapositie in hetzelfde
	 * venster. Een ladder waarop na de wijziging alles groen is, meet niets meer;
	 * deze moet de nulmeting op elke sport nog steeds afkeuren. Dat de nulmeting
	 * daarbij zijn eigen banked getal (14 px op ~10 m) terugvindt, ijkt de ladder.
	 */
	void ArmMarkLadder(UWorld& World);
}
