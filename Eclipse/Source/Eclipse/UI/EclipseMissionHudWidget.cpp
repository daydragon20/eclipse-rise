#include "UI/EclipseMissionHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Characters/EclipseCommandModeComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Quests/EclipseMissionLogic.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Quests/EclipseMissionTypes.h"
#include "Squad/EclipseSquadSubsystem.h"

namespace
{
	/**
	 * Master switch of the feel-gauntlet panel. Default OFF: a normal run shows
	 * the Phase 1 HUD it always showed and nothing else (the overlay is a
	 * measuring instrument, not a feature). F2/H stay live either way — a key
	 * press is the tester explicitly asking for a panel.
	 */
	TAutoConsoleVariable<int32> CVarEclipseGauntletOverlay(
		TEXT("Eclipse.Gauntlet.Overlay"),
		0,
		TEXT("1 = show the P2-02 R3 criteria panel on the debug HUD and arm its manual keys (F4-F8). Default off."),
		ECVF_Default);

	/**
	 * In-game test guide (phase0/INGAME_TESTGIDS.md; owner picked variant A —
	 * detect and tick off, never lock a control). Default OFF like every other
	 * debug tier: 1 opens the guide the moment the mission HUD mounts. F3 toggles
	 * it either way, following the F2/H convention that a keypress is the tester
	 * explicitly asking for a panel. Deliberately absent: Eclipse.Guide.Strict —
	 * variant B stays unbuilt until A proves its detection per control.
	 */
	TAutoConsoleVariable<int32> CVarEclipseGuideOverlay(
		TEXT("Eclipse.Guide.Overlay"),
		0,
		TEXT("1 = open the in-game test guide as soon as the mission HUD mounts (F3 toggles it at any time). Default off."),
		ECVF_Default);

	/** In-place refresh cadence, wall clock: a burst of order facts may not turn into a burst of string work. */
	constexpr double GauntletRefreshIntervalSeconds = 0.1;

	const FLinearColor ColourNeutral(0.85f, 0.85f, 0.85f);
	const FLinearColor ColourDim(0.42f, 0.42f, 0.42f);
	const FLinearColor ColourPass(0.45f, 0.90f, 0.50f);
	const FLinearColor ColourOpen(0.70f, 0.55f, 0.20f);
	const FLinearColor ColourFail(0.95f, 0.35f, 0.25f);

	/** Colour of a verdict row by status — the panel says pass/open/fail without the tester reading the words. */
	const FLinearColor& StatusColour(EclipseGauntletOverlay::EEclipseGauntletStatus Status)
	{
		switch (Status)
		{
		case EclipseGauntletOverlay::EEclipseGauntletStatus::Pass: return ColourPass;
		case EclipseGauntletOverlay::EEclipseGauntletStatus::Fail: return ColourFail;
		default:                                                   return ColourOpen;
		}
	}

	/**
	 * Debug text row. Name is deliberately not "AddTextRow": the base hub's own
	 * anonymous-namespace helper carries that name, and in a unity build both
	 * files share one translation unit — a default argument here would make every
	 * three-argument call over there ambiguous.
	 */
	UTextBlock* AddHudTextRow(UWidgetTree& Tree, UVerticalBox& Box, const FString& Text, const FLinearColor& Colour = ColourNeutral)
	{
		UTextBlock* Row = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::FromString(Text));
		Row->SetColorAndOpacity(FSlateColor(Colour));
		Box.AddChildToVerticalBox(Row);
		return Row;
	}
}

