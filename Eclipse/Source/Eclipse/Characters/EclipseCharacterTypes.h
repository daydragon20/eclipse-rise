#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipseCharacterTypes.generated.h"

class UAnimSequence;
class USkeletalMesh;

/**
 * Character data (SPEC-P1-05). The movement numbers are the LOCKED graybox feel
 * targets (phase0/graybox_feel_targets.md §2, GDD 4.1.1) — changing them goes
 * through change management, not through this file's defaults.
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseCharacterTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** cm/s. Feel target: 1.8 m/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float WalkSpeed = 180.0f;

	/** cm/s. Feel target: 4.2 m/s (default gait). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float RunSpeed = 420.0f;

	/** cm/s. Feel target: 6.5 m/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float SprintSpeed = 650.0f;

	/** Feel target: crouch is the stealth default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float CrouchSpeed = 150.0f;

	/**
	 * First-person FOV. This field predates the camera itself: it sat here unused
	 * because AEclipseCharacter had no camera at all and the engine fell back to
	 * the pawn's own view point (owner playtest 2026-07-25 — "ik kijk vanuit het
	 * lichaam of vanaf de voeten"). It now drives the first-person half of the
	 * C toggle, where 90 is the right number; third person wants less.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 30, ClampMax = 140))
	float CameraFOV = 90.0f;

	/** Over-the-shoulder FOV. Narrower than first person so the shoulder framing
	 *  does not fisheye and distant targets keep their pixels (owner spec). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 30, ClampMax = 140))
	float ThirdPersonFOV = 80.0f;

	/** Boom length in third person: hip-to-shoulder distance that still reads at
	 *  command range. 0 is first person — the toggle lerps between the two. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float ThirdPersonArmLength = 300.0f;

	/** Over-the-shoulder offset in BOOM space (X forward, Y right, Z up), so it
	 *  stays over the same shoulder as the camera swings around. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera")
	FVector CameraSocketOffset = FVector(0.0f, 55.0f, 65.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float CameraLagSpeed = 12.0f;

	/** The boom's collision probe: the camera may never pass through a wall. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 1))
	float CameraProbeSize = 12.0f;

	/** Seconds for the first/third-person swap. Hard-cutting is nauseating; this
	 *  is short enough to feel instant and long enough to keep the horizon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float ViewToggleBlendTime = 0.2f;

	/**
	 * Command Mode framing (GDD 14.5 debug-grade): during the hold the boom pulls
	 * back and rises so the player reads the field instead of a shoulder. No new
	 * mode — SPEC-P2-07 owns the input contexts; this only lerps two numbers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float CommandModeArmLength = 520.0f;

	/**
	 * How far the camera RISES during the hold, in units, on top of the boom
	 * pulling back. Deliberately height and not pitch: the boom runs on
	 * bUsePawnControlRotation, so pitch belongs to the player's stick, and a
	 * camera that tilts itself while the player is aiming fights the person
	 * holding the controller. Raising the eye line reads as "overseeing the
	 * field" without taking the aim away.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float CommandModeCameraRise = 120.0f;

	/** Degrees per second at full stick deflection, yaw and pitch separately —
	 *  they are not the same task and never want the same speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0))
	float StickYawSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0))
	float StickPitchSpeed = 180.0f;

	/** Stick drift must never move the camera on its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0, ClampMax = 0.9))
	float StickDeadzone = 0.08f;

	/**
	 * Radial deadzone on the MOVEMENT stick. Its own number, and slightly wider
	 * than the look deadzone, because the two sticks fail differently: a drifting
	 * look stick wobbles the camera and you notice at once, while a drifting move
	 * stick walks the character away AND — with bOrientRotationToMovement — turns
	 * the body with it, so the whole world appears to rotate on its own. The
	 * owner's left stick measures LY = -0.048 at rest.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0, ClampMax = 0.9))
	float MoveDeadzone = 0.08f;

	/**
	 * How much slower looking gets while aiming. This was MISSING, and it is the
	 * likely answer to the owner's "kijken voelt op sommige momenten te scherp":
	 * ADS narrows the FOV to 0.80 but left the look speed alone, so the reticle
	 * swept the screen FASTER while aiming than while hip-firing — the opposite
	 * of what aiming is for.
	 * The physically neutral value is tan(32)/tan(40) = 0.745 (a target then keeps
	 * the same on-screen speed). Every shipped shooter goes deliberately below it:
	 * CoD ships 0.346 for yaw, Apex pros run 0.30-0.40. 0.60 sits between neutral
	 * and their strictness, because our zoom is mild — the band is sourced, this
	 * exact number is a choice inside it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0.1, ClampMax = 1.5))
	float AdsLookMultiplier = 0.35f;

	/**
	 * Response curve exponent for stick look, sign-preserving. A stick is a RATE
	 * controller with about a centimetre of travel that has to cover both a micro
	 * correction and a 180-degree turn; linear spends that centimetre evenly, so
	 * the slow band where aiming happens gets only 20% of the throw. At exponent 2
	 * it gets about 45%.
	 * The SHAPE is documented (Activision names its curves Standard = power curve,
	 * Linear, Dynamic = reverse-S). The NUMBER is not: no publisher prints its
	 * exponent, and an earlier comment here claimed "2.0 is the console-shooter
	 * default" — that is folklore and it has been removed. What is measured:
	 * Halo's older, livelier curve reads as roughly quadratic and Infinite's as
	 * roughly cubic, and Infinite is the one players call sluggish. Hence 2.0,
	 * chosen on that evidence rather than on a number someone repeated.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 1, ClampMax = 4))
	float StickResponseExponent = 2.0f;

	/** Mouse look stays raw — a mouse has no deadzone and no drift, and curving
	 *  it breaks muscle memory. Only the scale is tunable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0))
	float MouseLookScale = 1.0f;

	/**
	 * Aim assist, target-slowdown form: the camera gets heavier while the reticle
	 * is over a hostile. 0 = off, 1 = maximum. Deliberately NOT magnetism (which
	 * pulls the reticle toward a target): at command distance you sweep the
	 * reticle past your own squad to give orders, and a pull would fight that
	 * every time. Slowdown only ever resists, so it can never move your aim
	 * somewhere you did not point it — and it is the honest form when hits are
	 * locational (GDD 8.2).
	 * At full strength the look keeps AimAssistFloor of its speed; CoD ships 0.4
	 * hip / 0.5 ADS for the same mechanism, so this band is sourced even though
	 * this exact default is a starting point to tune against.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0, ClampMax = 1))
	float AimAssistStrength = 0.6f;

	/** Speed retained at full strength. 0.45 sits between CoD's 0.4 and 0.5. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0.1, ClampMax = 1))
	float AimAssistFloor = 0.45f;

	/** Half-angle of the cone around the reticle that counts as "over a target".
	 *  Halo's friction cone measures about 5 degrees, Lyra's box is tighter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0, ClampMax = 20))
	float AimAssistConeDegrees = 4.0f;

	/** Beyond this the assist fades out. 5000 uu = 50 m, which is where the GDD
	 *  puts the hitscan/projectile boundary — help stops where ballistics change. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0))
	float AimAssistRange = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look")
	bool bInvertLookY = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = -89, ClampMax = 0))
	float ViewPitchMin = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0, ClampMax = 89))
	float ViewPitchMax = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Attributes", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;
};

/**
 * Visual body definition (DT_BodyDefs row — step-2 character pipeline). Soft
 * references only: a missing asset degrades to the capsule body with a logged
 * warning, never a crash (GDD 14.3.5).
 *
 * The anim columns feed UEclipseAnimInstance's minimal locomotion (GDD 14.5
 * debug-grade). They are resolved PER BODY and checked against THAT body's
 * skeleton, because DT_BodyDefs points at nine packs that share a skeleton
 * family but not a skeleton asset — see FEclipseLocomotionSet. Missing takes
 * cost the body a rung on the ladder, not the frame.
 */
