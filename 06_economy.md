# PART 6 — ECONOMY SYSTEM
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 06 of 14*

---

## 6.1 Design Philosophy

The economy is the game's second protagonist. The campaign's thesis — *you cannot beat an empire with heroism, only with a civilization* — is expressed mechanically here. Three rules:

1. **Every resource has one clear fantasy.** No overlapping currencies.
2. **Scarcity migrates upward.** Act 1: bullets are scarce. Act 2: materials. Act 3: production capacity & food. Act 4: time and lives. The player never escapes scarcity; they graduate to nobler scarcities.
3. **Income comes from the map; the map comes from war.** Economy and territory are one system.

---

## 6.2 Resources

| Resource | Fantasy | Earned from | Spent on | Storage cap? |
|---|---|---|---|---|
| **Credits (C)** | Money — the grey world | Trade, smuggling, allied taxes, captured treasuries, mission rewards | Wages, bribes, rush orders, black-market gear, mercenaries, faction gifts | No cap |
| **Materials (M)** | Industrial matter (alloys, components) | Mining ops, salvage, captured factories, planet income | Construction, manufacturing, vehicles, ships, repairs | Storehouse cap |
| **Energy (E)** | Power throughput (rate, not stock) | Power Plants, captured grids, tidal/fusion research | Facility upkeep, production speed, shields; surplus enables overdrive modes | Rate-based |
| **Research Data (RD)** | Knowledge | Labs (rate), recovered tech caches, scientists rescued, reverse-engineering captured gear | Technology tree (Part 10) | No cap |
| **Intel (I)** | Secrets | Intelligence Center, Scouts, informants, espionage missions, decrypted cores | Mission prep (recon), espionage ops, counterintel, unlocking hidden strategic targets, twist-arc choices | Soft cap (stale intel decays 5%/week — secrets age) |
| **Food (F)** (Act 3+) | Feeding the freed | Elystra, agri-regions, trade | Liberated-population upkeep; army field rations; morale quality tiers | Storehouse cap |

**Why Intel decays and Credits don't:** Intel decay forces spending (espionage stays active, not hoarded); Credits hoarding is self-punishing anyway via inflation events (Bursary devaluations — economic warfare as story-systemic beats).

---

## 6.3 Income Streams (how the player earns)

1. **Missions:** direct rewards (authored) + looting field assets. Dominant in Act 1–2.
2. **Territory income:** each controlled region yields per-day resources by its type (mining region → M; trade district → C; farmland → F). Dominant from Act 2 on. *Regions produce at 40% while Contested, 100% Liberated, +25% with Forward Base Depot.*
3. **Capturing facilities:** factories, mines, spaceports flip to player production — a factory captured intact (optional stealth objectives) yields 100%; stormed, 60% until repaired. This single rule makes "how we win" a permanent tactical question.
4. **Trading:** the Hub + Shroud markets; price curves per planet (buy salvage cheap on Tarsis, sell to Meridia). Simple 2-good-per-planet arbitrage model — flavor and margin, not a trading sim. Smuggling runs (Kaya ops) = high-margin, risk-of-loss Credit income.
5. **Mining ops (strategic):** assign engineer teams + escort to asteroid/deep-site nodes; periodic raid events defend them.
6. **Alliances:** allied factions tithe resources per their identity (Krad-9: M; Meridia houses: C; Elystra: F) — and *withhold* during political crises (diplomacy has a wallet).

---

## 6.4 Costs (what the player pays for)

### 6.4.1 Personnel

| Item | Cost | Notes |
|---|---|---|
| Soldier wage | 15 C/day (Regular) → 60 C/day (Legend) | Wages scale with rank; a big roster is a payroll — the mid-game Credit sink |
| Recruitment drive | 500 C per wave | Faction levies free but political |
| Specialist (scientist/engineer) | 2000 C hire + 40 C/day | Also rescued (free, loyal) from prisons |
| Mercenaries (Shroud) | 3000–15000 C per op | Instant force, zero loyalty, story friction |

### 6.4.2 Equipment & War Materiel (representative)

| Item | Materials | Credits | Time |
|---|---|---|---|
| Rifle platform | 40 M | — | 2 h |
| Armor set (Medium) | 60 M | — | 3 h |
| Weapon mod | 15 M | 100 C | 1 h |
| Ground vehicle (APC) | 400 M | 1000 C | 1 day |
| Tank | 900 M | 2500 C | 2 days |
| Gunship | 1400 M | 6000 C | 3 days |
| Corvette | 5000 M | 20000 C | 8 days |
| Cruiser | 18000 M | 80000 C | 24 days |
| Battleship (capstone) | 60000 M | 250000 C | 60 days |

Ship costs are deliberately civilization-scale: a battleship is a *campaign project*, and losing one must be a strategic event (Part 7).

### 6.4.3 Structures & research: see Parts 5 and 10 tables. Research costs RD + sometimes a physical prerequisite (captured artifact, rescued specialist) — knowledge needs hands, not just points.

---

## 6.5 Economic Balance Model

Target macro-curve (per campaign day, mid-difficulty):

| Phase | Income M/day | Income C/day | Main sink | Player feeling |
|---|---|---|---|---|
| Act 1 | 20–60 | 50–150 | ammo/gear | "We are broke and desperate" |
| Act 2 | 200–800 | 400–1500 | construction | "We are building something" |
| Act 3 | 2000–6000 | 3000–10000 | army + fleet + food | "I run a nation's war economy" |
| Act 4 | 5000–9000 | 8000–15000 | fleet attrition, final ops | "Everything we built, spent" |

**Balancing levers (exposed as DataAssets for tuning, Part 12):** region yields, wage rates, production times, Dominion raid frequency (destroys income → the enemy attacks the economy, not just the player).

**Anti-snowball:** Dominion economic warfare escalates with player success — lane blockades (trade income −), scorched-earth withdrawals (captured facilities damaged), currency devaluation events. The player's answer is diversification and the Vorn/production endgame — teaching real strategic logic.

**Anti-frustration:** income floors per act (hidden), buy-back of any *story-critical* capability, and advisor prompts (Dex/Sela flag starvation spirals before they bite: "We can't pay them, Cinder. Choose what we stop doing.") — turning failure states into narrative decisions.

---

## 6.6 Economy × Other Systems (interconnection audit)

| System | Economy touchpoint |
|---|---|
| Story | Blight/food politics; Callis bribery arc; armistice offer includes trade terms |
| Squad | Wages, gear, food quality → morale |
| Base | All construction; Energy as base throughput |
| Military | Every unit is a purchase with upkeep; attrition is an invoice |
| Missions | Capture-intact incentives; economic raid targets (both directions) |
| AI/Dominion | Response tiers target economy (blockades, raids) |
| Tech tree | RD generation; techs that transform economy (automation, fusion) |

---

*Next document: [07_military.md](07_military.md).*