void UEclipseMissionHudWidget::NativeConstruct()
{
	using namespace EclipseGauntletOverlay;

	Super::NativeConstruct();

	Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionHudRoot"));
	WidgetTree->RootWidget = Root;

	if (!IsDebugHudAllowed())
	{
		// Shot round: no rows, no subscriptions, no console command — the widget
		// exists but is inert, so nothing can appear in a review still.
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	LiveBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(LiveBox);

	// A mount is a fresh run: the widget is cached by the controller and
	// re-constructed per mission, exactly like the two automatic tallies reset per
	// run (UnregisterAll / Event.Mission.Started). The previous run's block was
	// already logged and archived in NativeDestruct, so nothing is lost — and
	// mixing two runs into one verdict would be worse than losing one.
	CleanPicks = 0;
	MisPicks = 0;
	ComfortAnswer = EEclipseGauntletAnswer::Unanswered;
	ConfidenceAnswer = EEclipseGauntletAnswer::Unanswered;
	PlaytestAnswers.Init(EEclipseGauntletAnswer::Unanswered, PlaytestQuestionCount);

	// Same reasoning for the guide: a new mission is a new walk through the list,
	// and the previous run's summary was archived on teardown.
	GuideProgress.Reset();
	bGuideSummaryEmitted = false;
	bGuideVisible = CVarEclipseGuideOverlay.GetValueOnGameThread() > 0;

	BuildStaticPanels();

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		// Three families, not the whole Event root: this HUD draws mission, squad
		// and command facts. Subscribing wider only bought it a full widget-tree
		// rebuild on every economy/base/story commit.
		for (const TCHAR* Family : { TEXT("Event.Mission"), TEXT("Event.Squad"), TEXT("Event.Command") })
		{
			EventHandles.Add(Bus->Subscribe(
				FGameplayTag::RequestGameplayTag(Family),
				FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseMissionHudWidget::OnAnyFact)));
		}
	}

	// The gauntlet block on demand, so a session where the panel stayed closed can
	// still hand the owner his verdict input (phase0/FEEL_GAUNTLET_P2-02.md).
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Gauntlet.Summary")) == nullptr)
	{
		SummaryCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Gauntlet.Summary"),
			TEXT("Log + archive the R3-VERDICT INPUT block of the running feel gauntlet (SPEC-P2-02 R3)."),
			FConsoleCommandDelegate::CreateWeakLambda(this, [this]() { EmitVerdictSummary(); }),
			ECVF_Default);
	}

	// Active-device highlight is event-driven: Enhanced Input's device subsystem
	// broadcasts when the platform user switches hardware, so the overlay never
	// polls keys per frame.
	if (UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
	{
		DeviceChangedHandle = Devices->OnInputHardwareDeviceChangedNative.AddWeakLambda(this,
			[this](FPlatformUserId, FInputDeviceId) { RefreshDeviceHighlight(); });
	}

	ApplyPanelVisibility();
	RefreshDeviceHighlight();
	RefreshPlaytestRows();
	RefreshGauntletRows(/*bForce*/ true);
	RefreshGuideRows(/*bForce*/ true);
	Rebuild();
}

void UEclipseMissionHudWidget::NativeDestruct()
{
	using namespace EclipseGauntletOverlay;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			for (FEclipseEventSubscriptionHandle& Handle : EventHandles)
			{
				Bus->Unsubscribe(Handle);
			}
		}
	}
	EventHandles.Reset();

	if (DeviceChangedHandle.IsValid())
	{
		if (UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
		{
			Devices->OnInputHardwareDeviceChangedNative.Remove(DeviceChangedHandle);
		}
		DeviceChangedHandle.Reset();
	}

	if (SummaryCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(SummaryCommand);
		SummaryCommand = nullptr;
	}

	// Teardown is "closing the gauntlet" (mission end, level travel, quit): the
	// verdict block must survive it. Nothing measured or answered = nothing to
	// say, and a shot round is not a gauntlet at all. A half-walked test guide is
	// evidence too, so it counts as something to say.
	if (IsDebugHudAllowed()
		&& (ComposeVerdict(GatherCriteria()).OpenCount < CriterionCount || GuideProgress.HasAnyProgress()))
	{
		EmitVerdictSummary();
	}

	Super::NativeDestruct();
}

bool UEclipseMissionHudWidget::IsDebugHudAllowed()
{
	return !FParse::Param(FCommandLine::Get(), TEXT("EclipseShot"));
}

