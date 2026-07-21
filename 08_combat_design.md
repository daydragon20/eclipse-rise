# PART 8 — COMBAT DESIGN
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 08 of 14*

---

## 8.1 Combat Vision — "How it feels"

**Feel statement:** *Combat should feel like being the smartest person in a deadly room — not the strongest.* Weighty, deliberate, readable. Bullets kill fast in both directions (player TTK vs. basic enemy: ~0.6 s well-aimed; enemy vs. exposed player: ~2.5 s). Survival comes from position, preparation, and people. The rhythm is: **read → plan → strike → adapt** — a 20–60 s cycle per encounter beat.

Reference blend: The Last of Us II (lethality, audio dread), Gears (cover language), Ghost Recon Wildlands (open-approach encounters, squad sync), XCOM (consequence weight, brought into real time).

**Difficulty philosophy:** difficulties change information and margins (enemy telegraphs, command-mode time dilation depth, resource pressure), never bullet-sponge scaling.

---

## 8.2 Weapons (feel + roles)

| Family | Role | Feel signature |
|---|---|---|
| Sidearms | Backup, silenced stealth | Snappy, intimate |
| SMGs | CQB, Light-armor synergy | Rattling, controllable spray |
| Assault rifles | The baseline | Punchy 3–5 round discipline |
| Marksman/Sniper | Overwatch, deletion | Held breath, heavy report, killcam earned at rank |
| Shotguns | Breaching, panic answer | Doors + bodies move |
| LMGs | Suppression (real mechanic: suppressed AI accuracy −70%, pinned behavior) | The room changes when it speaks |
| Launchers | Armor/structures | Scarce, campaign-level ammo |
| Deployables | Traps, cover, turrets | Preparation made visible |
| Energy tier (research) | Late-game variants: shield-piercing, no ballistic drop, heat instead of ammo | The economy's triumph, felt in the hands |

**Gunplay specs:** recoil patterns per platform (learnable), hybrid hitscan/projectile (projectile ≥ 50 m), locational damage (head ×2.5, limbs cripple), armor as damage-type interaction (kinetic vs. plate vs. shield vs. energy) — creating loadout-vs-enemy-composition strategy fed by pre-mission intel (Intel currency → known enemy comp → loadout choice: the fairness loop).

## 8.3 Cover System

- Soft-attach contextual cover (no button-glue), height states (low/high), destructible classes: soft (wood/sheet — degrades under fire, forcing movement), hard (plate/rock), reactive (explosive/collapsing — both a threat and a tool).
- **Cover is a conversation:** LMG suppression pins, grenades evict, marksmen punish leaning, Engineers build new cover. No safe forever-spot; encounters breathe.
- Squad AI and enemy AI use identical cover scoring (Part 9) — fairness the player can feel and learn.

## 8.4 Tactical & Squad Commands

**Command Mode** (hold ⌘key): time dilates to 30% (full pause on Tactician), camera lifts slightly, order interface appears.

| Order | Target | Notes |
|---|---|---|
| Move / Hold | Position | With stance (stealth/ready/aggressive) |
| Focus target | Enemy | Class-appropriate execution |
| Suppress | Area | Heavies excel |
| Flank | Enemy group | Squad computes route; player approves |
| Breach | Door/wall | Synchronized entry set-piece |
| Ability use | Class ability | E.g., Medic stabilize, Engineer turret |
| Sync strike | Up to 4 marked | Simultaneous silenced takedowns (stealth apex verb) |
| Regroup / Fall back | Player / rally point | Morale-safe withdrawal |

Orders are *promises the AI keeps* (Part 9.5): squadmates execute competently or verbally explain why they can't ("No route, boss — window's covered."). Command trust is the entire squad fantasy; an order silently failing is a critical bug by definition.

## 8.5 Stealth

Full parallel path for most missions (mandated in mission design: every non-siege mission has a stealth-viable route). Systems: light/shadow + noise + LOS perception model (Part 9), body management, camera/drone hacking (Engineer/Scout), distraction tools, hand-sign squad stealth orders (Kessara fiction integrated), detection meter with **Alert stages**: Unaware → Suspicious → Searching → Alarmed (reinforcements, lockdown) → Hunted. Partial recovery is always possible (return to Searching) — stealth failure bends into loud play rather than restart; "ghost" completion is a bonus objective (Intel + morale rewards), never a fail-state wall.

## 8.6 Large Battles (hybrid scale)

Player-experience goals: chaos with readable structure — front lines visible via tracer language, artillery arcs, comms chatter; the battle map (tab) shows fronts shifting in real time. Player levers inside battle: personal combat at the decisive point, battle-orders (commit reserves, shift artillery, call air/orbital per assets), and redeployment hops. Performance approach (Part 12): one fully simulated combat bubble (~40 active agents max) + Mass-crowd background forces + simulator-driven front state.

## 8.7 Boss Encounters

Bosses are **arena arguments**, not HP walls — each teaches/tests a system:

| Boss | Act | The argument |
|---|---|---|
| Enforcer-Captain Rhek (mini) | 1 | Armor vs. positioning: unkillable frontally; environment kills |
| The Warden of Vel'Naar | 2 | Stealth under pursuit: reversed hunter dynamic in a blizzard |
| Veil Inquisitor Threx | 3 | Intel warfare: he uses *your* squad's traits against you (calls their names, targets bonds — mechanical + narrative fusion) |
| Kaine's Command Walker | 3/4 | Combined arms: requires squad orders + artillery timing |
| The AEGIS Vault | 4 | Systems mastery gauntlet: no guns for one phase — drones, hacking, dark |
| Vex + Radiant Guard | 4 | The dialogue-boss finale (Part 2.10): combat phases interleaved with the argument you've been building all campaign |

## 8.8 Feedback & Juice standards

Hit confirmation hierarchy (hitmarker < armor spark < flesh reaction < kill punctuation), squad callouts as primary game-state audio, screen-space restraint (no HUD soup: diegetic-leaning UI, scalable), controller + M/KB parity, full remapping, and accessibility: aim assist tiers, high-contrast enemy outlines option, subtitle/speaker tags, colorblind-safe tracer palettes.

---

*Next document: [09_ai_systems.md](09_ai_systems.md).*
