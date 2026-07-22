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

/**
 * Move-order posture (SPEC-P1-06 stub: "Move to position (with stance stub:
 * ready/aggressive)"). PLACEHOLDER(GDD 8.4): drives posture/ROE and the cover-vs-
 * advance bias in the feel pass; Phase 1 stores it but splits no behavior yet.
 */
UENUM(BlueprintType)
enum class EEclipseSquadStance : uint8
{
	Ready,
	Aggressive
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

	/** Feel bar: order -> visible answer within 1 s (graybox feel targets §4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad", meta = (ClampMin = 0.1))
	float ResponseTimeoutSeconds = 1.0f;

	/** Rows: FEclipseSquadOrderDefRow keyed by order id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Squad")
	TSoftObjectPtr<UDataTable> OrderDefs;
};
