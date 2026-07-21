# Phase 0 — Pre-Production: Status Tracker
*Milestone authority: `../13_roadmap.md` → ACTIVE_MILESTONE. Scope: feature specs for Phase 1 only; UE project setup + CI; art style bible; dialogue-plugin decision; graybox feel targets. Nothing beyond.*

## Deliverable status

| # | Deliverable | Status | Where |
|---|---|---|---|
| 1 | Feature specs, Phase 1 only | **Done** (9 specs, event contracts included per 14.3.4) | `specs/SPEC-P1-00` … `SPEC-P1-08` |
| 2 | UE project setup | **Done** (skeleton: modules, config, Iris off, event catalog) — first build **verified 2026-07-19** on Windows + UE 5.8 (engine bumped 5.7→5.8 per GDD 12 "5.7+"; see Eclipse repo commits `b7fcd74`/`0294e5b`) | `../Eclipse/` |
| 3 | CI from day one | **Done** (workflow + ValidateData commandlet stub + catalog check) — needs a self-hosted UE runner provisioned | `../Eclipse/.github/workflows/ci.yml` |
| 4 | Art style bible + 1 concept piece per planet | **Bible done; briefs done** — concept *execution* open (needs artist / supervised image pipeline) | `art_style_bible.md` |
| 5 | Dialogue-plugin decision | **Done** (build in-house, phased; front-end re-eval at Phase 2 gate) | `dialogue_plugin_decision.md` |
| 6 | Graybox feel targets locked | **Doc locked** — reference clips still to capture | `graybox_feel_targets.md` |

## Open items to close Phase 0 (human/hardware-dependent)

1. ~~Windows workstation: generate project files, verify `EclipseEditor` builds clean.~~ **Done 2026-07-19** (UE 5.8: build 14/14 clean, ValidateData green, event-catalog check green; automation-tests job goes green with the first Phase 1 test commit — zero project tests exist pre-Phase 1 and UE 5.8 exits non-zero on an empty match).
2. Provision self-hosted CI runner (`UE_ROOT`), confirm all three jobs run. **Carried into Phase 1.**
3. Commission/execute the 10 concept pieces from the briefs in `art_style_bible.md` §5. **Carried into Phase 1.**
4. Capture the 5 reference clips (`graybox_feel_targets.md` §5) into `Eclipse/Docs/FeelReferences/`. **Carried into Phase 1.**
5. ~~Milestone gate review~~ **Passed 2026-07-19 by project owner** (explicit instruction to start Phase 1); ACTIVE_MILESTONE updated. Items 2–4 remain open as carryover — none block prototype code.

## Rules observed in this phase

- No Phase 1+ feature code written (ACTIVE_MILESTONE rule; the only C++ is project/CI infrastructure, placeholder-tagged where relevant).
- All Phase 1 event tags pre-documented in `../Eclipse/Docs/EventCatalog.md` (14.2), status `specified`.
- Specs follow the modularity method order (14.5) and name their events before code exists (14.3.4).
