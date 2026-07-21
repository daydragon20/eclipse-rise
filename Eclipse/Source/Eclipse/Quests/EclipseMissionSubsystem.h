#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Quests/EclipseMissionLogic.h"
#include "Quests/EclipseMissionTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseMissionSubsystem.generated.h"

class IConsoleObject;

/**
 * Mission runtime (SPEC-P1-05): runs the universal loop (GDD 11.1) and commits
 * consequences exclusively through the CampaignState transaction at debrief —
 * killing the run mid-mission loses no strategic state because nothing is
 * committed until then (SPEC-P1-05 DoD).
 *
 * Phase 1 wiring: the offer arrives via Event.Strategy.MissionSelected, the
 * launch via Event.Prep.MissionLaunchRequested (or the debug console). Level
 * actors (objective components, spawners) attach in the graybox pass; the
 * console commands double as the Gauntlet script surface until then.
 */
UCLASS()
class ECLIPSE_API UEclipseMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EEclipseMissionPhase GetPhase() const { return Phase; }
	const FEclipseMissionOutcome& GetLastOutcome() const { return LastOutcome; }

	/** Start the selected mission (normally driven by Event.Prep.MissionLaunchRequested). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Mission")
	bool StartMission(const TArray<FGuid>& SquadSoldierIds, FString& OutError);

	/** An objective primitive reports completion (level components / debug console). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Mission")
	bool CompleteObjective(FName ObjectiveId, FString& OutError);

	/** Site-driven completion (objective triggers report the site; the active spec maps site -> objective). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Mission")
	void CompleteObjectiveByTarget(FName TargetId);

	/** Roster ids deployed on the active mission (game-mode spawn wiring). */
	const TArray<FGuid>& GetDeployedSoldierIds() const { return DeployedSoldierIds; }

	/** Active objective list (HUD/debug display; empty outside a run). */
	const TArray<FEclipseObjectiveDef>& GetActiveObjectives() const { return ActiveObjectives; }

	/** A squadmate went down in the field (SPEC-P1-06 wiring; resolution happens at debrief). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Mission")
	void NotifySoldierDowned(const FGuid& SoldierId, FName Cause);

	/**
	 * End the mission and commit consequences (GDD 14.3.3). bSuccess=false is
	 * fail-forward (GDD 11.4): casualties and salvage intel still commit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Mission")
	bool ResolveDebrief(bool bSuccess, FString& OutError);

private:
	void OnMissionSelected(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnLaunchRequested(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void BroadcastMissionEvent(const FGameplayTag& Tag, FName ObjectiveId, bool bSuccess);
	void ResetRuntime();
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

	/** Resolve the mission asset for a template; missing content synthesizes a default spec (GDD 14.3.5). */
	void ResolveMissionSpec(FName TemplateId, TArray<FEclipseObjectiveDef>& OutObjectives, bool& bOutProgressRegion) const;

	EEclipseMissionPhase Phase = EEclipseMissionPhase::None;

	/** Offer picked on the map (region, template, rewards) awaiting launch. */
	FName PendingRegionId;
	FName PendingTemplateId;
	FEclipseMissionRewards PendingRewards;
	bool bHasPendingOffer = false;

	/** Active-run state. */
	TArray<FEclipseObjectiveDef> ActiveObjectives;
	TArray<FName> CompletedObjectiveIds;
	TArray<FGuid> DeployedSoldierIds;
	TMap<FGuid, FName> DownedSoldiers;
	bool bProgressRegionOnSuccess = true;

	FEclipseMissionOutcome LastOutcome;

	FEclipseEventSubscriptionHandle MissionSelectedHandle;
	FEclipseEventSubscriptionHandle LaunchRequestedHandle;
	TArray<IConsoleObject*> ConsoleCommands;
};
