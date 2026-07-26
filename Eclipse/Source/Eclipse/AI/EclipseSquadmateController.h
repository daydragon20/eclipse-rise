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

	/** Diagnostiek: hoeveel schoten deze soldaat op zijn FocusTarget-order heeft gelost. */
	int32 GetFocusFireShots() const { return FocusFireShots; }

	/** Diagnostiek: verplaatsingen die uit MEELOPEN kwamen, niet uit een order. */
	int32 GetFollowMoves() const { return FollowMoves; }

	/** Diagnostiek: schoten zonder order — het bewijs dat laag 2 draait. */
	int32 GetAutoFireShots() const { return AutoFireShots; }

	/** Diagnostiek: verplaatsingen naar dekking onder vuur — het bewijs van laag 3. */
	int32 GetCoverRuns() const { return CoverRuns; }

	/** Diagnostiek: hoe vaak het klasse-verb uit zichzelf iets deed (laag 5). */
	int32 GetVerbUses() const { return VerbUses; }

	/** Start de basislaag: meelopen. Aangeroepen zodra de soldaat een lichaam heeft. */
	void BeginFollowing(float FollowDistanceCm, float CoverSearchRadiusCm);

	/**
	 * De doctrine zetten (26-07 avond, laag 4). Geldt METEEN en blijft gelden tot
	 * je hem wisselt — het is een kader, geen orderparameter.
	 */
	void SetDoctrine(EEclipseSquadStance Stance);
	EEclipseSquadStance GetDoctrine() const { return CurrentStance; }

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

	/**
	 * Heeft deze soldaat OOIT een order gekregen? (26-07 avond, laag 1.)
	 *
	 * Nodig omdat `CurrentOrder` standaard op Hold staat — de beginstand uit het
	 * model van vóór vanavond, waarin de squad alleen op een order bewoog. Zonder
	 * dit onderscheid leest "niemand heeft me iets gezegd" als "ik ben op mijn
	 * post gezet", en dan loopt niemand ooit mee. Precies dat mat de eerste ronde:
	 * nul verplaatsingen, terwijl de code klopte.
	 */
	bool bHasStandingOrder = false;

	/**
	 * Het doelwit van een lopende FocusTarget-order, plus de klok die hem
	 * uitvoert (26-07). Zie ContinueFocusFire voor waarom dit er is.
	 */
	TWeakObjectPtr<AActor> FocusTargetActor;
	FTimerHandle FocusFireTimer;
	int32 FocusFireShots = 0;

	void ContinueFocusFire();

	/**
	 * MEELOPEN (owner-opdracht 26-07 avond, punt 1 — laag 1 van zes).
	 *
	 * De owner: "ze lopen mee ... Dat is geen feature die je aanzet, dat is de
	 * basis." Tot vandaag bewoog de squad alleen op een order, en stond
	 * `FollowDistance` in de data met een comment die zei dat hij niet gelezen
	 * werd.
	 *
	 * Op een timer en niet op een tick, precies zoals ContinueFocusFire: dit
	 * controleert een AFSTAND, en dat hoeft geen zestig keer per seconde.
	 *
	 * ORDERS BLIJVEN BELOFTES (8.4). Meelopen geldt alleen als er geen order
	 * staat die hem ergens houdt — een soldaat die je op een positie zette, hoort
	 * daar te blijven staan ook als jij wegloopt. Dat is het hele verschil tussen
	 * "de squad handelt zelf" en "de squad negeert je".
	 */
	void UpdateFollow();

	/**
	 * AUTONOOM VUREN (owner-opdracht 26-07 avond, punt 1 — laag 2 van zes).
	 *
	 * De owner: "ze vuren op wat ze zien". Tot vandaag vuurde een squadmate alleen
	 * op een expliciete FocusTarget-order; zonder order stond hij ernaast te kijken
	 * terwijl je beschoten werd.
	 *
	 * Dit is de grootste balansverschuiving die er ligt — drie extra schutters
	 * halveert hoe lang een groep vijanden overeind blijft — en daarom landt hij
	 * apart van laag 1, met een eigen meting.
	 */
	void UpdateEngagement();

	/**
	 * DEKKING ZOEKEN ONDER VUUR (26-07 avond, punt 1 — laag 3 van zes).
	 *
	 * De owner: "als er op ze geschoten wordt volgen ze hun training: dekking
	 * zoeken, verplaatsen, dekkingsvuur." `CoverSearchRadius` stond in de data met
	 * een comment die zei dat er geen dekkingzoekgedrag was.
	 *
	 * Op het TREFFER-feit en niet op een klok: dekking zoeken is een reactie, geen
	 * gewoonte. Een soldaat die elke halve seconde nadenkt over dekking loopt
	 * heen en weer; een die het doet als er op hem geschoten wordt, reageert.
	 */
	void HandleHitTaken(AEclipseCharacter* Shooter, float Amount);

	/** Tot wanneer een verse dekkingszoektocht niet nog eens mag starten. */
	double CoverCooldownUntil = -1.0;

	/** Recon: tot wanneer hij mag vuren omdat er op hem geschoten is. */
	double WeaponsFreeUntil = -1.0;

	/** Zoekstraal uit DA_SquadTuning; 0 = geen dekkingzoekgedrag. */
	float CoverSearchRadius = 0.0f;

	/** Diagnostiek: hoe vaak deze soldaat onder vuur naar dekking vertrok. */
	int32 CoverRuns = 0;

	/**
	 * KLASSE-VERBS (26-07 avond, punt 1 — laag 5 van zes).
	 *
	 * Momentum en Killzone bestonden als tag plus getal en werden nergens
	 * afgevuurd (squad-audit punt 10). Ze horen IN de basislaag en niet op een
	 * eigen knop: een Assault gebruikt Momentum wanneer zijn doctrine hem laat
	 * opsluiten, een Sniper zet Killzone wanneer hij in Overwatch staat. Zo doet
	 * Mass Effect het ook — de krachten zijn autonoom, de speler kan ze hooguit
	 * overrulen.
	 */
	int32 VerbUses = 0;
	void NoteVerbUsed();

	/** Killzone raakte dit doelwit alleen dankzij zijn laanbereik (mutable: gezet in een const zoeker). */
	mutable bool bKillzoneEngaged = false;
	void ContinueAutoFire();

	FTimerHandle AutoFireTimer;
	FTimerHandle EngagementTimer;
	TWeakObjectPtr<AEclipseCharacter> AutoTarget;

	/** De dichtstbijzijnde vijand binnen wapenbereik met zicht, of null. */
	AEclipseCharacter* FindHostileInRange() const;

	/** Diagnostiek: schoten die deze soldaat ZONDER order heeft gelost. */
	int32 AutoFireShots = 0;

	FTimerHandle FollowTimer;

	/** Volgafstand uit DA_SquadTuning; 0 tot BeginFollowing hem zet. */
	float FollowDistance = 0.0f;

	/** Diagnostiek: hoe vaak meelopen een verplaatsing startte. */
	int32 FollowMoves = 0;

	/** De speler, of null. Meelopen heeft een leider nodig om achter te lopen. */
	APawn* GetLeaderPawn() const;

	/** Last move stance (SPEC-P1-06 stub; no behavior split yet — PLACEHOLDER in the enum). */
	EEclipseSquadStance CurrentStance = EEclipseSquadStance::Ready;

	/** Resolved class kit (classless defaults until the game mode applies one). */
	EclipseClassLogic::FEclipseResolvedClassKit ClassKit;

	/** Active triage run: the body being saved + the move request that carries us there. */
	TWeakObjectPtr<AEclipseCharacter> TriageTarget;
	FAIRequestID TriageMoveId;
};
