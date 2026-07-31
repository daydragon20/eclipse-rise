#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "CoreMinimal.h"
#include "EclipseStrategyMapWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/**
 * UButton that knows which region it launches (UMG buttons click without a
 * sender, so the payload lives on the button). Debug-grade UI (GDD 14.5 step 4).
 */
UCLASS()
class UEclipseStrategyOfferButton : public UButton
{
	GENERATED_BODY()

public:
	void Bind(FName InRegionId, TFunction<void(FName)> InOnPicked);

private:
	UFUNCTION()
	void HandleClicked();

	FName RegionId;
	TFunction<void(FName)> OnPicked;
};

/**
 * The district board (SPEC-P1-04, N-b): every region with its OWN LANES — status,
 * travel days, risk — the Dominion Response Tier, and one offer button per legal
 * target.
 *
 * Until N-b this widget read `FEclipseCampaignState::Regions` and nothing else,
 * and that structure has four fields and no edges. The topology existed one
 * header away and was never fetched, so the board could not show a single one of
 * GDD 3.1's four map rules. It now composes `EclipseStrategyMap::ComposeMapView`
 * over the region graph the routing layer reads, which is also why a
 * self-contradicting graph reaches a banner instead of the board.
 *
 * Pixels only: WHAT is on the board is decided in EclipseStrategyMapLogic and
 * tested headless (GDD 14.5). This file owns colour, order and spacing.
 *
 * Consequence visibility comes free by re-rendering on the whole
 * `Event.Strategy` family — RegionControlChanged, LiberationResolved and
 * ResponseTierChanged all land here, because the bus walks a tag's parent chain.
 */
UCLASS()
class ECLIPSE_API UEclipseStrategyMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** De wortel, vóór RebuildWidget hem pakt — zie de toelichting in de implementatie. */
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void Rebuild();
	void OnBoardChanged(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void HandleOfferPicked(FName RegionId);

	/**
	 * One board line. Ink outline on every line (15.5): a hard black offset drop
	 * makes text hold up against a light map plate the same way the world's
	 * outlines do — one visual language, not a separate one for the screen layer.
	 */
	UTextBlock* AddLine(const FText& Text, const FLinearColor& Color, int32 FontSize, bool bBold, float TopPadding = 0.0f);

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	FEclipseEventSubscriptionHandle RegionChangedHandle;
	FEclipseEventSubscriptionHandle BeatReachedHandle;
};
