#include "Characters/EclipsePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipseCommandModeComponent.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "UI/EclipseBaseHubWidget.h"
#include "UI/EclipseMissionHudWidget.h"
#include "UI/EclipseTestGuideLogic.h"

namespace
{
	/**
	 * Invert-Y is taste, not a defect, and the owner asked for it to stop being a
	 * discussion: -1 follows DA_CharacterTuning, 0 forces normal, 1 forces
	 * inverted. A console variable rather than a setting because there is no
	 * options menu yet (SPEC-P2-07 owns UI) and this must not wait for one.
	 */
	TAutoConsoleVariable<int32> CVarEclipseInvertLookY(
		TEXT("Eclipse.Look.InvertY"),
		-1,
		TEXT("Y-as van het kijken: -1 = volg DA_CharacterTuning, 0 = normaal, 1 = omgekeerd."),
		ECVF_Default);
}

AEclipsePlayerController::AEclipsePlayerController()
{
	CommandMode = CreateDefaultSubobject<UEclipseCommandModeComponent>(TEXT("CommandMode"));
}

void AEclipsePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(MappingContext, /*Priority*/ 0);
	}

	EnsureCampaignStarted();
	// After EnsureCampaignStarted, because the tuning asset hangs off the active
	// campaign setup and does not exist before there is one.
	ApplyLookTuning();

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		MissionEventsHandle = Bus->Subscribe(
			FGameplayTag::RequestGameplayTag(TEXT("Event.Mission")),
			FEclipseEventNativeDelegate::CreateUObject(this, &AEclipsePlayerController::OnMissionEvent));
	}

	// The loop starts at the menu base (SPEC-P1-08); launching drops into the mission.
	EnterBaseMode();
}

void AEclipsePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Unsubscribe(MissionEventsHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AEclipsePlayerController::EnsureCampaignStarted()
{
	UEclipseCampaignSubsystem* Campaign = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
	if (Campaign == nullptr || Campaign->GetActiveSetup() != nullptr)
	{
		return; // already running (or no subsystem) — never wipe an active campaign
	}

	const UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (Setup == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Boot: DA_CampaignSetup not found — starting an empty campaign (GDD 14.3.5)."));
	}
	Campaign->StartNewCampaign(Setup);
}

void AEclipsePlayerController::EnterBaseMode()
{
	if (BaseHub == nullptr)
	{
		BaseHub = CreateWidget<UEclipseBaseHubWidget>(this, UEclipseBaseHubWidget::StaticClass());
	}
	if (MissionHud != nullptr)
	{
		MissionHud->RemoveFromParent();
	}
	if (BaseHub != nullptr && !BaseHub->IsInViewport())
	{
		BaseHub->AddToViewport(10);
	}

	bShowMouseCursor = true;
	// Playtest finding 13.2 (owner, 2026-07-25): the tester spent minutes pressing
	// WASD in the hub, convinced the controls were broken, because nothing on screen
	// said the pawn was parked and input was UI-only. The code was correct — it was
	// unreadable. The cursor SHAPE is now the state indicator (Supreme Commander
	// style): a hand means "you are planning, click an offer"; a crosshair means
	// "you are in the field". Engine cursor types, so no art dependency, and it costs
	// nothing. The base-hub header line says the same thing in words.
	CurrentMouseCursor = EMouseCursor::Hand;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->DisableInput(this); // parked while the tester plans at the base
	}
}

