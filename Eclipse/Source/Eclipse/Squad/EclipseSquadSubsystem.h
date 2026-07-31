#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Squad/EclipseSquadTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "EclipseSquadSubsystem.generated.h"

class AActor;
class AEclipseCharacter;
class AEclipsePlayerController;
class AEclipseSquadmateController;
class IConsoleObject;
class UEclipseCommandModeTuningAsset;

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

	/** Registered squadmates whose body is up, in stable registration order (Command Mode selection — SPEC-P2-02). */
	TArray<FGuid> GetAliveSquadmateIds() const;

	/** True when the id belongs to a registered squadmate (selection filter: enemies/player never selectable). */
	bool IsRegisteredSquadmate(const FGuid& SoldierId) const;

	/** Per-squadmate "id: order" lines for the mission HUD (SPEC-P1-06 order-state widget). */
	TArray<FString> GetOrderStateLines() const;

	/**
	 * Wall-clock order round-trip tally (R3 criterion 1 of the Stage A feel
	 * gauntlet). It lives here because both halves of the round trip fall in
	 * IssueOrder — the issue and the ack/refusal that answers it — so no consumer
	 * has to guess which broadcast belongs to which order, and no consumer can
	 * accidentally time it on the dilated clock. The debug HUD only reads it.
	 */
	const EclipseSquadOrderLogic::FEclipseOrderRoundTripStats& GetOrderRoundTripStats() const { return OrderRoundTrip; }

	/** Clear the tally (run-scoped; called by UnregisterAll and the console). */
	void ResetOrderRoundTripStats() { OrderRoundTrip.Reset(); }

	/** Active squad tuning asset (cover/acceptance/timeout data); null degrades to code defaults (GDD 14.3.5). */
	const UEclipseSquadTuningAsset* ResolveTuning() const;

	// ======================================================================
	// SPEC-P2-02 Stage B — de vijf nieuwe verbs, op het BESTAANDE ordercontract.
	//
	// Er komt geen tweede orderpad bij. Alles hieronder is ofwel een FEIT dat de
	// beslissingstabel nodig heeft (concealment, markeringen, breekpunten), ofwel
	// een klaarstaande order die nog geen uitvoering is (flank, marks). Uitvoeren
	// blijft IssueOrder → ExecuteOrder → Acknowledged/Refused, één keer, met bark.
	// ======================================================================

	/** DA_CommandModeTuning (dezelfde die de mode-component leest, zie EclipseCommandMode::DefaultTuningPath); null = code-defaults. */
	const UEclipseCommandModeTuningAsset* ResolveCommandTuning() const;

	/**
	 * Markeer/ontmarkeer een doel voor de sync strike (8.4: "up to 4 marked").
	 * Geeft false als de druk niets deed — en alleen dan, want dat is het enige
	 * geval dat een zin verdient ("that's four already, boss").
	 */
	bool ToggleSyncStrikeMark(AActor* Target, bool& bOutMarked);

	int32 GetSyncStrikeMarkCount() const { return SyncMarks.Num(); }
	TArray<AActor*> GetSyncStrikeTargets() const;
	void ClearSyncStrikeMarks();

	/**
	 * Mag dit doel nog stil worden uitgeschakeld? (Leeft, staat, en heeft ons
	 * NIET opgemerkt.) De sync strike is per SPEC-P2-02 alleen geldig tegen
	 * onwetende vijanden; wie ons doorheeft is een gewoon vuurgevecht.
	 */
	bool IsSyncStrikeTargetStillValid(const AActor* Target) const;

	/** De doelen die déze soldaat toebedeeld krijgt bij het uitvoeren (rond verdeeld, deterministisch). */
	TArray<AActor*> GetAssignedSyncStrikeTargets(const FGuid& SoldierId) const;

	/**
	 * Wordt dit lichaam op dit moment door een vijand GEZIEN? Eén definitie van
	 * "ongezien" voor de hele build: dezelfde waarnemingstoets die de vijand
	 * gebruikt om te schieten.
	 */
	bool IsBodyConcealed(const AActor* Body) const;

	/**
	 * Weet de vijand dat we er zijn? (Iemand heeft ons gezien, of het alarm staat
	 * aan.) De tweede poort van de stealth-doctrine — vanaf hier koopt zwijgen
	 * niets meer.
	 */
	bool IsEnemyAwareOfSquad() const;

	/** De vaste stapelplek van deze soldaat bij een breach (index in de registratievolgorde). */
	int32 GetSquadmateSlot(const FGuid& SoldierId) const;

	/**
	 * Een soldaat staat op zijn stapelplek. Als iedereen die op dit breekpunt
	 * beval er staat, gaan ze TEGELIJK naar binnen — dat is wat 8.4 met
	 * "synchronized entry" bedoelt op sliceschaal: geen choreografie, wel samen.
	 */
	void NotifyBreachStacked(AEclipseSquadmateController* Arriver);

	/**
	 * DE ENIGE zender van Event.Squad.OrderQueued (catalogusregel: één emitter per
	 * feit). De controller meldt zijn flank-overgangen hierlangs in plaats van
	 * zelf te broadcasten — anders ontstaat er een tweede pad naar dezelfde tag en
	 * is "één ordercontract" nog maar een bewering.
	 */
	void BroadcastOrderQueued(const FGuid& SoldierId, FName Transition, const FString& BarkLine, FName TargetId = NAME_None);

	/** Een gesproken regel uit DT_SquadOrderDefs voor een klaarstaande order; leeg pool = stockzin, nooit stilte. */
	FString ResolveQueuedBark(FName RowName, const FGuid& SoldierId, uint32 Salt) const;

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

	/** R3 criterion 1 tally, reset per run (UnregisterAll runs at mission teardown). */
	EclipseSquadOrderLogic::FEclipseOrderRoundTripStats OrderRoundTrip;

	// ---- SPEC-P2-02 Stage B state ---------------------------------------
	/**
	 * De markeringen als pure set (id's), plus de brug naar de lichamen. Twee
	 * containers en niet één map: de pure laag mag geen actoren kennen, en de cap-
	 * en snoeiregels horen daar te staan waar de test ze headless kan afgaan.
	 */
	EclipseSquadOrderLogic::FEclipseSyncStrikeMarkSet SyncMarks;
	TMap<FGuid, TWeakObjectPtr<AActor>> MarkedBodies;

	/**
	 * Snoeiklok, alleen actief zolang er markeringen staan (12.4: buiten de sync
	 * strike nul werk per frame). Op een timer en niet op tick, en niet lui bij
	 * het uitvoeren: een pip die blijft staan terwijl het doel allang omgevallen
	 * is, is een leugen op het scherm — en de speler rekent erop.
	 */
	FTimerHandle MarkPruneTimer;
	void PruneSyncStrikeMarks();

	/** 14.3.5: één melding per ontbrekende weigerpool, niet één per order. */
	TSet<FName> WarnedMissingRefusalPools;

	IConsoleObject* DumpCommand = nullptr;

	/** Debug-drivers (14.5 stap 4): de vijf verbs, de doctrine en de markeringen vanaf de console. */
	TArray<IConsoleObject*> StageBCommands;
	void RegisterStageBConsole();
	AEclipsePlayerController* GetPlayerController() const;
};
