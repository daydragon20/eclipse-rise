# SPEC-P1-04 — Strategy Mini-Map (6 Nodes)
*Phase 1 feature spec | GDD refs: 3.1 (map rules), 12.3 (strategy map), 13.2 | Skill owner: Game Architecture Expert (UI: UE Programmer)*

---

## Goal

The smallest strategy board that still produces a real strategic *choice*: 6 regions of one district, connected as a graph, each offering at most one mission. Choosing where to strike (and seeing the map change afterwards) is the top half of the loop. Map rule heritage (3.1): it is a graph of connections, control is per-region, and edges matter — scaled down to one district.

## In scope

- Region graph as DataAsset: 6 nodes (id, name, type: industrial/residential/checkpoint, edges, base yield, starting owner/garrison/unrest).
- Region state lives in `FCampaignState` (SPEC-P1-02); the map world/screen only *reads* it.
- Mini-map screen (debug-grade UMG): nodes colored by owner (Dominion/contested/player), select node → see 1 available mission offer + region info → launch preparation (SPEC-P1-08).
- Mission offers: static per-region templates in data (assault / sabotage / rescue — 2–3 templates total), offer text includes a one-line causal context stub (11.3's context-line rule, hand-written per template).
- Consequence visibility: after a mission, the changed node state must be visibly different on return (color/garrison number) — the loop's "the map noticed" beat.
- Adjacency rule (3.1 rule 1 in miniature): missions can only target regions adjacent to player-held territory.

## Out of scope

Planets, jump-lanes, Gate Spires, supply lines, travel time, Dominion counter-moves, the Mission Generator (offers are static data), fog of war.

## Data

`UEclipseRegionGraphAsset` (nodes + edges + yields), `DT_MissionOffers` (per region type: template id, context line, reward table ref).

## Events

Consumed: `Event.Strategy.RegionControlChanged`, `Event.Economy.ResourcesChanged` (refresh UI).
Emitted: `Event.Strategy.MissionSelected` (payload: region id, template id) — consumed by preparation (SPEC-P1-08).

## Debug UI

The screen itself is debug-grade. Console: `Eclipse.Strategy.FlipRegion <id>` (debug builds).

## Tests (14.4)

- Unit: adjacency legality (non-adjacent target rejected); graph asset validation (no orphan nodes — ValidateData commandlet rule).
- Functional (Gauntlet stub): select mission → complete by script → region state change visible in CampaignState.

## Definition of Done

A tester picks a target for a *reason* they can state ("checkpoint first, it unlocks the factory node"), and after the mission the board shows their progress without any dev explanation.
