#pragma once

#include "CoreMinimal.h"
#include "Base/EclipsePrepLogic.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipsePrepSubsystem.generated.h"

class IConsoleObject;

/**
 * Preparation flow service (SPEC-P1-08): validates the squad/loadout/insertion
 * pick, charges the optional intel reveal through a campaign transaction
 * (GDD 14.3.3 — even prep spending goes through the single writer), and emits
 * Event.Prep.MissionLaunchRequested. Preparation converts strategic wealth
 * into tactical fairness (GDD 11.1) — this subsystem is that conversion.
 */
UCLASS()
class ECLIPSE_API UEclipsePrepSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Loadouts currently pickable (base + produced unlocks). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Prep")
	TArray<FGameplayTag> GetAvailableLoadoutTags() const;

	/** Spend intel for the briefing reveal; returns false (with reason) when the wallet can't cover it. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Prep")
	bool SpendIntelForReveal(FString& OutError);

	/** True after a successful reveal for the currently selected mission. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Prep")
	bool WasIntelRevealed() const { return bIntelRevealed; }

	/**
	 * Validate the full pick and broadcast the launch request. The mission
	 * runtime consumes the event; this subsystem never touches the runtime
	 * directly (GDD 14.3.1).
	 */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Prep")
	bool LaunchMission(const TArray<FGuid>& SquadSoldierIds, FGameplayTag LoadoutTag, FName InsertionId, FString& OutError);

	/** Debug/Gauntlet: first available squad, first loadout, first insertion (SPEC-P1-08 AutoLaunch). */
	bool AutoLaunch(FString& OutError);

private:
	const class UEclipsePrepTuningAsset* ResolveTuning() const;
	TArray<EclipsePrepLogic::FEclipseLoadoutOption> ResolveLoadoutOptions() const;
	void OnMissionSelected(FGameplayTag EventTag, const FInstancedStruct& Payload);

	bool bIntelRevealed = false;
	FName SelectedRegionId;

	FEclipseEventSubscriptionHandle MissionSelectedHandle;
	IConsoleObject* AutoLaunchCommand = nullptr;
};