void UEclipseMissionHudWidget::OnAnyFact(FGameplayTag /*EventTag*/, const FInstancedStruct& /*Payload*/)
{
	Rebuild();
	ApplyPanelVisibility();
	RefreshGauntletRows(/*bForce*/ false);

	// The guide's responsiveness row carries the live order measurement, so it
	// follows the same throttled, hidden-is-free path — never a rebuild.
	RefreshGuideRows(/*bForce*/ false);
}

void UEclipseMissionHudWidget::Rebuild()
{
	if (LiveBox == nullptr)
	{
		return;
	}
	LiveBox->ClearChildren();

	auto AddLine = [this](const FString& Text, const FLinearColor& Colour = ColourNeutral)
	{
		return AddHudTextRow(*WidgetTree, *LiveBox, Text, Colour);
	};

	const UGameInstance* GameInstance = GetGameInstance();
	if (const UEclipseMissionSubsystem* Mission = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseMissionSubsystem>() : nullptr)
	{
		const bool bAlarm = Mission->IsAlarmRaised();
		const bool bCasualty = Mission->HasAnyCasualtyThisRun();
		// The mode word and the two controls that matter come FIRST (13.2 finding):
		// a tester who boots into a run should never have to guess whether the game
		// has his input. The base hub says the mirror image of this line.
		AddLine(FString::Printf(TEXT("== MISSION ACTIVE  [%s]%s%s  —  WASD move · Q hold = Command Mode · F2 controls · F3 testgids =="),
			*UEnum::GetValueAsString(Mission->GetPhase()).RightChop(FString(TEXT("EEclipseMissionPhase::")).Len()),
			bAlarm ? TEXT("  ALARM") : TEXT(""),
			bCasualty ? TEXT("  CASUALTY") : TEXT("")));

		const TArray<FName>& Done = Mission->GetCompletedObjectiveIds();

		// Voided optionals through the SAME pure evaluation the debrief pays out
		// with (SPEC-P2-04), so the HUD and the wallet can never disagree — the
		// art review's finding was a completed optional that quietly stayed ticked
		// after the alarm/casualty latch had already killed its payout.
		TArray<FEclipseObjectiveDef> PaidOptionals;
		TArray<FName> MissedOptionals;
		EclipseMissionLogic::EvaluateOptionalObjectives(
			Mission->GetActiveObjectives(), Done, bAlarm, bCasualty, PaidOptionals, MissedOptionals);

		for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
		{
			const bool bDone = Done.Contains(Objective.ObjectiveId);
			const bool bVoidedNow = MissedOptionals.Contains(Objective.ObjectiveId);

			// An optional whose latch already tripped is lost whether it was
			// completed or not; the tester should see that while there is still
			// time to change plans, not first at debrief.
			const bool bConditionLost = Objective.bOptional
				&& ((Objective.bRequiresNoAlarm && bAlarm) || (Objective.bRequiresNoCasualties && bCasualty));

			FString Suffix;
			if (Objective.bOptional)
			{
				const TCHAR* Cause = Objective.bRequiresNoAlarm && bAlarm && Objective.bRequiresNoCasualties && bCasualty
					? TEXT("alarm + casualty")
					: (Objective.bRequiresNoAlarm && bAlarm ? TEXT("alarm") : TEXT("casualty"));
				Suffix = bVoidedNow
					? FString::Printf(TEXT("  (optional — VOID: %s, not paid)"), Cause)
					: bConditionLost
						? FString::Printf(TEXT("  (optional — already lost: %s)"), Cause)
						: FString(TEXT("  (optional)"));
			}

			AddLine(FString::Printf(TEXT("  [%s] %s%s"),
				bDone ? TEXT("X") : TEXT(" "), *Objective.Description.ToString(), *Suffix),
				bVoidedNow || bConditionLost ? ColourOpen : ColourNeutral);
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const UEclipseSquadSubsystem* Squad = World->GetSubsystem<UEclipseSquadSubsystem>())
		{
			AddLine(TEXT("-- squad orders --"));
			for (const FString& Line : Squad->GetOrderStateLines())
			{
				AddLine(FString::Printf(TEXT("  %s"), *Line));
			}
		}
	}

	// Command Mode debug lines (SPEC-P2-02 step 4 — the HUD stays a consumer;
	// the component owns the state). Rebuild is event-driven, so these refresh
	// on ModeEntered/Exited and every order fact.
	if (const APlayerController* Controller = GetOwningPlayer())
	{
		if (const UEclipseCommandModeComponent* Command = Controller->FindComponentByClass<UEclipseCommandModeComponent>())
		{
			for (const FString& Line : Command->GetDebugLines())
			{
				AddLine(Line);
			}
		}
	}
}

