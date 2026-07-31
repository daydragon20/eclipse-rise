#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Strategy/EclipseCampaignTypes.h" // EEclipseDominionResponseTier (GDD 9.4) — pure data, no engine actors
#include "EclipseEventPayloads.generated.h"

/**
 * Typed payload structs per event family (SPEC-P1-01), carried over the bus as
 * FInstancedStruct. One struct per family (not per tag) keeps the catalog table
 * readable and lets subscribers type-check a whole family at once. Fields are the
 * Phase 1 minimum named in Docs/EventCatalog.md; emitting specs (P1-02..08) extend
 * them in their own commits — payloads are transient, never serialized, so adding
 * fields is not a save-schema change (GDD 14.3.6 does not apply).
 */

/** Event.Campaign.* — campaign clock facts (SPEC-P1-02). */
USTRUCT(BlueprintType)
struct FEclipseCampaignEventPayload
{
	GENERATED_BODY()

	/** Campaign day after the mutation that emitted this event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Day = 0;
};

/** Event.Economy.* — wallet and production facts (SPEC-P1-02/03). */
USTRUCT(BlueprintType)
struct FEclipseEconomyEventPayload
{
	GENERATED_BODY()

	/** Which resource changed (Resource.* tag family lands with SPEC-P1-03 data). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag ResourceType;

	/** Signed change applied to the balance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Delta = 0;

	/** Balance after the change — UI renders this, never recomputes (single source of truth). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 NewBalance = 0;

	/** Ledger reason line (GDD 7.6 transparency applied to money: every number has an origin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Reason;

	/** Production item this event refers to (ProductionQueued/Completed only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName ItemId;

	/** Days until completion (ProductionQueued only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 EtaDays = 0;

	/** Loadout unlock granted by a completed item (ProductionCompleted only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag LoadoutTag;
};

/** Event.Strategy.* — region-graph facts (SPEC-P1-02/04). */
USTRUCT(BlueprintType)
struct FEclipseStrategyEventPayload
{
	GENERATED_BODY()

	/** Region node id from the region graph asset (SPEC-P1-04). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName RegionId;

	// PLACEHOLDER(GDD 12.3 strategy map): owners become EEclipseRegionOwner when
	// SPEC-P1-02 defines FCampaignState; FName keeps the bus decoupled until then.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName OldOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName NewOwner;

	/** Mission template offered/selected at this region (MissionSelected only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName TemplateId;
};

/**
 * Event.Strategy.LiberationResolved — EEN rij bevrijding is gecommit.
 *
 * Eigen struct en geen veld op FEclipseStrategyEventPayload, omdat de schaal
 * verschilt: RegionControlChanged vuurt PER VAK, dit feit vuurt per RIJ. De
 * Foothold draait drie vakken om met een zin; die zin in het vak-feit zetten
 * betekent hem drie keer op het debriefscherm.
 */
USTRUCT(BlueprintType)
struct FEclipseLiberationEventPayload
{
	GENERATED_BODY()

	/** Rijnaam uit DT_LiberationInstances — het audit-spoor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName RowName;

	/** Hoeveel vakken deze rij daadwerkelijk omdraaide (niet hoeveel er in de rij staan). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 RegionCount = 0;

	/** De geauthorde zin: WAAROM dit gebied kantelde, in mensentaal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FText ContextLine;
};

/**
 * Event.Strategy.ResponseTierChanged — the empire's temperature moved (GDD 9.4).
 *
 * Its own struct, not a field on FEclipseStrategyEventPayload, for the same
 * reason LiberationResolved got one: the SCALE differs. RegionControlChanged is
 * a fact about one square; this is a fact about the whole campaign, and the
 * region id on that payload would be permanently empty here — a field that is
 * always None is a lie the compiler cannot catch.
 *
 * Carries the STEP, not just the landing: the diegetic broadcast the GDD asks
 * for ("the player learns the empire's temperature by living in it") needs to
 * know it went 2 -> 3 to pick the right propaganda line.
 */
USTRUCT(BlueprintType)
struct FEclipseResponseTierEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	EEclipseDominionResponseTier OldTier = EEclipseDominionResponseTier::Indifference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	EEclipseDominionResponseTier NewTier = EEclipseDominionResponseTier::Indifference;

	/** Campaign day the escalation landed on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Day = 0;

	/** Ledger reason — same discipline as the wallet: an escalation with no cause is unexplainable to the player (GDD 7.6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Reason;
};

/** Event.Prep.* — the full launch request composed by preparation (SPEC-P1-08). */
USTRUCT(BlueprintType)
struct FEclipsePrepEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName MissionId;

	/** Roster ids of the picked squad (MaxDeployed - 1 squadmates; player + 3 per SPEC-P2-01). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	TArray<FGuid> SquadSoldierIds;

	/** Chosen loadout (options gated by produced items — SPEC-P1-03/08). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag LoadoutTag;

	/** Chosen insertion point (one of the mission's entries — SPEC-P1-05). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName InsertionId;

	/** Intel spent on the briefing reveal (0 = none — SPEC-P1-08). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 IntelLevel = 0;
};

/** Event.Mission.* — mission lifecycle facts (SPEC-P1-05). */
USTRUCT(BlueprintType)
struct FEclipseMissionEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName MissionId;

