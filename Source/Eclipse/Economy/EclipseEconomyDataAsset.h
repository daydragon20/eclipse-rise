#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipseEconomyDataAsset.generated.h"

/**
 * One production item row (DT_ProductionItems, SPEC-P1-03). Values from GDD
 * 6.4.2 live in the table asset, never here (GDD 14.2).
 */
USTRUCT(BlueprintType)
struct FEclipseProductionItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (ClampMin = 0))
	int32 CostMaterials = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (ClampMin = 0))
	int32 CostCredits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (ClampMin = 1))
	int32 TimeDays = 1;

	/** Loadout option unlocked when this item completes (consumed by SPEC-P1-08 preparation). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy")
	FGameplayTag ResultLoadoutTag;
};

/**
 * All economy tunables (SPEC-P1-03: "All numbers in one UEclipseEconomyDataAsset
 * + DataTables"). A hardcoded gameplay constant is a defect (GDD 14.2).
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseEconomyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** GDD 6.4.1: flat wage stub (15 C/day Regular). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (ClampMin = 0))
	int32 WagePerSoldierPerDay = 15;

	/** GDD 6.2: stale intel decays 5%/week. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float IntelDecayPerWeek = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (ClampMin = 1))
	int32 IntelDecayIntervalDays = 7;

	/** GDD 6.3.2: contested regions produce at 40%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float ContestedYieldFactor = 0.4f;

	/** Rows: FEclipseProductionItemRow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Economy", meta = (RequiredAssetDataTags = "RowStructure=/Script/Eclipse.EclipseProductionItemRow"))
	TSoftObjectPtr<UDataTable> ProductionItems;
};
