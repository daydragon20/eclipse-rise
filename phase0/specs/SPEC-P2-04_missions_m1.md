# SPEC-P2-04 — Authored Missions M1.1–M1.4 — **CONCEPT, main-review verwerkt (2026-07-24)**
*Phase 2 feature spec | GDD refs: 2.9 (Act 1 missions, prologue), 11.1 (mission loop, Preparation first-class), 11.4 (design standards: ≥2 approach families, fail-forward, length bands), 12.3 (quest runtime: DataAsset objective graphs + StateTree phases), 8.5 (stealth — explicitly the *boundary line*, see Non-goals), 14.3.3/14.3.4/14.5, SPEC-P1-04/05 (offers, runtime as built), SPEC-P2-01 (classes), SPEC-P2-02 (Command Mode, Stage A/B split), SPEC-P2-03 (clock rule, econ check), EXECUTION_PLAN §3 (R7, R8) | Skill owner: Narrative Designer (beats/briefings) + World Builder (sites); Architecture Expert reviews the runtime seams*

## Purpose
Author the four opening missions — M1.1 *Thirteen Bullets*, M1.2 *The Dead Drop*,
M1.3 *Signal Fire*, M1.4 *The Quartermaster* (2.9) — on the **shared quest
runtime as built** (SPEC-P1-05: `UEclipseMissionAsset` objective graphs, phase
machine, debrief transaction), with the full 11.1 loop (Briefing → Preparation →
Execution → Extraction → Debrief) and a stealth-viable route per mission (11.4).

This spec carries two execution-plan risks and is falsification-first on both:
- **R7** — *does the quest runtime carry authored missions at all?* Answered by
  building the M1.1 skeleton as a Gauntlet (spawn → complete-by-script →
  CampaignState asserts) and getting it green **before** M1.2–M1.4 authoring
  starts (build order step 1, below).
- **R8** — *prologue scope explosion.* Answered by locked decision 1: no
  playable prologue in the slice; a briefing recap opens at M1.1, with a
  cold-reader falsification instead of 2 hours of one-off content.

Missions are the *consumers* of everything Phase 2 built so far: squad of 4 with
classes (P2-01), Command Mode (P2-02), the vault prep loop and the +1-day clock
rule (P2-03). They add **content and sequencing, not systems** (Non-goals).

## Locked decisions (this spec)
1. **No playable prologue; the slice opens at M1.1 with a briefing recap**
   (SPEC-P2-00 open question 5 resolved, per R8). The 2.9 prologue (2 h: ration
   lines, the Tithe lottery, Petra's detention, Mara's route out, the container
   escape into the Ember Cell vault) compresses to **5–7 recap cards** (still
   frame + 2–3 lines each) shown before M1.1's briefing until M1.1 completes.
   Rationale: the prologue is two hours of one-shot walking/scripting tech that
   proves nothing the 13.2 gate asks, while the recap is testable today.
   **Falsification: a cold reader** (not on the project; the first R11 reviewer
   session doubles as this) reads the recap and answers 4 comprehension
   questions — who am I, where am I, why do I fight, who is Mara — **4/4
   correct without follow-up questions**. Fail → recap rewrite, retest; the
   prologue itself re-enters scope **only** if the Phase 2 gate review says the
   opening doesn't land. Recap cards are static screens on the existing
   briefing/debrief debug surface — no cutscene tech, no dialogue system.
2. **R7 falsification is build-order step 1.** M1.1 skeleton = mission asset on
   the as-built runtime + story-pinned offer stub + Gauntlet
   (spawn → complete-by-script → asserts on rewards, story flag, day +1, region
   unchanged). This Gauntlet is **green before any M1.2–M1.4 authoring
   begins**. If the runtime can't carry it (phase machine or consequence commit
   too light), the runtime is extended **first, as a spec amendment** — never
   worked around per-mission with bespoke scripting.
3. **Runtime shape: outer loop stays the as-built phase machine; authored beats
   are per-mission StateTrees *inside* the Objectives phase.** This resolves the
   `// PLACEHOLDER(GDD 12.1)` in `EclipseMissionLogic.h` without a rewrite:
   `CanAdvancePhase` remains the single legality rule for
   Insertion → Objectives → Extraction → Debrief; a mission's authored
   choreography (ambush trigger, reinforcement wave, alarm response, heist
   escalation) is an optional `UStateTree` asset per mission whose tasks call
   subsystem APIs and report **named sub-phase facts** via the new
   `Event.Mission.PhaseChanged`. Missing StateTree asset → mission still runs on
   the outer phases alone (14.3.5). Rationale: R7 is answered on proven code;
   StateTree is added where authoring needs it, not where a doc said so.
