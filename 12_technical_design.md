# PART 12 — TECHNICAL DEVELOPMENT PLAN (TDD)
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 12 of 14 | Engine: Unreal Engine 5.7+ (evaluate 5.8 features per-case)*

---

## 12.1 Engine Baseline & Feature Usage

| UE5 feature | Use in ECLIPSE | Status note (2026) |
|---|---|---|
| **World Partition + OFPA + Data Layers** | Planet playspaces (2–6 km²); Occupied/Contested/Liberated visual states as Data Layers | Core, mature |
| **Nanite (+ Nanite Foliage, 5.7)** | Environment density; Sylvaris canopy | Production-ready |
| **Lumen / MegaLights** | Dynamic lighting incl. district-state changes, the Eclipse twilight | MegaLights production-ready in 5.8 |
| **PCG framework** | Biome dressing, wreck fields, factory interiors | Production-ready since 5.7 |
| **Gameplay Ability System (GAS)** | ALL abilities, weapons-fire effects, buffs, injuries, morale effects | Core architecture decision |
| **Behavior Trees + EQS** | Enemy & squad tactical AI | Mature |
| **StateTree** | NPC schedules/routines, mission phase logic | Modern replacement where BT is overkill |
| **Mass Entity/MassAI** | Crowds, background battle forces | For crowds only — bounded scope |
| **Enhanced Input** | All input, context stacks (foot/command/vehicle/map) | Standard |
| **CommonUI** | HUD + complex strategy/base UI stack | Standard for layered UI |
| **MetaHuman pipeline** | Companions + modular NPC faces | Audio-driven anim for scale |
| **Chaos Destruction** | Cover degradation, breach walls (authored, budgeted) | Scoped, not global |
| **Iris replication** | *Not used* (single-player) — disable | Cost avoidance |
| **Substrate** | Evaluate for hero materials only | Optional |

**Language split:** C++ for systems/frameworks/performance-critical (subsystems, GAS attributes, save, simulator, AI coordinators); Blueprints for content glue, mission scripts, UI logic; **all balance data in DataAssets/DataTables** (designer/AI-agent editable without code). Target split ~40% C++ / 60% BP-and-data.

---

## 12.2 Project Structure

```
/Eclipse
  /Source/Eclipse
    /Core        (game instance, save, subsystem bases, event bus)
    /Characters  (player, soldier, NPC classes + components)
    /AbilitySystem (GAS: attributes, abilities, effects, cues)
    /Combat      (weapons, damage, cover, projectiles)
    /AI          (BT nodes, EQS, coordinators, perception ext.)
    /Squad       (roster, orders, traits, bonds)
    /Strategy    (campaign state, map graph, DominionAI, BattleSimulator)
    /Economy     (resources, production, trade)
    /Base        (facilities, construction, staffing)
    /Quests      (quest runtime, mission generator, objectives)
    /Dialogue    (dialogue runtime, condition system)
    /Vehicles    (ground/air rigs, ship command layer)
    /UI          (CommonUI widgets' C++ bases)
  /Content
    /Core /Characters /Weapons /Planets/[PlanetName]/... 
    /Missions /Data (all DataAssets) /UI /Audio /VFX /Cinematics
  /Plugins (in-house: EclipseSaveSystem, EclipseDialogue, EclipseSimulator)
```

**Architecture spine — five rules:**
1. **Subsystems as service layer:** each major system = a `UGameInstanceSubsystem`/`UWorldSubsystem` (CampaignSubsystem, EconomySubsystem, RosterSubsystem, QuestSubsystem, DominionAISubsystem...). No god-objects.
2. **Event bus, not references:** systems communicate via a central gameplay event bus (typed GameplayTags payloads). Missions never call Economy directly; they broadcast `Event.Mission.FacilityCaptured` — Part 4's interconnection arrows are *literally* event subscriptions. This is what keeps 12 systems buildable by a small team.
3. **Data-driven everything:** soldiers, traits, techs, facilities, units, mission templates, region yields = DataAssets validated on load (a `ValidateData` commandlet runs in CI).
4. **The campaign is one struct tree:** the entire strategic state (`FCampaignState`) is serializable, human-readable in debug export (JSON), and is the *single source of truth*; ground missions read from and write back into it through a transaction API (prevents save divergence — the classic hybrid-game killer bug).
5. **Simulation/presentation split:** the BattleSimulator and Mission Generator are pure-logic C++ (no engine actor dependencies) — unit-testable headless, usable by both auto-resolve and hybrid battles.

## 12.3 Key System Implementations (per requested checklist)

- **Character system:** one `AEclipseCharacter` for player/soldiers/enemies, composed via components (Health/Cover/Perception/Loadout) + GAS AttributeSets (Health, Stamina, Armor, Suppression, Morale). Player-specific = a controller + input contexts, not a divergent class (soldiers must be player-quality — squad AI drives the same body).
- **Inventory:** lightweight ID+stack model on a component; equipment as GAS-granted ability sets from Weapon/Armor DataAssets. No physical-grid inventory (scope + fantasy: soldiers, not loot goblins).
- **Save system:** custom plugin — versioned SaveGame with per-subsystem serialization contracts; roster + campaign + world Data Layer states + memorial history; autosave at strategic transitions, checkpoint saves in missions; save-migration tests in CI (long-campaign game = save corruption is a project-killer; invest early).
- **AI framework:** shared `UEclipseAIComponent` (perception + knowledge tokens); BT assets per archetype; `USquadCoordinator` (both factions); DominionAI = utility planner ticking on strategic time only.
- **Quest system:** quest = DataAsset graph of Objectives (StateTree-driven phases) + Conditions + Consequences (event-bus emissions); the Mission Generator composes the same objective primitives the hand-authored missions use — one runtime, three authorship tiers (Part 11).
- **Dialogue system:** node-graph asset (in-house light editor or licensed plugin decision at prototype gate) + condition queries against CampaignState/personality axes; barks = tag-queried line pools (soldier persona × event), stitched with per-planet accent VO sets; TTS placeholder pipeline during development, recorded VO for companions/story at production.
- **Economy system:** pure C++ ledger in EconomySubsystem; per-day tick on strategic clock; all yields/costs from DataTables; deterministic given state (replayable in tests).
- **Building system:** slot-graph per base level asset; facility states as Data Layers + level instances swapped per upgrade; construction timers on strategic clock; siege missions load the base level with current facility states applied.
- **Strategy map:** separate map/world (galaxy UI world) reading CampaignState; lane graph as DataAsset; travel/supply/AI plans visualized; transitions ground↔map via seamless-ish loading (World Partition streaming out + map world in).
- **Vehicle system:** Chaos Vehicles for ground; air/dropships as spline-assisted flight (not full sim); ships never player-piloted (command layer only) — biggest deliberate scope save in the project.

## 12.4 Performance Budgets & Targets

Min spec: RTX 2060/RX 5700, 16 GB, NVMe. Targets: 60 fps at 1440p high on RTX 3070-class (Lumen), 60 fps low-spec via scalability tiers.
Budgets: ≤40 full-fidelity AI agents in combat bubble (+Mass crowds ≤400 statistical), ≤2 ms game thread for strategic tick (async where possible), streaming budget per planet cell 1.5 GB, hybrid-battle front-swap load ≤10 s.

---

*Next document: [13_roadmap.md](13_roadmap.md).*
