# ECLIPSE Event Catalog
*Governance: GDD 14.2 — every event-bus tag is documented here in the same commit that adds it. GDD 12.2 rule 2 — subsystems communicate only via these events (or explicit public interfaces).*

**Status legend:** `specified` = named in a Phase 1 spec, no code yet · `declared` = native tag exists in `Source/Eclipse/Core/EclipseGameplayTags.cpp` (SPEC-P1-01 bus infrastructure), emitter pending · `implemented` = native tag + emitter exist.

Payload structs live in `Core/EclipseEventPayloads.h`. Rows move to `implemented` in the commit that adds their emitter (specs P1-02…08).

| Tag | Payload (struct) | Emitted by | Consumed by | Spec | Status |
|---|---|---|---|---|---|
| `Event.Campaign.DayAdvanced` | `FEclipseCampaignEventPayload` (new day index) | CampaignSubsystem (commit) | Economy (tick) | P1-02 | implemented |
| `Event.Economy.ResourcesChanged` | `FEclipseEconomyEventPayload` (type, delta, new balance, reason) | CampaignSubsystem (commit) | Mini-map UI, Base UI | P1-02/03 | implemented |
| `Event.Economy.ProductionQueued` | `FEclipseEconomyEventPayload` (item id, eta) | CampaignSubsystem (commit) | Base UI | P1-03 | implemented |
| `Event.Economy.ProductionCompleted` | `FEclipseEconomyEventPayload` (item id, loadout tag) | Economy tick → commit | Base UI, Preparation | P1-03 | declared |
| `Event.Strategy.RegionControlChanged` | `FEclipseStrategyEventPayload` (region id, old/new owner) | CampaignSubsystem (commit) | Mini-map UI | P1-02/04 | implemented |
| `Event.Strategy.MissionSelected` | `FEclipseStrategyEventPayload` (region id, template id) | StrategySubsystem (map screen pick) | Preparation flow | P1-04 | implemented |
| `Event.Prep.MissionLaunchRequested` | `FEclipsePrepEventPayload` (mission id, squad ids, loadout, insertion, intel level) | PrepSubsystem (validated launch) | Mission runtime | P1-08 | implemented |
| `Event.Mission.Started` | `FEclipseMissionEventPayload` (mission id) | MissionSubsystem | Squad (spawn), UI | P1-05 | implemented |
| `Event.Mission.ObjectiveCompleted` | `FEclipseMissionEventPayload` (mission id, objective id) | MissionSubsystem (objective components report in) | Mission phases, UI | P1-05 | implemented |
| `Event.Mission.Completed` | `FEclipseMissionEventPayload` (mission id, results) | MissionSubsystem (debrief) | Economy, Roster, Base UI | P1-05 | implemented |
| `Event.Mission.Failed` | `FEclipseMissionEventPayload` (mission id, results) | MissionSubsystem (debrief) | Economy, Roster, Base UI | P1-05 | implemented |
| `Event.Squad.OrderIssued` | `FEclipseSquadEventPayload` (soldier id, order, target) | Order input layer | Squad AI, debug UI | P1-06 | declared |
| `Event.Squad.OrderAcknowledged` | `FEclipseSquadEventPayload` (soldier id, order, bark line) | Squad AI | Debug UI, (later) VO | P1-06 | declared |
| `Event.Squad.OrderRefused` | `FEclipseSquadEventPayload` (soldier id, order, reason) | Squad AI | Debug UI, scenario tests | P1-06 | declared |
| `Event.Squad.SoldierDowned` | `FEclipseSquadEventPayload` (soldier id, cause) | Character/health | Mission runtime (debrief resolution) | P1-06/07 | declared |
| `Event.Roster.SoldierAdded` | `FEclipseRosterEventPayload` (soldier record) | CampaignSubsystem (commit) | Barracks UI | P1-07 | implemented |
| `Event.Roster.SoldierDied` | `FEclipseRosterEventPayload` (soldier id, cause, day) | CampaignSubsystem (commit) | Barracks UI, Memorial | P1-07 | implemented |
| `Event.Roster.SoldierWounded` | `FEclipseRosterEventPayload` (soldier id, days out) | CampaignSubsystem (commit) | Barracks UI | P1-07 | implemented |
| `Event.Memorial.EntryAdded` | `FEclipseRosterEventPayload` (memorial entry) | CampaignSubsystem (commit) | Memorial UI | P1-07 | implemented |

**Rules for adding a tag:** name follows `Event.<System>.<PastTenseFact>`; facts, not commands (the bus reports what happened; the transaction API is how you make things happen — GDD 14.3.3). State-changing facts are emitted only by the CampaignState commit, never directly by gameplay code.
