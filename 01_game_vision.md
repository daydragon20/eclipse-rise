# PART 1 — GAME VISION DOCUMENT
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 01 of 14 | Confidential — Internal Development Use*

---

## 1.1 Game Identity

| Field | Value |
|---|---|
| **Title** | ECLIPSE: RISE OF THE RESISTANCE |
| **Genre** | Single-player third-person Action-Strategy RPG |
| **Platform** | PC (Windows), Steam / Epic Games Store |
| **Engine** | Unreal Engine 5.7+ |
| **Perspective** | Third-person (ground gameplay) + Strategic galaxy map (campaign layer) |
| **Rating target** | PEGI 16 / ESRB M (violence, war themes, moral choice) |
| **Session length** | 30–90 min core loop; 100–300 h total campaign |
| **Team assumption** | Small core team (3–8 people) + AI-assisted development |
| **One-line pitch** | *Start as a nobody under a galactic empire. End as the leader of the greatest rebellion in history.* |

**Elevator pitch (30 seconds):**
ECLIPSE is a single-player sci-fi epic where a nameless citizen becomes the general of a galaxy-wide rebellion. You fight on the ground in tactical third-person combat, command squads whose members can permanently die, build hidden bases into a military civilization, research technology, manage an economy, and conquer a strategic galaxy map — all woven into a Mass Effect-caliber story about the price of freedom. Every layer feeds every other layer: the soldier you save on the ground becomes the veteran who wins the space battle three acts later.

**Design pillars (every feature must serve at least one):**

1. **THE CLIMB** — Tangible progression from powerless individual to galactic power. Scale must be *felt*, not told.
2. **EVERYTHING IS CONNECTED** — Ground combat, base building, economy, story, and the strategy map form one machine. No isolated minigames.
3. **PEOPLE, NOT UNITS** — Soldiers have names, faces, histories, and permadeath. Losing someone must hurt.
4. **EARNED VICTORY** — The Dominion cannot be beaten by one hero. Victory is built from infrastructure, alliances, and sacrifice.
5. **ACHIEVABLE AMBITION** — Every system is scoped so a small AI-assisted team can actually ship it (see Part 13).

---

## 1.2 Player Fantasy

The core fantasy is **becoming**. Not *being* powerful — *becoming* powerful, step by visible step.

**The Ladder of Becoming:**

| Stage | Player identity | What the player controls | Approx. hours |
|---|---|---|---|
| 1 | Unknown civilian | One character, no weapons | 0–2 |
| 2 | Resistance fighter | Self + borrowed gear | 2–10 |
| 3 | Squad leader | 4-person squad | 10–30 |
| 4 | Commander | Multiple squads, one base | 30–60 |
| 5 | General | Army, several bases, one liberated planet | 60–110 |
| 6 | Rebellion leader | Multi-planet coalition, fleet | 110–170 |
| 7 | The one who ends the Dominion | A civilization at war | 170–250+ |

Design rule: **every stage must make the previous stage feel small.** When the player commands their first squad, the solo years should feel distant. When they order a fleet into orbit, they should remember hiding in a cargo container in hour one. We deliberately re-visit early locations late in the game (see Part 11, "Return missions") to make the contrast explicit.

**Fantasy verbs by layer:**

- Ground layer: *fight, sneak, lead, rescue, sabotage*
- Base layer: *build, upgrade, research, train, plan*
- Strategy layer: *expand, liberate, ally, defend, outmaneuver*
- Narrative layer: *decide, sacrifice, inspire, remember*

---

## 1.3 Target Audience

**Primary audience — "The System Strategist" (25–45):**
Players of XCOM 2, Mount & Blade II: Bannerlord, Ghost Recon Wildlands, Mass Effect, BattleTech, Jagged Alliance 3. They want deep systems, meaningful consequences, long campaigns, and stories that respect their intelligence. PC-first, tolerate complexity, love spreadsheet-adjacent depth *if* the moment-to-moment play is kinetic.

**Secondary audience — "The Story Soldier" (18–35):**
Players of Mass Effect, Star Wars Jedi series, Cyberpunk 2077. Come for the narrative and characters, stay because the strategy layer gives their story weight. For them we provide difficulty options that soften (never remove) the strategic layer: advisor auto-suggestions, optional permadeath ("Iron Will" mode is opt-in at higher difficulties, injuries-only at Story difficulty).

