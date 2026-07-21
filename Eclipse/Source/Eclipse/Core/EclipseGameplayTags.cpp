#include "Core/EclipseGameplayTags.h"

// Definitions mirror Docs/EventCatalog.md row-for-row (GDD 14.2 governance).
// The CI check (Tools/check_event_catalog.py) greps these definitions against the catalog.
namespace EclipseTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Campaign_DayAdvanced, "Event.Campaign.DayAdvanced", "Campaign clock advanced one day; emitted by CampaignState commit only (SPEC-P1-02).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Economy_ResourcesChanged, "Event.Economy.ResourcesChanged", "A resource balance changed; emitted by CampaignState commit only (SPEC-P1-02/03).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Economy_ProductionQueued, "Event.Economy.ProductionQueued", "A production order entered the queue; emitted by CampaignState commit only (SPEC-P1-03).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Economy_ProductionCompleted, "Event.Economy.ProductionCompleted", "A production order finished; emitted via economy tick -> commit (SPEC-P1-03).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Strategy_RegionControlChanged, "Event.Strategy.RegionControlChanged", "A region changed owner; emitted by CampaignState commit only (SPEC-P1-02/04).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Strategy_MissionSelected, "Event.Strategy.MissionSelected", "Player picked a mission offer on the mini-map (SPEC-P1-04).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Prep_MissionLaunchRequested, "Event.Prep.MissionLaunchRequested", "Preparation flow requests mission start with squad/loadout/insertion/intel payload (SPEC-P1-08).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_Started, "Event.Mission.Started", "Mission runtime entered play (SPEC-P1-05).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_ObjectiveCompleted, "Event.Mission.ObjectiveCompleted", "An objective primitive reported completion (SPEC-P1-05).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_Completed, "Event.Mission.Completed", "Mission ended in success; consequences arrive separately as commit events (GDD 14.3.3).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_Failed, "Event.Mission.Failed", "Mission ended in failure; fail-forward consequences still commit (GDD 11.4).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_OrderIssued, "Event.Squad.OrderIssued", "Player issued a squad order (SPEC-P1-06).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_OrderAcknowledged, "Event.Squad.OrderAcknowledged", "Squadmate accepted an order with a bark line; silence is forbidden (GDD 9.5).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_OrderRefused, "Event.Squad.OrderRefused", "Squadmate refused an order with a reason; silent failure is a release blocker (GDD 8.4).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_SoldierDowned, "Event.Squad.SoldierDowned", "A squadmate went down in mission; resolution to dead/wounded happens at debrief (SPEC-P1-06/07).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Roster_SoldierAdded, "Event.Roster.SoldierAdded", "A soldier joined the roster; emitted by CampaignState commit only (SPEC-P1-07).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Roster_SoldierDied, "Event.Roster.SoldierDied", "A soldier died permanently; emitted by CampaignState commit only (SPEC-P1-07).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Roster_SoldierWounded, "Event.Roster.SoldierWounded", "A soldier was wounded and is out N days; emitted by CampaignState commit only (SPEC-P1-07).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Memorial_EntryAdded, "Event.Memorial.EntryAdded", "A memorial record was written; the wall never resets (SPEC-P1-07, Pillar 3).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Resource_Credits, "Resource.Credits", "Money — the grey world (GDD 6.2). Phase 1 wallet resource.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Resource_Materials, "Resource.Materials", "Industrial matter for construction/manufacturing (GDD 6.2). Phase 1 wallet resource.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Resource_Intel, "Resource.Intel", "Secrets; decays weekly to force spending (GDD 6.2). Phase 1 wallet resource.")
}
