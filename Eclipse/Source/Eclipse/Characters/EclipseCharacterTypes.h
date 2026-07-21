#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "EclipseCharacterTypes.generated.h"

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

/** One enemy archetype (DT_EnemyArchetypes row, SPEC-P1-05 data). */
USTRUCT(BlueprintType)
struct FEclipseEnemyArchetypeRow : public FTableRowBase
{
	GENERATED_BODY()

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
