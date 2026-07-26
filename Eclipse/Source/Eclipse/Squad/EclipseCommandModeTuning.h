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
	 * De drie Stage B-getallen: SyncStrike, Flank en Suppress. **NIET GELEZEN —
	 * die drie orders bestaan niet.**
	 *
	 * Ze staan in data omdat de spec ze vastlegt en het schema dan niet halverwege
	 * een fase hoeft te veranderen. Dat is een prima reden om ze te hebben en geen
	 * reden om te doen alsof ze iets doen: er is geen SyncStrike-order, geen
	 * flank-goedkeuring en geen onderdrukkingsgebied.
	 *
	 * Zodra die orders er komen zijn dit hun knoppen. Tot dan zijn het drie
	 * getallen die niemand kan verdraaien met gevolg.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 1))
	int32 MaxSyncStrikeMarks = 4;

	/** Stage B (Flank) goedkeuringsvenster. **NIET GELEZEN**, zie MaxSyncStrikeMarks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.5))
	float FlankApprovalTimeoutSeconds = 6.0f;

	/** Stage B (Suppress) straal. **NIET GELEZEN**, zie MaxSyncStrikeMarks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 50.0))
	float SuppressRadiusCm = 400.0f;
};
