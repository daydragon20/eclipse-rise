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
	case EEclipseCampaignMutationType::AdvanceDay:
		return true;
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
	case EEclipseCampaignMutationType::AdvanceDay:
	{
		State.Day += 1;
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
