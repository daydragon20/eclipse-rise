#include "UI/EclipseBaseHubWidget.h"

#include "Base/EclipsePrepSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Economy/EclipseEconomySubsystem.h"
#include "Squad/EclipseRosterLogic.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "UI/EclipseStrategyMapWidget.h"

namespace
{
	/** Tiny helper: a text row appended to a box (debug-grade UI building block). */
	UTextBlock* AddTextRow(UWidgetTree& Tree, UVerticalBox& Box, const FString& Text)
	{
		UTextBlock* Row = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::FromString(Text));
		Box.AddChildToVerticalBox(Row);
		return Row;
	}

	/** A button row whose payload lives in the reusable offer-button class. */
	UEclipseStrategyOfferButton* AddButtonRow(UWidgetTree& Tree, UVerticalBox& Box, const FString& Label, FName PayloadId, TFunction<void(FName)> OnClicked)
	{
		UEclipseStrategyOfferButton* Button = Tree.ConstructWidget<UEclipseStrategyOfferButton>(UEclipseStrategyOfferButton::StaticClass());
		Button->Bind(PayloadId, MoveTemp(OnClicked));
		UTextBlock* Text = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Button->AddChild(Text);
		Box.AddChildToVerticalBox(Button);
		return Button;
	}
}

void UEclipseBaseHubWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>())
	{
		// One subscription at the Event root: the hub re-renders on any fact and
		// assembles the debrief from commit events as they stream past.
		EventsHandle = Bus->Subscribe(
			FGameplayTag::RequestGameplayTag(TEXT("Event")),
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseBaseHubWidget::OnAnyFact));
	}

	BuildLayout();
	RefreshAll();
}

void UEclipseBaseHubWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			Bus->Unsubscribe(EventsHandle);
		}
	}
	Super::NativeDestruct();
}

void UEclipseBaseHubWidget::BuildLayout()
{
	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HubRoot"));
	WidgetTree->RootWidget = RootBox;

	HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	RootBox->AddChildToVerticalBox(HeaderText);

	// Tab bar.
	UVerticalBox* TabBar = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	RootBox->AddChildToVerticalBox(TabBar);
	const TCHAR* TabNames[] = { TEXT("COMMAND"), TEXT("WORKSHOP"), TEXT("BARRACKS"), TEXT("MEMORIAL") };
	for (int32 TabIndex = 0; TabIndex < 4; ++TabIndex)
	{
		AddButtonRow(*WidgetTree, *TabBar, TabNames[TabIndex], FName(*FString::FromInt(TabIndex)),
			[this](FName Id) { HandleTab(FCString::Atoi(*Id.ToString())); });
	}

	TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
	RootBox->AddChildToVerticalBox(TabSwitcher);

	CommandTab = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TabSwitcher->AddChild(CommandTab);
	WorkshopTab = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TabSwitcher->AddChild(WorkshopTab);
	BarracksTab = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TabSwitcher->AddChild(BarracksTab);
	MemorialTab = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	TabSwitcher->AddChild(MemorialTab);

	// Command tab: advance-day + the district map (SPEC-P1-04 widget hosted here).
	AddButtonRow(*WidgetTree, *CommandTab, TEXT("ADVANCE DAY"), NAME_None, [this](FName) { HandleAdvanceDay(); });
	MapWidget = CreateWidget<UEclipseStrategyMapWidget>(this, UEclipseStrategyMapWidget::StaticClass());
	CommandTab->AddChildToVerticalBox(MapWidget);

	// Preparation panel lives under the command tab (briefing -> launch).
	PrepPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CommandTab->AddChildToVerticalBox(PrepPanel);
}