USTRUCT(BlueprintType)
struct FEclipseBodyDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/**
	 * Looping standing pose. Mandatory: it is the floor of the locomotion ladder
	 * (EEclipseLocomotionTier) — without it a body has nothing to cross-fade out
	 * of and falls all the way back to the mesh's ref pose.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> IdleAnim;

	/** Looping walk cycle, blended in at DA_CharacterTuning's WalkSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> WalkAnim;

	/**
	 * Looping run/jog cycle, blended in at RunSpeed and driven faster (never
	 * swapped) for sprint. Optional: a body with only a walk still gets a gait,
	 * it just carries the whole ramp (GDD 14.3.5).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> RunAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> ShootAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> DeathAnim;

	/**
	 * Toon restyle (15.5 asset policy: downloaded, then restyled): re-dress every
	 * material slot with the cel master — the slot's own base texture becomes
	 * luminance detail, the faction palette below supplies the hue. Keeps every
	 * body on the same exposure tier as the unlit district (lit PBR bodies
	 * underexpose to silhouettes against the emissive world).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	bool bToonRestyle = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body", meta = (EditCondition = "bToonRestyle"))
	FLinearColor TintLit = FLinearColor(0.20f, 0.20f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body", meta = (EditCondition = "bToonRestyle"))
	FLinearColor TintShade = FLinearColor(0.07f, 0.07f, 0.09f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body", meta = (ClampMin = 0.1))
	float MeshScale = 1.0f;

	/** Mesh-root drop below the capsule center; -90 fits a standing humanoid in the default capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	float MeshZOffset = -90.0f;

	/** UE humanoid meshes face +Y; -90 turns them to the capsule's forward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	float MeshYaw = -90.0f;
};

/**
 * Named story-character slot (DT_NamedCharacters — step-3 MetaHuman pipeline).
 * The owner authors MH_<Name> in MetaHuman Creator (phase0/metahuman_recipes.md);
 * until that asset exists the slot dresses itself from FallbackBodyDef, so the
 * missing face never blocks missions or dialogue wiring (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseNamedCharacterRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	FText DisplayName;

	/** Imported MetaHuman body (MH_<Name>); unset = fallback body below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	TSoftObjectPtr<USkeletalMesh> MetaHumanMesh;

	/** DT_BodyDefs row that stands in while the MetaHuman is absent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	FName FallbackBodyDef;

	/** Rebel / Dominion — drives tint and bark selection later (GDD 08/16). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	FName Faction;
};

/**
 * One soldier class (DT_ClassDefs row; row name = class id — SPEC-P2-01,
 * GDD 4.2.3). Classes are data, not subclasses (GDD 12.3): the one
 * AEclipseCharacter body stays; a class changes kit, one signature verb and
 * bark flavor — never the shared movement/health/order contract. A soldier
 * whose ClassId has no row here degrades to the classless kit (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseClassDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FText DisplayName;

	/** DT_Weapons row this class carries; NAME_None = the platform default (first row). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FName WeaponRow;

	/** Optional DT_BodyDefs row that dresses this class (visible kit); NAME_None = the shared squad body pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FName BodyDefOverride;

	/** Signature verb identity (Class.Verb.* family — Momentum/Stabilize/Killzone). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FGameplayTag SignatureVerb;

	/** Seconds after a down in which this class can stabilize (GDD 4.2.5 window; 30 for Medic, 0 = cannot). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float StabilizeWindowSeconds = 0.0f;

	/** Killzone lane range in cm (6000 for Sniper, 0 = none; Command Mode wiring = SPEC-P2-02). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float KillzoneRangeCm = 0.0f;

	/** Bark-pool set id for class-flavored lines (content tier fills the pools). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FName BarkSet;

	/**
	 * Per-class order modulation as data (GDD 9.5: traits/classes modulate
	 * parameters, orders stay unchanged in surface). Assault pushes past the
	 * ordered point by this many cm toward the order direction; 0 = obey exactly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float OrderPushDistanceCm = 0.0f;

	/**
	 * Cover-scorer lane bias (existing ring scorer, SPEC-P1-06): 0 = nearest
	 * cover wins; higher values prefer covered samples with a longer clear lane
	 * to the threat (Sniper overwatch preference).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float CoverLaneBias = 0.0f;

	/** Auto-triage: move to downed squadmates and stabilize without an order (Medic true). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	bool bAutoTriage = false;
};

/** One enemy archetype (DT_EnemyArchetypes row, SPEC-P1-05 data). */
USTRUCT(BlueprintType)
struct FEclipseEnemyArchetypeRow : public FTableRowBase
{
	GENERATED_BODY()

