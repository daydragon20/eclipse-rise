#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipsePrepTypes.generated.h"

/**
 * Preparation data (SPEC-P1-08). Preparation is a first-class phase (GDD 11.1):
 * it converts strategic wealth into tactical fairness, so its numbers are
 * strategy data, never UI constants (GDD 14.2).
 */

/** One pickable loadout (DT_LoadoutOptions row; row name = option id). */
USTRUCT(BlueprintType)
struct FEclipseLoadoutOptionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FText DisplayName;

	/** Loadout identity handed to the mission runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FGameplayTag LoadoutTag;

	/** Empty = base option; otherwise requires this tag in CampaignState.UnlockedLoadoutTags (SPEC-P1-03 production gate). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FGameplayTag RequiredUnlockTag;
};

UCLASS(BlueprintType)
class ECLIPSE_API UEclipsePrepTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Intel price of the briefing reveal (GDD 11.1 fairness loop, stub form). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep", meta = (ClampMin = 0))
	int32 IntelRevealCost = 5;

	/** Fallback squadmate count when no squad tuning resolves; DA_SquadTuning.MaxDeployed - 1 is the real source (SPEC-P2-01). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep", meta = (ClampMin = 1))
	int32 SquadSize = 2;

	/** Rows: FEclipseLoadoutOptionRow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	TSoftObjectPtr<UDataTable> LoadoutOptions;
};
