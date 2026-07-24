# SPEC-P2-05 — Liberation Template Instance — **CONCEPT for main-review (2026-07-24)**
*Phase 2 feature spec | GDD refs: 11.2 (campaign-mission templates; the planet-liberation skeleton — Foothold is its named first phase), 11.3 (inputs only, no generator), 3.1/3.3 (district graph scale-down), 6.3.2 (owner yield trichotomy 100/40/0), 2.9/13.2 ("seeing one district liberated on the map"), 14.3.2–14.3.5, 14.5, SPEC-P1-02/04 (transaction API, region graph, `Event.Strategy.RegionControlChanged` as built), SPEC-P2-03 (econ calendar + soak invariants), SPEC-P2-04 (M1.3 seam, decision 6, beat flags), EXECUTION_PLAN backlog #7 | Skill owner: Game Architecture Expert (seam + transaction); World Builder consulted (map read)*

## Purpose
Instantiate one campaign-mission template (11.2) on Kessara's district graph so
that M1.3 *Signal Fire* — the slice's first world-state change (2.9) — visibly
flips region state and **the strategic layer answers**: new map colors, a new
income band, a new offer frontier. This is the systemic half of "authored +
systemic" proven at slice scale, and the last consequence layer the 13.2 gate
sentence needs ("seeing one district liberated on the map").

The spec is falsification-first on one claim: **the as-built strategic machinery
already carries a liberation** — authored completion → one CampaignState
transaction → the board answers — with *zero* new events, *zero* schema change,
and *zero* bespoke scripting. Two falsifications:
- **F1 — the chain carries.** The functional Gauntlet (scripted M1.3 completion
  → one commit → three `RegionControlChanged` in commit order → offers + income
  answer) is green on existing machinery. If it cannot be (bus re-entrancy,
  transaction shape), the machinery is amended **as a spec amendment** — never
  worked around with a per-mission script (the P2-04 decision 2 discipline,
  re-applied).
- **F2 — the calendar coupling holds.** The P2-03/P2-04 econ calendar *assumed*
  this spec's output (Foothold trio flips ≈ day 9 → 28 M / 110 C / 2 I per
  day) before it existed. The nightly soak now asserts that assumption against
  the real flip; if the three P2-03 econ paths break, the calendar was wrong
  and gets retuned in data — the flip set does not silently shrink to fit.

Everything here is consequence wiring, not content: M1.3 itself is SPEC-P2-04's;
the fidelity read of a liberated district is SPEC-P2-08's; the UI is P2-07's.

## Locked decisions (this spec)
1. **Scale: the Foothold trio — TransitCheckpoint + WorkerHousing + SupplyDepot
   flip to Player on M1.3 completion** (SPEC-P2-00 open question 4 resolved, per
   the standing proposal). Rationale: the slice economy was balance-checked
   against exactly this flip — the trio is the *minimal* set that touches all
   three resource columns the calendar leans on (TC = C+I, WH = C, SD = M+C),
   yielding the 28 M / 110 C / 2 I band from day ~10.
   **Alternative (one region) rejected:** no single region carries the band —
   e.g. TransitCheckpoint alone leaves materials at 8 M/day for the whole slice
   (−140 M against the 688 M baseline), the Builder path cannot finish its third
   L1 build and Thrifty loses Workshop L2, so *all three* P2-03 soak invariants
   break; and a one-region flip is mechanically indistinguishable from the
   Phase 1 one-step ladder — it would prove nothing the slice doesn't already
   have. Shape honesty besides: 11.2's liberation skeleton names **Foothold** as
   its first phase; a contiguous three-region pocket around the Underworks *is*
   a Foothold in miniature (all four player regions connect: UW–TC, UW–WH,
   WH–SD, TC–SD), and the fiction concurs — M1.3 kills the *district* jammer,
   so the district answers, not one block.
