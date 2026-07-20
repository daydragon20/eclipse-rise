#include "Strategy/EclipseStrategySubsystem.h"

#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseStrategyLogic.h"
#include "StructUtils/InstancedStruct.h"

void UEclipseStrategySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UEclipseEventBusSubsystem>();
	Collection.InitializeDependency<UEclipseCampaignSubsystem>();

#if !UE_BUILD_SHIPPING
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Strategy.FlipRegion")) == nullptr)
	{
		FlipRegionCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Strategy.FlipRegion"),
			TEXT("Usage: Eclipse.Strategy.FlipRegion <RegionId> — debug-cycle owner Dominion -> Contested -> Player."),
			FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
			{
				if (Args.Num() != 1)
				{
					UE_LOG(LogEclipse, Error, TEXT("Usage: Eclipse.Strategy.FlipRegion <RegionId>"));
					return;
				}

				UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
				const FEclipseRegionState* Region = Campaign != nullptr ? Campaign->GetState().FindRegion(FName(*Args[0])) : nullptr;
				if (Region == nullptr)
				{
					UE_LOG(LogEclipse, Error, TEXT("FlipRegion: unknown region '%s'"), *Args[0]);
					return;
				}

				FEclipseCampaignTransaction Transaction;
				Transaction.Source = TEXT("DebugConsole");
				FEclipseCampaignMutation& Mutation = Transaction.Mutations.AddDefaulted_GetRef();
				Mutation.Type = EEclipseCampaignMutationType::SetRegionOwner;
				Mutation.RegionId = Region->RegionId;
				Mutation.NewOwner = Region->Owner == EEclipseRegionOwner::Dominion ? EEclipseRegionOwner::Contested
					: Region->Owner == EEclipseRegionOwner::Contested ? EEclipseRegionOwner::Player
					: EEclipseRegionOwner::Dominion;

				FString Error;
				if (!Campaign->CommitTransaction(Transaction, Error))
				{
					UE_LOG(LogEclipse, Error, TEXT("FlipRegion failed: %s"), *Error);
				}
			}),
			ECVF_Default);
	}
#endif
}

void UEclipseStrategySubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	if (FlipRegionCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(FlipRegionCommand);
		FlipRegionCommand = nullptr;
	}
#endif

	Super::Deinitialize();
}

const UEclipseRegionGraphAsset* UEclipseStrategySubsystem::ResolveGraph() const
{
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	return Setup != nullptr ? Setup->RegionGraph.LoadSynchronous() : nullptr;
}

const FEclipseMissionOfferRow* UEclipseStrategySubsystem::FindOfferForType(const UEclipseRegionGraphAsset& Graph, EEclipseRegionType RegionType) const
{
	const UDataTable* Offers = Graph.MissionOffers.LoadSynchronous();
	if (Offers == nullptr)
	{
		return nullptr;
	}

	const FEclipseMissionOfferRow* Found = nullptr;
	Offers->ForeachRow<FEclipseMissionOfferRow>(TEXT("StrategyOffers"),
		[&Found, RegionType](const FName&, const FEclipseMissionOfferRow& Row)
		{
			if (Found == nullptr && Row.RegionType == RegionType)
			{
				Found = &Row;
			}
		});
	return Found;
}

TArray<FEclipseMissionOfferView> UEclipseStrategySubsystem::GetAvailableOffers() const
{
	TArray<FEclipseMissionOfferView> Views;

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipseRegionGraphAsset* Graph = ResolveGraph();
	if (Campaign == nullptr || Graph == nullptr)
	{
		// Missing data = empty board, logged, never a crash (GDD 14.3.5).
		UE_LOG(LogEclipse, Warning, TEXT("Strategy: no campaign/graph — the board is empty."));
		return Views;
	}

	const TArray<FName> LegalTargets = EclipseStrategyLogic::GetLegalMissionTargets(Campaign->GetState(), Graph->Regions);
	for (const FName& RegionId : LegalTargets)
	{
		const FEclipseRegionDefinition* Definition = Graph->Regions.FindByPredicate(
			[RegionId](const FEclipseRegionDefinition& D) { return D.RegionId == RegionId; });
		const FEclipseMissionOfferRow* Offer = Definition != nullptr ? FindOfferForType(*Graph, Definition->RegionType) : nullptr;
		if (Offer == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Strategy: region '%s' has no offer row for its type — skipped (GDD 14.3.5)."), *RegionId.ToString());
			continue;
		}

		FEclipseMissionOfferView& View = Views.AddDefaulted_GetRef();
		View.RegionId = RegionId;
		View.TemplateId = Offer->TemplateId;
		View.ContextLine = Offer->ContextLine;
		View.RewardCredits = Offer->RewardCredits;
		View.RewardMaterials = Offer->RewardMaterials;
		View.RewardIntel = Offer->RewardIntel;
	}
	return Views;
}

bool UEclipseStrategySubsystem::TryGetOffer(FName RegionId, FEclipseMissionOfferView& OutOffer) const
{
	const UEclipseRegionGraphAsset* Graph = ResolveGraph();
	if (Graph == nullptr)
	{
		return false;
	}

	const FEclipseRegionDefinition* Definition = Graph->Regions.FindByPredicate(
		[RegionId](const FEclipseRegionDefinition& D) { return D.RegionId == RegionId; });
	const FEclipseMissionOfferRow* Offer = Definition != nullptr ? FindOfferForType(*Graph, Definition->RegionType) : nullptr;
	if (Offer == nullptr)
	{
		return false;
	}

	OutOffer.RegionId = RegionId;
	OutOffer.TemplateId = Offer->TemplateId;
	OutOffer.ContextLine = Offer->ContextLine;
	OutOffer.RewardCredits = Offer->RewardCredits;
	OutOffer.RewardMaterials = Offer->RewardMaterials;
	OutOffer.RewardIntel = Offer->RewardIntel;
	return true;
}

bool UEclipseStrategySubsystem::SelectMission(FName RegionId, FString& OutError)
{
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipseRegionGraphAsset* Graph = ResolveGraph();
	if (Campaign == nullptr || Graph == nullptr)
	{
		OutError = TEXT("No active campaign/region graph");
		return false;
	}

	if (!EclipseStrategyLogic::IsMissionTargetLegal(Campaign->GetState(), Graph->Regions, RegionId, OutError))
	{
		return false;
	}

	const FEclipseRegionDefinition* Definition = Graph->Regions.FindByPredicate(
		[RegionId](const FEclipseRegionDefinition& D) { return D.RegionId == RegionId; });
	const FEclipseMissionOfferRow* Offer = Definition != nullptr ? FindOfferForType(*Graph, Definition->RegionType) : nullptr;
	if (Offer == nullptr)
	{
		OutError = FString::Printf(TEXT("Region '%s' has no mission offer"), *RegionId.ToString());
		return false;
	}

	UEclipseEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>();
	if (Bus == nullptr)
	{
		OutError = TEXT("No event bus");
		return false;
	}

	FEclipseStrategyEventPayload Payload;
	Payload.RegionId = RegionId;
	Payload.TemplateId = Offer->TemplateId;
	Bus->Broadcast(EclipseTags::Event_Strategy_MissionSelected, FInstancedStruct::Make(Payload));
	return true;
}