void UEclipseBaseHubWidget::OnAnyFact(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	// Debrief transparency (GDD 7.6): while a mission resolves, every commit
	// fact becomes a human line with its cause attached.
	if (EventTag == EclipseTags::Event_Mission_Completed.GetTag() || EventTag == EclipseTags::Event_Mission_Failed.GetTag())
	{
		bCollectingDebrief = false;
		DebriefLines.Insert(FString::Printf(TEXT("DEBRIEF — %s"),
			EventTag == EclipseTags::Event_Mission_Completed.GetTag() ? TEXT("MISSION ACCOMPLISHED") : TEXT("MISSION FAILED (we carry it forward)")), 0);
	}
	else if (EventTag == EclipseTags::Event_Mission_Started.GetTag())
	{
		DebriefLines.Reset();
		bCollectingDebrief = true;
	}
	else if (bCollectingDebrief)
	{
		if (const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>())
		{
			if (EventTag == EclipseTags::Event_Economy_ResourcesChanged.GetTag())
			{
				DebriefLines.Add(FString::Printf(TEXT("%+d %s (%s)"), Economy->Delta, *Economy->ResourceType.ToString(), *Economy->Reason.ToString()));
			}
		}
		else if (const FEclipseRosterEventPayload* Roster = Payload.GetPtr<FEclipseRosterEventPayload>())
		{
			if (EventTag == EclipseTags::Event_Roster_SoldierDied.GetTag())
			{
				DebriefLines.Add(FString::Printf(TEXT("KIA — cause: %s"), *Roster->Cause.ToString()));
			}
			else if (EventTag == EclipseTags::Event_Roster_SoldierWounded.GetTag())
			{
				DebriefLines.Add(FString::Printf(TEXT("Wounded — out %d days"), Roster->DaysOut));
			}
		}
		else if (const FEclipseLiberationEventPayload* Liberation = Payload.GetPtr<FEclipseLiberationEventPayload>())
		{
			if (EventTag == EclipseTags::Event_Strategy_LiberationResolved.GetTag() && !Liberation->ContextLine.IsEmpty())
			{
				// De enige regel in het debrief die geen getal is. Alle andere
				// regels melden WAT er veranderde; deze zegt wat het betekent.
				DebriefLines.Add(Liberation->ContextLine.ToString());
			}
		}
		else if (const FEclipseStrategyEventPayload* Strategy = Payload.GetPtr<FEclipseStrategyEventPayload>())
		{
			if (EventTag == EclipseTags::Event_Strategy_RegionControlChanged.GetTag())
			{
				DebriefLines.Add(FString::Printf(TEXT("%s: %s -> %s"), *Strategy->RegionId.ToString(), *Strategy->OldOwner.ToString(), *Strategy->NewOwner.ToString()));
			}
		}
	}

	RefreshAll();
}

void UEclipseBaseHubWidget::RefreshAll()
{
	RefreshHeader();
	RefreshWorkshop();
	RefreshBarracks();
	RefreshMemorial();
	RefreshPrepPanel();
}

void UEclipseBaseHubWidget::RefreshHeader()
{
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr || HeaderText == nullptr)
	{
		return;
	}
	// Playtest finding 13.2 (owner, 2026-07-25): the old header named the place and
	// the wallet but never said WHERE YOU ARE or WHAT TO DO NEXT, so a tester who
	// booted straight into the hub pressed WASD for minutes and concluded the
	// controls were broken. The pawn is parked here by design (SPEC-P1-08) — that
	// design just has to be legible. Mode word first, then the next action; the
	// numbers move to the back where they belong.
	const FEclipseCampaignState& State = Campaign->GetState();
	HeaderText->SetText(FText::FromString(FString::Printf(TEXT("BASE — Hollow Point · day %d · pick a mission below to start (walking is disabled here)  |  C %d  M %d  I %d"),
		State.Day,
		State.GetBalance(EclipseTags::Resource_Credits.GetTag()),
		State.GetBalance(EclipseTags::Resource_Materials.GetTag()),
		State.GetBalance(EclipseTags::Resource_Intel.GetTag()))));
}

void UEclipseBaseHubWidget::RefreshWorkshop()
{
	if (WorkshopTab == nullptr)
	{
		return;
	}
	WorkshopTab->ClearChildren();

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseEconomySubsystem* Economy = GetGameInstance()->GetSubsystem<UEclipseEconomySubsystem>();
	if (Campaign == nullptr || Economy == nullptr)
	{
		return;
	}

	const FEclipseCampaignState& State = Campaign->GetState();
	if (State.ProductionQueue.IsEmpty())
	{
		AddTextRow(*WidgetTree, *WorkshopTab, TEXT("Production slot: idle — choose one order:"));
		for (const FEclipseProductionItemParams& Item : Economy->ResolveTickParams().ProductionItems)
		{
			AddButtonRow(*WidgetTree, *WorkshopTab,
				FString::Printf(TEXT("BUILD %s (%d M, %d C, %d d)"), *Item.ItemId.ToString(), Item.CostMaterials, Item.CostCredits, Item.TimeDays),
				Item.ItemId, [this](FName ItemId) { HandleProduce(ItemId); });
		}
	}
	else
	{
		const FEclipseProductionOrder& Order = State.ProductionQueue[0];
		AddTextRow(*WidgetTree, *WorkshopTab,
			FString::Printf(TEXT("In production: %s (done day %d)"), *Order.ItemId.ToString(), Order.CompletesOnDay));
	}

	for (const FGameplayTag& Unlock : State.UnlockedLoadoutTags)
	{
		AddTextRow(*WidgetTree, *WorkshopTab, FString::Printf(TEXT("  Unlocked: %s"), *Unlock.ToString()));
	}
}

