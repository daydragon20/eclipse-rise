# Eclipse runtime module — planned layout (GDD 12.2)

Subfolders are created when their first system lands (empty dirs don't survive version control). Planned structure, fixed by the TDD:

```
/Core          game instance, save, subsystem bases, event bus        (Phase 1: SPEC-P1-01/02)
/Characters    player, soldier, NPC classes + components              (Phase 1: SPEC-P1-05/06)
/AbilitySystem GAS: attributes, abilities, effects, cues              (Phase 1: health/damage attributes)
/Combat        weapons, damage, cover, projectiles                    (Phase 1: minimal hitscan)
/AI            BT nodes, EQS, coordinators, perception ext.           (Phase 1: SPEC-P1-06)
/Squad         roster, orders, traits, bonds                          (Phase 1: SPEC-P1-06/07)
/Strategy      campaign state, map graph, DominionAI, BattleSimulator (Phase 1: SPEC-P1-02/04; DominionAI/Simulator later)
/Economy       resources, production, trade                           (Phase 1: SPEC-P1-03)
/Base          facilities, construction, staffing                     (Phase 2+; Phase 1 base is menu-only UI)
/Quests        quest runtime, mission generator, objectives           (Phase 1: SPEC-P1-05; generator Phase 3)
/Dialogue      dialogue runtime, condition system                     (Phase 2; see phase0/dialogue_plugin_decision.md)
/Vehicles      ground/air rigs, ship command layer                    (Phase 3+)
/UI            CommonUI widgets' C++ bases                            (Phase 1: SPEC-P1-04/08 screens)
```

Architecture constitution: GDD 12.2 (five rules) + 14.3. Planned in-house plugins: EclipseSaveSystem, EclipseDialogue, EclipseSimulator.
