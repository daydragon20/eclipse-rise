# ECLIPSE: RISE OF THE RESISTANCE — DOCUMENTATION README

## PURPOSE
This file explains how to navigate the project documentation.
The project contains several large documents. The AI must read and use them in the correct order before generating code, assets, systems, or design changes.

**File mapping note:** the documentation is split into 18 numbered files for maintainability. The logical "Bibles" referenced below map onto them as follows:

| Logical document | Physical files |
|---|---|
| **GAME_DESIGN_BIBLE** | `01_game_vision.md` … `11_missions.md` (Parts 1–11) |
| **TECHNICAL_DESIGN_BIBLE** | `12_technical_design.md` (Part 12) |
| **DEVELOPMENT_ROADMAP** | `13_roadmap.md` (Part 13, incl. ACTIVE_MILESTONE) |
| **MASTER_DEVELOPMENT_PROMPT** | `14_ai_dev_instructions.md` (Part 14) |
| **VISUAL_QUALITY_CHARTER** | `15_visual_quality_charter.md` (Part 15, art & rendering direction) |
| **AUDIO_SYSTEM** | `16_audio_system.md` (Part 16, audio + ElevenLabs pipeline) |
| **CINEMATIC_ANIMATION_SYSTEM** | `17_cinematic_animation_system.md` (Part 17, animation & cinematics) |
| **Master index & canon glossary** | `00_INDEX.md` |

---

# DOCUMENT HIERARCHY (HIGHEST PRIORITY FIRST)

## 1. GAME DESIGN BIBLE (Parts 1–11) — PRIMARY SOURCE OF TRUTH
Core vision, gameplay pillars, story, world lore, player progression, base building, economy, combat, research, mission structure.
**RULE:** if any other document conflicts with this one, the Game Design Bible wins. Name/lore conflicts are resolved by the canon glossary in `00_INDEX.md`.

## 2. TECHNICAL DESIGN BIBLE (`12_technical_design.md`)
Unreal Engine architecture, folder structure, C++ class design, Blueprint rules, AI framework, save system, data asset structure, networking assumptions (single-player only — Iris disabled), performance targets.
**RULE:** all code must follow this architecture.

## 3. MASTER DEVELOPMENT PROMPT (`14_ai_dev_instructions.md`)
AI development behavior, coding standards, refactoring rules, testing requirements, modular design principles, optimization guidelines, the 7 AI developer skills.
**RULE:** follow these instructions during every development session.

## 4. DEVELOPMENT ROADMAP (`13_roadmap.md`)
Build order, milestones, prototype goals, alpha/beta scope, feature priorities.
**RULE:** only implement the current milestone (see ACTIVE_MILESTONE in `13_roadmap.md`) unless explicitly instructed otherwise.

## 5. VISUAL QUALITY CHARTER (`15_visual_quality_charter.md`)
AAA art & rendering target, hardware reality (dev box vs. shipping spec), per-phase fidelity plan, optimization discipline.
**RULE:** subordinate to the Game Design Bible and bound by the performance budgets (12.4); **paced by the roadmap — it never overrides the ACTIVE_MILESTONE.** Phase 1 stays deliberately graybox (13.1); fidelity work begins at Phase 2.

---

# REQUIRED READING ORDER
Before writing any code, read in this exact order. Do not skip steps.

1. `DOCUMENTATION_README.md` (this file)
2. `00_INDEX.md` (pillars + canon glossary)
3. Game Design Bible: `01`–`11` (or at minimum `01` + the parts relevant to the task)
4. `12_technical_design.md`
5. `14_ai_dev_instructions.md`
6. `13_roadmap.md` → ACTIVE_MILESTONE

---

# QUICK INDEX

| Topic | File → Section |
|---|---|
| Story | `02_story_bible.md` |
| World / planets | `03_world_design.md` |
| Player & squad systems | `04_core_gameplay.md` |
| Base building | `05_base_building.md` |
| Economy | `06_economy.md` |
| Army / vehicles / fleet | `07_military.md` |
| Combat | `08_combat_design.md` |
| AI behavior | `09_ai_systems.md` |
| Research | `10_technology_tree.md` |
| Missions & generator | `11_missions.md` |
| Unreal Engine structure | `12_technical_design.md` → 12.1–12.2 |
| Save system | `12_technical_design.md` → 12.3 |
| Performance budgets | `12_technical_design.md` → 12.4 |
| Coding conventions | `14_ai_dev_instructions.md` → 14.2–14.3 |
| Testing requirements | `14_ai_dev_instructions.md` → 14.4 |
| Visual quality / art & rendering | `15_visual_quality_charter.md` |
| Hardware reality (dev vs. target) | `15_visual_quality_charter.md` → 15.2 |
| Audio / voice / music / ElevenLabs | `16_audio_system.md` |
| Audio credit budget & always-on music | `16_audio_system.md` → 16.7, 16.13 |
| Animation & cinematics | `17_cinematic_animation_system.md` |
| Current task | `13_roadmap.md` → ACTIVE_MILESTONE |

---

# CORE DESIGN PILLARS
Every feature must support at least one of these pillars (canonical five in `00_INDEX.md`; expanded formulation):

- From nobody to galactic leader (THE CLIMB)
- Meaningful strategic growth (EARNED VICTORY)
- Persistent army and consequences (PEOPLE, NOT UNITS)
- Player-driven rebellion
- Large-scale but readable warfare
- Strong narrative immersion
- Modular systems that interact with each other (EVERYTHING IS CONNECTED / ACHIEVABLE AMBITION)

If a feature does not support a pillar, reject or redesign it.

---

# IMPLEMENTATION CHECKLIST
Before implementing a feature:

- Read the relevant design section.
- Check technical constraints (`12_technical_design.md`).
- Verify it fits the current roadmap milestone.
- Identify required data assets.
- Identify save/load requirements.
- Identify AI interactions.
- Identify UI requirements.
- Estimate performance impact (budgets: 12.4).
- Write modular code (headless-core-first method: 14.5).
- Add debug/testing hooks (14.4).

---

# CHANGE MANAGEMENT
Never silently change gameplay rules. When proposing a change:

1. Explain the problem.
2. Explain the proposed solution.
3. List affected systems (event catalog check).
4. Update the relevant document section (same commit).
5. Then implement the change.

---

# FORBIDDEN ACTIONS
Do NOT:

- Invent lore that contradicts the Bible (check `00_INDEX.md` glossary).
- Add multiplayer systems.
- Create hardcoded gameplay values (all tunables in DataAssets — 14.2).
- Skip save-system integration (serialization contract per subsystem — 12.3).
- Bypass AI architecture (perception/knowledge-token model — 09).
- Implement placeholder systems without marking them (`// PLACEHOLDER(GDD x.y):` tag required).
- Add features outside the roadmap scope.

---

# SESSION START PROTOCOL
At the start of every session:

1. Read this README.
2. Read the active milestone (`13_roadmap.md`).
3. Read all referenced sections for the task.
4. Declare the active AI developer skill (14.6) and summarize the task in one sentence.
5. List affected systems.
6. Begin implementation.
7. Run validation checks (CI: data validation, unit tests, squad-AI scenario suite, save round-trip).
8. Report completed work and next steps.

---

# GOLDEN RULE
**Consistency is more important than speed.**
The project should feel like it was built by one coordinated AAA development team, not by isolated AI sessions.
