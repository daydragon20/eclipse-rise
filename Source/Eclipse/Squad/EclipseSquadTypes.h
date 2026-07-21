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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float FollowDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0))
	float CoverSearchRadius = 800.0f;

	/** Feel bar: order -> visible answer within 1 s (graybox feel targets §4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0.1))
	float ResponseTimeoutSeconds = 1.0f;

	/** Rows: FEclipseSquadOrderDefRow keyed by order id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad")
	TSoftObjectPtr<UDataTable> OrderDefs;
};
