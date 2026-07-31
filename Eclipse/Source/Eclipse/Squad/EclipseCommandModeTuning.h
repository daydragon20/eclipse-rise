#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EclipseCommandModeTuning.generated.h"

/**
 * Command Mode tunables (SPEC-P2-02 data schema — DA_CommandModeTuning). Every
 * number of the mode lives here (GDD 14.2); the Stage B fields (marks, flank
 * window, suppress radius, camera pullback) are data-ready now so the asset
 * never needs a schema change mid-phase. A missing asset degrades to these
 * defaults with a logged warning (GDD 14.3.5).
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseCommandModeTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Global time dilation while the mode is held (locked decision 1: ~30%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float DilationFactor = 0.30f;

	/**
	 * Tactician-moeilijkheidsfactor (0 = volledige pauze). **NIET GELEZEN.**
	 *
	 * Er is één moeilijkheidsgraad en geen keuzescherm, dus er is niets dat deze
	 * waarde zou kunnen kiezen. Hij staat er omdat de spec hem vastlegt; hij wordt
	 * gelezen op de dag dat moeilijkheidsgraden bestaan.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float TacticianDilationFactor = 0.0f;

	/**
	 * In- en uitvloeien van de tijdvertraging. **NIET GELEZEN — en dat is een
	 * KEUZE, geen omissie.**
	 *
	 * Command Mode knipt hard naar 30% en weer terug naar 1.0. Dat is bewust: het
	 * Stage A-oordeel is op die harde knip geveld, en de gids belooft de speler dat
	 * loslaten "exact terug op 1.0" zet. Een vloeiende overgang zou dat allebei
	 * ongeldig maken, en het is precies het soort ding dat je moet VOELEN voordat
	 * je het inruilt.
	 *
	 * Aansluiten is een half uur werk zodra de owner zegt dat de knip te hard
	 * aanvoelt. Tot die tijd zou het gedrag veranderen op een getal dat niemand
	 * heeft afgesteld.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.0))
	float EnterBlendSeconds = 0.0f;

	/** Uitvloeien. **NIET GELEZEN**, zelfde reden als EnterBlendSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.0))
	float ExitBlendSeconds = 0.0f;

	/** Direct-pick reach for soldier selection under the reticle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 100.0))
	float SoldierSelectMaxRangeCm = 10000.0f;

	/**
	 * Hoeveel doelen je tegelijk kunt markeren voor een sync strike (8.4: "up to
	 * 4 marked"). **GELEZEN sinds Stage B** — dit is de cap in
	 * `UEclipseSquadSubsystem::ToggleSyncStrikeMark`; verlagen naar 2 betekent
	 * echt dat de derde markering wordt geweigerd.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 1))
	int32 MaxSyncStrikeMarks = 4;

	/**
	 * Hoe lang een flank-voorstel op je goedkeuring wacht. **GELEZEN sinds Stage B.**
	 *
	 * Op de WANDKLOK, net als `ResponseTimeoutSeconds` en om dezelfde reden
	 * (locked decision 2): Command Mode vertraagt de wereld naar 0,30, dus een
	 * venster op speltijd zou voor de speler 3,3x langer duren dan het getal
	 * belooft — en de hele bedoeling van dit venster is dat je moet BESLISSEN.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.5))
	float FlankApprovalTimeoutSeconds = 6.0f;

	/** Straal van het gebied dat een Suppress-order onder vuur legt. **GELEZEN sinds Stage B.** */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 50.0))
	float SuppressRadiusCm = 400.0f;

	/**
	 * Hoe lang een soldaat een gebied onder vuur houdt voor de order uitdooft.
	 *
	 * Suppress heeft een EINDE nodig: een order die eeuwig doorloopt is geen
	 * onderdrukking maar een soldaat die zijn magazijn leegschiet op een muur en
	 * daarna nooit meer iets anders doet. Zonder dit getal zou de duur uit de
	 * lucht komen, en 14.2 verbiedt precies dat.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.5))
	float SuppressBurstSeconds = 5.0f;

	/**
	 * Hoe ver naast de rechte lijn een flankroute uitzwenkt.
	 *
	 * De flank is geen tweede padzoeker maar een PUNT opzij van de as
	 * soldaat→doel; is er geen route naartoe, dan weigert de order met NoRoute en
	 * zegt hij dat. Dat is de goedkope helft die aantoonbaar werkt; een echte
	 * omtrekkende beweging is EQS-werk in de AI-contentpas (12.1).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 100.0))
	float FlankOffsetCm = 900.0f;

	/**
	 * Hoe dicht een geauthord breekpunt bij je aangewezen plek moet liggen om te
	 * tellen. Geen punt binnen deze straal = de refusal `NoBreachPoint`, gesproken.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 100.0))
	float BreachPointRangeCm = 1200.0f;
};

namespace EclipseCommandMode
{
	/**
	 * Waar DA_CommandModeTuning staat — EEN pad, twee lezers.
	 *
	 * De component (`UEclipseCommandModeComponent`, Characters/) leest hem voor de
	 * tijdvertraging; de squad-subsystem leest hem voor de Stage B-getallen. Twee
	 * losse letterlijke paden zouden precies één keer uit elkaar lopen en dan
	 * onvindbaar zijn: de mode zou op het ene asset afstellen en de orders op het
	 * andere. `Eclipse.Command.StageB.TuningHasOneSource` valt om zodra dat gebeurt.
	 */
	inline const TCHAR* DefaultTuningPath = TEXT("/Game/Data/DA_CommandModeTuning.DA_CommandModeTuning");
}
