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
	 * Twee slots uit de gekozen loadout (26-07 avond, punt 5). Slot 0 is het
	 * primaire wapen, slot 1 de sidearm; je begint met het primaire in je handen.
	 *
	 * Elk slot houdt zijn EIGEN magazijn bij. Dat is wat elke shooter met een
	 * wapenwissel doet, en het is ook de reden dat wisselen tactisch is in plaats
	 * van cosmetisch: een halfleeg wapen wegstoppen betekent dat het halfleeg
	 * terugkomt.
	 */
	void ApplyLoadout(const FEclipseWeaponRow& Primary, FName PrimaryName,
		const FEclipseWeaponRow& Sidearm, FName SidearmName);

	/** Wissel naar het andere slot. False als er niets te wisselen valt. */
	bool SwapWeapon();

	int32 GetSlotCount() const { return SlotRows.Num(); }
	int32 GetActiveSlot() const { return ActiveSlot; }
	/**
	 * De rijnaam van het wapen in je handen (AR_Foundry, Sidearm_Scrap, ...).
	 *
	 * Voor de HUD: met twee slots en een wisselknop is "hoeveel kogels" niet genoeg
	 * — je moet ook zien WELK wapen dat magazijn heeft. De rijnaam en niet de
	 * RoleSummary: die laatste is een hele zin, en een zin lees je niet middenin
	 * een vuurgevecht.
	 */
	FName GetActiveWeaponName() const { return SlotNames.IsValidIndex(ActiveSlot) ? SlotNames[ActiveSlot] : NAME_None; }
	/** Klaar om te vuren? Vlak na een wissel niet — dat is de handling-tijd. */
	bool IsReady() const;

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

	/**
	 * Schoten die de WERELD raakten in plaats van een personage — dus de missers.
	 * Bestond niet tot 27-07: die tak deed `return false` en liet geen enkel spoor
	 * na, waardoor elke mis onzichtbaar was. Zie de toelichting bij de branch zelf.
	 */
	/** Het zichtbare spoor van een wereldtreffer: een platte quad op de inslagplek. */
	void SpawnImpactMark(UWorld& World, const struct FHitResult& Hit);

	/** Schoten die HELEMAAL niets raakten — de derde uitkomst naast personage en wereld. */
	int32 GetCleanMisses() const { return CleanMisses; }

	int32 GetWorldHits() const { return WorldHits; }
	/**
	 * MAGAZIJN EN HERLADEN (owner-opdracht 26-07 avond, punt 4).
	 *
	 * Herladen is wat een vuurtempo een BETEKENIS geeft: zonder magazijn is een
	 * hoge cadans gratis, en dan is "40 kogels tegen 10" een getal zonder gevolg.
	 * Borderlands, Destiny en Fortnite hangen er alle drie hun wapengevoel aan.
	 *
	 * De VOORRAAD is oneindig — je magazijn raakt leeg, je munitie niet. Dat is
	 * bewust de kleine keuze van de twee: eindige munitie raakt loadouts, de
	 * economie en de missiebalans tegelijk, en dat is een owner-beslissing. Deze
	 * kant op is hij later nog te maken; andersom zou ik hem al genomen hebben.
	 */
	bool StartReload(FName Cause);
	bool IsReloading() const { return bReloading; }
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
	int32 GetMagazineSize() const { return Weapon.MagazineSize; }
	float GetReloadSeconds() const { return Weapon.ReloadSeconds; }
	int32 GetReloadCount() const { return ReloadCount; }

	float GetFalloffStartCm() const { return Weapon.FalloffStartCm; }
	float GetFalloffMinFraction() const { return Weapon.FalloffMinFraction; }
	float GetRecoilPitchDegrees() const { return Weapon.RecoilPitchDegrees; }
	float GetRecoilRecoveryDegreesPerSecond() const { return Weapon.RecoilRecoveryDegreesPerSecond; }

private:
	int32 ShotsFired = 0;

	/** Zie GetWorldHits(): de missers, die tot 27-07 helemaal niets achterlieten. */
	int32 WorldHits = 0;

	/** Schoten waarbij de trace niets raakte. */
	int32 CleanMisses = 0;

	/** Kogels in het magazijn. -1 tot ApplyWeaponRow hem vult. */
	int32 AmmoInMagazine = -1;
	bool bReloading = false;
	double ReloadEndSeconds = -1.0;
	int32 ReloadCount = 0;

	void FinishReload();

	/** Beide slots, met per slot het magazijn dat erin zit. */
	TArray<FEclipseWeaponRow> SlotRows;
	TArray<int32> SlotAmmo;
	TArray<FName> SlotNames;
	int32 ActiveSlot = 0;

	/** Tot wanneer het opgetilde wapen nog niet kan vuren (ReadySeconds). */
	double ReadyAtSeconds = -1.0;

	/** Schoten in de huidige reeks; 0 = het volgende schot is zuiver. */
	int32 ConsecutiveShots = 0;

	bool ShooterBodyIsAiming() const;

	/** Eén diagnostische regel over de eerste kopschot-beslissing. */
	int32 HeadshotProbesLogged = 0;

	FEclipseWeaponRow Weapon;
	double LastFireTimeSeconds = -1.0;
};