void UEclipseMissionHudWidget::BuildStaticPanels()
{
	using namespace EclipseGauntletOverlay;

	// The controller caches this widget across missions, so a second mount runs
	// this again: drop the previous mount's row pointers or the refreshes would
	// write into orphaned widgets that are no longer in the tree.
	GauntletRows.Reset();
	PlaytestRows.Reset();
	GuideRows.Reset();
	MouseKeyboardCells.Reset();
	ControllerCells.Reset();

	// ---- R3 criteria panel: one row per verdict line, never rebuilt ----------
	GauntletPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(GauntletPanel);
	GauntletRows.Reserve(VerdictLineCount);
	for (int32 Line = 0; Line < VerdictLineCount; ++Line)
	{
		GauntletRows.Add(AddHudTextRow(*WidgetTree, *GauntletPanel, FString()));
	}
	AddHudTextRow(*WidgetTree, *GauntletPanel,
		TEXT("F4 schone pick · F5 mis-pick · F6 comfort · F7 vertrouwen · F8 nieuwe beat · F2 besturing · F3 testgids · H playtest"),
		ColourDim);

	// ---- Control overview (F2): label column + the two device columns --------
	ControlsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(ControlsPanel);
	AddHudTextRow(*WidgetTree, *ControlsPanel, TEXT("== BESTURING (F2) =="));

	UHorizontalBox* Table = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ControlsPanel->AddChildToVerticalBox(Table);

	auto AddColumn = [this, Table](const FString& Header, TArray<TObjectPtr<UTextBlock>>* OutCells)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (UHorizontalBoxSlot* Slot = Table->AddChildToHorizontalBox(Column))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
		}
		UTextBlock* HeaderCell = AddHudTextRow(*WidgetTree, *Column, Header);
		if (OutCells != nullptr)
		{
			OutCells->Add(HeaderCell);
		}
		return Column;
	};

	UVerticalBox* ActionColumn = AddColumn(TEXT("ACTIE"), nullptr);
	UVerticalBox* MouseKeyboardColumn = AddColumn(TEXT("MUIS + TOETSENBORD"), &MouseKeyboardCells);
	UVerticalBox* ControllerColumn = AddColumn(TEXT("CONTROLLER"), &ControllerCells);

	for (const FEclipseControlRow& Row : GetControlRows())
	{
		AddHudTextRow(*WidgetTree, *ActionColumn, Row.Action);
		MouseKeyboardCells.Add(AddHudTextRow(*WidgetTree, *MouseKeyboardColumn, Row.MouseKeyboard));
		ControllerCells.Add(AddHudTextRow(*WidgetTree, *ControllerColumn, Row.Controller));
	}

	// ---- 13.2 playtest checklist (H) ----------------------------------------
	PlaytestPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(PlaytestPanel);
	for (int32 Line = 0; Line < PlaytestQuestionCount + 2; ++Line)
	{
		PlaytestRows.Add(AddHudTextRow(*WidgetTree, *PlaytestPanel, FString()));
	}
	AddHudTextRow(*WidgetTree, *PlaytestPanel, TEXT("toggle per regel: 6 7 8 9 0  (onbeantwoord -> goed -> slecht)"), ColourDim);

	// ---- In-game test guide (F3): fixed rows, refreshed in place ------------
	GuidePanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Root->AddChildToVerticalBox(GuidePanel);
	GuideRows.Reserve(EclipseTestGuide::GuidePanelLineCount);
	for (int32 Line = 0; Line < EclipseTestGuide::GuidePanelLineCount; ++Line)
	{
		GuideRows.Add(AddHudTextRow(*WidgetTree, *GuidePanel, FString()));
	}
}

