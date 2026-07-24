#include "Strategy/EclipseCampaignSubsystem.h"

#include "Base/EclipseBaseTypes.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "EclipseSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "JsonObjectConverter.h"
#include "Squad/EclipseRosterLogic.h"
#include "Squad/EclipseRosterTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "StructUtils/InstancedStruct.h"

namespace
{
	/** Serialize member-by-member: the byte layout is a deliberate contract (schema v1), not reflection-order luck. */
	void SerializeState(FArchive& Ar, FEclipseCampaignState& State)
	{
		Ar << State.SchemaVersion;
		Ar << State.Day;

		int32 WalletCount = State.Wallet.Num();
		Ar << WalletCount;
		if (Ar.IsLoading())
		{
			State.Wallet.Reset();
			for (int32 Index = 0; Index < WalletCount; ++Index)
			{
				FName TagName;
				int32 Balance = 0;
				Ar << TagName << Balance;
				State.Wallet.Add(FGameplayTag::RequestGameplayTag(TagName), Balance);
			}
		}
		else
		{
			// Key-sorted for canonical bytes: identical states produce identical files.
			TArray<FGameplayTag> Keys;
			State.Wallet.GenerateKeyArray(Keys);
			Keys.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });
			for (const FGameplayTag& Key : Keys)
			{
				FName TagName = Key.GetTagName();
				int32 Balance = State.Wallet[Key];
				Ar << TagName << Balance;
			}
		}

		int32 RegionCount = State.Regions.Num();
		Ar << RegionCount;
		if (Ar.IsLoading())
		{
			State.Regions.SetNum(RegionCount);
		}
		for (FEclipseRegionState& Region : State.Regions)
		{
			Ar << Region.RegionId;
			uint8 Owner = static_cast<uint8>(Region.Owner);
			Ar << Owner;
			Region.Owner = static_cast<EEclipseRegionOwner>(Owner);
			Ar << Region.Unrest;
			Ar << Region.GarrisonStrength;
		}

		int32 RosterCount = State.Roster.Num();
		Ar << RosterCount;
		if (Ar.IsLoading())
		{
			State.Roster.SetNum(RosterCount);
		}
		for (FEclipseSoldierRecord& Soldier : State.Roster)
		{
			Ar << Soldier.SoldierId;
			Ar << Soldier.Name;
			Ar << Soldier.OriginId;
			Ar << Soldier.TraitId;
			Ar << Soldier.MissionsServed;
			uint8 Status = static_cast<uint8>(Soldier.Status);
			Ar << Status;
			Soldier.Status = static_cast<EEclipseSoldierStatus>(Status);
			Ar << Soldier.WoundedUntilDay;
		}

		int32 MemorialCount = State.Memorial.Num();
		Ar << MemorialCount;
		if (Ar.IsLoading())
		{
			State.Memorial.SetNum(MemorialCount);
		}
		for (FEclipseMemorialEntry& Entry : State.Memorial)
		{
			Ar << Entry.SoldierId;
			Ar << Entry.Name;
			Ar << Entry.MissionsServed;
			Ar << Entry.Cause;
			Ar << Entry.Day;
		}

		int32 QueueCount = State.ProductionQueue.Num();
		Ar << QueueCount;
		if (Ar.IsLoading())
		{
			State.ProductionQueue.SetNum(QueueCount);
		}
		for (FEclipseProductionOrder& Order : State.ProductionQueue)
		{
			Ar << Order.ItemId;
			Ar << Order.CompletesOnDay;
		}

		// Schema v2 (SPEC-P1-03): unlocked loadout tags, appended at block end so
		// the v1->v2 migration is a pure tail-append of an empty count.
		int32 UnlockCount = State.UnlockedLoadoutTags.Num();
		Ar << UnlockCount;
		if (Ar.IsLoading())
		{
			State.UnlockedLoadoutTags.Reset();
			for (int32 Index = 0; Index < UnlockCount; ++Index)
			{
				FName TagName;
				Ar << TagName;
				State.UnlockedLoadoutTags.Add(FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false));
			}
		}
		else
		{
			for (const FGameplayTag& LoadoutTag : State.UnlockedLoadoutTags)
			{
				FName TagName = LoadoutTag.GetTagName();
				Ar << TagName;
			}
		}

		// Schema v3 (SPEC-P2-01): soldier ClassIds, appended at block end in
		// roster order — tail-append keeps the migration mechanical (GDD 14.3.6).
		// The read is version-gated on the block's own leading SchemaVersion: a
		// v2 block simply has no tail, and its soldiers come home classless
		// Recruits (NAME_None — the GDD 4.2.3 default), never a crash.
		if (Ar.IsLoading())
		{
			if (State.SchemaVersion >= 3)
			{
				int32 ClassIdCount = 0;
				Ar << ClassIdCount;
				for (int32 Index = 0; Index < ClassIdCount && !Ar.IsError(); ++Index)
				{
					FName ClassId;
					Ar << ClassId;
					// Count mismatch tolerated: extra names drop, short tails leave
					// the remaining soldiers classless (GDD 14.3.5).
					if (State.Roster.IsValidIndex(Index))
					{
						State.Roster[Index].ClassId = ClassId;
					}
				}
			}
		}
		else
		{
			int32 ClassIdCount = State.Roster.Num();
			Ar << ClassIdCount;
			for (FEclipseSoldierRecord& Soldier : State.Roster)
			{
				Ar << Soldier.ClassId;
			}
		}

		// Schema v4 (SPEC-P2-03): the walkable-base facility states, appended at
		// block end - the same mechanical tail as v2/v3. The read is version-gated:
		// a pre-v4 block has no tail and keeps the type's seeded default. An empty
		// base is not a legal campaign (5.3.1 mandatory core), so empty-after-read
		// also lands on the spec start state - Command Center pre-built at L1
		// (SPEC-P2-03 locked decision 2) - never a crash (GDD 14.3.5).
		if (Ar.IsLoading())
		{
			if (State.SchemaVersion >= 4)
			{
				State.BaseState.Facilities.Reset();
				int32 FacilityCount = 0;
				Ar << FacilityCount;
				for (int32 Index = 0; Index < FacilityCount && !Ar.IsError(); ++Index)
				{
					FEclipseFacilityState& Facility = State.BaseState.Facilities.AddDefaulted_GetRef();
					Ar << Facility.SlotId;
					Ar << Facility.FacilityId;
					Ar << Facility.Level;
					Ar << Facility.DaysRemaining;
					int32 StaffCount = 0;
					Ar << StaffCount;
					for (int32 StaffIndex = 0; StaffIndex < StaffCount && !Ar.IsError(); ++StaffIndex)
					{
						FGuid SoldierId;
						Ar << SoldierId;
						Facility.AssignedSoldierIds.Add(SoldierId);
					}
				}
			}
			if (State.BaseState.Facilities.IsEmpty())
			{
				State.BaseState = FEclipseBaseState();
			}
		}
		else
		{
			int32 FacilityCount = State.BaseState.Facilities.Num();
			Ar << FacilityCount;
			for (FEclipseFacilityState& Facility : State.BaseState.Facilities)
			{
				Ar << Facility.SlotId;
				Ar << Facility.FacilityId;
				Ar << Facility.Level;
				Ar << Facility.DaysRemaining;
				int32 StaffCount = Facility.AssignedSoldierIds.Num();
				Ar << StaffCount;
				for (FGuid& SoldierId : Facility.AssignedSoldierIds)
				{
					Ar << SoldierId;
				}
			}
		}

		// Schema v5 (SPEC-P2-04): committed story beats, appended at block end —
		// the same mechanical tail as v2..v4. A pre-v5 block has no tail: that
		// campaign simply hasn't reached a beat yet (empty flags), never a crash.
		if (Ar.IsLoading())
		{
			State.StoryFlags.Reset();
			if (State.SchemaVersion >= 5)
			{
				int32 FlagCount = 0;
				Ar << FlagCount;
				for (int32 Index = 0; Index < FlagCount && !Ar.IsError(); ++Index)
				{
					FName FlagName;
					Ar << FlagName;
					const FGameplayTag Flag = FGameplayTag::RequestGameplayTag(FlagName, /*ErrorIfNotFound*/ false);
					if (Flag.IsValid())
					{
						// AddUnique defends the set-only invariant against
						// corrupt or hand-edited files (duplicates must not
						// round-trip).
						State.StoryFlags.AddUnique(Flag);
					}
					else
					{
						// Removed content must never brick a save (GDD 14.3.5)
						// — but vanished story progress deserves a trace.
						UE_LOG(LogEclipse, Warning, TEXT("Campaign load: story flag '%s' is no longer a registered tag — dropped."), *FlagName.ToString());
					}
				}
			}
			// Whatever shape came in, the in-memory state is now current.
			// (Convention: this normalize lives at the LAST tail — v6 moves it.)
			State.SchemaVersion = 5;
		}
		else
		{
			int32 FlagCount = State.StoryFlags.Num();
			Ar << FlagCount;
			for (const FGameplayTag& Flag : State.StoryFlags)
			{
				FName FlagName = Flag.GetTagName();
				Ar << FlagName;
			}
		}
	}
}

void UEclipseCampaignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The bus must outlive us so Deinitialize-order issues can't drop events.
	Collection.InitializeDependency<UEclipseEventBusSubsystem>();

	if (UEclipseSaveSubsystem* SaveSubsystem = Collection.InitializeDependency<UEclipseSaveSubsystem>())
	{
		SaveSubsystem->RegisterProvider(this);

		// Migration scaffold (SPEC-P1-02): a no-op v0->v1 step proves the pipeline
		// end-to-end in CI before the first real schema break needs it.
		SaveSubsystem->RegisterMigration(0, UEclipseSaveSubsystem::FEclipseSaveMigration::CreateLambda(
			[](TMap<FName, TArray<uint8>>&) { return true; }));

		// v1 -> v2 (SPEC-P1-03): the Campaign block gained a trailing
		// UnlockedLoadoutTags array. Rewrite the leading state SchemaVersion int
		// and append an empty array count — old campaigns simply own no unlocks.
		SaveSubsystem->RegisterMigration(1, UEclipseSaveSubsystem::FEclipseSaveMigration::CreateLambda(
			[](TMap<FName, TArray<uint8>>& SaveBlocks)
			{
				TArray<uint8>* CampaignBlock = SaveBlocks.Find(TEXT("Campaign"));
				if (CampaignBlock == nullptr || CampaignBlock->Num() < static_cast<int32>(sizeof(int32)))
				{
					return false;
				}
				*reinterpret_cast<int32*>(CampaignBlock->GetData()) = 2;
				const int32 EmptyCount = 0;
				CampaignBlock->Append(reinterpret_cast<const uint8*>(&EmptyCount), sizeof(int32));
				return true;
			}));

		// v2 -> v3 (SPEC-P2-01): the Campaign block gained a trailing roster
		// ClassId array. Same mechanical tail-append: pre-class soldiers come
		// home as classless Recruits (ClassId None — GDD 4.2.3's default), and
		// the serializer's own version gate backstops any block this step missed.
		SaveSubsystem->RegisterMigration(2, UEclipseSaveSubsystem::FEclipseSaveMigration::CreateLambda(
			[](TMap<FName, TArray<uint8>>& SaveBlocks)
			{
				TArray<uint8>* CampaignBlock = SaveBlocks.Find(TEXT("Campaign"));
				if (CampaignBlock == nullptr || CampaignBlock->Num() < static_cast<int32>(sizeof(int32)))
				{
					return false;
				}
				*reinterpret_cast<int32*>(CampaignBlock->GetData()) = 3;
				const int32 EmptyCount = 0;
				CampaignBlock->Append(reinterpret_cast<const uint8*>(&EmptyCount), sizeof(int32));
				return true;
			}));

		// v3 -> v4 (SPEC-P2-03): the Campaign block gained the trailing base
		// facility tail. Same mechanical empty-count append; the serializer's
		// empty-base rule then lands migrated campaigns deterministically on the
		// spec start state — Command Center pre-built at L1, slots B–D unbuilt
		// (SPEC-P2-03 locked decision 2), never a crash (GDD 14.3.5).
		SaveSubsystem->RegisterMigration(3, UEclipseSaveSubsystem::FEclipseSaveMigration::CreateLambda(
			[](TMap<FName, TArray<uint8>>& SaveBlocks)
			{
				TArray<uint8>* CampaignBlock = SaveBlocks.Find(TEXT("Campaign"));
				if (CampaignBlock == nullptr || CampaignBlock->Num() < static_cast<int32>(sizeof(int32)))
				{
					return false;
				}
				*reinterpret_cast<int32*>(CampaignBlock->GetData()) = 4;
				const int32 EmptyCount = 0;
				CampaignBlock->Append(reinterpret_cast<const uint8*>(&EmptyCount), sizeof(int32));
				return true;
			}));

		// v4 -> v5 (SPEC-P2-04): the Campaign block gained the trailing story-flag
		// tail. Same mechanical empty-count append — a pre-story campaign simply
		// hasn't reached a beat yet; the serializer's version gate backstops.
		SaveSubsystem->RegisterMigration(4, UEclipseSaveSubsystem::FEclipseSaveMigration::CreateLambda(
			[](TMap<FName, TArray<uint8>>& SaveBlocks)
			{
				TArray<uint8>* CampaignBlock = SaveBlocks.Find(TEXT("Campaign"));
				if (CampaignBlock == nullptr || CampaignBlock->Num() < static_cast<int32>(sizeof(int32)))
				{
					return false;
				}
				*reinterpret_cast<int32*>(CampaignBlock->GetData()) = 5;
				const int32 EmptyCount = 0;
				CampaignBlock->Append(reinterpret_cast<const uint8*>(&EmptyCount), sizeof(int32));
				return true;
			}));
	}

	RegisterConsoleCommands();
}

