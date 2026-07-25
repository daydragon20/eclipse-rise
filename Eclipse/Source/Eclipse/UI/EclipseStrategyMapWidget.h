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
 * The 6-node district board, debug-grade (SPEC-P1-04): region list colored by
 * owner, one offer button per legal target, context line included. Reads
 * CampaignState only; consequence visibility comes free by re-rendering on
 * Event.Strategy.RegionControlChanged — "the map noticed" beat.
 */
UCLASS()
class ECLIPSE_API UEclipseStrategyMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void Rebuild();
	void OnBoardChanged(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void HandleOfferPicked(FName RegionId);

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox;

	FEclipseEventSubscriptionHandle RegionChangedHandle;
	FEclipseEventSubscriptionHandle BeatReachedHandle;
};
