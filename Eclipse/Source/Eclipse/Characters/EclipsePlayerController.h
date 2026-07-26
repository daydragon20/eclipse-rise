#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Characters/EclipseCharacter.h" // FEclipseFeelSample voor de F9-delta
#include "Squad/EclipseSquadTypes.h"
#include "EclipsePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UEclipseBaseHubWidget;
class UEclipseCommandModeComponent;
class UEclipseMissionHudWidget;

/**
 * Player input (SPEC-P1-05/06): Enhanced Input only (GDD 12.1). Move/look/
 * fire/sprint/crouch plus the four order hotkeys — orders go to the squad at
 * the aim point. Command Mode (SPEC-P2-02 Stage A) wraps this path: holding
 * the command input dilates time and routes orders per-soldier through the
 * same IssueOrder contract; releasing restores the Phase 1 baseline exactly.
 *
 * // PLACEHOLDER(GDD 12.1): input objects are built in code until the input
 * // asset pass; bindings stay identical when they move to assets.
 */
UCLASS()
class ECLIPSE_API AEclipsePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AEclipsePlayerController();

	/** Aim point = camera-forward hitscan against world geometry (order target + fire direction + command-mode soldier pick). */
	bool GetAimPoint(FVector& OutLocation, AActor*& OutActor) const;

	/**
	 * Harnas-naad (14.4 laag 2). De headless feel-harnas injecteert op DEZELFDE
	 * UInputAction-objecten die de hardware van de speler aanstuurt — een harnas
	 * dat zijn eigen acties bouwt bewijst niets over de verscheepte bindings.
	 * Namen: Move, Look, Fire, SprintHold, SprintToggle, Crouch, ToggleView,
	 * Jump, Aim, CommandHold, SelectNext, SelectPrev, DirectPick, StanceToggle,
	 * Order1..Order4. Onbekende naam = nullptr, nooit een crash.
	 */
	UInputAction* FindInputAction(FName ActionName) const;
	UInputMappingContext* GetMappingContext() const { return MappingContext; }

	/**
	 * Sprint-toestand (feel-audit S2). Twee apparaten, twee gebruiken: Shift is
	 * een HOLD, L3 is een TOGGLE. De toggle vervalt zodra de speler ophoudt met
	 * vooruit duwen, mikt, vuurt, of nogmaals L3 drukt — Borderlands / Gears /
	 * The Division doen het alle drie zo.
	 */
	bool IsSprinting() const { return bSprinting; }
	bool IsSprintLatched() const { return bSprintLatched; }

	/**
	 * TERUGSLAG (owner-opdracht 26-07 avond, punt 4). Het wapen duwt je kruis
	 * omhoog; de stabiliteit uit DT_Weapons bepaalt hoe snel het terugzakt.
	 * Aangeroepen door het wapencomponent, want alleen dat weet welk profiel er
	 * vuurt.
	 */
	void AddRecoil(float PitchDegrees, float YawDegrees, float RecoveryDegreesPerSecond);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaSeconds) override;


private:
	/** Herladen als er iets te herladen valt; false laat de knop doorvallen. */
	bool TryReload();

	/** Wisselen als er een tweede wapen is; false laat de knop doorvallen. */
	bool TrySwapWeapon();

	void HandleMove(const struct FInputActionValue& Value);
	void HandleLook(const struct FInputActionValue& Value);
	void HandleFire();
	/** Shift: klassieke hold. Zolang de toets neer is, sprint je. */
	void HandleSprintHold(const struct FInputActionValue& Value);
	/** L3: één klik zet de latch om. Zie CancelSprintLatch voor het uitstappen. */
	void HandleSprintToggle();
	/** Eén schrijver naar MaxWalkSpeed: hold OF latch = sprint, anders rennen. */
	void ApplySprintDrive();
	/** De vier uitstappen van de toggle (vooruit loslaten, mikken, vuren, L3). */
	void CancelSprintLatch(const TCHAR* Reason);
	void HandleCrouch();
	/** C / R3: swap first and third person on the pawn (owner request 2026-07-25). */
	void HandleToggleView();
	/** Space / A — did not exist before the 2026-07-25 controller-parity pass. */
	void HandleJump();
	/** RMB / LT. LT only aims outside the Command hold; inside it, it cycles. */
	void HandleAimStart();
	void HandleAimStop();
	void SetAiming(bool bNewAiming);
	/** Which device the look axis should be read as — mouse raw, stick curved. */
	bool IsUsingGamepadLook() const;
	/** Push the tuning asset's look/pitch numbers onto this controller. */
	void ApplyLookTuning();

	/** De feel-meting naar log én scherm. Eén implementatie voor F9, voor
	 *  Eclipse.Feel.Dump en voor het harnas (S3). */
	void DumpFeelState() const;
	/** Vorige F9-meting, zodat de dump het VERSCHIL kan tonen (S1 is een verschil,
	 *  geen absoluut getal — zie DumpFeelState). Mutable: de dump is const. */
	mutable FEclipseFeelSample PreviousFeelSample;
	mutable bool bHasPreviousFeelSample = false;

	/** Navmesh-stand naar het log. Wordt TWEE keer aangeroepen — bij missiestart
	 *  en vijf seconden later — omdat Recast asynchroon bouwt en een meting op
	 *  t=0 dus altijd "nee" zegt, ongeacht of het goed komt. */
	void LogNavigationState(const TCHAR* When) const;
	/** 1.0 = untouched. Below 1.0 while the reticle sits on a hostile. */
	float ComputeAimAssistScale() const;

	/** Rem het kijken af vlak vóór de pitch-limiet (CAM-07b, Nesky #47). Werkt op
	 *  de invoer, niet op de klem: klemmen doet de camera manager al, maar die
	 *  laat je er met volle snelheid tegenaan lopen. */
	float DampPitchNearLimit(float PitchDelta) const;