4. **Story missions surface as pinned offers on the existing map/offer flow.**
   `DT_StoryMissions` rows pin a specific mission to a specific region; a
   pinned row takes precedence over the region-type offer in the existing offer
   query. Briefing = the offer's briefing text (+ recap cards before M1.1);
   Preparation/launch reuse P1-08 unchanged — `Event.Prep.MissionLaunchRequested`
   payload is **not** touched. Rationale: zero new mission-select flow; the
   strategic map stays the single place missions come from.
5. **Zero new objective primitives.** The four existing primitives cover all
   four missions: M1.1 `DestroyTarget` (patrol) + `ExtractSquad`; M1.2
   `CollectItem` (drops) + `ExtractSquad`; M1.3 `DestroyTarget` (jammer) +
   `ExtractSquad`; M1.4 `CollectItem` (armory crates) + `ExtractSquad`.
   Authored feel comes from spawn choreography, sub-phase trees, and site
   dressing — not from new verbs. Any primitive addition requires a spec
   amendment with the beat it unblocks named. Rationale: the generator (11.3)
   must one day compose the same primitives; every bespoke primitive is future
   debt.
6. **World-state discipline: M1.3 is the *first* world-state change** (2.9).
   M1.1, M1.2 and M1.4 set `bProgressRegionOnSuccess = false` — they commit
   rewards, casualties, story beats, roster/loadout changes, **never** a region
   flip. M1.3's completion is consumed by SPEC-P2-05, which commits the
   Foothold liberation flips; M1.3's own transaction carries only
   rewards + beat. Rationale: the liberation instance stays P2-05's single
   authorship point — two writers of region state is the divergence bug 12.3
   exists to prevent.
7. **Stealth = routes + existing facts, no detection model** (the 8.5 line).
   The stealth-viable route per 11.4 is carried by **site layout** (a patrol-free
   approach lane + a flanking/sewer insertion per mission) and the existing
   perception → attack BT facts. One new *pure fold*, not a model: site alarm
   `bAlarmRaised` = any enemy has entered combat (existing facts), latched per
   mission, powering the ghost optional objectives and alarm sub-phases. The
   8.5 five-stage alert meter (Unaware → … → Hunted), light/noise perception,
   body management: **Phase 3.** Rationale: the slice must prove routes exist,
   not build the stealth sandbox.
8. **Every mission is completable with Command Mode Stage A's four verbs**
   (Move/Hold, Focus, Regroup + stances) — the R3 verdict is open, so Stage B
   verbs are *enhancements with authored fallbacks*, never requirements:
   M1.4's armory door has a breach point (Stage B `Breach`) **and** an
   interaction-openable side grate; M1.2's quiet route is walkable without the
   `Stealth` stance (patrol gaps are generous enough at Unaware). If R3 lands
   "false", every mission still ships. Rationale: honest dependency on a
   falsification that has not yet run (EXECUTION_PLAN §2-S7).
9. **M1.1 scarcity is loadout-tier + encounter design, not an ammo system.**
   Pre-Workshop prep offers scavenged gear only (SPEC-P2-03), the ambush
   structure gives one free volley, and the enemy count is low-but-lethal;
   "thirteen bullets" stays fiction (briefing text), because an ammo economy is
   a new inventory/GAS system the slice doesn't need (Non-goals). Rationale:
   the lesson is *combat-as-problem-solving* (2.9) — positioning and the first
   volley teach it; a bullet counter is Phase 3+ if ever.
10. **Calendar + reward bands: the SPEC-P2-03 econ-check proposal is adopted as
    initial data.** Slice soak calendar ~16 days, missions ≈ day 2 / 5 / 9 / 13
    (soak script, not a player-facing gate — pacing emerges from build/wait);
    rewards M1.1 +25 M/50 C · M1.2 +15 M/30 C/8 I · M1.3 +40 M/60 C/4 I
    + 60 M capture-intact windfall · M1.4 +200 M/100 C; optional objectives
    ≤ +80 M total. All values live in `DT_StoryMissions`/objective rows and are
    tunable without spec revision (6.5 levers); the P2-03 three-path econ
    invariants are the soak contract. Rationale: the numbers were already
    balance-checked against the ledger as built; this spec just makes them
    authoritative data.
