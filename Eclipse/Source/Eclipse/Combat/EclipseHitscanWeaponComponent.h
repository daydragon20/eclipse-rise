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
	/** Vermenigvuldiger op een treffer in de bone "head". */
	float GetHeadshotMultiplier() const { return Weapon.HeadshotMultiplier; }
	/** Maximale hitscan-afstand in cm. */
	float GetRangeCm() const { return Weapon.RangeCm; }
	/** Seconden tussen twee schoten — de poort die het vuurtempo bepaalt. */
	float GetFireInterval() const { return Weapon.FireInterval; }

	/**
	 * Schoten die de cadanspoort passeerden, raak of mis (26-07).
	 *
	 * Bestaat omdat de vuurtempo-test het aantal schoten AFLEIDDE uit de schade.
	 * Dat werkte zolang elk schot raak was; met spreiding en terugslag meet hij
	 * dan het aantal TREFFERS en noemt dat het tempo. De meting robuust maken
	 * hoort vóór de gedragswijziging, niet erna.
	 */
	int32 GetShotsFired() const { return ShotsFired; }

private:
	int32 ShotsFired = 0;

	/** Schoten in de huidige reeks; 0 = het volgende schot is zuiver. */
	int32 ConsecutiveShots = 0;

	bool ShooterBodyIsAiming() const;

	/** Eén diagnostische regel over de eerste kopschot-beslissing. */
	int32 HeadshotProbesLogged = 0;

	FEclipseWeaponRow Weapon;
	double LastFireTimeSeconds = -1.0;
};
