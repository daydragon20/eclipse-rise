#include "Quests/EclipseMissionLogic.h"

#include "Quests/EclipseStoryLogic.h"

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

FName GetPhaseName(EEclipseMissionPhase Phase)
{
	switch (Phase)
	{
	case EEclipseMissionPhase::Insertion:  return FName(TEXT("Insertion"));
	case EEclipseMissionPhase::Objectives: return FName(TEXT("Objectives"));
	case EEclipseMissionPhase::Extraction: return FName(TEXT("Extraction"));
	case EEclipseMissionPhase::Debrief:    return FName(TEXT("Debrief"));
	default:                               return NAME_None; // None/Finished: rest states, never broadcast
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

void EvaluateOptionalObjectives(
	const TArray<FEclipseObjectiveDef>& Objectives,
	const TArray<FName>& CompletedIds,
	bool bAlarmRaised,
	bool bAnySoldierDowned,
	TArray<FEclipseObjectiveDef>& OutPaid,
	TArray<FName>& OutMissedIds)
{
	OutPaid.Reset();
	OutMissedIds.Reset();
	for (const FEclipseObjectiveDef& Objective : Objectives)
	{
		// Conditions and rewards are read only on optionals (SPEC-P2-04): a
		// mandatory objective can never pay a stretch bonus, and alarm never
		// fails a mission (GDD 11.4) — the void below is the whole price.
		if (!Objective.bOptional)
		{
			continue;
		}

		// VOORWAARDE-OBJECTIVES vinken zichzelf af (owner-beslissing 26-07).
		//
		// Een optional met een conditie-vlag en GEEN doelwit is geen taak maar een
		// voorwaarde: "Bring everyone home standing" heeft niets om te bereiken, te
		// pakken of om te leggen. Tot vandaag moest hij tóch voltooid worden om uit
		// te betalen, en niets voltooide hem — de HUD toonde een stretch-doel dat
		// nooit afvinkte en de +20 materiaal kwam nooit binnen (gemeten: nul
		// gewonden, 25 in plaats van 45).
		//
		// De regel staat hier en niet op een nieuw veld: TargetId leeg + minstens
		// één conditie IS de definitie van een voorwaarde, en die is af te lezen
		// uit de data die er al staat. Decision 5 verbiedt nieuwe objective-verbs;
		// conditie-vlaggen zijn juist het precedent.
		const bool bHasCondition = Objective.bRequiresNoAlarm || Objective.bRequiresNoCasualties;
		const bool bIsConditionObjective = bHasCondition && Objective.TargetId.IsNone();

		if (!bIsConditionObjective && !CompletedIds.Contains(Objective.ObjectiveId))
		{
			continue; // an optional never attempted is simply absent — not "missed"
		}

		const bool bVoided = (Objective.bRequiresNoAlarm && bAlarmRaised)
			|| (Objective.bRequiresNoCasualties && bAnySoldierDowned);
		if (bVoided)
		{
			OutMissedIds.Add(Objective.ObjectiveId);
		}
		else
		{
			OutPaid.Add(Objective);
		}
	}
}

FEclipseCampaignTransaction ComposeConsequences(
	const FEclipseMissionOutcome& Outcome,
	const FEclipseMissionRewards& Rewards,
	const TArray<FEclipseResolvedCasualty>& Casualties,
	const FEclipseCampaignState& State,
	const FGameplayTag& CreditsTag,
	const FGameplayTag& MaterialsTag,
	const FGameplayTag& IntelTag,
	bool bProgressRegionOnSuccess,
	const FGameplayTag& CompletionBeatTag,
	const TArray<FEclipseObjectiveDef>& Objectives)
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

	// Optional stretch payouts ride the same atomic debrief (SPEC-P2-04) — the
	// same pattern as the completion beat below: one transaction, or nothing.
	// Success-gated like the base reward; the latches arrive on the outcome
	// (bAlarmRaised + DownedSoldierIds, which retains stabilized soldiers —
	// "ever went down" is the fact, the save only changes the resolution).
	if (Outcome.bSuccess)
	{
		TArray<FEclipseObjectiveDef> PaidOptionals;
		TArray<FName> MissedOptionalIds;
		EvaluateOptionalObjectives(Objectives, Outcome.CompletedObjectiveIds,
			Outcome.bAlarmRaised, !Outcome.DownedSoldierIds.IsEmpty(), PaidOptionals, MissedOptionalIds);
		for (const FEclipseObjectiveDef& Optional : PaidOptionals)
		{
			AddReward(CreditsTag, Optional.OptionalRewardCredits, TEXT("OptionalObjective"));
			AddReward(MaterialsTag, Optional.OptionalRewardMaterials, TEXT("OptionalObjective"));
			AddReward(IntelTag, Optional.OptionalRewardIntel, TEXT("OptionalObjective"));
		}
	}

	// The story completion beat is one atomic fact with the rest of the debrief
	// (SPEC-P2-04 decision 12). Skip-if-set is load-bearing, not cosmetic:
	// SetStoryFlag rejects duplicates atomically, so composing the beat into a
	// re-completed mission's debrief would reject rewards, casualties and the
	// day tick wholesale (the P2-05 no-op-reject lesson mirrored). Loss never
	// commits a beat — fail-forward means retry, not story progress (GDD 11.4).
	if (Outcome.bSuccess && EclipseStoryLogic::ShouldCommitBeat(FGameplayTagContainer::CreateFromArray(State.StoryFlags), CompletionBeatTag))
	{
		FEclipseCampaignMutation& BeatMutation = Transaction.Mutations.AddDefaulted_GetRef();
		BeatMutation.Type = EEclipseCampaignMutationType::SetStoryFlag;
		BeatMutation.StoryFlagTag = CompletionBeatTag;
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
