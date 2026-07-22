# SPEC-P2-00 — Vertical Slice "Thirteen Bullets": Overview & Gate
*Phase 2 feature spec set | GDD refs: 13.2 (Phase 2), 14.3.4, 14.5, 15.0/15.5, 2.9 (Act 1), 3.3 (Kessara)*
*Skill owner: Game Architecture Expert*

---

## Purpose

Phase 1 built and live-wired the loop on graybox (its own 13.2 gate — the voluntary-second-loop playtest — remains the owner's open call; 2026-07-19 was the Phase 0→1 gate). Phase 2 proves the *game*: Act 1's opening ~3 hours at target quality (13.2 — "quality benchmark + (if sought) funding/partner material"). Concretely, per 13.2:

1. **Kessara Underworks polished slice** — one district at the 15.5 stylized fidelity bar (amber smog, layer-cake verticality, occupation dressing per 3.3).
2. **Hollow Point walkable base** — 4 facilities in the Act 1 geothermal vault (5.2), replacing the Phase 1 menu base.
3. **Squad of 4 with 3 classes** — up from 2 classless recruits (4.2.3).
4. **Command Mode at final feel** — 30% time dilation, the 8.4 order set, orders as promises.
5. **Authored missions M1.1–M1.4** (2.9: *Thirteen Bullets*, *The Dead Drop*, *Signal Fire*, *The Quartermaster*) + **one liberation-template instance** (11.2).
6. **Save system v1** and **UI stack v1** (12.3, 12.1 CommonUI).

**Gate question (13.2): does the slice review as "I want the rest of this game"?** External reviewers (not the team) play the 3 hours cold. If the answer is "nice systems, but…" — the fidelity/feel gap gets fixed before any Phase 3 work starts.

> **Standing note:** ACTIVE_MILESTONE (13_roadmap.md) governs. This spec set becomes buildable scope only when the owner flips that block to Phase 2; until then it is Phase 0-style pre-production for the next milestone. Per 15.0, Phase 2 is where fidelity work is *authorized* — the Graybox Rule expires here for the slice district only.

## Scope

| In (Phase 2) | Out (later phases) |
|---|---|
| One planet: Kessara; one polished district + Hollow Point vault | Tarsis and all other planets (Phase 3+); second districts |
| Squad of 4; 3 classes; existing roster/permadeath/bonds carried forward | All 9 classes (Phase 3); fireteams of 8+; officer/company layer (Part 7) |
| Command Mode final feel (8.4) | Hybrid large battles (8.6, Phase 3); battle command |
| Hollow Point walkable, 4 facilities, construction on strategic clock | Base tiers 2–3, sieges of the base, forward bases, relocation |
| Authored M1.1–M1.4 on the shared quest runtime | M1.5–M1.8 (Phase 3); companions beyond Mara + Brick; Mission Generator v1 (11.3) |
| One liberation-template instance (district-scale region flips) | Full planet-liberation campaigns; Dominion Response Tiers 1+ (scripted pressure only) |
| Save v1 (versioned plugin + v0 migration) | Long-campaign soak coverage beyond slice fixtures |
| UI stack v1 (CommonUI: HUD, command, base, map, debrief) | Full strategy-map UI for multi-planet play; accessibility completion (Phase 5) |
| Fidelity per 15.5 revision (stylized, Nanite-dense, tuned software Lumen) | HW-Lumen/RTX validation (needs target-class card, 15.2A); photoreal anything |
| Audio Phase 2 infra: 16.7 adaptive music (vertical layers + never-silent contract), combat audio basics | Recorded VO (Phase 5 — TTS placeholder per 16.4); full AI Audio Director (16.11) |

## Spec set & build order

Ordered by dependency; every spec follows 14.5 (data schema → pure-logic core + unit tests → subsystem wrapper + events → debug UI → real UI/content *last* — in Phase 2 "real UI/content" finally arrives, but only after the headless layer is true). Per 14.3.4 each spec lists its events consumed/emitted before code is written; new tags land in `Eclipse/Docs/EventCatalog.md` in the same commit (14.2).

| # | Spec | Depends on |
|---|---|---|
| 01 | Squad of 4 & Classes | P1-06, P1-07 |
| 02 | Command Mode (final feel) | 01 |
| 03 | Hollow Point Walkable Base | P1-08, P1-03 |
| 04 | Authored Missions M1.1–M1.4 | 01, 02, 03, P1-05 |
| 05 | Liberation Template Instance | 04, P1-04 |
| 06 | Save System v1 | 01, 03, 05 |
| 07 | UI Stack v1 | 02, 03, 04 |
| 08 | Fidelity District (Kessara Underworks) | 04 (spaces locked) |
| 09 | Audio Infrastructure (16.7) | 02, 04 |

08 and 09 can run in parallel with 05–07 once mission spaces are locked; 06 lands before content freeze so every new struct ships serializable.

### SPEC-P2-01 — Squad of 4 & Classes
Grow the squad to 4 and introduce the first 3 classes with distinct kit, orders, and barks so a squad reads as *people with jobs*, not clones. Implements 4.2.3 (classes), 4.2.2 (traits/bonds at 4), 8.3 (shared cover scoring at squad scale). Squadmates arrive pre-classed as authored Act 1 recruits (Academy facility deferred — see Open questions).
**Events** — consumed: `Event.Prep.MissionLaunchRequested`, `Event.Squad.OrderIssued`. Emitted: `Event.Squad.OrderAcknowledged` / `OrderRefused` / `SoldierDowned` (move to implemented at 4-strong scale); new `Event.Squad.SoldierRevived` (Medic stabilize, 4.2.3), `Event.Squad.ClassAbilityUsed`.

### SPEC-P2-02 — Command Mode (final feel)
Take Command Mode from debug order-picker to shipping feel: hold-to-enter, 30% time dilation (full pause on Tactician), camera lift, the 8.4 order table (Move/Hold, Focus, Suppress, Flank, Breach, Ability, Sync strike, Regroup), and verbal refusal messaging — "orders are promises the AI keeps" (8.4/9.5). Implements 8.4, 4.1.2.
**Events** — consumed: `Event.Squad.OrderAcknowledged` / `OrderRefused` (feedback surfacing). Emitted: new `Event.Command.ModeEntered` / `ModeExited` (audio/UI/tutorial consumers); orders themselves flow through the existing `Event.Squad.OrderIssued`.

### SPEC-P2-03 — Hollow Point Walkable Base
Replace the menu base with the walkable Act 1 vault: 4 facility slots in authored shells, construction/upgrades on the strategic clock, visible-growth mandate per level (5.4). Proposed facilities: Command Center, Barracks, Workshop, Medbay (5.3.1; see Open questions). Implements 5.1–5.4 (Act 1 subset), 12.3 (building system: slot-graph + level instances).
**Events** — consumed: `Event.Economy.ResourcesChanged`, `Event.Campaign.DayAdvanced` (timers), `Event.Roster.*` (staffing/bunks/memorial presence). Emitted via CampaignState commit only: new `Event.Base.ConstructionStarted`, `Event.Base.FacilityBuilt`, `Event.Base.FacilityUpgraded`, `Event.Base.StaffAssigned`.

### SPEC-P2-04 — Authored Missions M1.1–M1.4
Author the four opening missions on the shared quest runtime (12.3: DataAsset objective graphs, StateTree phases) with the full 11.1 loop — Briefing → Preparation → Execution → Extraction → Debrief — and stealth-viable routes per 11.4. M1.1 teaches scarcity combat, M1.2 stealth+intel, M1.3 the first district world-state change, M1.4 the armory heist that recruits Brick (2.9).
**Events** — consumed: `Event.Prep.MissionLaunchRequested`. Emitted: existing `Event.Mission.Started` / `ObjectiveCompleted` / `Completed` / `Failed`; new `Event.Mission.PhaseChanged` (StateTree phase facts), `Event.Story.BeatReached` (narrative flags, committed via CampaignState).

### SPEC-P2-05 — Liberation Template Instance
Instantiate one campaign-mission template (11.2) on Kessara's district graph so an authored mission (M1.3's jammer sabotage) visibly flips region state and the strategic layer answers — the systemic half of "authored + systemic" proven at slice scale. Implements 11.2 (template instantiation on authored mission sites), 11.3 inputs *without* the generator.
**Events** — consumed: `Event.Mission.Completed`. Emitted via commit: existing `Event.Strategy.RegionControlChanged`; new `Event.Strategy.LiberationPhaseAdvanced`.

