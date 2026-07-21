# PART 7 — MILITARY SYSTEM
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 07 of 14*

---

## 7.1 The Three Scales of War

| Scale | Player control | Resolution |
|---|---|---|
| **Squad (1–8 soldiers)** | Direct third-person + command mode | Fully simulated (Part 8) |
| **Battle (companies, vehicles, 1 theater)** | Player on the ground *inside* a larger battle; issues battle-orders to AI companies | Hybrid: player's zone fully simulated; adjacent fronts resolved by the Battle Simulator with player orders as inputs |
| **Theater/Fleet (planets, space)** | Orders on the strategy map; may embark in person for set-pieces | Battle Simulator (deterministic core + bounded randomness), presented as 3D tactical view |

**The Hybrid Battle is the signature military feature:** in large engagements (liberation assaults, sieges), the player fights a real firefight at one *front* while the battle UI shows other fronts progressing per the forces, officers, and plans the player prepared. The player can redeploy mid-battle (dropship hop between fronts — loads a new combat zone with the battle-state carried over). Mount & Blade's "you are in your army" feeling, at achievable scope: we simulate one zone at a time, *not* 500 concurrent actors.

---

## 7.2 Ground Forces

Units are formed at the Academy/Barracks from roster soldiers (squads) or from levies (companies — aggregated units with an officer, a class mix, equipment tier, XP tier, and traits inherited from origin).

| Unit | Composition | Role | Unlock |
|---|---|---|---|
| Infantry Company | 60 levies + officer | Line holding, garrisons | Act 2 |
| **Elite troops** (Vanguard Teams) | 8 hand-picked roster veterans | Spearheads, special ops (deployable as AI squads in player missions) | Act 2 |
| Recon Company | Scouts + light vehicles | Intel, screening, ambush | Act 2 |
| Armor Troop | 4 tanks + crews (Pilots) | Breakthrough | Act 3 |
| **Mech Lance** | 2 salvaged/produced walkers | Assault anchor, terror counter | Act 3 (research) |
| Artillery Battery | Guns + fire-control | The player can call its fire in hybrid battles | Act 3 |
| Special units | Named one-offs (e.g., Krad-9 Breachers, Sylvaris Ghostline) | Faction-flavored unique companies from alliances | Act 3 |

**Command system:** companies require **officers** (Commander-class soldiers). Officer stats (Tactics, Logistics, Inspiration — grown from their soldier career) directly parameterize the Battle Simulator. *Your XCOM-style soldier investment becomes your Total-War-style army quality* — the Persistence Chain at army scale.

## 7.3 Air Forces

| Unit | Role | Notes |
|---|---|---|
| Fighters (wings of 4) | Air superiority over battles; intercept raids | Presence flips air-support availability in hybrid battles |
| Bombers | Pre-battle strikes (soften a front; risk civilian cost — moral system hooks) | Player-called in battles at Fleet Command L1+ |
| Dropships | Insertion variety (Part 11 mission prep), mid-battle redeploy, medevac (reduces death→critical odds if LZ held) | The logistics workhorse; escorting/losing them matters |

## 7.4 Space Forces

| Class | Role | Fleet points | Notes |
|---|---|---|---|
| Strike fighters (wing) | Screens, bomber intercept | 1 | Crewed by Pilot-class soldiers (named pilots can die) |
| Corvette | Raiding, escort, smuggler-lane ops | 2 | First ship: the *Loyal Ghost* (Act 2, authored) |
| Frigate | Line workhorse | 4 | |
| Cruiser | Heavy line, orbital fire support (unlocks orbital strikes in ground battles — space and ground literally interlock) | 8 | |
| Battleship | Fleet anchor, siege of Gate Spires | 16 | Campaign projects; max ~3 per campaign |
| **Flagship *Dawnbreak*** | Mobile HQ (Act 3+) | — | Authored; upgradeable; the finale's stage |

**Fleet battles:** order-based tactical resolution presented in a 3D orbital view — the player sets formation, doctrine (aggressive/screen/withdraw), targets, and timing of special orders (fighter launch, focus fire, boarding action, retreat) in a pausable real-time presentation; outcomes computed by the same Battle Simulator with naval parameters. **Boarding actions** convert a space battle into a third-person mission on the enemy ship (the player's personal skill can flip a losing space battle — every layer can rescue every other layer).

## 7.5 Recruitment → Training → Upgrade Pipeline

```
Population pools (liberated regions) ─▶ Levy recruitment (C + political capital)
        │                                        │
Prison breaks / rescues ─▶ Named specialists     ▼
        │                              Training Academy (time + wages)
        ▼                                        │
   Roster soldiers ──▶ classes/ranks ──▶ Officer school ──▶ Company command
                                                 │
Factory/Shipyard output (M + time) ──▶ Equipment tiers per unit ──▶ Field upgrades (battle XP)
```

Units have **equipment tier** (bought) and **experience tier** (earned, lost with casualties) — rebuilding a destroyed veteran company costs more than money, which makes *preserving* units a strategic instinct and links every battle back to the economy and the memorial theme.

## 7.6 Battles on the Strategy Map

- **Auto-resolve with plans:** for battles the player skips, they choose a plan (Assault / Flank / Siege / Raid / Relief) + commit assets; the Simulator resolves with officer skill, unit tiers, terrain, garrison/forward-base ratings, and supply. Results are *always* explainable in the after-action report (each modifier itemized — trust through transparency; the anti-"gacha battle" rule).
- **Supply rule:** units out of supply range (Depots, lanes) fight at −25% and bleed readiness. Cutting supply is always a legitimate alternative to battle — the map rewards maneuver, not just mass.
- **The Dominion uses the same rules.** Kaine's counteroffensives obey supply, production, and officer logic — beatable by outthinking, not just outgrowing (Part 9.4).

---

*Next document: [08_combat_design.md](08_combat_design.md).*