	/** Completed objective (ObjectiveCompleted only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName ObjectiveId;

	/** Overall result (Completed/Failed). Detailed results struct lands with SPEC-P1-05. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bSuccess = false;

	/** Phase just entered (PhaseChanged only): an outer loop phase name (GDD 11.1) or a named sub-phase like "Alarm" (SPEC-P2-04). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName PhaseName;

	/** PhaseChanged only: false = outer loop phase from the mission subsystem; true = authored sub-phase (alarm now, StateTree tasks later). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bAuthoredSubPhase = false;
};

/**
 * Event.Combat.ShotFired — er is geschoten (26-07, punt 1).
 *
 * Het FEIT is de loop die knalt, niet de kogel die aankomt: een gemist schot
 * verraadt je precies zo goed als een raak schot. Dat is ook wat Borderlands en
 * The Division doen — het geluid komt van de muzzle.
 */
USTRUCT(BlueprintType)
struct FEclipseCombatEventPayload
{
	GENERATED_BODY()

	/** Wie schoot. Nodig om zijn eigen kant niet op zichzelf te laten reageren. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	TWeakObjectPtr<AActor> Shooter;

	/** Waar de loop stond op het moment van vuren — de laatst bekende positie. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FVector Origin = FVector::ZeroVector;

	/** Hoe ver dit wapen te horen is (DT_Weapons). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float AlertRadiusCm = 0.0f;

	/**
	 * ShotFired: welke geluidsfamilie er knalde, en of het gedempt was.
	 *
	 * In het FEIT en niet opgezocht door de luisteraar: de audiolaag zou anders
	 * terug moeten redeneren van actor naar component naar datatabel, en dat is
	 * precies de knoop die de bus moet voorkomen (12.2).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName WeaponSoundFamily;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bSuppressed = false;

	/**
	 * ReloadStarted: hoe lang het herladen duurt. De foley-keten verdeelt zich
	 * hierover — zonder dit getal zou de audiolaag de wapentabel moeten lezen om
	 * te weten wanneer de grendel valt.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float DurationSeconds = 0.0f;

	/** True als de schutter aan spelerskant staat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bPlayerSide = false;

	/** HitLanded: was het een kopschot? Nodig voor eigen feedback per treffersoort. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bHeadshot = false;

	/** HitLanded: hoeveel schade er geland is (na de kopschot-multiplier). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float Damage = 0.0f;

	/**
	 * HitLanded: WIE er geraakt is. Nodig om te weten of JIJ het was, en dan waar
	 * de klap vandaan kwam — de schutterpositie zit al in dit feit.
	 *
	 * Hier en niet in een tweede event: er bestond al een feit dat "iemand is
	 * geraakt" betekent, en dat had alleen het slachtoffer niet bij zich.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	TWeakObjectPtr<AActor> Victim;
};

/** Event.Squad.* — order/soldier facts in-mission (SPEC-P1-06). */
USTRUCT(BlueprintType)
struct FEclipseSquadEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid SoldierId;

	/** Order id from DT_SquadOrderDefs (SPEC-P1-06). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Order;

	/** Order target: region location id / enemy id — resolved by the order layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName TargetId;

	/** Acknowledge bark shown on screen (GDD 9.5 verbal transparency). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FString BarkLine;

	/** Refusal reason ("NoRoute", "NoLineOfSight", ...) — never empty on OrderRefused (GDD 8.4). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Reason;

	/** Cause of a SoldierDowned fact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Cause;

	/** Who performed the save (SoldierStabilized) or fired the verb (ClassAbilityUsed) — SPEC-P2-01. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid StabilizerId;

	/** Signature verb that fired (ClassAbilityUsed only; Class.Verb.* family). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag AbilityVerb;
};

/** Event.Roster.* / Event.Memorial.* — persistent-people facts (SPEC-P1-07, Pillar 3). */
USTRUCT(BlueprintType)
struct FEclipseRosterEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid SoldierId;

	/** Cause of death/wound; also the memorial "how" line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Cause;

	/** Campaign day of the fact (memorial records carry the day forever). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Day = 0;

	/** Days unavailable (SoldierWounded only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 DaysOut = 0;
};

/** Event.Base.* — Hollow Point construction/staffing facts (SPEC-P2-03), emitted only by the CampaignState commit (GDD 14.3.3). */
USTRUCT(BlueprintType)
struct FEclipseBaseEventPayload
{
	GENERATED_BODY()