11. **Brick is a roster record, not a companion system.** M1.4's debrief
    transaction adds Brick as a pre-classed **Assault** authored recruit
    (existing `Event.Roster.SoldierAdded` path, body via the
    `DT_NamedCharacters` slot), plus `Story.Beat.BrickRecruited`. Barks come
    from his P2-01 bark set. Companion loyalty/dialogue systems: Phase 3+.
    Rationale: 2.9 asks "recruit Brick", and the roster already knows how.
12. **Mission resolution advances the clock +1 day** — inherited verbatim from
    SPEC-P2-03 locked decision 4; the debrief transaction commits it. No
    checkpoint saves this spec: the P1-05 contract "nothing committed until
    debrief; quitting mid-mission loses no strategic state" **holds**, and the
    mid-mission checkpoint question is named as an explicit seam for
    **SPEC-P2-06** (30–50 min story missions per 11.4 make checkpoints a real
    need — but save infrastructure is spec 06's, not content's).

## The four missions
All four play on the **existing graybox district** (~200×200 m, P1-05); per-
mission variation = a site-variant layer (level instance/Data Layer: props,
one small interior blockout for M1.4's armory, patrol routes, spawn/objective/
insertion tags) — **dressings and variants, never a new district or system**.
Spaces are declared *locked* per mission when its Gauntlet lands, which is the
SPEC-P2-08 start signal ("04 — spaces locked"). Length target 20–40 min each
(11.4 story band, slice-trimmed).

| # | Goal (one sentence) | Approach families (11.4) | Region flip |
|---|---|---|---|
| M1.1 *Thirteen Bullets* | Ambush a Dominion supply patrol at a chokepoint with scavenged gear and one free volley. | Quiet setup → loud spring (canonical) / full loud frontal | no |
| M1.2 *The Dead Drop* | Retrieve two intel caches across patrolled blocks without raising the alarm. | Ghost (canonical) / loud fail-forward | no |
| M1.3 *Signal Fire* | Destroy the district jammer tower and get out before the response closes. | Quiet approach + sabotage / loud assault | via P2-05 |
| M1.4 *The Quartermaster* | Heist the Dominion armory for real weapons and walk out with Brick. | Quiet infiltration (grate) / loud breach-and-clear | no |

**M1.1 — Thirteen Bullets** *(teaches combat-as-problem-solving + Command Mode)*
- Loop: recap cards + briefing (Mara text) → prep (scavenged tier only; squad
  pick) → execution: reach the overlook (`ReachLocation` implicit via sub-phase
  arm-trigger — the ambush arms when the squad holds position), patrol enters,
  `DestroyTarget` (patrol leader) + kill set, reinforcement sub-phase on
  `bAlarmRaised` → `ExtractSquad` → debrief.
- Stealth-viable route: the overlook lane is patrol-free; a squad that holds
  position and opens on its own terms fights 4 enemies staggered instead of 7
  at once. Loud frontal is allowed and survivable, just harder.
- Teaching beat: briefing prompt "hold [command] — think faster"; the debug
  prompt clears when `Event.Command.ModeEntered` fires (the catalog already
  names P2-04 as this consumer). Debug-grade prompt; real tutorial UI = P2-07.
- Commit: +25 M/50 C, casualties, day +1, `Story.Beat.M11_ThirteenBullets`.
  Optional: zero-casualty (+20 M).

**M1.2 — The Dead Drop** *(teaches stealth + intel currency)*
- Loop: briefing (drop locations shown only if the 5 I prep reveal was bought —
  the 11.1 fairness loop made explicit; without it, drop *areas* only) →
  execution: 2× `CollectItem` in separate patrol zones; alarm sub-phase spawns
  a hunter set and marks the ghost optional failed → `ExtractSquad` → debrief.
- Stealth-viable route: patrol gaps at Unaware are generous (decision 8);
  sewer insertion bypasses the first zone entirely. Loud is fail-forward
  (11.4): alarm ≠ mission failure, it costs the optional and adds pressure.
- Commit: +15 M/30 C/8 I, day +1, `Story.Beat.M12_DeadDrop`. Optional: ghost
  — never alarmed (+10 M, +4 I).
**M1.3 — Signal Fire** *(first district-level world-state change)*
- Loop: briefing → prep (insertion choice matters: tower plaza vs. service
  tunnel) → execution: `DestroyTarget` (jammer tower base), timed response
  sub-phase after the blast (the district *answers*) → `ExtractSquad` under
  pressure → debrief.