### SPEC-P2-06 — Save System v1
Promote save v0 to the versioned plugin architecture (12.3): per-subsystem serialization contracts, schema version + migration entry for every break (14.3.6), autosave at strategic transitions, mission checkpoints. Slice content (base state, class data, story flags, liberation phases) all round-trips. Implements 12.3 (save system), 14.3.6.
**Events** — consumed: `Event.Mission.Completed`, `Event.Campaign.DayAdvanced` (autosave triggers). Emitted: new `Event.Save.Completed` (UI toast); otherwise infrastructure, like P1-01.

### SPEC-P2-07 — UI Stack v1
Replace debug UI with the CommonUI stack (12.1): diegetic-leaning HUD (8.8 — no HUD soup), command-mode interface, base/facility screens, mission prep and debrief, map screen; controller + M/KB parity, Enhanced Input context stacks (foot/command/map). Implements 12.1 (CommonUI, Enhanced Input), 8.8.
**Events** — consumed (pure consumer): `Event.Economy.*`, `Event.Mission.*`, `Event.Squad.*`, `Event.Base.*`, `Event.Roster.*`, `Event.Command.*`, `Event.Save.Completed`. Emitted: none — UI input calls subsystem APIs; the bus reports facts, it does not carry commands (EventCatalog rules).

