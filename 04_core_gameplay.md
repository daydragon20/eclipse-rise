# PART 4 — CORE GAMEPLAY SYSTEMS
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 04 of 14*

---

## 4.1 PLAYER SYSTEM

### 4.1.1 Movement

Third-person, cover-centric, weighty-but-responsive (reference feel: The Last of Us Part II traversal readability + Gears cover snappiness, lighter than both).

| Mechanic | Spec | Why |
|---|---|---|
| Walk / Run / Sprint | 1.8 / 4.2 / 6.5 m/s; sprint drains stamina only when armored above Medium | Readable pacing tiers; armor as a real tradeoff |
| Crouch / Prone | Crouch = stealth default; prone only in Sniper/Scout gameplay zones (tall grass, ridgelines) | Prone everywhere causes level-design pain; scoped use keeps it special |
| Cover | Contextual soft-attach (no sticky snap), corner lean, cover-to-cover dash (8 m max) | Fluidity over commitment; dash creates tactical rhythm |
| Vault / Mantle / Climb | Up to 2.4 m mantle; ladders; authored climb routes | Verticality without full parkour scope |
| Dodge | Directional combat roll, 0.5 s, i-frames only vs. melee | Guns should not be dodgeable; melee needs an answer |
| Sprint-slide | Into cover or through low gaps | Signature kinetic verb; feels like a soldier, not an acrobat |
| Environmental hazards | Heat (Tarsis), cold (Vel'Naar), vacuum/low-g (Krad-9) modify stamina, health regen, jump rules | Planets must *play* different, not just look different (Pillar: worlds as pressure) |

**Camera:** over-shoulder, swappable shoulder, FOV 90 default; pulls back to **520 cm boom (+73% van de 300 cm basis)** when commanding squad (command mode), forward on aim. *(Owner-beslissing 26-07: de eerdere ~15% is nooit gespeeld — de camera-blend liep tot 25-07 helemaal niet, dus die 15% is nooit door iemand beoordeeld. De 520 is bewust geauthord om het commando-veld te kunnen lezen en is wél gespeeld. De waarde staat in `DA_CharacterTuning::CommandModeArmLength`; dat is sinds vandaag de enige plek.)*

### 4.1.2 Combat (player-level summary; full design in Part 8)

Hybrid of aimed third-person shooting + tactical command layer. Time-dilated **Command Mode** (30% game speed, not full pause on default difficulty; full pause on Tactician difficulty) for issuing squad orders. The player is powerful but never an army: engaging 5+ enemies without squad or preparation should be lethal at all stages. Power growth = options and resources, not bullet-sponging.

### 4.1.3 Weapons

Weapon families (full stat tables in Part 8): Sidearms, SMGs, Assault Rifles, Marksman/Sniper, Shotguns, LMGs, Launchers, Thrown/Deployables, plus Energy variants unlocked via research (Part 10). Weapons use a **platform + modification** model: ~14 base platforms, each with 5 mod slots (optic, barrel, magazine, underbarrel, core). Mods are crafted/looted; platforms are manufactured. Rationale: bounded art scope, deep customization, economy integration (your factories make your guns — Part 6).

**Ammo philosophy:** scarce in Act 1 (drama), industrialized later (logistics replaces scavenging as the limiting system — scarcity moves up the chain as the player rises, mirroring the fantasy).

### 4.1.4 Armor

Three weight classes worn as **Suit + Plate + Helmet** slots:

| Class | Move penalty | Protection | Special |
|---|---|---|---|
| Light | none | low | +stealth profile, +stamina |
| Medium | −5% sprint | med | balanced; most mod slots |
| Heavy | −15% sprint, no slide | high | shoulder-weapon mount (Heavy class synergy) |

Armor uses **plate durability** (plates crack and are replaced at base/supply crates) rather than regenerating shields — reinforcing logistics fantasy. Health: segmented bar; last segment regenerates, others need medkits/Medic.

### 4.1.5 Abilities

Player abilities come from **Field Protocols** — equippable technique/gadget hybrids (max 3 equipped + 1 Signature). No magic; everything is fiction-grounded tech or leadership.

Examples by tree (full trees in Part 10 integration):

- **Operator tree:** Overwatch Mark (tag-and-track targets through walls briefly — drone-fed), Breach Charge, Adrenal Surge (short self-heal + stagger immunity).
- **Ghost tree:** Optic Veil (3 s near-cloak, breaks on fire), Echo Decoy (holographic sound lure), Silent Step.
- **Commander tree (unlocks with rank):** Rally (squad morale surge + minor heal), Focus Fire (squad-wide damage bonus on marked target), Reinforce (call reserve fireteam — cooldown gated by base Comms level: base building buffs the hero, closing the loop).
- **Signature abilities** (1 equipped; earned at story milestones): e.g., **Eclipse Protocol** (Act 3+: brief local blackout — lights, drones, optics die for 8 s; stealth reset or breach window).

### 4.1.6 Skills & Level Progression

- **Player XP** from missions, objectives, and (largest share) *campaign firsts* (first liberation, first alliance...) — rewarding progress over grinding.
- **Level cap 40.** Each level: +1 skill point into three trees (Operator / Ghost / Commander). Levels raise *options*, not raw damage: damage growth comes from gear/research (keeps late enemies dangerous without HP inflation).
- **Rank (separate from level):** Fighter → Squad Leader → Commander → General → Concord-General. Rank is story/campaign-gated, unlocks command features (squad size 2→4→8 [two fireteams]→battle command→fleet command). Level = personal capability; Rank = scope of command. Two progression bars = two fantasies (soldier & leader) advancing in parallel.

---

## 4.2 SQUAD SYSTEM

The squad system is the emotional core (Pillar 3: People, Not Units).

### 4.2.1 Recruiting Soldiers

Sources, in escalating scale:

1. **Hand recruits (Act 1+):** individually met in missions/settlements; highest starting loyalty; personal intro scenes.
2. **Liberation waves (Act 2+):** freeing conscripts/camps/prisons yields recruit pools; player screens candidates in the base **Muster** UI (stats, traits, background one-liners — every recruit has a two-sentence procedurally assembled history seeded from their origin planet).
3. **Faction levies (Act 3+):** allied factions contribute troops with faction-flavored traits (Krad-9: +explosives, stubborn; Shroud: +piloting, mercenary).
4. **Elite prison breaks (Vel'Naar):** named, hand-authored recruits with unique perks.

**Roster caps by Barracks level (L1–L3):** 12 → 24 → 48. Beyond 48, manpower scales through forward bases and army-unit aggregation (Part 7) up to ~200 tracked individuals — above squad scale, soldiers aggregate into companies, with individuals remaining trackable as officers.

### 4.2.2 Soldier Personalities

Each soldier has:

- **3 Traits** from a pool of ~60 (e.g., *Steady Hands* +accuracy under suppression; *Grudge: Veil* +damage vs. Veil, −stealth discipline near them; *Claustrophobic* −aim in tunnels; *Lucky* one cheat-death per campaign). Traits are visible, mechanical, and drive barks/dialogue selection.
- **Bond system:** soldiers who deploy together build Bonds (fireteam buffs; grief penalties on partner death — XCOM-proven attachment engine, extended with base-life scenes: bonded pairs appear together in hideout idle vignettes).
- **Voice/bark matrix:** 8 personality archetypes × planet accent sets. Procedural soldiers feel authored through combinatorial barks referencing traits, bonds, and campaign events ("Third siege this month, sir. We held the other two.").

### 4.2.3 Classes

Nine classes. Recruits arrive as **Recruit** (classless); classes assigned at the Training Academy (Part 5) gated by aptitude stats.

| Class | Battlefield role | Signature kit | Unique mechanic |
|---|---|---|---|
| **Assault** | Point, close engagement | AR/shotgun, breach charges | *Momentum*: buffs after closing distance |
| **Heavy** | Suppression, anti-armor | LMG/launcher, deployable shield | *Suppressive field*: cone that pins AI |
| **Medic** | Sustain, revival | SMG, trauma kit, stim drones | *Stabilize*: converts deaths to Critical Injuries within 30 s window |
| **Engineer** | Deployables, tech denial | Carbine, turret, EMP, repair | Builds field cover; hacks doors/drones |
| **Sniper** | Long range, overwatch | Marksman/sniper, spotter optic | *Killzone*: lane denial in command mode |
| **Scout** | Recon, flanks | SMG/silenced, sensor darts | Reveals map intel pre-mission (links to Part 11 mission-prep) |
| **Pilot** | Vehicles/air/space | Sidearm; vehicle mastery | Crews vehicles, gunships, later ship wings |
| **Commander** (officer track) | Force multiplier | Player-like command auras | Leads AI-resolved battles (delegation system — Part 7) |
| **Drone Specialist** | Unmanned warfare | Control deck, drone family | Micro-managed drone as second unit |

**Class design rule:** every class must have (a) a combat identity, (b) a strategic-layer use, (c) a base-life presence. E.g., Engineers accelerate construction between missions; Scouts generate passive intel; Medics reduce base-wide injury recovery time.

### 4.2.4 Experience & Ranks

Soldier ranks: Recruit → Regular → Veteran → Elite → Legend (per-class perk choice at each rank: A/B pick, XCOM-style meaningful builds). XP from deployment, kills/assists, objectives, and class-action bonuses (Medics from heals, not kills). **Legend soldiers** (max 6 alive at once) get a unique cosmetic, a bark set upgrade, and one Signature perk — campaign heroes the player did not script but will remember.

### 4.2.5 Injuries

| Result | Cause | Effect |
|---|---|---|
| **Grazed** | Downed but stabilized fast | 1 mission rest |
| **Wounded** | Standard down | Out (real-time strategic days, reduced by Medbay level + Medics) |
| **Critical Injury** | Bleed-out reached but Medic-stabilized | Permanent trait roll: e.g., *Shrapnel Lung* (−stamina) or *Iron Scar* (+will). Prosthetics research (Part 10) can later remove negatives — medicine as hope |
| **Death** | Bleed-out, no stabilization / catastrophic hit | Permanent. Memorial Wall entry; gear partially recoverable if body extracted (risk/reward: leave no one behind as *mechanic*) |

### 4.2.6 Morale & Loyalty

Two distinct stats:

- **Morale (squad-level, volatile):** moved by victories, losses, food quality, base amenities, leave rotation. Low morale → accuracy/discipline penalties, desertion events at worst. High → bonus will, extra bark warmth.
- **Loyalty (individual, slow):** moved by player choices matching the soldier's values (traits define what they care about), loyalty scenes, promotions, rescuing their people. High loyalty unlocks personal side-content; low loyalty risks informants (ties to Veil infiltration system, Part 9).

**Design intent:** Morale is logistics; Loyalty is leadership. The player manages one with resources and the other with decisions.

### 4.2.7 Permanent Death

- Default ON for regular soldiers at all difficulties; Story difficulty converts deaths to Critical Injuries **except** in authored high-stakes finales (flagged in fiction as "no-extraction ops") — protecting the theme even for story players.
- Companions (the 8) die only at authored branch points — never systemically. Rationale: systemic companion death deletes recorded content unpredictably; authored death preserves both stakes and production sanity.
- **Memorial Wall:** every death is recorded with name, planet, missions served, and cause. Visiting it is optional; the game never forces grief, only offers it. Some soldiers leave **Letters** (procedural + template) unlocked on death.

---

## 4.3 SYSTEM INTERCONNECTION MAP (Part 4 scope)

```
 Soldier recruited (Ground/Story) ──▶ Trained (Base: Academy) ──▶ Classed & equipped (Economy: gear)
        │                                                            │
        ▼                                                            ▼
 Bonds & traits (Squad)  ◀──── deploys with player ────▶  Mission outcomes (Part 11)
        │                                                            │
        ▼                                                            ▼
 Morale/Loyalty (Base life, choices)                    XP, injuries, deaths (Medbay, Wall)
        │                                                            │
        └────────────▶ Officer promotion ──▶ Army command (Part 7) ◀─┘
```

Every arrow above is a hard design requirement: if a feature breaks one of these arrows, the feature is wrong (Pillar 2).

---

*Next document: [05_base_building.md](05_base_building.md).*
