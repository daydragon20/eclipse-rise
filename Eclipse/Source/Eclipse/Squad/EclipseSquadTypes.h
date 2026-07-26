#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "EclipseSquadTypes.generated.h"

/**
 * Squad order data (SPEC-P1-06). The refusal message is as much a feature as
 * the execution (GDD 8.4/9.5): line pools live in data so personality scales
 * by content, not code.
 */

/** Phase 1 order subset (GDD 8.4). */
UENUM(BlueprintType)
enum class EEclipseSquadOrder : uint8
{
	MoveTo,
	FocusTarget,
	Hold,
	Regroup
};

/** Why an order was refused — never silence (GDD 9.5 verbal transparency). */
UENUM(BlueprintType)
enum class EEclipseOrderRefusalReason : uint8
{
	None,
	NoRoute,
	NoLineOfSight,
	InvalidTarget,
	Downed
};

/**
 * Move-order posture (SPEC-P1-06 stub: "Move to position (with stance stub:
 * ready/aggressive)"). PLACEHOLDER(GDD 8.4): drives posture/ROE and the cover-vs-
 * advance bias in the feel pass; Phase 1 stores it but splits no behavior yet.
 */
UENUM(BlueprintType)
enum class EEclipseSquadStance : uint8
{
	/**
	 * DOCTRINE (owner-opdracht 26-07 avond, punt 1 — laag 4 van zes).
	 *
	 * Geen schakelaars voor basisgedrag: elke waarde hier PERKT DE BASIS IN of
	 * LAAT HEM LOS. Dat is het verschil dat de owner aanwees — "kamikaze" betekent
	 * niet "zet aanvallen aan" maar "laat dekking zoeken weg".
	 *
	 * De set komt uit Ghost Recon (Recon / Assault / Suppress) plus het
	 * kamikaze-voorbeeld van de owner zelf. Vier, niet acht: de referentie houdt
	 * het klein omdat de basis groot is.
	 */

	/** Vuurt NIET tenzij er op hem geschoten wordt. Zoekt dekking. (GR: Recon.) */
	Recon,

	/** De volledige basis: vuurt op wat hij ziet, zoekt dekking, loopt mee. */
	Ready,

	/** Blijft staan waar hij staat. Vuurt vrij. Loopt niet mee. (FSW / GR: Suppress.) */
	Overwatch,

	/** Zoekt GEEN dekking; sluit af op de dichtstbijzijnde vijand. (Het kamikaze-kader.) */
	Aggressive
};

namespace EclipseSquad
{
	/** Eén woord per doctrine, voor de HUD en de logs. */
	inline const TCHAR* StanceLabel(EEclipseSquadStance Stance)
	{
		switch (Stance)
		{
		case EEclipseSquadStance::Recon:      return TEXT("recon");
		case EEclipseSquadStance::Overwatch:  return TEXT("overwatch");
		case EEclipseSquadStance::Aggressive: return TEXT("aggressive");
		default:                              return TEXT("ready");
		}
	}
}

/** One order's line pools (DT_SquadOrderDefs row; row name = order id). */
USTRUCT(BlueprintType)
struct FEclipseSquadOrderDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad")
	TArray<FString> AcknowledgeLines;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad")
	TArray<FString> RefusalLines;
};

/** Squad tunables (SPEC-P1-06: follow distance, cover search, refusal timeout — no hardcoded numbers). */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseSquadTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Deployed squad size including the player (SPEC-P2-01: 4 = player + 3).
	 * Preparation derives its squadmate pick count from this; fireteams (8+)
	 * are Phase 3 (GDD 4.1.6).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 1))
	int32 MaxDeployed = 4;

	/**
	 * Hoe ver een soldaat van je weg mag raken voor hij uit zichzelf bijloopt
	 * (26-07 avond, punt 1 — laag 1 van de doctrine).
	 *
	 * Hier stond tot vandaag "NIET GELEZEN: passief meelopen bestaat niet, de
	 * squad beweegt alleen op een order". Dat was waar en het was fout: de owner
	 * noemde meelopen basisgedrag, geen feature. Orders blijven beloftes — een
	 * staande Hold, MoveTo of FocusTarget wint van meelopen, dus een soldaat die
	 * je ergens neerzette blijft daar staan.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float FollowDistance = 400.0f;

	/**
	 * Zoekstraal voor dekking als er op je geschoten wordt. NOG NIET GELEZEN —
	 * dat is laag 3 van de doctrine (zie phase0/SQUAD_DOCTRINE.md); laag 1
	 * (meelopen) en laag 2 (autonoom vuren) gaan eraan vooraf.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float CoverSearchRadius = 800.0f;

	/** Ring radius sampled around an ordered point when picking cover (SPEC-P1-06 Data). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float CoverRingRadius = 200.0f;

	/** How many ring samples the cover scorer tests. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 3))
	int32 CoverRingSamples = 8;

	/** Move-to acceptance radius (how close to the cover point counts as arrived). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 1))
	float MoveAcceptanceRadius = 50.0f;

	/** Regroup acceptance radius (looser — gather near, not on, the leader). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 1))
	float RegroupAcceptanceRadius = 150.0f;

	/**
	 * Gevoelslat: een order krijgt binnen 1 s een zichtbaar antwoord (graybox feel
	 * targets §4). **NIET GELEZEN — en hij hoort het ook niet te zijn.**
	 *
	 * Dit is een DOEL, geen instelling. De code wacht nergens op deze seconde: een
	 * order krijgt zijn antwoord in hetzelfde frame (gemeten 0,000 s). Het getal
	 * staat er om de lat vast te leggen waaraan je de meting houdt, en de dag dat
	 * de code hem zou gaan lezen, zou hij van lat in vertraging veranderen.
	 *
	 * Hij dook 26-07 avond op in de dode-veldensweep doordat ik elders een comment
	 * herschreef die hem noemde — de sweep telde die vermelding als "gelezen".
	 * Dat is precies de zwakte die dit label afdekt: een naam in een comment is
	 * geen gebruik.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0.1))
	float ResponseTimeoutSeconds = 1.0f;

	/** Cover-scorer: flat bonus for a sample that blocks the threat line (P2-01 review m6 — was hardcoded). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float CoverBlockBonus = 10.0f;

	/** Cover-scorer: score weight per cm for order-distance penalty and lane bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float CoverDistanceWeightPerCm = 0.001f;

	/** Squad spawn fan: base offset from the player (P2-01 review m4 — was hardcoded). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float SpawnFanBaseCm = 150.0f;

	/** Squad spawn fan: per-soldier step so four bodies never stack in one capsule scrum. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float SpawnFanStepCm = 130.0f;

	/** Rows: FEclipseSquadOrderDefRow keyed by order id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad")
	TSoftObjectPtr<UDataTable> OrderDefs;
};