	/** DT_BodyDefs row that dresses this archetype (step-2 character pipeline); NAME_None = capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy")
	FName BodyDef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 1))
	float Health = 60.0f;

	/** Damage per hit. Feel target: exposed player dies in ~2.5 s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float Damage = 10.0f;

	/** Sight radius (cm) for the perception stub. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float PerceptionRadius = 2500.0f;

	/** Seconds between shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0.05))
	float FireInterval = 0.8f;

	/** Pursuit stop distance (cm) — how close this archetype closes before holding to shoot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float EngageRange = 600.0f;

	/** Weaponless fallback strike range (cm) — a data mistake degrades to weak melee, never invincible (GDD 14.3.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float MeleeRange = 200.0f;
};

/** One weapon platform (DT_Weapons row; Phase 1: one AR, one sidearm per feel targets). */
USTRUCT(BlueprintType)
struct FEclipseWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Damage per shot. Feel target: well-aimed player TTK vs. basic enemy ~0.6 s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 0))
	float Damage = 22.0f;

	/** Hitscan under 50 m (feel targets); projectile tier is Phase 2+. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 0))
	float RangeCm = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 0.05))
	float FireInterval = 0.15f;

	/** Locational damage stub (GDD 8.2): headshot multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 1))
	float HeadshotMultiplier = 2.5f;
};
