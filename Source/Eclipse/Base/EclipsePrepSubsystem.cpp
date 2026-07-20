#include "Base/EclipsePrepSubsystem.h"

#include "Base/EclipsePrepTypes.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Squad/EclipseRosterLogic.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "StructUtils/InstancedStruct.h"

void UEclipsePrepSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEclipseEventBusSubsystem* Bus = Collection.InitializeDependency<UEclipseEventBusSubsystem>();
	Collection.InitializeDependency<UEclipseCampaignSubsystem>();

	if (Bus != nullptr)
	{
		MissionSelectedHandle = Bus->Subscribe(
			EclipseTags::Event_Strategy_MissionSelected,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipsePrepSubsystem::OnMissionSelected),
			FEclipseStrategyEventPayload::StaticStruct());
	}

#if !UE_BUILD_SHIPPING
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Prep.AutoLaunch")) == nullptr)
	{
		AutoLaunchCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Prep.AutoLaunch"),
			TEXT("Launch the selected mission with default squad/loadout/insertion (Gauntlet loops, SPEC-P1-08)."),
			FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
			{
				FString Error;
				if (!AutoLaunch(Error))
				{
					UE_LOG(LogEclipse, Error, TEXT("AutoLaunch failed: %s"), *Error);
				}
			}),
			ECVF_Default);
	}
#endif
}

void UEclipsePrepSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	if (AutoLaunchCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(AutoLaunchCommand);
		AutoLaunchCommand = nullptr;
	}
#endif

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Unsubscribe(MissionSelectedHandle);
	}

	Super::Deinitialize();
}

void UEclipsePrepSubsystem::OnMissionSelected(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (const FEclipseStrategyEventPayload* Strategy = Payload.GetPtr<FEclipseStrategyEventPayload>())
	{
		SelectedRegionId = Strategy->RegionId;
		bIntelRevealed = false; // each briefing pays for its own secrets (Intel decays; so does relevance)
	}
}

const UEclipsePrepTuningAsset* UEclipsePrepSubsystem::ResolveTuning() const
{
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	return Setup != nullptr ? Setup->PrepTuning.LoadSynchronous() : nullptr;
}

TArray<EclipsePrepLogic::FEclipseLoadoutOption> UEclipsePrepSubsystem::ResolveLoadoutOptions() const
{
	TArray<EclipsePrepLogic::FEclipseLoadoutOption> Options;

	const UEclipsePrepTuningAsset* Tuning = ResolveTuning();
	const UDataTable* Table = Tuning != nullptr ? Tuning->LoadoutOptions.LoadSynchronous() : nullptr;
	if (Table == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Prep: no loadout options table — loadout list is empty (GDD 14.3.5)."));
		return Options;
	}

	Table->ForeachRow<FEclipseLoadoutOptionRow>(TEXT("PrepLoadouts"),
		[&Options](const FName& RowName, const FEclipseLoadoutOptionRow& Row)
		{
			EclipsePrepLogic::FEclipseLoadoutOption& Option = Options.AddDefaulted_GetRef();
			Option.OptionId = RowName;
			Option.LoadoutTag = Row.LoadoutTag;
			Option.RequiredUnlockTag = Row.RequiredUnlockTag;
		});
	return Options;
}

TArray<FGameplayTag> UEclipsePrepSubsystem::GetAvailableLoadoutTags() const
{
	TArray<FGameplayTag> Tags;
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		return Tags;
	}

	for (const EclipsePrepLogic::FEclipseLoadoutOption& Option :
		EclipsePrepLogic::GetAvailableLoadouts(Campaign->GetState(), ResolveLoadoutOptions()))
	{
		Tags.Add(Option.LoadoutTag);
	}
	return Tags;
}

bool UEclipsePrepSubsystem::SpendIntelForReveal(FString& OutError)
{
	if (SelectedRegionId.IsNone())
	{
		OutError = TEXT("No mission selected");
		return false;
	}
	if (bIntelRevealed)
	{
		OutError = TEXT("Briefing already revealed");
		return false;
	}

	const UEclipsePrepTuningAsset* Tuning = ResolveTuning();
	if (Tuning == nullptr)
	{
		OutError = TEXT("No prep tuning asset");
		return false;
	}

	UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	FEclipseCampaignTransaction Transaction;
	Transaction.Source = TEXT("PrepIntelReveal");
	FEclipseCampaignMutation& Spend = Transaction.Mutations.AddDefaulted_GetRef();
	Spend.Type = EEclipseCampaignMutationType::AdjustResource;
	Spend.ResourceType = EclipseTags::Resource_Intel.GetTag();
	Spend.Amount = -Tuning->IntelRevealCost;
	Spend.Reason = TEXT("IntelReveal");

	if (!Campaign->CommitTransaction(Transaction, OutError))
	{
		return false;
	}

	bIntelRevealed = true;
	return true;
}

bool UEclipsePrepSubsystem::LaunchMission(const TArray<FGuid>& SquadSoldierIds, FGameplayTag LoadoutTag, FName InsertionId, FString& OutError)
{
	if (SelectedRegionId.IsNone())
	{
		OutError = TEXT("No mission selected on the strategy map");
		return false;
	}

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipsePrepTuningAsset* Tuning = ResolveTuning();
	const int32 SquadSize = Tuning != nullptr ? Tuning->SquadSize : 2;

	if (!EclipsePrepLogic::ValidateSquadPick(Campaign->GetState(), SquadSoldierIds, SquadSize, OutError))
	{
		return false;
	}
	if (!EclipsePrepLogic::ValidateLoadout(Campaign->GetState(), ResolveLoadoutOptions(), LoadoutTag, OutError))
	{
		return false;
	}

	UEclipseEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>();
	if (Bus == nullptr)
	{
		OutError = TEXT("No event bus");
		return false;
	}

	FEclipsePrepEventPayload Payload;
	Payload.MissionId = SelectedRegionId;
	Payload.SquadSoldierIds = SquadSoldierIds;
	Payload.LoadoutTag = LoadoutTag;
	Payload.InsertionId = InsertionId;
	Payload.IntelLevel = bIntelRevealed ? 1 : 0;
	Bus->Broadcast(EclipseTags::Event_Prep_MissionLaunchRequested, FInstancedStruct::Make(Payload));

	SelectedRegionId = NAME_None;
	bIntelRevealed = false;
	return true;
}

bool UEclipsePrepSubsystem::AutoLaunch(FString& OutError)
{
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		OutError = TEXT("No campaign");
		return false;
	}

	const UEclipsePrepTuningAsset* Tuning = ResolveTuning();
	const int32 SquadSize = Tuning != nullptr ? Tuning->SquadSize : 2;

	TArray<FGuid> Squad;
	for (const FEclipseSoldierRecord& Soldier : Campaign->GetState().Roster)
	{
		if (EclipseRosterLogic::IsSoldierAvailableOnDay(Soldier, Campaign->GetState().Day))
		{
			Squad.Add(Soldier.SoldierId);
			if (Squad.Num() == SquadSize)
			{
				break;
			}
		}
	}

	const TArray<FGameplayTag> Loadouts = GetAvailableLoadoutTags();
	if (Loadouts.IsEmpty())
	{
		OutError = TEXT("No loadout options available");
		return false;
	}

	return LaunchMission(Squad, Loadouts[0], TEXT("Entry_Default"), OutError);
}
