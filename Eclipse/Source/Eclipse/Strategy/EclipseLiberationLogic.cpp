#include "Strategy/EclipseLiberationLogic.h"

namespace EclipseLiberationLogic
{

bool IsLiberationTriggered(const FEclipseCampaignState& State, const FEclipseLiberationRow& Row, FName CompletedMissionId)
{
	if (Row.TriggerMissionId.IsNone() || Row.TriggerMissionId != CompletedMissionId)
	{
		// A row without identity is data damage, never a wildcard (GDD 14.3.5).
		return false;
	}
	return !Row.RequiredBeatTag.IsValid() || State.StoryFlags.Contains(Row.RequiredBeatTag);
}

FEclipseCampaignTransaction ResolveLiberationTransaction(const FEclipseCampaignState& State, const FEclipseLiberationRow& Row, TArray<FName>& OutDroppedRegionIds)
{
	OutDroppedRegionIds.Reset();

	FEclipseCampaignTransaction Transaction;
	Transaction.Source = LiberationTransactionSource;

	TSet<FName> SeenRegionIds;
	for (const FName& RegionId : Row.RegionIds)
	{
		bool bAlreadySeen = false;
		SeenRegionIds.Add(RegionId, &bAlreadySeen);
		if (bAlreadySeen)
		{
			// A duplicate's second flip would be a mid-transaction no-op —
			// ValidateMutation would reject it and the atomic commit would
			// reject the whole liberation with it (locked decision 3).
			OutDroppedRegionIds.Add(RegionId);
			continue;
		}

		const FEclipseRegionState* Region = State.FindRegion(RegionId);
		if (Region == nullptr)
		{
			// A typo must not reject the whole liberation (GDD 14.3.5); the
			// wiring layer warns with these ids, the rest still flips.
			OutDroppedRegionIds.Add(RegionId);
			continue;
		}

		if (Region->Owner == Row.NewOwner)
		{
			// Already at target: excluded, not dropped — the legitimate half
			// of state-derived idempotence (decision 3: re-entry commits
			// nothing; a player pre-flip earns exactly the remainder).
			continue;
		}

		FEclipseCampaignMutation& Mutation = Transaction.Mutations.AddDefaulted_GetRef();
		Mutation.Type = EEclipseCampaignMutationType::SetRegionOwner;
		Mutation.RegionId = RegionId;
		Mutation.NewOwner = Row.NewOwner;
	}

	return Transaction;
}

bool IsLiberationComplete(const FEclipseCampaignState& State, const FEclipseLiberationRow& Row)
{
	bool bAnyKnownRegion = false;
	for (const FName& RegionId : Row.RegionIds)
	{
		const FEclipseRegionState* Region = State.FindRegion(RegionId);
		if (Region == nullptr)
		{
			continue; // unknown ids fold out exactly as resolution drops them (GDD 14.3.5)
		}
		bAnyKnownRegion = true;
		if (Region->Owner != Row.NewOwner)
		{
			return false;
		}
	}
	return bAnyKnownRegion;
}

} // namespace EclipseLiberationLogic