### SPEC-P2-08 — Fidelity District: Kessara Underworks
Dress the slice district to the locked 15.5 direction — stylized Borderlands-leaning with the 2026-07-22 fidelity revision: Nanite-dense kits, tuned-up software Lumen, cel palette + ink outlines, richer post/atmospherics — run through the 15.8 loop until it clears 15.12 ("AAA-ready") on the strong dev PC (15.2C), inside 12.4 budgets. Implements 15.5/15.6/15.8/15.12, 3.3 (Kessara identity: amber smog, sodium vs. white-gold light, occupation dressing).
**Events** — consumed: `Event.Strategy.RegionControlChanged` (occupation-state Data Layers, 12.1). Emitted: none (art/content spec; no gameplay logic).

### SPEC-P2-09 — Audio Infrastructure
Stand up the 16.7 adaptive music system — vertical layers (atmosphere floor → theme motif → combat layers), Quartz-quantized transitions, the never-silent contract — plus combat audio basics (16.9) and squad barks as primary game-state audio (8.8), TTS placeholder voices per 16.4. Implements 16.7, 16.9 (subset), 16.14.
**Events** — consumed (pure consumer): `Event.Command.ModeEntered`/`ModeExited`, `Event.Mission.*`, `Event.Squad.*`, alert-stage facts from combat. Emitted: none (debug state dump via console command instead).

## Shared conventions (unchanged from Phase 1)

- All tunables in DataAssets/DataTables (14.2); a hardcoded gameplay constant is a defect.
- New event tags documented in `Eclipse/Docs/EventCatalog.md` in the same commit; facts not commands; state-changing facts emitted only by the CampaignState commit (14.3.3).
- Pure-logic cores compile without engine actor headers (14.3.2).
- No system hard-depends on content existing (14.3.5) — a missing facility mesh or music stem logs a warning and defaults gracefully.

## Tests (14.4)

| Layer | Phase 2 requirement |
|---|---|
| Unit | Class kit/order resolution, construction timers + cost math, liberation-template instantiation (deterministic seeds), save v0→v1 migration |
| Functional (Gauntlet) | Per mission M1.1–M1.4 + the liberation instance: spawn → complete-by-script → consequences committed to CampaignState |
| Squad AI quality bar | Scenario suite extended to squad of 4 + classes: cover under fire, order refusal messaging, stealth discipline, medic triage — per merge; any silent order failure = release blocker |
| Save integrity | Full slice fixture round-trip per merge; migration test for every schema version including v0 campaigns |
| Performance | Per-merge budget checks on the Underworks district and Hollow Point interior vs. 12.4, profiled on the strong dev PC at dev-preview scalability (15.2C) |
| Campaign soak | Nightly scripted slice playthrough (M1.1→M1.4 + liberation flip) asserting economy/state invariants |

## Global out-of-scope (all Phase 2 specs — explicit non-goals)

**No second planet** (Kessara only; Tarsis is Phase 3). **No fleet/space layer** (Part 7 space, Hangar/Spaceport, vehicles). **No multiplayer** — ever a non-goal (single-player, 12.1: Iris disabled). Also out, per 13.2 boundaries: Mission Generator (11.3), hybrid battles (8.6), Dominion strategic AI beyond scripted Act 1 pressure, research/tech tree (Part 10), classes beyond the chosen 3, base tiers beyond the 4-facility vault, companions beyond Mara + Brick, recorded VO, M1.5–M1.8. Anything not listed in a spec's "In scope" is out.

## Definition of Done (Phase 2 as a whole)

A cold external player can, without dev intervention: start a new campaign, play the ~3-hour slice — M1.1 through M1.4 with a squad of 4 across 3 classes, Command Mode throughout, returning to a walkable Hollow Point that visibly grows, seeing one district liberated on the map — at 15.12 visual quality with always-on adaptive music, on the shipping UI, with save/load working at any point (including a v0 save migrating cleanly). CI green across all six 14.4 layers. Then the gate review: **"I want the rest of this game" — yes or no.**

## Open design questions (to resolve in the numbered specs)

1. **Which 3 classes?** Proposal: Assault, Medic, Scout (matches M1.1 combat / M1.4 heist / M1.2 stealth beats); 13.2 doesn't lock the pick. Combat Designer + Narrative to confirm in SPEC-P2-01.
2. **Class assignment without the Training Academy** — 4.2.3 assigns classes at the Academy, which is not among the 4 proposed facilities. Proposal: Act 1 hand recruits arrive pre-classed; Academy arrives Phase 3. Alternative: swap Medbay for Academy in SPEC-P2-03.
3. **Facility pick** — Command Center/Barracks/Workshop/Medbay vs. a variant including the Intelligence Center (11.1 makes intel-driven Preparation first-class, and M1.2 teaches intel currency). Needs an economy-loop check in SPEC-P2-03.
4. **Liberation instance scale** — how many district region flips make "one liberation-template instance" honest without building the Phase 3 campaign layer (proposal: Foothold phase only, 3 regions).
5. **Prologue (2 h, 2.9)** — is the playable prologue inside the 3-hour slice or does the slice open at M1.1 with a briefing recap? Affects SPEC-P2-04 scope materially.
