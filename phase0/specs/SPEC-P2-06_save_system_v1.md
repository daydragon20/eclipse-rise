# SPEC-P2-06 — Save System v1 — **CONCEPT for main-review (2026-07-27)**
*Phase 2 feature spec | GDD refs: 12.3 (save architecture, single writer of state), 14.3.6 (schema version + migration per break, honesty), 14.5 (build order: data → pure core → wiring → debug surface), 14.3.4 (events before code), SPEC-P1-02 (campaign state + transaction API), SPEC-P2-03 (base state tail), SPEC-P2-04 (story flags tail, v5), SPEC-P2-05 (liberation commits region state) | Skill owner: Game Architecture Expert*

## Purpose
Promote the save system from "one block that happens to hold everything" to the
versioned, per-subsystem architecture 12.3 asks for, and add the two things a
slice needs to be *played* rather than *tested*: **autosave at strategic
transitions** and a **loud, checkable answer to "did my progress survive?"**

The spec is falsification-first on one claim, and that claim already failed once:
**a save that restores perfectly can still leave the player with nothing to do.**
Measured 2026-07-27 in a fresh instance — the campaign state came back
bit-for-bit (day, credits, story beats, state hash identical) and **not one region
offered a mission**, because the campaign *setup asset* is not part of the file
and nothing re-established it. There was no error, no warning, no clue. That is
the shape every decision below is written against.

## What exists today (measured, 2026-07-27)
- **One provider.** `UEclipseCampaignSubsystem` is the only
  `IEclipseSaveDataProvider`; block id `Campaign`. Base state, roster class ids
  and story flags all ride inside `FEclipseCampaignState` as schema tails
  (v3 → v4 → v5).
- **Migration works and is tested**: v2/v3/v4 fixture round-trips, byte-faithful
  reconstruction, `GetLastLoadMigrationStepCount()` asserts exactly-N steps.
- **Progress survives a round trip** — proven end-to-end on shipped data on
  2026-07-27: play M1.1, save, load in a fresh instance, and WorkerHousing still
  offers `MT_M12`. The assertion deliberately checks *behaviour* (the gate still
  remembers) rather than "the tag is in the array".
- **`AutosaveSlotName` exists and nothing writes it.** There is no autosave.
- **`Event.Save.Completed` does not exist.**
- **The file does not record which content it belongs to** (see decision 1).

## Locked decisions (this spec)

1. **The save records the campaign setup it was made against, and refuses to
   load into a different one without saying so.**
   Authored content — region graph, mission tables, tuning — stays *out* of the
   file; it belongs to the build, not to the player. But the file must carry the
   setup's *identity* (soft path), for two reasons that a measurement on 27-07
   made concrete: (a) loading with no setup at all currently yields a silent
   empty map, and (b) loading a save from an older content build silently gives
   the player a campaign whose pins point at regions that may not exist.
   Both are the same failure: the file and the content disagree and nobody says
   so. **Schema v5 → v6**, migration entry + fixture test, and a load into a
   mismatched setup degrades LOUDLY (14.3.5) instead of half-working.
   *Rationale: the alternative — "assume the caller knows" — is what produced the
   empty board, and it is unfalsifiable from inside the game.*

2. **Autosave triggers are STRATEGIC only: after a debrief commit and after a day
   tick.** Not mid-mission, not on a timer. The mission runtime has no
   serialization contract (decision 3), so an autosave during a mission would
   promise a resume it cannot deliver.
   *Rationale: 12.3's single-writer rule. Two ideas of "where you are" is the
   divergence bug the transaction API exists to prevent.*

3. **No mission checkpoints in v1.** SPEC-P2-00 names them; this spec defers
   them with the beat they would unblock stated: resuming a *failed* mission
   mid-run. That needs the mission runtime to serialize spawned actors, squad
   positions, objective progress and the alarm latch — a second state machine
   next to the campaign, and 12.3's single-writer rule says that is its own spec.
   *Rationale: a checkpoint that restores the map but not the squad is worse than
   no checkpoint, because the player trusts it once.*

4. **Every provider owns its own block and its own version.** Today everything
   rides in `Campaign` as tails, which is why the base state and story flags
   needed byte-arithmetic in their migration tests. New state (P2-07 UI
   preferences, P2-09 audio mix) gets its own block id, not another tail.
   *Rationale: 12.3. A tail is cheap to write and expensive to migrate — the v3
   and v4 tests reconstruct file bytes by hand to prove it.*

