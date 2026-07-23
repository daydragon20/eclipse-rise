# SPEC-P2-03 — Hollow Point Walkable Base
*Phase 2 feature spec | GDD refs: 5.1–5.4 (Hollow Point, slots, clock, visible growth), 6.5 (Act 1 band), 11.1 (prep effects), 12.3 (building system: slot-graph + level instances), 14.3.4/14.5, SPEC-P1-03/08 (ledger, prep flow) | Skill owner: Game Architecture Expert*

## Purpose
Replace the Phase 1 menu base with the walkable Act 1 geothermal vault: 4 facility
slots in an authored graybox shell, construction and upgrades on the strategic
clock, visible growth per build step (5.4 — *the base is the progress bar of the
whole game*). Slot-based inside authored shells, never freeform (5.1). The Phase 1
loop (select → prep → launch → debrief) must never regress: facilities *add*
capability, they never gate the core loop.

## Locked decisions (this spec)
1. **The 4 slice facilities: Command Center, Barracks, Workshop, Intelligence
   Center** — the Medbay loses the fourth slot (SPEC-P2-00 open question 3
   resolved). Rationale: the Intelligence Center is the only candidate that feeds
   the 11.1 prep loop the slice must prove — it turns M1.2's intel lesson into
   standing income (pre-Foothold the wallet affords exactly one 5 I reveal per
   prep with zero buffer; see econ check) — while Medbay L1's recovery bonus (5 → 4 days out on
   a roster of 8 with 4 deployed) almost never changes a muster decision in a
   4-mission slice, and the in-mission medical fantasy is already carried by the
   Medic's Stabilize (SPEC-P2-01). GDD 5.3.2 concurs: the IC is "the
   anti-frustration engine". Medbay arrives with tier growth in Phase 3.
2. **Command Center is pre-built at L1, free** (5.3.1 mandatory core): the stolen
   map table of 5.2. Slots B–D start unbuilt (Slot B's empty state is the 5.2
   bunk camp — see slot-graph). Build *order* is free — a focused order is a
   strategic identity (5.4); costs pace it, story never forces it. Placement is
   authored: each slot's `AllowedFacilityRows` pins which facility can occupy it.
3. **Act-1 cost table, GDD ratios preserved.** 5.3.1 values (150/200/300 M) are
   full-campaign representative numbers tuned against Act 2+ income; the slice
   table uses ×0.8 values (120/160/240 M) so the Act 1 band (6.5: 20–60 M/day)
   carries three builds plus one upgrade on a thrifty run. Ratios Barracks :
   Workshop : IC = 1 : 1.33 : 2 are kept. All values live in `DT_Facilities`
   (14.2); the 5.3.1 numbers return with Phase 3 rebalancing.
4. **Mission resolution advances the clock by +1 day**, committed with the
   debrief transaction — construction progresses while you fight. Manual
   advance-day (map table) remains for waiting out builds; waiting is never free
   (wages tick, 6.4.1). *(Cross-spec note: touches the P1-05 debrief commit;
   SPEC-P2-04 inherits this rule.)*
5. **No Energy resource in Phase 2.** The 5.3.1 upkeep column would add a meter
   with zero decisions at 4 facilities; the coughing generator is ambient story.
   `// PLACEHOLDER(GDD 6.2 Energy)` at the upkeep seam; Energy lands in Phase 3.
6. **Staffing v1 = two roles only:** a construction **crew** (1 soldier, −1 build
   day, soldier undeployable while assigned — the 5.3.2 staff dilemma in
   miniature) and an **IC analyst** (1 soldier, +1 Intel/day). Everything else is
   Phase 3.

## Slot-graph (5.2 Act 1 vault — authored shell, graybox)
```
        [Vault door / entry airlock]
                   |
            [THE SPINE corridor]
      /        |         |        \
 [Slot A]  [Slot B]  [Slot C]  [Slot D]
 Command   Barracks  Workshop  Intelligence
 Center    (bunks)   (+gen.room) Center
 (pre-built)  |
        [Memorial alcove]  — non-slot, auto-grows with losses (5.3.1: free)
```
- `Slots A–D` adjacency: each slot connects to the Spine; Slot C additionally to
  the generator room (non-slot ambient), Slot B to the memorial alcove.
