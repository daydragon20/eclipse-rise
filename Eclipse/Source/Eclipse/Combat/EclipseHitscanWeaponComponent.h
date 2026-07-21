#pragma once

#include "Components/ActorComponent.h"
#include "Characters/EclipseCharacterTypes.h"
#include "CoreMinimal.h"
#include "EclipseHitscanWeaponComponent.generated.h"

class AEclipseCharacter;

/**
 * Minimal hitscan weapon (SPEC-P1-05: "minimal hitscan"; GDD 8.2 hybrid model —
 * the projectile tier is Phase 2+). Stats come from a DT_Weapons row; a
 * hardcoded damage number is a defect (GDD 14.2).
 */
UCLASS(ClassGroup = (Eclipse), meta = (BlueprintSpawnableComponent))
class ECLIPSE_API UEclipseHitscanWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEclipseHitscanWeaponComponent();

	void ApplyWeaponRow(const FEclipseWeaponRow& Row);

	/**
	 * Fire one hitscan from ViewLocation along ViewDirection. Returns true on a
	 * character hit. Respects the row's fire interval (readable cadence beats
	 * spam — GDD 8.1 deliberate rhythm).
	 */
	bool Fire(const FVector& ViewLocation, const FVector& ViewDirection, FName Cause);

	float GetDamage() const { return Weapon.Damage; }

private:
	FEclipseWeaponRow Weapon;
	double LastFireTimeSeconds = -1.0;
};