bool UEclipseMissionHudWidget::IsGauntletPanelVisible() const
{
	return CVarEclipseGauntletOverlay.GetValueOnGameThread() > 0;
}

void UEclipseMissionHudWidget::ApplyPanelVisibility()
{
	// SetVisibility invalidates layout, so only ever call it on a real change —
	// this runs on every fact.
	auto Apply = [](UVerticalBox* Panel, bool bVisible)
	{
		if (Panel == nullptr)
		{
			return;
		}
		const ESlateVisibility Desired = bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
		if (Panel->GetVisibility() != Desired)
		{
			Panel->SetVisibility(Desired);
		}
	};

	Apply(GauntletPanel, IsGauntletPanelVisible());
	Apply(ControlsPanel, bControlsVisible);
	Apply(PlaytestPanel, bPlaytestVisible);
	Apply(GuidePanel, IsGuidePanelVisible());
}

void UEclipseMissionHudWidget::RefreshGauntletRows(bool bForce)
{
	using namespace EclipseGauntletOverlay;

	if (GauntletRows.IsEmpty() || !IsGauntletPanelVisible())
	{
		return; // hidden panel = zero work per fact
	}

	const double NowWallSeconds = FPlatformTime::Seconds();
	if (!bForce && (NowWallSeconds - LastGauntletRefreshWallSeconds) < GauntletRefreshIntervalSeconds)
	{
		return; // a four-soldier broadcast order arrives as four facts in one frame
	}
	LastGauntletRefreshWallSeconds = NowWallSeconds;

	const FEclipseGauntletVerdict Verdict = ComposeVerdict(GatherCriteria());
	for (int32 Line = 0; Line < GauntletRows.Num() && Line < Verdict.Lines.Num(); ++Line)
	{
		UTextBlock* Row = GauntletRows[Line];
		if (Row == nullptr)
		{
			continue;
		}
		Row->SetText(FText::FromString(Verdict.Lines[Line]));

		// Line 0 is the title, the last line the tally; in between, one criterion
		// each — the same order ComposeVerdict guarantees.
		const bool bCriterionRow = Line >= 1 && Line <= CriterionCount;
		Row->SetColorAndOpacity(FSlateColor(bCriterionRow
			? StatusColour(Verdict.Statuses[Line - 1])
			: (Line == 0 ? ColourNeutral
				: (Verdict.FailedCount > 0 ? ColourFail : (Verdict.OpenCount > 0 ? ColourOpen : ColourPass)))));
	}
}

void UEclipseMissionHudWidget::RefreshPlaytestRows()
{
	using namespace EclipseGauntletOverlay;

	const TArray<FString> Lines = ComposePlaytestBlock(PlaytestAnswers);
	for (int32 Line = 0; Line < PlaytestRows.Num() && Line < Lines.Num(); ++Line)
	{
		if (PlaytestRows[Line] != nullptr)
		{
			PlaytestRows[Line]->SetText(FText::FromString(Lines[Line]));
		}
	}
}

FString UEclipseMissionHudWidget::DescribeOrderRoundTrip() const
{
	// The same wall-clock meter criterion 1 is judged on (the squad layer owns it);
	// the guide only quotes it, so the two panels can never disagree. Empty means
	// "not measured", never "fine so far".
	const UWorld* World = GetWorld();
	const UEclipseSquadSubsystem* Squad = World != nullptr ? World->GetSubsystem<UEclipseSquadSubsystem>() : nullptr;
	if (Squad == nullptr)
	{
		return FString();
	}

	const EclipseSquadOrderLogic::FEclipseOrderRoundTripStats& Stats = Squad->GetOrderRoundTripStats();
	if (Stats.SampleCount <= 0)
	{
		return FString();
	}
	constexpr double BarSeconds = EclipseSquadOrderLogic::FEclipseOrderRoundTripStats::BarSeconds;
	return FString::Printf(TEXT("%d/%d binnen %.2f s (slechtste %.3f s)"),
		Stats.WithinBarCount, Stats.SampleCount, BarSeconds, Stats.WorstSeconds);
}

