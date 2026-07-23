# SPEC-P2-02 — Command Mode (final feel)
*Phase 2 feature spec | GDD refs: 8.4 (order table, "orders are promises"), 4.1.2 (30% dilation, player fantasy), 4.1.1 (camera ~15% pullback), 9.5 (verbal transparency, priority stack), 12.4 (budgets), 14.3.4/14.5, graybox_feel_targets §4 (≤1 s answer bar), SPEC-P1-06 (order contract), SPEC-P2-01 (squad of 4; classes as data) | Skill owner: Combat Designer (feel verdict) + AI Systems Engineer (order table); Architecture Expert reviews the seams*

## Purpose
Take Command Mode from the Phase 1 hotkey order-picker to shipping feel. This is
**risk R3** (EXECUTION_PLAN §3): *does tactically commanding in real time at 30%
dilation survive actual firefights* — readability, chaos, comfort? The spec is
falsification-first (14.5): a **debug-feel version** (step 4 of 14.5 — ugly on
purpose) lands in the existing graybox district and renders the R3 verdict
**before** any camera work, UI polish, or new order verbs exist. "The mode feels
like thinking faster" is proven on the worst-looking build it will ever have;
polish can only make a true verdict better, never manufacture one.

The existing SPEC-P1-06 contract is the foundation, not a parallel system: the
pure `DecideOrder` table, the zero-silence broadcast in `EclipseSquadSubsystem`,
and the per-soldier `IssueOrder` API (already present, currently unbound) all
carry forward. Command Mode changes how orders are *issued*; execution and the
9.5 priority stack are untouched.

## Locked decisions (this spec)
1. **Hold-to-enter, ~30% global time dilation, debug-feel-first.** Hold the
   command input → time dilation to `0.30` (data); release → `1.0`. Stage A
   (the R3 gate) ships hold-to-enter + dilation + per-soldier selection on the
   **existing four verbs** in the **existing district**, with debug HUD lines
   only. Stage B (the rest of the 8.4 table) starts only after the Stage A
   verdict reads "true". Tactician full pause is **data-ready** (per-difficulty
   `DilationFactor`, `0` = pause) but headless-tested only — the slice ships one
   difficulty (SPEC-P2-00 scope).
2. **Answers run on the real-time clock.** The graybox feel bar (order → visible
   acknowledgment ≤ 1 s) is a *wall-clock* promise: at 30% dilation, 1 s of game
   time is 3.3 s of player time — an ack timer left on game time would triple
   the perceived latency and falsify R3 by accident. `ResponseTimeoutSeconds`
   and bark surfacing are measured in real (undilated) seconds while Command
   Mode is held. This is the single most load-bearing feel decision in the spec.
3. **Feel-verdict criteria — what makes R3 true/false** (measured on Stage A,
   before polish; verdict logged as the status line in EXECUTION_PLAN §2):
   - **Order round-trip:** hold → select soldier → order → visible/spoken
     acknowledgment ≤ 1 s wall clock, 10/10 consecutive orders.
   - **Targeting readability:** 10 consecutive orders under fire land on the
     intended soldier + target first try ≥ 9/10 (mis-picks = falsified).
   - **Dilation comfort:** owner plays 3 encounter beats; entering reads as
     *thinking faster*, not *opening a menu*; no disorientation/nausea; release
     resumes clean (no pop, no input eaten).
   - **Command trust under dilation:** the "order under fire" scenario (Tests)
     shows zero silent failures at 0.30 — the 8.4 promise holds while dilated.
   - **Usage pull:** in a free-play encounter the owner enters Command Mode
     voluntarily at least once per encounter beat (the P1-06 gate-signal logic,
     re-applied: an unused mode is a failed mode).
   - **Verdict "false" → stop.** Pre-authorized fallback ladder, decided as a
     spec amendment before any polish or Stage B work: (a) dilation 0.5,
     (b) full pause as default, (c) hold-to-order without dilation. Fidelity
     and UI investment on a falsified feel is forbidden (14.5 discipline).
