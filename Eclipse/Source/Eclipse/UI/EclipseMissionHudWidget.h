#pragma once

#include "Blueprint/UserWidget.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "CoreMinimal.h"
#include "EclipseMissionHudWidget.generated.h"

class UVerticalBox;

/**
 * Debug-grade in-mission HUD (SPEC-P1-05 objective list + SPEC-P1-06 order-state
 * widget). Rebuilds from the mission + squad subsystems whenever a fact arrives on
 * the bus — no art, the readout *is* the deliverable (GDD 14.5 step 4).
 */
UCLASS()
class ECLIPSE_API UEclipseMissionHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void OnAnyFact(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void Rebuild();

	UPROPERTY()
	TObjectPtr<UVerticalBox> Root;

	FEclipseEventSubscriptionHandle EventsHandle;
};
