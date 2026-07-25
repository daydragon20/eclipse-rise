#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "CoreMinimal.h"
#include "UI/EclipseGauntletOverlayLogic.h"
#include "EclipseMissionHudWidget.generated.h"

class IConsoleObject;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;

/**
 * Debug-grade in-mission HUD (SPEC-P1-05 objective list + SPEC-P1-06 order-state
 * widget) and — since the P2-02 feel gauntlet — the single in-game debug overlay:
 * control overview (F2), the five R3 criteria, and the 13.2 playtest checklist
 * (H). One widget on purpose: a second overlay would draw the same facts twice.
 * No art, the readout *is* the deliverable (GDD 14.5 step 4).
 *
 * Two rendering paths, deliberately separated:
 *  - the live sections (objectives, squad orders, Command Mode state) rebuild
 *    their widget tree when a fact arrives, as they always have;
 *  - the gauntlet/playtest/control panels are built ONCE and afterwards only ever
 *    SetText/SetColor in place, throttled on the wall clock. Per-event widget
 *    allocation in a firefight is exactly what the feel measurement must not pay
 *    for (GDD 12.4).
 *
 * State ownership is unchanged: the round trip is measured in the squad layer
 * (both halves of it happen there) and the usage pull in the Command Mode
 * component (it owns ModeEntered/Exited). This widget reads and formats; only the
 * tester's manual answers live here, because nothing else in the game knows them.
 */
UCLASS()
class ECLIPSE_API UEclipseMissionHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 * May the debug HUD render at all? False during a -EclipseShot review round:
	 * screenshots judge the art, and debug text in the frame has been mistaken for
	 * shipped UI in review rounds (15.8/15.9). Checked at the mount site AND here,
	 * so no creation path can leak it into a still.
	 */
	static bool IsDebugHudAllowed();

	/** F2 — control overview; H — 13.2 playtest checklist. Driven by the controller's debug bindings. */
	void ToggleControlsPanel();
	void TogglePlaytestPanel();

	/** Criterion 2 by hand: one clean pick, or one mis-pick (a mis-pick falsifies the criterion — draaiboek). */
	void NoteTargetingPick(bool bCleanPick);

	/** Criteria 3 and 4 by hand: comfort and trust cycle unanswered -> good -> bad. */
	void CycleComfortAnswer();
	void CycleConfidenceAnswer();

	/** Criterion 5 by hand: close the running encounter beat in the component's tally, entries or not. */
	void MarkEncounterBeat();

	/** One 13.2 playtest row (0-based); no-op while that panel is closed. */
	void CyclePlaytestAnswer(int32 QuestionIndex);

	/** Log + archive the R3-VERDICT INPUT block (console: Eclipse.Gauntlet.Summary; also on teardown). */
	void EmitVerdictSummary();

private:
	void OnAnyFact(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Rebuild the live sections only (objectives, orders, Command Mode state). */
	void Rebuild();

	/** Build the panels whose rows never change identity — once, at construct. */
	void BuildStaticPanels();

	/** In-place refresh of the gauntlet rows; wall-clock throttled, and free when the panel is hidden. */
	void RefreshGauntletRows(bool bForce);

	void RefreshPlaytestRows();

	/** Mark the column of the device the player last touched (Enhanced Input's device subsystem tells us; no polling). */
	void RefreshDeviceHighlight();

	void ApplyPanelVisibility();

	/** True when the R3 criteria panel is switched on (CVar Eclipse.Gauntlet.Overlay). */
	bool IsGauntletPanelVisible() const;

	/** Read the five criteria out of the layers that own them. */
	EclipseGauntletOverlay::FEclipseGauntletCriteria GatherCriteria() const;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Root;

	/** Objectives + squad orders + Command Mode state: the per-fact rebuild lives in here and nowhere else. */
	UPROPERTY()
	TObjectPtr<UVerticalBox> LiveBox;

	UPROPERTY()
	TObjectPtr<UVerticalBox> GauntletPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ControlsPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> PlaytestPanel;

	/** One row per verdict line (title + 5 criteria + tally), created once. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> GauntletRows;

	/** One row per playtest block line (header + 5 statements + gate), created once. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> PlaytestRows;

	/** Column cells of the control table (index 0 = the column header), for the active-device highlight. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> MouseKeyboardCells;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> ControllerCells;

	/** Three family subscriptions instead of the whole Event tree: the HUD shows mission, squad and command facts, so nothing else may cost it a rebuild. */
	TArray<FEclipseEventSubscriptionHandle> EventHandles;

	FDelegateHandle DeviceChangedHandle;

	/** Manual criterion state (the only state this widget owns — nothing else knows the tester's answers). */
	int32 CleanPicks = 0;
	int32 MisPicks = 0;
	EclipseGauntletOverlay::EEclipseGauntletAnswer ComfortAnswer = EclipseGauntletOverlay::EEclipseGauntletAnswer::Unanswered;
	EclipseGauntletOverlay::EEclipseGauntletAnswer ConfidenceAnswer = EclipseGauntletOverlay::EEclipseGauntletAnswer::Unanswered;
	TArray<EclipseGauntletOverlay::EEclipseGauntletAnswer> PlaytestAnswers;

	bool bControlsVisible = false;
	bool bPlaytestVisible = false;

	/** Wall-clock stamp of the last in-place refresh (throttle; never the dilated clock). */
	double LastGauntletRefreshWallSeconds = 0.0;

	IConsoleObject* SummaryCommand = nullptr;
};