void UEclipseCampaignSubsystem::Deinitialize()
{
	UnregisterConsoleCommands();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEclipseSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UEclipseSaveSubsystem>())
		{
			SaveSubsystem->UnregisterProvider(this);
		}
	}

	Super::Deinitialize();
}

void UEclipseCampaignSubsystem::StartNewCampaign(const UEclipseCampaignSetupAsset* Setup)
{
	State = FEclipseCampaignState();
	ActiveSetup = Setup;

	if (Setup == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("StartNewCampaign: no setup asset — starting an empty campaign (GDD 14.3.5 graceful default)."));
		return;
	}

	State.Day = Setup->StartingDay;
	State.Wallet = Setup->StartingResources;

	if (const UEclipseRegionGraphAsset* Graph = Setup->RegionGraph.LoadSynchronous())
	{
		for (const FEclipseRegionDefinition& Definition : Graph->Regions)
		{
			FEclipseRegionState Region;
			Region.RegionId = Definition.RegionId;
			Region.Owner = Definition.StartingOwner;
			Region.Unrest = Definition.StartingUnrest;
			Region.GarrisonStrength = Definition.StartingGarrison;
			State.Regions.Add(Region);
		}
	}
	else
	{
		UE_LOG(LogEclipse, Warning, TEXT("StartNewCampaign: setup asset has no region graph — campaign starts with zero regions."));
	}

	// SPEC-P1-07: soldiers are generated people, not numbers. Pools resolve from
	// data; empty/missing pools degrade to numbered recruits inside the core.
	EclipseRosterLogic::FEclipseNameGenerationParams NameParams;
	const FName OriginId = TEXT("Kessara");
	if (const UEclipseRosterTuningAsset* RosterTuning = Setup->RosterTuning.LoadSynchronous())
	{
		if (const UDataTable* NamePools = RosterTuning->NamePools.LoadSynchronous())
		{
			if (const FEclipseNamePoolRow* Pool = NamePools->FindRow<FEclipseNamePoolRow>(OriginId, TEXT("StartNewCampaign"), /*bWarnIfMissing*/ false))
			{
				NameParams.FirstNames = Pool->FirstNames;
				NameParams.LastNames = Pool->LastNames;
			}
		}
		if (const UDataTable* TraitStubs = RosterTuning->TraitStubs.LoadSynchronous())
		{
			NameParams.TraitIds = TraitStubs->GetRowNames();
		}
	}
	else
	{
		UE_LOG(LogEclipse, Warning, TEXT("StartNewCampaign: no roster tuning asset — recruits get fallback names (GDD 14.3.5)."));
	}

	// Pre-classed authored recruits (SPEC-P2-01 decision 2): the starting roster
	// cycles the DT_ClassDefs rows in table order — deterministic, data-driven,
	// and the Academy assignment flow stays Phase 3. Missing table = classless
	// Recruits with a logged warning (GDD 14.3.5), never a crash.
	TArray<FName> StartingClassIds;
	if (const UDataTable* ClassDefs = Setup->ClassDefs.LoadSynchronous())
	{
		if (ClassDefs->GetRowStruct() == FEclipseClassDefRow::StaticStruct())
		{
			StartingClassIds = ClassDefs->GetRowNames();
		}
	}
	if (StartingClassIds.IsEmpty())
	{
		UE_LOG(LogEclipse, Warning, TEXT("StartNewCampaign: no class defs table — recruits start classless (GDD 14.3.5)."));
	}

	for (int32 Index = 0; Index < Setup->StartingRosterSize; ++Index)
	{
		FEclipseSoldierRecord Soldier = EclipseRosterLogic::GenerateSoldier(OriginId, NameParams, Index + 1);
		if (!StartingClassIds.IsEmpty())
		{
			Soldier.ClassId = StartingClassIds[Index % StartingClassIds.Num()];
		}
		State.Roster.Add(MoveTemp(Soldier));
	}

	UE_LOG(LogEclipse, Display, TEXT("New campaign: day %d, %d regions, %d soldiers."), State.Day, State.Regions.Num(), State.Roster.Num());
}

