# SPEC-P1-05 — Mission Runtime & Consequences (Graybox District)
*Phase 1 feature spec | GDD refs: 11.1 (mission loop), 12.3 (quest system), 14.3.3, 13.2 | Skill owner: Game Architecture Expert (level: World Builder)*

---

## Goal

One graybox district level that runs the universal mission loop (11.1): briefing → preparation (SPEC-P1-08) → execution (objectives) → extraction → debrief, and then **commits consequences to CampaignState through the transaction API**. This is the ground half of the loop and the seam where hybrid games usually rot; it must be airtight before anything is pretty.

## In scope

- One graybox district (~200×200 m, BSP/simple meshes): cover pieces, two buildings, one objective site, one extraction zone. Three entry points (preparation chooses one — insertion choice stub, 11.1).
- Objective primitives as data-driven components (12.3: one runtime for all tiers): `ReachLocation`, `DestroyTarget`, `CollectItem`, `ExtractSquad`. Mission = DataAsset listing 1–2 objectives + 1 optional objective (11.4's stretch layer, e.g. "no alarms").
- Mission phase logic via StateTree (12.1): Insertion → Objectives → Extraction → Debrief.
- Enemy presence: 4–8 dummy enemies (basic BT: idle/patrol → attack on perception; no coordinator yet) — enough to make squad orders meaningful (SPEC-P1-06), not a combat showcase.
- Consequence pipeline: debrief screen composes one `FCampaignTransaction` (rewards, region flip if objectives met, casualties, injuries stub) → commit → events fire. Fail-forward (11.4): mission failure commits consequences too, never a retry wall.
- Player character: basic third-person locomotion + hitscan weapon per graybox feel targets doc (Phase 0); GAS-based health/damage attributes from day one (12.1: GAS is the core architecture decision).

## Out of scope

Stealth model, alert stages, cover *system* (soft-attach etc. — Phase 2), command mode time dilation (orders via menu/hotkey are enough to test "feels obeyed"), loot, vehicles, weather, civilians, mission generator.

## Data

`UEclipseMissionAsset` (objectives, entries, enemy spawn sets, reward table ref, consequence rules), `DT_EnemyArchetypes` (health, damage, perception radius).

## Events

Consumed: `Event.Strategy.MissionSelected` (+ preparation payload from SPEC-P1-08).
Emitted: `Event.Mission.Started`, `Event.Mission.ObjectiveCompleted`, `Event.Mission.Completed`, `Event.Mission.Failed` (consequences themselves arrive as CampaignState commit events, 14.3.3).

## Debug UI

On-screen objective list; `Eclipse.Mission.CompleteObjective <n>` and `Eclipse.Mission.ForceEnd <win|lose>` for Gauntlet scripting.

## Tests (14.4)

- Functional (Gauntlet, per merge): spawn → complete-by-script → verify consequences committed to CampaignState (the canonical test of 14.4, running from Phase 1 on).
- Unit: objective primitive completion logic; consequence composition (win/lose/optional-objective permutations).

## Definition of Done

Mission runs start-to-debrief without dev intervention; killing the run mid-mission loses no strategic state (nothing committed until debrief); Gauntlet consequence test green.
