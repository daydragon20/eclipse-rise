# PART 11 — MISSIONS
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 11 of 14*

---

## 11.1 Mission Architecture

Three authorship tiers sharing one runtime (Quest framework, Part 12):

| Tier | Count | Authorship |
|---|---|---|
| **Main missions** | 34 story missions (M1.1–M4.7: 8+9+10+7) + 8 companion loyalty missions | Fully hand-authored set-pieces |
| **Campaign missions** | ~60 templates × parameters | Hand-designed templates, systemically instantiated (liberation campaigns, sieges, faction arcs) |
| **Generated operations** | Unbounded | The Mission Generator (below) fills the strategic layer |

**Universal mission loop:** Briefing (context + stakes) → **Preparation** (squad pick, loadout, insertion choice, Intel spending) → Execution (objectives + 1–3 optional objectives + emergent events) → Extraction (often its own beat) → **Debrief** (rewards, consequences, roster outcomes, story reactions).

**Preparation is a first-class phase:** Intel spending reveals enemy composition/patrols/alternate entries; insertion options (ground/sewer/dropship/Ghost) exist per site; the Scout class generates free partial intel. Preparation converts strategy-layer wealth into tactical-layer fairness — the loop that welds the game's halves together.

---

## 11.2 Mission Type Catalog (template library)

**Offense:** Assault (base/convoy/artillery), Sabotage (fuel, comms, factory — each with distinct world-state effects), Assassination (officer targets degrade Dominion AI), Heist (tech/artifact/treasury), Breach (prison/blacksite/camp liberation), Demolition (bridge/spire/grid).
**Stealth/Intel:** Infiltration (data theft), Surveillance (pattern-of-life before big ops), Dead Drop networks, Mole Hunt (counterintel), Extraction (agent/defector/VIP), Wire-tap (lane relay).
**Defense:** Base Defense (your actual base), Settlement Defense, Convoy Escort, Evacuation (save civilians under attack — score = lives), Holdout (survive until relief; relief timing = your strategic assets), Counter-raid interception.
**Political/Social:** Negotiation summits (dialogue-driven, armed-social spaces), Rescue-the-informant dilemma missions, Broadcast seizure (propaganda flips district mood), Hearts-and-minds (Elystra style zero-casualty ops), Prisoner exchange.
**Military campaign:** Front Assault (hybrid battles), Siege (attacking fortified regions; artillery/supply prep sub-missions), Relief Force, Orbital Drop (fleet-enabled aggressive insertions), Boarding Action, Gate Spire Assault.
**Events (strategic-layer random, world-reactive):** Dominion reprisal raids, refugee columns (aid = food cost, reputation gain), defector walk-ins (trap or treasure — Intel checks), black-market windfalls, lane pirate surges, propaganda counter-ops, memorial requests from soldiers (small, personal, free — the roster's soul in mission form).

**Planet-liberation campaign template (the game's strategic centerpiece):** each planet liberation = authored skeleton of 3 mandatory phase-missions (Foothold → Momentum-phase using region-appropriate templates → Capital Push hybrid battle) + systemic region flips in between; planet-specific authored twists (Part 3 identities) skin every instance. Guarantees: every liberation feels both systemic (player-driven order) and authored (planet personality).

---

## 11.3 The Mission Generator

**Inputs:** region state (owner, type, unrest, garrison), Dominion Response Tier, active strategic AI plans, player state (roster, assets, recent actions), narrative flags.
**Assembly:** pick template per strategic need → instantiate on the region's authored *mission sites* (each region ships 3–6 hand-built sites with data-tagged spawn/objective/insertion points — generation composes authored spaces; it never generates geometry) → parameterize enemies (tier-appropriate), optional objectives, and a **context line** stitched from cause ("Because the Vorn raid cut walker deliveries, the Tarsis garrison convoy runs unarmored — hit it.") — generated missions must always *explain their existence causally*; this single rule makes procedural content feel like a living war instead of a quest board.
**Constraints:** max 3 concurrent generated offers per planet, cooldowns per template (variety pacing), difficulty band from player power ±1 tier (challenge without rubber-banding).

---

## 11.4 Design Standards

- Every mission supports ≥2 approach families (loud/quiet or assault/maneuver); mandatory-combat exceptions flagged in-fiction ("no-extraction op").
- Optional objectives = the reward economy's stretch layer (capture-intact, zero-casualty, ghost, side-rescue) — never mandatory for progression.
- Fail-forward: failed non-critical missions produce consequences (world-state, injuries, story reactions) rather than retry walls; only main-mission deaths of the player reload.
- Length bands: Operations 15–25 min, Story 30–50 min, Hybrid battles 40–70 min; the strategic layer must always offer at least one < 25-min option (respect for session time).

---

*Next document: [12_technical_design.md](12_technical_design.md).*
