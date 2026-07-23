# SPEC-P2-01 — Squad of 4 & Classes
*Phase 2 feature spec | GDD refs: 4.2.3 (classes), 4.2.2 (traits/bonds), 8.3 (fairness/cover), 9.5 (squad AI promise), 12.3 (shared body), SPEC-P1-06/07 (base) | Skill owner: Game Architecture Expert → AI Systems Engineer*

## Purpose
Grow the deployed squad from 2 to 4 and introduce the first three classes so a
squad reads as *people with jobs* (pillar 3). Classes are data, not subclasses:
the one `AEclipseCharacter` body stays (12.3); a class changes kit, one signature
verb, and bark flavor — never the shared movement/health/order contract.

## Locked decisions (this spec)
1. **The three slice classes: Assault, Medic, Sniper.** Rationale: maximum
   role-contrast on the order verbs (point/close, sustain/revive, overwatch/lane),
   and Medic's *Stabilize* makes permadeath dramatic instead of instant (4.2.5 —
   the Critical-Injury window is the emotional core of the slice).
2. **Pre-classed authored recruits** (SPEC-P2-00 open question resolved): Act 1
   squadmates arrive with a fixed class; the Training Academy assignment flow is
   Phase 3 (with the facility). The Muster UI shows the class as read-only.
3. **Squad size 4 = player + 3.** Command Mode order targets stay per-soldier;
   fireteam grouping (8+) is Phase 3 (4.1.6 rank ladder).

## Data schema (14.5 step 1)
- `DT_ClassDefs` — `FEclipseClassDefRow`: `DisplayName (FText)`, `WeaponRow (FName → DT_Weapons)`,
  `BodyDefOverride (FName → DT_BodyDefs, optional)`, `SignatureVerb (FGameplayTag)`,
  `StabilizeWindowSeconds (float, Medic only, 0 = none)`, `KillzoneRangeCm (float, Sniper only, 0 = none)`,
  `BarkSet (FName)`.
- `FEclipseSoldierRecord` gains `FName ClassId` (save-migration entry + test, 14.3.6).
- `DA_SquadTuning` gains `MaxDeployed (int32, default 4)`.

## Pure-logic core (14.5 step 2)
- Class resolution + kit application is a pure function over (record, class row):
  testable headless. Stabilize window logic lives in the downed pipeline as data-driven
  timing, not Medic-specific branches.

## Events (14.3.4 — before code)
- **Consumed:** `Event.Prep.MissionLaunchRequested`, `Event.Squad.OrderIssued`,
  `Event.Squad.SoldierDowned`.
- **Emitted (new, EventCatalog same commit):** `Event.Squad.SoldierStabilized`
  (Medic save within window), `Event.Squad.ClassAbilityUsed` (signature verb fired).
- **Moved to implemented at 4-scale:** existing order acknowledge/refuse events.

## Order/AI integration (9.5)
- Orders unchanged in surface; per-class modulation in data: Assault +push distance,
  Sniper prefers elevated/long-lane cover (existing scorer bias parameter), Medic
  auto-triage toggle (existing stub becomes real for the Medic only).
- Refusals stay verbal (9.5); no new order types this spec (Command Mode = SPEC-P2-02).

## Tests (14.4, blocking)
- Unit: class resolution (missing class row → classless fallback, never crash);
  stabilize-window math (edge: down at window edge); save round-trip with ClassId
  (+ migration from v0 records without it).
- Scenario suite: all existing squad scenarios re-run at squad-of-4; new scenario
  "Medic stabilizes downed Assault under fire"; any silent order failure = blocker.
- Soak: mission loop with 4 deployed × 3 missions, roster consistency asserted.

## Definition of Done
Spec'd events in catalog · data + core + wrapper landed · squad of 4 deploys in
PIE with visible class kits (weapon + body override) · Medic stabilize works and
barks · 31+ tests green incl. new ones · dashboard updated.

## Non-goals
No Academy/class-assignment UI (Phase 3) · no remaining 6 classes · no fireteams ·
no Command Mode changes (SPEC-P2-02) · no new weapons beyond DT_Weapons rows.
