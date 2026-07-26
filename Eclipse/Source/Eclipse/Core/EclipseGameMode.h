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

	/** Een schot vertalen naar wie het hoort (26-07, punt 1). */
	void OnShotFired(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Eén regel over het eerste gehoorde schot; daarna zwijgt het. */
	bool bLoggedFirstShotAlert = false;
};
