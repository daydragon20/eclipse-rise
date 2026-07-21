# PART 5 — BASE BUILDING SYSTEM
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 05 of 14*

---

## 5.1 Design Philosophy

The base is three things at once:

1. **A machine** — the engine converting resources into military capability (mechanics).
2. **A home** — where the roster lives, grieves, and celebrates (emotion). Companion scenes, memorial wall, mess-hall vignettes all live here.
3. **A place** — a walkable 3D space that visibly grows from a damp vault into a headquarters (fantasy). *The base is the progress bar of the whole game.*

**Model:** slot-based construction inside authored shells (XCOM/AC-homestead model), **not** freeform building. Why: freeform building (Fallout 4 style) explodes art/AI/navmesh scope and produces ugly bases; slot-based keeps art quality, walkability, and camera work while preserving meaningful choice (which rooms, which order, which upgrades — with real scarcity in slot count).

The player will ultimately hold **multiple bases** (one Primary HQ + Forward Bases per liberated planet). Forward bases use a compressed 6-slot version of the system. This chapter specifies the Primary HQ.

---

## 5.2 The Starting Base — Hollow Point

A disused geothermal maintenance vault beneath Kessara's Underworks. Act 1 state: one corridor, a generator that coughs, 11 bunks, a stolen map table. **Slots unlock with story + Excavation projects:** 4 → 8 → 12 → 16 slots across the campaign.

The HQ relocates twice with the story (Act 2: Tarsis wreck-fortress "The Carcass"; Act 3+: mobile flagship *Dawnbreak* + planetary HQ), each relocation carrying upgrades forward (no progress loss — relocation is a set-piece, not a reset; the Siege of Hollow Point makes the *reason* for moving playable).

---

## 5.3 Buildings (Rooms/Facilities)

Each facility: 3 upgrade levels, visual state per level (art requirement: every level visibly different — new machinery, lighting, staff density), staffing assignment (soldiers/specialists boost output — roster and base interlock).

### 5.3.1 Facility Table (costs in Materials/Credits/Energy upkeep; full economy math in Part 6)

| Facility | Purpose | L1 → L2 → L3 benefits | Build cost (L1/L2/L3) | Energy upkeep |
|---|---|---|---|---|
| **Command Center** (mandatory core) | Strategy map access, mission selection, comms | Map range: local → cluster → full Expanse; +1 concurrent side-op per level | free / 400M+2kC / 1200M+8kC | 2/4/6 |
| **Barracks** | Roster capacity, morale base | Cap 12→24→48; L2 adds mess hall (morale+), L3 adds rec deck (bond growth+) | 150M / 500M / 1500M | 1/2/4 |
| **Workshop → Factory** | Craft/repair gear; later mass production | L1 craft mods; L2 manufacture weapons/armor; L3 vehicle assembly + production queues | 200M / 800M / 2500M | 2/5/9 |
| **Research Lab** | Technology tree progress (Part 10) | +1 research slot per level; L3 enables capstone projects | 300M / 1000M / 3000M | 3/6/10 |
| **Medbay** | Injury recovery, critical-injury care | Recovery time −25%/−40%/−60%; L2 unlocks prosthetics fitting; L3 trauma theater (death→critical window +10 s squad-wide) | 200M / 700M / 2000M | 2/4/7 |
| **Training Academy** | Class assignment, XP training, retraining | L1 assign classes; L2 train to Veteran cap; L3 officer school (Commander class) + cross-training | 250M / 900M / 2600M | 2/4/6 |
| **Intelligence Center** | Intel generation, counter-espionage, mission prep | L1 decrypt intel; L2 run agents (espionage ops); L3 Veil counterintel (infiltration defense) + strategic forecasts | 300M / 1100M / 3200M | 2/5/8 |
| **Power Plant** | Energy supply | +10/+25/+50 Energy | 150M / 600M / 1800M | — |
| **Storehouse** | Resource caps, supply logistics | Caps ×2/×4/×8; L3 automated distribution (+forward-base supply range) | 100M / 400M / 1200M | 1/1/2 |
| **Hangar → Spaceport** | Vehicle storage; later air/space ops | L1 ground vehicles; L2 dropship pad (new insertion options in missions); L3 spaceport (fleet basing, orbital logistics) | 400M / 1500M / 5000M | 3/7/12 |
| **Fleet Command** (Act 3+, requires Spaceport L3) | Space-layer command (Part 7) | L1 wing ops; L2 task forces; L3 grand fleet + the *Dawnbreak* refit | 2000M+20kC / 5000M+50kC / — | 8/14 |
| **Foundry Annex** (liberated-planet unique) | Converts captured Dominion factories | Planet-level production bonuses | capture, not build | varies |
| **Memorial Hall** (auto-grows) | The Wall; morale/loyalty rituals | Grows with losses; ceremonies grant post-tragedy morale recovery | free | 0 |

### 5.3.2 Facility design notes (the WHY per facility)

- **Command Center** gates strategic scope so the map complexity grows with player skill.
- **Workshop→Factory** is the economy's heart: it converts Materials into concrete power and is deliberately the most expensive upgrade line — industrialization *is* the mid-game plot.
- **Intelligence Center** is the anti-frustration engine: intel spent on mission prep (Part 11) converts player knowledge into fairness (fog-of-war relief), and its L3 counterintel is the mechanical answer to the Act 3 mole storyline.
- **Medbay vs. Academy tension:** healing the wounded vs. training the next wave competes for staff — a deliberate recurring dilemma.
- **Memorial Hall costs nothing.** Grief is never a purchase.

---

## 5.4 Construction Rules

- **Time:** construction takes strategic time (hours–days on the campaign clock); Engineers assigned reduce it. Rush option burns Credits (Shroud contractors) — money vs. time, always available, never free.
- **Slots are scarce on purpose:** a full campaign can build ~80% of all L3s; a focused build order is a real strategic identity (military rush vs. tech vs. intel openings — mirroring Civ build-order strategy).
- **Damage & sieges:** base-attack missions (Part 11) use the actual built base as the level. Facilities can be damaged (offline until repaired) — the base is a place you *defend*, not a menu.
- **Visual growth mandate:** each build/upgrade must change the walkable space (staff, light, sound, machinery). The Act 1 vault and Act 4 HQ must be unrecognizable as the same system — this is the fantasy made visible.

---

## 5.5 Forward Bases (per liberated planet)

6 slots, choose from: Garrison (defense rating), Airfield (rapid response range), Clinic, Depot (supply), Listening Post (intel), Recruitment Office, Trade Office. Forward bases determine how well a planet *resists Dominion counterattack* (the Fall-of-Meridia lesson becomes a system: garrisons matter). AI-resolved defense battles (Part 7) weigh forward-base investment heavily.

---

*Next document: [06_economy.md](06_economy.md).*
