#include "Economy/EclipseEconomySubsystem.h"

#include "Base/EclipseBaseSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Economy/EclipseEconomyDataAsset.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"

namespace
{
	TAutoConsoleVariable<int32> CVarEclipseLedgerSize(
		TEXT("Eclipse.Economy.LedgerSize"),
		20,
		TEXT("Number of ledger lines kept for Eclipse.Economy.Report (SPEC-P1-03: last 20)."),
		ECVF_Default);
}

void UEclipseEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEclipseEventBusSubsystem* Bus = Collection.InitializeDependency<UEclipseEventBusSubsystem>();
	Collection.InitializeDependency<UEclipseCampaignSubsystem>();

	if (Bus != nullptr)
	{
		DayAdvancedHandle = Bus->Subscribe(
			EclipseTags::Event_Campaign_DayAdvanced,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseEconomySubsystem::OnDayAdvanced),
			FEclipseCampaignEventPayload::StaticStruct());

		const FGameplayTag EconomyFamily = EclipseTags::Event_Economy_ResourcesChanged.GetTag().RequestDirectParent();
		EconomyEventsHandle = Bus->Subscribe(
			EconomyFamily,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseEconomySubsystem::OnEconomyEvent));
	}

#if !UE_BUILD_SHIPPING
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Economy.Report")) == nullptr)
	{
		ReportCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Economy.Report"),
			TEXT("Log balances and the recent ledger lines with reasons (SPEC-P1-03 DoD: every number's origin)."),
			FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
			{
				const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
				if (Campaign != nullptr)
				{
					const FEclipseCampaignState& State = Campaign->GetState();
					UE_LOG(LogEclipse, Display, TEXT("Economy report — day %d:"), State.Day);
					UE_LOG(LogEclipse, Display, TEXT("  Credits: %d | Materials: %d | Intel: %d"),
						State.GetBalance(EclipseTags::Resource_Credits),
						State.GetBalance(EclipseTags::Resource_Materials),
						State.GetBalance(EclipseTags::Resource_Intel));
				}
				UE_LOG(LogEclipse, Display, TEXT("  Ledger (%d lines, oldest first):"), LedgerLog.Num());
				for (const FString& Line : LedgerLog)
				{
					UE_LOG(LogEclipse, Display, TEXT("    %s"), *Line);
				}
			}),
			ECVF_Default);
	}
#endif
}

void UEclipseEconomySubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	if (ReportCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ReportCommand);
		ReportCommand = nullptr;
	}
#endif

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Unsubscribe(DayAdvancedHandle);
		Bus->Unsubscribe(EconomyEventsHandle);
	}

	Super::Deinitialize();
}

