#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "Characters/EclipseClassLogic.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Squad/EclipseSquadTypes.h"
#include "EclipseSquadmateController.generated.h"

class AEclipseCharacter;

/**
 * Squadmate order execution (SPEC-P1-06). The controller gathers world facts
 * (nav path, line of sight, target validity), the pure decision table rules,
 * and the squad subsystem broadcasts the answer — competence with a voice
 * (GDD 9.5).
 *
 * // PLACEHOLDER(GDD 12.1): behavior moves into BT assets + EQS cover queries
 * // in the content pass; the decision contract stays in EclipseSquadOrderLogic.
 */
UCLASS()
class ECLIPSE_API AEclipseSquadmateController : public AAIController
{
	GENERATED_BODY()

public:
	/**
	 * Evaluate and (if accepted) execute an order. Always returns a decision —
	 * the caller turns it into an Acknowledged/Refused event; silence cannot
	 * happen by construction (GDD 8.4).
	 */
	EclipseSquadOrderLogic::FEclipseOrderDecision ExecuteOrder(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor, EEclipseSquadStance Stance = EEclipseSquadStance::Ready);

	/**
	 * Stand-down on down: stop and clear focus when the body goes down — the
	 * SPEC-P1-07 death transition, not self-preservation.
	 * PLACEHOLDER(SPEC-P1-06): conscious self-preservation tiers 1-2 (GDD 9.5
	 * priority stack) still pending — belongs in the BT/EQS content pass.
	 */
	void HandlePawnDowned();

	EEclipseSquadOrder GetCurrentOrder() const { return CurrentOrder; }

	/**
	 * Apply this soldier's resolved class kit (SPEC-P2-01). Pure data over the
	 * shared body (GDD 12.3): modulates order execution (push distance, cover
	 * lane bias) and enables auto-triage — never a divergent behavior class.
	 */
	void ApplyClassKit(const EclipseClassLogic::FEclipseResolvedClassKit& Kit) { ClassKit = Kit; }
	const EclipseClassLogic::FEclipseResolvedClassKit& GetClassKit() const { return ClassKit; }

	/**
	 * Auto-triage (GDD 9.5, SPEC-P2-01: the stub becomes real for kits with
	 * bAutoTriage): move to the downed body; arrival attempts the stabilize
	 * through the squad subsystem. Event-driven — OnMoveCompleted, no tick
	 * (GDD 12.4). Returns true when the run started.
	 */
	bool BeginTriage(AEclipseCharacter* DownedBody);

protected:
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	EclipseSquadOrderLogic::FEclipseOrderWorldFacts GatherFacts(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor) const;

	/**
	 * Cover-biased destination near the ordered point: ring samples scored by
	 * whether geometry blocks the nearest hostile's line of fire — the same
	 * scoring enemies will use (GDD 8.3 fairness rule).
	 * PLACEHOLDER(GDD 12.1): becomes an EQS query in the content pass.
	 */
	FVector SelectCoverPointNear(const FVector& OrderedLocation) const;

	/** Active squad tuning (cover/acceptance data) via the squad subsystem; null -> code defaults. */
	const class UEclipseSquadTuningAsset* ResolveTuning() const;

	/** Triage arrival: hand the stabilize attempt to the squad subsystem (it owns events + the window check). */
	void FinishTriage();

	EEclipseSquadOrder CurrentOrder = EEclipseSquadOrder::Hold;

	/** Last move stance (SPEC-P1-06 stub; no behavior split yet — PLACEHOLDER in the enum). */
	EEclipseSquadStance CurrentStance = EEclipseSquadStance::Ready;

	/** Resolved class kit (classless defaults until the game mode applies one). */
	EclipseClassLogic::FEclipseResolvedClassKit ClassKit;

	/** Active triage run: the body being saved + the move request that carries us there. */
	TWeakObjectPtr<AEclipseCharacter> TriageTarget;
	FAIRequestID TriageMoveId;
};