void AEclipsePlayerController::EnterMissionMode()
{
	if (BaseHub != nullptr)
	{
		BaseHub->RemoveFromParent();
	}
	// A -EclipseShot review round gets NO debug HUD at all (15.8/15.9): the whole
	// widget is skipped here, not just hidden, so no fact can put text in a still.
	if (MissionHud == nullptr && UEclipseMissionHudWidget::IsDebugHudAllowed())
	{
		MissionHud = CreateWidget<UEclipseMissionHudWidget>(this, UEclipseMissionHudWidget::StaticClass());
	}
	if (MissionHud != nullptr && !MissionHud->IsInViewport())
	{
		MissionHud->AddToViewport(5);
	}
	// Say out loud whether the debug HUD actually mounted (GDD 14.3.5). Without
	// this line the owner's "F3 does nothing" was unanswerable from a log: every
	// debug key silently no-ops when the widget is absent, so a missing HUD and a
	// broken key look identical from the outside.
	UE_LOG(LogEclipse, Display, TEXT("Mission mode: debug HUD %s (F2 controls, F3 test guide, H playtest)."),
		MissionHud == nullptr
			? (UEclipseMissionHudWidget::IsDebugHudAllowed()
				? TEXT("NOT CREATED — widget construction failed")
				: TEXT("suppressed by -EclipseShot, as designed"))
			: (MissionHud->IsInViewport() ? TEXT("mounted") : TEXT("created but NOT in the viewport")));

	// Mission mode: no cursor at all — the absence IS the signal that the pawn has
	// your input now (the other half of the 13.2 readability fix above).
	bShowMouseCursor = false;
	CurrentMouseCursor = EMouseCursor::Crosshairs;
	FInputModeGameOnly Mode;
	SetInputMode(Mode);

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->EnableInput(this);

		// Insert at the main entry (SPEC-P1-05: preparation picks an entry; Phase 1
		// uses Entry_Main until the insertion-choice UI lands).
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(TEXT("Entry_Main")))
			{
				ControlledPawn->SetActorLocation(It->GetActorLocation() + FVector(0, 0, 100.0f));
				break;
			}
		}
	}
}

void AEclipsePlayerController::OnMissionEvent(FGameplayTag EventTag, const FInstancedStruct& /*Payload*/)
{
	if (EventTag == EclipseTags::Event_Mission_Started)
	{
		EnterMissionMode();
	}
	else if (EventTag == EclipseTags::Event_Mission_Completed || EventTag == EclipseTags::Event_Mission_Failed)
	{
		EnterBaseMode();
	}
}

void AEclipsePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	MappingContext = NewObject<UInputMappingContext>(this);

	auto MakeAction = [this](EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		Action->ValueType = ValueType;
		return Action;
	};

	MoveAction = MakeAction(EInputActionValueType::Axis2D);
	LookAction = MakeAction(EInputActionValueType::Axis2D);
	FireAction = MakeAction(EInputActionValueType::Boolean);
	SprintAction = MakeAction(EInputActionValueType::Boolean);
	CrouchAction = MakeAction(EInputActionValueType::Boolean);
	ToggleViewAction = MakeAction(EInputActionValueType::Boolean);

	// WASD with swizzle/negate modifiers (standard EI idiom, in code for now).
	auto MapKey = [this](UInputAction* Action, FKey Key, bool bNegate = false, bool bSwizzle = false)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(Action, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		}
	};

	MapKey(MoveAction, EKeys::W, /*negate*/ false, /*swizzle*/ true);
	MapKey(MoveAction, EKeys::S, /*negate*/ true, /*swizzle*/ true);
	MapKey(MoveAction, EKeys::D);
	MapKey(MoveAction, EKeys::A, /*negate*/ true);
	MapKey(LookAction, EKeys::Mouse2D);
	MapKey(FireAction, EKeys::LeftMouseButton);
	MapKey(SprintAction, EKeys::LeftShift);
	MapKey(CrouchAction, EKeys::LeftControl);
	// C for the view swap: the owner's call, and it is the Battlefield/Bethesda
	// convention rather than GTA's V. Verified free — C appears in no other
	// mapping in this module.
	MapKey(ToggleViewAction, EKeys::C);

	// Gamepad (owner request 2026-07-22): the same actions, Xbox-layout keys, so
	// pad and mouse/keyboard coexist in one context — no mode switch, last device
	// wins (EnhancedInput default). Left2D matches HandleMove's axes directly
	// (X = right, Y = forward), so no swizzle is needed on the stick.
	MapKey(MoveAction, EKeys::Gamepad_Left2D);
	// Stick look carries no SCALAR here on purpose: it used to have a (2.0, 1.5)
	// modifier with a PLACEHOLDER(GDD 8.1) note that the feel pass would give look
	// its own curve and deltatime scaling — that is what HandleLook now does, in
	// degrees per second with a deadzone and an exponential response. Leaving the
	// scalar in would silently double-scale it.
	// It DOES need a Y negate, and only on this mapping (owner playtest
	// 2026-07-25): UE reports mouse Y and stick Y with opposite signs, so a single
	// handler cannot be right for both. Negating here keeps the mouse correct and
	// fixes the stick, instead of flipping the handler and breaking the mouse.
	{
		FEnhancedActionKeyMapping& StickLook = MappingContext->MapKey(LookAction, EKeys::Gamepad_Right2D);
		UInputModifierScalar* StickYFlip = NewObject<UInputModifierScalar>(this);
		StickYFlip->Scalar = FVector(1.0, -1.0, 1.0);
		StickLook.Modifiers.Add(StickYFlip);
	}
	MapKey(FireAction, EKeys::Gamepad_RightTrigger);
	MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);
	// R3 was free (L3 carries sprint), so the pad gets the owner's first choice
	// and no D-pad fallback is needed.
	MapKey(ToggleViewAction, EKeys::Gamepad_RightThumbstick);

	const FKey OrderKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	// D-pad mirrors the 1-4 order keys in reading order: up, right, down, left.
	const FKey OrderPadKeys[] = { EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Left };
	OrderActions.SetNum(4);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		OrderActions[Index] = MakeAction(EInputActionValueType::Boolean);
		MapKey(OrderActions[Index], OrderKeys[Index]);
		MapKey(OrderActions[Index], OrderPadKeys[Index]);
	}

	// Command Mode (SPEC-P2-02 Stage A, provisional debug bindings — documented
	// on the HUD): hold Q / pad LB. LB was the pad stance-modifier; stance moves
	// inside the held mode (Y toggles), KB keeps LeftAlt at issue time.
	CommandHoldAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(CommandHoldAction, EKeys::Q);
	MapKey(CommandHoldAction, EKeys::Gamepad_LeftShoulder);
	SelectNextAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(SelectNextAction, EKeys::Tab);
	MapKey(SelectNextAction, EKeys::MouseScrollUp);
	MapKey(SelectNextAction, EKeys::Gamepad_RightShoulder);
	SelectPrevAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(SelectPrevAction, EKeys::MouseScrollDown);
	MapKey(SelectPrevAction, EKeys::Gamepad_LeftTrigger); // pad prev (review minor; RT stays fire)
	DirectPickAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(DirectPickAction, EKeys::E);
	MapKey(DirectPickAction, EKeys::Gamepad_FaceButton_Left);
	StanceToggleAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(StanceToggleAction, EKeys::Gamepad_FaceButton_Top);

	// Genre parity (owner playtest 2026-07-25): jump and aim did not exist at all,
	// and LT — the aim button in this genre — was carrying "previous soldier".
	JumpAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(JumpAction, EKeys::SpaceBar);
	MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);   // A
	AimAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(AimAction, EKeys::RightMouseButton);
	// LT is deliberately mapped TWICE — to aim here and to SelectPrev above — and
	// the two handlers split on CommandMode->IsHeld(). That is the owner's
	// preferred context-dependent behaviour WITHOUT a second mapping context:
	// SPEC-P2-07 owns the Enhanced Input context stacks, and pre-empting it with a
	// rival stack is exactly what this must not do. One branch in each handler is
	// not a mode system; it is the same button reading the state that already
	// exists.
	MapKey(AimAction, EKeys::Gamepad_LeftTrigger);

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleLook);
	Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleFire);
	Input->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEclipsePlayerController::HandleSprint);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleCrouch);
	Input->BindAction(ToggleViewAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleToggleView);

	Input->BindActionValueLambda(OrderActions[0], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::MoveTo); });
	Input->BindActionValueLambda(OrderActions[1], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::FocusTarget); });
	Input->BindActionValueLambda(OrderActions[2], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::Hold); });
	Input->BindActionValueLambda(OrderActions[3], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::Regroup); });

	// Command Mode framing rides along with the existing hold — the camera is a
	// consumer here, not a second mode (SPEC-P2-07 owns the context stacks).
	Input->BindActionValueLambda(CommandHoldAction, ETriggerEvent::Started, [this](const FInputActionValue&)
	{
		if (CommandMode != nullptr) { CommandMode->OnHoldPressed(); }
		if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn())) { Body->SetCommandModeCamera(true); }
	});
	Input->BindActionValueLambda(CommandHoldAction, ETriggerEvent::Completed, [this](const FInputActionValue&)
	{
		if (CommandMode != nullptr) { CommandMode->OnHoldReleased(); }
		if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn())) { Body->SetCommandModeCamera(false); }
	});
	Input->BindActionValueLambda(SelectNextAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->CycleSoldierSelection(+1); } });
	// LT shares itself with aim: during the hold it cycles, in the field it does
	// not (the aim handler takes the opposite branch). Mouse-scroll keeps working
	// either way — a wheel is not ambiguous.
	Input->BindActionValueLambda(SelectPrevAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->CycleSoldierSelection(-1); } });
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleJump);
	Input->BindAction(AimAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleAimStart);
	Input->BindAction(AimAction, ETriggerEvent::Completed, this, &AEclipsePlayerController::HandleAimStop);
	Input->BindActionValueLambda(DirectPickAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->PickSoldierUnderReticle(); } });
	Input->BindActionValueLambda(StanceToggleAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->ToggleHeldStance(); } });

	// Debug overlay (feel gauntlet, SPEC-P2-02 R3) + the in-game test guide
	// (phase0/INGAME_TESTGIDS.md). Function keys and letters that no gameplay
	// action claims: F1 is the engine's, F2/F4-F8/H were already the gauntlet's,
	// and F3/J/N were free (checked against every EKeys reference in the module).
	// The handlers route into the one HUD widget — there is no second overlay.
	// PadKey is EKeys::Invalid where a debug function has no controller button:
	// the gauntlet's measurement keys stay keyboard-only on purpose (they are the
	// reviewer's instrumentation, not controls the player exercises), but the
	// GUIDE has to be reachable on a pad or a controller playtest cannot use it —
	// which is what the owner ran into. Special_Left/Right (View and Menu) were
	// the only untaken pad buttons; View opens the guide, Menu confirms, and
	// skipping doubles up on B, which is "no" on every console.
	struct FDebugOverlayBinding
	{
		FKey Key;
		FKey PadKey;
		void (*Invoke)(UEclipseMissionHudWidget&);
	};
	const FDebugOverlayBinding DebugBindings[] = {
		{ EKeys::F2, EKeys::Invalid,                 [](UEclipseMissionHudWidget& Hud) { Hud.ToggleControlsPanel(); } },
		{ EKeys::H,  EKeys::Invalid,                 [](UEclipseMissionHudWidget& Hud) { Hud.TogglePlaytestPanel(); } },
		{ EKeys::F3, EKeys::Gamepad_Special_Left,    [](UEclipseMissionHudWidget& Hud) { Hud.ToggleGuidePanel(); } },
		{ EKeys::J,  EKeys::Gamepad_Special_Right,   [](UEclipseMissionHudWidget& Hud) { Hud.ConfirmGuideStep(); } },
		{ EKeys::N,  EKeys::Invalid,                 [](UEclipseMissionHudWidget& Hud) { Hud.SkipGuideStep(); } },
		{ EKeys::F4, EKeys::Invalid, [](UEclipseMissionHudWidget& Hud) { Hud.NoteTargetingPick(/*bCleanPick*/ true); } },
		{ EKeys::F5, EKeys::Invalid, [](UEclipseMissionHudWidget& Hud) { Hud.NoteTargetingPick(/*bCleanPick*/ false); } },
		{ EKeys::F6, EKeys::Invalid, [](UEclipseMissionHudWidget& Hud) { Hud.CycleComfortAnswer(); } },
		{ EKeys::F7, EKeys::Invalid, [](UEclipseMissionHudWidget& Hud) { Hud.CycleConfidenceAnswer(); } },
		{ EKeys::F8, EKeys::Invalid, [](UEclipseMissionHudWidget& Hud) { Hud.MarkEncounterBeat(); } },
		{ EKeys::Six, EKeys::Invalid,   [](UEclipseMissionHudWidget& Hud) { Hud.CyclePlaytestAnswer(0); } },
		{ EKeys::Seven, EKeys::Invalid, [](UEclipseMissionHudWidget& Hud) { Hud.CyclePlaytestAnswer(1); } },
		{ EKeys::Eight, EKeys::Invalid, [](UEclipseMissionHudWidget& Hud) { Hud.CyclePlaytestAnswer(2); } },
		{ EKeys::Nine, EKeys::Invalid,  [](UEclipseMissionHudWidget& Hud) { Hud.CyclePlaytestAnswer(3); } },
		{ EKeys::Zero, EKeys::Invalid,  [](UEclipseMissionHudWidget& Hud) { Hud.CyclePlaytestAnswer(4); } }
	};

	DebugOverlayActions.SetNum(static_cast<int32>(UE_ARRAY_COUNT(DebugBindings)));
	for (int32 Index = 0; Index < DebugOverlayActions.Num(); ++Index)
	{
		DebugOverlayActions[Index] = MakeAction(EInputActionValueType::Boolean);
		MapKey(DebugOverlayActions[Index], DebugBindings[Index].Key);
		if (DebugBindings[Index].PadKey.IsValid())
		{
			MapKey(DebugOverlayActions[Index], DebugBindings[Index].PadKey);
		}

		void (*Invoke)(UEclipseMissionHudWidget&) = DebugBindings[Index].Invoke;
		Input->BindActionValueLambda(DebugOverlayActions[Index], ETriggerEvent::Started, [this, Invoke](const FInputActionValue&)
		{
			// Mounted HUD only: at the base the widget is torn down, and pressing a
			// gauntlet key there must not quietly edit the next run's numbers.
			if (MissionHud != nullptr && MissionHud->IsInViewport())
			{
				Invoke(*MissionHud);
			}
		});
	}

	// Test-guide detection (variant A). A SECOND delegate on the actions bound
	// above — Enhanced Input dispatches every binding that matches an action and
	// event, so the gameplay handler keeps running untouched, nothing is consumed
	// and no mapping changes. ETriggerEvent::Started, not Triggered: Started fires
	// once when an action actuates (None -> Triggered raises Started first,
	// EnhancedPlayerInput.cpp), so holding W does not call the guide every frame.
	// This is why the guide needs no tick and reads no keys itself.
	struct FGuideSignalBinding
	{
		UInputAction* Action;
		EclipseTestGuide::EEclipseGuideSignal Signal;
	};
	const FGuideSignalBinding GuideSignals[] = {
		{ MoveAction,          EclipseTestGuide::EEclipseGuideSignal::Move },
		{ LookAction,          EclipseTestGuide::EEclipseGuideSignal::Look },
		{ FireAction,          EclipseTestGuide::EEclipseGuideSignal::Fire },
		{ SprintAction,        EclipseTestGuide::EEclipseGuideSignal::Sprint },
		{ CrouchAction,        EclipseTestGuide::EEclipseGuideSignal::Crouch },
		{ JumpAction,          EclipseTestGuide::EEclipseGuideSignal::Jump },
		{ AimAction,           EclipseTestGuide::EEclipseGuideSignal::Aim },
		{ ToggleViewAction,    EclipseTestGuide::EEclipseGuideSignal::ToggleView },
		{ CommandHoldAction,   EclipseTestGuide::EEclipseGuideSignal::CommandMode },
		{ SelectNextAction,    EclipseTestGuide::EEclipseGuideSignal::SelectNext },
		{ SelectPrevAction,    EclipseTestGuide::EEclipseGuideSignal::SelectPrev },
		{ DirectPickAction,    EclipseTestGuide::EEclipseGuideSignal::DirectPick },
		{ StanceToggleAction,  EclipseTestGuide::EEclipseGuideSignal::Stance },
		// All four order keys answer the one "orders" step; the step is about the
		// order path working, not about which of the four you happened to press.
		{ OrderActions[0],     EclipseTestGuide::EEclipseGuideSignal::Order },
		{ OrderActions[1],     EclipseTestGuide::EEclipseGuideSignal::Order },
		{ OrderActions[2],     EclipseTestGuide::EEclipseGuideSignal::Order },
		{ OrderActions[3],     EclipseTestGuide::EEclipseGuideSignal::Order }
	};

	for (const FGuideSignalBinding& Binding : GuideSignals)
	{
		if (Binding.Action == nullptr)
		{
			continue; // a missing action costs the guide one undetectable step, never a crash (GDD 14.3.5)
		}
		const EclipseTestGuide::EEclipseGuideSignal Signal = Binding.Signal;
		Input->BindActionValueLambda(Binding.Action, ETriggerEvent::Started, [this, Signal](const FInputActionValue&)
		{
			if (MissionHud != nullptr && MissionHud->IsInViewport())
			{
				MissionHud->NoteGuideSignal(Signal);
			}
		});
	}
}