public:
	/** The live look values as one line, for the test guide's header. Reads the
	 *  same members the handler uses, so it cannot advertise a stale number. */
	FString DescribeLookTuning() const;

private:
	void IssueSquadOrder(EEclipseSquadOrder Order);

	/** Boot a fresh campaign from data if none is running (SPEC-P1-08 live loop). */
	void EnsureCampaignStarted();

	/**
	 * Is er een scherm om widgets op te zetten? Headless (harnas, commandlet,
	 * -nullrhi) is er geen game viewport, en dan is een widget bouwen niet alleen
	 * zinloos maar ook de plek waar zo'n run omvalt. De game moet zonder scherm
	 * kunnen draaien — dat is precies wat de geautomatiseerde speelronde doet.
	 */
	bool HasGameViewport() const;

	/** Menu base presentation: show the hub, UI input, pawn parked. */
	void EnterBaseMode();

	/** Ground presentation: hub hidden, game input, mission HUD, inserted at the entry. */
	void EnterMissionMode();

	/** Mission lifecycle drives the base<->mission presentation swap. */
	void OnMissionEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	UPROPERTY()
	TObjectPtr<UEclipseBaseHubWidget> BaseHub;

	UPROPERTY()
	TObjectPtr<UEclipseMissionHudWidget> MissionHud;

	/**
	 * Hoe ver je mag wegkijken voor het lichaam meedraait (locomotie-audit 26-07).
	 * 90 graden, zoals Gears en The Division. Lager kan zodra er een
	 * draai-animatie in de packs zit; nu zouden de voeten te vaak schuiven.
	 */
	static constexpr float IdleTurnThresholdDegrees = 90.0f;

	/**
	 * TERUGSLAG (owner-opdracht 26-07 avond, punt 4). Het wapen duwt je kruis
	 * omhoog; de stabiliteit uit DT_Weapons bepaalt hoe snel het terugzakt.
	 *
	 * Het onverwerkte deel wordt onthouden zodat het TERUG kan: zonder dat is
	 * terugslag alleen straf, en "stabiliteit" een getal zonder betekenis. De
	 * herstelbeweging stopt zodra jij zelf kijkt — anders vecht het spel tegen je
	 * eigen correctie in, en dat is precies waar spelers terugslag om haten.
	 */


	void ApplyRecoil(float DeltaSeconds);

	float PendingRecoilKickPitch = 0.0f;
	float PendingRecoilKickYaw = 0.0f;
	float PendingRecoilPitch = 0.0f;
	float RecoilRecoveryRate = 0.0f;
	bool bLookedThisFrame = false;

	FEclipseEventSubscriptionHandle MissionEventsHandle;

	/** Command Mode wrapper (SPEC-P2-02): dilation + selection; owns nothing of the order path. */
	UPROPERTY(VisibleAnywhere, Category = "Eclipse|Command")
	TObjectPtr<UEclipseCommandModeComponent> CommandMode;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> FireAction;

	/** Toetsenbord-sprint (Shift): hold. */
	UPROPERTY()
	TObjectPtr<UInputAction> SprintAction;

	/**
	 * Pad-sprint (L3): toggle. Een EIGEN actie, geen tweede key op de bestaande —
	 * anders is bij de handler niet te zien welk apparaat er drukte, en juist dat
	 * onderscheid IS de fix (feel-audit S2, gebied INPUT).
	 */
	UPROPERTY()
	TObjectPtr<UInputAction> SprintToggleAction;

	UPROPERTY()
	TObjectPtr<UInputAction> CrouchAction;

	/** One action on the EXISTING contract, not a new mode: SPEC-P2-07 owns the
	 *  Enhanced Input context stacks and this must not pre-empt it. */
	UPROPERTY()
	TObjectPtr<UInputAction> ToggleViewAction;

	UPROPERTY()
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY()
	TObjectPtr<UInputAction> AimAction;

	bool bAiming = false;

	// Sprint (feel-audit S2). Twee bronnen, één uitkomst: bSprinting is altijd
	// (hold || latch), zodat er precies één schrijver naar MaxWalkSpeed is.
	bool bSprinting = false;
	bool bSprintHeld = false;
	bool bSprintLatched = false;
	/** Voorwaartse stickcomponent waaronder de toggle vervalt ("ophouden met
	 *  vooruit duwen"). Ruim onder half stick, zodat sturen tijdens sprinten mag. */
	float SprintForwardRelease = 0.35f;
	/** Snelheden gecachet uit DA_CharacterTuning. Ze werden per input-event uit
	 *  het asset geladen, en HandleSprint hing aan Triggered — dus een
	 *  LoadSynchronous per frame zolang je sprintte (GDD 12.4). */
	float RunSpeedCm = 420.0f;
	float SprintSpeedCm = 650.0f;

	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> OrderActions;

	// Look feel, pushed from DA_CharacterTuning at possession (GDD 14.2 — feel
	// numbers are data). Defaults mirror the asset so an untuned boot still aims.
	float StickYawSpeed = 240.0f;
	float StickPitchSpeed = 180.0f;
	float StickDeadzone = 0.08f;
	float MoveDeadzone = 0.08f;
	float StickResponseExponent = 2.0f;
	float MouseLookScale = 1.0f;
	float AdsLookMultiplier = 0.35f;
	float AimAssistStrength = 0.6f;
	float AimAssistFloor = 0.45f;
	float AimAssistConeDegrees = 4.0f;
	float AimAssistRange = 5000.0f;
	float PitchDampBandDegrees = 10.0f;
	float PitchDampFloor = 0.15f;
	bool bInvertLookY = false;

	/** Welke acties al een gamepad-actuatie hebben gelogd — één regel per actie,
	 *  zodat het log een diagnose is en geen spam. */
	TSet<FString> SeenGamepadActions;

	/** Eclipse.Feel.Dump — losgekoppeld in EndPlay, want de console overleeft ons. */
	IConsoleCommand* FeelDumpCommand = nullptr;

	// Command Mode inputs (SPEC-P2-02 Stage A; provisional debug bindings —
	// the Enhanced Input context stack is a SPEC-P2-07 seam).
	UPROPERTY()
	TObjectPtr<UInputAction> CommandHoldAction;

	UPROPERTY()
	TObjectPtr<UInputAction> SelectNextAction;

	UPROPERTY()
	TObjectPtr<UInputAction> SelectPrevAction;

	UPROPERTY()
	TObjectPtr<UInputAction> DirectPickAction;

	UPROPERTY()
	TObjectPtr<UInputAction> StanceToggleAction;

	/**
	 * Debug-overlay bindings (F2/F3/H/F4-F8, J/N and the playtest row keys), built
	 * from one table in SetupInputComponent. They only ever call into the mission
	 * HUD; with no HUD mounted (base mode, -EclipseShot) they do nothing at all,
	 * and the panels themselves ignore their keys while closed.
	 *
	 * The test guide's *detection* needs no entry here: it binds a second
	 * ETriggerEvent::Started delegate onto the gameplay actions that already
	 * exist, so it observes without owning, consuming or remapping anything
	 * (phase0/INGAME_TESTGIDS.md, variant A).
	 */
	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> DebugOverlayActions;

	/** F9 — feel-dump zonder configlaag (S3). Staat los van DebugOverlayActions
	 *  omdat die allemaal de mission-HUD aanroepen en deze de pawn leest. */
	UPROPERTY()
	TObjectPtr<UInputAction> FeelDumpAction;
};