void UEclipseMissionHudWidget::RefreshGuideRows(bool bForce)
{
	if (GuideRows.IsEmpty() || !IsGuidePanelVisible())
	{
		return; // hidden guide = zero work per fact
	}

	const double NowWallSeconds = FPlatformTime::Seconds();
	if (!bForce && (NowWallSeconds - LastGuideRefreshWallSeconds) < GauntletRefreshIntervalSeconds)
	{
		return;
	}
	LastGuideRefreshWallSeconds = NowWallSeconds;

	const int32 ActiveStep = GuideProgress.GetActiveIndex();
	const TArray<FString> Lines = EclipseTestGuide::ComposeGuidePanelLines(GuideProgress, DescribeOrderRoundTrip());
	for (int32 Line = 0; Line < GuideRows.Num() && Line < Lines.Num(); ++Line)
	{
		UTextBlock* Row = GuideRows[Line];
		if (Row == nullptr)
		{
			continue;
		}
		Row->SetText(FText::FromString(Lines[Line]));

		// Line 0 is the header, the last line the tally; in between one step each,
		// in the order ComposeGuidePanelLines guarantees. The active step is the
		// only bright row — settled steps dim so the eye lands on what to do next.
		const int32 StepIndex = Line - 1;
		const bool bStepRow = StepIndex >= 0 && StepIndex < EclipseTestGuide::GuideStepCount;
		Row->SetColorAndOpacity(FSlateColor(bStepRow
			? (StepIndex == ActiveStep ? ColourNeutral : ColourDim)
			: ColourNeutral));
	}
}

