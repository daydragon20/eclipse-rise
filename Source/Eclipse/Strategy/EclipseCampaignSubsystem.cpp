#include "Strategy/EclipseCampaignSubsystem.h"

#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "EclipseSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "JsonObjectConverter.h"
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
			[](TMap<FName, TArray<uint8>>& Blocks)
			{
				TArray<uint8>* CampaignBlock = Blocks.Find(TEXT("Campaign"));
				if (CampaignBlock == nullptr || CampaignBlock->Num() < static_cast<int32>(sizeof(int32)))
				{
					return false;
				}
				*reinterpret_cast<int32*>(CampaignBlock->GetData()) = 2;
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

	// PLACEHOLDER(GDD 4.2.1): numbered recruits until SPEC-P1-07's name/trait
	// tables land; ids are deterministic per slot for test reproducibility.
	for (int32 Index = 0; Index < Setup->StartingRosterSize; ++Index)
	{
		FEclipseSoldierRecord Soldier;
		Soldier.SoldierId = FGuid(0x45434C53, 0x0BADCAFE, 0x00000000, Index + 1);
		Soldier.Name = FString::Printf(TEXT("Recruit %02d"), Index + 1);
		Soldier.OriginId = TEXT("Kessara");
		Soldier.Status = EEclipseSoldierStatus::Available;
		State.Roster.Add(Soldier);
	}

	UE_LOG(LogEclipse, Display, TEXT("New campaign: day %d, %d regions, %d soldiers."), State.Day, State.Regions.Num(), State.Roster.Num());
}

bool UEclipseCampaignSubsystem::CommitTransaction(const FEclipseCampaignTransaction& Transaction, FString& OutError)
{
	TArray<FEclipseAppliedMutation> Applied;
	if (!EclipseCampaignLogic::CommitTransaction(State, Transaction, Applied, OutError))
	{
		UE_LOG(LogEclipse, Warning, TEXT("Campaign transaction from '%s' rejected: %s"), *Transaction.Source.ToString(), *OutError);
		return false;
	}

	EmitEventsForApplied(Applied);
	return true;
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