- Stealth-viable route: service tunnel reaches the tower base unseen; the
  blast itself always raises the alarm — quiet buys setup, extraction is
  always contested (in-fiction flag per 11.4's mandatory-combat rule).
- Commit: +40 M/60 C/4 I, day +1, `Story.Beat.M13_SignalFire`. Optional:
  SupplyDepot stores captured intact (+60 M windfall, 6.3.3).
  `Event.Mission.Completed` for M1.3 is the **SPEC-P2-05 seam**: the
  liberation instance consumes it and commits the Foothold region flips —
  this spec commits none.

**M1.4 — The Quartermaster** *(first real weapons; recruit Brick)*
- Loop: briefing → prep (IC intel reveals guard rotation if bought) →
  execution: enter the armory (breach point *or* side grate, decision 8),
  2× `CollectItem` (weapon crates), escalation sub-phases on alarm and on
  first crate (quartermaster locks down; Brick — a prisoner-conscript in the
  fiction — opens the back route) → `ExtractSquad` → debrief.
- Stealth-viable route: grate infiltration + rotation gaps reaches crate 1
  unseen; crate 2 always triggers lockdown (escalation is authored, not
  detection-driven).
- Commit: +200 M/100 C, day +1, Brick roster add (decision 11),
  `UnlockedLoadoutTags` += real-rifle tier (existing P1-03 mechanism — "first
  real weapons" is literally the loadout unlock), `Story.Beat.M14_Quartermaster`
  + `Story.Beat.BrickRecruited`. Optional: no soldier downed (+15 M).

## Data schema (14.5 step 1)
- `FEclipseCampaignState` gains `TArray<FGameplayTag> StoryFlags` —
  **SchemaVersion 4 → 5**, migration entry + v0 and v4 fixture tests in the
  same commit (14.3.6, R6 discipline). Flags are set-only in Phase 2.
- `DT_StoryMissions` — `FEclipseStoryMissionRow`: `MissionId`,
  `MissionAsset (TSoftObjectPtr<UEclipseMissionAsset>)`, `PinnedRegionId`,
  `UnlockBeatTag` (empty = available from campaign start), `CompletionBeatTag`,
  `RewardCredits/Materials/Intel`, `BriefingText`, `BriefingSpeaker`.
  Sequence M1.1 → M1.4 is expressed purely by unlock tags.
- `UEclipseMissionAsset` gains: `SubPhaseTree (TSoftObjectPtr<UStateTree>,
  optional)`, `TeachingPromptText (FText)` + `TeachingClearEventTag
  (FGameplayTag)` (M1.1 only in the slice), `AuthoredConsequences`
  (`FEclipseAuthoredConsequence`: roster-add row ref in `DT_NamedCharacters`,
  loadout unlock tags — empty for M1.1–M1.3).
- `FEclipseObjectiveDef` gains `bRequiresNoAlarm (bool)` (ghost optionals) and
  `OptionalRewardCredits/Materials/Intel (int32)` — read only when `bOptional`.
- `DA_CampaignSetup` gains `RecapCards (TArray<FEclipseRecapCard>: still ref +
  FText lines)` (decision 1).
- Missing anything → logged warning + graceful default (14.3.5): missing story
  row = mission never offered (campaign still runs on region offers); missing
  StateTree = outer phases only; missing recap stills = text-only cards.

## Pure-logic core (14.5 step 2)
- `EclipseStoryLogic` (new, headless per 14.3.2): pinned-offer resolution over
  (`StoryFlags`, story rows) — which mission is pinned where, deterministic;
  beat-set idempotence (completing a mission whose beat is already set commits
  nothing twice — Brick can never join twice).
- Site-alarm fold (decision 7): latch over existing combat-entry facts;
  ghost-objective evaluation (`bRequiresNoAlarm` × alarm state) as a pure
  function.
- `ComposeConsequences` extends over authored consequences (story beats,
  roster add, loadout unlocks) — win/lose/optional permutations stay one pure
  function; fail-forward unchanged (failure commits casualties + partial
  rewards + day, never a beat).
- `CanAdvancePhase` untouched — sub-phase trees run *inside* Objectives and
  cannot violate the outer machine by construction (decision 3).

## Events (14.3.4 — before code; catalog rows land with the implementation commit)
New payload struct `FEclipseStoryEventPayload` (Core/EclipseEventPayloads.h):
`BeatTag (FGameplayTag)`, `Day (int32)`.

| Tag | Payload | Emitted by | Consumed by |
|---|---|---|---|
| `Event.Mission.PhaseChanged` | `FEclipseMissionEventPayload` (mission id, phase name in the objective-id field) | MissionSubsystem (outer phase transitions) + sub-phase StateTree tasks via subsystem API (named authored beats) | audio (P2-09 combat layers), UI (P2-07), debug HUD, scenario tests |
| `Event.Story.BeatReached` | Story payload (BeatTag, Day) | **CampaignState commit only** (debrief transaction sets a story flag) | story pinned-offer refresh (map), UI (P2-07), audio sting (16.12), soak asserts |

Consumed (existing, no payload changes): `Event.Prep.MissionLaunchRequested`
(launch), `Event.Command.ModeEntered` (M1.1 teaching-beat clear),
`Event.Mission.ObjectiveCompleted` (sub-phase triggers, ghost evaluation),
`Event.Squad.SoldierDowned` / `SoldierStabilized` (outcome composition).
Emitted (existing): `Event.Mission.Started` / `ObjectiveCompleted` /
`Completed` / `Failed` — unchanged. **No other new tags**: alarm state and
escalations travel as `PhaseChanged` facts, not bespoke events; facts not
commands throughout (14.3.3).

## Build order (14.5 — R7 explicit)
1. **R7 falsification — M1.1 skeleton Gauntlet** (before everything):
   M1.1 mission asset on the runtime as built + pinned-offer stub + Gauntlet
   spawn → complete-by-script → asserts (rewards committed, day +1, region
   unchanged, outcome recorded). **Green = authorization to author M1.2–M1.4;
   red = runtime amendment first** (decision 2). Uses temporary hardcoded
   pinning if `DT_StoryMissions` isn't in yet — the skeleton tests the runtime,
   not the story layer.
2. Data schema: `StoryFlags` (v4→v5 + fixtures), `DT_StoryMissions`, asset/row
   extensions, recap cards.
3. Pure logic + unit tests (`EclipseStoryLogic`, alarm fold, consequence
   permutations).
4. Subsystem wrapper + both new events + catalog rows (same commit).
5. Debug UI: recap cards on the briefing debug surface, teaching prompt,
   `Eclipse.Story.Report` console dump (flags, pinned offers, alarm state).
6. Content last, one mission at a time in play order, each landing with its
   Gauntlet green before the next starts: site variant (eb per mission site,
   parallelizable once its predecessor's *spaces* are locked), sub-phase tree,
   barks/briefing text. M1.1 content first — it is also the R8 recap testbed.

## Integration
- Offer precedence: story-pinned rows resolve before region-type rows in the
  existing offer query (`GetAvailableOffers`/`TryGetOffer`);
  `Event.Strategy.MissionSelected` and the whole P1-08 prep flow unchanged.
- SPEC-P2-05 seam: M1.3 `Event.Mission.Completed` is the liberation trigger;
  this spec guarantees the mission id is stable and the event fires exactly
  once per completion (idempotence via beat flag).
- SPEC-P2-06 seam (named, not built): mid-mission checkpoints for 30–50 min
  missions — spec 06 decides checkpoint granularity; until then quitting
  mid-mission costs mission progress, never strategic state (P1-05 contract).
- SPEC-P2-08 seam: per-mission "spaces locked" declaration at Gauntlet-green
  starts the fidelity clock for that site.
- Consent/asset discipline: site variants are graybox + existing curated
  assets; any new Fab pulls ride the standing owner click-list (never
  auto-installed).

## Tests (14.4, blocking)
- **Unit:** story sequencing (each beat unlocks exactly its successor; missing
  row/asset warns and degrades, never crashes); beat idempotence (double-
  complete M1.4 → one Brick, one loadout unlock); alarm fold + ghost
  evaluation; consequence permutations per mission (win/lose × optionals);
  v4→v5 migration (v4 fixture loads with empty `StoryFlags`).
- **Functional (Gauntlet, per mission — the 14.4 canonical):** spawn →
  complete-by-script → asserts: rewards match rows, day +1, beat flag set,
  region unchanged (M1.1/M1.2/M1.4), Brick in roster + loadout tags unlocked
  (M1.4), fail path commits fail-forward consequences (casualties + day, no
  beat). The M1.1 instance of this **is** the R7 falsification (build step 1).
- **Scenario suite (per merge):** "quiet route M1.2" — squad walks the ghost
  route with Stage A verbs only, alarm never latches; "ambush under fire" —
  the P2-02 order-under-fire scenario re-run inside M1.1's arena. Any silent
  order failure = release blocker (unchanged bar).
- **Save integrity:** quit mid-mission at every outer phase → strategic state
  hash unchanged; story flags round-trip; checkpoint coverage explicitly
  deferred to SPEC-P2-06 (seam above).
- **Campaign soak (nightly):** scripted M1.1 → M1.4 on the ~16-day calendar
  (missions ≈ day 2/5/9/13) **through the M1.3 → liberation handoff**
  (P2-05 seam): all four beats set, Brick recruited, the P2-03 three-path econ
  invariants stay green, `Wages_Short` never blocks progression.
- **Performance:** per-merge 12.4 budget check on each site variant (agent
  count within the ≤40 bubble; M1.4 lockdown wave included).

## Definition of Done
R7 Gauntlet (M1.1 skeleton) green and logged as the risk verdict in
EXECUTION_PLAN §3 · all four missions playable cold from a new campaign via
the map — recap → M1.1 … M1.4 → Brick on the roster and real rifles in prep ·
recap cold-reader check passed (4/4, decision 1) or rewrite queued · both new
events in catalog (implementation commit) · v4→v5 migration + fixtures green ·
per-mission Gauntlets + scenario suite + soak-with-handoff green · spaces-locked
declared per site (P2-08 start signal) · dashboard updated.

## Non-goals
No playable prologue (decision 1 — gate review can reopen it) · no cutscene
tech (Part 17 later; recap/briefings are static screens — this is not the spec
that buys a camera) · no dialogue system dependency (briefing text ≠ the 12.3
node-graph decision) · **no 8.5 detection model beyond existing facts** — no
alert-stage meter, no light/noise perception, no body management; the binary
site-alarm fold is the entire stealth surface (decision 7) · **no new AI
systems** — existing archetypes/BT/perception only; enemy variety = archetype
rows + spawn choreography · no new objective primitives (decision 5; amendment
gate) · no ammo/inventory system (decision 9) · no companion system (Brick =
roster record, decision 11) · no Mission Generator (11.3, Phase 3) · no
M1.5–M1.8 · no new order verbs or Command Mode changes (SPEC-P2-02 owns; Stage
B used only with fallbacks, decision 8) · no region flips outside the P2-05
seam (decision 6) · no mission-select/briefing/tutorial UI polish (SPEC-P2-07)
· no fidelity dressing (SPEC-P2-08 owns; this spec delivers graybox variants +
locked spaces) · no mid-mission checkpoints (SPEC-P2-06) · no morale/reputation
reactions to mission outcomes (Phase 3).

## Open review points — RESOLVED at main concept review (2026-07-24)
1. **Reward bands: adopted verbatim, with one added soak invariant.** The
   M1.4 +200 M windfall stands (it *is* the heist beat), but the nightly econ
   soak gains the assert: **immediately post-M1.4, no path can afford
   Workshop L2 and the Intelligence Center simultaneously** — the windfall
   funds *a* choice, never both (6.1: a choice with no cost is no choice).
   Breaks → retune the windfall first, not the facility costs (P2-03 owns those).
2. **`PhaseChanged` payload: do NOT overload the objective-id field.**
   `FEclipseMissionEventPayload` gains `PhaseName (FName)` +
   `bAuthoredSubPhase (bool)` in the wrapper commit (payloads are transient —
   no schema impact). Rationale: a field named ObjectiveId carrying phase names
   is exactly the readable-catalog rule this project keeps winning by.
3. **Cold-reader logistics: accepted as written**, plus fallback — if T-7
   reviewer recruitment slips past M1.1 content-complete, one interim cold
   reader (a human outside the project, not an AI session) enters the owner
   click-list; the check never floats past M1.2 authoring start.
4. **Brick = Assault: CONFIRMED.** He is a morale carrier (13.1) walking out of
   an armory carrying the first real rifles — Assault fits the beat and avoids
   a second Medic in a 3-class slice. Standing veto for the Narrative Designer
   until M1.4 content starts, as drafted.

*Verdict: concept ACCEPTED as planning basis. Implementation starts at build
step 1 (R7 Gauntlet) once P2-02 Stage A and P2-03 step 3 have landed; the
v4→v5 schema break follows R6 discipline in its own changeset.*
