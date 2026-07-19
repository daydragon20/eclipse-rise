# ECLIPSE: Rise of the Resistance — UE project
*Phase 0 pre-production skeleton. Design authority: `../` (the Game Design Bible, 00–14). Read `../DOCUMENTATION_README.md` before touching anything.*

## Status

Pre-production (Phase 0, GDD 13.2). This is a compile-ready project skeleton: modules, config, CI contract, event catalog. **No gameplay features exist yet** — Phase 1 implementation starts only after the Phase 0 gate, per the ACTIVE_MILESTONE rule in `../13_roadmap.md`.

## First-time setup (Windows workstation with UE 5.7)

1. Install UE 5.7 + Visual Studio 2022 (C++ game dev workload).
2. Right-click `Eclipse.uproject` → Generate Visual Studio project files.
3. Build `EclipseEditor` (Development Editor | Win64), open the project.
4. CI: provision a self-hosted runner with `UE_ROOT` set (see `.github/workflows/ci.yml`).

## Layout

- `Source/Eclipse` — runtime module (planned subfolders: see `Source/Eclipse/README.md`, fixed by GDD 12.2).
- `Source/EclipseEditor` — editor module; hosts the `EclipseValidateData` CI commandlet.
- `Docs/EventCatalog.md` — the event-bus contract (GDD 14.2 governance; update in the same commit as any tag).
- `Tools/` — CI helper scripts.
- Phase 1 specs live in `../phase0/specs/`.

## Non-negotiables (from GDD 14)

Event bus between subsystems, no direct cross-subsystem access · all tunables in DataAssets · pure-logic cores headless-testable · CampaignState transaction API is the only strategic-state writer · commit format `[System] Verb summary (GDD ref)`.