5. **`Event.Save.Completed` is a FACT, not a request.** Emitted after a
   successful write with slot name and whether it was an autosave. UI consumes
   it for a toast (P2-07). Nothing on the bus ever *asks* for a save.
   *Rationale: EventCatalog rule — the bus reports what happened.*

6. **A failed save is loud and does not pretend.** No silent retry, no "saved!"
   toast on a failed write. The slot keeps its previous contents; a partially
   written file is never left in place.
   *Rationale: 14.3.5, and the same reason the audio loader stopped accepting a
   half-filled bank.*

## Data schema (14.5 step 1)
- File header gains a **content identity block**: setup asset soft path + a
  content hash of the region graph ids. Schema **v6**; migration v5 → v6 fills it
  from the active setup (an old save keeps working, it just cannot detect a
  mismatch retroactively — and says so once).
- `FEclipseSaveResult`: slot, bytes written, block count, migration steps,
  and a typed failure reason.

## Events (14.3.4 — catalog rows land in the implementation commit)
- **Emitted:** `Event.Save.Completed` (slot, bAutosave, block count).
- **Consumed:** `Event.Mission.Completed` and `Event.Campaign.DayAdvanced` as
  autosave triggers. Both already exist; no new producers.

## Build order (14.5)
1. **Content identity in the file** + v5 → v6 migration + fixture test, and the
   loud mismatch path. This is step 1 because it is the falsification target.
2. **Autosave** on the two strategic triggers, writing `AutosaveSlotName`.
3. **`Event.Save.Completed`** + catalog rows.
4. **Debug surface:** `Eclipse.Save.Report` — which slots exist, their schema
   version, their setup identity, and whether it matches the running build.
   Same shape as `Eclipse.Liberation.Report`: **first whether the thing is
   connected, then what is in it.** That command is what turns "my map is empty"
   into a one-line answer.
5. **Per-block split** of the campaign tails (base state, story flags) into their
   own providers — last, because it is a pure refactor behind green tests.

## Tests (14.4, blocking)
- **Round-trip on shipped data, behaviour not bytes:** play a mission, save,
  load in a fresh instance, and assert the *gate still opens* (the existing
  `ProgressSurvivesASaveLoad` pattern). A state-hash comparison alone passed
  while the board was empty.
- **Mismatch is loud:** a save with setup identity A loaded against setup B
  produces exactly one error and does not silently half-load.
- **No setup at all:** the current warning becomes a typed failure with the same
  wording; the seven migration fixtures that deliberately load without a setup
  must stay green (they test the format, not the game — an assertion that goes
  red on correct use is worse than no assertion).
- **Autosave fires exactly once per trigger**, and never mid-mission.
- **A failed write leaves the previous slot intact** (write to temp, then move).
- **Migration:** byte-faithful v5 file loads via exactly the 5 → 6 step.

## Definition of Done
Content identity in the file with a green v5 → v6 fixture · a mismatched load
fails loudly and a setup-less load fails typed · autosave on debrief and day tick,
never mid-mission · `Event.Save.Completed` emitted with catalog rows updated in
the same commit · `Eclipse.Save.Report` answers "why is my map empty" in one line
and is documented in BESTURING.md (the guard there checks it exists) · the shipped
M1-chain round-trips and the gate still opens · dashboard updated.

## Non-goals
Mission checkpoints (decision 3) · cloud/steam sync · multiple named profiles ·
compression · save file encryption · a save/load UI (that is P2-07; this spec
ships the console surface).

## Open review points (for main / owner)
1. **Does a mismatched setup BLOCK the load or warn and continue?** Blocking is
   honest but can strand a player after a content update; warning risks a
   campaign whose pins point at regions that no longer exist. This spec assumes
   *warn loudly and continue*, and the owner should overrule if the slice is ever
   shipped to someone who cannot read a log.
2. **How many autosave slots?** One rolling slot is simplest; three rotating
   slots survive a bad save. Not decided here because it is a player-facing
   choice, not an architectural one.
3. **Is the content hash over region ids enough**, or should mission template ids
   count too? Region ids catch the case that broke on 27-07; template ids would
   also catch a renamed mission. The wider hash means more false mismatches
   during authoring.
