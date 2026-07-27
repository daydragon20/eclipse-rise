# SPEC-P2-07 — UI Stack v1 — **CONCEPT for main-review (2026-07-27)**
*Phase 2 feature spec | GDD refs: 12.1 (CommonUI + Enhanced Input), 8.8 (no HUD soup, diegetic-leaning, squad audio as primary game-state channel), 14.2 (tunables in data, a hardcoded gameplay constant is a defect), 14.3.4 (events before code), 14.3.5 (loud degradation), 14.5 (build order), 12.4 (budgets) | Seams: SPEC-P2-02 (foot/command context stack), SPEC-P2-06 (save/load UI + `Event.Save.Completed` toast), SPEC-P2-05/P2-08 (liberation presentation) | Skill owner: Game Architecture Expert*

## Purpose
Replace the debug UI with the real CommonUI stack: HUD, command-mode interface,
base/facility screens, mission prep and debrief, map screen — with controller and
mouse+keyboard parity, on Enhanced Input context stacks (foot / command / map).

The spec is falsification-first on one claim, and that claim is unusual because
it is about the *verification method* rather than the feature:

> **The UI layer cannot be photographed.** Measured 2026-07-27: `HighResShot`,
> `FScreenshotRequest::RequestScreenshot` with `bShowUI = true`, and a Slate
> window capture all three produce the same 3D image with **no widgets in it**.

