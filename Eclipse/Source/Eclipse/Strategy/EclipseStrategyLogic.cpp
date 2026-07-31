#include "Strategy/EclipseStrategyLogic.h"

#include "Algo/Reverse.h"

namespace EclipseStrategyLogic
{

namespace
{
	const FEclipseRegionDefinition* FindDefinition(const TArray<FEclipseRegionDefinition>& Definitions, FName RegionId)
	{
		return Definitions.FindByPredicate(
			[RegionId](const FEclipseRegionDefinition& D) { return D.RegionId == RegionId; });
	}

	/**
	 * Whether a Gate Spire is hostile. "Hostile" is Dominion-held, per GDD 3.1
	 * rule 2. A gate node that has no campaign state at all counts as hostile:
	 * an unknown gate is not an open gate, and silently opening lanes because a
	 * region row is missing is exactly the failure mode 14.3.5 warns about.
	 */
	EEclipseRegionOwner GateOwner(const FEclipseCampaignState& State, FName GateRegionId)
	{
		const FEclipseRegionState* Gate = State.FindRegion(GateRegionId);
		return Gate != nullptr ? Gate->Owner : EEclipseRegionOwner::Dominion;
	}

	int32 TierSurcharge(const FEclipseCampaignState& State, const FEclipseLaneTuning& Tuning)
	{
		return static_cast<int32>(State.ResponseTier) * Tuning.RiskPerResponseTier;
	}
}

bool IsReachable(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions,
	FName StartRegionId, FName GoalRegionId, EEclipseTransitMode Mode, const FEclipseLaneTuning& Tuning)
{
	if (StartRegionId == GoalRegionId)
	{
		return FindDefinition(Definitions, StartRegionId) != nullptr;
	}

	// Plain BFS, deliberately not FindRoute: this exists to explain why FindRoute
	// failed, and calling it from inside that failure path would be a loop.
	TSet<FName> Seen;
	TArray<FName> Frontier;
	Seen.Add(StartRegionId);
	Frontier.Add(StartRegionId);

	while (!Frontier.IsEmpty())
	{
		const FName Current = Frontier.Pop();
		const FEclipseRegionDefinition* Definition = FindDefinition(Definitions, Current);
		if (Definition == nullptr)
		{
			continue;
		}
		for (const FEclipseLaneDefinition& Lane : Definition->Lanes)
		{
			if (Seen.Contains(Lane.NeighborRegionId)
				|| !ResolveLaneTransit(State, Lane, Mode, Tuning).bPassable)
			{
				continue;
			}
			if (Lane.NeighborRegionId == GoalRegionId)
			{
				return true;
			}
			Seen.Add(Lane.NeighborRegionId);
			Frontier.Add(Lane.NeighborRegionId);
		}
	}
	return false;
}

FEclipseLaneTransit ResolveLaneTransit(const FEclipseCampaignState& State, const FEclipseLaneDefinition& Lane, EEclipseTransitMode Mode, const FEclipseLaneTuning& Tuning)
{
	FEclipseLaneTransit Transit;
	Transit.TravelDays = Lane.TravelDays;
	Transit.Risk = Lane.Risk;

	// The smuggler surcharge, applied wherever a leg is run as a smuggler leg.
	const auto ChargeSmuggler = [&Lane, &Transit]()
	{
		Transit.bSmugglerLeg = true;
		Transit.TravelDays += Lane.SmugglerDelayDays;
		Transit.Risk += Lane.SmugglerRiskPenalty;
	};

	switch (Lane.Status)
	{
	case EEclipseLaneStatus::Open:
		Transit.bPassable = true;
		break;

	case EEclipseLaneStatus::SpireGated:
	{
		const EEclipseRegionOwner Owner = GateOwner(State, Lane.GateRegionId);
		if (Owner == EEclipseRegionOwner::Dominion)
		{
			// Shut to columns. Smugglers still run it — that is the entire
			// difference between this status and "no lane".
			if (Mode == EEclipseTransitMode::Military)
			{
				Transit.bPassable = false;
				Transit.BlockedReason = FString::Printf(
					TEXT("Gate Spire '%s' is hostile — military transit blocked (GDD 3.1 rule 2)"), *Lane.GateRegionId.ToString());
				return Transit;
			}
			Transit.bPassable = true;
			ChargeSmuggler();
			break;
		}

		Transit.bPassable = true;
		if (Owner == EEclipseRegionOwner::Contested)
		{
			// The gate is nobody's: you get through, but through a firefight.
			Transit.Risk += Tuning.ContestedGateRiskPenalty;
		}
		break;
	}

	case EEclipseLaneStatus::SmugglerOnly:
		if (Mode == EEclipseTransitMode::Military)
		{
			Transit.bPassable = false;
			Transit.BlockedReason = TEXT("Smuggler lane — no military column fits through, in any campaign state");
			return Transit;
		}
		Transit.bPassable = true;
		ChargeSmuggler();
		break;
	}

	// The empire's temperature is felt on every road it watches (GDD 9.4).
	Transit.Risk += TierSurcharge(State, Tuning);
	return Transit;
}

bool IsMissionTargetLegal(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName TargetRegionId, FString& OutReason)
{
	const FEclipseRegionDefinition* TargetDefinition = FindDefinition(Definitions, TargetRegionId);
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

	// Default tuning: legality asks only "can anyone cross this", which no
	// board-wide number changes. Pricing the approach is ClassifyMissionApproach's
	// job, and that one takes the caller's tuning.
	const FEclipseLaneTuning DefaultTuning;
	bool bSawPlayerNeighbor = false;
	for (const FEclipseLaneDefinition& Lane : TargetDefinition->Lanes)
	{
		const FEclipseRegionState* Neighbor = State.FindRegion(Lane.NeighborRegionId);
		if (Neighbor == nullptr || Neighbor->Owner != EEclipseRegionOwner::Player)
		{
			continue;
		}
		bSawPlayerNeighbor = true;

		if (ResolveLaneTransit(State, Lane, EEclipseTransitMode::Military, DefaultTuning).bPassable
			|| ResolveLaneTransit(State, Lane, EEclipseTransitMode::Smuggler, DefaultTuning).bPassable)
		{
			return true;
		}
	}

	OutReason = bSawPlayerNeighbor
		? TEXT("Every lane from player-held ground into this region is shut (GDD 3.1 rule 2)")
		: TEXT("No adjacent player-held region (GDD 3.1 rule 1: no lane, no movement)");
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

bool ClassifyMissionApproach(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName TargetRegionId, const FEclipseLaneTuning& Tuning, EEclipseTransitMode& OutMode)
{
	FString Unused;
	if (!IsMissionTargetLegal(State, Definitions, TargetRegionId, Unused))
	{
		return false;
	}

	const FEclipseRegionDefinition* TargetDefinition = FindDefinition(Definitions, TargetRegionId);
	check(TargetDefinition != nullptr); // legality already proved it exists

	OutMode = EEclipseTransitMode::Smuggler;
	for (const FEclipseLaneDefinition& Lane : TargetDefinition->Lanes)
	{
		const FEclipseRegionState* Neighbor = State.FindRegion(Lane.NeighborRegionId);
		if (Neighbor == nullptr || Neighbor->Owner != EEclipseRegionOwner::Player)
		{
			continue;
		}
		if (ResolveLaneTransit(State, Lane, EEclipseTransitMode::Military, Tuning).bPassable)
		{
			OutMode = EEclipseTransitMode::Military;
			return true;
		}
	}
	return true;
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

		if (!Definition.ConnectedRegionIds_DEPRECATED.IsEmpty())
		{
			// PostLoad should have folded these away. Seeing them here means the
			// graph was hand-built in code and skipped the migration, and half
			// its topology is invisible to routing.
			OutErrors.Add(FString::Printf(
				TEXT("Region '%s' still carries pre-lane ConnectedRegionIds — run EclipseRegionGraph::UpgradeLegacyLanes"),
				*Definition.RegionId.ToString()));
		}
	}

	for (const FEclipseRegionDefinition& Definition : Definitions)
	{
		if (Definition.Lanes.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("Orphan region '%s' (no edges)"), *Definition.RegionId.ToString()));
		}

