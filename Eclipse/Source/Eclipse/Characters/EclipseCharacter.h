#pragma once

#include "AbilitySystemInterface.h"
#include "Characters/EclipseLocomotionTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EclipseCharacter.generated.h"

class AEclipseCharacter;
class UAbilitySystemComponent;
class UCameraComponent;
class UEclipseCharacterTuningAsset;
class UEclipseHealthAttributeSet;
class USkeletalMesh;
class USkeletalMeshComponent;
class USpringArmComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FEclipseCharacterDownedDelegate, AEclipseCharacter* /*Character*/, FName /*Cause*/);

/**
 * One instant of "how big is my character on screen, and why" (feel-audit S1).
 *
 * The owner's symptom — "mijn personage schaalt met snelheid" — has exactly four
 * candidate causes and they cannot be told apart by looking: the mesh scale, the
 * boom length, the FOV, or the distance the camera actually ends up at after lag
 * and collision have had their say. Only the LAST of those is speed-coupled by
 * construction (camera lag trails the pawn by v / CameraLagSpeed), which is why
 * CameraToPawnCm is measured from the two world transforms rather than derived
 * from TargetArmLength — a derived number would have hidden the very term that
 * moves.
 *
 * ApparentHeightDegrees is the honest answer to "bigger or smaller": the angle
 * the capsule subtends at the camera. It is resolution-independent, so it can be
 * measured headless, and it is what the eye actually reads.
 */
struct FEclipseFeelSample
{
	float SpeedCm = 0.0f;
	float MeshScale = 0.0f;
	/** What the spring arm was asked for (post-blend). */
	float BoomArmLength = 0.0f;
	/** Where the blend is heading — differs from BoomArmLength mid-blend only. */
	float BoomTargetArmLength = 0.0f;
	/** Measured camera-to-pawn distance: arm length PLUS lag PLUS collision pull-in. */
	float CameraToPawnCm = 0.0f;
	float FieldOfView = 0.0f;
	float TargetFieldOfView = 0.0f;
	float SocketOffsetZ = 0.0f;
	float CapsuleHalfHeightCm = 0.0f;
	/** Angle the full capsule subtends at the camera — "how big it looks". */
	float ApparentHeightDegrees = 0.0f;
	/** That angle as a fraction of the FOV: 0.25 = a quarter of the screen. */
	float ApparentFractionOfView = 0.0f;
};

/**
 * The one character body (GDD 12.3: player, soldiers and enemies share
 * AEclipseCharacter — soldiers must be player-quality; behavior differs by
 * controller and components, never by a divergent class). GAS carries health
 * from day one (SPEC-P1-05).
 */
UCLASS()
class ECLIPSE_API AEclipseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEclipseCharacter(const FObjectInitializer& ObjectInitializer);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	virtual void PostInitializeComponents() override;

	/** Apply tuning (movement speeds, max health) from data — never hardcode (GDD 14.2). */
	void ApplyTuning(const UEclipseCharacterTuningAsset* Tuning);

	/**
	 * Dress this body from a DT_BodyDefs row (step-2 character pipeline): mesh,
	 * toon restyle and locomotion clips. Missing assets degrade to the capsule
	 * silently-but-logged (GDD 14.3.5).
	 */
	void ApplyBodyDef(const struct FEclipseBodyDefRow& BodyDef);

	/**
	 * The clips this body resolved, for its anim instance to pull on init. The
	 * character owns them, not the anim instance: the mesh re-creates its anim
	 * instance on every SetSkeletalMesh and on LOD changes, and the body must
	 * still know what it can play afterwards.
	 */
	const FEclipseLocomotionSet& GetLocomotionSet() const { return LocomotionSet; }

	/** Direct health init for archetype-driven spawns (enemies from DT_EnemyArchetypes). */
	void InitializeHealth(float MaxHealth);

	/**
	 * Reset a persistent body for a fresh run: full health, standing, mobile.
	 * The player pawn survives between missions, so a lost run would otherwise
	 * leave it downed with movement disabled forever (the mission spawn calls this).
	 */
	void ReviveForMission();

	/**
	 * Hitscan damage entry point (SPEC-P1-05 minimal combat): routes through the
	 * GAS meta attribute so future mitigation composes.
	 */
	void ApplyDamage(float Amount, AEclipseCharacter* Instigator, FName Cause);

	bool IsDowned() const { return bDowned; }
	float GetHealth() const;

	/** Campaign identity for roster soldiers (invalid for enemies/player in Phase 1). */
	FGuid GetSoldierId() const { return SoldierId; }
	void SetSoldierId(const FGuid& InSoldierId) { SoldierId = InSoldierId; }

	/**
	 * Phase 1 friend/foe — one source of truth (enemy AI, squad cover scorer,
	 * objective triggers): the player and roster squadmates are "player side"
	 * (player-controlled or carrying a soldier id); everything else is hostile.
	 */
	bool IsPlayerSide() const { return SoldierId.IsValid() || IsPlayerControlled(); }

	/** Fired once when health reaches zero; mission/squad wiring listens (SPEC-P1-06/07 pipeline). */
	FEclipseCharacterDownedDelegate OnDowned;

	/**
	 * First/third-person toggle (owner request 2026-07-25, C on keyboard / R3 on
	 * the pad). Blends the boom and the FOV over ViewToggleBlendTime rather than
	 * cutting — a hard swap between 300 and 0 units is nauseating. In first person
	 * the own mesh is hidden so the player is not inside their own skull.
	 */
	void SetFirstPerson(bool bNewFirstPerson);
	bool IsFirstPerson() const { return bFirstPerson; }

	/**
	 * Command Mode framing: pull back and look down while the hold lasts, so the
	 * player reads the field. Not a mode — SPEC-P2-07 owns the input contexts —
	 * just the same two numbers going somewhere else for the duration.
	 */
	void SetCommandModeCamera(bool bActive);

	/**
	 * Debug-grade ADS (GDD 14.5): pull the boom in and narrow the FOV so aiming
	 * reads. No spread model and no accuracy change — that is the combat feel
	 * pass, not this.
	 */
	void SetAiming(bool bNewAiming);

	UCameraComponent* GetViewCamera() const { return ViewCamera; }

	/** Feel-audit-instrument: snelheid, mesh-schaal, boom, FOV en modus op één
	 *  regel, zodat "het personage schaalt met snelheid" meetbaar wordt in plaats
	 *  van geraden. Uitgelezen door Eclipse.Feel.Dump. */
	FString DescribeFeelState() const;

	/** Dezelfde meting als DescribeFeelState, maar als getallen — het harnas moet
	 *  erop kunnen asserten en niet op een tekstregel hoeven te parsen. */
	FEclipseFeelSample SampleFeelState() const;

	/** Camera-lag uitzetten zolang dit true is (feel-audit S1 / CAM-04). De
	 *  spring arm laat zijn oorsprong achterlopen op de pawn, en die achterstand
	 *  is per definitie evenredig met de snelheid — het is de enige term in de
	 *  hele camerarig die MET de snelheid meebeweegt. */
	void SetCameraLagSuspended(bool bSuspended);
	bool IsCameraLagSuspended() const { return bCameraLagSuspended; }

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * De sprongknop, met de twee vergevingsvensters eromheen (JMP-07/08). De
	 * controller roept DIT aan en niet ACharacter::Jump, want een druk die net te
	 * laat of net te vroeg komt hoort niet verloren te gaan.
	 */
	void RequestJump();

	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode = 0) override;
	virtual bool CanJumpInternal_Implementation() const override;

