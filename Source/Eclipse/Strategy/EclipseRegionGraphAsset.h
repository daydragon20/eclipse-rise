#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "EclipseRegionGraphAsset.generated.h"

/** District node flavors (SPEC-P1-04); each maps to one mission-offer row. */
UENUM(BlueprintType)
enum class EEclipseRegionType : uint8
{
	Industrial,
	Residential,
	Checkpoint
};

/**
 * One mission offer per region type (DT_MissionOffers, SPEC-P1-04). Offers are
 * static data in Phase 1 — the Mission Generator (GDD 11.3) replaces this table
 * in Phase 3; the context line is its hand-written stub (11.3's causal rule).
 */
USTRUCT(BlueprintType)
struct FEclipseMissionOfferRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	EEclipseRegionType RegionType = EEclipseRegionType::Industrial;

	/** Mission template this offer launches (SPEC-P1-05 asset id). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName TemplateId;

	/** One-line causal context ("generated missions must explain their existence" — GDD 11.3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FText ContextLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 RewardCredits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 RewardMaterials = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 RewardIntel = 0;
};

/**
 * Static region definitions for the 6-node district (SPEC-P1-04 data schema;
 * referenced by the campaign setup asset per SPEC-P1-02).
 */
USTRUCT(BlueprintType)
struct FEclipseRegionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	EEclipseRegionType RegionType = EEclipseRegionType::Industrial;

	/** Undirected edges; the validator enforces symmetry (GDD 3.1 rule 1: it is a graph, edges matter). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TArray<FName> ConnectedRegionIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	EEclipseRegionOwner StartingOwner = EEclipseRegionOwner::Dominion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0, ClampMax = 100))
	int32 StartingUnrest = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 StartingGarrison = 0;

	/** Per-day yield by Resource.* tag at 100% control (SPEC-P1-03; GDD 6.3.2 factors apply per owner). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TMap<FGameplayTag, int32> BaseYieldPerDay;
};

UCLASS(BlueprintType)
class ECLIPSE_API UEclipseRegionGraphAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TArray<FEclipseRegionDefinition> Regions;

	/** Rows: FEclipseMissionOfferRow — one offer per region type (SPEC-P1-04). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TSoftObjectPtr<UDataTable> MissionOffers;
};
