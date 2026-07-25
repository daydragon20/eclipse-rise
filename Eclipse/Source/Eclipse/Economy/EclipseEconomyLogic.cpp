#include "Economy/EclipseEconomyLogic.h"

namespace EclipseEconomyLogic
{

bool BuildDayTick(const FEclipseCampaignState& State, const FEclipseEconomyTickParams& Params, FEclipseCampaignTransaction& OutTransaction)
{
	OutTransaction = FEclipseCampaignTransaction();
	OutTransaction.Source = TEXT("EconomyTick");

	// 1. Region yields (GDD 6.3.2): player-owned at 100%, contested at the data
	//    factor, Dominion-held at zero. Aggregated per resource so the wallet
	//    events stay readable (one line per resource, reason names the stream).
	TMap<FGameplayTag, int32> YieldTotals;
	for (const FEclipseRegionYieldParams& RegionParams : Params.Regions)
	{
		const FEclipseRegionState* Region = State.FindRegion(RegionParams.RegionId);
		if (Region == nullptr)
		{
			continue;
		}

		float Factor = 0.0f;
		switch (Region->Owner)
		{
		case EEclipseRegionOwner::Player:    Factor = 1.0f; break;
		case EEclipseRegionOwner::Contested: Factor = Params.ContestedYieldFactor; break;
		case EEclipseRegionOwner::Dominion:  Factor = 0.0f; break;
		default: break;
		}
		if (Factor <= 0.0f)
		{
			continue;
		}

		for (const TPair<FGameplayTag, int32>& Yield : RegionParams.BaseYieldPerDay)
		{
			const int32 Amount = FMath::FloorToInt32(static_cast<float>(Yield.Value) * Factor);
			if (Amount > 0)
			{
				YieldTotals.FindOrAdd(Yield.Key) += Amount;
			}
		}
	}

	// Sorted for deterministic mutation order (TMap iteration is not a contract).
	TArray<FGameplayTag> YieldKeys;
	YieldTotals.GenerateKeyArray(YieldKeys);
	YieldKeys.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });
	for (const FGameplayTag& Key : YieldKeys)
	{
		FEclipseCampaignMutation& Mutation = OutTransaction.Mutations.AddDefaulted_GetRef();
		Mutation.Type = EEclipseCampaignMutationType::AdjustResource;
		Mutation.ResourceType = Key;
		Mutation.Amount = YieldTotals[Key];
		Mutation.Reason = TEXT("RegionYield");
	}

	// 1b. Facility yields (SPEC-P2-03): the base produces in this same tick,
	//     as its own ledger lines — every number keeps its origin (GDD 7.6).
	TArray<FGameplayTag> FacilityKeys;
	Params.FacilityYields.YieldPerDay.GenerateKeyArray(FacilityKeys);
	FacilityKeys.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });
	for (const FGameplayTag& Key : FacilityKeys)
	{
		const int32 Amount = Params.FacilityYields.YieldPerDay[Key];
		// Amount > 0 is a deliberate drop-filter, not an oversight: no slice
		// facility yields negatively — upkeep is explicitly out of Phase 2
		// (SPEC-P2-03 locked decision 5, no Energy) — so a negative row is bad
		// data. Clamped into the tick it could drive a balance below zero,
		// which AdjustResource validation rejects, and one bad row would then
		// veto the ENTIRE atomic day tick (wages, decay, completions). Bad
		// data degrades to a skipped line, never a blocked day (GDD 14.3.5);
		// the Max(0, ...) on the wage-clamp base below is the same reading.
		// When Phase 3 lands real upkeep, it enters as its own clamped
		// mutation with its own reason, not through this yield stream.
		if (Amount > 0)
		{
			FEclipseCampaignMutation& Mutation = OutTransaction.Mutations.AddDefaulted_GetRef();
			Mutation.Type = EEclipseCampaignMutationType::AdjustResource;
			Mutation.ResourceType = Key;
			Mutation.Amount = Amount;
			Mutation.Reason = TEXT("FacilityYield");
		}
	}

	// Balance the later mutations validate against includes today's yields
	// (region and facility alike — the wage clamp sees the whole day's income).
	const int32 CreditsAfterYields = State.GetBalance(Params.CreditsTag)
		+ YieldTotals.FindRef(Params.CreditsTag)
		+ FMath::Max(0, Params.FacilityYields.YieldPerDay.FindRef(Params.CreditsTag));

	// 2. Wages (GDD 6.4.1 stub): every non-dead soldier draws pay — wounded
	//    soldiers on payroll is deliberate (people, not units).
	int32 PayrollCount = 0;
	for (const FEclipseSoldierRecord& Soldier : State.Roster)
	{
		if (Soldier.Status != EEclipseSoldierStatus::Dead)
		{
			++PayrollCount;
		}
	}
	const int32 WagesDue = PayrollCount * Params.WagePerSoldierPerDay;
	if (WagesDue > 0)
	{
		const int32 WagesPaid = FMath::Min(WagesDue, CreditsAfterYields);
		if (WagesPaid > 0)
		{
			FEclipseCampaignMutation& Mutation = OutTransaction.Mutations.AddDefaulted_GetRef();
			Mutation.Type = EEclipseCampaignMutationType::AdjustResource;
			Mutation.ResourceType = Params.CreditsTag;
			Mutation.Amount = -WagesPaid;
			Mutation.Reason = WagesPaid < WagesDue ? TEXT("Wages_Short") : TEXT("Wages");
		}
	}

	// 3. Intel decay (GDD 6.2): secrets age on a weekly cadence.
	if (Params.IntelDecayIntervalDays > 0 && State.Day > 0 && (State.Day % Params.IntelDecayIntervalDays) == 0)
	{
		const int32 Intel = State.GetBalance(Params.IntelTag);
		const int32 Decay = FMath::FloorToInt32(static_cast<float>(Intel) * Params.IntelDecayPerWeek);
		if (Decay > 0)
		{
			FEclipseCampaignMutation& Mutation = OutTransaction.Mutations.AddDefaulted_GetRef();
			Mutation.Type = EEclipseCampaignMutationType::AdjustResource;
			Mutation.ResourceType = Params.IntelTag;
			Mutation.Amount = -Decay;
			Mutation.Reason = TEXT("IntelDecay");
		}
	}

	// 4. Production completions: orders whose day has come.
	for (const FEclipseProductionOrder& Order : State.ProductionQueue)
	{
		if (Order.CompletesOnDay <= State.Day)
		{
			const FEclipseProductionItemParams* Item = Params.ProductionItems.FindByPredicate(
				[&Order](const FEclipseProductionItemParams& Candidate) { return Candidate.ItemId == Order.ItemId; });

			FEclipseCampaignMutation& Mutation = OutTransaction.Mutations.AddDefaulted_GetRef();
			Mutation.Type = EEclipseCampaignMutationType::CompleteProduction;
			Mutation.ProductionItemId = Order.ItemId;
			Mutation.LoadoutTag = Item != nullptr ? Item->ResultLoadoutTag : FGameplayTag();
			Mutation.Reason = TEXT("ProductionComplete");
		}
	}

	return !OutTransaction.Mutations.IsEmpty();
}