private:
	void HandleHealthChanged(const struct FOnAttributeChangeData& Data);

	/** Animation half of ApplyBodyDef: resolve the row's takes against the mesh's
	 *  own skeleton and pick a rung of the locomotion ladder (GDD 14.3.5). */
	void ApplyBodyDefAnimation(const struct FEclipseBodyDefRow& BodyDef, const USkeletalMesh* BodyMesh, USkeletalMeshComponent& MeshComponent);

	/** Where the boom/FOV should be right now, given view mode + command mode. */
	void RefreshCameraTargets();
	/** Ticking is switched on only while a blend is running (GDD 12.4: no idle
	 *  per-frame work). This is the one place that decides that. */
	void UpdateCameraBlend(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Eclipse|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Eclipse|Camera")
	TObjectPtr<UCameraComponent> ViewCamera;

	UPROPERTY(VisibleAnywhere, Category = "Eclipse")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UEclipseHealthAttributeSet> HealthAttributes;

	/** Resolved + skeleton-checked locomotion takes for the body currently worn.
	 *  UPROPERTY because it is the only strong reference keeping those clips
	 *  alive; the anim instance's copies are deliberately raw. */
	UPROPERTY(Transient)
	FEclipseLocomotionSet LocomotionSet;

	FGuid SoldierId;
	FName LastDamageCause;
	bool bDowned = false;

	// Camera state. Defaults mirror the tuning asset's defaults so a pawn that
	// never receives tuning still frames correctly instead of sitting at 0.
	bool bFirstPerson = false;
	bool bCommandModeCamera = false;
	bool bAiming = false;
	bool bCameraLagSuspended = false;

	// Sprong-vergevingsvensters (JMP-07/08). Wereldtijd in seconden; < 0 = leeg.
	float CoyoteTimeSeconds = 0.11f;
	float JumpInputBufferSeconds = 0.15f;
	double LeftGroundAtSeconds = -1.0;
	double JumpPressedWhileFallingAtSeconds = -1.0;
	/** Gebufferde sprong die bij het NEERKOMEN nog moet afgaan. Zie Landed(). */
	bool bBufferedJumpPending = false;
	/** Lag speed as tuned, so suspending and restoring is lossless. */
	float TunedCameraLagSpeed = 12.0f;
	float ThirdPersonArmLength = 300.0f;
	float CommandModeArmLength = 520.0f;
	float CommandModeCameraRise = 120.0f;
	/** Socket offset Z as authored, so the Command Mode rise has a zero to
	 *  return to instead of accumulating every time the hold ends. */
	float BaseSocketOffsetZ = 65.0f;
	float FirstPersonFOV = 90.0f;
	float ThirdPersonFOV = 80.0f;
	float ViewToggleBlendTime = 0.2f;
	float TargetArmLength = 300.0f;
	float TargetFOV = 80.0f;
};
