#pragma once

#include "Components/ActorComponent.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Combat/EclipseWeaponStatusFeed.h"
#include "CoreMinimal.h"
#include "EclipseHitscanWeaponComponent.generated.h"

class AEclipseCharacter;
class UStaticMesh;

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
	/**
	 * DE NAAM DIE DE SPELER LEEST ("Foundry AR"), uit DT_Weapons.DisplayName.
	 *
	 * Naast GetActiveWeaponName en niet in plaats daarvan: de rijnaam blijft de
	 * SLEUTEL (voor meshes, logregels en de diagnose) en deze is de LEESTEKST. Ze
	 * door elkaar gebruiken is precies hoe `Sidearm_Scrap` op het scherm van 31-07
	 * terechtkwam. Leeg mogelijk — de schermlaag hoort dan luid te degraderen.
	 */
	const FText& GetActiveWeaponDisplayName() const { return Weapon.DisplayName; }
	/**
	 * Het zichtbare mesh van het wapen in je handen (O-5 "volledig", 31-07).
	 *
	 * Uit de ACTIEVE rij en niet uit een eigen veld: het wapen dat je vasthoudt is
	 * één ding, en zijn getallen en zijn vorm horen dus uit dezelfde rij te komen.
	 * Twee bronnen zouden betekenen dat een wissel het magazijn kan veranderen
	 * zonder de vorm — en precies die ontkoppeling is de bug die dit repareert.
	 */
	const TSoftObjectPtr<UStaticMesh>& GetActiveWeaponMesh() const { return Weapon.Mesh; }
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

	/**
	 * Ben je NU aan het herladen? Op de KLOK, niet op een vlag.
	 *
	 * DIT WAS DEFECT 2, en de vorm ervan is leerzaam. `bReloading` werd gezet in
	 * StartReload en alleen weer gewist door FinishReload — en FinishReload had
	 * precies één aanroeper: de tak bovenin Fire(). Wie na een herlaadbeurt niet
	 * meer schoot, bleef dus voor eeuwig "aan het herladen", met een magazijn dat
	 * nooit werd bijgevuld. GEMETEN met een tikkende wereld
	 * (Tests/EclipseWeaponReloadTests.cpp): op t = 2,4 / 3,0 / 6,0 s na een beurt
	 * van 2,2 s stond herladen=1 en munitie=29/30.
	 *
	 * De reparatie is bewust GEEN timer en GEEN tick, want beide zouden het
	 * architectuurbesluit van dit component omkeren (event-driven, 14.2) en een
	 * timer die een missiewissel overleeft kan een wapen voorgoed blokkeren. In
	 * plaats daarvan is de VRAAG eerlijk gemaakt: de beurt heeft een eindtijd, dus
	 * "ben ik nog bezig" is uit te rekenen in plaats van te onthouden. Er is niets
	 * meer dat kan vergeten af te lopen.
	 */
	bool IsReloading() const;

	/**
	 * Kogels in het magazijn — met dezelfde klok-correctie als IsReloading().
	 *
	 * Zonder deze correctie zou een afgelopen beurt wél "niet meer aan het herladen"
	 * melden en tóch het oude, lage magazijn tonen. Dat is een tweede leugen in
	 * plaats van een halve reparatie: de speler leest "klaar" en heeft 3 kogels.
	 */
	int32 GetAmmoInMagazine() const;

	/**
	 * 0..1 door de lopende herlaadbeurt heen; 0 als er niet herladen wordt.
	 * De schermlaag tekent hier zijn voortgang mee — dat is het verschil tussen
	 * "er staat iets" en "ik weet wanneer ik weer kan schieten".
	 */
	float GetReloadProgress() const;

	int32 GetMagazineSize() const { return Weapon.MagazineSize; }
	float GetReloadSeconds() const { return Weapon.ReloadSeconds; }
	int32 GetReloadCount() const { return ReloadCount; }

	/**
	 * DE SPREIDING DIE HET VOLGENDE SCHOT KRIJGT, in graden.
	 *
	 * ÉÉN BRON VOOR DE KOGEL EN VOOR HET KRUIS, en dat is geen netheid maar een
	 * les uit dit project: DEBUG_DISCIPLINE.md §4.2 beschrijft hoe dezelfde formule
	 * twee keer stond (in de proxy die de speler ziet en in de spiegel die de test
	 * uitleest) en hoe een meting aan de ene helft daardoor niets bewees over de
	 * andere. Een richtkruis dat zijn eigen spreiding uitrekent, belooft iets wat
	 * de kogel niet doet — en dat is erger dan geen kruis.
	 *
	 * Inclusief de "eerste schot is zuiver"-regel én het VERLOPEN daarvan op de
	 * klok: laat je de trekker drie vuurintervallen los, dan is het volgende schot
	 * weer zuiver en hoort het kruis dus dicht te trekken. Precies zoals bij het
	 * herladen wordt dat hier UITGEREKEND en niet onthouden — een teller die pas
	 * bij de volgende trekkerbeweging wordt bijgewerkt, zou het kruis wijd laten
	 * staan terwijl het wapen allang weer zuiver is.
	 */
	float GetCurrentSpreadDegrees() const;

	float GetHipSpreadDegrees() const { return Weapon.HipSpreadDegrees; }
	float GetAimSpreadDegrees() const { return Weapon.AimSpreadDegrees; }
	int32 GetPelletsPerShot() const { return Weapon.PelletsPerShot; }

	float GetFalloffStartCm() const { return Weapon.FalloffStartCm; }
	float GetFalloffMinFraction() const { return Weapon.FalloffMinFraction; }
	float GetRecoilPitchDegrees() const { return Weapon.RecoilPitchDegrees; }
	float GetRecoilRecoveryDegreesPerSecond() const { return Weapon.RecoilRecoveryDegreesPerSecond; }

	/** Zie EEclipseWeaponFireMode: vandaag voor elke DT_Weapons-rij `Unspecified`. */
	EEclipseWeaponFireMode GetFireMode() const { return Weapon.FireMode; }

	/**
	 * Hoeveel Event.Player.WeaponStatusChanged dit wapen werkelijk heeft uitgezonden.
	 *
	 * Bestaat om dezelfde reden als GetVitalsBroadcastCount op het lichaam: bij een
	 * meting van 0 feiten op de bus moet de keten te bisecteren zijn. Zonder dit
	 * getal kan "er kwam niets aan" evengoed betekenen dat de poort het wapen
	 * afwees, dat de feed het geen verandering vond, of dat de bus niet afleverde —
	 * en een kale teller scheidt die drie niet.
	 */
	int32 GetWeaponStatusBroadcastCount() const { return StatusTracker.GetBroadcastCount(); }

	/**
	 * De huidige stand als feit op de bus zetten, als er iets veranderd is.
	 *
	 * Publiek en niet privé: de bedrading hangt aan de plekken waar de stand
	 * WERKELIJK verschuift (vuren, herladen, wisselen, uitrusten), en een test moet
	 * kunnen bewijzen dat een extra aanroep zonder verandering niets oplevert. De
	 * rem zit in EclipseWeaponStatusFeed, niet hier: deze functie mag rustig vaker
	 * aangeroepen worden dan er iets gebeurt.
	 */
	void PublishWeaponStatus();

	/**
	 * TIKT ALLEEN TIJDENS EEN HERLAADBEURT, en alleen voor het wapen van een lokale
	 * speler.
	 *
	 * Het component was en blijft event-gedreven (GDD 14.2) — de constructor zet de
	 * tick uit en StartReload zet hem aan, TickComponent zet hem zelf weer uit. De
	 * uitzondering is er omdat herlaadVOORTGANG als enige van de zes feiten geen
	 * gebeurtenis is maar een KLOK: tussen "beurt begonnen" en "beurt klaar" gebeurt
	 * er niets waar een balk aan kan hangen. Zonder deze tick zou de HUD daarvoor
	 * alsnog moeten pollen, en dan is precies het ene stuk overgebleven waar dit
	 * werk voor bestond.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	 * Kijkt er een LOKALE SPELER door dit wapen heen?
	 *
	 * Zonder deze poort zou elke vijand op de kaart zijn magazijn over de
	 * spelersbus sturen. Bewust dezelfde vraag en dezelfde formulering als
	 * `AEclipseCharacter::PublishVitals`, inclusief de daar gemeten les: NIET
	 * `APawn::IsPlayerControlled()`, want die leest `GetPlayerState() && !IsABot()`
	 * en hangt dus aan een object dat pas bestaat als een GameMode het spawnt.
	 */
	bool HasLocalPlayerConsumer() const;

	/** De rauwe waarneming voor de feed; alle conclusies staan in EclipseWeaponStatusFeed. */
	EclipseWeaponStatusFeed::FEclipseWeaponSnapshot MakeStatusSnapshot() const;

	/** Het geheugen achter Event.Player.WeaponStatusChanged: wat er het laatst uit ging. */
	EclipseWeaponStatusFeed::FEclipseWeaponStatusTracker StatusTracker;

	/** Het zichtbare wapen bijwerken naar de actieve rij. Zie de toelichting in de .cpp. */
	void NotifyActiveWeaponChanged();

	int32 ShotsFired = 0;

	/** Zie GetWorldHits(): de missers, die tot 27-07 helemaal niets achterlieten. */
	int32 WorldHits = 0;

	/** Schoten waarbij de trace niets raakte. */
	int32 CleanMisses = 0;

	/** Hoeveel zichtbare inslagsporen er echt zijn ontstaan. */
	int32 ImpactMarksSpawned = 0;

	/** Kogels in het magazijn. -1 tot ApplyWeaponRow hem vult. */
	int32 AmmoInMagazine = -1;
	bool bReloading = false;
	double ReloadEndSeconds = -1.0;
	int32 ReloadCount = 0;

	void FinishReload();

	/**
	 * De administratie bijwerken als de klok de beurt al voorbij is.
	 *
	 * De getters hierboven rekenen de waarheid uit en zijn const; deze functie legt
	 * hem vast en staat op elk pad dat de toestand tóch al aanraakt (StartReload,
	 * Fire). Zo blijven de opgeslagen velden en wat de wereld te horen krijgt nooit
	 * langer dan één handeling uit elkaar lopen.
	 */
	void SettleReloadIfElapsed();

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