void AEclipsePlayerController::HandleMove(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		return;
	}
	FVector2D Axis = Value.Get<FVector2D>();

	// Movement had NO deadzone at all while look had one, and that single omission
	// produced two symptoms that looked like separate bugs (owner measurement
	// 2026-07-25, phase0/controller_kalibratie.json: the left stick rests at
	// LY = -0.048). The pawn crept forward forever, and because the movement
	// component orients the body to its movement direction, the character also
	// rotated slowly on its own while nobody touched anything.
	//
	// RADIAL, not per-axis: a per-axis deadzone carves a SQUARE hole out of a round
	// stick, so a diagonal push has to clear the zone on both axes and diagonal
	// walking would start later and feel different from walking straight. The same
	// rescale as the look path — past the deadzone the value restarts at zero
	// instead of jumping to the deadzone's worth of speed, so a gentle push is
	// actually a gentle walk.
	if (IsUsingGamepadLook())
	{
		const float Magnitude = Axis.Size();
		if (Magnitude <= MoveDeadzone)
		{
			return;
		}
		const float Live = FMath::Clamp((Magnitude - MoveDeadzone) / FMath::Max(1.0f - MoveDeadzone, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		Axis = (Axis / FMath::Max(Magnitude, KINDA_SMALL_NUMBER)) * Live;
	}

	const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AEclipsePlayerController::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const int32 InvertOverride = CVarEclipseInvertLookY.GetValueOnGameThread();
	const bool bInvert = InvertOverride < 0 ? bInvertLookY : (InvertOverride > 0);
	const float InvertY = bInvert ? -1.0f : 1.0f;

	// Mouse and stick are different devices and must not share a curve. A mouse
	// has no drift and no rest position, so it stays RAW — deadzoning or curving
	// it would break the muscle memory of every player who has used one. A stick
	// rests at a noisy near-zero and needs both.
	if (IsUsingGamepadLook())
	{
		const float Magnitude = Axis.Size();
		if (Magnitude <= StickDeadzone)
		{
			return; // stick drift may never move the camera on its own
		}
		// Rescale past the deadzone so the first live degree of deflection starts
		// at zero speed instead of jumping to the deadzone's worth of speed.
		const FVector2D Direction = Axis / FMath::Max(Magnitude, KINDA_SMALL_NUMBER);
		const float Live = FMath::Clamp((Magnitude - StickDeadzone) / FMath::Max(1.0f - StickDeadzone, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		// Sign-preserving exponential response: small deflections stay small so
		// fine aim is possible, full deflection still gives full speed. This is
		// the difference between aiming and wrestling.
		const float Curved = FMath::Pow(Live, StickResponseExponent);
		const float DeltaSeconds = GetWorld() != nullptr ? GetWorld()->GetDeltaSeconds() : 0.0f;
		// Degrees per second, so stick look is framerate-independent — unlike the
		// mouse path, which is already per-event.
		AddYawInput(Direction.X * Curved * StickYawSpeed * DeltaSeconds);
		AddPitchInput(-Direction.Y * Curved * StickPitchSpeed * DeltaSeconds * InvertY);
		return;
	}

	AddYawInput(Axis.X * MouseLookScale);
	AddPitchInput(-Axis.Y * MouseLookScale * InvertY);
}

bool AEclipsePlayerController::IsUsingGamepadLook() const
{
	// Enhanced Input hands mouse and stick to the SAME action, so the device has
	// to be asked rather than inferred from the value: a stick held halfway and a
	// slow mouse produce identical numbers. Same source the HUD's device column
	// already uses (EclipseMissionHudWidget::RefreshDeviceHighlight) — one answer
	// to "what is the player holding", not two that can disagree.
	if (const UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
	{
		return Devices->GetMostRecentlyUsedHardwareDevice(GetPlatformUserId()).PrimaryDeviceType
			== EHardwareDevicePrimaryType::Gamepad;
	}
	return false; // no subsystem (commandlet) — treat as mouse, i.e. raw
}

void AEclipsePlayerController::HandleToggleView()
{
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (Body == nullptr)
	{
		return;
	}
	Body->SetFirstPerson(!Body->IsFirstPerson());
	UE_LOG(LogEclipse, Verbose, TEXT("View toggled to %s person."),
		Body->IsFirstPerson() ? TEXT("first") : TEXT("third"));
}

void AEclipsePlayerController::HandleFire()
{
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	UEclipseHitscanWeaponComponent* Weapon = Body != nullptr ? Body->FindComponentByClass<UEclipseHitscanWeaponComponent>() : nullptr;
	if (Weapon == nullptr)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	Weapon->Fire(ViewLocation, ViewRotation.Vector(), TEXT("PlayerFire"));
}

void AEclipsePlayerController::HandleSprint(const FInputActionValue& Value)
{
	// PLACEHOLDER(feel targets §2): sprint toggles run<->sprint speed; stamina
	// only bites above Medium armor and armor lands later.
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (Body == nullptr)
	{
		return;
	}

	// Speeds from DA_CharacterTuning (GDD 14.2); the fallbacks mirror the locked
	// feel targets so a missing asset degrades to the same numbers (GDD 14.3.5).
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	const UEclipseCharacterTuningAsset* Tuning = Setup != nullptr ? Setup->CharacterTuning.LoadSynchronous() : nullptr;

	const bool bSprinting = Value.Get<bool>();
	Body->GetCharacterMovement()->MaxWalkSpeed = bSprinting
		? (Tuning != nullptr ? Tuning->SprintSpeed : 650.0f)
		: (Tuning != nullptr ? Tuning->RunSpeed : 420.0f);
}

void AEclipsePlayerController::HandleCrouch()
{
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
	{
		Body->bIsCrouched ? Body->UnCrouch() : Body->Crouch();
	}
}

void AEclipsePlayerController::HandleJump()
{
	if (ACharacter* Body = Cast<ACharacter>(GetPawn()))
	{
		Body->Jump();
	}
}

void AEclipsePlayerController::HandleAimStart()
{
	// LT is aim in the field and "previous soldier" during the Command hold. The
	// split lives here rather than in a second mapping context, because the input
	// context stack belongs to SPEC-P2-07 and must not be forked early.
	if (CommandMode != nullptr && CommandMode->IsHeld())
	{
		return;
	}
	SetAiming(true);
}

void AEclipsePlayerController::HandleAimStop()
{
	SetAiming(false);
}

void AEclipsePlayerController::SetAiming(bool bNewAiming)
{
	if (bAiming == bNewAiming)
	{
		return;
	}
	bAiming = bNewAiming;
	// Debug-grade ADS (GDD 14.5): narrow the FOV so aiming READS, and slow the
	// stick so the smaller angle is not harder to hold. No weapon spread model,
	// no accuracy change, no new system — those belong to the combat feel pass.
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
	{
		Body->SetAiming(bAiming);
	}
}

void AEclipsePlayerController::ApplyLookTuning()
{
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance() != nullptr
		? GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	const UEclipseCharacterTuningAsset* Tuning = Setup != nullptr ? Setup->CharacterTuning.LoadSynchronous() : nullptr;
	if (Tuning == nullptr)
	{
		// The member defaults already mirror the asset, so look still works
		// (GDD 14.3.5) — but say so, because silent defaults are how a tuning
		// asset stops being the authority without anyone noticing.
		UE_LOG(LogEclipse, Warning, TEXT("Look tuning: no DA_CharacterTuning on the active setup — using the built-in look defaults."));
		return;
	}

	StickYawSpeed = Tuning->StickYawSpeed;
	StickPitchSpeed = Tuning->StickPitchSpeed;
	StickDeadzone = Tuning->StickDeadzone;
	MoveDeadzone = Tuning->MoveDeadzone;
	StickResponseExponent = Tuning->StickResponseExponent;
	MouseLookScale = Tuning->MouseLookScale;
	bInvertLookY = Tuning->bInvertLookY;

	// Pitch limits live on the camera manager, not on the look handler: clamping
	// after the fact would let the view reach the limit and stick there, and the
	// manager is also what the spring arm reads.
	if (PlayerCameraManager != nullptr)
	{
		PlayerCameraManager->ViewPitchMin = Tuning->ViewPitchMin;
		PlayerCameraManager->ViewPitchMax = Tuning->ViewPitchMax;
	}
}

bool AEclipsePlayerController::GetAimPoint(FVector& OutLocation, AActor*& OutActor) const
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	constexpr float AimReachCm = 10000.0f;
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * AimReachCm;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseAim), false, GetPawn());

	// Geometry (Visibility) fixes the move-to point — walls and floor, which pawn
	// capsules deliberately don't block.
	FHitResult GeoHit;
	const bool bHitGeo = GetWorld()->LineTraceSingleByChannel(GeoHit, ViewLocation, TraceEnd, ECC_Visibility, Params);
	OutLocation = bHitGeo ? GeoHit.ImpactPoint : TraceEnd;

	// Actor acquisition MUST use a channel pawn capsules block (Pawn) — the same
	// channel the weapon fires on. Tracing actors on Visibility passes straight
	// through every character, so Focus-target could never lock a live enemy.
	FHitResult PawnHit;
	OutActor = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(PawnHit, ViewLocation, TraceEnd, ECC_Pawn, Params))
	{
		OutActor = PawnHit.GetActor();
		if (!bHitGeo || PawnHit.Distance < GeoHit.Distance)
		{
			OutLocation = PawnHit.ImpactPoint; // a body in front of the wall is the aim point
		}
	}
	return OutActor != nullptr || bHitGeo;
}

