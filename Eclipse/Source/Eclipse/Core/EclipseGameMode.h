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
#endif

	/** Actors spawned for the active mission (squad + enemies), destroyed at teardown. */
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedMissionActors;

	FEclipseEventSubscriptionHandle MissionEventsHandle;
};
