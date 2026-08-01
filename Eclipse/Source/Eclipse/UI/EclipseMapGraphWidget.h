#pragma once

#include "Components/Widget.h"
#include "CoreMinimal.h"
#include "UI/EclipseStrategyMapLogic.h"
#include "EclipseMapGraphWidget.generated.h"

/**
 * DE TEKENLAAG: knopen op posities, lanes als lijnen (`REFERENTIE_BASE_MAP.md`
 * §1.4 rij 1).
 *
 * WAAROM DIT GEEN SAMENSTELLING VAN UMG-WIDGETS IS. Een lijn tussen twee punten
 * bestaat niet in UMG: je zou hem moeten namaken met een geroteerde,
 * geschaalde afbeelding per lane, en dan is elke lane een widget met een
 * transform die niemand kan nalezen. Slate kan wél lijnen (`MakeLines`), dus
 * tekent dit ding zichzelf. Het is bewust een BLAD (`SLeafWidget`): het heeft
 * geen kinderen, alleen verf.
 *
 * WAT HET NIET DOET, en dat is de bouwvolgorde 14.5: het beslist niets. Welke
 * knopen er zijn, waar ze staan, welke lanes er lopen en wat ze kosten komt
 * kant-en-klaar uit `EclipseStrategyMap::ComposeMapView` — headless getoetst,
 * zonder viewport. Dit bestand bezit alleen kleur, dikte en plaatsing.
 *
 * STIJL (15.5, O-6 = A: de Borderlands-lock blijft). Elke lijn en elke knoop
 * krijgt eerst een dikke zwarte INKTLAAG en daarna de kleur — hetzelfde gebaar
 * als de outline op de 3D-geometrie, hier met de hand omdat een post-effect een
 * widget per definitie nooit haalt. Eén palet: de eigendom-trichotomie voor de
 * knopen, de lane-status voor de lijnen, precies de kleuren die de tekstregels
 * eronder al gebruiken.
 */
UCLASS()
class ECLIPSE_API UEclipseMapGraphWidget : public UWidget
{
	GENERATED_BODY()

public:
	UEclipseMapGraphWidget();

	/**
	 * Het bord dat getekend moet worden. Kopie en geen verwijzing: de view komt
	 * uit een lokale `ComposeMapView` in de widget-rebuild en zou als pointer een
	 * frame later al dood zijn.
	 */
	void SetBoard(const EclipseStrategyMap::FEclipseMapView& InView);

	/** Hoeveel lijnen de laatste verf trok. Nul terwijl er lanes zijn = een stille tekenfout. */
	int32 GetDrawnEdgeCount() const;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<class SEclipseMapGraph> Graph;

	/** Bewaard, want RebuildWidget kan ná SetBoard komen (en doet dat ook). */
	EclipseStrategyMap::FEclipseMapView PendingView;
};
