#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EclipseCommandModeTuning.generated.h"

/**
 * Command Mode tunables (SPEC-P2-02 data schema — DA_CommandModeTuning). Every
 * number of the mode lives here (GDD 14.2); the Stage B fields (marks, flank
 * window, suppress radius, camera pullback) are data-ready now so the asset
 * never needs a schema change mid-phase. A missing asset degrades to these
 * defaults with a logged warning (GDD 14.3.5).
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseCommandModeTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Global time dilation while the mode is held (locked decision 1: ~30%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float DilationFactor = 0.30f;

	/** Tactician difficulty factor (0 = full pause). Data-ready, headless-tested only this phase (SPEC-P2-00 scope). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float TacticianDilationFactor = 0.0f;

	/** Enter/exit dilation blend; 0 = hard cut (the Stage A verdict runs on the cut). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.0))
	float EnterBlendSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.0))
	float ExitBlendSeconds = 0.0f;

	/** Direct-pick reach for soldier selection under the reticle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 100.0))
	float SoldierSelectMaxRangeCm = 10000.0f;

	/** Stage B (SyncStrike) cap — reserved in data per the spec. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 1))
	int32 MaxSyncStrikeMarks = 4;

	/** Stage B (Flank) approval window, wall-clock seconds (same basis as ResponseTimeoutSeconds — locked decision 2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 0.5))
	float FlankApprovalTimeoutSeconds = 6.0f;

	/** Stage B (Suppress) area radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Command", meta = (ClampMin = 50.0))
	float SuppressRadiusCm = 400.0f;
};
