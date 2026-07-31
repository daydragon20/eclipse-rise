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
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Strategy_ResponseTierChanged, "Event.Strategy.ResponseTierChanged", "De Dominion Response Tier is gestegen (GDD 9.4). Draagt de STAP (oud -> nieuw), niet alleen de landing, zodat de diegetische uiting de juiste toon kan kiezen. Alleen omhoog; de transactie weigert een daling.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Strategy_LiberationResolved, "Event.Strategy.LiberationResolved", "Een bevrijdingsrij is gecommit: rijnaam, aantal omgedraaide vakken en de geauthorde zin die uitlegt WAAROM. Vuurt per RIJ, waar RegionControlChanged per VAK vuurt (SPEC-P2-05 bouwvolgorde stap 4).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Prep_MissionLaunchRequested, "Event.Prep.MissionLaunchRequested", "Preparation flow requests mission start with squad/loadout/insertion/intel payload (SPEC-P1-08).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_Started, "Event.Mission.Started", "Mission runtime entered play (SPEC-P1-05).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_ObjectiveCompleted, "Event.Mission.ObjectiveCompleted", "An objective primitive reported completion (SPEC-P1-05).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_PhaseChanged, "Event.Mission.PhaseChanged", "Mission runtime entered a phase (SPEC-P2-04): outer loop phases (GDD 11.1) carry bAuthoredSubPhase=false; named sub-phases like \"Alarm\" carry true. Alarm travels HERE — it is a sub-phase fact, never its own event and never mission failure (GDD 11.4).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_Completed, "Event.Mission.Completed", "Mission ended in success; consequences arrive separately as commit events (GDD 14.3.3).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Mission_Failed, "Event.Mission.Failed", "Mission ended in failure; fail-forward consequences still commit (GDD 11.4).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_OrderIssued, "Event.Squad.OrderIssued", "Player issued a squad order (SPEC-P1-06).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_OrderAcknowledged, "Event.Squad.OrderAcknowledged", "Squadmate accepted an order with a bark line; silence is forbidden (GDD 9.5).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_WeaponSwapped, "Event.Combat.WeaponSwapped", "Er is van wapen gewisseld: welke familie er nu in de handen ligt en hoe lang het optillen duurt (26-07 avond, punt 5).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_ReloadStarted, "Event.Combat.ReloadStarted", "Er wordt herladen: welke wapenfamilie en hoe lang het duurt. De foley-keten hangt aan de FASEN van dit feit, niet aan een enkel geluid bij de start (26-07 avond).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_HitLanded, "Event.Combat.HitLanded", "Een schot RAAKT iemand: plek, of het een kopschot was, en hoeveel schade. Het tegenovergestelde feit van ShotFired, dat ook bij een misser vuurt (26-07).")
	// EEN EIGEN FEIT EN GEEN VELD OP HitLanded, om dezelfde reden als de
	// bevrijdingszin: HitLanded betekent 'schade aan een personage' en wordt door
	// consumenten zo geteld — de hitmarker hangt eraan. Een muur die geraakt wordt
	// hoort daar niet tussen te staan, want dan telt elke misser als treffer.
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_WorldImpact, "Event.Combat.WorldImpact", "Een schot raakt de WERELD in plaats van een personage: de misser. Tot 27-07 werd die tak stil verworpen, waardoor elke mis onzichtbaar en onhoorbaar was — en juist daaruit leidt de speler af of hij raakt (owner-punt 4).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_ShotFired, "Event.Combat.ShotFired", "Er is geschoten: oorsprong plus de geluidsradius van het wapen. Raak of mis maakt niet uit — de loop maakt het lawaai (26-07, punt 1).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_OrderRefused, "Event.Squad.OrderRefused", "Squadmate refused an order with a reason; silent failure is a release blocker (GDD 8.4).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_SoldierDowned, "Event.Squad.SoldierDowned", "A squadmate went down in mission; resolution to dead/wounded happens at debrief (SPEC-P1-06/07).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_SoldierStabilized, "Event.Squad.SoldierStabilized", "A downed squadmate was stabilized inside the data-driven window - the Medic save that turns permadeath into a wound (SPEC-P2-01, GDD 4.2.5).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_ClassAbilityUsed, "Event.Squad.ClassAbilityUsed", "A class signature verb fired (Stabilize/Momentum/Killzone); payload names the verb tag (SPEC-P2-01, GDD 4.2.3).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_OrderQueued, "Event.Squad.OrderQueued", "Een order staat klaar en wacht: een flankvoorstel op goedkeuring, of een sync-strike-markering. Het Order-veld draagt de OVERGANG (Flank.Proposed/Approved/Expired/Cancelled, SyncStrike.Marked/Unmarked/Pruned), zodat de debug-UI zijn routelijnen en pips uit dezelfde stroom wist als waaruit hij ze tekent (SPEC-P2-02 Stage B).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Squad_SelfAction, "Event.Squad.SelfAction", "Een soldaat doet uit zichzelf iets dat de speler raakt: vuur openen, dekking zoeken, herladen. Reason zegt welk van de drie (GDD 9.5 verbale transparantie).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Player_VitalsChanged, "Event.Player.VitalsChanged", "De toestand van het SPELERSLICHAAM veranderde: gezondheid (huidig + max), houding (staand/hurken/sprint/dekking) of neer. Vuurt alleen als er echt iets veranderde — de HUD hoeft niet te pollen (GDD 12.2 regel 2, bouwvolgorde 14.5).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Command_ModeEntered, "Event.Command.ModeEntered", "Command Mode hold began; payload carries the applied dilation (SPEC-P2-02). Audio ducks/filters on this (SPEC-P2-09); UI is a consumer, never the emitter.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Command_ModeExited, "Event.Command.ModeExited", "Command Mode ended (release or fail-safe); payload carries HeldSeconds + OrdersIssuedWhileHeld — the R3 feel-gauntlet telemetry (SPEC-P2-02).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Base_ConstructionStarted, "Event.Base.ConstructionStarted", "A build/upgrade order was accepted at a slot; emitted by CampaignState commit only (SPEC-P2-03).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Base_FacilityBuilt, "Event.Base.FacilityBuilt", "A facility reached L1 (day tick or rush, in that commit); vault presentation swaps the slot state on this (SPEC-P2-03, GDD 5.4).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Base_FacilityUpgraded, "Event.Base.FacilityUpgraded", "A facility reached L2+ (slice: Workshop only, per data); emitted by CampaignState commit only (SPEC-P2-03).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Base_StaffAssigned, "Event.Base.StaffAssigned", "Base staffing changed (crew/analyst by position, empty role = unassigned incl. completion and casualty releases); emitted by CampaignState commit only (SPEC-P2-03).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Story_BeatReached, "Event.Story.BeatReached", "A story beat committed to campaign state (set-only flag); emitted by CampaignState commit only (SPEC-P2-04). Consumers: pinned-offer refresh, UI (P2-07), audio sting (16.12), soak asserts.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Roster_SoldierAdded, "Event.Roster.SoldierAdded", "A soldier joined the roster; emitted by CampaignState commit only (SPEC-P1-07).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Roster_SoldierDied, "Event.Roster.SoldierDied", "A soldier died permanently; emitted by CampaignState commit only (SPEC-P1-07).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Roster_SoldierWounded, "Event.Roster.SoldierWounded", "A soldier was wounded and is out N days; emitted by CampaignState commit only (SPEC-P1-07).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Memorial_EntryAdded, "Event.Memorial.EntryAdded", "A memorial record was written; the wall never resets (SPEC-P1-07, Pillar 3).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Resource_Credits, "Resource.Credits", "Money — the grey world (GDD 6.2). Phase 1 wallet resource.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Resource_Materials, "Resource.Materials", "Industrial matter for construction/manufacturing (GDD 6.2). Phase 1 wallet resource.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Resource_Intel, "Resource.Intel", "Secrets; decays weekly to force spending (GDD 6.2). Phase 1 wallet resource.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Verb_Momentum, "Class.Verb.Momentum", "Assault signature verb identity (GDD 4.2.3); mechanics land per-spec, numbers in DT_ClassDefs.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Verb_Stabilize, "Class.Verb.Stabilize", "Medic signature verb identity (GDD 4.2.3/4.2.5); window seconds live in DT_ClassDefs.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Verb_Killzone, "Class.Verb.Killzone", "Sniper signature verb identity (GDD 4.2.3); lane range lives in DT_ClassDefs (Command Mode wiring = SPEC-P2-02).")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Base_Staff_Crew, "Base.Staff.Crew", "Construction-crew role identity (SPEC-P2-03 staffing v1); positional — staff on a building site. Day reduction lives in DA_BaseTuning.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Base_Staff_Analyst, "Base.Staff.Analyst", "IC-analyst role identity (SPEC-P2-03 staffing v1); positional — staff on an operational site. Bonus lives in DA_BaseTuning.")

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Story_Beat_M11_ThirteenBullets, "Story.Beat.M11_ThirteenBullets", "M1.1 completed (SPEC-P2-04); unlocks M1.2 via DT_StoryMissions.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Story_Beat_M12_DeadDrop, "Story.Beat.M12_DeadDrop", "M1.2 completed (SPEC-P2-04); unlocks M1.3.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Story_Beat_M13_SignalFire, "Story.Beat.M13_SignalFire", "M1.3 completed (SPEC-P2-04); the SPEC-P2-05 liberation trigger consumes this beat's mission completion.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Story_Beat_M14_Quartermaster, "Story.Beat.M14_Quartermaster", "M1.4 completed (SPEC-P2-04); Act 1 opening closed.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Story_Beat_BrickRecruited, "Story.Beat.BrickRecruited", "Brick joined the roster (SPEC-P2-04 decision 11; committed by M1.4's debrief).")
}
