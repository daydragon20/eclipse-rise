#include "Characters/EclipsePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipseCommandModeComponent.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
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
	if (MissionHud == nullptr)
	{
		MissionHud = CreateWidget<UEclipseMissionHudWidget>(this, UEclipseMissionHudWidget::StaticClass());
	}
	if (MissionHud != nullptr && !MissionHud->IsInViewport())
	{
		MissionHud->AddToViewport(5);
	}

	bShowMouseCursor = false;
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

	// Gamepad (owner request 2026-07-22): the same actions, Xbox-layout keys, so
	// pad and mouse/keyboard coexist in one context — no mode switch, last device
	// wins (EnhancedInput default). Left2D matches HandleMove's axes directly
	// (X = right, Y = forward), so no swizzle is needed on the stick.
	MapKey(MoveAction, EKeys::Gamepad_Left2D);
	{
		// Stick look adds per-frame rate, not mouse deltas — scale it up or a
		// full deflection turns ~3x slower than a normal mouse swipe.
		// PLACEHOLDER(GDD 8.1): per-frame rate is framerate-bound; the combat
		// feel pass gives look its own sensitivity curve + deltatime scaling.
		FEnhancedActionKeyMapping& StickLook = MappingContext->MapKey(LookAction, EKeys::Gamepad_Right2D);
		UInputModifierScalar* LookRate = NewObject<UInputModifierScalar>(this);
		LookRate->Scalar = FVector(2.0, 1.5, 1.0);
		StickLook.Modifiers.Add(LookRate);
	}
	MapKey(FireAction, EKeys::Gamepad_RightTrigger);
	MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);

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

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleLook);
	Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleFire);
	Input->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEclipsePlayerController::HandleSprint);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleCrouch);

	Input->BindActionValueLambda(OrderActions[0], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::MoveTo); });
	Input->BindActionValueLambda(OrderActions[1], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::FocusTarget); });
	Input->BindActionValueLambda(OrderActions[2], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::Hold); });
	Input->BindActionValueLambda(OrderActions[3], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::Regroup); });

	Input->BindActionValueLambda(CommandHoldAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->OnHoldPressed(); } });
	Input->BindActionValueLambda(CommandHoldAction, ETriggerEvent::Completed, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->OnHoldReleased(); } });
	Input->BindActionValueLambda(SelectNextAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->CycleSoldierSelection(+1); } });
	Input->BindActionValueLambda(SelectPrevAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->CycleSoldierSelection(-1); } });
	Input->BindActionValueLambda(DirectPickAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->PickSoldierUnderReticle(); } });
	Input->BindActionValueLambda(StanceToggleAction, ETriggerEvent::Started, [this](const FInputActionValue&) { if (CommandMode != nullptr) { CommandMode->ToggleHeldStance(); } });
}

void AEclipsePlayerController::HandleMove(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AEclipsePlayerController::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);
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