2. **Single authorship + trigger: this spec is the only writer of region flips
   from authored missions.** The trigger is the existing
   `Event.Mission.Completed` for M1.3's stable mission id (the P2-04 seam
   guarantee: fires exactly once per completion, only on success — `Failed` is
   a different tag). `UEclipseStrategySubsystem` consumes it, resolves the flip
   set through the pure core, and proposes **one transaction**
   (`Source = "LiberationInstance"`) of existing `SetRegionOwner` mutations;
   `UEclipseCampaignSubsystem::CommitTransaction` commits it (14.3.3 — the
   board proposes, the transaction commits; the subsystem header's own contract
   "region flips arrive exclusively through CampaignState commits" holds).
   M1.3's debrief transaction stays flip-free (`bProgressRegionOnSuccess =
   false`, P2-04 decision 6) — as do M1.1/M1.2/M1.4, regression-tested here.
   The Phase 1 one-step ladder for *generic region-type offers* is untouched:
   it rides the same single `SetRegionOwner` mutation + commit path, so there
   is one mutation type, one committer, and no second bookkeeping to diverge.
   Rationale: two writers of region state is the divergence bug 12.3 exists to
   prevent; one writer *per mission family* on one shared mutation is not.
3. **Idempotence is state-derived — no stored marker, no schema change.** The
   pure resolution excludes every region already at the target owner and drops
   unknown region ids (warning); an empty remainder means **no transaction, no
   events, one log line**. This is not just elegance, it is load-bearing: the
   as-built `ValidateMutation` *rejects* a no-op `SetRegionOwner` ("already has
   that owner") and `CommitTransaction` is atomic — a naive always-three-flips
   proposal would be rejected wholesale on any re-entry or player pre-flip. The
   beat flag cannot gate this instead: the debrief commit (which sets
   `Story.Beat.M13_SignalFire`) completes *before* the `Completed` broadcast in
   the as-built order, so the flag is already true on the first legitimate
   trigger. Consequences, both intended: a hypothetical second M1.3 completion
   commits nothing (the task's exactly-once contract), and a player who already
   liberated part of the trio through generic offers gets exactly the remainder
   — the resolution is monotone, never un-flips, never double-pays (rewards are
   P2-04's, ownership is ours). Region owners already serialize in
   `FEclipseCampaignState.Regions`; **SchemaVersion is untouched by this spec.**
   Escape hatch, not expectation: should main-review demand a stored
   liberation marker after all, it enters under full R6 discipline (version
   bump + migration entry + fixture test, same commit).
4. **No new events — `Event.Strategy.LiberationPhaseAdvanced` is not added**
   (deliberate amendment of the SPEC-P2-00 spec-sheet line; flagged for
   main-review). The slice instance has exactly one phase, so the proposed
   family would carry exactly one possible value all Phase 2 — catalog noise
   that violates the "new tags only when strictly needed" bar. The three
   `RegionControlChanged` facts in one commit *are* the complete fact stream,
   and every intended consumer already listens: strategy map (P1-04), base hub
   ledger, P2-08 occupation Data Layers, soak asserts. "The trio is done" is a
   pure fold over those three facts (provided in the core for consumers and the
   debug report); an audio sting keys off M1.3's `Event.Story.BeatReached`
   (16.12), not a region event. A phase event earns its place in Phase 3, when
   Foothold → Momentum → Capital Push makes it a real state machine (named
   seam, with Dominion response tiers / backlog #18).
5. **The instance is data, not code: `DT_LiberationInstances`** — no hardcoded
   trio anywhere. One row (`FEclipseLiberationRow`) maps mission-beat →
   region-set → owner-transition; the slice ships exactly one row (Foothold).
   14.3.5 degradation ladder: missing table or no row for the completed mission
   = **no flip + one logged warning, never a crash** — M1.3 still completes,
   the campaign still runs, and the soak catches the missing income band;
   unknown region id inside a row = dropped in resolution with a warning while
   the remaining ids still flip (a typo must not reject the whole liberation —
   the atomic transaction would); empty `RequiredBeatTag` = gate skipped.
   Rationale: Phase 3's real liberation campaigns will instantiate this same
   row shape per planet; hardcoding the slice trio would be the future debt
   P2-04 decision 5 warns about.
6. **Owner-only mutation.** The flip changes `Owner` and nothing else —
   `Unrest` and `GarrisonStrength` are untouched, and no reward/resource rides
   the liberation transaction (M1.3's debrief already paid). Rationale: every
   additional side-effect is a Phase 3 pressure-system decision (response
   tiers, backlog #18) this slice must not pre-empt with numbers nobody
   balances yet.

## The strategic layer answers (11.2 — what visibly changes after the flip)
| Surface | Change | Carried by (existing) |
|---|---|---|
| Strategy map | Three nodes turn player-held; the liberated pocket reads contiguous | `Event.Strategy.RegionControlChanged` ×3 → P1-04 map widget |
| Income | Next economy tick pays 28 M / 110 C / 2 I (owner factor 100%, 6.3.2); ledger lines name each region (7.6 transparency) | `Event.Campaign.DayAdvanced` tick + `Event.Economy.ResourcesChanged` |
| Intel stream | +2 I/day standing (TransitCheckpoint) — the P2-03 "territory pays the intel" beat | same economy tick |
| Offer board | The trio stops being a target (player-held); the frontier extends to **FoundryRow + CommsRelay** — the whole remaining district becomes strikable, the map literally opens | `GetLegalMissionTargets` adjacency rule + `GetAvailableOffers`, unchanged |
| District dressing | Occupation-state Data Layers flip in the fidelity district | `RegionControlChanged` → SPEC-P2-08 (consumer named there; delivered there) |
| Debrief/debug surface | The row's `ContextLine` shown once after the flip commit (11.3's causal rule, hand-written) | debug UI (step 4); real presentation = P2-07 |

## Data schema (14.5 step 1)
- `DT_LiberationInstances` — `FEclipseLiberationRow`:
  `TriggerMissionId (FName)` (matches the stable `Event.Mission.Completed`
  payload id — the P2-04 seam guarantee), `RequiredBeatTag (FGameplayTag)`
  (audit gate against the P2-04 story flags; empty = ungated; checkable in pure
  logic once v5 `StoryFlags` land), `RegionIds (TArray<FName>)`,
  `NewOwner (EEclipseRegionOwner)` (Player for the slice; data so a Phase 3 row
  can express a Contested step), `ContextLine (FText)`.
- `UEclipseCampaignSetupAsset` gains
  `LiberationInstances (TSoftObjectPtr<UDataTable>)` — same slot pattern as
  `Facilities`/`ClassDefs`; missing = warning + no liberations (14.3.5).
- Boot/editor validation mirrors `ValidateGraph`: row region ids must exist in
  the region graph; duplicates and empty sets are reported.
- The slice row (Foothold: the trio → Player, gated on
  `Story.Beat.M13_SignalFire`) lands in the phase-2 content script, not in C++.
- **No `FEclipseCampaignState` change, no SchemaVersion bump** (locked
  decision 3).

## Pure-logic core (14.5 step 2)
New `EclipseLiberationLogic` (Strategy/, headless per 14.3.2 — no engine actor
headers, deterministic, unit-tested):
- `ResolveLiberationTransaction(State, Row) → FEclipseCampaignTransaction`:
  filters the row's region set (unknown id → dropped + reported; owner already
  at target → excluded), emits one `SetRegionOwner` mutation per remaining id
  **in row order** (commit order = event order = row order — deterministic for
  tests and the map), `Source = "LiberationInstance"`. Empty result = nothing
  to commit.
- `IsLiberationTriggered(Row, CompletedMissionId, StoryFlags) → bool`:
  id match + optional beat gate. (Until P2-04's v5 `StoryFlags` land, the flags
  view is empty and only ungated rows fire — the dependency is honest: this
  spec builds after 04.)
- `IsLiberationComplete(State, Row) → bool`: the "trio is done" fold consumers
  and the debug report use (locked decision 4) — true iff every row region is
  at the target owner.

## Events (14.3.4 — before code; catalog rows updated in the implementation commit)
**No new tags** (locked decision 4). The catalog changes are two *consumer/
emitter* row updates:

| Tag | Payload | Change |
|---|---|---|
| `Event.Mission.Completed` | `FEclipseMissionEventPayload` — unchanged | Consumed-by += StrategySubsystem (liberation trigger) |
| `Event.Strategy.RegionControlChanged` | `FEclipseStrategyEventPayload` — unchanged | Emitted-by unchanged (CampaignSubsystem commit — now also from the liberation transaction); Consumed-by += P2-08 occupation Data Layers, soak asserts |

Facts, not commands, throughout: the liberation listener never mutates on the
bus — it proposes a transaction; the commit emits the facts (14.3.3).

## Build order (14.5)
1. **Data:** row struct + `DT_LiberationInstances` + setup-asset slot +
   validation; content row for the Foothold trio in the content script.
2. **Pure core + unit tests** (`EclipseLiberationLogic`, the matrix below).
3. **Wiring:** StrategySubsystem subscription + commit + the two catalog row
   updates (same commit). Includes the **re-entrancy check**: the liberation
   commit broadcasts `RegionControlChanged` from *inside* the
   `Mission.Completed` dispatch — asserted safe in the Gauntlet, not assumed
   (F1; if the P1-01 bus can't carry a nested broadcast, the fix is a bus
   amendment, not a deferred-tick workaround that breaks commit-order
   determinism).
4. **Debug surface:** `Eclipse.Liberation.Report` console dump (rows loaded,
   trigger armed/consumed, last resolution incl. dropped/excluded ids,
   `IsLiberationComplete`); the ContextLine on the existing debrief debug
   screen. The P1-04 `FlipRegionCommand` stays the manual debug path.
5. **Content/soak last:** soak script gains the day-~9 handoff asserts (Tests).

## Integration
- **Ordering (as built, load-bearing):** M1.3's debrief transaction commits
  fully *before* `Event.Mission.Completed` broadcasts
  (`TryResolveMission`: commit → broadcast). The liberation commit therefore
  runs strictly after rewards/beat/day — no nested-transaction hazard, and the
  day has already advanced, so the new income band starts at the *next*
  economy tick, exactly as the P2-03 calendar models (flip ≈ day 9 →
  "d10–16 @ 28 M").
- **SPEC-P2-04 seam (upstream):** stable mission id, exactly-once `Completed`,
  `bProgressRegionOnSuccess = false` on all M1.x — inherited, and re-asserted
  here as regression.
- **SPEC-P2-06 seam (downstream):** nothing new to serialize; spec 06 gains a
  post-Foothold campaign fixture (all trio Player) for its round-trip suite.
- **SPEC-P2-07/P2-08 seams:** UI beyond the debug surface is 07's; the
  district's visual answer (Data Layers per occupation state) is 08's — both
  consume the same three facts, nothing bespoke.
- **Player-driven pre-flips are legal:** generic region offers can flip trio
  members early (P1 ladder, decision 2); the monotone resolution absorbs any
  overlap (decision 3). The day-9 figure is a soak-script assumption, never a
  player-facing gate.

## Tests (14.4, blocking)
- **Unit** (deterministic, headless): resolution matrix — full trio from the
  campaign start owners (Dominion/Contested/Dominion) → 3 mutations in row
  order; partially pre-flipped → exactly the remainder; all at target → empty
  transaction; unknown id dropped + reported while the rest still flips;
  missing row/table → empty + degradation flag, never a throw; beat gate
  (gated row + flag unset → not triggered; empty gate → triggered on id);
  `IsLiberationComplete` truth table; resolved transaction always passes
  `ValidateMutation` for every mutation (the no-op-rejection contract).
- **Functional (Gauntlet — F1):** scripted M1.3 completion → exactly one
  liberation commit → three `RegionControlChanged` **in row order** (nested
  inside the `Completed` dispatch — the re-entrancy assert) → owners Player →
  next `DayAdvanced` tick pays 28 M / 110 C / 2 I → offer board answers
  (FoundryRow + CommsRelay legal, trio no longer offered). Then a forced
  second `Completed` for the same id → **no commit, no events, state hash
  unchanged, one log line**.
- **Regression (P2-04 decision 6 enforced):** scripted M1.1, M1.2 and M1.4
  completions each produce **zero** `SetRegionOwner` mutations and zero
  `RegionControlChanged` — asserted on both the debrief transaction contents
  and the bus.
- **Campaign soak (nightly — F2):** the existing M1.1→M1.4 slice script gains
  the liberation handoff asserts: the flip lands on the calendar day M1.3
  resolves (≈ day 9), the post-flip band is active from the next tick, and the
  three P2-03 econ paths (Builder / Intel opening / Thrifty) stay green
  end-to-end; `Wages_Short` never blocks the handoff.
- **Save integrity:** post-Foothold fixture round-trips (owners persist —
  covered by the existing state-hash contract; no migration test because no
  schema change).
- **Performance:** nothing new per-frame (event-driven only); the per-merge
  budget checks simply keep running.

## Definition of Done
Open question 4 recorded as resolved in SPEC-P2-00 · data + pure core + wiring
landed, catalog consumer rows updated in the implementation commit · scripted
M1.3 in PIE flips the trio on the debug map, prints the ContextLine, and the
next day's ledger shows the 28/110/2 band · idempotence Gauntlet green
(second completion commits nothing) · regression green (M1.1/2/4 flip
nothing) · soak with the day-~9 handoff asserts green three nights running ·
F1/F2 verdicts logged in EXECUTION_PLAN §2 · dashboard updated.

## Non-goals
No Dominion response tiers or counter-pressure (Phase 3, backlog #18 — the
strategic AI does not answer back yet, decision 6) · no re-capture/reconquest
(owners never regress in Phase 2; a liberated region stays liberated) · no
additional liberation instances or templates (exactly one row ships; the row
*shape* is the Phase 3 contract, not its content) · no Momentum/Capital Push
phases and no `LiberationPhaseAdvanced` event (decision 4; Phase 3 seam) · no
Mission Generator (11.3 read for input shape only) · no unrest/garrison/morale
side-effects of liberation (decision 6) · no new regions, graph edits, or
second districts · no UI beyond the debug surface (P2-07 owns presentation; the
map "moment" — stinger, banner — is P2-07/P2-09 material over existing facts) ·
no changes to the P1 generic offer ladder (as built; decision 2) · no new
event tags, no schema bump, no save-migration entry (nothing changed shape).

## Open review points (for main)
1. **The `LiberationPhaseAdvanced` deviation** (decision 4) amends the
   SPEC-P2-00 spec-sheet line ("new `Event.Strategy.LiberationPhaseAdvanced`").
   Confirm dropping it outright over shipping it as declared-but-unemitted;
   proposal: drop — an unemitted tag is catalog debt with no consumer.
2. **Generic-ladder purity** (decision 2): the P1 one-step ladder can flip trio
   members before M1.3 (player-driven). Proposal: leave as-built — the
   monotone resolution absorbs overlap and free play must not regress.
   Alternative (cap generic flips at Contested for the slice) only if review
   wants the M1.3 moment guaranteed-maximal; that is a feel call, not an
   architecture need.
3. **`RequiredBeatTag` gate**: keep (data-couples the row to the P2-04 beat
   system + audit value once `StoryFlags` land) vs. drop (mission-id matching
   alone suffices mechanically). Proposal: keep, empty-means-ungated.
4. **Garrison on flip**: should the liberation commit also zero
   `GarrisonStrength` on flipped regions (fiction: garrison routed)? Proposal:
   no (decision 6) — nothing reads it post-flip in the slice; Phase 3's
   pressure systems own that number.

## Main concept review — RESOLVED (2026-07-24)
1. **`LiberationPhaseAdvanced`: DROPPED outright.** An unemitted tag is
   catalog debt with no consumer; `RegionControlChanged` in commit order is
   the complete fact stream. Recorded as a SPEC-P2-00 sheet amendment — the
   sheet line is superseded by this spec.
2. **Generic ladder stays as-built.** Capping generic flips at Contested would
   put a story special-case inside the generic offer ladder — hidden coupling
   for a feel win the fiction doesn't need: a player who already contested a
   trio region *earned* a smaller M1.3 delta, and the monotone resolution
   absorbs the overlap correctly.
3. **`RequiredBeatTag`: KEEP**, empty-means-ungated — self-documenting data,
   audits for free once `StoryFlags` land (P2-04 build step 2), costs nothing.
4. **Garrison: untouched on flip**, as proposed — owner-only mutation keeps
   the Phase 3 response-tier design space clean.

*Verdict: concept ACCEPTED as planning basis. Implementation follows P2-04
build step 2 (StoryFlags) so the beat gate lands against real flags; the
nested-broadcast re-entrancy assert from the drafter's risk note is a
mandatory Gauntlet item in build step 3.*