- Both Spine ends terminate in **sealed excavation faces** (rubble + blast doors):
  the visible tease of slots 5–8 (5.2's 4 → 8 unlock), zero functionality in
  Phase 2.
- **Slot B's empty state is the 5.2 bunk camp** — 11 bunks + the muster board,
  working from day 1 (roster cap 11), so squad pick never waits on construction
  (Purpose rule: facilities add capability, never gate the loop). Barracks L1
  formalizes it: cap 12, proper muster venue.
- Interaction points (debug-grade UI until SPEC-P2-07): map table → strategy map
  + advance day (P1-04/08), muster board (Slot B, day 1 onward) → squad pick,
  workbench (Workshop) → production queue (P1-03), IC console → intel report,
  survey post at each empty slot → build menu.

### Visible growth per build step (5.4 mandate — must read in graybox)
| State | Per-slot read | Global read (per completed facility count) |
|---|---|---|
| Empty | raw rock, tarp, dead cable run | 1 built: hanging work-lights in Spine |
| Under construction | scaffold blockout, sparks, drill loop; crew idler if staffed | 2 built: wall sconces, cable trays filled |
| L1 built | facility blockout set + own light temperature + audio bed + staff idler | 3 built: full strip lighting, steam pipes live, generator sound smooths |
| L2 (Workshop only) | second workbench, powered rack, brighter task light | — |

*(Slot B's "empty" reads as the bunk camp — bedrolls, crate table, muster board —
not raw rock.)*
Growth is carried by **state swaps (level instances/Data Layers per 12.3) +
lighting + audio + idlers** — deliberately not by art fidelity, so the mandate
survives the Graybox Rule (15.0) until the dressing pass.

## Strategic-clock timers & costs (initial `DT_Facilities` values)
| Facility | Cost | Build days (crew −1, min 1) | Yield/day | 11.1 prep effect |
|---|---|---|---|---|
| Command Center L1 | free (pre-built) | — | — | enables the loop: map, mission select, advance day |
| Barracks L1 | 120 M | 2 | — | formalizes the day-1 bunk camp: roster cap 11 → 12, proper muster venue |
| Workshop L1 | 160 M | 3 | — | unlocks the P1-03 production queue → loadout options (pre-build: scavenged gear only — M1.1's scarcity is real) |
| Intelligence Center L1 | 240 M | 4 | +2 Intel (+1 with analyst) | funds the 5 I prep reveal (11.1 fairness loop) ~every 2 days |
| Workshop L2 (only slice upgrade) | 200 M | 3 | — | manufacture tier: +2 loadout rows in `DT_LoadoutOptions` |
| *(rejected variant)* Medbay L1 | 160 M | 3 | — | wounded days-out 5 → 4; ~2–4 soldier-days saved across the whole slice — negligible at roster 8 / deploy 4 |

*(Workshop L2's 200 M is a slice-native price set inside the Act-1 band — not
×0.8 of the 5.3.1 L2 value, which returns in Phase 3.)*

**Clock rules:** construction advances only on `Event.Campaign.DayAdvanced`;
mission resolution commits +1 day (locked decision 4); **rush** = 60 C ×
remaining days (`DA_BaseTuning`) — the rush commit sets `DaysRemaining` to 0 and
emits `Event.Base.FacilityBuilt` in that same commit (instant completion) — with
the Act 1 credit curve payroll-clamped (see below) rush is *available, never
comfortable* (5.4: money vs. time).

## Econ check (run on the P1-03 ledger as built)
Inputs (as-shipped data: `create_phase1_content.py`, `DA_CampaignSetup`,
`DA_PrepTuning`): start 150 C / 80 M / 10 I; roster 8 × 15 C/day wages (clamped
at 0 → `Wages_Short`); Underworks yield 8 M + 20 C/day; WorkerHousing contested
40 C × 0.4; intel reveal 5 I; decay 5%/week. Slice calendar model (**proposal**
— SPEC-P2-04 owns the final numbers): ~16 days, missions ≈ day 2 / 5 / 9 / 13;
the M1.3 liberation
instance flips the Foothold trio (TransitCheckpoint, WorkerHousing, SupplyDepot)
at day 9 → income becomes 28 M / 110 C / 2 I per day. Authored reward bands
(input to SPEC-P2-04): M1.1 +25 M/50 C · M1.2 +15 M/30 C/8 I · M1.3 +40 M/60 C/
4 I **+60 M capture-intact windfall** (SupplyDepot, 6.3.3) · M1.4 armory heist
+200 M/100 C · optional objectives ≤ +80 M total (11.4 stretch layer). Reward
bands are likewise a **proposal** for SPEC-P2-04; only the affordability
invariants below are this spec's contract.

**Materials throughput:** 80 start + 72 (d1–9 @8) + 196 (d10–16 @28) + 280
missions + 60 windfall = **688 M** baseline inflow.

| Scripted path | Spends | Construction afforded | Margin |
|---|---|---|---|
| **Builder** (rifle+armor 100 M) | gear after IC start (≈ d14) | Barracks d2 → Workshop d9 → IC starts d13 (heist), completes ≈ d16 | +68 M |
| **Intel opening** (no production) | IC first | IC affordable d9 (292 M cum.), **built d12 → feeds M1.4 prep**; Barracks+Workshop post-heist | scavenged-gear combat is the price of early intel |
| **Thrifty** (rifle only, all optionals) | upgrade hunt | 3 × L1 + **Workshop L2** (200 M) ordered before day 16 (completes ≈ d18) | +8 M |

**Credits (honesty row):** inflow ≈ 1484 C vs. 1920 C wages over 16 days →
credits sit near zero mid-slice, `Wages_Short` fires — the intended 6.5 Act 1
feeling ("we are broke and desperate"); morale consequences of short pay are
explicitly Phase 3. **Intel:** post-Foothold the territory itself pays +2 I/day
(TransitCheckpoint), so the slice *total* is healthy — ~34 I ≈ 6–7 reveals (10
start + 12 mission + 14 territory − ~2 decay). The scarcity is **pre-flip**:
d1–9 the wallet runs 10 → 5 (M1.1 prep) → 0 (M1.2), and M1.3 preps on mission
intel alone (8 → 3) — exactly one reveal per prep, zero buffer, nothing spare
for the liberation layer. The IC buys the *standing* stream: margin in the
tight half and continuity into Phase 3 — which, with the Medbay negligibility
above and GDD 5.3.2, is the facility decision (locked decision 1).
These three paths are the **soak invariants**, asserted nightly; retuning happens
in `DT_Facilities`/reward tables without spec revision (6.5 balancing levers).

## Data schema (14.5 step 1)
- `UEclipseBaseLayoutAsset` (slot-graph per base level asset, 12.3):
  `Slots: TArray<FEclipseBaseSlotDef>` — `SlotId (FName)`, `DisplayName`,
  `AllowedFacilityRows (TArray<FName>)`, `AdjacentSlotIds`, per-state streaming
  ids (level instance / Data Layer name per construction state).
- `DT_Facilities` — `FEclipseFacilityRow`: `FacilityId`, `DisplayName`, per-level
  array of `{CostMaterials, CostCredits, BuildDays, YieldPerDay (tag→int32),
  UnlockTag}`.
- `DA_BaseTuning`: `RushCostCreditsPerDay = 60`, `CrewDayReduction = 1`,
  `AnalystIntelBonusPerDay = 1`, `MaxCrewPerSite = 1`.
- `FCampaignState` gains `FEclipseBaseState { TArray<FEclipseFacilityState> }` —
  `SlotId`, `FacilityId`, `Level`, `DaysRemaining (0 = operational)`,
  `AssignedSoldierIds` (assignment lives in base state; the roster record is
  untouched — muster validation reads base state). Save-migration entry + v0
  fixture test (14.3.6).

## Pure-logic core (14.5 step 2)
- `EclipseBaseLogic`: build-order validation (slot empty, facility allowed,
  ledger funds — reuses the P1-03 insufficient-funds rejection) and the
  construction day-tick (decrement, crew reduction, rush, completion mutations).
  Headless, deterministic, no engine actor headers (14.3.2).
- Facility yields (IC intel) enter the existing economy day-tick as
  `FEclipseFacilityYieldParams`, parallel to region yields — one deterministic
  tick keeps the replayability contract (12.3).

## Events (14.3.4 — before code; catalog rows land with the implementation commit)
New payload struct `FEclipseBaseEventPayload` (Core/EclipseEventPayloads.h):
`SlotId (FName)`, `FacilityId (FName)`, `Level (int32)`, `EtaDay (int32)`,
`SoldierId` (same id type as `FEclipseRosterEventPayload`), `RoleTag`.

| Tag | Payload fields used | Emitted by | Consumed by |
|---|---|---|---|
| `Event.Base.ConstructionStarted` | SlotId, FacilityId, Level (target), EtaDay | CampaignState commit (build order accepted) | vault presentation (scaffold state on), base/debug UI, audio |
| `Event.Base.FacilityBuilt` | SlotId, FacilityId, Level = 1 | CampaignState commit (construction tick completion) | vault presentation (state swap + global growth tier), UI, audio, soak asserts |
| `Event.Base.FacilityUpgraded` | SlotId, FacilityId, Level (new) | CampaignState commit | same as FacilityBuilt |
| `Event.Base.StaffAssigned` | SlotId, FacilityId, SoldierId, RoleTag (crew/analyst/none = unassign) | CampaignState commit | muster/roster UI (soldier unavailable), vault presentation (crew idlers) |

Consumed: `Event.Campaign.DayAdvanced` (construction/yield tick),
`Event.Economy.ResourcesChanged` (build-menu affordability refresh),
`Event.Roster.*` (staff availability, memorial alcove growth). All four new tags
are facts emitted **only** via the CampaignState commit (14.3.3); UI issues
build/staff orders through the subsystem API, never the bus.

## Integration
- Walkable vault = authored graybox level; interaction points open the existing
  debug screens (P1-03/04/07/08). `Event.Prep.MissionLaunchRequested` payload is
  **unchanged** — P1-05 mission runtime is untouched.
- The P1-08 menu hub is retired only after the walkable parity Gauntlet passes
  (everything the menu could do, done in the vault). Missing shell mesh/state
  instance → log warning + placeholder blockout, never a crash (14.3.5).
- Debug console: `Eclipse.Base.Report` (slots, states, ETAs, staff),
  `Eclipse.Base.Build <FacilityId>` (validated build order), for Gauntlet loops.

## Tests (14.4, blocking)
- Unit: construction tick (multi-day, crew −1 floor at 1, rush completes in its
  own commit); build validation (occupied slot, unknown facility, insufficient M —
  never crash); IC yield in the econ tick; upgrade path L1→L2; save round-trip
  with `FEclipseBaseState` + v0 migration (base-less save loads, CC pre-built).
- Functional (Gauntlet): walkable parity script — full P1 loop start-to-second-
  loop entirely via vault interaction points; build-to-completion script
  asserting all four `Event.Base.*` fire in commit order.
- Regression (locked decision 4): the existing P1-05/P1-08 flow tests gain a day
  assert at implementation — the debrief commit advances the day by exactly 1 —
  and the debrief screen lists that day-tick's ledger lines (wages, yields) per
  the 7.6 transparency rule.
- Soak (nightly slice script): the three econ paths above — Builder ≥ 3 L1 by
  day 16, Intel-opening IC operational before M1.4, Thrifty has Workshop L2
  ordered (under construction) before day 16; credits never negative;
  `Wages_Short` never blocks a build.
- Perf: Hollow Point interior within 12.4 budgets per merge (graybox-trivial
  now; keeps the harness honest for the dressing pass).

## Definition of Done
Spec'd events in catalog (implementation commit) · layout asset + data + core +
wrapper landed · menu base retired with Gauntlet parity green · three builds +
one upgrade reachable per soak invariants · every build step visibly changes the
vault (state swap + light + audio + idlers) in PIE · save v0 migrates · tests
green · dashboard updated.

## Non-goals
No base-UI polish (debug-grade until SPEC-P2-07) · **no interior fidelity before
the phase-gate — the Graybox Rule (15.0) holds for the vault; the SPEC-P2-08
fidelity budget belongs to the Underworks district** · no Medbay/Academy/
Research Lab/Power Plant/Storehouse/Hangar (Phase 3+) · no Energy resource or
upkeep (locked decision 5) · no slots 5–16, excavation projects, tiers 2–3 ·
no base damage/siege states · no relocation · no staffing beyond crew + analyst ·
no morale reaction to `Wages_Short` (Phase 3) · no L2/L3 for anything but the
Workshop · Memorial stays the auto-grown alcove (grief is never a purchase,
5.3.2).
