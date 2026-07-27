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
#include "Strategy/EclipseLiberationLogic.h"
#include "Strategy/EclipseStrategyLogic.h"
#include "StructUtils/InstancedStruct.h"

void UEclipseStrategySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEclipseEventBusSubsystem* Bus = Collection.InitializeDependency<UEclipseEventBusSubsystem>();
	Collection.InitializeDependency<UEclipseCampaignSubsystem>();

	if (Bus != nullptr)
	{
		// The liberation trigger (SPEC-P2-05 locked decision 2): the stable
		// mission id on the P2-04 seam — Completed fires exactly once per
		// completion, success only; Failed is a different tag. The typed
		// subscription lets the bus type-check the payload at the seam.
		MissionCompletedHandle = Bus->Subscribe(
			EclipseTags::Event_Mission_Completed,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseStrategySubsystem::OnMissionCompleted),
			FEclipseMissionEventPayload::StaticStruct());
	}

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

	// The missing link in the test route (phase0/TESTROUTE_OBJECTIVES.md): SelectMission
	// existed in C++ but had no console surface, so a fast run still needed a click on
	// the map screen before Eclipse.Prep.AutoLaunch could do anything. Same shape as
	// FlipRegion above — no new system, just the existing function exposed in the
	// debug layer (GDD 14.5). Pairs with -EclipseStartMission for a boot-time start.
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Strategy.SelectMission")) == nullptr)
	{
		SelectMissionCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Strategy.SelectMission"),
			TEXT("Usage: Eclipse.Strategy.SelectMission <RegionId> — pick that region's offer (then Eclipse.Prep.AutoLaunch to run it)."),
			FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
			{
				if (Args.Num() != 1)
				{
					UE_LOG(LogEclipse, Error, TEXT("Usage: Eclipse.Strategy.SelectMission <RegionId>"));
					return;
				}
				FString Error;
				if (!SelectMission(FName(*Args[0]), Error))
				{
					UE_LOG(LogEclipse, Error, TEXT("SelectMission '%s': %s"), *Args[0], *Error);
				}
			}),
			ECVF_Default);
	}

	// DE LIBERATIE-DUMP (SPEC-P2-05 bouwvolgorde stap 4).
	//
	// Dit commando bestond nog niet, en het had de bug van 27-07 in een seconde
	// zichtbaar gemaakt: de Foothold-rij stond klaar, de logica was tien keer
	// unit-getest, en DA_CampaignSetup.LiberationInstances was simpelweg NIET
	// GEKOPPELD. Op de kaart was daar niets van te zien - er draaide gewoon nooit
	// een regio om, precies zoals een spel waarin die missie er nog niet is.
	//
	// Vandaar dat de eerste regel hieronder de KOPPELING is en niet de inhoud. Een
	// rapport dat met "0 rijen" begint laat in het midden of de tabel leeg is of
	// niet gevonden, en dat zijn twee verschillende reparaties.
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Liberation.Report")) == nullptr)
	{
		LiberationReportCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Liberation.Report"),
			TEXT("Dump de liberation-staat: tabel gekoppeld, rijen, welke rij al gevuurd is, en wat er nog zou omdraaien."),
			FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
			{
				const UEclipseCampaignSubsystem* Campaign = GetGameInstance() != nullptr
					? GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
				if (Campaign == nullptr)
				{
					UE_LOG(LogEclipse, Warning, TEXT("Liberatie: geen campagne actief."));
					return;
				}

				const UEclipseCampaignSetupAsset* Setup = Campaign->GetActiveSetup();
				const bool bLinked = Setup != nullptr && !Setup->LiberationInstances.IsNull();
				UE_LOG(LogEclipse, Display, TEXT("Liberatie: tabel %s"),
					bLinked ? *Setup->LiberationInstances.ToString() : TEXT("NIET GEKOPPELD aan DA_CampaignSetup"));
				if (!bLinked)
				{
					UE_LOG(LogEclipse, Display,
						TEXT("Liberatie: er draait dus NOOIT een regio om. Draai Tools/setup_liberation_data.py."));
					return;
				}

				const UDataTable* Table = GetValidatedLiberationTable(*Campaign);
				if (Table == nullptr)
				{
					UE_LOG(LogEclipse, Warning, TEXT("Liberatie: tabel gekoppeld maar niet bruikbaar (verkeerde rijstruct?)."));
					return;
				}

				int32 Rows = 0;
				Table->ForeachRow<FEclipseLiberationRow>(TEXT("LiberationReport"),
					[this, &Rows, &Campaign](const FName& RowName, const FEclipseLiberationRow& Row)
					{
						++Rows;
						// Al gevuurd of niet: dat is de vraag die je stelt als er
						// iets NIET omdraaide, dus die staat vooraan.
						const bool bComplete = EclipseLiberationLogic::IsLiberationComplete(Campaign->GetState(), Row);
						UE_LOG(LogEclipse, Display,
							TEXT("Liberatie: rij '%s' trigger=%s beat=%s -> %d regio's, staat: %s"),
							*RowName.ToString(),
							*Row.TriggerMissionId.ToString(),
							Row.RequiredBeatTag.IsValid() ? *Row.RequiredBeatTag.ToString() : TEXT("(ongepoort)"),
							Row.RegionIds.Num(),
							bComplete ? TEXT("AL GEVUURD") : TEXT("wacht"));

						// Per regio de huidige eigenaar. Zonder die regels weet je
						// wel DAT hij nog wacht, niet wat er straks verandert.
						for (const FName& RegionId : Row.RegionIds)
						{
							const FEclipseRegionState* State = Campaign->GetState().FindRegion(RegionId);
							UE_LOG(LogEclipse, Display, TEXT("Liberatie:   %s is nu %s"),
								*RegionId.ToString(),
								State != nullptr ? *UEnum::GetValueAsString(State->Owner) : TEXT("(BESTAAT NIET in de graaf)"));
						}
					});
				UE_LOG(LogEclipse, Display, TEXT("Liberatie: %d rij(en) in de tabel."), Rows);
			}),
			ECVF_Default);
	}
