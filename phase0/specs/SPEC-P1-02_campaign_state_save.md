# SPEC-P1-02 — CampaignState, Transaction API & Save v0
*Phase 1 feature spec | GDD refs: 12.2 rule 4, 12.3 (save system), 14.3.3, 14.3.6 | Skill owner: Game Architecture Expert*

---

## Goal

The entire strategic state is one serializable struct tree (`FCampaignState`) — the single source of truth. Ground missions read from it and write back **only** through a transaction API (14.3.3: "Ground gameplay proposes; the transaction commits"). This kills the classic hybrid-game save-divergence bug before it exists. Save v0 is minimal but **versioned from day one** because save-migration tests are a CI requirement from day one (13.2 Phase 0).

## In scope

- `FCampaignState` v1: campaign clock (day counter), resource wallet, region states (6 nodes: owner, unrest, garrison strength), roster (soldier records), memorial entries, production queue (1 slot), pending mission consequences.
- `UEclipseCampaignSubsystem` owning the state; read access via const getters only.
- Transaction API: `FCampaignTransaction` (list of typed mutations: `AdjustResource`, `SetRegionOwner`, `AddSoldier`, `KillSoldier`, `AddMemorialEntry`, `QueueProduction`, `AdvanceDay`); `Commit(Transaction)` validates then applies atomically, then broadcasts resulting events on the bus. No other writer exists.
- Save v0: `EclipseSaveSystem` plugin skeleton — versioned header (`SchemaVersion`), serialize/deserialize `FCampaignState`, one autosave slot + manual slot. Human-readable JSON debug export (`Eclipse.Campaign.ExportJson`).
- Migration scaffold: version registry + one no-op migration (v1→v1) proving the pipeline, with CI round-trip fixture.

## Out of scope

Mid-mission checkpoint saves; Data Layer world-state persistence; memorial letters; multiple campaign slots UI.

## Data

`UEclipseCampaignSetupAsset` (DataAsset): starting resources, starting roster size, region definitions reference (SPEC-P1-04), campaign clock start. No hardcoded starting values.

## Events

Consumed: none directly (mutations arrive as transactions from other subsystems).
Emitted (on commit, per mutation): `Event.Campaign.DayAdvanced`, `Event.Economy.ResourcesChanged`, `Event.Strategy.RegionControlChanged`, `Event.Roster.SoldierAdded`, `Event.Roster.SoldierDied`, `Event.Memorial.EntryAdded`, `Event.Economy.ProductionQueued`.

## Debug UI

Console: `Eclipse.Campaign.ExportJson`, `Eclipse.Campaign.AdvanceDay`, `Eclipse.Campaign.GrantResource <type> <amount>` (debug builds only).

## Tests (14.4)

- Unit: transaction atomicity (invalid mutation rejects whole transaction); deterministic state hash after scripted transaction sequence.
- Save integrity (CI, per merge): full-fixture round-trip serialize → deserialize → state hash equal; migration registry test.

## Definition of Done

A scripted sequence (grant resources, flip region, kill soldier, advance day) commits, emits the right events, exports readable JSON, and round-trips through save/load with identical hash.