FEclipseEconomyTickParams UEclipseEconomySubsystem::ResolveTickParams() const
{
	FEclipseEconomyTickParams Params;
	Params.CreditsTag = EclipseTags::Resource_Credits.GetTag();
	Params.IntelTag = EclipseTags::Resource_Intel.GetTag();

	// Facility output rides the same tick as region yields (SPEC-P2-03: one
	// deterministic tick); the base wrapper owns the base-data resolution.
	if (const UEclipseBaseSubsystem* Base = GetGameInstance()->GetSubsystem<UEclipseBaseSubsystem>())
	{
		Params.FacilityYields = Base->ComputeTodaysFacilityYields();
	}

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	if (Setup == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Economy: no active campaign setup — tick runs with empty params (GDD 14.3.5)."));
		return Params;
	}

	const UEclipseEconomyDataAsset* EconomyData = Setup->EconomyData.LoadSynchronous();
	if (EconomyData == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Economy: campaign setup has no economy data asset — tick runs with empty params (GDD 14.3.5)."));
		return Params;
	}

	Params.WagePerSoldierPerDay = EconomyData->WagePerSoldierPerDay;
	Params.IntelDecayPerWeek = EconomyData->IntelDecayPerWeek;
	Params.IntelDecayIntervalDays = EconomyData->IntelDecayIntervalDays;
	Params.ContestedYieldFactor = EconomyData->ContestedYieldFactor;

	if (const UEclipseRegionGraphAsset* Graph = Setup->RegionGraph.LoadSynchronous())
	{
		for (const FEclipseRegionDefinition& Definition : Graph->Regions)
		{
			FEclipseRegionYieldParams& RegionParams = Params.Regions.AddDefaulted_GetRef();
			RegionParams.RegionId = Definition.RegionId;
			RegionParams.BaseYieldPerDay = Definition.BaseYieldPerDay;
		}
	}

	if (const UDataTable* Items = EconomyData->ProductionItems.LoadSynchronous())
	{
		Items->ForeachRow<FEclipseProductionItemRow>(TEXT("EconomyTickParams"),
			[&Params](const FName& RowName, const FEclipseProductionItemRow& Row)
			{
				FEclipseProductionItemParams& Item = Params.ProductionItems.AddDefaulted_GetRef();
				Item.ItemId = RowName;
				Item.CostMaterials = Row.CostMaterials;
				Item.CostCredits = Row.CostCredits;
				Item.TimeDays = Row.TimeDays;
				Item.ResultLoadoutTag = Row.ResultLoadoutTag;
			});
	}

	return Params;
}

bool UEclipseEconomySubsystem::TryQueueProduction(FName ItemId, FString& OutError)
{
	UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		OutError = TEXT("No campaign subsystem");
		return false;
	}

	FEclipseCampaignTransaction Transaction;
	if (!EclipseEconomyLogic::BuildProductionOrder(
		Campaign->GetState(), ResolveTickParams(), ItemId,
		EclipseTags::Resource_Materials.GetTag(), Transaction, OutError))
	{
		return false;
	}

	return Campaign->CommitTransaction(Transaction, OutError);
}

void UEclipseEconomySubsystem::OnDayAdvanced(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		return;
	}

	FEclipseCampaignTransaction Tick;
	if (!EclipseEconomyLogic::BuildDayTick(Campaign->GetState(), ResolveTickParams(), Tick))
	{
		return;
	}

	FString Error;
	if (!Campaign->CommitTransaction(Tick, Error))
	{
		// A rejected tick means the pure core produced an invalid transaction —
		// a programming error worth a loud log, never a silent skip.
		UE_LOG(LogEclipse, Error, TEXT("Economy day tick rejected: %s"), *Error);
	}
}

void UEclipseEconomySubsystem::OnEconomyEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>();
	if (Economy == nullptr)
	{
		return;
	}

	if (EventTag == EclipseTags::Event_Economy_ResourcesChanged.GetTag())
	{
		AppendLedgerLine(FString::Printf(TEXT("%+d %s -> %d (%s)"),
			Economy->Delta,
			*Economy->ResourceType.ToString(),
			Economy->NewBalance,
			*Economy->Reason.ToString()));
	}
	else if (EventTag == EclipseTags::Event_Economy_ProductionQueued.GetTag())
	{
		AppendLedgerLine(FString::Printf(TEXT("Queued %s (ETA %d days)"), *Economy->ItemId.ToString(), Economy->EtaDays));
	}
	else if (EventTag == EclipseTags::Event_Economy_ProductionCompleted.GetTag())
	{
		AppendLedgerLine(FString::Printf(TEXT("Completed %s -> unlock %s"), *Economy->ItemId.ToString(), *Economy->LoadoutTag.ToString()));
	}
}

void UEclipseEconomySubsystem::AppendLedgerLine(const FString& Line)
{
	LedgerLog.Add(Line);
	const int32 MaxLines = FMath::Max(1, CVarEclipseLedgerSize.GetValueOnGameThread());
	while (LedgerLog.Num() > MaxLines)
	{
		LedgerLog.RemoveAt(0);
	}
}
