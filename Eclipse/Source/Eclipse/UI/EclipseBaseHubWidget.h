#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EclipseBaseHubWidget.generated.h"

class UButton;
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
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildLayout();
	void RefreshAll();
	void RefreshHeader();
	void RefreshWorkshop();
	void RefreshBarracks();
	void RefreshMemorial();
	void RefreshPrepPanel();
	void OnAnyFact(FGameplayTag EventTag, const FInstancedStruct& Payload);

	void HandleTab(int32 TabIndex);
	void HandleAdvanceDay();
	void HandleProduce(FName ItemId);
	void HandleToggleSquadPick(const FGuid& SoldierId);
	void HandleIntelReveal();
	void HandleLaunch(FGameplayTag LoadoutTag);

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> HeaderText;

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
