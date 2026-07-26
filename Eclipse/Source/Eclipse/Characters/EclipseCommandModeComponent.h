#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Squad/EclipseCommandLogic.h"
#include "Squad/EclipseSquadTypes.h"
#include "EclipseCommandModeComponent.generated.h"

class UEclipseCommandModeTuningAsset;
class IConsoleObject;

/**
 * Command Mode wrapper (SPEC-P2-02 Stage A). Owns exactly the impure half:
 * applying/restoring global time dilation, soldier selection over the live
 * squad, and the ModeEntered/ModeExited facts. All decisions live in
 * EclipseCommandLogic (pure); the order path itself is untouched — dispatch
 * still runs through UEclipseSquadSubsystem::IssueOrder/IssueOrderToAll, and
 * acks/refusals stay synchronous, so the ≤1 s answer bar is wall-clock-true
 * under any dilation (locked decision 2).
 *
 * Fail-safe (locked decision 5): every exit path — release, pawn death,
 * mission end, EndPlay/level travel — funnels through ExitMode, which always
 * restores dilation 1.0. The component ticks ONLY while the mode is held
 * (pawn-death watch); outside the hold it adds zero per-frame work (12.4).
 */
UCLASS()
class ECLIPSE_API UEclipseCommandModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEclipseCommandModeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Input: hold began/ended. */
	void OnHoldPressed();
	void OnHoldReleased();

	/** Input: cycle selection (+1 next / -1 prev). No-op outside the held mode. */
	void CycleSoldierSelection(int32 Direction);

	/** Input: select the squadmate under the reticle; unknown/enemy bodies leave selection unchanged. */
	void PickSoldierUnderReticle();

	/** Input (pad, in-mode): toggle the stance applied to orders issued while held. */
	void ToggleHeldStance();

	bool IsHeld() const { return State.bHeld; }
	FGuid GetSelectedSoldier() const { return State.SelectedSoldier; }
	EEclipseSquadStance GetHeldStance() const { return HeldStance; }

	/**
	 * Zet de doctrine METEEN bij alle squadleden neer (26-07 avond, laag 4).
	 *
	 * Niet pas bij de volgende order, en dat is het hele punt van een KADER: de
	 * owner stuurt intentie, en intentie geldt vanaf het moment dat je hem
	 * uitspreekt. Tot vanavond reisde de stance als parameter mee met een order,
	 * dus wisselen van houding deed niets tot je toevallig iets beval.
	 */
	void PushDoctrineToSquad();

	/** The order path reports each issue so ModeExited telemetry counts usage (R3 metric). */
	void NotifyOrderIssued();

	/** Debug-HUD lines (14.5 step 4): state, dilation, selection, provisional bindings. */
	TArray<FString> GetDebugLines() const;

	/**
	 * R3 criterion 5 (usage pull) read-out, kept apart from GetDebugLines so the
	 * HUD's Command section and the gauntlet panel each draw their own thing once
	 * — the console dump prints both.
	 */
	TArray<FString> GetGauntletUsageLines() const;

	/** Entries in the running encounter beat (gauntlet panel reads, never writes). */
	int32 GetCommandEntriesThisBeat() const { return BeatTally.EntriesThisBeat; }

	/** Closed encounter beats; the average is judged over these only. */
	int32 GetObservedEncounterBeatCount() const { return BeatTally.ClosedBeatCount; }

	float GetAverageEntriesPerBeat() const { return BeatTally.GetAverageEntriesPerBeat(); }

	/**
	 * Close the running encounter beat (overlay's beat key). bCountEmptyBeat
	 * records a beat the tester says happened even with zero entries — the
	 * measurement that actually fails criterion 5.
	 */
	void BeginNextEncounterBeat(bool bCountEmptyBeat);

private:
	void ExitMode(EclipseCommandLogic::EEclipseCommandSignal Signal);
	void ApplyDesiredDilation();
	float ResolveDilationFactor() const;
	const UEclipseCommandModeTuningAsset* ResolveTuning() const;
	void OnMissionEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void BroadcastMode(bool bEntered) const;

	/**
	 * DA_CommandModeTuning (SPEC-P2-02 data). Soft-ref on the component (not on
	 * the campaign setup) so this spec owns no shared files; missing asset =
	 * logged warning + code defaults (GDD 14.3.5).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Eclipse|Command")
	TSoftObjectPtr<UEclipseCommandModeTuningAsset> Tuning;

	EclipseCommandLogic::FEclipseCommandModeState State;

	/** Usage-pull tally per encounter beat (R3 criterion 5); reset by Event.Mission.Started. */
	EclipseCommandLogic::FEclipseEncounterBeatTally BeatTally;

	EEclipseSquadStance HeldStance = EEclipseSquadStance::Ready;
	FEclipseEventSubscriptionHandle MissionEventsHandle;
	IConsoleObject* DumpCommand = nullptr;
	mutable TWeakObjectPtr<const UEclipseCommandModeTuningAsset> CachedTuning;
	mutable bool bTuningLoadAttempted = false;
};