	/** The authored facility slot (UEclipseBaseLayoutAsset slot-graph). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName SlotId;

	/** DT_Facilities row occupying/entering the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName FacilityId;

	/** ConstructionStarted: target level. FacilityBuilt: 1. FacilityUpgraded: the new level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Level = 0;

	/** ConstructionStarted: campaign day the build completes uncrewed (a crew or rush beats it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 EtaDay = 0;

	/** StaffAssigned: the (un)assigned soldier (same id type as FEclipseRosterEventPayload). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid SoldierId;

	/** StaffAssigned: Base.Staff.Crew / Base.Staff.Analyst (positional role at commit), empty = unassigned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag RoleTag;
};

/** Event.Story.* — committed story beats (SPEC-P2-04); emitted only by the CampaignState commit (GDD 14.3.3). */
USTRUCT(BlueprintType)
struct FEclipseStoryEventPayload
{
	GENERATED_BODY()

	/** The beat that just committed (Story.Beat.* once the content tags land; any set-only flag works). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag BeatTag;

	/** Campaign day the beat committed on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Day = 0;
};

/**
 * Wat het lichaam van de speler op DIT moment doet, als één woord.
 *
 * Eén waarde en geen losse vlaggen in de payload, omdat de schermlaag één regel
 * heeft om het in te zetten: "STAAND / HURKEN / SPRINT / DEKKING". Wie drie bools
 * doorgeeft, laat de widget de voorrangsregel bedenken — en dan bedenkt elke
 * widget hem opnieuw, net iets anders. De voorrang staat in EclipseVitalsFeed.
 *
 * InCover heeft vandaag GEEN producent, en dat is met opzet zo opgeschreven in
 * plaats van weggelaten: de dekking die er wél is (SelectCoverPointNear in de
 * squad-AI) is een BESTEMMING om heen te lopen, niet een toestand van een
 * lichaam. Zodra er een echte dekkingstoestand komt, zet die bInCover op het
 * monster aan — zonder dat het schema, de catalogus of de HUD meeverandert.
 */
UENUM(BlueprintType)
enum class EEclipseStance : uint8
{
	Standing,
	Crouched,
	Sprinting,
	InCover,
};

/**
 * Event.Player.VitalsChanged — de toestand van het spelerslichaam is veranderd.
 *
 * Bestaat omdat de HUD anders moet POLLEN (UI/EclipseMissionHudWidget deed dat
 * met de munitie) of rechtstreeks aan de pawn moet hangen. Beide breken GDD 12.2
 * regel 2: de schermlaag is een consument van de bus, niet een lezer van
 * gameplay-objecten.
 *
 * Huidige ÉN maximum, geen percentage: een balk heeft een lengte nodig en een
 * getal heeft twee helften ("62 / 100"). Een percentage gooit de helft van dat
 * antwoord weg en is niet terug te rekenen.
 *
 * Vorige én nieuwe waarde, want een overgang is het feit dat je op het scherm
 * ziet: rood knipperen hoort bij schade, niet bij genezen, en die twee zijn
 * alleen uit elkaar te houden met het getal van ervoor erbij. Zonder dat zou
 * elke widget zijn eigen kopie van de vorige toestand bijhouden — precies de
 * verborgen staat die een event-gedreven HUD moet vermijden.
 */
USTRUCT(BlueprintType)
struct FEclipsePlayerVitalsPayload
{
	GENERATED_BODY()

	/** Gezondheid na de verandering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float Health = 0.0f;

	/** Het maximum waar die gezondheid tegen afgezet hoort te worden. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float MaxHealth = 0.0f;

	/** Gezondheid vóór de verandering — het teken van het verschil is de klap of het verband. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float PreviousHealth = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	EEclipseStance Stance = EEclipseStance::Standing;

	/** De houding waaruit hij komt; gelijk aan Stance als deze gebeurtenis niet over houding ging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	EEclipseStance PreviousStance = EEclipseStance::Standing;

	/** Ligt dit lichaam? Apart van Stance: neer zijn is geen houding maar een uitkomst. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bDowned = false;

	/**
	 * WELK deel veranderde. Zonder deze drie zou de HUD elk feit als "alles opnieuw
	 * tekenen" moeten lezen, en zou een houdingswissel een schadeflits geven.
	 * bHealthChanged dekt Health én MaxHealth: allebei veranderen wat de balk toont.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bHealthChanged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bStanceChanged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bDownedChanged = false;

	/**
	 * De EERSTE foto van dit lichaam, niet een verandering eraan. De HUD hoort hier
	 * zijn beginwaarden uit te zetten zónder overgangsanimatie: bij bezetting van de
	 * pawn is er niets veranderd, er was alleen nog niets bekend. Alle drie de
	 * vlaggen hierboven staan dan op false — anders zou "vol leven" bij spawn als
	 * een treffer op het scherm landen.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bInitial = false;
};

/** Event.Command.* — Command Mode lifecycle facts (SPEC-P2-02). */
USTRUCT(BlueprintType)
struct FEclipseCommandEventPayload
{
	GENERATED_BODY()

	/** World rate applied while held (ModeEntered; from DA_CommandModeTuning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float DilationFactor = 1.0f;

	/** How long the mode was held, in wall-clock seconds (ModeExited; locked decision 2 telemetry). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float HeldSeconds = 0.0f;

	/** Orders issued during the hold (ModeExited; the R3 usage-pull metric). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 OrdersIssuedWhileHeld = 0;
};