bool BuildProductionOrder(const FEclipseCampaignState& State, const FEclipseEconomyTickParams& Params, FName ItemId, FGameplayTag MaterialsTag, FEclipseCampaignTransaction& OutTransaction, FString& OutError)
{
	OutTransaction = FEclipseCampaignTransaction();
	OutTransaction.Source = TEXT("Workshop");

	const FEclipseProductionItemParams* Item = Params.ProductionItems.FindByPredicate(
		[ItemId](const FEclipseProductionItemParams& Candidate) { return Candidate.ItemId == ItemId; });
	if (Item == nullptr)
	{
		OutError = FString::Printf(TEXT("Unknown production item '%s'"), *ItemId.ToString());
		return false;
	}

	if (Item->CostMaterials > 0)
	{
		FEclipseCampaignMutation& Spend = OutTransaction.Mutations.AddDefaulted_GetRef();
		Spend.Type = EEclipseCampaignMutationType::AdjustResource;
		Spend.ResourceType = MaterialsTag;
		Spend.Amount = -Item->CostMaterials;
		Spend.Reason = FName(*FString::Printf(TEXT("Produce_%s"), *ItemId.ToString()));
	}
	if (Item->CostCredits > 0)
	{
		FEclipseCampaignMutation& Spend = OutTransaction.Mutations.AddDefaulted_GetRef();
		Spend.Type = EEclipseCampaignMutationType::AdjustResource;
		Spend.ResourceType = Params.CreditsTag;
		Spend.Amount = -Item->CostCredits;
		Spend.Reason = FName(*FString::Printf(TEXT("Produce_%s"), *ItemId.ToString()));
	}

	FEclipseCampaignMutation& Queue = OutTransaction.Mutations.AddDefaulted_GetRef();
	Queue.Type = EEclipseCampaignMutationType::QueueProduction;
	Queue.ProductionItemId = Item->ItemId;
	Queue.EtaDays = Item->TimeDays;
	Queue.Reason = TEXT("WorkshopOrder");

	return true;
}

} // namespace EclipseEconomyLogic