void AEclipsePlayerController::IssueSquadOrder(EEclipseSquadOrder Order)
{
	UEclipseSquadSubsystem* Squad = GetWorld()->GetSubsystem<UEclipseSquadSubsystem>();
	if (Squad == nullptr)
	{
		return;
	}

	FVector AimLocation = GetPawn() != nullptr ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	AActor* AimActor = nullptr;
	GetAimPoint(AimLocation, AimActor);

	const FVector Target = Order == EEclipseSquadOrder::Regroup && GetPawn() != nullptr
		? GetPawn()->GetActorLocation()
		: AimLocation;

	// Stance stub (SPEC-P1-06): outside the mode, hold Left Alt while ordering
	// for Aggressive (pad LB is the command hold now — SPEC-P2-02); inside the
	// held mode the component's toggled stance applies. PLACEHOLDER(GDD 8.4):
	// stance drives posture/ROE in the feel pass.
	const bool bHeld = CommandMode != nullptr && CommandMode->IsHeld();
	const EEclipseSquadStance Stance = bHeld
		? CommandMode->GetHeldStance()
		: (IsInputKeyDown(EKeys::LeftAlt) ? EEclipseSquadStance::Aggressive : EEclipseSquadStance::Ready);

	// Per-soldier dispatch (SPEC-P2-02 locked decision 4): a selection routes
	// through the existing IssueOrder; no selection keeps the Phase 1 broadcast.
	// A stale selection (died this hold) falls back to everyone — audibly, via
	// the normal ack/refusal chain, never silently dropped.
	const FGuid Selected = bHeld ? CommandMode->GetSelectedSoldier() : FGuid();
	if (Selected.IsValid() && Squad->IssueOrder(Selected, Order, Target, AimActor, Stance))
	{
		// single-soldier path took it
	}
	else
	{
		Squad->IssueOrderToAll(Order, Target, AimActor, Stance);
	}
	if (bHeld)
	{
		CommandMode->NotifyOrderIssued();
	}
}
