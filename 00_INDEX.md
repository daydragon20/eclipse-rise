# ECLIPSE: RISE OF THE RESISTANCE
## Game Design Bible — Master Index
*Version 1.0 — July 2026 | Single-player third-person Action-Strategy RPG | Unreal Engine 5.7+*

> **One-line pitch:** Start as a nobody under a galactic empire. End as the leader of the greatest rebellion in history.

---

> **Start here for development sessions:** [STATUS.md](STATUS.md) — the compact status card: where we stand, the two tracks, open dossiers, owner queue, hard rules. Read this first, not the full bible.
> **Nathan's screen:** `START_DASHBOARD.bat` → **http://127.0.0.1:8377/** — the live dashboard (tasks, agents + their chats, all documents, screenshots, credits).
> **Document hierarchy and session protocol:** [DOCUMENTATION_README.md](DOCUMENTATION_README.md).

## Document Map

| # | Document | Contents |
|---|---|---|
| 01 | [Game Vision](01_game_vision.md) | Identity, pillars, player fantasy, audience, core loop, signature moments, scope honesty |
| 02 | [Story Bible](02_story_bible.md) | Galaxy history, the Dominion, characters, villains, moral framework, twists, 4 acts, endings |
| 03 | [World Design](03_world_design.md) | Jump-lane galaxy map, 10 planets in full detail, world-production rules |
| 04 | [Core Gameplay](04_core_gameplay.md) | Player movement/combat/abilities/progression; squad system, 9 classes, injuries, morale, permadeath |
| 05 | [Base Building](05_base_building.md) | Slot-based HQ, 13 facilities with upgrade tables, forward bases, sieges |
| 06 | [Economy](06_economy.md) | 6 resources, income/costs tables, balance curves, anti-snowball/anti-frustration design |
| 07 | [Military](07_military.md) | Three scales of war, ground/air/space forces, officers, hybrid battles, fleet combat |
| 08 | [Combat Design](08_combat_design.md) | Feel statement, weapons, cover, command mode, stealth, large battles, bosses |
| 09 | [AI Systems](09_ai_systems.md) | NPC/enemy/squad AI, perception model, Dominion Response Tiers (strategic AI) |
| 10 | [Technology Tree](10_technology_tree.md) | 5 eras × 9 categories, ~120 nodes, physical prerequisites, arms-race design |
| 11 | [Missions](11_missions.md) | 3 authorship tiers, template catalog, liberation campaigns, Mission Generator |
| 12 | [Technical Design](12_technical_design.md) | UE5 feature map, project structure, architecture spine, system implementations, budgets |
| 13 | [Roadmap](13_roadmap.md) | 6 phases over ~4.5 years, gates, pre-committed cut lines, team shape |
| 14 | [AI Dev Instructions](14_ai_dev_instructions.md) | Priorities, coding standards, architecture constitution, testing bar, 7 AI skills |
| 15 | [Visual Quality Charter](15_visual_quality_charter.md) | AAA art & rendering target, hardware reality, per-phase fidelity plan, optimization discipline |
| 16 | [Audio System](16_audio_system.md) | Audio architecture, ElevenLabs pipeline (TTS/music/SFX), always-on adaptive music, credit budget |
| 17 | [Cinematic & Animation System](17_cinematic_animation_system.md) | MetaHuman animation, motion matching, facial performance, dynamic camera, runtime cinematics |
| 18 | [Writing Standard](18_writing_standard.md) | Dialogue craft, line-length bands, character voice fingerprints, bark design, the anti-slop gate |
| 19 | [Voice Production](19_voice_production.md) | ElevenLabs casting, audio tags, the credit ladder, the generation workflow |
| 20 | [World Dressing Standard](20_world_dressing_standard.md) | Composition, believability, the no-filler law, what makes a view epic |
| 21 | [Quality Mandate](21_quality_mandate.md) | **Standing owner instruction** — graphics and comprehensiveness are always the two highest priorities. Never the shortest path. |

## Working documents (phase0)

| Document | Contents |
|---|---|
| [EXECUTION_PLAN](phase0/EXECUTION_PLAN.md) | Track B backlog — risk-first, with falsification tests |
| [SCRIPT_PRODUCTION_PLAN](phase0/SCRIPT_PRODUCTION_PLAN.md) | Track A — the layer model, the calibration mission, the sprint calendar |
| [SCRIPT_FORMAT](phase0/SCRIPT_FORMAT.md) | The one file shape every scene uses |
| [DEBUG_DISCIPLINE](phase0/DEBUG_DISCIPLINE.md) | How a bug takes hours instead of sessions — plus the known-cause catalogue |
| [VOICE_LEDGER](phase0/VOICE_LEDGER.md) | Actual ElevenLabs credit spend |

## Canon Glossary (names — single source of truth)

**Setting:** the Vantara Expanse · Origin (mythic homeworld) · jump-lanes · Gate Spires · AE (After Exodus; present = 503 AE)
**Factions:** The Dominion (formerly Helion Compact) · the Veil (secret police) · the Bursary · AEGIS (predictive network) · Ember Cell → **Eclipse** → the Free Vantara Concord · Iron Chorus (rival cell) · Ashline Cartel (collaborators)
**Protagonist:** Voss, callsign **"Cinder"** · aunt Petra Voss
**Companions:** Mara Sovann · Dex Callum · Dr. Elin Reyes · Torren Vale · Kaya Renn · Whisper (= Ilan Vex) · Sela Vann · Brick (Oram Bex)
**Villains:** High Arbiter Malachar Vex · Grand Marshal Sera Kaine · Inquisitor Dahl Threx · Magistrate Oren Callis
**Planets (10):** Kessara · Tarsis · Krad-9 · Vel'Naar · The Shroud (moon Nym) · Meridia · Sylvaris · Elystra · Vorn · Aurelion
**Key ships/places:** Hollow Point (first base) · The Carcass (Tarsis HQ) · *Loyal Ghost* (first corvette) · *Dawnbreak* (flagship) · the Radiant Spire · Blacksite K-77
**Key events:** the Long Diaspora · Reunification Wars · the Meridia Blight (engineered) · the Silent Coup · Sylvaris Reprisals · Tithe of Hands · the Fall of Meridia · the Eclipse (finale)
**Resources:** Credits (C) · Materials (M) · Energy (E) · Research Data (RD) · Intel (I) · Food (F)

## The Five Pillars (test every feature against these)
1. THE CLIMB — scale must be felt. 2. EVERYTHING IS CONNECTED — no isolated minigames. 3. PEOPLE, NOT UNITS — losses must hurt. 4. EARNED VICTORY — no single-battle wins. 5. ACHIEVABLE AMBITION — shippable by a small AI-assisted team.

## Reading Paths
- **New team member:** 01 → 02 → 04 → 11.
- **Engineer/AI agent:** 01 → 12 → 14 → then per-system docs as tasked.
- **Investor/pitch:** 01 → 03 → 13.
