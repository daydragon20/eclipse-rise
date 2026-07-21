# PART 9 — AI SYSTEM DESIGN
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 09 of 14*

---

## 9.1 AI Architecture Overview (design intent; implementation in Part 12)

Three AI families share one perception/navigation foundation but serve different masters:

| Family | Serves | Tech (UE5) | Core promise to the player |
|---|---|---|---|
| **NPC/Civilian AI** | The world's believability | StateTree + Mass (crowds) + schedule data | "This world lives whether I watch or not" |
| **Enemy AI** | Challenge & fairness | Behavior Trees + EQS + squad coordinator | "They are smart, and I can learn them" |
| **Squad AI** | The player's trust | BT + command layer + the player's orders | "My people are competent" |
| **Strategic AI (The Dominion)** | The campaign's drama | Utility AI on the strategy layer (no BT) | "The empire reacts like an empire" |

**Perception model (shared):** sight cones with light-level & foliage modifiers, hearing events with material occlusion, and *knowledge tokens* — AI acts on what it knows (last-known-position ghosts), never on server-truth. Fairness rule: any detection must be traceable to a sight/sound event the player could have predicted.

---

## 9.2 NPC AI (civilians, base population)

- **Daily routines:** schedule-driven (home → work → ration line → curfew), data-defined per settlement; Mass framework LOD: full agents near player, statistical crowds far. Curfews, checkpoints, and raids visibly disrupt routines — oppression is *shown as broken schedules*.
- **Relationships & memory:** lightweight per-NPC memory record (faction attitude, notable player events witnessed, gratitude/fear flags). Saved NPCs remember (bounded: max 3 memory slots per NPC, oldest non-pinned decays — scope honesty). District-level **mood** aggregates NPC states → feeds unrest/recruitment rates (Part 6/11): crowds are a readable political weather system.
- **Reactions:** witness matrix per event type (gunfire, arrest, rebel graffiti, player recognized): flee/report/watch/help, weighted by district mood + Dominion presence. Informant risk: some civilians report the player (Veil infiltration pressure) — countered by Intel Center L3.
- **Base NPCs:** roster soldiers use the same schedule system inside bases (train, eat, mourn, celebrate) — the "living base" is the same tech as the living city (build once, use twice).

## 9.3 Enemy AI (tactical)

**Individual layer (BT):** combat roles per archetype (Enforcer, Trooper, Shock, Marksman, Drone Op, Heavy, Radiant Guard elite) — each with distinct silhouettes, telegraphs, and counterplay (design rule: every archetype has a *readable tell* and a *preferred mistake* the player can exploit).

**Squad layer (coordinator):** enemy squads of 4–8 share a blackboard: assign suppress/flank/hold roles, use bounding movement, throw grenades to evict, retreat when broken (morale model mirrors the player's — enemies rout, surrender, drag wounded; killing routing enemies has moral-system weight).

**Patrols & alert states:** patrol routes data-authored + jitter; alert escalation per Part 8.5, with *site-wide* alarm states (lockdowns, QRF timers, searchlights) — infiltration sites are systemic clockworks, not scripted corridors.

**Adaptation (bounded, honest):** per-mission, the coordinator counters observed player patterns from a small playbook (player snipes → smoke + flank; player turtles → grenadiers; heavy drone use → jammers). Across the campaign, the **Dominion doctrine track** unlocks counter-equipment based on the player's aggregate style (fictionalized as Kaine studying "Cinder"). Bounded playbooks — not ML — for testability (Rule 5: avoid impossible features).

**Command hierarchy:** officers on the field are real force multipliers (buff discipline, enable combos); killing the officer degrades the squad to individual-layer AI — decapitation as a learnable tactic, mirroring the game's own officer system. Consistency of rules across factions is a pillar-level requirement.

## 9.4 Strategic AI — The Dominion Response System

Utility-based planner running on the campaign layer, evaluating: player threat score, territory, economy targets, and doctrine. **Response Tiers:**

| Tier | Name | Trigger (threat score) | Behavior |
|---|---|---|---|
| 0 | Indifference | pre-story | Police only |
| 1 | Nuisance | Act 1 | Veil investigations, checkpoints |
| 2 | Insurgency | regional attacks | Garrison reinforcement, patrol density, bounties |
| 3 | Rebellion | first liberation | Military occupation, economic blockades, named counter-operations (authored+systemic hybrids) |
| 4 | War | multi-planet | Kaine offensives: real army/fleet movements under the same supply rules as the player |
| 5 | Existential | Act 4 | Total mobilization, AEGIS protocol, scorched earth |

Tier changes are **broadcast diegetically** (propaganda, patrol chatter, NPC dialogue) — the player always learns the empire's temperature by living in it, not from a menu.

## 9.5 Squad AI (the player's people)

Priority stack: (1) player orders, (2) self-preservation thresholds, (3) role behavior, (4) bond behaviors (cover a downed bond-partner). Key behaviors: intelligent cover selection near ordered positions (same scoring as enemies), automatic corner-cover-lean fire, stealth discipline (match player stance, hold fire until ordered/compromised), medic auto-triage toggles, and **verbal transparency** — refusals/failures are always spoken with reason. Squadmate "stupidity" destroys the core fantasy; the squad AI's bug bar is the highest in the project (Part 14 testing standards).

**Trait/personality hooks:** traits modulate BT parameters (aggressive soldiers push further, cautious hold; a Grudge trait may fire a controllable disobedience *story event* only at authored moments — systemic flavor, authored risk).

---

*Next document: [10_technology_tree.md](10_technology_tree.md).*