**Market positioning:**
There is a persistent gap in the market: no game combines *third-person action* with *X4/XCOM-style strategic conquest* in a *narrative single-player* package. Mount & Blade proves the "nobody to ruler" loop sells for hundreds of hours but lacks story and sci-fi. XCOM proves squad permadeath creates attachment but lacks direct action control. ECLIPSE sits exactly at that intersection.

**Anti-audience (deliberately not served):** competitive multiplayer players, players who want < 20 h campaigns, players allergic to any resource management. We do not compromise the pillars to chase them.

---

## 1.4 What Makes ECLIPSE Unique

1. **The Persistence Chain.** A single soldier can be recruited in a ground mission, trained at your academy, promoted through five ranks, injured at the battle of Vel'Naar, retrained as a pilot, and die defending your flagship in the finale — with the game tracking and referencing all of it. No other action game persists individuals across action, base, and strategy layers.

2. **One War, Three Altitudes.** The same war is playable at three altitudes — boots (third-person), base (management), and map (strategy) — and actions at each altitude visibly change the others. Destroy a fuel depot on foot → the Dominion garrison on the strategy map loses armor support for 10 days → your next liberation battle spawns without enemy tanks.

3. **The Dominion Reacts Like an Empire.** The enemy is not a static difficulty curve. The Dominion Response System (Part 9) escalates: ignores you, polices you, hunts you, besieges you, and finally fears you. Player notoriety changes patrol density, propaganda broadcasts, NPC dialogue, and which enemy units deploy.

4. **Permadeath With a Face.** Squadmates have procedurally assigned personalities, relationships, and memorial walls. XCOM's grief, but you were *standing next to them* when it happened.

5. **Original universe, grounded tone.** No chosen ones, no space magic. Power comes from logistics, people, and technology. The tone is *The Expanse* meets *Andor*: hard, human, hopeful.

---

## 1.5 Emotional Experience

**Emotional arc across the campaign (target curve):**

| Act | Dominant emotion | Secondary | Design levers |
|---|---|---|---|
| Prologue | Oppression, smallness | Fear | Scripted powerlessness, scale of Dominion architecture, curfew ambience |
| Act 1 | Defiance, fragility | Grief (first losses) | Scarce ammo, hunted stealth, first permadeath |
| Act 2 | Momentum, pride | Anxiety (retaliation) | First base upgrades, first liberated settlement, Dominion crackdowns |
| Act 3 | Power, responsibility | Doubt (moral cost) | Army command, coalition politics, civilian-cost choices |
| Act 4 | Resolve, catharsis | Loss | Final sacrifices, callbacks to Act 1, liberation of the capital |

**Emotional design rules:**

