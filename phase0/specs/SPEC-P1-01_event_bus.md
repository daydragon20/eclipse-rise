# SPEC-P1-01 — Gameplay Event Bus
*Phase 1 feature spec | GDD refs: 12.2 rule 2, 4.3, 14.2, 14.3.1 | Skill owner: Game Architecture Expert*

---

## Goal

One central, typed publish/subscribe bus through which all subsystems communicate. GDD 12.2: "Missions never call Economy directly; they broadcast `Event.Mission.FacilityCaptured`." The interconnection arrows of Part 4.3 become literal subscriptions. This is the first thing built because every other Phase 1 spec names its contract in bus events.

## In scope

- `UEclipseEventBusSubsystem` (`UGameInstanceSubsystem`): `Broadcast(FGameplayTag, const FInstancedStruct& Payload)` + `Subscribe(FGameplayTag, handler)` with hierarchical tag matching (subscribing to `Event.Mission` receives `Event.Mission.Completed`).
- Typed payload structs per event family (`FEclipseMissionEventPayload`, `FEclipseEconomyEventPayload`, ...), carried as `FInstancedStruct`.
- Event tags declared in one central `EclipseGameplayTags` native tag file.
- Debug: console command `Eclipse.Events.Dump` (log last N events with payload summary); optional on-screen event ticker.
- Governance: every tag documented in `Docs/EventCatalog.md`, same commit (14.2). CI check greps declared native tags against the catalog.

## Out of scope

Cross-save event persistence; async/queued delivery (synchronous broadcast is fine at prototype scale); Blueprint-defined new event families (BP may subscribe/broadcast existing ones).

## Data

No DataAssets. Native gameplay tags only.

## Events

Consumed: none (infrastructure).
Emitted: none of its own; transports all tags listed in `Docs/EventCatalog.md`.

## Tests (14.4)

- Unit: subscribe/broadcast/unsubscribe; hierarchical matching; payload type mismatch logs an error and does not crash (14.3.5 spirit).
- CI: EventCatalog coverage check (every native `Event.*` tag has a catalog row).

## Definition of Done

Two dummy subsystems in a test communicate exclusively via the bus; `Eclipse.Events.Dump` shows the traffic; catalog check green.
