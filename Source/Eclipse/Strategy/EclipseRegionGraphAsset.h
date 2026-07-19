#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "EclipseRegionGraphAsset.generated.h"

/**
 * Static region definitions for the 6-node district (SPEC-P1-04 data schema;
 * referenced by the campaign setup asset per SPEC-P1-02). This commit carries
 * the SPEC-P1-02 subset — starting state per node; edges, region types, yields
 * and mission offers land with SPEC-P1-04 in their own commit.
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
	EEclipseRegionOwner StartingOwner = EEclipseRegionOwner::Dominion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0, ClampMax = 100))
	int32 StartingUnrest = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 StartingGarrison = 0;
};

UCLASS(BlueprintType)
class ECLIPSE_API UEclipseRegionGraphAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TArray<FEclipseRegionDefinition> Regions;
};
