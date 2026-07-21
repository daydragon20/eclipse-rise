# SPEC-P1-06 — Squad of 2 & Orders
*Phase 1 feature spec | GDD refs: 8.4, 9.5, 12.3 (character system), 13.2, 14.4 (squad AI bar) | Skill owner: AI Systems Engineer*

---

## Goal

Two AI squadmates who execute orders so reliably that a tester says "my people are competent" (9.5's core promise). This is existential risk #2 (13.1). The quality bar is absolute even at prototype scale: **an order silently failing is a release blocker by definition** (8.4) — the refusal message is as much a feature as the execution.

## In scope

- Squad of 2 recruits drawn from the roster (SPEC-P1-07); same `AEclipseCharacter` body as the player (12.3: soldiers must be player-quality; one class, component-composed).
- Orders (subset of 8.4): **Move to position** (with stance stub: ready/aggressive), **Focus target**, **Hold**, **Regroup**. Issued via targeting reticle + hotkeys; no time dilation yet.
- Order execution: BT per squadmate + minimal shared blackboard; intelligent-enough cover-point pick near ordered position (EQS query, simple scoring — the *same* query enemies will later use, 8.3/9.3 fairness rule).
- **Verbal transparency (9.5):** every order gets an acknowledge bark (text on screen + log); impossible orders get a refusal with reason ("No route", "Can't see target"). No silent state.
- Self-preservation layer under player orders (9.5 priority stack, tiers 1–2 only).
- Squadmate death: permanent, flows into SPEC-P1-07's pipeline.

## Out of scope

Classes, abilities, bonds/traits (roster carries the fields; AI ignores them), suppression, flanking/breach/sync-strike orders, stealth discipline, medic behavior, enemy squad coordinator.

## Data

`DT_SquadOrderDefs` (order id, ack/refusal line pools), BT/EQS assets, `UEclipseSquadTuningAsset` (follow distances, cover search radius, refusal timeout — no hardcoded numbers).

## Events

Consumed: mission lifecycle events (`Event.Mission.Started` — spawn squad; `Event.Mission.Completed/Failed` — stand down).
Emitted: `Event.Squad.OrderIssued`, `Event.Squad.OrderAcknowledged`, `Event.Squad.OrderRefused` (payload: soldier id, order, reason), `Event.Squad.SoldierDowned`.

## Debug UI

Order-state widget per squadmate (current order, BT state); `Eclipse.Squad.DumpOrders` console log.

## Tests (14.4 — squad AI scenario suite, runs per merge from Phase 1)

- Scenario: move-to-cover under fire (reaches point or refuses with reason within timeout).
- Scenario: order refusal messaging (blocked route → `OrderRefused` event with reason, never timeout-silence).
- Scenario: focus-target on dead/invalid target → refusal.
- Assertion in all scenarios: zero orders in terminal state "unanswered".

## Definition of Done

Ten consecutive playtest orders each get visible acknowledgment or reasoned refusal; scenario suite green in CI; a tester spontaneously uses orders in loop #2 (gate signal).