That is not a nuisance, it is the governing constraint of this spec. On 26→27
July it twice nearly produced a false defect report ("the ammo counter is
missing", "the guide does not open") — both times the UI was working and only
the *evidence* was absent. Every acceptance criterion below is therefore written
against a **state dump**, not against an image. A DoD line that says "looks right
on a screenshot" is unachievable in this project and must not be written.

## What exists today (measured, 2026-07-27)
- **Three widgets, none of them CommonUI.** `UEclipseMissionHudWidget`,
  `UEclipseBaseHubWidget` and `UEclipseStrategyMapWidget` all derive from plain
  `UUserWidget`. `CommonUI` and `CommonInput` are already linked in
  `Eclipse.Build.cs`, and the `CommonGameViewportClient` misconfiguration was
  fixed on 27-07 (it logged "Input routing will not function correctly" as an
  **error** on every start, and was the likely cause of the owner's View button
  not arriving).
- **One input context, built in C++ at runtime.**
  `AEclipsePlayerController` does `MappingContext = NewObject<UInputMappingContext>(this)`
  and then `MapKey(...)` per action, added once at priority 0. There is **no
  context stack** and **no mapping data asset** — which makes every binding a
  hardcoded gameplay constant, i.e. a 14.2 defect that this spec inherits.
- **The binding table already exists as data**: `EclipseGauntletOverlayLogic::GetBindings()`
  returns 16 `FEclipseBinding` rows over two modes (`OnFoot`, `CommandMode`),
  each with action, key and pad button, guarded by a test that a button never
  carries two meanings *within* one mode.
- **The debug surface that must survive the transition**: F2 controls, F3 test
  guide, H playtest panel, the R3 verdict rows, the ammo readout and the hit
  marker — plus `Eclipse.UI.Report`, which dumps panels, visibility and row
  counts and is currently the only way to verify any of it.
- **`Event.Save.Completed` does not exist yet** (SPEC-P2-06, unreviewed).

## Locked decisions (this spec)

1. **The UI emits nothing on the bus. It is a pure consumer.**
   UI input calls subsystem APIs directly; the bus reports facts and never
   carries commands.
   *Rationale: EventCatalog rule. The moment a widget can publish, "what
   happened" and "what someone asked for" share a channel and the debrief
   becomes unreadable.*

2. **The input context stack is DERIVED from the existing binding table, not
   re-declared next to it.**
   `GetBindings()` already knows which action belongs to which mode. The foot and
   command contexts are built from those rows; adding a binding in one place
   changes the context, the F2 table and the parity test together.
   *Rationale: on 27-07 the same control was described in four places and they
   drifted; a fifth declaration is how that happens again. The one guard that
   would have caught it — action↔key pairing — is exactly the one a second
   source makes impossible.*

3. **Bindings move from code into a data asset in the same step.**
   Today they are `MapKey` calls in C++. 14.2 says a hardcoded gameplay constant
   is a defect, and the context stack cannot be authored without the asset.
   *Rationale: these are the same change; splitting them means doing the risky
   half twice.*

4. **Verification is a state dump, never a screenshot.**
   `Eclipse.UI.Report` grows with the stack: every screen reports whether it is
   mounted, visible, focused, and how many rows it holds — and emits a `UI: FOUT`
   line for states the player experiences as broken (a panel open with zero rows,
   a HUD outside the viewport, a hidden ammo counter while carrying a weapon).
   *Rationale: measured — the UI layer is not capturable. See Purpose.*

5. **Controller and mouse+keyboard parity is asserted, not assumed.**
   Every screen reachable on one device is reachable on the other. The binding
   table already carries both columns; the parity test reads it rather than a
   hand-written list.
   *Rationale: the owner plays on a pad. A screen that is only reachable with a
   mouse is invisible to him, and "it works" would be true and useless.*

6. **The debug overlays stay until their replacement passes the same report.**
   F2/F3/H and the R3 verdict rows are the owner's playtest instrument. They are
   removed per-panel, only once the CommonUI screen that replaces them answers
   the same `Eclipse.UI.Report` questions.
   *Rationale: 14.3.5 and plain sequencing — losing the test guide mid-transition
   costs a play session, and the guide is how the owner reports at all.*

## Data schema (14.5 step 1)
- `UEclipseInputConfigAsset`: the mapping contexts (foot / command / map) plus
  the `UInputAction` handles, replacing the runtime-constructed context.
- `UEclipseUIStyleAsset`: fonts, colours and spacings pulled from the existing
  palette — no per-widget literals (14.2).
- Screen registry: a data-driven list of screen ids → widget classes, so a
  missing widget class degrades loudly instead of a null dereference (14.3.5).

## Events (14.3.4)
- **Emitted:** none (decision 1).
- **Consumed:** `Event.Economy.*`, `Event.Mission.*`, `Event.Squad.*`,
  `Event.Base.*`, `Event.Roster.*`, `Event.Command.*`,
  `Event.Strategy.LiberationResolved` (the one debrief line that says *why*),
  and `Event.Save.Completed` **if** SPEC-P2-06 lands it.
- No catalog rows are added by this spec; consumer columns are updated in the
  implementation commit.

## Build order (14.5)
1. **Input config asset + context stack**, derived from `GetBindings()`, with the
   parity test. This is step 1 because it is the falsification target and because
   decisions 2 and 3 are the same change.
2. **`Eclipse.UI.Report` grows into the screen registry** — the verification
   surface before the thing it verifies, since there is no other way to see it.
3. **HUD** (ammo, hit marker, alert state) on CommonUI, debug HUD retired panel
   by panel per decision 6.
4. **Command-mode interface**, then **prep/debrief**, then **base/facility**,
   then **map** — each one retiring its debug counterpart only after its report
   answers.
5. **Save/load screen** — last, and only if SPEC-P2-06 has landed; otherwise it
   is deferred with the reason stated.

## Tests (14.4, blocking)
- **Parity:** every screen reachable on pad is reachable on M/KB and vice versa,
  read from the binding table.
- **No double meaning within a mode:** the existing guard, extended to the new
  contexts.
- **Context stack pops cleanly:** entering and leaving command mode restores the
  foot context exactly — asserted on the active-context list, not on a symptom.
- **Every screen answers `Eclipse.UI.Report`**: mounted / visible / focused /
  row count, and a `UI: FOUT` line for each of the four broken states. Each
  `FOUT` is falsified against its own real failure mode.
- **Degradation:** a missing widget class in the registry logs and continues
  (14.3.5), asserted.
- **Budget:** UI tick stays inside 12.4 (16.7 ms frame); the debug HUD currently
  costs nothing measurable and the replacement must not regress that.

## Definition of Done
Bindings live in a data asset and the contexts are built from the binding
table · foot/command/map contexts push and pop with an asserted active-context
list · pad and M/KB parity green · every shipped screen answers
`Eclipse.UI.Report` with mounted/visible/focused/rows and its `FOUT` lines
falsified · debug panels retired only where replaced (decision 6) · UI tick
inside budget · BESTURING.md regenerated or re-guarded against the binding
table · dashboard updated.

**Explicitly not in the DoD:** any criterion phrased as "looks correct on a
screenshot". It cannot be met — see Purpose.

## Non-goals
Radial menus / command wheel (named in SPEC-P2-02 as P2-07 material, deferred
again here — the four-order D-pad works and a wheel is a second input grammar) ·
diegetic 3D-worldspace UI beyond the HUD's leaning · localisation · accessibility
options beyond existing invert-Y (own spec) · the map "moment" (stinger, banner)
which is P2-09 material over existing facts · save/load screen if P2-06 has not
landed.

## Open review points (for main / owner)
1. **How far does "diegetic-leaning" (8.8) actually go for the HUD?** The ammo
   counter is screen-space bottom-right today and reads fine. Pushing it onto the
   weapon costs readability at 1920×1080 in a dark district, and 8.8's real
   target is "no HUD soup" rather than "no HUD". This spec assumes: keep ammo and
   hit marker screen-space, move alert state and squad status diegetic.
2. **Do F2/F3/H survive shipping, or only development?** Decision 6 keeps them
   through the transition, but the test guide is also how the owner reports on a
   build. Recommendation: keep them behind the existing CVar permanently — they
   cost nothing and they are the only UI that is verifiable by design.
3. **Does this spec wait for SPEC-P2-06?** Only the save/load screen depends on
   it. Everything else is independent, so the recommendation is: do not block —
   ship steps 1–4 and defer step 5.
