#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "EclipseGameMode.generated.h"

class AEclipseCharacter;

/**
 * Graybox game mode (SPEC-P1-05): possesses the player body, and — driven by the
 * mission lifecycle on the event bus — spawns the squad of 4 (player + 3, class
 * kits from DT_ClassDefs — SPEC-P2-01) and the mission's enemy sets on
 * Event.Mission.Started, and tears them down on Completed/Failed. All spawn
 * parameters come from mission/tuning data (GDD 14.2).
 */
UCLASS()
class ECLIPSE_API AEclipseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Diagnostiek: vijanden die door een schot van de spelerskant in beweging kwamen. */
	int32 GetEnemiesAlertedByShots() const { return EnemiesAlertedByShots; }


	AEclipseGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Route mission lifecycle facts: Started -> spawn, Completed/Failed -> despawn. */
	void OnMissionLifecycle(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void SpawnMissionActors();
	void DespawnMissionActors();
	AEclipseCharacter* SpawnBodyNear(const FVector& Location, const FString& Label);
	FVector FindSiteLocation(FName SiteId, const FVector& Fallback) const;

	/** Player body down = mission failure (fail-forward at debrief, GDD 11.4). */
	void HandlePlayerDowned(AEclipseCharacter* Player, FName Cause);

	/**
	 * Een vijand van de missie ging neer. Ligt de LAATSTE van de set die bij het
	 * doelwit-site hoort, dan is het DestroyTarget-objective vervuld.
	 *
	 * Dit pad ontbrak volledig tot 2026-07-25: DestroyTarget had geen enkele
	 * voltooiingsroute, en de overlap-trigger vinkte hem daarom af zodra je
	 * langsliep. De game mode is de eerlijke plek voor deze koppeling, want hij is
	 * degene die de vijandenset spawnt en dus als enige weet wanneer hij op is.
	 */
	void HandleHostileDowned(AEclipseCharacter* Hostile, FName Cause);

	/** De vijanden die bij het DestroyTarget-site horen, en dat site zelf. */
	TArray<TWeakObjectPtr<AEclipseCharacter>> ObjectiveHostiles;
	FName ObjectiveHostileSiteId;

#if !UE_BUILD_SHIPPING
	/**
	 * Screenshot review rig (Part 15.8/15.9): with -EclipseShot on the command
	 * line, cycle fixed vantage cameras, HighResShot each, then quit — the
	 * "review screenshots every pass" ritual, automatable headless.
	 */
	void SetupShotRig();
	void AdvanceShotRig();
	FTimerHandle ShotRigTimer;
	int32 ShotRigStep = 0;

	/**
	 * SPEELRONDE MET OPNAMES (owner-opdracht 26-07, 22:00).
	 *
	 * De vaste review-camera's hierboven beoordelen de KUNST. Deze ronde
	 * beoordeelt of het SPEL er is: staat mijn personage in beeld, beweegt hij
	 * mee, klopt zijn schaal. Vanuit de speler, tijdens het spelen, op vaste
	 * momenten — en er kijkt daarna iemand naar.
	 *
	 * Aanleiding, de owner letterlijk: *"Je speelronde meet uitkomsten en die
	 * kunnen groen zijn terwijl er niets te zien is."* Op 26-07 waren 152 tests
	 * groen terwijl het personage bij stilstand onzichtbaar was.
	 *
	 * Aparte vlag en niet in -EclipseShot: die teleporteert naar overzichts-
	 * camera's en zet de pawn op vliegen. Hier moet hij juist gewoon lopen.
	 */
	void SetupPlayShotRound();
	void AdvancePlayShotRound();

	FTimerHandle PlayShotTimer;
	FTimerHandle PlayShotDriveTimer;
	int32 PlayShotStep = 0;
	bool bPlayShotWalking = false;
	bool bPlayShotFiring = false;
	bool bPlayShotTurning = false;

	/** Waar de camera stond bij de vorige opname; om te zien of beweging het beeld haalt. */
	FVector PlayShotLastCamera = FVector::ZeroVector;

	/**
	 * Hoogste snelheid sinds het vorige opnamemoment, bijgehouden in de 50 Hz-duw.
	 * Een momentopname op het opnamemoment gaf 0 cm/s terwijl er 222 cm was
	 * afgelegd — die viel op een niet-representatief ogenblik. Zie DrivePlayShotInput.
	 */
	float PlayShotIntervalTopSpeed = 0.0f;

	/**
	 * Opgetelde afgelegde weg sinds het vorige opnamemoment. Naast de NETTO
	 * verplaatsing van de camera scheidt dit "hij heeft nooit bewogen" (weg ~ 0)
	 * van "hij liep en werd teruggezet" (weg groot, netto klein).
	 */
	float PlayShotIntervalPathLength = 0.0f;

	/** Positie bij de vorige drive-tick, nodig voor die optelling. */
	FVector PlayShotLastDriveLocation = FVector::ZeroVector;

	/** Duwt elke tick beweging/vuur door zodat de opnames een LOPEND spel vangen. */
	void DrivePlayShotInput();

	/** Hangt een naam en een maat aan elke vorm in het frame; zonder dit is een screenshot een vermoeden. */
	void MeasurePlayShot(int32 ShotIndex);

	/** De aankleedfiguren zijn geen EclipseCharacter; zonder deze meting blijven ze buiten beeld van elke controle. */
	void MeasureDressingFigures(int32 ShotIndex);

	/** Staat er na het lopen nog iemand van je squad in beeld? Wereldruimte bewijst dat niet. */
	void ReportSquadInFrame(int32 ShotIndex, int32 DrawnNearby);

	/**
	 * Playtest shortcut (13.2 owner finding): -EclipseStartMission=<RegionId>
	 * selects that region's offer and auto-launches, so a feel-gauntlet run does
	 * not start with hub clicks. Drives the same SelectMission -> AutoLaunch seam
	 * the hub uses, so it cannot mask a bug in the real path; every failure falls
	 * back to a normal hub start with a loud reason (GDD 14.3.5).
	 */
	void StartMissionFromCommandLine();
#endif

	/** Actors spawned for the active mission (squad + enemies), destroyed at teardown. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedMissionActors;

	FEclipseEventSubscriptionHandle MissionEventsHandle;
	FEclipseEventSubscriptionHandle ShotFiredHandle;

	/**
	 * Hoe vaak een schot van de spelerskant een vijand in beweging zette.
	 *
	 * Bestaat sinds 26-07 avond om één vraag te kunnen meten die de squad-doctrine
	 * opriep: nu je squad UIT ZICHZELF vuurt, verraadt hij jou. Elk schot van de
	 * spelerskant alarmeert, en dat geldt net zo goed voor het schot dat jij niet
	 * gaf. Dat maakt `recon` niet zomaar een houding maar je enige sluipoptie.
	 */
	int32 EnemiesAlertedByShots = 0;

	/** Een schot vertalen naar wie het hoort (26-07, punt 1). */
	void OnShotFired(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Eén regel over het eerste gehoorde schot; daarna zwijgt het. */
	bool bLoggedFirstShotAlert = false;
};
