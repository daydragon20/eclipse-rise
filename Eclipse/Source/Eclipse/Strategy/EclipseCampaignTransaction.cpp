#include "Strategy/EclipseCampaignTransaction.h"

uint32 FEclipseCampaignState::ComputeStateHash() const
{
	uint32 Hash = GetTypeHash(SchemaVersion);
	Hash = HashCombine(Hash, GetTypeHash(Day));

	// TMap iteration order is insertion-dependent; hash over key-sorted pairs so
	// two states with identical content always hash identically.
	TArray<FGameplayTag> Keys;
	Wallet.GenerateKeyArray(Keys);
	Keys.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });
	for (const FGameplayTag& Key : Keys)
	{
		Hash = HashCombine(Hash, GetTypeHash(Key.GetTagName()));
		Hash = HashCombine(Hash, GetTypeHash(Wallet[Key]));
	}

	for (const FEclipseRegionState& Region : Regions)
	{
		Hash = HashCombine(Hash, GetTypeHash(Region.RegionId));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Region.Owner)));
		Hash = HashCombine(Hash, GetTypeHash(Region.Unrest));
		Hash = HashCombine(Hash, GetTypeHash(Region.GarrisonStrength));
	}

	for (const FEclipseSoldierRecord& Soldier : Roster)
	{
		Hash = HashCombine(Hash, GetTypeHash(Soldier.SoldierId));
		Hash = HashCombine(Hash, GetTypeHash(Soldier.Name));
		Hash = HashCombine(Hash, GetTypeHash(Soldier.OriginId));
		Hash = HashCombine(Hash, GetTypeHash(Soldier.TraitId));
		Hash = HashCombine(Hash, GetTypeHash(Soldier.ClassId));
		Hash = HashCombine(Hash, GetTypeHash(Soldier.MissionsServed));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Soldier.Status)));
		Hash = HashCombine(Hash, GetTypeHash(Soldier.WoundedUntilDay));
	}

	for (const FEclipseMemorialEntry& Entry : Memorial)
	{
		Hash = HashCombine(Hash, GetTypeHash(Entry.SoldierId));
		Hash = HashCombine(Hash, GetTypeHash(Entry.Name));
		Hash = HashCombine(Hash, GetTypeHash(Entry.MissionsServed));
		Hash = HashCombine(Hash, GetTypeHash(Entry.Cause));
		Hash = HashCombine(Hash, GetTypeHash(Entry.Day));
	}

	for (const FEclipseProductionOrder& Order : ProductionQueue)
	{
		Hash = HashCombine(Hash, GetTypeHash(Order.ItemId));
		Hash = HashCombine(Hash, GetTypeHash(Order.CompletesOnDay));
	}

	for (const FGameplayTag& LoadoutTag : UnlockedLoadoutTags)
	{
		Hash = HashCombine(Hash, GetTypeHash(LoadoutTag.GetTagName()));
	}

	for (const FEclipseFacilityState& Facility : BaseState.Facilities)
	{
		Hash = HashCombine(Hash, GetTypeHash(Facility.SlotId));
		Hash = HashCombine(Hash, GetTypeHash(Facility.FacilityId));
		Hash = HashCombine(Hash, GetTypeHash(Facility.Level));
		Hash = HashCombine(Hash, GetTypeHash(Facility.DaysRemaining));
		for (const FGuid& SoldierId : Facility.AssignedSoldierIds)
		{
			Hash = HashCombine(Hash, GetTypeHash(SoldierId));
		}
	}

	for (const FGameplayTag& StoryFlag : StoryFlags)
	{
		Hash = HashCombine(Hash, GetTypeHash(StoryFlag.GetTagName()));
	}

	// v6 (GDD 9.4): two campaigns that differ only in the empire's temperature
	// are not the same campaign — routing risk reads this.
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(ResponseTier)));

	return Hash;
}