void UEclipseMissionHudWidget::ToggleGuidePanel()
{
	bGuideVisible = !bGuideVisible;
	ApplyPanelVisibility();
	RefreshGuideRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::ConfirmGuideStep()
{
	if (!IsGuidePanelVisible() || !GuideProgress.ConfirmActive())
	{
		return; // a closed guide steals no key; a finished guide has nothing to settle
	}
	OnGuideStepSettled();
}

void UEclipseMissionHudWidget::SkipGuideStep()
{
	if (!IsGuidePanelVisible() || !GuideProgress.RejectActive())
	{
		return;
	}
	OnGuideStepSettled();
}

void UEclipseMissionHudWidget::NoteGuideSignal(EclipseTestGuide::EEclipseGuideSignal Signal)
{
	// Cheapest possible early-out: this runs from the controller's extra Started
	// bindings, i.e. only when the player actually presses something, and with the
	// guide closed it costs one bool test. No tick, no key polling, no consumption.
	if (!IsGuidePanelVisible() || !GuideProgress.NoteSignal(Signal))
	{
		return;
	}
	OnGuideStepSettled();
}

void UEclipseMissionHudWidget::OnGuideStepSettled()
{
	using namespace EclipseGauntletOverlay;

	// Deel 3 IS the 13.2 checklist, so answering it in the guide answers it in the
	// playtest block: one set of books on the owner's gate question.
	const TArray<EclipseTestGuide::EEclipseGuideStepState>& States = GuideProgress.GetStates();
	for (int32 Step = 0; Step < States.Num(); ++Step)
	{
		const int32 Question = EclipseTestGuide::GetPlaytestQuestionIndex(Step);
		if (Question == INDEX_NONE || !PlaytestAnswers.IsValidIndex(Question))
		{
			continue;
		}
		switch (States[Step])
		{
		case EclipseTestGuide::EEclipseGuideStepState::Confirmed:
			PlaytestAnswers[Question] = EEclipseGauntletAnswer::Good;
			break;
		case EclipseTestGuide::EEclipseGuideStepState::Rejected:
			PlaytestAnswers[Question] = EEclipseGauntletAnswer::Bad;
			break;
		default:
			break; // Pending/Skipped leave the row unanswered — never a fabricated verdict
		}
	}
	RefreshPlaytestRows();
	RefreshGuideRows(/*bForce*/ true);

	// Finishing the list archives it through the gauntlet's existing summary path
	// — one mechanism, one file. Once per mount: further keys have nothing to add.
	if (GuideProgress.IsComplete() && !bGuideSummaryEmitted)
	{
		bGuideSummaryEmitted = true;
		EmitVerdictSummary();
	}
}

void UEclipseMissionHudWidget::RefreshDeviceHighlight()
{
	// UInputDeviceSubsystem::GetMostRecentlyUsedHardwareDevice is Enhanced Input's
	// own answer to "what is the player holding right now" (Engine/Source/Runtime/
	// Engine/Classes/GameFramework/InputDeviceSubsystem.h); its
	// OnInputHardwareDeviceChangedNative delegate is what re-runs this function.
	// Missing subsystem (commandlet/dedicated server) = keyboard column marked.
	bool bGamepadActive = false;
	const APlayerController* Controller = GetOwningPlayer();
	if (const UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
	{
		if (Controller != nullptr)
		{
			const FHardwareDeviceIdentifier Hardware = Devices->GetMostRecentlyUsedHardwareDevice(Controller->GetPlatformUserId());
			bGamepadActive = Hardware.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad;
		}
	}

	auto ApplyColumn = [](TArray<TObjectPtr<UTextBlock>>& Cells, bool bActive, const FString& HeaderText)
	{
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			if (Cells[Index] == nullptr)
			{
				continue;
			}
			Cells[Index]->SetColorAndOpacity(FSlateColor(bActive ? ColourNeutral : ColourDim));
			if (Index == 0)
			{
				Cells[Index]->SetText(FText::FromString(bActive
					? FString::Printf(TEXT(">> %s <<"), *HeaderText)
					: HeaderText));
			}
		}
	};

	ApplyColumn(MouseKeyboardCells, !bGamepadActive, TEXT("MUIS + TOETSENBORD"));
	ApplyColumn(ControllerCells, bGamepadActive, TEXT("CONTROLLER"));
}

void UEclipseMissionHudWidget::ToggleControlsPanel()
{
	bControlsVisible = !bControlsVisible;
	ApplyPanelVisibility();
	if (bControlsVisible)
	{
		RefreshDeviceHighlight();
	}
}

void UEclipseMissionHudWidget::TogglePlaytestPanel()
{
	bPlaytestVisible = !bPlaytestVisible;
	ApplyPanelVisibility();
	if (bPlaytestVisible)
	{
		RefreshPlaytestRows();
	}
}

void UEclipseMissionHudWidget::NoteTargetingPick(bool bCleanPick)
{
	if (!IsGauntletPanelVisible())
	{
		return; // a closed panel steals no input
	}
	bCleanPick ? ++CleanPicks : ++MisPicks;
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::CycleComfortAnswer()
{
	using namespace EclipseGauntletOverlay;

	if (!IsGauntletPanelVisible())
	{
		return;
	}
	ComfortAnswer = CycleAnswer(ComfortAnswer);
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::CycleConfidenceAnswer()
{
	using namespace EclipseGauntletOverlay;

	if (!IsGauntletPanelVisible())
	{
		return;
	}
	ConfidenceAnswer = CycleAnswer(ConfidenceAnswer);
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::MarkEncounterBeat()
{
	if (!IsGauntletPanelVisible())
	{
		return;
	}

	// The tally lives in the component (it owns ModeEntered); the overlay only
	// tells it a beat ended — including an empty one, which is the measurement
	// that fails criterion 5.
	APlayerController* Controller = GetOwningPlayer();
	if (UEclipseCommandModeComponent* Command = Controller != nullptr ? Controller->FindComponentByClass<UEclipseCommandModeComponent>() : nullptr)
	{
		Command->BeginNextEncounterBeat(/*bCountEmptyBeat*/ true);
	}
	RefreshGauntletRows(/*bForce*/ true);
}

void UEclipseMissionHudWidget::CyclePlaytestAnswer(int32 QuestionIndex)
{
	using namespace EclipseGauntletOverlay;

	if (!bPlaytestVisible || !PlaytestAnswers.IsValidIndex(QuestionIndex))
	{
		return;
	}
	PlaytestAnswers[QuestionIndex] = CycleAnswer(PlaytestAnswers[QuestionIndex]);
	RefreshPlaytestRows();
}

EclipseGauntletOverlay::FEclipseGauntletCriteria UEclipseMissionHudWidget::GatherCriteria() const
{
	using namespace EclipseGauntletOverlay;

	FEclipseGauntletCriteria Criteria;

	// Criterion 1 from the squad layer: it measured the round trips, wall clock.
	if (const UWorld* World = GetWorld())
	{
		if (const UEclipseSquadSubsystem* Squad = World->GetSubsystem<UEclipseSquadSubsystem>())
		{
			const EclipseSquadOrderLogic::FEclipseOrderRoundTripStats& Stats = Squad->GetOrderRoundTripStats();
			Criteria.RoundTripSamples = Stats.SampleCount;
			Criteria.RoundTripWithinBar = Stats.WithinBarCount;
			Criteria.RoundTripWorstSeconds = Stats.WorstSeconds;
			Criteria.RoundTripBarSeconds = EclipseSquadOrderLogic::FEclipseOrderRoundTripStats::BarSeconds;
		}
	}

	// Criterion 5 from the Command Mode component: it owns the entries.
	if (const APlayerController* Controller = GetOwningPlayer())
	{
		if (const UEclipseCommandModeComponent* Command = Controller->FindComponentByClass<UEclipseCommandModeComponent>())
		{
			Criteria.EntriesThisBeat = Command->GetCommandEntriesThisBeat();
			Criteria.ClosedBeatCount = Command->GetObservedEncounterBeatCount();
			Criteria.AverageEntriesPerBeat = Command->GetAverageEntriesPerBeat();
		}
	}

	// Criteria 2-4: the tester's own answers, the only state this widget owns.
	Criteria.CleanPicks = CleanPicks;
	Criteria.MisPicks = MisPicks;
	Criteria.Comfort = ComfortAnswer;
	Criteria.Confidence = ConfidenceAnswer;
	return Criteria;
}

void UEclipseMissionHudWidget::EmitVerdictSummary()
{
	using namespace EclipseGauntletOverlay;

	const FEclipseGauntletVerdict Verdict = ComposeVerdict(GatherCriteria());

	TArray<FString> Block = Verdict.Lines;
	Block.Add(FString());
	Block.Append(ComposePlaytestBlock(PlaytestAnswers));

	// The test guide rides along in the same block and the same file (spec §4: the
	// guide is a mode of this overlay, not a second archive). A guide nobody walked
	// adds nothing — an empty section would only dilute the verdict above it.
	if (GuideProgress.HasAnyProgress())
	{
		Block.Add(FString());
		Block.Append(EclipseTestGuide::ComposeGuideSummaryBlock(GuideProgress));
	}

	for (const FString& Line : Block)
	{
		UE_LOG(LogEclipse, Display, TEXT("[R3] %s"), *Line);
	}

	// Saved/Logs keeps the block even when the session's console scroll is gone
	// (owner requirement: the verdict input may not be lost on shutdown).
	const FString Path = FPaths::ProjectLogDir() /
		FString::Printf(TEXT("EclipseGauntletR3_%s.txt"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	const FString Body = FString::Join(Block, TEXT("\r\n")) + TEXT("\r\n");
	if (!FFileHelper::SaveStringToFile(Body, *Path))
	{
		// A missing archive must not be a crash and must not be silent (14.3.5).
		UE_LOG(LogEclipse, Warning, TEXT("Gauntlet: could not write the R3 block to %s — the log above is the record."), *Path);
		return;
	}
	UE_LOG(LogEclipse, Display, TEXT("Gauntlet: R3 block archived at %s"), *Path);
}