bool UEclipseCampaignSubsystem::CommitTransaction(const FEclipseCampaignTransaction& Transaction, FString& OutError)
{
	FEclipseCampaignTransaction Stamped = Transaction;
	StampAdvanceDayTuning(Stamped);

	TArray<FEclipseAppliedMutation> Applied;
	if (!EclipseCampaignLogic::CommitTransaction(State, Stamped, Applied, OutError))
	{
		UE_LOG(LogEclipse, Warning, TEXT("Campaign transaction from '%s' rejected: %s"), *Transaction.Source.ToString(), *OutError);
		return false;
	}

	EmitEventsForApplied(Applied);
	return true;
}

void UEclipseCampaignSubsystem::StampAdvanceDayTuning(FEclipseCampaignTransaction& Transaction) const
{
	const UEclipseBaseTuningAsset* Tuning = ActiveSetup != nullptr ? ActiveSetup->BaseTuning.LoadSynchronous() : nullptr;
	if (Tuning == nullptr)
	{
		return; // mutation defaults are the SPEC-P2-03 values (GDD 14.3.5)
	}
	for (FEclipseCampaignMutation& Mutation : Transaction.Mutations)
	{
		if (Mutation.Type == EEclipseCampaignMutationType::AdvanceDay)
		{
			Mutation.CrewDayReduction = Tuning->CrewDayReduction;
			Mutation.MaxStaffPerSite = Tuning->MaxCrewPerSite;
		}
	}
}

