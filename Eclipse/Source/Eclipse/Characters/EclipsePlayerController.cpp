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
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"
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

	/**
	 * Welk apparaat de handlers moeten aannemen. Bestaat om twee redenen, en
	 * allebei zijn ze meetbaar: (1) het feel-harnas injecteert op de actie en
	 * raakt dus nooit echte hardware, waardoor de autodetectie altijd
	 * "muis/toetsenbord" zegt en de hele stick-tak (deadzone, curve, graden per
	 * seconde) ongetest zou blijven; (2) de owner kan met één regel A/B'en of een
	 * klacht aan de stick-tak of aan de muis-tak ligt.
	 */
	TAutoConsoleVariable<int32> CVarEclipseForceGamepad(
		TEXT("Eclipse.Input.ForceGamepad"),
		-1,
		TEXT("Behandel invoer als: -1 = autodetectie (apparaat vragen), 0 = muis/toetsenbord, 1 = gamepad."),
		ECVF_Default);
}

AEclipsePlayerController::AEclipsePlayerController()
{
	CommandMode = CreateDefaultSubobject<UEclipseCommandModeComponent>(TEXT("CommandMode"));

	// GEMETEN DEFECT (feel-harnas laag 2, 2026-07-25): het kijken liep op
	// 600 gr/s terwijl DA_CharacterTuning 240 gr/s zegt en het testgids-paneel
	// "1,50 s per 360" toont. De echte draai kostte 0,60 s.
	//
	// Oorzaak: APlayerController::AddYawInput/AddPitchInput vermenigvuldigen nog
	// met de LEGACY-schalen uit Engine/Config/BaseGame.ini — InputYawScale = 2.5
	// en InputPitchScale = -2.5 — zolang UInputSettings::bEnableLegacyInputScales
	// aanstaat, en die staat standaard aan voor achterwaartse compatibiliteit. Die
	// schalen stammen uit het oude input-systeem, waar een as-mapping zelf geen
	// gevoeligheid droeg; met Enhanced Input rekent HandleLook zelf in graden per
	// seconde, en dan is de vermenigvuldiging een tweede, onzichtbare
	// gevoeligheidsknop bovenop de getunede.
	//
	// Erger dan de factor is het TEKEN: de pitch-schaal is NEGATIEF, dus de
	// handler compenseerde onbewust een verborgen omkering, en "invert Y" was
	// daardoor niet te redeneren maar alleen uit te proberen.
	//
	// De schakelaar zelf staat in Eclipse/Config/DefaultInput.ini (het is een
	// project-instelling, geen actor-veld); HandleLook draagt het pitch-teken nu
	// zelf. Laag 1 van het harnas bewaakt dat de instelling uit blijft staan.
}

void AEclipsePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(MappingContext, /*Priority*/ 0);
	}

	// Feel-audit-commando: één regel met snelheid, mesh-schaal, boomlengte, FOV en
	// modus naast elkaar. Debug-tier (14.5), geen tick, alleen op verzoek.
	FeelDumpCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Eclipse.Feel.Dump"),
		TEXT("Print snelheid, mesh-schaal, camera-boom, FOV en cameramodus van de speler."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]() { DumpFeelState(); }),
		ECVF_Default);

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
	if (FeelDumpCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(FeelDumpCommand);
		FeelDumpCommand = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AEclipsePlayerController::LogNavigationState(const TCHAR* When) const
{
	const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (Nav == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Navigatie (%s): GEEN navigatiesysteem in deze wereld — de squad kan nergens heen."), When);
		return;
	}
	FNavLocation Projected;
	const bool bNavUnderPawn = GetPawn() != nullptr
		&& Nav->ProjectPointToNavigation(GetPawn()->GetActorLocation(), Projected, FVector(500.0f, 500.0f, 500.0f));
	// De GROOTTE van de grens erbij, want "1 grens" zegt niets als die grens leeg
	// is. Een runtime-gespawnde ANavMeshBoundsVolume draagt zijn brush niet
	// vanzelf, en dan staat er wel een grens geregistreerd met een lege doos erin
	// — dat is precies het onderscheid dat de vorige drie runs niet konden maken.
	FBox Total(ForceInit);
	for (const FNavigationBounds& Bounds : Nav->GetNavigationBounds())
	{
		Total += Bounds.AreaBox;
	}
	UE_LOG(LogEclipse, Display,
		TEXT("Navigatie (%s): %d grens(zen), samen %.0f x %.0f x %.0f uu, navmesh onder de speler = %s.%s"),
		When, Nav->GetNavigationBounds().Num(),
		Total.IsValid ? Total.GetSize().X : 0.0f, Total.IsValid ? Total.GetSize().Y : 0.0f,
		Total.IsValid ? Total.GetSize().Z : 0.0f,
		bNavUnderPawn ? TEXT("JA") : TEXT("NEE"),
		bNavUnderPawn ? TEXT("") : TEXT(" Zonder navmesh weigert de squad elke verplaatsingsorder met 'no route'."));
}

void AEclipsePlayerController::DumpFeelState() const
{
	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (Body == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Feel: geen bestuurd personage."));
		return;
	}
	const FString Line = FString::Printf(TEXT("%s · sprint %s"),
		*Body->DescribeFeelState(),
		bSprinting ? (bSprintLatched ? TEXT("AAN (L3-latch)") : TEXT("AAN (hold)")) : TEXT("uit"));
	UE_LOG(LogEclipse, Display, TEXT("Feel: %s"), *Line);
	// Tweede regel: de bewegingswaarden zoals ze op het component staan. Eén toets
	// hoort alles te geven wat er tijdens het spelen kan afwijken.
	UE_LOG(LogEclipse, Display, TEXT("Feel: %s"), *Body->DescribeMovementState());
	UE_LOG(LogEclipse, Display, TEXT("Feel: %s"), *DescribeLookTuning());
	// Ook op het scherm: wie de meting doet, speelt op dat moment — die leest geen
	// logbestand terwijl hij loopt (dat is precies waarom S3 een symptoom werd).
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(/*Key*/ 90901, /*Time*/ 8.0f, FColor::Yellow, FString::Printf(TEXT("FEEL: %s"), *Line));

		// En het VERSCHIL met de vorige druk, want dat is de eigenlijke vraag.
		// De owner moet weten of zijn personage met de snelheid meeschaalt, en
		// daarvoor drukt hij één keer stappend en één keer sprintend. Twee losse
		// regels dwingen hem getallen te onthouden terwijl hij speelt — en de
		// tweede overschreef de eerste ook nog, want het is dezelfde sleutel.
		// Nu rekent de dump het zelf uit.
		const FEclipseFeelSample Now = Body->SampleFeelState();
		if (bHasPreviousFeelSample)
		{
			const float SizeDeltaPct = PreviousFeelSample.ApparentHeightDegrees > KINDA_SMALL_NUMBER
				? (Now.ApparentHeightDegrees - PreviousFeelSample.ApparentHeightDegrees)
					/ PreviousFeelSample.ApparentHeightDegrees * 100.0f
				: 0.0f;
			// Meer dan 2% verschil in schijnbare grootte is wat je als "hij schaalt"
			// zou zien; daaronder is het meetruis van de camera-lag.
			const bool bSuspicious = FMath::Abs(SizeDeltaPct) > 2.0f;
			GEngine->AddOnScreenDebugMessage(/*Key*/ 90902, /*Time*/ 8.0f,
				bSuspicious ? FColor::Red : FColor::Green,
				FString::Printf(TEXT("FEEL Δ sinds vorige druk: snelheid %+.0f cm/s · schijnbare grootte %+.2f%% · camera-afstand %+.1f cm  %s"),
					Now.SpeedCm - PreviousFeelSample.SpeedCm,
					SizeDeltaPct,
					Now.CameraToPawnCm - PreviousFeelSample.CameraToPawnCm,
					bSuspicious ? TEXT("<< SCHAALT MEE — dit is S1") : TEXT("(binnen ruis)")));
			UE_LOG(LogEclipse, Display,
				TEXT("Feel: Δ sinds vorige druk — snelheid %+.0f cm/s, schijnbare grootte %+.2f%%, camera-afstand %+.1f cm."),
				Now.SpeedCm - PreviousFeelSample.SpeedCm, SizeDeltaPct,
				Now.CameraToPawnCm - PreviousFeelSample.CameraToPawnCm);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(/*Key*/ 90902, /*Time*/ 8.0f, FColor::White,
				TEXT("FEEL: druk nu nogmaals F9 op een ANDERE snelheid — dan verschijnt hier het verschil"));
		}
		PreviousFeelSample = Now;
		bHasPreviousFeelSample = true;
	}
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

bool AEclipsePlayerController::HasGameViewport() const
{
	return GetWorld() != nullptr && GetWorld()->GetGameViewport() != nullptr;
}

void AEclipsePlayerController::EnterBaseMode()
{
	if (BaseHub == nullptr && HasGameViewport())
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

	// Leaving the field clears the sprint state in both its forms: a latched
	// sprint that survived into the hub would silently start the next mission at
	// sprint speed with nothing on screen saying so.
	bSprintHeld = false;
	bSprintLatched = false;
	ApplySprintDrive();

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
	if (MissionHud == nullptr && UEclipseMissionHudWidget::IsDebugHudAllowed() && HasGameViewport())
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
	// Owner-blokkade 2026-07-25: geen enkele controller-knop leek aan te komen, en
	// "de mapping klopt" en "het apparaat levert niets" zien er van buitenaf
	// identiek uit. Dit zegt bij missiestart plat wat de engine ziet, zodat de
	// vraag "ligt het aan mij of aan de game" beantwoordbaar is zonder te gokken.
	if (const UInputDeviceSubsystem* Devices = UInputDeviceSubsystem::Get())
	{
		const FHardwareDeviceIdentifier Hardware = Devices->GetMostRecentlyUsedHardwareDevice(GetPlatformUserId());
		// BEGINSTAND, niet een oordeel: bij missiestart heeft de speler nog niets
		// aangeraakt, dus "Gamepad = nee" betekent hier alleen "nog geen pad-input
		// gezien". De regels die tellen zijn de per-actie-regels hieronder.
		UE_LOG(LogEclipse, Display, TEXT("Input (BEGINSTAND, vóór de eerste druk): apparaat = '%s' (input class '%s', type %d). Gamepad = %s."),
			*Hardware.HardwareDeviceIdentifier.ToString(), *Hardware.InputClassName.ToString(),
			static_cast<int32>(Hardware.PrimaryDeviceType),
			Hardware.PrimaryDeviceType == EHardwareDevicePrimaryType::Gamepad ? TEXT("JA") : TEXT("nee"));
	}
	else
	{
		UE_LOG(LogEclipse, Warning, TEXT("Input: geen UInputDeviceSubsystem — apparaatdetectie onbeschikbaar."));
	}

	UE_LOG(LogEclipse, Display, TEXT("Mission mode: debug HUD %s (F2 controls, F3 test guide, F9 feel-dump, H playtest)."),
		MissionHud == nullptr
			? (!UEclipseMissionHudWidget::IsDebugHudAllowed()
				? TEXT("suppressed by -EclipseShot, as designed")
				: (!HasGameViewport()
					? TEXT("skipped — no game viewport (headless run), as designed")
					: TEXT("NOT CREATED — widget construction failed")))
			: (MissionHud->IsInViewport() ? TEXT("mounted") : TEXT("created but NOT in the viewport")));

	// Kan de squad überhaupt ergens heen? Zonder navmesh weigert elke MoveTo met
	// NoRoute, en dat ziet er voor de speler uit als een squad die niets doet.
	// Deze regel maakt "volgt mijn squad me?" een MEETBARE vraag in plaats van een
	// indruk — dezelfde reden als de bindings-regel hieronder.
	LogNavigationState(TEXT("bij missiestart"));
	// En nog een keer NA vijf seconden, en dat is geen luxe: Recast bouwt zijn
	// tegels asynchroon, dus een meting op t=0 is per constructie te vroeg en zegt
	// altijd "nee". Drie diagnostische runs zijn daarop stukgelopen voordat ik
	// doorhad dat het meetmoment het probleem was, niet de navmesh.
	FTimerHandle NavRecheck;
	GetWorldTimerManager().SetTimer(NavRecheck, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		LogNavigationState(TEXT("vijf seconden later"));
	}), 5.0f, /*bLoop*/ false);

	// S3, feel-audit 2026-07-25: de owner bond F9 aan Eclipse.Feel.Dump via
	// DebugExecBindings en er kwam nooit een regel in het log. "De binding is niet
	// geladen" en "de toets is niet geraakt" zien er van buitenaf identiek uit, dus
	// zegt de game nu plat wat de PlayerInput daadwerkelijk aan debug-bindings
	// heeft. Merk op dat F9 sinds deze sessie ook een ECHTE Enhanced-Input-binding
	// is (zie de debug-tabel in SetupInputComponent) — dat pad heeft geen config
	// nodig en is daarmee het antwoord; deze regel is de diagnose voor de rest.
	if (PlayerInput != nullptr)
	{
		FString Binds;
		for (const FKeyBind& Bind : PlayerInput->DebugExecBindings)
		{
			Binds += FString::Printf(TEXT("%s=%s "), *Bind.Key.ToString(), *Bind.Command);
		}
		UE_LOG(LogEclipse, Display, TEXT("DebugExecBindings geladen: %d %s(bron: Config/DefaultInput.ini; Saved/Config is GEGENEREERD en wordt door de engine overschreven)."),
			PlayerInput->DebugExecBindings.Num(),
			Binds.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("[%s] "), *Binds.TrimEnd()));
	}

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
			if (!It->ActorHasTag(TEXT("Entry_Main")))
			{
				continue;
			}

			// Op de GROND zetten in plaats van er een vaste marge boven. De vorige
			// versie zette de pawn 100 cm boven het insertiepunt, en omdat het
			// insertiepunt zelf ook al boven de vloer staat, viel de speler bij
			// ELKE missiestart 1,6 meter — 0,35 seconde waarin de besturing niet
			// van hem was (gemeten door de speelronde, 2026-07-26).
			//
			// De marge bestond om te voorkomen dat je in de vloer spawnt, en die
			// zorg blijft terecht; hij wordt alleen niet meer geraden. Traceren
			// naar beneden geeft de echte vloer, en daar komt de halve capsule plus
			// een paar centimeter bovenop — dat is exact de rusthoogte, dus er valt
			// niets meer te vallen én niets meer in te zakken.
			const FVector Entry = It->GetActorLocation();
			FVector Target = Entry + FVector(0.0f, 0.0f, 100.0f);
			const ACharacter* Body = Cast<ACharacter>(ControlledPawn);
			const float HalfHeight = Body != nullptr && Body->GetCapsuleComponent() != nullptr
				? Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.0f;

			FHitResult Ground;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseInsertion), false, ControlledPawn);
			if (GetWorld()->LineTraceSingleByChannel(Ground, Entry + FVector(0.0f, 0.0f, 200.0f),
					Entry - FVector(0.0f, 0.0f, 2000.0f), ECC_Visibility, Params))
			{
				Target = Ground.ImpactPoint + FVector(0.0f, 0.0f, HalfHeight + 5.0f);
			}
			else
			{
				// Geen vloer gevonden (gat, of het insertiepunt hangt boven niets):
				// terug naar de oude marge, en luid, want dan valt hij alsnog.
				UE_LOG(LogEclipse, Warning,
					TEXT("Insertie: geen vloer onder Entry_Main — terug op de vaste marge van 100 cm, de speler valt bij de start (GDD 14.3.5)."));
			}
			ControlledPawn->SetActorLocation(Target);
			break;
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
	SprintToggleAction = MakeAction(EInputActionValueType::Boolean);
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
	// L3 hangt aan de TOGGLE-actie, niet aan de hold-actie waar Shift op zit
	// (feel-audit S2). Zie de device-regel bij ApplySprintDrive.
	MapKey(SprintToggleAction, EKeys::Gamepad_LeftThumbstick);
	MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);
	// R3 is er BEWUST af (owner playtest 2026-07-25: "gaat bij mij constant per
	// ongeluk af, want het is de stick waarmee ik richt"). Dat is geen smaak maar
	// een ontwerpfout van mijn kant: een knop die je perspectief omgooit hoort niet
	// op de stick die je de hele tijd vasthoudt. Elke andere pad-knop draagt al iets
	// (A springen, B hurken, X soldaat-pick, Y stance, LB Command Mode, RB volgende,
	// LT mikken, RT vuren, L3 sprint, View gids, Menu bevestigen), dus er is geen
	// vrije plek — en een view-swap is geen gevechtsactie, dus hij verliest die
	// afweging terecht. Toetsenbord houdt C.

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
	// Loslaten van de bewegingsstick is een EIGEN gebeurtenis en moet dat zijn:
	// zodra de speler niets meer duwt, wordt HandleMove niet meer aangeroepen, dus
	// zonder deze regel zou de sprint-toggle blijven hangen op de laatste waarde
	// die hij nog zag (feel-audit S2, uitstap "ophouden met vooruit duwen").
	Input->BindActionValueLambda(MoveAction, ETriggerEvent::Completed, [this](const FInputActionValue&)
	{
		CancelSprintLatch(TEXT("bewegingsstick losgelaten"));
		// En stoppen met meedraaien: vanaf hier is elke camerabeweging puur kijken.
		if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
		{
			Body->SetOrientationFollowsCameraNow(false);
		}
	});
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleLook);
	Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleFire);
	Input->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleSprintHold);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEclipsePlayerController::HandleSprintHold);
	Input->BindAction(SprintToggleAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleSprintToggle);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleCrouch);
	Input->BindAction(ToggleViewAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleToggleView);

	// Eén logregel per actie-naam, de EERSTE keer dat hij vanaf een gamepad
	// actueert. Dat scheidt de drie mogelijke oorzaken van "geen enkele knop doet
	// iets": komt er geen XInput binnen (dan blijft dit stil), bereikt het de
	// mapping niet (ook stil, maar de apparaatregel hierboven zegt dan wel
	// "Gamepad = JA"), of komt het wél aan en doet de handler niets (dan staat er
	// een regel en ligt het aan de handler). Zonder dit onderscheid is elke
	// diagnose een gok.
	{
		const TPair<UInputAction*, const TCHAR*> Watched[] = {
			{ MoveAction, TEXT("Move") },       { LookAction, TEXT("Look") },
			{ FireAction, TEXT("Fire") },       { SprintAction, TEXT("SprintHold") },
			{ SprintToggleAction, TEXT("SprintToggle") },
			{ CrouchAction, TEXT("Crouch") },   { ToggleViewAction, TEXT("ToggleView") },
			{ JumpAction, TEXT("Jump") },       { AimAction, TEXT("Aim") },
			{ CommandHoldAction, TEXT("CommandHold") },
			{ SelectNextAction, TEXT("SelectNext") }, { SelectPrevAction, TEXT("SelectPrev") },
			{ DirectPickAction, TEXT("DirectPick") }, { StanceToggleAction, TEXT("StanceToggle") },
		};
		for (const TPair<UInputAction*, const TCHAR*>& Row : Watched)
		{
			if (Row.Key == nullptr)
			{
				continue;
			}
			const FString ActionName = Row.Value;
			Input->BindActionValueLambda(Row.Key, ETriggerEvent::Started, [this, ActionName](const FInputActionValue&)
			{
				if (!IsUsingGamepadLook() || SeenGamepadActions.Contains(ActionName))
				{
					return;
				}
				SeenGamepadActions.Add(ActionName);
				UE_LOG(LogEclipse, Display, TEXT("Input: gamepad bereikte actie '%s' (eerste keer)."), *ActionName);
			});
		}
	}

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
	// GEEN DODE KNOPPEN (owner-opdracht 26-07, punt 4).
	//
	// RB deed buiten Command Mode niets. Nu wisselt hij daar het camerastandpunt —
	// en dat vult een echt gat: sinds R3 eraf ging (te vaak per ongeluk) kón de
	// controller helemaal niet meer wisselen tussen 1e en 3e persoon, terwijl het
	// toetsenbord daar C voor heeft. Binnen de hold blijft hij "volgende soldaat".
	//
	// Waarom niet "wapen wisselen", jouw eerdere wens: er IS geen tweede wapen.
	// Die knop zou dan opnieuw niets doen, alleen met een mooiere naam — precies
	// wat deze opdracht wil uitroeien. Zodra er een loadout-wissel bestaat is dit
	// de plek.
	Input->BindActionValueLambda(SelectNextAction, ETriggerEvent::Started, [this](const FInputActionValue&)
	{
		const bool bHeld = CommandMode != nullptr && CommandMode->IsHeld();
		if (bHeld)
		{
			CommandMode->CycleSoldierSelection(+1);
			return;
		}
		HandleToggleView();
	});

	// HET LT-CONFLICT, opgelost zoals andere games het oplossen: door het niet te
	// hebben. LT was buiten de modus mikken en erbinnen "vorige soldaat", en zo'n
	// overlading op een TRIGGER is in dit genre ongebruikelijk — Division en Gears
	// houden de triggers heilig (mikken en vuren) en zetten moduskeuzes op de
	// bumpers en het d-pad. Daar is een goede reden voor: een trigger heeft een
	// analoge slag, dus je "drukt" hem half per ongeluk.
	//
	// Selectie cycelt daarom nog maar één kant op, met RB, en wrapt rond. Dat is
	// bij vier soldaten geen verlies (drie keer RB is hetzelfde als één keer
	// terug) en het maakt LT weer één ding: mikken, altijd. De muiswiel-tak
	// hieronder blijft beide richtingen doen, want een wiel is niet dubbelzinnig.
	Input->BindActionValueLambda(SelectPrevAction, ETriggerEvent::Started, [this](const FInputActionValue&)
	{
		if (CommandMode != nullptr && CommandMode->IsHeld()) { CommandMode->CycleSoldierSelection(-1); }
	});
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleJump);
	Input->BindAction(AimAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleAimStart);
	Input->BindAction(AimAction, ETriggerEvent::Completed, this, &AEclipsePlayerController::HandleAimStop);
	// X/E: binnen de hold "soldaat onder het kruis", daarbuiten een SNELLE
	// hergroepeer-order aan de hele squad. Genre-conventie (Mass Effect, Ghost
	// Recon, Division): één face-knop geeft de meest gevraagde order zonder dat je
	// een menu in hoeft. Het gebruikt het bestaande orderpad, dus het is echt
	// gedrag en geen decoratie — je squad komt naar je toe en antwoordt hoorbaar.
	Input->BindActionValueLambda(DirectPickAction, ETriggerEvent::Started, [this](const FInputActionValue&)
	{
		const bool bHeld = CommandMode != nullptr && CommandMode->IsHeld();
		if (bHeld)
		{
			CommandMode->PickSoldierUnderReticle();
			return;
		}
		IssueSquadOrder(EEclipseSquadOrder::Regroup);
	});
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

		// Ook de debug-acties krijgen een eerste-actuatie-regel. De owner meldde dat
		// twaalf gameplay-acties de gamepad bereikten maar de gids op de View-knop
		// niet in het log stond - en zonder deze regel is "niet geraakt" niet te
		// onderscheiden van "binding komt niet aan". CommonUI draait hier zonder
		// afgeleide viewport client (waarschuwing bij boot), en View/Menu zijn
		// precies de knoppen die een UI-laag pleegt op te eten.
		if (DebugBindings[Index].PadKey.IsValid())
		{
			const FString DebugName = FString::Printf(TEXT("debug:%s"), *DebugBindings[Index].PadKey.ToString());
			Input->BindActionValueLambda(DebugOverlayActions[Index], ETriggerEvent::Started, [this, DebugName](const FInputActionValue&)
			{
				if (!SeenGamepadActions.Contains(DebugName))
				{
					SeenGamepadActions.Add(DebugName);
					UE_LOG(LogEclipse, Display, TEXT("Input: gamepad bereikte actie '%s' (eerste keer)."), *DebugName);
				}
			});
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

	// F9 — de feel-dump als ECHTE binding (S3). De owner bond hem via
	// +DebugExecBindings in Saved/Config/WindowsEditor/Input.ini, en er kwam nooit
	// een regel in het log. Saved/Config is de GEGENEREERDE configlaag: de engine
	// bezit hem en schrijft hem bij afsluiten terug, dus een handmatig toegevoegde
	// regel overleeft daar niet betrouwbaar. De duurzame plekken zijn (a) een
	// binding in code — deze — die geen enkele configlaag nodig heeft, en (b)
	// Eclipse/Config/DefaultInput.ini, die in de repo staat. Beide zijn er nu; de
	// console-route Eclipse.Feel.Dump blijft ook gewoon werken.
	FeelDumpAction = MakeAction(EInputActionValueType::Boolean);
	MapKey(FeelDumpAction, EKeys::F9);
	Input->BindActionValueLambda(FeelDumpAction, ETriggerEvent::Started, [this](const FInputActionValue&) { DumpFeelState(); });

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
		/** Deze actie DOET alleen iets terwijl Command Mode vastgehouden wordt. */
		bool bNeedsCommandMode = false;
		/** ...en deze juist alleen DAARBUITEN (LT deelt zich met "vorige soldaat"). */
		bool bBlockedByCommandMode = false;
	};
	const FGuideSignalBinding GuideSignals[] = {
		{ MoveAction,          EclipseTestGuide::EEclipseGuideSignal::Move },
		{ LookAction,          EclipseTestGuide::EEclipseGuideSignal::Look },
		{ FireAction,          EclipseTestGuide::EEclipseGuideSignal::Fire },
		{ SprintAction,        EclipseTestGuide::EEclipseGuideSignal::Sprint },
		{ SprintToggleAction,  EclipseTestGuide::EEclipseGuideSignal::Sprint },
		{ CrouchAction,        EclipseTestGuide::EEclipseGuideSignal::Crouch },
		{ JumpAction,          EclipseTestGuide::EEclipseGuideSignal::Jump },
		// LT is mikken in het veld en "vorige soldaat" tijdens de hold; HandleAimStart
		// keert terug als de modus vast staat. Zonder deze vlag vinkt de gids "Mikken"
		// af op een druk die in werkelijkheid de SELECTIE verzette — hij zou dus een
		// andere handeling bevestigen dan de stap beschrijft.
		{ AimAction,           EclipseTestGuide::EEclipseGuideSignal::Aim,         false, true },
		// ToggleView zit niet meer in deze tabel: HandleToggleView meldt het zelf,
		// zodat ELKE knop die het standpunt wisselt de stap afvinkt (26-07).
		{ CommandHoldAction,   EclipseTestGuide::EEclipseGuideSignal::CommandMode },
		{ SelectNextAction,    EclipseTestGuide::EEclipseGuideSignal::SelectNext,  true },
		{ SelectPrevAction,    EclipseTestGuide::EEclipseGuideSignal::SelectPrev,  true },
		{ DirectPickAction,    EclipseTestGuide::EEclipseGuideSignal::DirectPick,  true },
		{ StanceToggleAction,  EclipseTestGuide::EEclipseGuideSignal::Stance,      true },
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
		const bool bNeedsCommandMode = Binding.bNeedsCommandMode;
		const bool bBlockedByCommandMode = Binding.bBlockedByCommandMode;
		Input->BindActionValueLambda(Binding.Action, ETriggerEvent::Started,
			[this, Signal, bNeedsCommandMode, bBlockedByCommandMode](const FInputActionValue&)
		{
			// De gids vinkte af op de TOETSDRUK, niet op het effect. Vier van deze
			// acties doen buiten Command Mode niets (CycleSoldierSelection,
			// PickSoldierUnderReticle en ToggleHeldStance keren meteen terug), dus
			// RB indrukken in het veld zette de stap op "gehaald" terwijl er op het
			// scherm niets gebeurde — en de stap beschrijft juist wat je zou moeten
			// zien ("de regel 'target:' springt van ALL naar een soldaat-id").
			//
			// Een gids die zichzelf afvinkt terwijl het beschreven effect uitblijft,
			// is erger dan geen detectie: hij spreekt de speler tegen.
			const bool bCommandHeld = CommandMode != nullptr && CommandMode->IsHeld();
			if (bNeedsCommandMode && !bCommandHeld)
			{
				return;
			}
			if (bBlockedByCommandMode && bCommandHeld)
			{
				return; // de druk deed iets anders dan deze stap beschrijft
			}
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

	// Sprint-toggle-uitstap 1 van 4: ophouden met VOORUIT duwen (feel-audit S2).
	// Op de voorwaartse component, niet op de totale uitslag: zijwaarts sturen
	// tijdens een sprint moet mogen, achteruit of stilstaan beëindigt hem.
	if (bSprintLatched && Axis.Y < SprintForwardRelease)
	{
		CancelSprintLatch(TEXT("niet meer vooruit"));
	}

	// Het lichaam mag meedraaien met de camera zolang er invoer is (26-07, punt 8).
	// Zonder deze poort zou stilstaand ronddraaien voetslip geven: er zit geen
	// draai-animatie in de packs.
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(ControlledPawn))
	{
		Body->SetOrientationFollowsCameraNow(true);
	}

	const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

UInputAction* AEclipsePlayerController::FindInputAction(FName ActionName) const
{
	// Eén tabel, zodat een nieuwe actie die het harnas moet kunnen aansturen hier
	// zichtbaar ontbreekt in plaats van stil.
	static const TMap<FName, int32> Index = {
		{ TEXT("Move"), 0 }, { TEXT("Look"), 1 }, { TEXT("Fire"), 2 },
		{ TEXT("SprintHold"), 3 }, { TEXT("SprintToggle"), 4 }, { TEXT("Crouch"), 5 },
		{ TEXT("ToggleView"), 6 }, { TEXT("Jump"), 7 }, { TEXT("Aim"), 8 },
		{ TEXT("CommandHold"), 9 }, { TEXT("SelectNext"), 10 }, { TEXT("SelectPrev"), 11 },
		{ TEXT("DirectPick"), 12 }, { TEXT("StanceToggle"), 13 },
		{ TEXT("Order1"), 14 }, { TEXT("Order2"), 15 }, { TEXT("Order3"), 16 }, { TEXT("Order4"), 17 },
	};
	const int32* Slot = Index.Find(ActionName);
	if (Slot == nullptr)
	{
		return nullptr;
	}
	switch (*Slot)
	{
	case 0:  return MoveAction;
	case 1:  return LookAction;
	case 2:  return FireAction;
	case 3:  return SprintAction;
	case 4:  return SprintToggleAction;
	case 5:  return CrouchAction;
	case 6:  return ToggleViewAction;
	case 7:  return JumpAction;
	case 8:  return AimAction;
	case 9:  return CommandHoldAction;
	case 10: return SelectNextAction;
	case 11: return SelectPrevAction;
	case 12: return DirectPickAction;
	case 13: return StanceToggleAction;
	default: break;
	}
	const int32 OrderIndex = *Slot - 14;
	return OrderActions.IsValidIndex(OrderIndex) ? OrderActions[OrderIndex].Get() : nullptr;
}

void AEclipsePlayerController::HandleLook(const FInputActionValue& Value)
{
	// Ver genoeg wegkijken draait het lichaam alsnog mee (locomotie-audit 26-07).
	//
	// Sinds het camera-relatieve model volgt het lichaam de camera alleen tijdens
	// bewegingsinvoer — dat voorkomt voetslip bij elk klein kijkbeweginkje. Maar
	// zonder bovengrens kun je stilstaand 180 graden van je eigen lichaam
	// wegkijken, en dat is erger dan de slip die het moest vermijden: je personage
	// staat dan letterlijk over zijn schouder te kijken.
	//
	// 90 graden is waar de referentie het ook legt. Gears en The Division draaien
	// het lichaam bij ongeveer een kwartslag na, met een draai-animatie erbij. Die
	// animatie zit niet in de packs (nagekeken: geen enkele Turn_*-take), dus de
	// voeten schuiven een fractie. Dat gebeurt alleen bij een kwartslag of meer,
	// en nooit tijdens het kleine corrigeren waar dit model voor gemaakt is.
	// De draai-animatie staat als owner-item; zodra hij er is kan de drempel omlaag.
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
	{
		const float BodyYaw = static_cast<float>(Body->GetActorRotation().Yaw);
		const float ViewYaw = static_cast<float>(GetControlRotation().Yaw);
		const float Apart = FMath::Abs(FMath::FindDeltaAngleDegrees(BodyYaw, ViewYaw));
		if (Apart > IdleTurnThresholdDegrees)
		{
			Body->SetOrientationFollowsCameraNow(true);
		}
	}

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
		// Aiming slows the look down. Without this, narrowing the FOV made the
		// reticle sweep the screen faster while aiming than while hip-firing.
		const float AdsScale = (bAiming ? AdsLookMultiplier : 1.0f) * ComputeAimAssistScale();
		// Degrees per second, so stick look is framerate-independent — unlike the
		// mouse path, which is already per-event.
		AddYawInput(Direction.X * Curved * StickYawSpeed * AdsScale * DeltaSeconds);
		// Teken POSITIEF sinds bEnableLegacyInputScales uit staat: die legacy-schaal
		// was -2.5, dus de min hier compenseerde een verborgen omkering. Netto
		// richting blijft exact gelijk aan wat er verscheept werd; alleen de factor
		// 2.5 is weg en de tuning betekent nu wat er staat.
		AddPitchInput(DampPitchNearLimit(Direction.Y * Curved * StickPitchSpeed * AdsScale * DeltaSeconds * InvertY));
		return;
	}

	// The mouse gets the same ADS slowdown but no curve and no deadzone: it has no
	// rest position and no drift, so there is nothing to filter, and curving it
	// would break the muscle memory a player built in every other game. Valve and
	// id both ship raw with acceleration off for exactly this reason.
	const float MouseAds = bAiming ? AdsLookMultiplier : 1.0f;
	AddYawInput(Axis.X * MouseLookScale * MouseAds);
	// Zelfde tekenwissel als op de stick-tak, om dezelfde reden (legacy-pitchschaal
	// was -2.5). MouseLookScale staat op 2.5 zodat de muis exact even snel blijft
	// als voorheen — bij de muis is het getal een kale schaal zonder eenheid, dus
	// daar is de eerlijke keuze "gedrag ongewijzigd", niet "getal mooier".
	AddPitchInput(DampPitchNearLimit(Axis.Y * MouseLookScale * MouseAds * InvertY));
}

bool AEclipsePlayerController::IsUsingGamepadLook() const
{
	// Expliciete override wint (harnas + owner-A/B). -1 = niets forceren.
	const int32 Forced = CVarEclipseForceGamepad.GetValueOnGameThread();
	if (Forced >= 0)
	{
		return Forced > 0;
	}

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

	// De gids moet dit ZIEN, ongeacht welke knop het deed (26-07).
	//
	// Het signaal hing aan de ToggleView-ACTIE, en sinds RB buiten Command Mode
	// ook het standpunt wisselt kwam die weg langs de gids heen: je drukte RB, de
	// camera verspringt, en de stap bleef onafgevinkt. Dat is precies de stille
	// mismatch die deze gids moet uitroeien, en ik heb hem vanochtend zelf
	// gemaakt.
	//
	// Hier en niet in de bindingtabel, want dit is de plek waar het FEIT gebeurt.
	// Elke toekomstige knop die hier langskomt vinkt de stap vanzelf af.
	if (MissionHud != nullptr && MissionHud->IsInViewport())
	{
		MissionHud->NoteGuideSignal(EclipseTestGuide::EEclipseGuideSignal::ToggleView);
	}
	UE_LOG(LogEclipse, Verbose, TEXT("View toggled to %s person."),
		Body->IsFirstPerson() ? TEXT("first") : TEXT("third"));
}

void AEclipsePlayerController::HandleFire()
{
	// Sprint-toggle-uitstap 3 van 4: vuren. Vóór de wapen-check, want het is de
	// INTENTIE om te vuren die de sprint beëindigt — een pawn zonder wapen is een
	// placeholder-toestand, geen ontwerptoestand. Idempotent, dus dat Fire aan
	// Triggered hangt (elk frame terwijl je de trekker vasthoudt) kost niets.
	CancelSprintLatch(TEXT("vuren"));

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

void AEclipsePlayerController::HandleSprintHold(const FInputActionValue& Value)
{
	// Toetsenbord (Shift). Hold is daar de conventie: een pinktoets vasthouden
	// kost niets, en elke shooter op muis+toetsenbord doet het zo.
	bSprintHeld = Value.Get<bool>();
	ApplySprintDrive();
}

void AEclipsePlayerController::HandleSprintToggle()
{
	// Controller (L3). Toggle, want de stick waarmee je stuurt ingedrukt HOUDEN
	// terwijl je ermee stuurt is onhandig — en het is niet de conventie
	// (Borderlands / Gears / The Division: één klik start, en hij blijft aan).
	bSprintLatched = !bSprintLatched;
	UE_LOG(LogEclipse, Verbose, TEXT("Sprint: L3 zet de latch %s."), bSprintLatched ? TEXT("AAN") : TEXT("UIT"));
	ApplySprintDrive();
}

void AEclipsePlayerController::CancelSprintLatch(const TCHAR* Reason)
{
	if (!bSprintLatched)
	{
		return;
	}
	bSprintLatched = false;
	UE_LOG(LogEclipse, Verbose, TEXT("Sprint: latch uit (%s)."), Reason);
	ApplySprintDrive();
}

void AEclipsePlayerController::ApplySprintDrive()
{
	// PLACEHOLDER(feel targets §2): sprint switches between the run and sprint
	// speed; stamina only bites above Medium armor and armor lands later.
	//
	// ÉÉN schrijver naar MaxWalkSpeed, en dat is de reden dat deze functie
	// bestaat: met twee bronnen (hold + latch) die allebei zelf schrijven, zet de
	// een de ander stil zodra ze elkaar overlappen. Nu is de uitkomst altijd de
	// OR van de twee en is er geen volgorde-afhankelijkheid.
	bSprinting = bSprintHeld || bSprintLatched;
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
	{
		// Speeds cached from DA_CharacterTuning in ApplyLookTuning (GDD 14.2);
		// the fallbacks mirror the locked feel targets so a missing asset lands
		// on the same numbers (GDD 14.3.5).
		Body->GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeedCm : RunSpeedCm;
	}
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
	// RequestJump en niet Jump: dat pad draagt de twee vergevingsvensters
	// (coyote time en de inputbuffer, JMP-07/08).
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
	{
		Body->RequestJump();
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
	// Sprint-toggle-uitstap 2 van 4: mikken. Je kunt niet tegelijk sprinten en
	// richten, en van de twee wint de intentie die je zojuist uitsprak.
	CancelSprintLatch(TEXT("mikken"));
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

float AEclipsePlayerController::ComputeAimAssistScale() const
{
	if (AimAssistStrength <= KINDA_SMALL_NUMBER)
	{
		return 1.0f; // 0 = off, and off means literally untouched input
	}
	// Never during the Command hold: there the reticle is a SELECTION tool for
	// squadmates and positions, and a camera that gets heavy over a soldier would
	// fight the order you are trying to give.
	if (CommandMode != nullptr && CommandMode->IsHeld())
	{
		return 1.0f;
	}

	const APawn* Self = GetPawn();
	if (Self == nullptr || GetWorld() == nullptr)
	{
		return 1.0f;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector Forward = ViewRotation.Vector();
	const float CosCone = FMath::Cos(FMath::DegreesToRadians(FMath::Max(AimAssistConeDegrees, 0.0f)));

	for (TActorIterator<AEclipseCharacter> It(GetWorld()); It; ++It)
	{
		const AEclipseCharacter* Other = *It;
		// Hostiles only, and never a downed one: help that keeps tracking a body
		// on the floor reads as the game aiming for you.
		if (Other == nullptr || Other == Self || Other->IsPlayerSide() || Other->IsDowned())
		{
			continue;
		}
		const FVector ToTarget = Other->GetActorLocation() - ViewLocation;
		const float Distance = ToTarget.Size();
		if (Distance > AimAssistRange || Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		if (FVector::DotProduct(ToTarget / Distance, Forward) < CosCone)
		{
			continue;
		}
		// Inside the cone: keep AimAssistFloor of the speed at full strength, and
		// scale linearly with strength so 0.5 is genuinely half the help.
		return FMath::Lerp(1.0f, FMath::Clamp(AimAssistFloor, 0.1f, 1.0f), FMath::Clamp(AimAssistStrength, 0.0f, 1.0f));
	}
	return 1.0f;
}

float AEclipsePlayerController::DampPitchNearLimit(float PitchDelta) const
{
	if (PlayerCameraManager == nullptr || PitchDampBandDegrees <= KINDA_SMALL_NUMBER || FMath::IsNearlyZero(PitchDelta))
	{
		return PitchDelta;
	}

	// NormalizeAxis, want een control rotation draagt pitch als 0..360: -10 graden
	// staat er als 350, en rechtstreeks vergelijken met een limiet van -70 geeft
	// dan onzin.
	const float Current = FRotator::NormalizeAxis(GetControlRotation().Pitch);
	const float Limit = PitchDelta > 0.0f ? PlayerCameraManager->ViewPitchMax : PlayerCameraManager->ViewPitchMin;
	const float Remaining = FMath::Abs(Limit - Current);
	if (Remaining >= PitchDampBandDegrees)
	{
		return PitchDelta;
	}
	// Naar een bodem en niet naar nul: bij nul wordt de limiet nooit gehaald.
	return PitchDelta * FMath::Lerp(FMath::Clamp(PitchDampFloor, 0.05f, 1.0f), 1.0f, Remaining / PitchDampBandDegrees);
}

FString AEclipsePlayerController::DescribeLookTuning() const
{
	// Seconds per full turn is the criterion the owner asked to judge against —
	// "yaw 240" means nothing to a hand, "1.5 s om rond te draaien" does.
	const float SecondsPerTurn = StickYawSpeed > KINDA_SMALL_NUMBER ? 360.0f / StickYawSpeed : 0.0f;
	return FString::Printf(
		TEXT("   kijken: %.2f s per 360 (yaw %.0f gr/s, pitch %.0f = %.2fx) · curve x^%.1f · deadzone %.2f · ADS x%.2f · aim-assist %.0f%%"),
		SecondsPerTurn, StickYawSpeed, StickPitchSpeed,
		StickYawSpeed > KINDA_SMALL_NUMBER ? StickPitchSpeed / StickYawSpeed : 0.0f,
		StickResponseExponent, StickDeadzone, AdsLookMultiplier, AimAssistStrength * 100.0f);
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

	// Sprint/run speeds cached here rather than re-read per input event: the hold
	// handler runs on ETriggerEvent::Triggered, i.e. every frame you sprint, and
	// it used to do a LoadSynchronous on the tuning asset each time (GDD 12.4).
	RunSpeedCm = Tuning->RunSpeed;
	SprintSpeedCm = Tuning->SprintSpeed;

	StickYawSpeed = Tuning->StickYawSpeed;
	StickPitchSpeed = Tuning->StickPitchSpeed;
	StickDeadzone = Tuning->StickDeadzone;
	MoveDeadzone = Tuning->MoveDeadzone;
	StickResponseExponent = Tuning->StickResponseExponent;
	MouseLookScale = Tuning->MouseLookScale;
	AdsLookMultiplier = Tuning->AdsLookMultiplier;
	AimAssistStrength = Tuning->AimAssistStrength;
	AimAssistFloor = Tuning->AimAssistFloor;
	AimAssistConeDegrees = Tuning->AimAssistConeDegrees;
	AimAssistRange = Tuning->AimAssistRange;
	PitchDampBandDegrees = Tuning->PitchDampBandDegrees;
	PitchDampFloor = Tuning->PitchDampFloor;
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
