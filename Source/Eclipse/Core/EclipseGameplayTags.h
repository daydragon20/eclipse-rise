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

	// Preparation (SPEC-P1-08)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Prep_MissionLaunchRequested)

	// Mission (SPEC-P1-05)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Started)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_ObjectiveCompleted)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Completed)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Mission_Failed)

	// Squad (SPEC-P1-06)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_OrderIssued)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_OrderAcknowledged)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_OrderRefused)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Squad_SoldierDowned)

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
}
