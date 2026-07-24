#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Squad/EclipseSquadTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "EclipseSquadSubsystem.generated.h"

class AEclipseCharacter;
class AEclipseSquadmateController;
class IConsoleObject;

/**
 * Order dispatch for the deployed squad (SPEC-P1-06; squad of 4 per SPEC-P2-01).
 * Every issued order produces exactly one Acknowledged or Refused event with a
 * bark line — the zero-silence contract (GDD 8.4) is enforced here by
 * construction: the controller API returns a decision synchronously, and this
 * subsystem always broadcasts it. Downs additionally dispatch the data-driven
 * auto-triage response (SPEC-P2-01).
 */
UCLASS()
class ECLIPSE_API UEclipseSquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Register a spawned squadmate (game mode wiring at mission start). */
	void RegisterSquadmate(AEclipseSquadmateController* Controller, const FGuid& SoldierId);
	void UnregisterAll();

	/** Issue to one squadmate by roster id; returns false only for unknown ids (unknown = caller bug, not refusal). */
	bool IssueOrder(const FGuid& SoldierId, EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor, EEclipseSquadStance Stance = EEclipseSquadStance::Ready);

	/** Issue to everyone (the Phase 1 hotkey path). */
	void IssueOrderToAll(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor, EEclipseSquadStance Stance = EEclipseSquadStance::Ready);

	/**
	 * A triage run reached its casualty (SPEC-P2-01): attempt the stabilize with
	 * the *arriving soldier's* window data and, on success, broadcast
	 * Event.Squad.SoldierStabilized + Event.Squad.ClassAbilityUsed with a bark —
	 * the save is spoken, like every squad answer (GDD 9.5).
	 */
	void NotifyTriageArrived(AEclipseSquadmateController* Arriver, AEclipseCharacter* DownedBody);

	/**
	 * A finished triage run freed its responder: re-scan for casualties that
	 * went down mid-run and can still be saved (SPEC-P2-01 — the second down
	 * never waits forever). Gated on the mission peek so an already-saved or
	 * window-closed body cannot shuttle responders in a loop.
	 */
	void DispatchPendingTriage();

	int32 GetSquadmateCount() const { return Squadmates.Num(); }

	/** Per-squadmate "id: order" lines for the mission HUD (SPEC-P1-06 order-state widget). */
	TArray<FString> GetOrderStateLines() const;

	/** Active squad tuning asset (cover/acceptance/timeout data); null degrades to code defaults (GDD 14.3.5). */
	const UEclipseSquadTuningAsset* ResolveTuning() const;

private:
	struct FSquadmateEntry
	{
		TWeakObjectPtr<AEclipseSquadmateController> Controller;
		FGuid SoldierId;
	};

	void BroadcastOrderEvent(const FGameplayTag& Tag, const FGuid& SoldierId, EEclipseSquadOrder Order, const FString& BarkLine, EEclipseOrderRefusalReason Reason);

	/** Dispatch the first triage-capable free squadmate to this casualty (data decides who — GDD 14.2). */
	void DispatchTriage(AEclipseCharacter* DownedBody);

	/** Widest StabilizeWindowSeconds across registered triage kits — the re-dispatch peek bound. */
	float WidestTriageWindowSeconds() const;

	TArray<FSquadmateEntry> Squadmates;
	IConsoleObject* DumpCommand = nullptr;
};
