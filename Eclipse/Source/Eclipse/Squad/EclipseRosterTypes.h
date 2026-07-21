#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "EclipseRosterTypes.generated.h"

/**
 * Roster data schemas (SPEC-P1-07). Names and traits are data so every soldier
 * is an individual by content, not by code (Pillar 3; GDD 4.2.1-4.2.2 stub).
 */

/** One origin's name pool (DT_NamePools row; row name = origin id). */
USTRUCT(BlueprintType)
struct FEclipseNamePoolRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Roster")
	TArray<FString> FirstNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Roster")
	TArray<FString> LastNames;
};

/** One visible trait line (DT_TraitStubs row; row name = trait id). Flavor only in Phase 1. */
USTRUCT(BlueprintType)
struct FEclipseTraitStubRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Roster")
	FText Line;
};

/** Roster tunables (SPEC-P1-07: no hardcoded numbers — GDD 14.2). */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseRosterTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Flat recovery duration for the Phase 1 injury stub (Medbay math is Phase 3 — GDD 4.2.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Roster", meta = (ClampMin = 1))
	int32 WoundedDaysOut = 5;

	/** Rows: FEclipseNamePoolRow keyed by origin id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Roster")
	TSoftObjectPtr<UDataTable> NamePools;

	/** Rows: FEclipseTraitStubRow keyed by trait id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Roster")
	TSoftObjectPtr<UDataTable> TraitStubs;
};
