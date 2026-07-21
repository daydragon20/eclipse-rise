# SPEC-P1-03 — Economy Ledger
*Phase 1 feature spec | GDD refs: 6.2, 6.3, 12.3 (economy system), 13.2 | Skill owner: Game Architecture Expert*

---

## Goal

A pure C++ ledger (headless, deterministic — 14.3.2) that makes one production choice *matter* in the loop: spend Materials on gear now vs. save for something better. Prototype thesis: the player must feel GDD 6.1 rule 2 ("scarcity migrates upward") in miniature — bullets are tight, choices are real.

## In scope

- Resources (prototype subset of 6.2): **Credits (C)**, **Materials (M)**, **Intel (I)**. Intel decay (5%/week, 6.2) included — it forces spending and is cheap to implement.
- Ledger core: balances, earn/spend ops with reasons (auditable log — the after-action transparency principle of 7.6 applied to money), per-day tick (wages stub: flat C/day per soldier, 6.4.1).
- Income: mission rewards (from consequences, SPEC-P1-05) + per-day region yield for player-owned regions (6.3.2: values from data, Contested = 40%).
- One production choice (13.2): Workshop menu offers exactly two mutually exclusive orders per loop, e.g. *Rifle platform (40 M, 2 h)* vs. *Armor set (60 M, 3 h)* (6.4.2 values) — produced item changes next mission's loadout options.
- All numbers in one `UEclipseEconomyDataAsset` + DataTables (region yields, item costs, wages).

## Out of scope

Food/Energy/RD; trading, smuggling, markets; inflation events; income floors; alliance tithes; storage caps (log a `// PLACEHOLDER(GDD 6.2)` where caps will bite).

## Data

`UEclipseEconomyDataAsset`: starting balances, wage table, intel decay rate. `DT_RegionYields`, `DT_ProductionItems` (cost M/C, time, resulting loadout tag).

## Events

Consumed: `Event.Campaign.DayAdvanced` (tick: yields, wages, intel decay, production timers), `Event.Mission.Completed` (apply rewards via transaction).
Emitted (via CampaignState transactions): `Event.Economy.ResourcesChanged`, `Event.Economy.ProductionQueued`, `Event.Economy.ProductionCompleted`.

## Debug UI

Console: `Eclipse.Economy.Report` (balances + last 20 ledger lines with reasons). Menu-base screen shows wallet + queue (SPEC-P1-08).

## Tests (14.4)

- Unit: deterministic given state (replayable — 12.3); day-tick math vs. data tables; intel decay; production completes on schedule; insufficient-funds rejection.
- Campaign soak stub: 30 scripted days stay within hand-set prototype bands (precursor of the Part 6 band assertion).

## Definition of Done

Playtester can articulate the tradeoff they made ("I took the armor, so we're out of rifle money") and the ledger log proves every number's origin.
