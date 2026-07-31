#pragma once

#include "NativeGameplayTags.h"

/**
 * Central native declaration of every event-bus tag (SPEC-P1-01: one file owns all
 * Event.* tags so the catalog check in CI has a single source to grep against).
 * Governance (GDD 14.2): every tag here has a row in Docs/EventCatalog.md, updated
 * in the same commit. Tag grammar (EventCatalog rules): Event.<System>.<PastTenseFact>
 * — the bus reports facts; the CampaignState transaction API is how things happen
 * (GDD 14.3.3).
 */
namespace EclipseTags
{
	// Campaign (SPEC-P1-02)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Campaign_DayAdvanced)

	// Economy (SPEC-P1-02/03)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Economy_ResourcesChanged)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Economy_ProductionQueued)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Economy_ProductionCompleted)

	// Strategy (SPEC-P1-02/04)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Strategy_RegionControlChanged)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Strategy_MissionSelected)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Strategy_LiberationResolved)

	/**
	 * De Dominion Response Tier is gestegen (GDD 9.4). Campagne-breed feit, geen
	 * vak-feit — daarom een eigen payload naast FEclipseStrategyEventPayload.
	 * Alleen omhoog: de ladder daalt niet, dus dit feit betekent altijd "erger".
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Strategy_ResponseTierChanged)

	// Preparation (SPEC-P1-08)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Prep_MissionLaunchRequested)

	// Mission (SPEC-P1-05; SPEC-P2-04 phase surface)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Started)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_ObjectiveCompleted)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_PhaseChanged)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Completed)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Failed)

	// Squad (SPEC-P1-06; SPEC-P2-01 class facts)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_OrderIssued)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_OrderAcknowledged)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_ShotFired)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_HitLanded)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_WorldImpact)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_ReloadStarted)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_WeaponSwapped)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_OrderRefused)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_SoldierDowned)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_SoldierStabilized)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_ClassAbilityUsed)

	/**
	 * Een order STAAT KLAAR maar is nog niet uitgevoerd (SPEC-P2-02 Stage B).
	 *
	 * Twee dingen dragen hierop: het flankvoorstel dat op jouw goedkeuring wacht,
	 * en de markeringen van een sync strike. Ook de NEGATIEVE overgangen rijden
	 * mee — verlopen, geannuleerd, markering weggevallen — want de debug-UI die de
	 * routelijn en de pips uit deze stroom TEKENT, moet ze uit dezelfde stroom
	 * kunnen wissen. Een voorstel dat stil verdwijnt is precies de stilte die 9.5
	 * verbiedt, ook al is niets-doen van de speler geen weigering.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_OrderQueued)

	/**
	 * Een soldaat doet uit zichzelf iets dat de speler RAAKT: vuur openen, dekking
	 * zoeken onder vuur, herladen. Reason draagt welk van de drie ("Contact",
	 * "TakingFire", "Reloading").
	 *
	 * Waarom een EIGEN tag en niet OrderAcknowledged: die betekent "ik heb jouw
	 * order gehoord", en dit is het tegenovergestelde — hij deed het zonder dat je
	 * het vroeg. De owner vroeg er stemmen bij omdat zijn squad sinds de
	 * autonomie-laag zelfstandig handelt en er niets bij zegt.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_SelfAction)

	/**
	 * Het lichaam van de SPELER is van toestand veranderd: gezondheid, houding of
	 * neer. Eén feit voor de drie, want ze horen bij hetzelfde lichaam op hetzelfde
	 * moment en de schermlaag tekent ze in één beeld; de payload zegt welk deel
	 * bewoog (bHealthChanged / bStanceChanged / bDownedChanged).
	 *
	 * Een EIGEN familie naast Event.Squad.*, en niet SoldierDowned hergebruiken:
	 * die feiten gaan over ROSTER-soldaten met een FGuid en worden door de
	 * missielaag geteld voor de debriefing. De speler heeft geen soldier-id, gaat
	 * niet naar de debriefing, en zijn gezondheid is geen squad-feit — het is wat
	 * er op zijn eigen scherm hoort te staan.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_VitalsChanged)

	/**
	 * De andere helft van hetzelfde dashboard: de STAND van het wapen in de handen
	 * van de speler.
	 *
	 * Hier en niet bij Event.Combat.*, want die familie beschrijft de wereld (elk
	 * lichaam mag erin voorkomen, de AI en de audiolaag luisteren mee) en dit
	 * beschrijft één scherm. Zie de toelichting bij FEclipseWeaponStatusPayload.
	 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Player_WeaponStatusChanged)

	// Command Mode lifecycle (SPEC-P2-02)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Command_ModeEntered)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Command_ModeExited)

	// Hollow Point base (SPEC-P2-03)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Base_ConstructionStarted)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Base_FacilityBuilt)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Base_FacilityUpgraded)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Base_StaffAssigned)

	// Story beats (SPEC-P2-04)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Story_BeatReached)

	// Roster & Memorial (SPEC-P1-07)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Roster_SoldierAdded)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Roster_SoldierDied)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Roster_SoldierWounded)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Memorial_EntryAdded)

	// Resource identities — Phase 1 subset of GDD 6.2 (C/M/I). Not events; the
	// wallet and all economy data key off these. Amounts stay in DataAssets.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Resource_Credits)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Resource_Materials)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Resource_Intel)

	// Class signature-verb identities (SPEC-P2-01, GDD 4.2.3). Not events; the
	// DT_ClassDefs SignatureVerb column keys off these — timing/range numbers
	// stay in the class rows, never in code.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Verb_Momentum)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Verb_Stabilize)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Verb_Killzone)

	// Base staff role identities (SPEC-P2-03 staffing v1). Not events; the
	// StaffAssigned payload reports the positional role through these — staff
	// on a building site is the crew, on an operational site the analyst.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Base_Staff_Crew)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Base_Staff_Analyst)

	// Story beat identities (SPEC-P2-04). Not events; DT_StoryMissions unlock/
	// completion columns and the StoryFlags state key off these — the beat
	// FACT travels as Event.Story.BeatReached.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Beat_M11_ThirteenBullets)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Beat_M12_DeadDrop)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Beat_M13_SignalFire)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Beat_M14_Quartermaster)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Story_Beat_BrickRecruited)
}
