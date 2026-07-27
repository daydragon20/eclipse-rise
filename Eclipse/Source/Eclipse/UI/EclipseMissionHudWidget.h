#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "CoreMinimal.h"
#include "UI/EclipseGauntletOverlayLogic.h"
#include "UI/EclipseTestGuideLogic.h"
#include "EclipseMissionHudWidget.generated.h"

class IConsoleObject;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;

/**
 * Debug-grade in-mission HUD (SPEC-P1-05 objective list + SPEC-P1-06 order-state
 * widget) and — since the P2-02 feel gauntlet — the single in-game debug overlay:
 * control overview (F2), the five R3 criteria, the 13.2 playtest checklist (H)
 * and the in-game test guide (F3). One widget on purpose: a second overlay would
 * draw the same facts twice. No art, the readout *is* the deliverable (14.5 step 4).
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
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual void NativeDestruct() override;

	/**
	 * May the debug HUD render at all? False during a -EclipseShot review round:
	 * screenshots judge the art, and debug text in the frame has been mistaken for
	 * shipped UI in review rounds (15.8/15.9). Checked at the mount site AND here,
	 * so no creation path can leak it into a still.
	 */
	static bool IsDebugHudAllowed();

	/** F2 — control overview; H — 13.2 playtest checklist; F3 — the in-game test guide. Driven by the controller's debug bindings. */
	void ToggleControlsPanel();
	void TogglePlaytestPanel();
	void ToggleGuidePanel();

	/**
	 * Test guide, variant A (phase0/INGAME_TESTGIDS.md BESLOTEN block): J settles
	 * the active step positively ("gehaald" / "goed" / "ja"), N negatively ("sla
	 * over" / "niet goed" / "nee"). Both no-op while the guide panel is closed —
	 * a closed panel steals no key, exactly like the gauntlet's manual keys.
	 */
	void ConfirmGuideStep();
	void SkipGuideStep();

	/**
	 * One of the player controller's EXISTING input actions fired. The guide
	 * listens next to the real handler and never consumes, blocks or rewrites it;
	 * with the panel closed this is a single bool test and nothing else happens.
	 */

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
	void OnHitLanded(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnAnyFact(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Rebuild the live sections only (objectives, orders, Command Mode state). */
	void Rebuild();

	/** Build the panels whose rows never change identity — once, at construct. */
	void BuildStaticPanels();

	/** In-place refresh of the gauntlet rows; wall-clock throttled, and free when the panel is hidden. */
	void RefreshGauntletRows(bool bForce);

	void RefreshPlaytestRows();

	/** In-place refresh of the guide rows; wall-clock throttled, and free while the guide is hidden. */
	void RefreshGuideRows(bool bForce);

	/** A settled guide step: refresh, mirror deel-3 answers into the 13.2 rows, and archive once when the list runs out. */
	void OnGuideStepSettled();

	/** Criterion 1's measurement as one short phrase for the guide's responsiveness row; empty while nothing is measured. */
	FString DescribeOrderRoundTrip() const;

	/** Mark the column of the device the player last touched (Enhanced Input's device subsystem tells us; no polling). */
	void RefreshDeviceHighlight();

	void ApplyPanelVisibility();

	/** True when the R3 criteria panel is switched on (CVar Eclipse.Gauntlet.Overlay). */
	bool IsGauntletPanelVisible() const;

	/** True while the test guide is on screen (CVar Eclipse.Guide.Overlay starts it open; F3 toggles). */
	bool IsGuidePanelVisible() const { return bGuideVisible; }

	/** Read the five criteria out of the layers that own them. */
	EclipseGauntletOverlay::FEclipseGauntletCriteria GatherCriteria() const;

	UPROPERTY()
	TObjectPtr<UVerticalBox> Root;

	/** Canvas-wortel: draagt de tekstlijst linksboven en de hitmarker in het midden. */
	UPROPERTY()
	TObjectPtr<class UCanvasPanel> Canvas;

	/** Trefferbevestiging in het schermmidden; verborgen tot er iets geraakt wordt. */
	UPROPERTY()
	TObjectPtr<class UTextBlock> HitMarker;

	/**
	 * MUNITIETELLER (26-07 avond). Een magazijn dat je niet ziet is geen mechaniek
	 * maar een verrassing: je klikt, er gebeurt niets, en je weet niet of je leeg
	 * bent of dat er iets stuk is. Rechtsonder, want daar staat hij in elke
	 * shooter sinds Doom — dat is geen smaak maar waar het oog hem zoekt.
	 */
	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> AmmoReadout;

	void RefreshAmmoReadout();

	/** Hoe lang de marker nog zichtbaar blijft (seconden). */
	float HitMarkerSecondsLeft = 0.0f;

	/** Richtingsindicator uit Screen_Damage_Indicator; null als de pack ontbreekt. */
	UPROPERTY()
	TObjectPtr<class UUserWidget> DamageIndicator;

	float DamageIndicatorSecondsLeft = 0.0f;

	/** Draai de indicator naar de plek waar de klap vandaan kwam. */
	void ShowDamageFrom(const FVector& ImpactPoint);

	/** Objectives + squad orders + Command Mode state: the per-fact rebuild lives in here and nowhere else. */
	UPROPERTY()
	TObjectPtr<UVerticalBox> LiveBox;

	UPROPERTY()
	TObjectPtr<UVerticalBox> GauntletPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ControlsPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> PlaytestPanel;

	UPROPERTY()
	TObjectPtr<UVerticalBox> GuidePanel;

	/** One row per verdict line (title + 5 criteria + tally), created once. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> GauntletRows;

	/** One row per playtest block line (header + 5 statements + gate), created once. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> PlaytestRows;

	/** One row per guide panel line (header + 20 steps + tally), created once. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> GuideRows;

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

	/** Zodat de HUD zich EEN keer bekendmaakt en niet elke frame. */
	bool bLoggedAmmoState = false;
	int32 MisPicks = 0;
	EclipseGauntletOverlay::EEclipseGauntletAnswer ComfortAnswer = EclipseGauntletOverlay::EEclipseGauntletAnswer::Unanswered;
	EclipseGauntletOverlay::EEclipseGauntletAnswer ConfidenceAnswer = EclipseGauntletOverlay::EEclipseGauntletAnswer::Unanswered;
	TArray<EclipseGauntletOverlay::EEclipseGauntletAnswer> PlaytestAnswers;

	/**
	 * Test-guide state (variant A). Progress is the only thing the guide owns; the
	 * steps themselves are pure data and the detection comes from the controller's
	 * existing input actions.
	 */
	EclipseTestGuide::FEclipseGuideProgress GuideProgress;

	bool bControlsVisible = false;
	bool bPlaytestVisible = false;
	bool bGuideVisible = false;

	/** The completion summary is written once per mount, not once per keypress after the last step. */
	bool bGuideSummaryEmitted = false;

	/** Wall-clock stamp of the last in-place refresh (throttle; never the dilated clock). */
	double LastGauntletRefreshWallSeconds = 0.0;
	double LastGuideRefreshWallSeconds = 0.0;

	IConsoleObject* SummaryCommand = nullptr;
};
