#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"
#include "UI/EclipseBaseViewLogic.h"
#include "EclipseBaseGridWidget.generated.h"

/**
 * HET SLOTRASTER ALS TEGELS (`phase0/REFERENTIE_BASE_MAP.md` §2.3 rij 1).
 *
 * WAAROM DIT GEEN LIJST TEKSTREGELS IS. Schaarste is de kern van de
 * bouwvolgorde, en schaarste is een VERHOUDING: twee bezet, twee vrij, vier
 * dichtgemetseld. Een lijst dwingt je die verhouding te tellen; een raster laat
 * hem zien. Dat is exact het onderscheid dat §1.5 voor de kaart maakt — tekst
 * is beter in exacte getallen, een vorm is beter in verhoudingen — en het geldt
 * hier net zo goed. De tekstregels blijven daarom náást dit raster staan.
 *
 * WAT HET NIET DOET, en dat is de bouwvolgorde 14.5: het beslist niets. Welke
 * slots er zijn, welke bezet zijn, hoe ver een bouw is en wat rushen kost komt
 * kant-en-klaar uit `EclipseBaseView::ComposeBaseView` — headless getoetst in
 * `Tests/EclipseBaseViewTests.cpp`. Dit bestand bezit alleen kleur, vorm en
 * plaatsing.
 *
 * VORM DRAAGT STATUS, NIET ALLEEN KLEUR. Elke toestand heeft een eigen SILHOUET,
 * precies zoals de kaartlaag zijn gestreepte smokkellane en zijn dwarsbalk op
 * een gepoorte lane heeft:
 *
 *   SEALED   dichte kruisarcering over de hele tegel — je ziet dat er niets kan
 *   EMPTY    onderbroken omtrek, donkere kern — een omlijnd gat
 *   BUILDING massieve voortgangsbalk + gevarenstrepen op het deel dat nog moet
 *   ONLINE   dikke accentbalk langs de linkerrand, gevulde tegel
 *   DAMAGED  zware diagonale striemen over de tegel — kapot leest als kapot
 *
 * Dat is de eis uit `15_visual_quality_charter.md`: wie kleuren slecht
 * onderscheidt, moet dit scherm nog steeds kunnen lezen.
 *
 * ALLES IN LAYOUT-EENHEDEN, NOOIT IN SCHERMPIXELS. GEMETEN 01-08: UMG rendert
 * op 720p met DPI-schaal ~0,71, dus een blad dat 440x300 vraagt komt er als
 * 311x213 uit. Dat geldt ook voor korpsgroottes — een 11-punts label leest als
 * ~8 px en verdwijnt.
 */
UCLASS()
class ECLIPSE_API UEclipseBaseGridWidget : public UWidget
{
	GENERATED_BODY()

public:
	UEclipseBaseGridWidget();

	/**
	 * Het raster dat getekend moet worden. Kopie en geen verwijzing: de view
	 * komt uit een lokale `ComposeBaseView` in de hub-refresh en zou als pointer
	 * een frame later al dood zijn — dezelfde reden als bij het kaartbord.
	 */
	void SetGrid(const EclipseBaseView::FEclipseBaseView& InView);

	/** Hoeveel tegels de laatste verf trok. Nul terwijl er slots zijn = een stille tekenfout. */
	int32 GetDrawnTileCount() const;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<class SEclipseBaseGrid> Grid;

	/** Bewaard, want RebuildWidget kan ná SetGrid komen (en doet dat ook). */
	EclipseBaseView::FEclipseBaseView PendingView;
};
