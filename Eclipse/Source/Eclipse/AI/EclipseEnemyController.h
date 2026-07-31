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

	/**
	 * Er is geschoten binnen gehoorsafstand (26-07, punt 1).
	 *
	 * Wat hij krijgt is de LAATST BEKENDE POSITIE, niet waar je nu bent — een
	 * momentopname van waar de loop stond. Dat is het model van The Division, en
	 * het is precies wat een sniper zijn afstandsvoordeel laat houden: ze komen
	 * kijken waar je WAS, en jij kunt er weg zijn. Blijf je staan, dan vinden ze
	 * je alsnog, en dat hoort ook.
	 */
	void NotifyGunshotHeard(const FVector& ShotOrigin);

	/**
	 * Heeft deze vijand onze kant OOIT gezien? (SPEC-P2-02 Stage B.)
	 *
	 * Dit is het feit waar de stealth-stance en de sync strike op draaien: zolang
	 * niemand ons heeft opgemerkt is stil blijven iets waard, en daarna niet meer.
	 * Een eigen veld en niet `bLoggedFirstContact`, hoe verleidelijk ook — dat is
	 * een LOGVLAG, en gedrag ophangen aan een logvlag betekent dat het gedrag
	 * verdwijnt zodra iemand het loggen opruimt.
	 */
	bool HasSeenPlayerSide() const { return bHasSeenPlayerSide; }

	/**
	 * Ziet deze vijand dit lichaam NU? Dezelfde regel als SenseAndAct gebruikt
	 * (waarnemingsstraal uit het archetype + zichtlijn) — één waarnemingsdefinitie,
	 * want twee zouden betekenen dat een soldaat "ongezien" is voor de sync strike
	 * terwijl de vijand al op hem schiet.
	 */
	bool CanSeeBody(const AActor* Body) const;

private:
	/** One perception/attack beat; cadence = archetype fire interval (timer, never tick — GDD 14.2). */
	void SenseAndAct();
	AEclipseCharacter* FindNearestVisibleHostile() const;

	FEclipseEnemyArchetypeRow Archetype;
	FTimerHandle ThinkTimer;

	/** Eén regel per vijand over een mislukte verplaatsing; daarna zwijgt hij.
	 *  Een uitkomst die per denkbeurt herhaalt voegt na de eerste keer niets toe. */
	bool bLoggedMoveOutcome = false;

	/** Idem voor het eerste moment dat hij iemand ziet. */
	bool bLoggedFirstContact = false;

	/** Het FEIT achter die logregel: hij heeft onze kant gezien. Zie HasSeenPlayerSide. */
	bool bHasSeenPlayerSide = false;

	/** Laatst gehoorde schot; wordt gewist zodra hij er is of iemand ziet. */
	FVector InvestigateLocation = FVector::ZeroVector;
	bool bHasInvestigateLocation = false;

	/** Eén regel over de eerste onderzoekspoging. */
	bool bLoggedInvestigate = false;
};