void UEclipseBaseHubWidget::RefreshBarracks()
{
	if (BarracksTab == nullptr)
	{
		return;
	}
	BarracksTab->ClearChildren();

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		return;
	}
	const FEclipseCampaignState& State = Campaign->GetState();

	AddTextRow(*WidgetTree, *BarracksTab, FString::Printf(TEXT("ROSTER — pick your squad (%d picked):"), PickedSquad.Num()));
	for (const FEclipseSoldierRecord& Soldier : State.Roster)
	{
		if (Soldier.Status == EEclipseSoldierStatus::Dead)
		{
			continue; // the dead live on the Memorial tab, not the muster list
		}

		const bool bAvailable = EclipseRosterLogic::IsSoldierAvailableOnDay(Soldier, State.Day);
		const bool bPicked = PickedSquad.Contains(Soldier.SoldierId);
		// Class shows read-only (SPEC-P2-01 decision 2: pre-classed recruits;
		// the Academy assignment flow is Phase 3).
		FString Label = FString::Printf(TEXT("%s %s <%s> [%s]%s"),
			bPicked ? TEXT("[X]") : TEXT("[ ]"),
			*Soldier.Name,
			Soldier.ClassId.IsNone() ? TEXT("Recruit") : *Soldier.ClassId.ToString(),
			*Soldier.TraitId.ToString(),
			bAvailable ? TEXT("") : *FString::Printf(TEXT(" — WOUNDED, back day %d"), Soldier.WoundedUntilDay));

		if (bAvailable)
		{
			const FGuid SoldierId = Soldier.SoldierId;
			AddButtonRow(*WidgetTree, *BarracksTab, Label, Soldier.OriginId,
				[this, SoldierId](FName) { HandleToggleSquadPick(SoldierId); });
		}
		else
		{
			AddTextRow(*WidgetTree, *BarracksTab, Label)->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.55f, 0.2f)));
		}
	}
}

void UEclipseBaseHubWidget::RefreshMemorial()
{
	if (MemorialTab == nullptr)
	{
		return;
	}
	MemorialTab->ClearChildren();

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		return;
	}

	AddTextRow(*WidgetTree, *MemorialTab, TEXT("THE WALL — we remember:"));
	for (const FEclipseMemorialEntry& Entry : Campaign->GetState().Memorial)
	{
		AddTextRow(*WidgetTree, *MemorialTab,
			FString::Printf(TEXT("  %s — %d missions — %s — day %d"),
				*Entry.Name, Entry.MissionsServed, *Entry.Cause.ToString(), Entry.Day));
	}
	if (Campaign->GetState().Memorial.IsEmpty())
	{
		AddTextRow(*WidgetTree, *MemorialTab, TEXT("  (no names yet — keep it that way)"));
	}
}

void UEclipseBaseHubWidget::RefreshPrepPanel()
{
	if (PrepPanel == nullptr)
	{
		return;
	}
	PrepPanel->ClearChildren();

	UEclipsePrepSubsystem* Prep = GetGameInstance()->GetSubsystem<UEclipsePrepSubsystem>();
	if (Prep == nullptr)
	{
		return;
	}

	for (const FString& Line : DebriefLines)
	{
		AddTextRow(*WidgetTree, *PrepPanel, Line);
	}

	if (!LastActionNote.IsEmpty())
	{
		AddTextRow(*WidgetTree, *PrepPanel, FString::Printf(TEXT("!! %s"), *LastActionNote))
			->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.35f, 0.25f)));
	}

	AddTextRow(*WidgetTree, *PrepPanel, TEXT("--- PREPARATION (select a strike on the map first) ---"));
	AddTextRow(*WidgetTree, *PrepPanel, Prep->WasIntelRevealed()
		? TEXT("Briefing: enemy positions revealed.")
		: TEXT("Briefing: fog. Spend Intel to reveal."));

	AddButtonRow(*WidgetTree, *PrepPanel, TEXT("SPEND INTEL — reveal briefing"), NAME_None, [this](FName) { HandleIntelReveal(); });

	for (const FGameplayTag& Loadout : Prep->GetAvailableLoadoutTags())
	{
		const FGameplayTag LoadoutCopy = Loadout;
		AddButtonRow(*WidgetTree, *PrepPanel,
			FString::Printf(TEXT("LAUNCH with %s (squad from Barracks)"), *Loadout.ToString()),
			Loadout.GetTagName(), [this, LoadoutCopy](FName) { HandleLaunch(LoadoutCopy); });
	}
}

