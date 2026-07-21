# PART 14 — AI DEVELOPMENT INSTRUCTIONS
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 14 of 14 | Audience: AI coding agents (Claude Code etc.) + the humans directing them*

---

## 14.1 Development Priorities (fixed order of truth)

When instructions conflict, resolve in this order:
1. **The Design Bible** (these 14 documents) — design intent wins.
2. **Architecture rules** (12.2 + below) — structure wins over speed.
3. **Existing code patterns** — consistency wins over preference.
4. **The task prompt** — never let a task prompt silently violate 1–3; flag the conflict instead.

**Priority of work:** core loop stability > squad AI quality > save integrity > new features > polish > optimization (until Beta, where optimization rises).

## 14.2 Coding Standards

- UE naming conventions strictly (`AEclipse*`, `UEclipse*`, `FEclipse*`, `EEclipse*`); no abbreviations in public APIs.
- C++: no logic in headers beyond inlines; every public function documented with *why*, not just what; no `Tick` without justification comment (event-driven default).
- Blueprints: max 30 nodes per graph function — refactor to C++ or BP functions beyond that; no BP-to-BP hard references across system boundaries (use the event bus / interfaces).
- All tunable numbers in DataAssets/DataTables — a hardcoded gameplay constant is a defect.
- Every event-bus tag documented in `Docs/EventCatalog.md` (agents must update it in the same commit that adds a tag).
- Commits: one system per commit; message format `[System] Verb summary (GDD ref)` e.g. `[Squad] Add bond grief penalty (GDD 4.2.6)`.

## 14.3 Architecture Rules (anti-mess constitution)

1. Subsystems only communicate via the event bus or explicit public interfaces — direct cross-subsystem member access is forbidden.
2. Pure-logic cores (BattleSimulator, MissionGenerator, Economy ledger) must compile without engine actor headers — headless-testable forever.
3. The CampaignState transaction API is the only writer of strategic state. Ground gameplay proposes; the transaction commits.
4. New features start as a one-page spec referencing the GDD section they implement; the spec lists the events consumed/emitted *before* code is written.
5. No system may hard-depend on content existing (missing DataAsset = logged warning + graceful default, never a crash).
6. Deprecation over mutation: breaking a serialized format requires a save-migration entry + test, same commit.

## 14.4 Testing Requirements (per system, non-negotiable)

| Layer | Requirement |
|---|---|
| Unit (Automation Spec) | All pure-logic cores: simulator battles, economy ticks, generator outputs, tech-tree dependency graph (no orphans/cycles) — deterministic seeds |
| Functional (Gauntlet) | Per mission template: spawn → complete-by-script → verify consequences committed to CampaignState |
| Squad AI quality bar | Scenario suite (cover under fire, order refusal messaging, stealth discipline, medic triage) run per merge; any silent order failure = release blocker |
| Save integrity | Round-trip serialize/deserialize full campaign fixtures per merge; migration tests for every schema version |
| Campaign soak | Nightly bot-driven strategic-layer full campaign; assert economy curves within Part 6 bands |
| Performance | Per-merge budget checks on reference scenes (12.4 budgets) |

**Definition of Done:** spec referenced + code + data + tests + EventCatalog/docs updated + passes CI + a human (or reviewing agent with human sign-off) has played/seen it.

## 14.5 Modularity Method

Build every system in this sequence: (1) data schema (DataAsset), (2) pure-logic core + unit tests, (3) subsystem wrapper + events, (4) minimal debug UI (console/ImGui-style), (5) real UI/content last. This lets systems ship "invisible but true" early and keeps content teams unblocked — and it is the single best defense against messy code: UI never grows tendrils into logic that was born headless.

## 14.6 AI Developer Skills (role personas)

The AI agent must declare its active skill at task start and switch explicitly. Each skill = a lens with its own checklist:

**Skill 1 — Game Architecture Expert.** Owns: subsystem boundaries, event bus, CampaignState, save. Checklist: does this change respect rules 14.3.1–6? What breaks at campaign hour 150? Veto power over all other skills.
**Skill 2 — Unreal Engine Programmer.** Owns: GAS, World Partition, StateTree/BT integration, performance idioms. Checklist: engine-native solution exists? (Search before building.) Correct thread/tick discipline? DataAsset-driven?
**Skill 3 — AI Systems Engineer.** Owns: Part 9. Checklist: is the behavior *readable* by the player? traceable to perception events? Does the squad verbally explain failures? Scenario suite updated?
**Skill 4 — Combat Designer.** Owns: Part 8 feel. Checklist: TTK bands respected? every enemy has tell + counterplay? does the change survive the "read→plan→strike→adapt" rhythm test? Tune in data, not code.
**Skill 5 — World Builder.** Owns: Part 3 identities, mission sites, state layers. Checklist: one-screenshot identity? landmark orientation? all 3 occupation states dressed? site tagged for the generator?
**Skill 6 — Narrative Designer.** Owns: Parts 2 & 11 story surface. Checklist: both sides of every choice defensible in one sentence? consequence touches a system? barks reference campaign state? no lore contradicting the Bible (check names against 00_INDEX glossary).
**Skill 7 — Optimization Expert.** Owns: budgets (12.4). Checklist: measure before changing (Insights trace attached to PR); optimize the top cost, not the interesting cost; never trade squad-AI quality for frames without a design sign-off.

**Switching rule:** a task touching multiple domains is decomposed into per-skill subtasks; the Architecture Expert reviews the seams. Agents must refuse "quick hacks" that violate the constitution and propose the compliant alternative — in this project, *the fast way is the way that doesn't create a second project called Refactoring Hell.*

---

*End of the ECLIPSE Game Design Bible. Master index: [00_INDEX.md](00_INDEX.md).*
