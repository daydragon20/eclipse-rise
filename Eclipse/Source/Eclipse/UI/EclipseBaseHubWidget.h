#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EclipseBaseHubWidget.generated.h"

class UButton;
class UOverlay;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class UEclipseStrategyMapWidget;

/**
 * The menu-only base (SPEC-P1-08, GDD 13.2): four tabs — Command (map +
 * advance day), Workshop (the one production choice), Barracks (squad pick),
 * Memorial (the wall's list stub) — plus the preparation and debrief panels.
 * Debug-grade by design (GDD 14.5 step 4); the *architecture* is the real
 * deliverable: every displayed number originates from CampaignState via
 * events, and every action goes through a subsystem API.
 */
UCLASS()
class ECLIPSE_API UEclipseBaseHubWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * De tabbladen bij naam, zodat een opnameronde niet op een indexgetal hoeft
	 * te gokken. Een tab toevoegen verschuift elke index erna, en een
	 * opnameronde die dan stil de verkeerde tab fotografeert is erger dan een
	 * die niets fotografeert.
	 */
	enum class ETab : int32
	{
		Command = 0,
		Facilities = 1,
		Workshop = 2,
		Barracks = 3,
		Memorial = 4
	};

	/** Zet het actieve tabblad (opnameronde en toekomstige invoerbinding). */
	void SelectTab(ETab Tab);

#if !UE_BUILD_SHIPPING
	/**
	 * Duw de PRESENTATIETOESTAND uit `EclipseBaseView::MakeReviewView` in het
	 * raster, zodat één opname alle vijf de tegelvormen draagt. Nooit gecommit,
	 * nooit opgeslagen; alleen de opnameronde roept dit aan. De eerstvolgende
	 * `RefreshFacilities()` overschrijft hem weer met de echte campagne.
	 */
	void ShowReviewGridForShot();
#endif

	/** De boom, vóór RebuildWidget hem pakt — zie de toelichting in de implementatie. */
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void RefreshAll();
	void RefreshHeader();

	/**
	 * Het basisscherm van `REFERENTIE_BASE_MAP.md` §2.3: slotraster, bouwvoortgang,
	 * rushprijs, bemanning, energiebalans en schade. Leest één keer
	 * `UEclipseBaseSubsystem::ComposeBaseView` en verdeelt die view over het
	 * geverfde raster en de tekstbanden — de widget rekent niets uit.
	 */
	void RefreshFacilities();

	void RefreshWorkshop();
	void RefreshBarracks();
	void RefreshMemorial();
	void RefreshPrepPanel();
	void OnAnyFact(FGameplayTag EventTag, const FInstancedStruct& Payload);

	void HandleTab(int32 TabIndex);
	/**
	 * Eén afhandeling voor elke knopdruk: loggen, de reden tonen of wissen, en
	 * verversen. Bestaat omdat 'volgende dag' zijn uitkomst weggooide en dus stil
	 * niets deed — met deze helper valt dat niet meer te vergeten.
	 */
	bool NoteActionResult(bool bSucceeded, const FString& Error, const TCHAR* What);

	void HandleAdvanceDay();
	void HandleProduce(FName ItemId);
	void HandleToggleSquadPick(const FGuid& SoldierId);
	void HandleIntelReveal();
	void HandleLaunch(FGameplayTag LoadoutTag);

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> HeaderText;

	/**
	 * Platen onder de twee tekstgroepen die de hub zelf bezit. De kaart draagt
	 * zijn eigen plaat (die hoort bij het bord, niet bij de hub) en de knoppen
	 * dragen hun inkt in hun stijl. Zie `UI/EclipseScreenPlate.h` voor de meting.
	 */
	UPROPERTY()
	TObjectPtr<UOverlay> HeaderPlate;

	UPROPERTY()
	TObjectPtr<UOverlay> PrepPlate;

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY()
	TObjectPtr<UVerticalBox> CommandTab;

	UPROPERTY()
	TObjectPtr<UVerticalBox> WorkshopTab;

	UPROPERTY()
	TObjectPtr<UVerticalBox> BarracksTab;

	UPROPERTY()
	TObjectPtr<UVerticalBox> MemorialTab;

	/** De FACILITIES-tab: het geverfde slotraster met de banden eronder (§2.3). */
	UPROPERTY()
	TObjectPtr<UVerticalBox> FacilitiesTab;

	/** De tekstbanden onder het raster; eigen doos zodat het raster niet elke refresh opnieuw wordt gebouwd. */
	UPROPERTY()
	TObjectPtr<UVerticalBox> FacilitiesBands;

	UPROPERTY()
	TObjectPtr<class UEclipseBaseGridWidget> FacilitiesGrid;

	UPROPERTY()
	TObjectPtr<UVerticalBox> PrepPanel;

	UPROPERTY()
	TObjectPtr<UEclipseStrategyMapWidget> MapWidget;

	/** Barracks squad pick for the next mission (SPEC-P1-08). */
	TArray<FGuid> PickedSquad;

	/** Debrief summary lines assembled from commit events (GDD 7.6: every changed number with its cause). */
	TArray<FString> DebriefLines;
	bool bCollectingDebrief = false;

	/** Last rejected action's reason, shown in the prep panel — a refused button must say why (SPEC-P1-08 DoD). */
	FString LastActionNote;

	FEclipseEventSubscriptionHandle EventsHandle;
};
