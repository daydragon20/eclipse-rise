#include "Strategy/EclipseRegionGraphAsset.h"

#include "Eclipse.h"

TArray<FEclipseLaneDefinition> EclipseRegionGraph::OpenLanes(const TArray<FName>& NeighborIds)
{
	TArray<FEclipseLaneDefinition> Lanes;
	Lanes.Reserve(NeighborIds.Num());
	for (const FName& NeighborId : NeighborIds)
	{
		Lanes.Add(FEclipseLaneDefinition(NeighborId));
	}
	return Lanes;
}

int32 EclipseRegionGraph::UpgradeLegacyLanes(TArray<FEclipseRegionDefinition>& Regions)
{
	int32 Created = 0;
	for (FEclipseRegionDefinition& Region : Regions)
	{
		if (Region.ConnectedRegionIds_DEPRECATED.IsEmpty())
		{
			continue;
		}

		for (const FName& NeighborId : Region.ConnectedRegionIds_DEPRECATED)
		{
			// An authored lane always wins: the legacy list carries topology
			// only, and re-adding it would either duplicate the edge or silently
			// reset a cost somebody typed in on purpose.
			if (NeighborId.IsNone() || Region.HasLaneTo(NeighborId))
			{
				continue;
			}
			Region.Lanes.Add(FEclipseLaneDefinition(NeighborId));
			++Created;
		}

		// Emptied on purpose: two places that answer "who borders whom" is the
		// bug this migration exists to prevent.
		Region.ConnectedRegionIds_DEPRECATED.Reset();
	}
	return Created;
}

void UEclipseRegionGraphAsset::PostLoad()
{
	Super::PostLoad();

	const int32 Created = EclipseRegionGraph::UpgradeLegacyLanes(Regions);
	if (Created > 0)
	{
		// Loud, because the in-memory graph now differs from the bytes on disk
		// until somebody re-saves: the lanes are open at unit cost, which is a
		// guess about the board, not authored intent.
		UE_LOG(LogEclipse, Warning,
			TEXT("Region graph '%s': %d pre-lane edge(s) folded into open unit-cost lanes. Re-author lane costs/status and re-save (Tools/author_region_lanes.py)."),
			*GetName(), Created);
	}
}
