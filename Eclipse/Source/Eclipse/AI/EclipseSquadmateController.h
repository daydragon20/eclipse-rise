#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
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
	EclipseSquadOrderLogic::FEclipseOrderDecision ExecuteOrder(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor);

	/** Self-preservation tiers 1-2 stub (GDD 9.5 priority stack): stop and report when the body goes down. */
	void HandlePawnDowned();

	EEclipseSquadOrder GetCurrentOrder() const { return CurrentOrder; }

private:
	EclipseSquadOrderLogic::FEclipseOrderWorldFacts GatherFacts(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor) const;

	/**
	 * Cover-biased destination near the ordered point: ring samples scored by
	 * whether geometry blocks the nearest hostile's line of fire — the same
	 * scoring enemies will use (GDD 8.3 fairness rule).
	 * PLACEHOLDER(GDD 12.1): becomes an EQS query in the content pass.
	 */
	FVector SelectCoverPointNear(const FVector& OrderedLocation) const;

	EEclipseSquadOrder CurrentOrder = EEclipseSquadOrder::Hold;
};
