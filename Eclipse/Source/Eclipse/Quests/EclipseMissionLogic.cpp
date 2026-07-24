#include "Quests/EclipseMissionLogic.h"

namespace EclipseMissionLogic
{

bool CanAdvancePhase(EEclipseMissionPhase From, EEclipseMissionPhase To)
{
	switch (From)
	{
	case EEclipseMissionPhase::None:       return To == EEclipseMissionPhase::Insertion;
	case EEclipseMissionPhase::Insertion:  return To == EEclipseMissionPhase::Objectives;
	case EEclipseMissionPhase::Objectives: return To == EEclipseMissionPhase::Extraction || To == EEclipseMissionPhase::Debrief; // Debrief direct = mission aborted/failed in the field
	case EEclipseMissionPhase::Extraction: return To == EEclipseMissionPhase::Debrief;
	case EEclipseMissionPhase::Debrief:    return To == EEclipseMissionPhase::Finished;
	case EEclipseMissionPhase::Finished:   return false;
	default:                               return false;
	}
}

bool AreMandatoryObjectivesComplete(const TArray<FEclipseObjectiveDef>& Objectives, const TArray<FName>& CompletedIds)
{
	for (const FEclipseObjectiveDef& Objective : Objectives)
	{
		if (!Objective.bOptional && !CompletedIds.Contains(Objective.ObjectiveId))
		{
			return false;
		}
	}
	return true;
}

FEclipseCampaignTransaction ComposeConsequences(
	const FEclipseMissionOutcome& Outcome,
	const FEclipseMissionRewards& Rewards,
	const TArray<FEclipseResolvedCasualty>& Casualties,
	const FEclipseCampaignState& State,
	const FGameplayTag& CreditsTag,
	const FGameplayTag& MaterialsTag,
	const FGameplayTag& IntelTag,
	bool bProgressRegionOnSuccess)
{
	FEclipseCampaignTransaction Transaction;
	Transaction.Source = TEXT("MissionDebrief");

	// Every deployed soldier served this mission, win or lose (SPEC-P1-07 roster
	// field; Pillar 3: the count is the person's history). Marks precede the
	// casualty mutations so a soldier who dies today still records the fatal
	// mission on their roster row — matching the memorial's Record + 1 snapshot.
	for (const FGuid& DeployedId : Outcome.DeployedSoldierIds)
	{
		const FEclipseSoldierRecord* Deployed = State.FindSoldier(DeployedId);
		if (Deployed == nullptr || Deployed->Status == EEclipseSoldierStatus::Dead)
		{
			continue; // unknown/dead ids must not reject the whole debrief (GDD 14.3.5)
		}
		FEclipseCampaignMutation& Served = Transaction.Mutations.AddDefaulted_GetRef();
		Served.Type = EEclipseCampaignMutationType::MarkMissionServed;
		Served.SoldierId = DeployedId;
	}

	auto AddReward = [&Transaction](const FGameplayTag& Tag, int32 Amount, FName Reason)
	{
		if (Tag.IsValid() && Amount > 0)
		{
			FEclipseCampaignMutation& Mutation = Transaction.Mutations.AddDefaulted_GetRef();
			Mutation.Type = EEclipseCampaignMutationType::AdjustResource;
			Mutation.ResourceType = Tag;
			Mutation.Amount = Amount;
			Mutation.Reason = Reason;
		}
	};

	if (Outcome.bSuccess)
	{
		AddReward(CreditsTag, Rewards.Credits, TEXT("MissionReward"));
		AddReward(MaterialsTag, Rewards.Materials, TEXT("MissionReward"));
		AddReward(IntelTag, Rewards.Intel, TEXT("MissionReward"));
	}
	else
	{
		// Fail-forward (GDD 11.4): the field always teaches something — a failed
		// op still brings home partial intel, never a retry wall.
		AddReward(IntelTag, Rewards.Intel / 2, TEXT("MissionIntel_Salvaged"));
	}

	if (Outcome.bSuccess && bProgressRegionOnSuccess)
	{
		if (const FEclipseRegionState* Region = State.FindRegion(Outcome.RegionId))
		{
			if (Region->Owner != EEclipseRegionOwner::Player)
			{
				FEclipseCampaignMutation& Flip = Transaction.Mutations.AddDefaulted_GetRef();
				Flip.Type = EEclipseCampaignMutationType::SetRegionOwner;
				Flip.RegionId = Outcome.RegionId;
				Flip.NewOwner = Region->Owner == EEclipseRegionOwner::Dominion ? EEclipseRegionOwner::Contested : EEclipseRegionOwner::Player;
			}
		}
	}

	for (const FEclipseResolvedCasualty& Casualty : Casualties)
	{
		if (Casualty.bDead)
		{
			FEclipseCampaignMutation& Kill = Transaction.Mutations.AddDefaulted_GetRef();
			Kill.Type = EEclipseCampaignMutationType::KillSoldier;
			Kill.SoldierId = Casualty.SoldierId;
			Kill.Cause = Casualty.Cause;

			// Death and its record are one atomic fact (Pillar 3: the wall never
			// misses a name because a commit was split).
			const FEclipseSoldierRecord* Record = State.FindSoldier(Casualty.SoldierId);
			FEclipseCampaignMutation& Memorial = Transaction.Mutations.AddDefaulted_GetRef();
			Memorial.Type = EEclipseCampaignMutationType::AddMemorialEntry;
			Memorial.MemorialEntry.SoldierId = Casualty.SoldierId;
			Memorial.MemorialEntry.Name = Record != nullptr ? Record->Name : Casualty.SoldierName;
			Memorial.MemorialEntry.MissionsServed = Record != nullptr ? Record->MissionsServed + 1 : 1;
			Memorial.MemorialEntry.Cause = Casualty.Cause;
			Memorial.MemorialEntry.Day = State.Day;
		}
		else
		{
			FEclipseCampaignMutation& Wound = Transaction.Mutations.AddDefaulted_GetRef();
			Wound.Type = EEclipseCampaignMutationType::WoundSoldier;
			Wound.SoldierId = Casualty.SoldierId;
			Wound.Cause = Casualty.Cause;
			Wound.EtaDays = Casualty.DaysOut;
		}
	}

	// Mission resolution advances the strategic clock by exactly one day, win
	// or lose (SPEC-P2-03 locked decision 4; SPEC-P2-04 decision 12 inherits
	// it) — time is a cost of going out, and the base's construction/yield
	// ticks ride the same commit. Last in the transaction so memorial entries
	// record the day the mission happened on, not the morning after.
	FEclipseCampaignMutation& DayTick = Transaction.Mutations.AddDefaulted_GetRef();
	DayTick.Type = EEclipseCampaignMutationType::AdvanceDay;

	return Transaction;
}

} // namespace EclipseMissionLogic