#endif
}

void UEclipseStrategySubsystem::Deinitialize()
{
	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Unsubscribe(MissionCompletedHandle);
	}

#if !UE_BUILD_SHIPPING
	if (FlipRegionCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(FlipRegionCommand);
		FlipRegionCommand = nullptr;
	}
	if (SelectMissionCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(SelectMissionCommand);
		SelectMissionCommand = nullptr;
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

const UDataTable* UEclipseStrategySubsystem::GetValidatedLiberationTable(const UEclipseCampaignSubsystem& Campaign) const
{
	const UEclipseCampaignSetupAsset* Setup = Campaign.GetActiveSetup();
	const UDataTable* Table = Setup != nullptr ? Setup->LiberationInstances.LoadSynchronous() : nullptr;
	if (Table == nullptr)
	{
		// Missing table = no liberations — the mission still completes, the
		// campaign still runs, and the soak catches the missing income band.
		// Warned once per lifetime, never a crash (GDD 14.3.5; the SPEC-P2-05
		// data-schema slot contract "missing = warning + no liberations").
		if (!bWarnedMissingLiberationTable)
		{
			bWarnedMissingLiberationTable = true;
			UE_LOG(LogEclipse, Warning, TEXT("Strategy: campaign setup has no LiberationInstances table — authored completions never flip regions (GDD 14.3.5; run Tools/setup_liberation_data.py)."));
		}
		return nullptr;
	}

	const UScriptStruct* RowStruct = Table->GetRowStruct();
	const bool bUsable = RowStruct != nullptr && RowStruct->IsChildOf(FEclipseLiberationRow::StaticStruct());
	if (!ValidatedLiberationTables.Contains(FObjectKey(Table)))
	{
		ValidatedLiberationTables.Add(FObjectKey(Table));
		if (!bUsable)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Strategy: LiberationInstances table '%s' has the wrong row struct — liberations disabled (GDD 14.3.5; row-content health lives in the ValidateData commandlet)."), *Table->GetName());
		}
	}
	return bUsable ? Table : nullptr;
}

