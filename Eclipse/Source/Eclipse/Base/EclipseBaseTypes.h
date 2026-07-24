#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipseBaseTypes.generated.h"

/**
 * Hollow Point base data (SPEC-P2-03, GDD 5.1-5.4 / 12.3). Slot-based inside an
 * authored shell, never freeform (5.1): the layout asset pins which facility can
 * occupy which slot, and every cost/timer/yield lives in DT_Facilities or
 * DA_BaseTuning (GDD 14.2 - a hardcoded gameplay constant is a defect).
 */

/**
 * One facility level's costs, timer and output (element of FEclipseFacilityRow.Levels;
 * index 0 = L1). The slice ships x0.8 Act-1 values (SPEC-P2-03 locked decision 3);
 * the GDD 5.3.1 numbers return with Phase 3 rebalancing - in this data, not in code.
 */
USTRUCT(BlueprintType)
struct FEclipseFacilityLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 CostMaterials = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 CostCredits = 0;

	/** Strategic-clock days uncrewed; a crew shaves DA_BaseTuning.CrewDayReduction off, never below 1 (5.3.2 staff dilemma). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 BuildDays = 1;

	/** Daily output while operational at this level, keyed by Resource.* tag (IC: +2 Resource.Intel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TMap<FGameplayTag, int32> YieldPerDay;

	/** Capability unlocked while operational (e.g. Workshop L2's manufacture tier); empty = none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FGameplayTag UnlockTag;
};

/**
 * One facility (DT_Facilities row; row name = facility id, matching the
 * DT_ClassDefs convention - SPEC-P2-03 data schema). A slot whose facility id
 * has no row here rejects gracefully at validation, never a crash (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseFacilityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FText DisplayName;

	/** Per-level data; index 0 = L1. Array length is the max level - Workshop ships 2 entries, everything else 1 (locked decision: no L2/L3 for anything but the Workshop). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FEclipseFacilityLevelData> Levels;
};

/**
 * One authored facility slot in the vault shell (SPEC-P2-03 slot-graph). The
 * streaming ids name the level instance / Data Layer swapped in per construction
 * state (GDD 12.3, 5.4 visible growth); a missing layer is a logged warning +
 * placeholder blockout, never a crash (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseBaseSlotDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FName SlotId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FText DisplayName;

	/** Which DT_Facilities rows may occupy this slot - placement is authored, build order is free (locked decision 2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FName> AllowedFacilityRows;

	/** Slot-graph edges (every slot connects to the Spine; C adds the generator room, B the memorial alcove). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FName> AdjacentSlotIds;

	/** Streaming id for the empty state (Slot B's empty state is the 5.2 bunk camp, not raw rock). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FName EmptyStateStreamingId;

	/** Streaming id while under construction (scaffold blockout, sparks, drill loop). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FName ConstructionStreamingId;

	/** Streaming id per built level; index 0 = L1 (Workshop adds an L2 entry). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FName> LevelStreamingIds;
};

/**
 * The slot-graph per base level (DA_BaseLayout_HollowPoint - SPEC-P2-03,
 * GDD 12.3 building system). Phase 2 ships the 4-slot Act 1 vault; the sealed
 * excavation faces (slots 5-8) are level dressing, not slots, until 5.2's
 * expansion lands in Phase 3.
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseBaseLayoutAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FEclipseBaseSlotDef> Slots;
};

/**
 * Base construction tunables (DA_BaseTuning - SPEC-P2-03 data schema). Rush is
 * available, never comfortable (5.4: money vs. time) - retuning happens here,
 * not in code (GDD 14.2).
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseBaseTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Rush = this x remaining days, paid in credits; the rush commit completes construction instantly (clock rules). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 RushCostCreditsPerDay = 60;

	/** Build days a construction crew shaves off, floor 1 total (locked decision 6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 CrewDayReduction = 1;

	/** Extra daily yield per staffed analyst at an operational facility that produces AnalystBonusResource (IC: +1 Intel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 AnalystIntelBonusPerDay = 1;

	/** Effective staff cap per site - both slice roles are 1 soldier (locked decision 6); Phase 3 splits this if roles diverge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 MaxCrewPerSite = 1;

	/** The resource the analyst bonus feeds (Resource.Intel in DA_BaseTuning); empty = no analyst bonus, logged once by the wrapper (GDD 14.3.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FGameplayTag AnalystBonusResource;
};