		TSet<FName> SeenNeighbors;
		for (const FEclipseLaneDefinition& Lane : Definition.Lanes)
		{
			const FName NeighborId = Lane.NeighborRegionId;
			if (NeighborId == Definition.RegionId)
			{
				OutErrors.Add(FString::Printf(TEXT("Region '%s' lists itself as neighbor"), *Definition.RegionId.ToString()));
				continue;
			}

			bool bDuplicateLane = false;
			SeenNeighbors.Add(NeighborId, &bDuplicateLane);
			if (bDuplicateLane)
			{
				// Two lanes to the same place cannot both be "the" lane: routing
				// would take the first and the map would draw the other.
				OutErrors.Add(FString::Printf(TEXT("Region '%s' has duplicate lanes to '%s'"),
					*Definition.RegionId.ToString(), *NeighborId.ToString()));
			}

			if (Lane.TravelDays < 1)
			{
				OutErrors.Add(FString::Printf(TEXT("Lane '%s' -> '%s' has TravelDays %d (a lane you cross for free is not a lane)"),
					*Definition.RegionId.ToString(), *NeighborId.ToString(), Lane.TravelDays));
			}
			if (Lane.Risk < 0 || Lane.SmugglerRiskPenalty < 0 || Lane.SmugglerDelayDays < 0)
			{
				OutErrors.Add(FString::Printf(TEXT("Lane '%s' -> '%s' has negative cost fields"),
					*Definition.RegionId.ToString(), *NeighborId.ToString()));
			}

			// The gate is only meaningful on the status that reads it. A gate id
			// on an Open lane reads to a human as "this is gated" and to the code
			// as nothing at all — the worst kind of disagreement.
			if (Lane.Status == EEclipseLaneStatus::SpireGated)
			{
				if (Lane.GateRegionId.IsNone())
				{
					OutErrors.Add(FString::Printf(TEXT("Lane '%s' -> '%s' is SpireGated but names no gate region"),
						*Definition.RegionId.ToString(), *NeighborId.ToString()));
				}
				else if (FindDefinition(Definitions, Lane.GateRegionId) == nullptr)
				{
					OutErrors.Add(FString::Printf(TEXT("Lane '%s' -> '%s' is gated by unknown region '%s'"),
						*Definition.RegionId.ToString(), *NeighborId.ToString(), *Lane.GateRegionId.ToString()));
				}
			}
			else if (!Lane.GateRegionId.IsNone())
			{
				OutErrors.Add(FString::Printf(TEXT("Lane '%s' -> '%s' names gate '%s' but is not SpireGated"),
					*Definition.RegionId.ToString(), *NeighborId.ToString(), *Lane.GateRegionId.ToString()));
			}

			const FEclipseRegionDefinition* Neighbor = FindDefinition(Definitions, NeighborId);
			if (Neighbor == nullptr)
			{
				OutErrors.Add(FString::Printf(TEXT("Region '%s' references unknown neighbor '%s'"), *Definition.RegionId.ToString(), *NeighborId.ToString()));
				continue;
			}

			const FEclipseLaneDefinition* Reverse = Neighbor->FindLane(Definition.RegionId);
			if (Reverse == nullptr)
			{
				OutErrors.Add(FString::Printf(TEXT("Asymmetric edge: '%s' -> '%s' has no reverse edge"), *Definition.RegionId.ToString(), *NeighborId.ToString()));
				continue;
			}

			// Symmetry of the RECORD, not just of its existence. A lane that is
			// open outbound and gated inbound is a data bug; a one-way street
			// would need its own schema and its own map symbol.
			if (Reverse->Status != Lane.Status)
			{
				OutErrors.Add(FString::Printf(TEXT("Asymmetric lane status: '%s' -> '%s' is %s but the reverse is %s"),
					*Definition.RegionId.ToString(), *NeighborId.ToString(),
					*UEnum::GetValueAsString(Lane.Status), *UEnum::GetValueAsString(Reverse->Status)));
			}
			if (Reverse->GateRegionId != Lane.GateRegionId)
			{
				OutErrors.Add(FString::Printf(TEXT("Asymmetric lane gate: '%s' -> '%s' gates on '%s' but the reverse gates on '%s'"),
					*Definition.RegionId.ToString(), *NeighborId.ToString(),
					*Lane.GateRegionId.ToString(), *Reverse->GateRegionId.ToString()));
			}
			if (Reverse->TravelDays != Lane.TravelDays || Reverse->Risk != Lane.Risk
				|| Reverse->SmugglerDelayDays != Lane.SmugglerDelayDays
				|| Reverse->SmugglerRiskPenalty != Lane.SmugglerRiskPenalty)
			{
				OutErrors.Add(FString::Printf(TEXT("Asymmetric lane cost: '%s' -> '%s' costs %dd/%dr but the reverse costs %dd/%dr"),
					*Definition.RegionId.ToString(), *NeighborId.ToString(),
					Lane.TravelDays, Lane.Risk, Reverse->TravelDays, Reverse->Risk));
			}
		}
	}

	return OutErrors.IsEmpty();
}