namespace EclipseCampaignLogic
{

bool ValidateMutation(const FEclipseCampaignState& State, const FEclipseCampaignMutation& Mutation, FString& OutError)
{
	switch (Mutation.Type)
	{
	case EEclipseCampaignMutationType::AdjustResource:
	{
		if (!Mutation.ResourceType.IsValid())
		{
			OutError = TEXT("AdjustResource: invalid resource tag");
			return false;
		}
		// Balances never go negative: scarcity is a design pillar (GDD 6.1), an
		// overdraft is always a caller bug or an affordability check that belongs
		// in UI, not a state the campaign can be in.
		const int64 NewBalance = static_cast<int64>(State.GetBalance(Mutation.ResourceType)) + Mutation.Amount;
		if (NewBalance < 0)
		{
			OutError = FString::Printf(TEXT("AdjustResource: %s balance would go negative (%lld)"), *Mutation.ResourceType.ToString(), NewBalance);
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::SetRegionOwner:
	{
		const FEclipseRegionState* Region = State.FindRegion(Mutation.RegionId);
		if (Region == nullptr)
		{
			OutError = FString::Printf(TEXT("SetRegionOwner: unknown region '%s'"), *Mutation.RegionId.ToString());
			return false;
		}
		if (Region->Owner == Mutation.NewOwner)
		{
			OutError = FString::Printf(TEXT("SetRegionOwner: region '%s' already has that owner"), *Mutation.RegionId.ToString());
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::AddSoldier:
	{
		if (!Mutation.SoldierRecord.SoldierId.IsValid())
		{
			OutError = TEXT("AddSoldier: soldier id is not set");
			return false;
		}
		if (Mutation.SoldierRecord.Name.IsEmpty())
		{
			OutError = TEXT("AddSoldier: soldier has no name (people, not units - GDD Pillar 3)");
			return false;
		}
		if (State.FindSoldier(Mutation.SoldierRecord.SoldierId) != nullptr)
		{
			OutError = FString::Printf(TEXT("AddSoldier: duplicate soldier id %s"), *Mutation.SoldierRecord.SoldierId.ToString());
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::KillSoldier:
	{
		const FEclipseSoldierRecord* Soldier = State.FindSoldier(Mutation.SoldierId);
		if (Soldier == nullptr)
		{
			OutError = FString::Printf(TEXT("KillSoldier: unknown soldier %s"), *Mutation.SoldierId.ToString());
			return false;
		}
		if (Soldier->Status == EEclipseSoldierStatus::Dead)
		{
			OutError = FString::Printf(TEXT("KillSoldier: %s is already dead"), *Soldier->Name);
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::AddMemorialEntry:
	{
		if (Mutation.MemorialEntry.Name.IsEmpty())
		{
			OutError = TEXT("AddMemorialEntry: entry has no name");
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::QueueProduction:
	{
		if (Mutation.ProductionItemId.IsNone())
		{
			OutError = TEXT("QueueProduction: no item id");
			return false;
		}
		if (Mutation.EtaDays <= 0)
		{
			OutError = TEXT("QueueProduction: ETA must be positive");
			return false;
		}
		// PLACEHOLDER(GDD 5.3): one production slot until Workshop facility levels land.
		if (!State.ProductionQueue.IsEmpty())
		{
			OutError = TEXT("QueueProduction: the production slot is occupied");
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::CompleteProduction:
	{
		const FEclipseProductionOrder* Order = State.ProductionQueue.FindByPredicate(
			[&Mutation](const FEclipseProductionOrder& Candidate) { return Candidate.ItemId == Mutation.ProductionItemId; });
		if (Order == nullptr)
		{
			OutError = FString::Printf(TEXT("CompleteProduction: '%s' is not in the queue"), *Mutation.ProductionItemId.ToString());
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::WoundSoldier:
	{
		const FEclipseSoldierRecord* Soldier = State.FindSoldier(Mutation.SoldierId);
		if (Soldier == nullptr)
		{
			OutError = FString::Printf(TEXT("WoundSoldier: unknown soldier %s"), *Mutation.SoldierId.ToString());
			return false;
		}
		if (Soldier->Status == EEclipseSoldierStatus::Dead)
		{
			OutError = FString::Printf(TEXT("WoundSoldier: %s is dead"), *Soldier->Name);
			return false;
		}
		if (Mutation.EtaDays <= 0)
		{
			OutError = TEXT("WoundSoldier: recovery days must be positive");
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::AdvanceDay:
		return true;
	case EEclipseCampaignMutationType::StartConstruction:
	{
		if (Mutation.EtaDays <= 0)
		{
			OutError = TEXT("StartConstruction: build days must be positive");
			return false;
		}
		// State-only invariants via the shared base core (SPEC-P2-03); the
		// wrapper's ValidateBuildOrder already covered slot-graph placement,
		// level data and the funds pre-check - and the spend mutations in this
		// same transaction remain the hard funds gate (P1-03 pattern).
		int32 TargetLevel = 0;
		return EclipseBaseLogic::ValidateStartConstructionState(State.BaseState, Mutation.SlotId, Mutation.FacilityId, TargetLevel, OutError);
	}
	case EEclipseCampaignMutationType::RushConstruction:
	{
		// The credit price travels as an AdjustResource mutation in the same
		// transaction (atomic); here only the state precondition matters.
		return EclipseBaseLogic::ValidateRushState(State.BaseState, Mutation.SlotId, OutError);
	}
	case EEclipseCampaignMutationType::AssignStaff:
	{
		const bool bAssign = Mutation.StaffRoleTag.IsValid();
		if (bAssign)
		{
			const FEclipseSoldierRecord* Soldier = State.FindSoldier(Mutation.SoldierId);
			if (Soldier == nullptr)
			{
				OutError = FString::Printf(TEXT("AssignStaff: unknown soldier %s"), *Mutation.SoldierId.ToString());
				return false;
			}
			if (Soldier->Status != EEclipseSoldierStatus::Available)
			{
				// The staffing dilemma cuts both ways: only a deployable soldier
				// can be committed to base duty (SPEC-P2-03 staffing v1).
				OutError = FString::Printf(TEXT("AssignStaff: %s is not available"), *Soldier->Name);
				return false;
			}
		}
		// Unassign carries no roster requirement: a dead or wounded soldier must
		// always be releasable from a post (GDD 14.3.5 - no stuck states).
		if (!EclipseBaseLogic::ValidateStaffChange(State.BaseState, Mutation.SlotId, Mutation.SoldierId, bAssign, OutError))
		{
			return false;
		}
		if (bAssign)
		{
			// The staff cap is enforced HERE, in the mutation layer, with the
			// DA_BaseTuning value stamped onto the mutation - not only in the
			// wrapper's pre-check (step-3 review). Over-cap assignment is
			// yield-harmless (EffectiveStaffCount clamps) but would still lock
			// a soldier undeployable for zero effect, so any caller reaching
			// the API past the cap is a bug worth rejecting loudly.
			const FEclipseFacilityState* Facility = State.BaseState.FindBySlot(Mutation.SlotId);
			if (Facility != nullptr && Facility->AssignedSoldierIds.Num() >= FMath::Max(0, Mutation.MaxStaffPerSite))
			{
				OutError = FString::Printf(TEXT("AssignStaff: slot '%s' is fully staffed (%d/%d)"),
					*Mutation.SlotId.ToString(), Facility->AssignedSoldierIds.Num(), FMath::Max(0, Mutation.MaxStaffPerSite));
				return false;
			}
		}
		return true;
	}
	case EEclipseCampaignMutationType::SetStoryFlag:
	{
		if (!Mutation.StoryFlagTag.IsValid())
		{
			OutError = TEXT("SetStoryFlag: invalid beat tag");
			return false;
		}
		if (State.StoryFlags.Contains(Mutation.StoryFlagTag))
		{
			// Same no-op discipline as SetRegionOwner: composers filter via
			// ShouldCommitBeat; a duplicate reaching the API is a caller bug
			// worth rejecting loudly (SPEC-P2-04 beat idempotence).
			OutError = FString::Printf(TEXT("SetStoryFlag: beat %s already set"), *Mutation.StoryFlagTag.ToString());
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::SetResponseTier:
	{
		// Raise-only, and strictly: re-committing the tier you are already on is
		// a caller that did not check, and "the empire relaxed" is a story event
		// that would have to arrive as one (GDD 9.4 — tiers escalate).
		if (Mutation.ResponseTier <= State.ResponseTier)
		{
			OutError = FString::Printf(TEXT("SetResponseTier: %s does not escalate past the current %s"),
				*UEnum::GetValueAsString(Mutation.ResponseTier), *UEnum::GetValueAsString(State.ResponseTier));
			return false;
		}
		if (Mutation.ResponseTier > EEclipseDominionResponseTier::Existential)
		{
			OutError = TEXT("SetResponseTier: tier is off the GDD 9.4 ladder");
			return false;
		}
		return true;
	}
	case EEclipseCampaignMutationType::MarkMissionServed:
	{
		const FEclipseSoldierRecord* Soldier = State.FindSoldier(Mutation.SoldierId);
		if (Soldier == nullptr)
		{
			OutError = FString::Printf(TEXT("MarkMissionServed: unknown soldier %s"), *Mutation.SoldierId.ToString());
			return false;
		}
		if (Soldier->Status == EEclipseSoldierStatus::Dead)
		{
			// Order service marks before KillSoldier in the same transaction: the
			// fatal mission still counts on the roster row (Pillar 3), and the
			// memorial snapshot (Record + 1) then matches the row exactly.
			OutError = FString::Printf(TEXT("MarkMissionServed: %s is already dead"), *Soldier->Name);
			return false;
		}
		return true;
	}
	default:
		OutError = TEXT("Unknown mutation type");
		return false;
	}
}

FEclipseAppliedMutation ApplyMutation(FEclipseCampaignState& State, const FEclipseCampaignMutation& Mutation)
{
	FEclipseAppliedMutation Applied;
	Applied.Mutation = Mutation;

	switch (Mutation.Type)
	{
	case EEclipseCampaignMutationType::AdjustResource:
	{
		int32& Balance = State.Wallet.FindOrAdd(Mutation.ResourceType);
		Balance += Mutation.Amount;
		check(Balance >= 0);
		Applied.NewBalance = Balance;
		break;
	}
	case EEclipseCampaignMutationType::SetRegionOwner:
	{
		FEclipseRegionState* Region = State.Regions.FindByPredicate(
			[&Mutation](const FEclipseRegionState& R) { return R.RegionId == Mutation.RegionId; });
		check(Region != nullptr);
		Applied.OldOwner = Region->Owner;
		Region->Owner = Mutation.NewOwner;
		break;
	}
	case EEclipseCampaignMutationType::AddSoldier:
	{
		State.Roster.Add(Mutation.SoldierRecord);
		break;
	}
	case EEclipseCampaignMutationType::KillSoldier:
	{
		FEclipseSoldierRecord* Soldier = State.Roster.FindByPredicate(
			[&Mutation](const FEclipseSoldierRecord& S) { return S.SoldierId == Mutation.SoldierId; });
		check(Soldier != nullptr);
		Soldier->Status = EEclipseSoldierStatus::Dead;
		// The casualty vacates their base post in this same apply: a stale id in
		// AssignedSoldierIds would keep earning the analyst bonus for a dead
		// soldier (the ghost-analyst yield gap, step-3 review) and hold the
		// slot's cap. The commit turns the releases into StaffAssigned(none)
		// facts (SPEC-P2-03 events table).
		Applied.StaffReleases = EclipseBaseLogic::ReleaseSoldierAssignments(State.BaseState, Mutation.SoldierId);
		break;
	}
	case EEclipseCampaignMutationType::AddMemorialEntry:
	{
		State.Memorial.Add(Mutation.MemorialEntry);
		break;
	}
	case EEclipseCampaignMutationType::QueueProduction:
	{
		FEclipseProductionOrder Order;
		Order.ItemId = Mutation.ProductionItemId;
		Order.CompletesOnDay = State.Day + Mutation.EtaDays;
		State.ProductionQueue.Add(Order);
		Applied.ProductionCompletesOnDay = Order.CompletesOnDay;
		break;
	}
	case EEclipseCampaignMutationType::CompleteProduction:
	{
		const int32 Removed = State.ProductionQueue.RemoveAll(
			[&Mutation](const FEclipseProductionOrder& Candidate) { return Candidate.ItemId == Mutation.ProductionItemId; });
		check(Removed > 0);
		if (Mutation.LoadoutTag.IsValid())
		{
			State.UnlockedLoadoutTags.AddUnique(Mutation.LoadoutTag);
		}
		break;
	}
	case EEclipseCampaignMutationType::WoundSoldier:
	{
		FEclipseSoldierRecord* Soldier = State.Roster.FindByPredicate(
			[&Mutation](const FEclipseSoldierRecord& S) { return S.SoldierId == Mutation.SoldierId; });
		check(Soldier != nullptr);
		Soldier->Status = EEclipseSoldierStatus::Wounded;
		Soldier->WoundedUntilDay = State.Day + Mutation.EtaDays;
		// Same contract as KillSoldier: a wounded soldier cannot work a post
		// (staffing requires Available - SPEC-P2-03 staffing v1), so the wound
		// releases them; re-staffing after recovery is a new explicit order.
		Applied.StaffReleases = EclipseBaseLogic::ReleaseSoldierAssignments(State.BaseState, Mutation.SoldierId);
		break;
	}
	case EEclipseCampaignMutationType::AdvanceDay:
	{
		State.Day += 1;
		// Construction advances with the day, inside this same commit
		// (SPEC-P2-03 clock rules: "construction progresses while you fight").
		// Tuning rides on the mutation (stamped from DA_BaseTuning by the
		// subsystem; defaults = spec values), keeping this apply pure.
		EclipseBaseLogic::FEclipseBaseTuningParams Tuning;
		Tuning.CrewDayReduction = Mutation.CrewDayReduction;
		Tuning.MaxCrewPerSite = Mutation.MaxStaffPerSite;
		Applied.FacilityCompletions = EclipseBaseLogic::TickConstructionDay(State.BaseState, Tuning);
		break;
	}
	case EEclipseCampaignMutationType::StartConstruction:
	{
		FEclipseFacilityState& Facility = EclipseBaseLogic::StartConstruction(State.BaseState, Mutation.SlotId, Mutation.FacilityId, Mutation.EtaDays);
		Applied.TargetLevel = Facility.Level + 1;
		Applied.EtaDay = State.Day + Facility.DaysRemaining;
		break;
	}
	case EEclipseCampaignMutationType::RushConstruction:
	{
		FEclipseFacilityState* Facility = State.BaseState.FindBySlot(Mutation.SlotId);
		check(Facility != nullptr);
		Applied.FacilityCompletions.Add(EclipseBaseLogic::ApplyRush(*Facility));
		break;
	}
	case EEclipseCampaignMutationType::AssignStaff:
	{
		const bool bAssign = Mutation.StaffRoleTag.IsValid();
		FEclipseFacilityState* Facility = EclipseBaseLogic::ApplyStaffChange(State.BaseState, Mutation.SlotId, Mutation.SoldierId, bAssign);
		check(Facility != nullptr);
		// Role is positional (SPEC-P2-03): capture it at apply time so the
		// emitted fact stays true even if a later mutation changes the site.
		Applied.bAssignedAsCrew = bAssign && Facility->DaysRemaining > 0;
		Applied.StaffedFacilityId = Facility->FacilityId;
		break;
	}
	case EEclipseCampaignMutationType::SetStoryFlag:
	{
		State.StoryFlags.Add(Mutation.StoryFlagTag);
		break;
	}
	case EEclipseCampaignMutationType::SetResponseTier:
	{
		Applied.OldResponseTier = State.ResponseTier;
		State.ResponseTier = Mutation.ResponseTier;
		break;
	}
	case EEclipseCampaignMutationType::MarkMissionServed:
	{
		FEclipseSoldierRecord* Soldier = State.Roster.FindByPredicate(
			[&Mutation](const FEclipseSoldierRecord& S) { return S.SoldierId == Mutation.SoldierId; });
		check(Soldier != nullptr);
		Soldier->MissionsServed += 1;
		break;
	}
	default:
		checkNoEntry();
		break;
	}

	Applied.DayAfter = State.Day;
	return Applied;
}

bool CommitTransaction(FEclipseCampaignState& State, const FEclipseCampaignTransaction& Transaction, TArray<FEclipseAppliedMutation>& OutApplied, FString& OutError)
{
	OutApplied.Reset();

	if (Transaction.Mutations.IsEmpty())
	{
		OutError = TEXT("Empty transaction");
		return false;
	}

	// Validate against a scratch copy that *applies as it validates*: later
	// mutations must be legal against the effects of earlier ones in the same
	// transaction (e.g. AddSoldier then KillSoldier), while the real state stays
	// untouched until the whole set proves valid.
	FEclipseCampaignState Scratch = State;
	for (int32 Index = 0; Index < Transaction.Mutations.Num(); ++Index)
	{
		FString MutationError;
		if (!ValidateMutation(Scratch, Transaction.Mutations[Index], MutationError))
		{
			OutError = FString::Printf(TEXT("Mutation %d rejected: %s"), Index, *MutationError);
			return false;
		}
		ApplyMutation(Scratch, Transaction.Mutations[Index]);
	}

	for (const FEclipseCampaignMutation& Mutation : Transaction.Mutations)
	{
		OutApplied.Add(ApplyMutation(State, Mutation));
	}
	return true;
}

} // namespace EclipseCampaignLogic
