#pragma once

#include "AIController.h"
#include "Characters/EclipseCharacterTypes.h"
#include "CoreMinimal.h"
#include "EclipseEnemyController.generated.h"

class AEclipseCharacter;

/**
 * Dummy enemy (SPEC-P1-05: "basic idle/patrol -> attack on perception; no
 * coordinator yet" — enough to make squad orders meaningful, not a combat
 * showcase). Perception is a timer-cadenced distance + line-of-sight check.
 *
 * // PLACEHOLDER(GDD 9.1/12.1): replaced by the shared perception component +
 * // BT assets in the AI content pass; archetype numbers already come from
 * // DT_EnemyArchetypes (GDD 14.2).
 */
UCLASS()
class ECLIPSE_API AEclipseEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void ApplyArchetype(const FEclipseEnemyArchetypeRow& Row);

private:
	/** One perception/attack beat; cadence = archetype fire interval (timer, never tick — GDD 14.2). */
	void SenseAndAct();
	AEclipseCharacter* FindNearestVisibleHostile() const;

	FEclipseEnemyArchetypeRow Archetype;
	FTimerHandle ThinkTimer;
};