FEclipseRoute FindRoute(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, const FEclipseRouteQuery& Query)
{
	FEclipseRoute Route;

	const FEclipseRegionDefinition* Start = FindDefinition(Definitions, Query.StartRegionId);
	const FEclipseRegionDefinition* Goal = FindDefinition(Definitions, Query.GoalRegionId);
	if (Start == nullptr)
	{
		Route.FailureReason = FString::Printf(TEXT("Unknown start region '%s'"), *Query.StartRegionId.ToString());
		return Route;
	}
	if (Goal == nullptr)
	{
		Route.FailureReason = FString::Printf(TEXT("Unknown goal region '%s'"), *Query.GoalRegionId.ToString());
		return Route;
	}

	if (Query.StartRegionId == Query.GoalRegionId)
	{
		Route.bValid = true;
		Route.RegionPath.Add(Query.StartRegionId);
		return Route;
	}

	struct FNodeCost
	{
		int32 Days = MAX_int32;
		int32 Risk = MAX_int32;
		FName Predecessor;
		bool bSettled = false;
	};

	TMap<FName, FNodeCost> Costs;
	Costs.Reserve(Definitions.Num());
	for (const FEclipseRegionDefinition& Definition : Definitions)
	{
		Costs.Add(Definition.RegionId, FNodeCost());
	}

	FNodeCost& StartCost = Costs.FindChecked(Query.StartRegionId);
	StartCost.Days = 0;
	StartCost.Risk = 0;

	/**
	 * Cheaper = fewer days, then less risk, then lexically smaller id. The third
	 * key is not cosmetic: without it two equal-cost routes swap places between
	 * runs and every assertion about "the" path becomes flaky.
	 */
	const auto IsCheaper = [](int32 DaysA, int32 RiskA, FName IdA, int32 DaysB, int32 RiskB, FName IdB)
	{
		if (DaysA != DaysB) { return DaysA < DaysB; }
		if (RiskA != RiskB) { return RiskA < RiskB; }
		return IdA.LexicalLess(IdB);
	};

	for (;;)
	{
		FName Current;
		int32 CurrentDays = MAX_int32;
		int32 CurrentRisk = MAX_int32;
		for (const TPair<FName, FNodeCost>& Pair : Costs)
		{
			if (Pair.Value.bSettled || Pair.Value.Days == MAX_int32)
			{
				continue;
			}
			if (Current.IsNone() || IsCheaper(Pair.Value.Days, Pair.Value.Risk, Pair.Key, CurrentDays, CurrentRisk, Current))
			{
				Current = Pair.Key;
				CurrentDays = Pair.Value.Days;
				CurrentRisk = Pair.Value.Risk;
			}
		}
		if (Current.IsNone() || Current == Query.GoalRegionId)
		{
			break;
		}
		Costs.FindChecked(Current).bSettled = true;

		const FEclipseRegionDefinition* CurrentDefinition = FindDefinition(Definitions, Current);
		if (CurrentDefinition == nullptr)
		{
			continue;
		}

		for (const FEclipseLaneDefinition& Lane : CurrentDefinition->Lanes)
		{
			FNodeCost* NeighborCost = Costs.Find(Lane.NeighborRegionId);
			if (NeighborCost == nullptr || NeighborCost->bSettled)
			{
				continue; // unknown neighbour is a validator error, not a routing crash
			}

			const FEclipseLaneTransit Transit = ResolveLaneTransit(State, Lane, Query.Mode, Query.Tuning);
			if (!Transit.bPassable)
			{
				continue; // a lane you cannot cross is not an edge
			}

			const int32 Days = CurrentDays + Transit.TravelDays;
			const int32 Risk = CurrentRisk + Transit.Risk;
			if (IsCheaper(Days, Risk, Lane.NeighborRegionId, NeighborCost->Days, NeighborCost->Risk, Lane.NeighborRegionId))
			{
				NeighborCost->Days = Days;
				NeighborCost->Risk = Risk;
				NeighborCost->Predecessor = Current;
			}
		}
	}

	const FNodeCost& GoalCost = Costs.FindChecked(Query.GoalRegionId);
	if (GoalCost.Days == MAX_int32)
	{
		// WHICH wall, not just "no". "Shut to columns but open to smugglers" and
		// "shut to everyone" are two different situations that ask the player for
		// two different things — hire Kaya, or take a Spire — and a refusal that
		// reads the same for both teaches neither (GDD 9.5: never silent, and a
		// message that cannot distinguish is a quieter kind of silent).
		const bool bSmugglersCouldGetThere =
			Query.Mode == EEclipseTransitMode::Military
			&& IsReachable(State, Definitions, Query.StartRegionId, Query.GoalRegionId, EEclipseTransitMode::Smuggler, Query.Tuning);

		Route.FailureReason = bSmugglersCouldGetThere
			? FString::Printf(TEXT("No military route from '%s' to '%s' — every path is gated or smuggler-only, but smugglers can reach it (GDD 3.1 rule 2)"),
				*Query.StartRegionId.ToString(), *Query.GoalRegionId.ToString())
			: FString::Printf(TEXT("No route at all from '%s' to '%s' — the graph is cut here, not gated (GDD 3.1 rule 1)"),
				*Query.StartRegionId.ToString(), *Query.GoalRegionId.ToString());
		return Route;
	}

	if (GoalCost.Risk > Query.MaxAcceptableRisk)
	{
		Route.FailureReason = FString::Printf(TEXT("Cheapest route costs %d risk, over the %d the caller will accept"),
			GoalCost.Risk, Query.MaxAcceptableRisk);
		return Route;
	}

	// Walk the predecessors back and turn them into legs in travel order.
	TArray<FName> Reversed;
	for (FName Walk = Query.GoalRegionId; !Walk.IsNone(); Walk = Costs.FindChecked(Walk).Predecessor)
	{
		Reversed.Add(Walk);
		if (Walk == Query.StartRegionId)
		{
			break;
		}
	}
	Algo::Reverse(Reversed);
	Route.RegionPath = MoveTemp(Reversed);

	for (int32 Index = 0; Index + 1 < Route.RegionPath.Num(); ++Index)
	{
		const FEclipseRegionDefinition* From = FindDefinition(Definitions, Route.RegionPath[Index]);
		const FEclipseLaneDefinition* Lane = From != nullptr ? From->FindLane(Route.RegionPath[Index + 1]) : nullptr;
		if (Lane == nullptr)
		{
			continue;
		}
		const FEclipseLaneTransit Transit = ResolveLaneTransit(State, *Lane, Query.Mode, Query.Tuning);

		FEclipseRouteStep& Step = Route.Steps.AddDefaulted_GetRef();
		Step.FromRegionId = Route.RegionPath[Index];
		Step.ToRegionId = Route.RegionPath[Index + 1];
		Step.TravelDays = Transit.TravelDays;
		Step.Risk = Transit.Risk;
		Step.bSmugglerLeg = Transit.bSmugglerLeg;

		Route.TotalTravelDays += Transit.TravelDays;
		Route.TotalRisk += Transit.Risk;
		Route.bUsesSmugglerLeg |= Transit.bSmugglerLeg;
	}

	Route.bValid = true;
	return Route;
}

