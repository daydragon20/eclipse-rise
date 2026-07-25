#include "Strategy/EclipseStrategySubsystem.h"

#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Quests/EclipseStoryLogic.h"
#include "Quests/EclipseStoryTypes.h"
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

const UDataTable* UEclipseStrategySubsystem::GetValidatedStoryTable(const UEclipseCampaignSubsystem& Campaign, const UEclipseRegionGraphAsset& Graph) const
{
	// Missing table = the campaign runs on region offers alone (GDD 14.3.5).
	const UEclipseCampaignSetupAsset* Setup = Campaign.GetActiveSetup();
	const UDataTable* StoryTable = Setup != nullptr ? Setup->StoryMissions.LoadSynchronous() : nullptr;
	if (StoryTable == nullptr)
	{
		return nullptr;
	}

	const UScriptStruct* RowStruct = StoryTable->GetRowStruct();
	const bool bUsable = RowStruct != nullptr && RowStruct->IsChildOf(FEclipseStoryMissionRow::StaticStruct());

	// Degradation must be loud, once per table — not once per process, and
	// never silent (GDD 14.3.5): a forgotten completion beat is a forever-
	// repeatable pin (econ-relevant by decision 10), an unknown region a story
	// that can never surface, and two same-unlock pins on one region a tie the
	// table order silently decides.
	if (!ValidatedStoryTables.Contains(FObjectKey(StoryTable)))
	{
		ValidatedStoryTables.Add(FObjectKey(StoryTable));
		if (!bUsable)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Strategy: StoryMissions table '%s' has the wrong row struct — story pins disabled, region offers stand (GDD 14.3.5)."), *StoryTable->GetName());
		}
		else
		{
			TSet<FString> SeenPins;
			StoryTable->ForeachRow<FEclipseStoryMissionRow>(TEXT("StoryPinValidation"),
				[&Graph, &SeenPins, StoryTable](const FName& RowName, const FEclipseStoryMissionRow& Row)
				{
					if (!Row.CompletionBeatTag.IsValid())
					{
						UE_LOG(LogEclipse, Warning, TEXT("Strategy: story row '%s' in '%s' has no CompletionBeatTag — its pin never retires, a repeatable story offer (GDD 14.3.5)."), *RowName.ToString(), *StoryTable->GetName());
					}
					if (!Graph.Regions.ContainsByPredicate([&Row](const FEclipseRegionDefinition& D) { return D.RegionId == Row.PinnedRegionId; }))
					{
						UE_LOG(LogEclipse, Warning, TEXT("Strategy: story row '%s' in '%s' pins unknown region '%s' — the mission can never surface (GDD 14.3.5)."), *RowName.ToString(), *StoryTable->GetName(), *Row.PinnedRegionId.ToString());
					}
					bool bAlreadySeen = false;
					SeenPins.Add(Row.PinnedRegionId.ToString() + TEXT("|") + Row.UnlockBeatTag.ToString(), &bAlreadySeen);
					if (bAlreadySeen)
					{
						UE_LOG(LogEclipse, Warning, TEXT("Strategy: story row '%s' in '%s' double-pins region '%s' with the same unlock — table order decides the tie (GDD 14.3.5)."), *RowName.ToString(), *StoryTable->GetName(), *Row.PinnedRegionId.ToString());
					}
				});
		}
	}
	return bUsable ? StoryTable : nullptr;
}

bool UEclipseStrategySubsystem::ResolveOfferForRegion(const UEclipseRegionGraphAsset& Graph, FName RegionId, FEclipseMissionOfferView& OutOffer) const
{
	// Story pin first (SPEC-P2-04 locked decision 4).
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UDataTable* StoryTable = Campaign != nullptr ? GetValidatedStoryTable(*Campaign, Graph) : nullptr;
	if (StoryTable != nullptr)
	{
		TArray<const FEclipseStoryMissionRow*> Rows;
		StoryTable->ForeachRow<FEclipseStoryMissionRow>(TEXT("StoryPins"),
			[&Rows](const FName&, const FEclipseStoryMissionRow& Row) { Rows.Add(&Row); });

		const FGameplayTagContainer Flags = FGameplayTagContainer::CreateFromArray(Campaign->GetState().StoryFlags);
		if (const FEclipseStoryMissionRow* Pinned = EclipseStoryLogic::ResolvePinnedMission(Rows, Flags, RegionId))
		{
			OutOffer.RegionId = RegionId;
			OutOffer.TemplateId = Pinned->MissionId;
			OutOffer.ContextLine = Pinned->BriefingText;
			OutOffer.RewardCredits = Pinned->RewardCredits;
			OutOffer.RewardMaterials = Pinned->RewardMaterials;
			OutOffer.RewardIntel = Pinned->RewardIntel;
			return true;
		}
	}

	const FEclipseRegionDefinition* Definition = Graph.Regions.FindByPredicate(
		[RegionId](const FEclipseRegionDefinition& D) { return D.RegionId == RegionId; });
	const FEclipseMissionOfferRow* Offer = Definition != nullptr ? FindOfferForType(Graph, Definition->RegionType) : nullptr;
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
		FEclipseMissionOfferView View;
		if (!ResolveOfferForRegion(*Graph, RegionId, View))
		{
			UE_LOG(LogEclipse, Warning, TEXT("Strategy: region '%s' has no offer row for its type — skipped (GDD 14.3.5)."), *RegionId.ToString());
			continue;
		}
		Views.Add(View);
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

	return ResolveOfferForRegion(*Graph, RegionId, OutOffer);
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

	FEclipseMissionOfferView Offer;
	if (!ResolveOfferForRegion(*Graph, RegionId, Offer))
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
	Payload.TemplateId = Offer.TemplateId;
	Bus->Broadcast(EclipseTags::Event_Strategy_MissionSelected, FInstancedStruct::Make(Payload));
	return true;
}
