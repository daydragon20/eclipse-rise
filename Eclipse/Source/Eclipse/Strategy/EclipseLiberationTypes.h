#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "EclipseLiberationTypes.generated.h"

/**
 * One liberation instance (DT_LiberationInstances row — SPEC-P2-05 data
 * schema; GDD 11.2's campaign-mission template as data, locked decision 5:
 * no hardcoded trio anywhere). The slice ships exactly one row — Foothold:
 * TransitCheckpoint + WorkerHousing + SupplyDepot flip to Player on M1.3
 * completion (locked decision 1). Phase 3 planets instantiate this same row
 * shape; its content is the slice's, not a contract.
 *
 * Degradation ladder (GDD 14.3.5), enforced across the layers:
 * - missing table, or no row matching the completed mission = no flip + one
 *   logged warning by the wiring layer, never a crash — the mission still
 *   completes and the campaign still runs;
 * - unknown region id inside a row = dropped during resolution and reported,
 *   while the remaining ids still flip (a typo must not reject the whole
 *   liberation — the atomic transaction would);
 * - empty RequiredBeatTag = gate skipped (empty-means-ungated, spec review
 *   point 3 resolved KEEP).
 */
USTRUCT(BlueprintType)
struct FEclipseLiberationRow : public FTableRowBase
{
	GENERATED_BODY()

	/**
	 * Identity AND trigger: matches the stable Event.Mission.Completed payload
	 * mission id (the SPEC-P2-04 seam guarantee — fires exactly once per
	 * completion, success only; Failed is a different tag). Locked decision 2:
	 * authored missions flip regions exclusively through this mapping — one
	 * writer per mission family on the shared SetRegionOwner mutation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Liberation")
	FName TriggerMissionId;

	/**
	 * The regions this liberation flips (the Foothold trio in the slice), in
	 * authored order. Resolution preserves it: row order = commit order =
	 * event order — deterministic for tests and the map (SPEC-P2-05 pure core
	 * contract).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Liberation")
	TArray<FName> RegionIds;

	/**
	 * Audit gate against the committed story beats (v5 StoryFlags, SPEC-P2-04):
	 * the row only triggers when this beat is set — the slice row gates on
	 * Story.Beat.M13_SignalFire. Empty = ungated, mission-id match alone fires
	 * (14.3.5 ladder).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Liberation")
	FGameplayTag RequiredBeatTag;

	/**
	 * Owner the regions flip TO — Player for the slice; data so a Phase 3 row
	 * can express a Contested step. Locked decision 6: the flip changes Owner
	 * and nothing else — Unrest and GarrisonStrength are untouched, and no
	 * reward rides the liberation transaction (M1.3's debrief already paid).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Liberation")
	EEclipseRegionOwner NewOwner = EEclipseRegionOwner::Player;

	/** One-line liberation blurb for the step-4 debug debrief surface; empty = the surface synthesizes nothing (14.3.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Liberation")
	FText ContextLine;
};
