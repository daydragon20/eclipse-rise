#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Squad/EclipseSquadTypes.h"
#include "EclipsePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UEclipseBaseHubWidget;
class UEclipseMissionHudWidget;

/**
 * Player input (SPEC-P1-05/06): Enhanced Input only (GDD 12.1). Move/look/
 * fire/sprint/crouch plus the four order hotkeys — orders go to the squad at
 * the aim point (no time dilation in Phase 1; feel targets §4).
 *
 * // PLACEHOLDER(GDD 12.1): input objects are built in code until the input
 * // asset pass; bindings stay identical when they move to assets.
 */
UCLASS()
class ECLIPSE_API AEclipsePlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

private:
	void HandleMove(const struct FInputActionValue& Value);
	void HandleLook(const struct FInputActionValue& Value);
	void HandleFire();
	void HandleSprint(const struct FInputActionValue& Value);
	void HandleCrouch();
	void IssueSquadOrder(EEclipseSquadOrder Order);

	/** Boot a fresh campaign from data if none is running (SPEC-P1-08 live loop). */
	void EnsureCampaignStarted();

	/** Menu base presentation: show the hub, UI input, pawn parked. */
	void EnterBaseMode();

	/** Ground presentation: hub hidden, game input, mission HUD, inserted at the entry. */
	void EnterMissionMode();

	/** Mission lifecycle drives the base<->mission presentation swap. */
	void OnMissionEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	UPROPERTY()
	TObjectPtr<UEclipseBaseHubWidget> BaseHub;

	UPROPERTY()
	TObjectPtr<UEclipseMissionHudWidget> MissionHud;

	FEclipseEventSubscriptionHandle MissionEventsHandle;

	/** Aim point = camera-forward hitscan against world geometry (order target + fire direction). */
	bool GetAimPoint(FVector& OutLocation, AActor*& OutActor) const;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY()
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY()
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY()
	TArray<TObjectPtr<UInputAction>> OrderActions;
};