bool IsRegionSupplied(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName RegionId, const FEclipseLaneTuning& Tuning)
{
	const FEclipseRegionState* Region = State.FindRegion(RegionId);
	if (Region == nullptr)
	{
		return false;
	}
	if (Region->Owner == EEclipseRegionOwner::Player)
	{
		return true; // ground you stand on supplies itself
	}

	for (const FEclipseRegionState& Source : State.Regions)
	{
		if (Source.Owner != EEclipseRegionOwner::Player)
		{
			continue;
		}

		FEclipseRouteQuery Query;
		Query.StartRegionId = Source.RegionId;
		Query.GoalRegionId = RegionId;
		Query.Mode = EEclipseTransitMode::Military; // convoys, not couriers
		Query.Tuning = Tuning;

		const FEclipseRoute Route = FindRoute(State, Definitions, Query);
		if (Route.bValid && Route.TotalTravelDays <= Tuning.MaxSupplyDays)
		{
			return true;
		}
	}
	return false;
}

EEclipseDominionResponseTier DeriveResponseTier(const FEclipseCampaignState& State)
{
	EEclipseDominionResponseTier Derived = EEclipseDominionResponseTier::Indifference;

	if (!State.StoryFlags.IsEmpty())
	{
		Derived = EEclipseDominionResponseTier::Nuisance;
	}

	int32 PlayerHeld = 0;
	bool bAnyContested = false;
	for (const FEclipseRegionState& Region : State.Regions)
	{
		if (Region.Owner == EEclipseRegionOwner::Player)
		{
			++PlayerHeld;
		}
		else if (Region.Owner == EEclipseRegionOwner::Contested)
		{
			bAnyContested = true;
		}
	}

	if (bAnyContested && Derived < EEclipseDominionResponseTier::Insurgency)
	{
		Derived = EEclipseDominionResponseTier::Insurgency;
	}
	if (PlayerHeld >= 2)
	{
		Derived = EEclipseDominionResponseTier::Rebellion;
	}

	// The ladder does not descend (see EEclipseDominionResponseTier).
	return FMath::Max(Derived, State.ResponseTier);
}

} // namespace EclipseStrategyLogic