void UEclipseBaseHubWidget::HandleTab(int32 TabIndex)
{
	if (TabSwitcher != nullptr)
	{
		TabSwitcher->SetActiveWidgetIndex(TabIndex);
	}
}

bool UEclipseBaseHubWidget::NoteActionResult(bool bSucceeded, const FString& Error, const TCHAR* What)
{
	// EEN plek waar een knopdruk zijn uitkomst afhandelt (26-07).
	//
	// De drie knoppen in dit bestand deden alle drie hetzelfde: loggen, de note
	// zetten of wissen, en verversen. Twee van de drie deden het goed en de derde
	// ('volgende dag') gooide zijn uitkomst helemaal weg — die deed dus stil
	// niets, wat de vervelendste soort defect is: je klikt nog een keer en
	// concludeert dat het spel hangt.
	//
	// Met deze helper KAN een volgende knop het niet meer vergeten, want er is
	// niets meer om te vergeten. Dat is structureel goedkoper dan een test op een
	// widget-laag die een viewport nodig heeft.
	if (bSucceeded)
	{
		LastActionNote.Reset();
	}
	else
	{
		UE_LOG(LogEclipse, Warning, TEXT("BaseHub: %s afgewezen — %s"), What, *Error);
		LastActionNote = Error;
	}
	RefreshAll();
	return bSucceeded;
}

void UEclipseBaseHubWidget::HandleAdvanceDay()
{
	UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		return;
	}
	FEclipseCampaignTransaction Transaction;
	Transaction.Source = TEXT("BaseHub");
	FEclipseCampaignMutation& Advance = Transaction.Mutations.AddDefaulted_GetRef();
	Advance.Type = EEclipseCampaignMutationType::AdvanceDay;
	// De uitkomst LEZEN (26-07). Dit hangt aan een knop die de speler indrukt, en
	// een knop die stil niets doet is de vervelendste soort defect: je klikt nog
	// een keer, en nog een keer, en concludeert dat het spel hangt.
	//
	// Gevonden door te zoeken naar aanroepen die een foutmelding teruggeven waar
	// de aanroeper niets mee doet — zelfde sweep die de debrief bij spelersdood
	// opleverde. Dit waren de enige twee in het hele project.
	FString Error;
	NoteActionResult(Campaign->CommitTransaction(Transaction, Error), Error, TEXT("volgende dag"));
}

void UEclipseBaseHubWidget::HandleProduce(FName ItemId)
{
	UEclipseEconomySubsystem* Economy = GetGameInstance()->GetSubsystem<UEclipseEconomySubsystem>();
	FString Error;
	NoteActionResult(Economy != nullptr && Economy->TryQueueProduction(ItemId, Error), Error, TEXT("produceren"));
}

void UEclipseBaseHubWidget::HandleToggleSquadPick(const FGuid& SoldierId)
{
	if (PickedSquad.Remove(SoldierId) == 0)
	{
		PickedSquad.Add(SoldierId);
	}
	RefreshBarracks();
}

void UEclipseBaseHubWidget::HandleIntelReveal()
{
	UEclipsePrepSubsystem* Prep = GetGameInstance()->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	NoteActionResult(Prep != nullptr && Prep->SpendIntelForReveal(Error), Error, TEXT("intel inzetten"));
}

void UEclipseBaseHubWidget::HandleLaunch(FGameplayTag LoadoutTag)
{
	UEclipsePrepSubsystem* Prep = GetGameInstance()->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (Prep != nullptr && !Prep->LaunchMission(PickedSquad, LoadoutTag, TEXT("Entry_Default"), Error))
	{
		UE_LOG(LogEclipse, Warning, TEXT("Launch rejected: %s"), *Error);
		LastActionNote = Error;
		RefreshAll();
		return;
	}
	LastActionNote.Reset();
	PickedSquad.Reset();
}
