#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
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
	/**
	 * De widget-BOOM wordt hier gebouwd, niet in NativeConstruct.
	 *
	 * UUserWidget::RebuildWidget maakt de Slate-boom uit WidgetTree->RootWidget en
	 * roept NativeConstruct pas DAARNA aan. Wie zijn wortel in NativeConstruct zet,
	 * is per definitie te laat: de Slate-kant is dan al een lege SSpacer en blijft
	 * dat. Zie de toelichting in de implementatie — dit is de oorzaak waar het
	 * ontbrekende richtkruis, de onzichtbare munitieteller en "F3 doet niets"
	 * alledrie op terugkomen.
	 */
	virtual void NativeOnInitialized() override;

	virtual void NativeConstruct() override;

	/**
	 * Dumpt wat de HUD toont: panelen, zichtbaarheid en regelaantallen, plus een
	 * FOUT-regel bij een toestand die de speler als kapot ervaart.
	 *
	 * Bestaat omdat de UI-laag met geen enkele opnamemethode vast te leggen is;
	 * zie Eclipse.UI.Report in BESTURING.md.
	 */
	void LogUiReport() const;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual void NativeDestruct() override;

	/**
	 * Mag de DEBUGLAAG renderen? False tijdens een -EclipseShot review-ronde:
	 * screenshots beoordelen de kunst, en debugtekst in het frame is in eerdere
	 * reviewrondes voor verscheepte UI aangezien (15.8/15.9).
	 *
	 * LET OP WAT DIT NIET IS: het is geen schakelaar op de HUD als geheel. De
	 * SPELERLAAG (BuildPlayerLayer) staat er altijd, ook in een opnameronde —
	 * anders is er per constructie geen beeldbewijs mogelijk van het enige deel van
	 * de HUD dat de speler ooit te zien krijgt.
	 *
	 * Merk ook op dat -EclipseShotPlay hier NIET onder valt: FParse::Param eist dat
	 * de vlag op een woordgrens eindigt, dus de speelronde met opnames draait met
	 * de volle HUD. Alleen de vaste-camera-reviewronde onderdrukt de debuglaag.
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

	// ---------------------------------------------------------------------------
	// MEETPUNTEN VAN DE MUNITIEHOEK — publiek, want een falsificatie die de HUD van
	// binnenuit moet openbreken, is geen falsificatie.
	// ---------------------------------------------------------------------------

	/**
	 * Hoe vaak het TEKENPAD van de munitiehoek werkelijk gelopen heeft.
	 *
	 * Dit is het getal waarmee "de HUD polt niet meer" een NUL wordt in plaats van
	 * een bewering: zestig frames zonder wapenfeit horen dit getal niet te bewegen.
	 * Zolang RefreshAmmoReadout in NativeTick stond, liep hij per definitie 60 keer
	 * per seconde op, en er was geen manier om dat verschil te meten.
	 */
	int32 GetAmmoDrawCount() const { return AmmoDrawCount; }

	/** Hoeveel Event.Player.WeaponStatusChanged deze HUD van de bus heeft aangenomen. */
	int32 GetWeaponStatusEventCount() const { return WeaponStatusEventsReceived; }

	/**
	 * Wat er NU in het magazijnveld staat — de tekst uit het WIDGET en niet uit de
	 * cache. Bewust die kant op: een test die de cache uitleest, bewijst dat de HUD
	 * het feit heeft ONTVANGEN, niet dat hij het heeft GETEKEND. Precies dat verschil
	 * is "het scherm mag nooit stiller zijn dan de bus".
	 */
	FString GetMagazineTextOnScreen() const;

	/** Idem voor de wapennaam; leeg als het veld verborgen staat. */
	FString GetWeaponTextOnScreen() const;

