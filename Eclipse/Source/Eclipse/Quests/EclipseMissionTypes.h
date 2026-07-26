#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EclipseMissionTypes.generated.h"

/**
 * Mission runtime data (SPEC-P1-05). One runtime serves all authorship tiers
 * (GDD 11.1/12.3): hand-authored missions, templates and (Phase 3) the
 * generator all instantiate these same primitives.
 */

/** The four Phase 1 objective primitives (GDD 12.3: data-driven components, one runtime for all tiers). */
UENUM(BlueprintType)
enum class EEclipseObjectiveType : uint8
{
	ReachLocation,
	DestroyTarget,
	CollectItem,
	ExtractSquad
};

/** The universal mission loop's phases (GDD 11.1). */
UENUM(BlueprintType)
enum class EEclipseMissionPhase : uint8
{
	None,
	Insertion,
	Objectives,
	Extraction,
	Debrief,
	Finished
};

USTRUCT(BlueprintType)
struct FEclipseObjectiveDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	FName ObjectiveId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	EEclipseObjectiveType Type = EEclipseObjectiveType::ReachLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	FText Description;

	/** Optional objectives are the stretch layer (GDD 11.4) — never required for success. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	bool bOptional = false;

	/** World site / target this objective binds to (level data tag; SPEC-P1-05 graybox sites). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	FName TargetId;

	/**
	 * Ghost condition (SPEC-P2-04): the completed optional is voided at debrief
	 * if the alarm was EVER raised this run. Alarm never fails the mission
	 * (GDD 11.4) — it costs exactly the optionals that demanded silence.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission", meta = (EditCondition = "bOptional"))
	bool bRequiresNoAlarm = false;

	/**
	 * Zero-casualty condition (SPEC-P2-04 amendment): voided if any soldier
	 * EVER went down this run. Latch semantics — a stabilized save still went
	 * down (M1.4's strictest "no soldier downed" reading applies everywhere).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission", meta = (EditCondition = "bOptional"))
	bool bRequiresNoCasualties = false;

	/** Stretch payout, composed as AdjustResource (Reason "OptionalObjective") in the same debrief transaction. Read only when bOptional. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission", meta = (ClampMin = 0, EditCondition = "bOptional"))
	int32 OptionalRewardCredits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission", meta = (ClampMin = 0, EditCondition = "bOptional"))
	int32 OptionalRewardMaterials = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission", meta = (ClampMin = 0, EditCondition = "bOptional"))
	int32 OptionalRewardIntel = 0;
};

/**
 * Eén vijand-spawnbatch. NOG NIET AANGESLOTEN — hier stond "consumed by the
 * graybox level wiring", maar niets leest deze struct. De vijanden in een missie
 * komen uit een vaste lus van VIER in AEclipseGameMode::SpawnMissionActors, die
 * de rijen van DT_EnemyArchetypes afwisselt en ze naast het primaire doel zet.
 *
 * Gevolg voor wie een missie authordt: vul je hier "6 Enforcers op SiteB" in, dan
 * krijg je nog steeds vier gemengde vijanden bij het hoofddoel, zonder enige
 * waarschuwing. Alle drie de velden zijn dood, Count incluis.
 */
USTRUCT(BlueprintType)
struct FEclipseEnemySpawnSet
{
	GENERATED_BODY()

	/** Row in DT_EnemyArchetypes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	FName ArchetypeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission", meta = (ClampMin = 0))
	int32 Count = 0;

	/** Spawn site tag in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	FName SpawnSiteId;
};

/**
 * A mission template (GDD 11.2 catalog entry, Phase 1 subset: 1-2 mandatory
 * objectives + up to 1 optional + extraction).
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseMissionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Template id offers refer to (DT_MissionOffers.TemplateId). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	FName TemplateId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	TArray<FEclipseObjectiveDef> Objectives;

	/** Insertion choice (GDD 11.1): the graybox district's three entries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	TArray<FName> InsertionPointIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	TArray<FEclipseEnemySpawnSet> EnemySpawns;

	/** Success flips the target region one step toward the player (Dominion->Contested->Player). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Mission")
	bool bProgressRegionOnSuccess = true;
};

/** Everything the debrief needs to compose consequences (SPEC-P1-05 outcome record). */
USTRUCT(BlueprintType)
struct FEclipseMissionOutcome
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	FName TemplateId;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	FName RegionId;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	TArray<FName> CompletedObjectiveIds;

	/**
	 * Soldiers who went down in-mission; dead/wounded resolution is P1-07 policy
	 * at debrief. Stabilized soldiers stay listed — this array IS the
	 * zero-casualty latch ("ever went down"), a save changes the resolution,
	 * never the fact (SPEC-P2-04 amendment).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	TArray<FGuid> DownedSoldierIds;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	TArray<FGuid> DeployedSoldierIds;

	/** Alarm latch of the run (SPEC-P2-04): did the site EVER go loud? Voids bRequiresNoAlarm optionals at compose; never fails the mission (GDD 11.4). */
	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	bool bAlarmRaised = false;

	/** Optionals completed in the field but voided at debrief by their conditions (ghost/zero-casualty): visible as "missed", never paid. */
	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Mission")
	TArray<FName> MissedOptionalObjectiveIds;
};
