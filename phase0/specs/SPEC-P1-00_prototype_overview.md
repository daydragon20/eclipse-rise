# SPEC-P1-00 — Prototype "The Loop": Overview & Gate
*Phase 1 feature spec set — Phase 0 deliverable | GDD refs: 13.2 (Phase 1), 1.6 (core loop), 14.3.4, 14.5*
*Skill owner: Game Architecture Expert*

---

## Purpose

Phase 1 exists to prove or kill the project's three existential risks (GDD 13.1):

1. The ground↔strategy state loop (mission → consequence → strategy choice → preparation → mission).
2. Squad AI quality (orders that *feel obeyed*, squad of 2).
3. The economy/consequence chain (ledger driving one real production choice).

The prototype is deliberately ugly: one graybox district, a 6-node strategy mini-map, a menu-only base. No art, no story content, no polish. Debug UI only (14.5 step 4).

**Gate question (13.2): "Do testers voluntarily play a second loop?"** If no — pivot or kill, honestly.

## Spec set & build order

Specs are ordered by dependency; each follows the modularity method (14.5): data schema → pure-logic core + unit tests → subsystem wrapper + events → debug UI. Real UI/content is out of scope for all of Phase 1.

| # | Spec | Depends on |
|---|---|---|
| 01 | Event Bus | — |
| 02 | CampaignState, Transaction API & Save v0 | 01 |
| 03 | Economy Ledger | 01, 02 |
| 04 | Strategy Mini-Map (6 nodes) | 02, 03 |
| 05 | Mission Runtime & Consequences | 01, 02 |
| 06 | Squad of 2 & Orders | 05 |
| 07 | Roster, Permadeath & Memorial Stub | 02, 05 |
| 08 | Menu Base & Preparation | 03, 07 |

## Global out-of-scope (all Phase 1 specs)

Per ACTIVE_MILESTONE rule and 13.2: no classes (recruits only), no research, no fleet/vehicles, no companions/dialogue content, no base 3D space, no Dominion strategic AI beyond static garrison data, no planets beyond the one graybox district, no VO/audio beyond debug barks (text). Anything not listed in a spec's "In scope" is out.

## Shared conventions

- All tunables in DataAssets/DataTables (14.2); a hardcoded gameplay constant is a defect.
- All new event tags land in `Eclipse/Docs/EventCatalog.md` in the same commit (14.2).
- Pure-logic cores compile without engine actor headers (14.3.2).
- Placeholder systems carry `// PLACEHOLDER(GDD x.y):` tags (DOCUMENTATION_README, Forbidden Actions).

## Definition of Done (Phase 1 as a whole)

A tester can, without dev intervention: pick a node on the mini-map → prepare (spend Intel, pick loadout, pick 2 squadmates) → play the graybox mission with squad orders → see consequences committed (region state, resources, roster deaths → memorial) → make one production choice at the menu base → start the next loop. Save/load round-trips the full state. CI green (build, data validation, save round-trip, squad-AI scenario stubs).
