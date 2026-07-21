# PART 10 — TECHNOLOGY TREE
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 10 of 14*

---

## 10.1 Research Philosophy

The tech tree is the rebellion's biography: **Scavenged → Adapted → Manufactured → Advanced → Ascendant.** Five eras, nine categories, ~120 nodes. Rules:

1. **Every tech changes something you can see or hold.** No "+3% invisible" nodes.
2. **Physical prerequisites:** key nodes need artifacts (captured gear), specialists (rescued people), or facilities — research is entangled with missions and story, not a passive timer bar.
3. **Choice pressure:** 1–3 research slots (Lab level); eras gate by campaign act; a campaign completes ~70% of the tree — builds have identity.
4. **The Dominion researches too** (doctrine track, Part 9.4): the tree is an arms *race*, not a shopping list.

**Eras:** E1 Scavenged (Act 1) → E2 Adapted (Act 1–2) → E3 Manufactured (Act 2–3) → E4 Advanced (Act 3–4) → E5 Ascendant (Act 4 capstones).

---

## 10.2 Category Highlights (representative nodes; format: *Node (era): effect — prerequisite*)

**WEAPONS**
- Reloading Workbenches (E1): craft ammo — Workshop L1
- Suppressor Fabrication (E2): silenced tiers — captured Veil kit
- Standard Arms Pattern (E3): mass-produce rifle platforms — Factory L2 (*the moment scavenging ends*)
- Rail Accelerators (E4): railgun family — Vorn shipwright specialist
- Energy Weapons Program (E4→E5): heat-based tier — AEGIS-adjacent cores + Krad-9 reactor delve
- *Capstone:* **Sunlance** (E5): orbital-cannon refit for *Dawnbreak* — finale option with moral weight (city targets…)

**ARMOR** — Plate Recycling (E1) → Composite Weaves (E2, Sylvaris bio-fiber) → Powered Frames (E3: Heavy exo — carries LMG+shield) → Adaptive Plating (E4: per-damage-type auto-harden) → **Aegis-Grade Suits** (E5: squad-wide, finale).

**VEHICLES** — Salvage Rigs (E1) → Technical Conversions (E2: armed trucks) → APC/Tank Lines (E3) → Mech Salvage Program (E4: walker lance — captured walker required) → Hover Doctrine (E5).

**SHIPS** — Lane-Runner Refits (E2: smuggler mobility) → Corvette Yard (E3) → Frigate/Cruiser Yards (E3–4, Vorn boosts) → Battleship Slip (E4) → **Gate Spire Override** (E5: unlocks the Act 4 blockade break — research as story key).

**AI & DRONES** — Repurposed Sentinels (E2, Krad-9) → Drone Corps (E3: Drone Specialist class unlock) → EW Suites (E4: jammers, Eclipse Protocol upgrade) → **AEGIS Interface** (E5: Whisper-gated; enables endgame AEGIS choices — the tree's narrative summit).

**ENERGY** — Geothermal Taps (E1) → Grid Theft (E2: siphon Dominion grids = Energy income via covert ops) → Fusion Cells (E3) → Tidal Arrays (E4, Meridia) → Zero-Point Research (E5 flavor capstone).

**MEDICAL** — Field Trauma Kits (E1) → Stim Program (E2) → Prosthetics Lab (E3: remove critical-injury penalties — *hope as tech*) → Gene-Therapy Vaults (E4, Sylvaris) → Revival Protocol (E5: one-per-mission death→critical conversion; the game's only resurrection-adjacent tech, deliberately late and limited).

**DEFENSE** — Barricade Kits (E1) → Sensor Nets (E2) → Turret Grids (E3) → Shield Emitters (E4: base/ship shields) → Fortress Doctrine (E5: forward-base defense ×2).

**STEALTH** — Signal Dampeners (E1) → Optic Veils (E2: player cloak protocol) → Ghost Insertion (E3: new mission insertion options) → Veil-Breaker Crypto (E4: expose infiltrators — counterintel arc) → **Black Fleet** (E5: fleet-level stealth, enables the Eclipse maneuver itself).

---

## 10.3 Tree Structure & Tuning

- Cross-category prerequisites knit the tree (Powered Frames needs Fusion Cells; Black Fleet needs EW Suites + Cruiser Yards) — ~30% of E4+ nodes have cross-links, forcing broad investment without soft-locking (validated by an automated dependency-graph test, Part 14).
- Costs scale E1: 50–150 RD → E5: 3000–6000 RD + physical prereqs; timers from hours to strategic weeks.
- All nodes are DataAssets (Part 12): designers/AI agents add nodes without code.

---

*Next document: [11_missions.md](11_missions.md).*
