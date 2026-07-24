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
| `Event.Squad.OrderIssued` | `FEclipseSquadEventPayload` (soldier id, order, target) | SquadSubsystem (order dispatch) | Squad AI, debug UI | P1-06/P2-01 | implemented |
| `Event.Squad.OrderAcknowledged` | `FEclipseSquadEventPayload` (soldier id, order, bark line) | SquadSubsystem (after decision) | Debug UI, (later) VO | P1-06/P2-01 | implemented |
| `Event.Squad.OrderRefused` | `FEclipseSquadEventPayload` (soldier id, order, reason) | SquadSubsystem (after decision) | Debug UI, scenario tests | P1-06/P2-01 | implemented |
| `Event.Squad.SoldierDowned` | `FEclipseSquadEventPayload` (soldier id, cause) | SquadSubsystem (body OnDowned) | Mission runtime (debrief resolution); auto-triage dispatch runs in-subsystem off the same down, not via a bus subscription | P1-06/07/P2-01 | implemented |
| `Event.Squad.SoldierStabilized` | `FEclipseSquadEventPayload` (soldier id, stabilizer id, bark line) | SquadSubsystem (triage arrival inside the window) | Mission runtime (casualty resolution), debrief UI | P2-01 | implemented |
| `Event.Squad.ClassAbilityUsed` | `FEclipseSquadEventPayload` (soldier id, ability verb tag) | SquadSubsystem (signature verb fired) | Debug UI, (later) VO/XP | P2-01 | implemented |
| `Event.Command.ModeEntered` | `FEclipseCommandEventPayload` (dilation factor) | CommandModeComponent (hold began) | Audio duck/filter (P2-09), debug UI now / UI stack (P2-07), M1.1 teaching beat (P2-04), scenario tests | P2-02 | implemented |
| `Event.Command.ModeExited` | `FEclipseCommandEventPayload` (held seconds, orders issued while held) | CommandModeComponent (release or fail-safe) | Same consumers as ModeEntered; feel-gauntlet telemetry | P2-02 | implemented |
| `Event.Roster.SoldierAdded` | `FEclipseRosterEventPayload` (soldier record) | CampaignSubsystem (commit) | Barracks UI | P1-07 | implemented |
| `Event.Roster.SoldierDied` | `FEclipseRosterEventPayload` (soldier id, cause, day) | CampaignSubsystem (commit) | Barracks UI, Memorial | P1-07 | implemented |
| `Event.Roster.SoldierWounded` | `FEclipseRosterEventPayload` (soldier id, days out) | CampaignSubsystem (commit) | Barracks UI | P1-07 | implemented |
| `Event.Memorial.EntryAdded` | `FEclipseRosterEventPayload` (memorial entry) | CampaignSubsystem (commit) | Memorial UI | P1-07 | implemented |

**Rules for adding a tag:** name follows `Event.<System>.<PastTenseFact>`; facts, not commands (the bus reports what happened; the transaction API is how you make things happen — GDD 14.3.3). State-changing facts are emitted only by the CampaignState commit, never directly by gameplay code.