void UEclipseStrategySubsystem::OnMissionCompleted(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseMissionEventPayload* Mission = Payload.GetPtr<FEclipseMissionEventPayload>();
	if (Mission == nullptr || Mission->MissionId.IsNone())
	{
		return; // an id-less completion cannot match a trigger — damaged data is not a wildcard
	}

	UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		return;
	}

	const UDataTable* Liberations = GetValidatedLiberationTable(*Campaign);
	if (Liberations == nullptr)
	{
		return; // warned inside the getter (GDD 14.3.5)
	}

	// Re-entrancy discipline (spec F1): idempotence is state-derived, never a
	// flag — a nested Mission.Completed arriving mid-commit (our commit
	// broadcasts RegionControlChanged from inside this dispatch) re-resolves
	// against the already-applied state, finds an empty remainder, and commits
	// nothing. No guard variable exists to get stuck.
	bool bAnyRowTriggered = false;
	Liberations->ForeachRow<FEclipseLiberationRow>(TEXT("LiberationTrigger"),
		[Campaign, Mission, &bAnyRowTriggered](const FName& RowName, const FEclipseLiberationRow& Row)
		{
			if (!EclipseLiberationLogic::IsLiberationTriggered(Campaign->GetState(), Row, Mission->MissionId))
			{
				return;
			}
			bAnyRowTriggered = true;

			TArray<FName> DroppedRegionIds;
			const FEclipseCampaignTransaction Transaction = EclipseLiberationLogic::ResolveLiberationTransaction(Campaign->GetState(), Row, DroppedRegionIds);

			if (!DroppedRegionIds.IsEmpty())
			{
				// One warning per resolution, all ids in it (the header-doc
				// obligation): a typo drops, the remaining regions still flip
				// (GDD 14.3.5 — the atomic commit would reject the whole set).
				UE_LOG(LogEclipse, Warning, TEXT("Strategy: liberation row '%s' dropped %d region id(s) [%s] — unknown or duplicate; the rest still flips (GDD 14.3.5)."),
					*RowName.ToString(), DroppedRegionIds.Num(), *FString::JoinBy(DroppedRegionIds, TEXT(", "), [](const FName& Id) { return Id.ToString(); }));
			}

			if (Transaction.Mutations.IsEmpty())
			{
				// Nothing to commit — never propose an empty transaction
				// (CommitTransaction rejects it). State-derived idempotence,
				// one log line (SPEC-P2-05 locked decision 3): a re-entry or a
				// player who pre-flipped the whole set is legitimate, not damage.
				// The reason matters and the two are not the same fact: an empty
				// resolution is "already at target" ONLY when nothing was dropped.
				// A row whose every id is a typo also resolves empty, and calling
				// that "already at its target owner" would report a broken row as
				// a healthy no-op (review finding, GDD 14.3.5 honesty).
				const bool bAllDropped = DroppedRegionIds.Num() >= Row.RegionIds.Num() && !Row.RegionIds.IsEmpty();
				UE_LOG(LogEclipse, Display, TEXT("Strategy: liberation '%s' has nothing to commit — %s."),
					*RowName.ToString(),
					bAllDropped
						? TEXT("every region id in the row was dropped (see the warning above), so there is nothing left to flip")
						: (DroppedRegionIds.IsEmpty()
							? TEXT("every region is already at its target owner")
							: TEXT("the surviving region ids are already at their target owner")));
				return;
			}

			FString Error;
			if (!Campaign->CommitTransaction(Transaction, Error))
			{
				// Resolution passes ValidateMutation by construction, so a
				// rejection here is a machinery defect — surfaced hard, never
				// thrown (GDD 12.2 rule 4).
				UE_LOG(LogEclipse, Error, TEXT("Strategy: liberation '%s' transaction rejected: %s"), *RowName.ToString(), *Error);
				return;
			}

			// The commit above already broadcast the RegionControlChanged facts
			// in row order; this line stays as the audit trail in the log.
			UE_LOG(LogEclipse, Display, TEXT("Strategy: liberation '%s' flipped %d region(s) on '%s'. %s"),
				*RowName.ToString(), Transaction.Mutations.Num(), *Mission->MissionId.ToString(), *Row.ContextLine.ToString());

			// En NU pas het feit voor het debriefscherm (SPEC-P2-05 stap 4). Na de
			// commit en dus na de vak-feiten: de zin verklaart iets dat gebeurd is,
			// en een uitleg vooraf zou een uitkomst aankondigen die nog kon worden
			// afgewezen. Op het scherm lees je daardoor eerst WAT er kantelde en
			// dan WAAROM; dat is de eerlijke volgorde, niet de mooiste.
			if (UEclipseEventBusSubsystem* Bus = Campaign->GetGameInstance() != nullptr
				? Campaign->GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
			{
				FEclipseLiberationEventPayload Fact;
				Fact.RowName = RowName;
				Fact.RegionCount = Transaction.Mutations.Num();
				Fact.ContextLine = Row.ContextLine;
				Bus->Broadcast(EclipseTags::Event_Strategy_LiberationResolved.GetTag(),
					FInstancedStruct::Make(Fact));
			}

		});

	if (!bAnyRowTriggered)
	{
		// No row matched this mission (or its beat gate is unset — a forgotten
		// debrief beat would strand the liberation). The common case for
		// generic/regular missions, so the warning fires once per mission id,
		// not per completion (GDD 14.3.5: loud, bounded, never a throw).
		bool bAlreadyWarned = false;
		WarnedUntriggeredLiberationMissionIds.Add(Mission->MissionId, &bAlreadyWarned);
		if (!bAlreadyWarned)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Strategy: no liberation row triggered for completed mission '%s' — no flip (GDD 14.3.5)."), *Mission->MissionId.ToString());
		}
	}
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