4. **Per-soldier order targets** (SPEC-P2-01 lock): Command Mode adds soldier
   selection — cycle next/prev + direct pick under reticle — and routes through
   the existing per-soldier `IssueOrder`. No selection = all (the Phase 1
   `IssueOrderToAll` broadcast survives as the "everyone" path). No fireteams.
5. **Dilation is a wrapper layer** (14.3.2): the pure order/targeting logic
   never reads or branches on time dilation. One component owns
   apply/restore with a fail-safe (mode ends → dilation `1.0`, always — on
   release, pawn death, mission end, level travel). A stuck slow-mo is a
   release blocker on par with a silent order failure.
6. **Camera = one pullback float, nothing else.** The 4.1.1 "~15% pullback in
   command mode" ships as a spring-arm/FOV scalar in data. Blends, DoF,
   dollies, shoulder framing are explicitly not this spec (Non-goals).

## The 8.4 order table → implementation mapping
Existing = SPEC-P1-06/P2-01 as built. Per-class modulation (Assault push,
Sniper lane bias, Medic triage) **stays data** (SPEC-P2-01) — new verbs get
class flavor through the same tuning parameters, never through class branches.

| 8.4 order | Target | Existing verb | Phase 2 work (Stage B unless noted) |
|---|---|---|---|
| Move / Hold | Position + stance | `MoveTo`, `Hold` + `EEclipseSquadStance` | Stage A as-is. Stage B adds stance value `Stealth` (8.4: stealth/ready/aggressive): hold fire until ordered/compromised (9.5 stealth discipline) — the *only* stealth behavior this spec touches |
| Focus target | Enemy | `FocusTarget` | Stage A as-is; class-appropriate execution already data |
| Suppress | Area | — | New verb `Suppress`: area point + radius (data); accept requires line to area (reuses `NoLineOfSight` refusal). "Heavies excel" is Phase 3 (no Heavy in slice); modulation parameter reserved in data |
| Flank | Enemy group | — | New verb `Flank`, two-step: squadmate computes route → `Event.Squad.OrderQueued` (route preview as debug lines) → player approves (same input again) or it expires (data timeout). No route → `NoRoute`, spoken |
| Breach | Door/wall | — | New verb `Breach` against authored breach-point actors (graybox door frames in the slice district); synchronized entry at slice scale = stack + simultaneous entry, no set-piece choreography. No point in range → new refusal `NoBreachPoint` |
| Ability use | Class ability | — | New verb `UseAbility`: routes to the SPEC-P2-01 signature verbs (`Class.Verb.Momentum`/`Stabilize`/`Killzone` — Killzone's Command Mode wiring lands here per the P2-01 seam note); emits existing `Event.Squad.ClassAbilityUsed`. Invalid context (e.g. Stabilize with nobody down) → `InvalidTarget`, spoken |
| Sync strike | Up to 4 marked | — | New verb `SyncStrike`: mark targets (≤ `MaxSyncStrikeMarks`, data) via `OrderQueued` facts, then execute simultaneously. Zero marks → new `NoTargetsMarked`; an assigned soldier compromised → new `NotConcealed`. Slice scope: valid vs. Unaware enemies only (alert model beyond that = SPEC-P2-04/Phase 3) |
| Regroup / Fall back | Player / rally point | `Regroup` | Stage A as-is (player position). Stage B: rally-point variant = `Regroup` with the aimed position as target — target plumbing exists; **no new verb** |

**Refusal reasons:** existing `NoRoute`/`NoLineOfSight`/`InvalidTarget`/`Downed`
are reused wherever they fit; exactly three new reasons enter the enum
(`NoBreachPoint`, `NoTargetsMarked`, `NotConcealed`), each with line pools in
`DT_SquadOrderDefs`. Every reason has a bark; silence stays forbidden (9.5).

## Data schema (14.5 step 1)
- `DA_CommandModeTuning` (new PrimaryDataAsset): `DilationFactor = 0.30`,
  `TacticianDilationFactor = 0.0` (pause; headless-only this phase),
  `EnterBlendSeconds` / `ExitBlendSeconds` (0 = hard cut for Stage A),
  `CameraPullbackPercent = 15`, `SoldierSelectMaxRangeCm`,
  `MaxSyncStrikeMarks = 4`, `FlankApprovalTimeoutSeconds` (wall-clock — the
  window runs while the mode is held, same basis as `ResponseTimeoutSeconds`),
  `SuppressRadiusCm`. No hardcoded numbers (14.2).
- `EEclipseSquadOrder` gains `Suppress`, `Flank`, `Breach`, `UseAbility`,
  `SyncStrike`; `EEclipseSquadStance` gains `Stealth`;
  `EEclipseOrderRefusalReason` gains the three reasons above. (Enums are
  in-mission only — no save-schema impact, no migration entry needed.)
- `DT_SquadOrderDefs` gains rows for the five new order ids (ack/refusal
  pools). Missing row degrades to the existing stock-line fallback + log
  warning (14.3.5), never silence, never a crash.

## Pure-logic core (14.5 step 2)
- `EclipseSquadOrderLogic::DecideOrder` extends over widened
  `FEclipseOrderWorldFacts` (`bHasLineToArea`, `bHasBreachPointInRange`,
  `MarkedTargetCount`, `bAllAssignedConcealed`, …). Contract unchanged: every
  order × facts combination maps to exactly one accept-or-refuse-with-reason;
  controllers gather facts, the table decides — headless, exhaustive in tests.
- New pure pieces, same namespace: sync-strike mark-set logic (add/remove, cap,
  dead-mark pruning, execute-validity) and the flank approval state machine
  (Proposed → Approved / Expired / Cancelled) as plain functions over plain
  structs. Route *computation* is a world fact (navmesh, controller-side); the
  pure core only decides with its result.
- Dilation appears nowhere in this layer (locked decision 5).

## Events (14.3.4 — before code; catalog rows land with the implementation commit)
New payload struct `FEclipseCommandEventPayload` (Core/EclipseEventPayloads.h):
`DilationFactor (float)`, `HeldSeconds (float)`, `OrdersIssuedWhileHeld (int32)`.

| Tag | Payload | Emitted by | Consumed by |
|---|---|---|---|
| `Event.Command.ModeEntered` | Command payload (DilationFactor) | Command Mode component (hold began) | audio (SPEC-P2-09 duck/filter layer), debug UI now / SPEC-P2-07 UI later, M1.1 teaching beat (SPEC-P2-04), scenario tests |
| `Event.Command.ModeExited` | Command payload (HeldSeconds, OrdersIssuedWhileHeld — feel-gauntlet telemetry) | Command Mode component (release/fail-safe) | same as ModeEntered |
| `Event.Squad.OrderQueued` | `FEclipseSquadEventPayload` (soldier id, order, target id) — struct unchanged | SquadSubsystem (flank proposal made; sync-strike mark added) | command debug UI (route preview, mark pips), scenario tests |

Consumed (existing): `Event.Squad.OrderAcknowledged` / `OrderRefused` (feedback
surfacing in the command HUD), `Event.Squad.ClassAbilityUsed` (UseAbility
confirmation). Orders themselves keep flowing through the existing
`Event.Squad.OrderIssued` → Acknowledged/Refused chain — no parallel order path.
All tags are facts, not commands; input calls subsystem APIs (catalog rules).

**Negative transitions surface too:** flank `Expired`/`Cancelled` and sync-mark
removal (dead-mark pruning) emit `Event.Squad.OrderQueued` with the transition
in the payload's order field, so the debug UI clears previews/pips from the same
stream it builds them from, and expiry gets a short spoken line ("window's
gone") — player inaction is not a refusal, but it is never silent (9.5).

## Integration
- New `UEclipseCommandModeComponent` on `AEclipsePlayerController`: hold state,
  dilation apply + fail-safe restore, soldier selection, reuse of the existing
  `GetAimPoint` dual-trace for targets, dispatch via per-soldier `IssueOrder`
  (selection) or `IssueOrderToAll` (no selection). The P1 stance-modifier and
  order hotkeys keep working outside the mode on KB (on pad, `LB` *is* the
  stance-modifier and becomes the hold key — stance moves inside the held mode,
  see bindings below) — Command Mode wraps the Phase 1 path, it does not
  replace it (no regression of the "feels obeyed" baseline).
- Debug bindings are provisional (documented on the debug HUD): KB hold `Q`,
  pad hold `LB` — the pad stance-modifier conflict on `LB` moves stance
  selection inside the held mode. Final bindings live in the Enhanced Input
  foot/command context stack — a shared seam with SPEC-P2-07, not this spec.
- Player keeps full body control while dilated (4.1.2: powerful, never paused);
  look input is inherently real-time. Squad BT/perception tick per frame as
  before — dilation changes game-time flow, not per-frame AI cost, so 12.4
  budgets (≤40 agents, game-thread ms) are untouched; selection traces run on
  input events only, zero added per-frame work outside the mode.
- Breach points: authored `AEclipseBreachPoint` markers placed on existing
  graybox doorways; a mission without them simply refuses breach orders with
  the spoken reason (14.3.5 — content absence degrades, never crashes).
- Debug UI (14.5 step 4): existing order-state HUD lines + selected-soldier
  marker + queued-order lines + a dilation indicator. Nothing beyond
  debug-grade; SPEC-P2-07 owns the real command interface.
- Console: `Eclipse.Command.Dump` (mode state, dilation, selection, queued
  orders) for Gauntlet loops and bug reports.

## Tests (14.4, blocking)
- **Unit:** extended `DecideOrder` matrix — every order × every fact
  combination answered, all three new refusal reasons reachable, no input maps
  to silence; sync-strike mark-set (cap at data value, dead marks pruned,
  execute validity); flank state machine (propose → approve / timeout / cancel);
  stance `Stealth` hold-fire decision logic.
- **Scenario suite (per merge, squad bar):**
  - *"Order under fire at 0.30 dilation"* — move-to-cover while shot at, in
    Command Mode: acknowledgment or refusal within 1 s **wall clock** (locked
    decision 2 asserted mechanically).
  - *"Refuse path stays verbal"* — blocked flank route → `OrderRefused`
    (`NoRoute`) with bark, never timeout-silence; breach with no point →
    `NoBreachPoint` spoken; sync strike with a compromised soldier →
    `NotConcealed` spoken.
  - Existing P1-06/P2-01 scenarios re-run with Command Mode held — dilation
    must not break any timer or acceptance radius. Any silent order failure =
    release blocker (unchanged bar).
- **Functional (Gauntlet):** 20× rapid enter/exit → dilation exactly `1.0`
  after each; mode held through pawn death and mission end → fail-safe restores;
  `ModeEntered`/`ModeExited` always emitted as a pair.
- **Feel gauntlet (owner playtest item — the R3 falsification):** Stage A build,
  existing district, scripted encounter + free play; owner scores the locked
  decision 3 criteria; verdict + `OrdersIssuedWhileHeld` telemetry recorded in
  EXECUTION_PLAN §2. This is a queued owner touchpoint (§4 discipline), not a
  build blocker for Stage A itself.
- **Perf:** per-merge budget check on the reference scene unchanged in and out
  of the mode (12.4).

## Definition of Done
Stage A playable in the existing district (hold-to-enter, 0.30 dilation,
per-soldier selection, 4 verbs, debug HUD) · R3 feel verdict recorded in
EXECUTION_PLAN with telemetry · Stage B: all eight 8.4 rows issue, acknowledge
or verbally refuse per-soldier with barks in PIE · events in catalog
(implementation commit) · unit + scenario + Gauntlet green, zero silent
failures, zero stuck dilation · dashboard updated.

## Non-goals
No camera cinematics (the 15% pullback float is the entire camera change —
blends/DoF/framing are later polish) · no UI polish (debug-grade until
SPEC-P2-07; radial menus/command wheel live there) · no fireteams (per-soldier
only, Phase 3) · no order verbs beyond the 8.4 table · no Tactician-pause
shipping surface (data + headless tests only) · no Heavy-class suppress
excellence (no Heavy in slice; parameter reserved in data) · no stealth system
beyond the `Stealth` stance flag + concealment fact (8.5 detection model =
SPEC-P2-04/Phase 3; hand-sign visuals = fiction later) · no order queueing
beyond flank-approve + sync-strike marks · no VO (text/TTS barks per 16.4) ·
no strategic-layer or battle orders (8.6, Phase 3).
