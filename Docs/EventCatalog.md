# ECLIPSE Event Catalog
*Governance: GDD 14.2 — every event-bus tag is documented here in the same commit that adds it. GDD 12.2 rule 2 — subsystems communicate only via these events (or explicit public interfaces).*

**Status legend:** `specified` = named in a Phase 1 spec, no code yet · `implemented` = native tag + emitter exist.

All tags below are **specified** (Phase 0 output; implementation lands in Phase 1).

| Tag | Payload (struct) | Emitted by | Consumed by | Spec |
|---|---|---|---|---|
| `Event.Campaign.DayAdvanced` | `FEclipseCampaignEventPayload` (new day index) | CampaignSubsystem (commit) | Economy (tick) | P1-02 |
| `Event.Economy.ResourcesChanged` | `FEclipseEconomyEventPayload` (type, delta, new balance, reason) | CampaignSubsystem (commit) | Mini-map UI, Base UI | P1-02/03 |
| `Event.Economy.ProductionQueued` | `FEclipseEconomyEventPayload` (item id, eta) | CampaignSubsystem (commit) | Base UI | P1-03 |
| `Event.Economy.ProductionCompleted` | `FEclipseEconomyEventPayload` (item id, loadout tag) | Economy tick → commit | Base UI, Preparation | P1-03 |
| `Event.Strategy.RegionControlChanged` | `FEclipseStrategyEventPayload` (region id, old/new owner) | CampaignSubsystem (commit) | Mini-map UI | P1-02/04 |
| `Event.Strategy.MissionSelected` | `FEclipseStrategyEventPayload` (region id, template id) | Mini-map screen | Preparation flow | P1-04 |
| `Event.Prep.MissionLaunchRequested` | `FEclipsePrepEventPayload` (mission id, squad ids, loadout, insertion, intel level) | Preparation flow | Mission runtime | P1-08 |
| `Event.Mission.Started` | `FEclipseMissionEventPayload` (mission id) | Mission runtime | Squad (spawn), UI | P1-05 |
| `Event.Mission.ObjectiveCompleted` | `FEclipseMissionEventPayload` (mission id, objective id) | Objective components | Mission StateTree, UI | P1-05 |
| `Event.Mission.Completed` | `FEclipseMissionEventPayload` (mission id, results) | Mission runtime (debrief) | Economy, Roster, Base UI | P1-05 |
| `Event.Mission.Failed` | `FEclipseMissionEventPayload` (mission id, results) | Mission runtime (debrief) | Economy, Roster, Base UI | P1-05 |
| `Event.Squad.OrderIssued` | `FEclipseSquadEventPayload` (soldier id, order, target) | Order input layer | Squad AI, debug UI | P1-06 |
| `Event.Squad.OrderAcknowledged` | `FEclipseSquadEventPayload` (soldier id, order, bark line) | Squad AI | Debug UI, (later) VO | P1-06 |
| `Event.Squad.OrderRefused` | `FEclipseSquadEventPayload` (soldier id, order, reason) | Squad AI | Debug UI, scenario tests | P1-06 |
| `Event.Squad.SoldierDowned` | `FEclipseSquadEventPayload` (soldier id, cause) | Character/health | Mission runtime (debrief resolution) | P1-06/07 |
| `Event.Roster.SoldierAdded` | `FEclipseRosterEventPayload` (soldier record) | CampaignSubsystem (commit) | Barracks UI | P1-07 |
| `Event.Roster.SoldierDied` | `FEclipseRosterEventPayload` (soldier id, cause, day) | CampaignSubsystem (commit) | Barracks UI, Memorial | P1-07 |
| `Event.Roster.SoldierWounded` | `FEclipseRosterEventPayload` (soldier id, days out) | CampaignSubsystem (commit) | Barracks UI | P1-07 |
| `Event.Memorial.EntryAdded` | `FEclipseRosterEventPayload` (memorial entry) | CampaignSubsystem (commit) | Memorial UI | P1-07 |

**Rules for adding a tag:** name follows `Event.<System>.<PastTenseFact>`; facts, not commands (the bus reports what happened; the transaction API is how you make things happen — GDD 14.3.3). State-changing facts are emitted only by the CampaignState commit, never directly by gameplay code.
