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

/**
 * De ordertabel van 8.4 (SPEC-P2-02 Stage B maakt hem compleet).
 *
 * De eerste vier zijn de Phase 1-subset; de vijf daaronder zijn de rest van de
 * 8.4-tabel. Ze staan er ACHTER en niet ertussen omdat de rijnaam in
 * DT_SquadOrderDefs de enum-naam is en de barksalt de enum-waarde: invoegen zou
 * elke bestaande soldaat een andere stem geven zonder dat iemand iets wijzigde.
 *
 * "Regroup / Fall back" uit 8.4 is bewust GEEN eigen verb: het rally-punt is
 * Regroup met een gemikte positie als doel, en die doelplumbing bestond al.
 */
UENUM(BlueprintType)
enum class EEclipseSquadOrder : uint8
{
	MoveTo,
	FocusTarget,
	Hold,
	Regroup,

	/** 8.4 "Suppress" — gebied onder vuur houden; accepteren vraagt zicht op het gebied. */
	Suppress,

	/** 8.4 "Flank" — twee stappen: de squad rekent de route, jij keurt hem goed (of hij verloopt). */
	Flank,

	/** 8.4 "Breach" — stapelen op een geauthord breekpunt en samen naar binnen. */
	Breach,

	/** 8.4 "Ability use" — het signatuurverb van de klasse, uit data (SPEC-P2-01). */
	UseAbility,

	/** 8.4 "Sync strike" — tot MaxSyncStrikeMarks gemarkeerde doelen tegelijk, stil. */
	SyncStrike
};

namespace EclipseSquad
{
	/** Aantal orders in de 8.4-tabel — de sweep telt hier op, niet op een los getal. */
	inline constexpr int32 OrderCount = static_cast<int32>(EEclipseSquadOrder::SyncStrike) + 1;
}

/**
 * Why an order was refused — never silence (GDD 9.5 verbal transparency).
 *
 * SPEC-P2-02 Stage B voegt er precies DRIE toe, en de spec-regel eronder is dat
 * elke reden ook echt kan afgaan: een reden die geen enkele combinatie van
 * wereldfeiten oplevert, is geen reden maar een dood label. De sweep in
 * Tests/EclipseCommandStageBTests.cpp pint dat vast — per reden minstens één
 * bereikbare combinatie, en géén reden die het werk van een andere overneemt.
 */
UENUM(BlueprintType)
enum class EEclipseOrderRefusalReason : uint8
{
	None,
	NoRoute,
	NoLineOfSight,
	InvalidTarget,
	Downed,

	/** Breach zonder AEclipseBreachPoint binnen bereik — content-afwezigheid degradeert, hij zegt het (14.3.5). */
	NoBreachPoint,

	/** Sync strike zonder (nog) geldige markeringen: niets om tegelijk te doen. */
	NoTargetsMarked,

	/** Sync strike terwijl deze soldaat gezien wordt — stil toeslaan kan niet meer. */
	NotConcealed
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
	Aggressive,

	/**
	 * STEALTH (8.4 "Move / Hold — with stance (stealth/ready/aggressive)",
	 * SPEC-P2-02 Stage B; 9.5 stealth-discipline).
	 *
	 * Vuurt NIET tot je het beveelt of tot de vijand ons dóór heeft. Dat laatste
	 * is precies het verschil met Recon, en het is geen woordenspel: Recon opent
	 * het vuur zodra er op HEM geschoten wordt — Stealth doet het zodra de vijand
	 * ons überhaupt heeft opgemerkt, want vanaf dat moment koopt zwijgen niets
	 * meer. Onder Recon sta je dus nog stil te zijn terwijl vier man op je af
	 * lopen; onder Stealth vuurt de squad dan al terug.
	 *
	 * Het KADER en niet de schakelaar (owner-doctrine 26-07): dit zet geen gedrag
	 * aan, het perkt de basis in — autonoom vuren blijft de basis, stealth houdt
	 * hem tegen tot een van de twee poorten opengaat.
	 *
	 * ACHTERAAN in de enum, met opzet. De doctrine-cyclus in
	 * UEclipseCommandModeComponent::ToggleHeldStance rekent modulo vier; door
	 * Stealth erachter te zetten blijft die cyclus kloppen (Recon → Ready →
	 * Overwatch → Aggressive) in plaats van stilletjes één doctrine over te slaan.
	 * Stealth is vandaag te kiezen via `Eclipse.Squad.SetDoctrine Stealth` en via
	 * de stance-parameter van een order; hem in de toggle opnemen is één regel in
	 * Characters/ en staat als overdracht in het rapport.
	 */
	Stealth
};

namespace EclipseSquad
{
	/** Aantal doctrines — zodat een cyclus nooit op een los getal rekent. */
	inline constexpr int32 StanceCount = static_cast<int32>(EEclipseSquadStance::Stealth) + 1;

	/** Eén woord per doctrine, voor de HUD en de logs. */
	inline const TCHAR* StanceLabel(EEclipseSquadStance Stance)
	{
		switch (Stance)
		{
		case EEclipseSquadStance::Recon:      return TEXT("recon");
		case EEclipseSquadStance::Overwatch:  return TEXT("overwatch");
		case EEclipseSquadStance::Aggressive: return TEXT("aggressive");
		case EEclipseSquadStance::Stealth:    return TEXT("stealth");
		default:                              return TEXT("ready");
		}
	}

	/** Doctrine uit een woord (console + debugpad); onbekend = niets gezet. */
	inline bool ParseStance(const FString& Word, EEclipseSquadStance& OutStance)
	{
		for (int32 Index = 0; Index < StanceCount; ++Index)
		{
			const EEclipseSquadStance Candidate = static_cast<EEclipseSquadStance>(Index);
			if (Word.Equals(StanceLabel(Candidate), ESearchCase::IgnoreCase))
			{
				OutStance = Candidate;
				return true;
			}
		}
		return false;
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
