# SPEC-P1-07 — Roster, Permadeath & Memorial Stub
*Phase 1 feature spec | GDD refs: 4.2.1, 4.2.7, 13.2, Pillar 3 (People, Not Units) | Skill owner: Game Architecture Expert (tone: Narrative Designer)*

---

## Goal

Prove the emotional core in stub form: soldiers are individuals, death is permanent, and the loss is *recorded*. Phase 1 asks whether even a graybox death registers with testers when the name they picked for the mission is gone from the list and present on the wall.

## In scope

- Roster in `FCampaignState`: ~8 starting soldiers (from campaign setup asset), each with: generated name, origin tag, 1 visible trait (flavor only in Phase 1), missions-served count, status (Available / Deployed / Dead).
- Name/trait generation from DataTables (name pools per origin — the 4.2.1 "two-sentence history" is Phase 3; one trait line is the stub).
- Permadeath (4.2.7): squadmate death in mission → debrief consequence → transaction `KillSoldier` → removed from available roster, permanent across save/load.
- Memorial stub: a debrief line + a plain memorial list screen (name, missions served, cause, campaign day). No 3D wall, no letters — but the *record* is real and saved (the Memorial Wall data contract starts here and never resets).
- Injury stub: non-fatal down → status "Wounded, out N days" (flat N from data; Medbay math is Phase 3).

## Out of scope

Classes, XP/ranks, bonds, morale/loyalty, memorial letters, gear recovery, Story-difficulty death conversion, companions.

## Data

`DT_NamePools`, `DT_TraitStubs`, `UEclipseRosterTuningAsset` (roster size, wound duration). Memorial entries are campaign state, not assets.

## Events

Consumed: `Event.Squad.SoldierDowned` (mission-side), `Event.Mission.Completed/Failed` (resolve downed → dead/wounded per rules).
Emitted (via transactions): `Event.Roster.SoldierAdded`, `Event.Roster.SoldierDied`, `Event.Roster.SoldierWounded`, `Event.Memorial.EntryAdded`.

## Debug UI

Roster screen (menu base, SPEC-P1-08): list + status; memorial list screen; `Eclipse.Roster.Kill <id>` (debug builds) for save/pipeline testing.

## Tests (14.4)

- Unit: death resolution permutations (downed+win, downed+fail, extraction without body stub → still dead, recorded).
- Save integrity: dead soldier stays dead and memorialized across save/load round-trip (CI fixture includes a death).

## Definition of Done

A tester loses a soldier in loop #1 and in loop #2 picks their squad *while looking at the memorial list* — the stub already changes behavior. State survives save/load.