- **Losses are authored to matter.** Every permadeath triggers: squad dialogue at base, a memorial entry, and (for named characters) story ripples.
- **Hope is rationed.** Act 1 gives small wins (a family saved, a district's lights restored). The game never becomes grimdark — the *point* is that resistance works.
- **Scale contrast moments** every 8–10 hours: an authored beat that juxtaposes where you were with where you are (see gameplay moments below).

---

## 1.6 Core Gameplay Loop

**Macro loop (the campaign engine):**

```
STRATEGY MAP                    BASE                        GROUND
Choose objective  ──────▶  Prepare (train, equip,  ──▶  Execute mission
(liberate, raid,           research, assign squad)       (third-person action)
 defend, ally)                                                 │
      ▲                                                        ▼
      │                                                   Consequences
      └──────  Rewards + world-state changes  ◀───  (resources, intel, recruits,
               (territory, economy, threat)          casualties, story beats)
```

**Loop timings:**

- **Micro loop (30 s–2 min):** combat encounter → tactical decision → reward/consequence.
- **Mission loop (20–45 min):** briefing → insertion → objectives → extraction → debrief.
- **Base loop (5–15 min):** spend resources → assign research/training → manage roster → talk to characters.
- **Strategic loop (1–3 h):** campaign decision → 2–4 missions → territory/story shift → new strategic options.
- **Act loop (15–40 h):** power tier unlocked → new systems introduced → Dominion escalates → act climax.

**Why the loop retains players for hundreds of hours:**

1. **Layer interleaving prevents fatigue.** When combat tires you, base management is a palate cleanser, and vice versa. (Proven by X-COM, Bannerlord retention patterns.)
2. **Always three horizons.** At any moment the player has a now-goal (this mission), a mid-goal (this planet), and a dream-goal (the Dominion). Classic Civilization "one more turn" structure.
3. **Roster attachment compounds.** Every hour invested in soldiers raises the stakes of every future mission.
4. **Procedural mission generation** (Part 11) + hand-authored story missions means the strategic layer never runs out of meaningful actions.
5. **New Game Plus / Campaign variability:** starting planet choice, cell background, and coalition composition materially change a second campaign.

---

## 1.7 Signature Gameplay Moments (authored examples)

**Moment 1 — "The Ration Line" (Prologue, hour 0.5)**
The player queues for protein rations in Kessara's Foundry District. A Dominion Enforcer drags a neighbor out of line for an expired work permit. The player can intervene (beaten, scripted loss — teaches that solo defiance fails), comply, or memorize the Enforcer's patrol badge (intel seed used in Act 1). No prompt tells them the third option exists. *Purpose: teach oppression through play; plant the intel mechanic.*

**Moment 2 — "Thirteen Bullets" (Act 1, hour ~6)**
First real firefight. The cell has exactly 13 rounds between four people. UI shows shared ammo. The mission is winnable through positioning, one grenade trap, and two melee takedowns — not marksmanship. *Purpose: scarcity as drama; combat as problem-solving.*

**Moment 3 — "First Name on the Wall" (Act 1 climax)**
The raid on Blacksite K-77 succeeds, but scripted-systemic design means at least one recruited (non-plot) soldier likely dies. Returning to the hideout, the survivors have welded a bunk plate to the wall and etched the name. The memorial wall UI is born. *Purpose: permadeath becomes ritual; base becomes home.*

**Moment 4 — "The Lights of Varga District" (Act 2, hour ~35)**
After the player restores power to a liberated district, they can walk it at night: windows lit, curfew sirens silent, a child chalk-drawing the Eclipse sigil. An NPC the player saved in the Prologue recognizes them. *Purpose: show what winning is for.*

**Moment 5 — "Two Fronts" (Act 3, hour ~80)**
The Dominion attacks two allied planets simultaneously. The player can only reinforce one in person; the other must be trusted to a lieutenant they promoted (AI-commanded battle resolved by that officer's stats and the garrison the player built). *Purpose: delegation as gameplay; earlier investment pays or fails.*

**Moment 6 — "The Container" (Act 4, hour ~120)**
En route to the final assault, the player's flagship passes a captured Dominion freighter — the same model cargo container the player hid in during the Prologue escape. Optional interaction: step inside. Sixty seconds of silence, then their general's coat catches on the same broken latch. *Purpose: the scale-contrast payoff of the entire fantasy.*

**Moment 7 — "Eclipse" (Finale)**
The combined rebel fleet moves into position above Aurelion, physically eclipsing the orbital sun-mirror that lights the Dominion capital — the literal fulfillment of the rebellion's name and sigil. Ground assault begins in the artificial twilight. *Purpose: title moment; every layer (fleet you built, army you trained, allies you won) is on screen at once.*

---

## 1.8 Scope Honesty (Vision-level)

To keep the vision achievable (Rule 6), the following are **explicit non-goals**:

- No multiplayer, no co-op, no live service.
- No seamless space-to-ground flight. Space battles are a commanded tactical layer (Part 7), not a flight sim.
- 10 planets, each a curated 2–6 km² playspace using World Partition — not full-planet open worlds.
- Fleet combat is auto-resolved-with-input (card/order-based tactical resolution with 3D presentation), not a real-time space RTS.
- Facial animation via MetaHuman pipeline + audio-driven animation; no bespoke performance capture beyond key scenes.

Full scoping logic in Part 13.

---

*Next document: [02_story_bible.md](02_story_bible.md) — the complete narrative universe.*
