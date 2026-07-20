#pragma once

#include "CoreMinimal.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "Strategy/EclipseRegionGraphAsset.h"

/**
 * Strategy-board rules as pure functions (GDD 14.3.2). GDD 3.1 scaled down to
 * one district: control is per-region, edges matter, and you can only strike
 * from territory you hold (SPEC-P1-04 adjacency rule).
 */
namespace EclipseStrategyLogic
{
	/**
	 * A region is a legal mission target iff it exists, is not already
	 * player-held, and borders at least one player-held region.
	 */
	ECLIPSE_API bool IsMissionTargetLegal(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName TargetRegionId, FString& OutReason);

	/** All currently legal target region ids, in definition order (deterministic for UI and tests). */
	ECLIPSE_API TArray<FName> GetLegalMissionTargets(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions);

	/**
	 * Structural graph validation (SPEC-P1-04 test contract + ValidateData rule):
	 * non-empty unique ids, edges reference existing regions, no orphan nodes,
	 * edges symmetric. Returns true when OutErrors stays empty.
	 */
	ECLIPSE_API bool ValidateGraph(const TArray<FEclipseRegionDefinition>& Definitions, TArray<FString>& OutErrors);
}
