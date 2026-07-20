#include "Strategy/EclipseStrategyLogic.h"

namespace EclipseStrategyLogic
{

bool IsMissionTargetLegal(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName TargetRegionId, FString& OutReason)
{
	const FEclipseRegionDefinition* TargetDefinition = Definitions.FindByPredicate(
		[TargetRegionId](const FEclipseRegionDefinition& D) { return D.RegionId == TargetRegionId; });
	if (TargetDefinition == nullptr)
	{
		OutReason = FString::Printf(TEXT("Unknown region '%s'"), *TargetRegionId.ToString());
		return false;
	}

	const FEclipseRegionState* TargetState = State.FindRegion(TargetRegionId);
	if (TargetState == nullptr)
	{
		OutReason = FString::Printf(TEXT("Region '%s' has no campaign state"), *TargetRegionId.ToString());
		return false;
	}

	if (TargetState->Owner == EEclipseRegionOwner::Player)
	{
		OutReason = TEXT("Region is already player-held");
		return false;
	}

	for (const FName& NeighborId : TargetDefinition->ConnectedRegionIds)
	{
		const FEclipseRegionState* Neighbor = State.FindRegion(NeighborId);
		if (Neighbor != nullptr && Neighbor->Owner == EEclipseRegionOwner::Player)
		{
			return true;
		}
	}

	OutReason = TEXT("No adjacent player-held region (GDD 3.1 rule 1: no lane, no movement)");
	return false;
}

TArray<FName> GetLegalMissionTargets(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions)
{
	TArray<FName> Legal;
	for (const FEclipseRegionDefinition& Definition : Definitions)
	{
		FString Unused;
		if (IsMissionTargetLegal(State, Definitions, Definition.RegionId, Unused))
		{
			Legal.Add(Definition.RegionId);
		}
	}
	return Legal;
}

bool ValidateGraph(const TArray<FEclipseRegionDefinition>& Definitions, TArray<FString>& OutErrors)
{
	OutErrors.Reset();

	if (Definitions.IsEmpty())
	{
		OutErrors.Add(TEXT("Region graph has no regions"));
		return false;
	}

	TSet<FName> Ids;
	for (const FEclipseRegionDefinition& Definition : Definitions)
	{
		if (Definition.RegionId.IsNone())
		{
			OutErrors.Add(TEXT("Region with empty id"));
			continue;
		}
		bool bAlreadyPresent = false;
		Ids.Add(Definition.RegionId, &bAlreadyPresent);
		if (bAlreadyPresent)
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate region id '%s'"), *Definition.RegionId.ToString()));
		}
	}

	for (const FEclipseRegionDefinition& Definition : Definitions)
	{
		if (Definition.ConnectedRegionIds.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("Orphan region '%s' (no edges)"), *Definition.RegionId.ToString()));
		}

		for (const FName& NeighborId : Definition.ConnectedRegionIds)
		{
			if (NeighborId == Definition.RegionId)
			{
				OutErrors.Add(FString::Printf(TEXT("Region '%s' lists itself as neighbor"), *Definition.RegionId.ToString()));
				continue;
			}
			const FEclipseRegionDefinition* Neighbor = Definitions.FindByPredicate(
				[NeighborId](const FEclipseRegionDefinition& D) { return D.RegionId == NeighborId; });
			if (Neighbor == nullptr)
			{
				OutErrors.Add(FString::Printf(TEXT("Region '%s' references unknown neighbor '%s'"), *Definition.RegionId.ToString(), *NeighborId.ToString()));
			}
			else if (!Neighbor->ConnectedRegionIds.Contains(Definition.RegionId))
			{
				OutErrors.Add(FString::Printf(TEXT("Asymmetric edge: '%s' -> '%s' has no reverse edge"), *Definition.RegionId.ToString(), *NeighborId.ToString()));
			}
		}
	}

	return OutErrors.IsEmpty();
}

} // namespace EclipseStrategyLogic
