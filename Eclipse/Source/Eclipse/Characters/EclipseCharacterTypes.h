#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
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

	/** Feel target: FOV 90, over-shoulder. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 30, ClampMax = 140))
	float CameraFOV = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Attributes", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;
};

/**
 * Visual body definition (DT_BodyDefs row — step-2 character pipeline). Soft
 * references only: a missing asset degrades to the capsule body with a logged
 * warning, never a crash (GDD 14.3.5). PLACEHOLDER(GDD 15.7): the minimal
 * idle-only anim tier; the locomotion AnimBlueprint tier is SPEC-P2-01 work.
 */
USTRUCT(BlueprintType)
struct FEclipseBodyDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/** Looping idle; single-node playback until the ABP tier lands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> IdleAnim;

	/** Reserved for the SPEC-P2-01 locomotion tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> WalkAnim;

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