bool UEclipseCampaignSubsystem::CommitTransactionBP(const FEclipseCampaignTransaction& Transaction)
{
	FString Error;
	return CommitTransaction(Transaction, Error);
}

void UEclipseCampaignSubsystem::EmitEventsForApplied(const TArray<FEclipseAppliedMutation>& Applied)
{
	UGameInstance* GameInstance = GetGameInstance();
	UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr;
	if (Bus == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Campaign commit applied but no event bus is available — facts not broadcast."));
		return;
	}

	// Build steps completed inside a commit (day tick or rush) become facts in
	// that same commit (SPEC-P2-03 clock rules), including the staff release
	// as StaffAssigned with an empty role (review R6/d).
	auto BroadcastCompletions = [Bus](const TArray<EclipseBaseLogic::FEclipseFacilityCompletion>& Completions)
	{
		for (const EclipseBaseLogic::FEclipseFacilityCompletion& Completion : Completions)
		{
			if (Completion.NewLevel < 1)
			{
				continue; // guarded no-op (e.g. rushing an operational site): nothing happened, no fact
			}

			FEclipseBaseEventPayload Payload;
			Payload.SlotId = Completion.SlotId;
			Payload.FacilityId = Completion.FacilityId;
			Payload.Level = Completion.NewLevel;
			Bus->Broadcast(Completion.NewLevel == 1 ? EclipseTags::Event_Base_FacilityBuilt : EclipseTags::Event_Base_FacilityUpgraded,
				FInstancedStruct::Make(Payload));

			for (const FGuid& ReleasedId : Completion.ReleasedSoldierIds)
			{
				FEclipseBaseEventPayload Release;
				Release.SlotId = Completion.SlotId;
				Release.FacilityId = Completion.FacilityId;
				Release.SoldierId = ReleasedId;
				// RoleTag stays empty: "none = unassign" (SPEC-P2-03 events table).
				Bus->Broadcast(EclipseTags::Event_Base_StaffAssigned, FInstancedStruct::Make(Release));
			}
		}
	};

	for (const FEclipseAppliedMutation& Record : Applied)
	{
		const FEclipseCampaignMutation& Mutation = Record.Mutation;
		switch (Mutation.Type)
		{
		case EEclipseCampaignMutationType::AdjustResource:
		{
			FEclipseEconomyEventPayload Payload;
			Payload.ResourceType = Mutation.ResourceType;
			Payload.Delta = Mutation.Amount;
			Payload.NewBalance = Record.NewBalance;
			Payload.Reason = Mutation.Reason;
			Bus->Broadcast(EclipseTags::Event_Economy_ResourcesChanged, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::SetRegionOwner:
		{
			FEclipseStrategyEventPayload Payload;
			Payload.RegionId = Mutation.RegionId;
			Payload.OldOwner = UEnum::GetValueAsName(Record.OldOwner);
			Payload.NewOwner = UEnum::GetValueAsName(Mutation.NewOwner);
			Bus->Broadcast(EclipseTags::Event_Strategy_RegionControlChanged, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::AddSoldier:
		{
			FEclipseRosterEventPayload Payload;
			Payload.SoldierId = Mutation.SoldierRecord.SoldierId;
			Payload.Day = Record.DayAfter;
			Bus->Broadcast(EclipseTags::Event_Roster_SoldierAdded, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::KillSoldier:
		{
			FEclipseRosterEventPayload Payload;
			Payload.SoldierId = Mutation.SoldierId;
			Payload.Cause = Mutation.Cause;
			Payload.Day = Record.DayAfter;
			Bus->Broadcast(EclipseTags::Event_Roster_SoldierDied, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::AddMemorialEntry:
		{
			FEclipseRosterEventPayload Payload;
			Payload.SoldierId = Mutation.MemorialEntry.SoldierId;
			Payload.Cause = Mutation.MemorialEntry.Cause;
			Payload.Day = Mutation.MemorialEntry.Day;
			Bus->Broadcast(EclipseTags::Event_Memorial_EntryAdded, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::WoundSoldier:
		{
			FEclipseRosterEventPayload Payload;
			Payload.SoldierId = Mutation.SoldierId;
			Payload.Cause = Mutation.Cause;
			Payload.Day = Record.DayAfter;
			Payload.DaysOut = Mutation.EtaDays;
			Bus->Broadcast(EclipseTags::Event_Roster_SoldierWounded, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::QueueProduction:
		{
			FEclipseEconomyEventPayload Payload;
			Payload.ItemId = Mutation.ProductionItemId;
			Payload.EtaDays = Record.ProductionCompletesOnDay - Record.DayAfter;
			Payload.Reason = Mutation.Reason;
			Bus->Broadcast(EclipseTags::Event_Economy_ProductionQueued, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::CompleteProduction:
		{
			FEclipseEconomyEventPayload Payload;
			Payload.ItemId = Mutation.ProductionItemId;
			Payload.LoadoutTag = Mutation.LoadoutTag;
			Payload.Reason = Mutation.Reason;
			Bus->Broadcast(EclipseTags::Event_Economy_ProductionCompleted, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::AdvanceDay:
		{
			FEclipseCampaignEventPayload Payload;
			Payload.Day = Record.DayAfter;
			Bus->Broadcast(EclipseTags::Event_Campaign_DayAdvanced, FInstancedStruct::Make(Payload));
			// Construction that finished on this day's tick (SPEC-P2-03).
			BroadcastCompletions(Record.FacilityCompletions);
			break;
		}
		case EEclipseCampaignMutationType::SetStoryFlag:
		{
			FEclipseStoryEventPayload Payload;
			Payload.BeatTag = Record.Mutation.StoryFlagTag;
			Payload.Day = Record.DayAfter;
			Bus->Broadcast(EclipseTags::Event_Story_BeatReached, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::StartConstruction:
		{
			FEclipseBaseEventPayload Payload;
			Payload.SlotId = Mutation.SlotId;
			Payload.FacilityId = Mutation.FacilityId;
			Payload.Level = Record.TargetLevel;
			Payload.EtaDay = Record.EtaDay;
			Bus->Broadcast(EclipseTags::Event_Base_ConstructionStarted, FInstancedStruct::Make(Payload));
			break;
		}
		case EEclipseCampaignMutationType::RushConstruction:
		{
			// Rush = instant completion: FacilityBuilt/Upgraded fires in this
			// same commit as the rush mutation (SPEC-P2-03 clock rules).
			BroadcastCompletions(Record.FacilityCompletions);
			break;
		}
		case EEclipseCampaignMutationType::AssignStaff:
		{
			FEclipseBaseEventPayload Payload;
			Payload.SlotId = Mutation.SlotId;
			Payload.FacilityId = Record.StaffedFacilityId;
			Payload.SoldierId = Mutation.SoldierId;
			if (Mutation.StaffRoleTag.IsValid())
			{
				// The positional role the assignment actually took at apply time.
				Payload.RoleTag = Record.bAssignedAsCrew ? EclipseTags::Base_Staff_Crew.GetTag() : EclipseTags::Base_Staff_Analyst.GetTag();
			}
			Bus->Broadcast(EclipseTags::Event_Base_StaffAssigned, FInstancedStruct::Make(Payload));
			break;
		}
		default:
			break;
		}
	}
}

FString UEclipseCampaignSubsystem::ExportStateAsJson() const
{
	FString Json;
	if (!FJsonObjectConverter::UStructToJsonObjectString(State, Json))
	{
		UE_LOG(LogEclipse, Error, TEXT("ExportStateAsJson failed."));
	}
	return Json;
}

void UEclipseCampaignSubsystem::WriteSaveData(FArchive& Archive)
{
	SerializeState(Archive, State);
}

bool UEclipseCampaignSubsystem::ReadSaveData(FArchive& Archive)
{
	FEclipseCampaignState Loaded;
	SerializeState(Archive, Loaded);
	if (Archive.IsError())
	{
		UE_LOG(LogEclipse, Error, TEXT("Campaign save block is corrupt — load rejected."));
		return false;
	}

	State = MoveTemp(Loaded);
	return true;
}

void UEclipseCampaignSubsystem::RegisterConsoleCommands()
{
#if !UE_BUILD_SHIPPING
	IConsoleManager& Console = IConsoleManager::Get();

	// Same multi-instance guard as the event bus: first campaign subsystem wins.
	if (Console.FindConsoleObject(TEXT("Eclipse.Campaign.ExportJson")) != nullptr)
	{
		return;
	}

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Campaign.ExportJson"),
		TEXT("Log the campaign state as JSON (GDD 12.2 rule 4 debug export)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			UE_LOG(LogEclipse, Display, TEXT("CampaignState:\n%s"), *ExportStateAsJson());
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Campaign.AdvanceDay"),
		TEXT("Commit an AdvanceDay transaction (debug)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			FEclipseCampaignTransaction Transaction;
			Transaction.Source = TEXT("DebugConsole");
			FEclipseCampaignMutation& Mutation = Transaction.Mutations.AddDefaulted_GetRef();
			Mutation.Type = EEclipseCampaignMutationType::AdvanceDay;
			FString Error;
			CommitTransaction(Transaction, Error);
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Campaign.GrantResource"),
		TEXT("Usage: Eclipse.Campaign.GrantResource <Resource.Tag> <Amount> — commit a debug resource adjustment."),
		FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
		{
			if (Args.Num() != 2)
			{
				UE_LOG(LogEclipse, Error, TEXT("Usage: Eclipse.Campaign.GrantResource <Resource.Tag> <Amount>"));
				return;
			}

			FEclipseCampaignTransaction Transaction;
			Transaction.Source = TEXT("DebugConsole");
			FEclipseCampaignMutation& Mutation = Transaction.Mutations.AddDefaulted_GetRef();
			Mutation.Type = EEclipseCampaignMutationType::AdjustResource;
			Mutation.ResourceType = FGameplayTag::RequestGameplayTag(FName(*Args[0]), /*ErrorIfNotFound*/ false);
			Mutation.Amount = FCString::Atoi(*Args[1]);
			Mutation.Reason = TEXT("DebugGrant");

			FString Error;
			if (!CommitTransaction(Transaction, Error))
			{
				UE_LOG(LogEclipse, Error, TEXT("GrantResource failed: %s"), *Error);
			}
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Roster.Kill"),
		TEXT("Usage: Eclipse.Roster.Kill <RosterIndex> — debug permadeath incl. memorial (SPEC-P1-07 save/pipeline testing)."),
		FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
		{
			const int32 Index = Args.Num() == 1 ? FCString::Atoi(*Args[0]) : INDEX_NONE;
			if (!State.Roster.IsValidIndex(Index))
			{
				UE_LOG(LogEclipse, Error, TEXT("Usage: Eclipse.Roster.Kill <RosterIndex 0..%d>"), State.Roster.Num() - 1);
				return;
			}
			const FEclipseSoldierRecord& Soldier = State.Roster[Index];

			FEclipseCampaignTransaction Transaction;
			Transaction.Source = TEXT("DebugConsole");
			FEclipseCampaignMutation& Kill = Transaction.Mutations.AddDefaulted_GetRef();
			Kill.Type = EEclipseCampaignMutationType::KillSoldier;
			Kill.SoldierId = Soldier.SoldierId;
			Kill.Cause = TEXT("DebugConsole");
			FEclipseCampaignMutation& Memorial = Transaction.Mutations.AddDefaulted_GetRef();
			Memorial.Type = EEclipseCampaignMutationType::AddMemorialEntry;
			Memorial.MemorialEntry.SoldierId = Soldier.SoldierId;
			Memorial.MemorialEntry.Name = Soldier.Name;
			Memorial.MemorialEntry.MissionsServed = Soldier.MissionsServed;
			Memorial.MemorialEntry.Cause = TEXT("DebugConsole");
			Memorial.MemorialEntry.Day = State.Day;

			FString Error;
			if (!CommitTransaction(Transaction, Error))
			{
				UE_LOG(LogEclipse, Error, TEXT("Roster.Kill failed: %s"), *Error);
			}
		}),
		ECVF_Default));
#endif
}

void UEclipseCampaignSubsystem::UnregisterConsoleCommands()
{
#if !UE_BUILD_SHIPPING
	for (IConsoleObject* Command : ConsoleCommands)
	{
		if (Command != nullptr)
		{
			IConsoleManager::Get().UnregisterConsoleObject(Command);
		}
	}
	ConsoleCommands.Reset();
#endif
}