private:
	void OnHitLanded(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnAnyFact(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/**
	 * Event.Player.WeaponStatusChanged: de ENIGE bron van de munitiehoek.
	 *
	 * Neemt het feit over in de cache en tekent meteen — één frame na het feit, niet
	 * bij de volgende tick. De schermlaag rekent hier niets uit en vraagt het wapen
	 * niets; wie in deze functie een GetAmmoInMagazine() ziet verschijnen, kijkt naar
	 * een terugval naar het pollen (GDD 12.1/8.8: UI is consument).
	 */
	void OnWeaponStatusChanged(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Rebuild the live sections only (objectives, orders, Command Mode state). */
	void Rebuild();

	/** Build the panels whose rows never change identity — once, at construct. */
	void BuildStaticPanels();

	/**
	 * DE SPELERLAAG: richtkruis, trefteken, munitieteller, richtingsindicator.
	 *
	 * Alles hierin is SPELBESTURING en geen debugtekst, dus het staat VÓÓR de
	 * debug-poort en bestaat dus ook in een opnameronde. De poort eronder is er om
	 * debugtekst uit review-stills te houden — rijen, panelen, de gauntlet-teller —
	 * en niet om de speler zijn eigen HUD af te nemen.
	 *
	 * VIER DINGEN HOREN BIJ ELKAAR per element, en dat is waar een eerdere poging
	 * op strandde: constructie, ABONNEMENT, refresh en opruiming. Een munitieteller
	 * die je naar boven verplaatst zonder zijn bron toont eeuwig "30 / 30", en dat
	 * is erger dan geen teller. Zie SubscribePlayerEvents voor de abonnementen,
	 * NativeTick voor de refresh en NativeDestruct voor de opruiming.
	 */
	void BuildPlayerLayer();

	/**
	 * Het richtkruis: vier balken rond een gat, geen tekstglyph meer.
	 *
	 * GEMETEN op alle elf HUD-frames van 31-07: de oude '+' was 7x9 px met 17 pixels
	 * inkt (0,0018 % van het beeld) en stond 1 px boven het echte schermmidden — een
	 * tekstglyph is gecentreerd op zijn regelvak, niet op zijn inkt. Beide zijn met
	 * geometrie per constructie weg.
	 */
	void BuildCrosshair();

	/** Het kruis naar de huidige spreiding en het huidige wapen zetten; per frame, en gratis als er niets veranderde. */
	void RefreshCrosshair();

	/** SetVisibility alleen bij een echte verandering — het ongeldigt de layout en dit draait per frame (12.4). */
	static void SetVisibilityIfChanged(class UWidget* Widget, bool bVisible);

	/** De titel-veilige marge voor de huidige viewport (EclipseHudReadout::TitleSafeMarginPx). */
	FVector2D CurrentTitleSafeMarginPx() const;

	/**
	 * De marge PER FRAME toepassen, niet bij de bouw.
	 *
	 * GEMETEN in drie opnamerondes op 31-07: slotwaarde 12 gaf 9 px op het frame,
	 * 36 gaf 25 px, en 54 gaf 55 px. Eén DPI-schaal kan die drie niet verklaren —
	 * de schaal die bij NativeOnInitialized geldt is nog niet de definitieve, want
	 * de viewport heeft daar zijn maat nog niet. Een marge die op dat moment wordt
	 * ingebakken, is dus een getal uit een keten die nog niet af was.
	 *
	 * Dezelfde klasse fout als RebuildWidget-vóór-NativeConstruct, en dezelfde
	 * oplossing: niet één keer vroeg vastleggen, maar nakijken zolang het kan
	 * veranderen. Het lost er meteen een echte speler-situatie mee op — venster
	 * slepen en resolutie wisselen.
	 */
	void ApplySafeAreaLayout();

	/** Laatst toegepaste marge, zodat een frame zonder verandering niets ongeldigt. */
	FVector2D LastAppliedSafeMarginPx = FVector2D(-1.0, -1.0);

	/** Trefferbevestiging in het schermmidden; gedoofd door de teller in NativeTick. */
	void BuildHitMarker();

	/** De munitieteller rechtsonder; gevoed door RefreshAmmoReadout per tick. */
	void BuildAmmoReadout();

	/**
	 * De richtingsindicator uit Screen_Damage_Indicator.
	 *
	 * Stond tot 31-07 MIDDEN IN NativeTick, in de tak die het trefteken dooft: hij
	 * werd dus pas aangemaakt nadat je zelf iets geraakt had, en tot dat moment
	 * viel elke ShowDamageFrom stil op een null-pointer. Constructie hoort bij de
	 * constructie.
	 */
	void BuildDamageIndicator();

	/** De abonnementen van de spelerlaag: treffers (marker + richting) en de wapenstand. */
	void SubscribePlayerEvents();

	/**
	 * Het wapen vragen zijn huidige stand OPNIEUW uit te zenden, één keer bij montage.
	 *
	 * De bus levert synchroon af en bewaart niets, en deze widget wordt gemonteerd op
	 * Event.Mission.Started — ruim ná het feit dat bij de bezetting is verstuurd. Zonder
	 * dit verzoek zou de teller tot het eerste schot leeg blijven. Zie de implementatie
	 * voor waarom dit een VERZOEK is en geen uitlezing.
	 */
	void RequestInitialWeaponStatus();

	/**
	 * Donkere rand rond een lichte glyph, zodat hij op ELKE ondergrond leesbaar is.
	 *
	 * De eis "in eerste én derde persoon écht leesbaar" gaat niet over de camera
	 * maar over waar het teken toevallig overheen valt: dezelfde lichte '+' staat op
	 * een donkere muur prima en op een gele wegmarkering niet. Eén functie, zodat
	 * kruis, trefteken en teller nooit uit elkaar lopen.
	 */
	static void ApplyLegibilityOutline(class UTextBlock& Text, int32 OutlineSizePx = 1);

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

	/** Volvlaks laag waar de vier kruisbalken en het middenpunt in hangen. */
	UPROPERTY()
	TObjectPtr<class UCanvasPanel> CrosshairRoot;

	/** Boven, onder, links, rechts — in die volgorde. */
	inline static constexpr int32 CrosshairArmCount = 4;

	UPROPERTY()
	TArray<TObjectPtr<class UImage>> CrosshairArms;

	/** Donkere onderlaag per balk: dezelfde rol als de font-outline op de teksten, maar dan op geometrie. */
	UPROPERTY()
	TArray<TObjectPtr<class UImage>> CrosshairShadows;

	UPROPERTY()
	TObjectPtr<class UImage> CrosshairCentre;

	/** Donkere rand om het middenpunt; zonder die rand bestond het punt alleen in de code (GEZIEN 31-07). */
	UPROPERTY()
	TObjectPtr<class UImage> CrosshairCentreShadow;

	/** Laatst gezette maten, zodat een frame zonder verandering geen layout ongeldigt. */
	float LastCrosshairArmPx = -1.0f;
	float LastCrosshairGapPx = -1.0f;
	float LastCrosshairThicknessPx = -1.0f;

	/**
	 * MUNITIETELLER (26-07 avond). Een magazijn dat je niet ziet is geen mechaniek
	 * maar een verrassing: je klikt, er gebeurt niets, en je weet niet of je leeg
	 * bent of dat er iets stuk is. Rechtsonder, want daar staat hij in elke
	 * shooter sinds Doom — dat is geen smaak maar waar het oog hem zoekt.
	 */
	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> AmmoReadout;

	/**
	 * HET KADER OM DE MUNITIEHOEK, met een VASTE maat.
	 *
	 * Niet AutoSize, en dat is de reparatie van defect 4. GEZIEN op
	 * HUD_wapen_E_na_wissel.png: de wapenregel liep tot beeldkolom 1278 van 1279 en
	 * de "12 / 12" stond buiten beeld — niet door de offset (die kwam op 1232 uit)
	 * maar doordat een AutoSize-slot die rechts is uitgelijnd, één frame lang de
	 * GEWENSTE MAAT VAN DE VORIGE LAYOUT gebruikt. Elke wapenwissel maakte zo'n
	 * frame. Met een vast kader kan geen enkele tekst zijn eigen rand meer uit.
	 */
	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> AmmoBlock;

	/** De magazijnMAAT ("/ 30"): staat vast, dus kleiner en gedimd naast het getal dat wel beweegt. */
	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> AmmoCapacity;

	/** De LEESBARE wapennaam uit DT_Weapons.DisplayName (defect 3), niet de rijnaam. */
	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> WeaponReadout;

	/**
	 * "RELOADING" op zijn EIGEN regel — de reparatie van defect 1.
	 *
	 * Er stond één tekstveld dat tijdens de beurt volledig werd overschreven, dus
	 * precies wanneer je wilt weten hoeveel er straks in zit, stond het er niet.
	 * Twee feiten die tegelijk gelden, horen twee velden te zijn.
	 */
	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> ReloadReadout;

	/** Inktrand en vlak achter de munitiehoek (REFERENTIE_HUD_BORDERLANDS.md §1/§4.2). */
	UPROPERTY(Transient)
	TObjectPtr<class UImage> AmmoPanelInk;

	UPROPERTY(Transient)
	TObjectPtr<class UImage> AmmoPanelFill;

	UPROPERTY(Transient)
	TObjectPtr<class UImage> ReloadBarTrack;

	UPROPERTY(Transient)
	TObjectPtr<class UImage> ReloadBarFill;

	/** Maten van de munitiehoek. Vast, want de hele reparatie van defect 4 is dat dit blok niet meer meegroeit. */
	inline static constexpr float AmmoBlockWidthPx = 300.0f;
	inline static constexpr float AmmoBlockHeightPx = 120.0f;
	inline static constexpr float ReloadBarWidthPx = 150.0f;

	/** Rijen waarover al geklaagd is dat hun DisplayName ontbreekt — één keer melden, niet per frame. */
	TSet<FName> ReportedMissingDisplayNames;

	/**
	 * DE LAATSTE STAND ZOALS DE BUS HEM AFLEVERDE — de enige bron waaruit de
	 * munitiehoek tekent.
	 *
	 * Hier stond niets, en dat was het probleem: RefreshAmmoReadout haalde zeven
	 * feiten rechtstreeks uit het wapencomponent en NativeTick riep hem elke frame
	 * aan. Zestig keer per seconde een vraag stellen die zes keer per seconde een
	 * ander antwoord heeft, is niet alleen duur — het is een tweede pad naar dezelfde
	 * waarheid, en twee paden kunnen uit elkaar lopen (GDD 12.2 regel 2).
	 *
	 * Een KOPIE en geen pointer naar het wapen: zodra hier een verwijzing zou staan,
	 * kan de teken-functie er alsnog doorheen vragen en is de scheiding weg.
	 */
	UPROPERTY(Transient)
	FEclipseWeaponStatusPayload WeaponStatus;

	/** Is er ooit een wapenfeit binnengekomen? Zonder feit hoort de hele hoek weg te blijven. */
	bool bHasWeaponStatus = false;

	/** Aantal aangenomen wapenfeiten en aantal keren dat de hoek daadwerkelijk getekend is. */
	int32 WeaponStatusEventsReceived = 0;
	int32 AmmoDrawCount = 0;

	/** Eén keer melden dat een feit niet getekend kon worden; daarna zou het per feit spammen. */
	bool bReportedSilentScreen = false;

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
