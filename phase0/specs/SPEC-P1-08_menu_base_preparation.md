# SPEC-P1-08 — Menu Base & Preparation
*Phase 1 feature spec | GDD refs: 11.1 (preparation as first-class phase), 5.1, 13.2 | Skill owner: Game Architecture Expert (UI: UE Programmer)*

---

## Goal

The menu-only base (13.2) closes the loop between strategy choice and mission execution. Preparation is a first-class phase (11.1): the prototype must prove that spending strategic resources on *knowledge and readiness* makes the next mission feel fairer and more owned — the "welds the game's halves together" loop in miniature.

## In scope

- Base hub screen (debug-grade CommonUI stack — the UI *architecture* is real even though the skin is not): four tabs.
  1. **Command:** open the mini-map (SPEC-P1-04), advance day button (campaign clock).
  2. **Workshop:** the one production choice (SPEC-P1-03), production queue status.
  3. **Barracks:** roster list (SPEC-P1-07), pick 2 squadmates for the next mission.
  4. **Memorial:** the memorial list (SPEC-P1-07).
- Preparation flow (after mission select, before launch): squad pick → loadout pick (options gated by produced items) → insertion point pick (3 entries, SPEC-P1-05) → optional **Intel spend**: pay I to reveal enemy count + positions on the briefing sketch (the 11.1 fairness loop, stub form).
- Briefing/debrief screens as part of this stack (mission context line in; consequence summary out — every changed number shown with its cause, the 7.6 transparency rule applied to the loop).

## Out of scope

3D walkable base, facilities/construction/upgrades, staffing, research, character scenes, time-of-day; any facility beyond the four tabs.

## Data

`DT_LoadoutOptions` (base options + unlock-by-production tags), `UEclipsePrepTuningAsset` (intel reveal cost, reveal contents).

## Events

Consumed: `Event.Strategy.MissionSelected`, `Event.Economy.*` (wallet/queue refresh), `Event.Roster.*` (list refresh), `Event.Mission.Completed/Failed` (debrief data).
Emitted: `Event.Prep.MissionLaunchRequested` (payload: mission id, squad ids, loadout, insertion, intel level) — consumed by mission runtime (SPEC-P1-05).

## Debug UI

The screens are debug-grade by design; add `Eclipse.Prep.AutoLaunch` (default squad/loadout) for Gauntlet loops.

## Tests (14.4)

- Functional: full-loop Gauntlet script — advance day → select mission → prep (with intel spend) → auto-launch → force-win → verify debrief shows committed consequences → second loop launches (the gate question, automated as a smoke test).
- Unit: launch payload validation (dead/wounded soldiers unpickable; loadout requires produced item).

## Definition of Done

The full circle (13.2) runs entirely through these screens with no console commands; intel spend visibly changes the briefing; a wounded soldier is visibly unavailable and the tester routes around it.
