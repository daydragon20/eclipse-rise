# PART 3 — WORLD DESIGN
**ECLIPSE: RISE OF THE RESISTANCE**
*Game Design Bible — Document 03 of 14*

---

## 3.1 The Galaxy Map (Strategy Layer)

The Vantara Expanse is presented as a **jump-lane graph**: 10 playable planets (nodes) + 14 non-landable strategic nodes (Gate Spires, stations, asteroid fields) connected by jump-lanes (edges). This is the core strategic board.

```
                         [AURELION] ← Capital (Act 4)
                          /       \
                 (Spire Gamma)   (Spire Delta)
                    /                  \
              [ELYSTRA]              [VORN]
                  |                     |
             [MERIDIA] ——— (Hub) ——— [SYLVARIS]
              /      \       |
      [THE SHROUD]  (Spire Beta)
           |             |
        [TARSIS] —— [KESSARA] ← Start
           |             |
        [KRAD-9] —— [VEL'NAAR]
```

**Map rules (all strategy mechanics derive from these):**

1. **Lanes only.** Fleets and armies move along lanes. No lane, no movement.
2. **Spires gate lanes.** A hostile Gate Spire blocks military transit (smuggler-tier movement still possible at cost/risk — Kaya's specialty).
3. **Control is per-region, not per-planet.** Each planet has 4–8 control regions; planetary liberation means flipping enough regions (Part 11).
4. **Distance = time = risk.** Supply lines are real: cut lanes strangle economies (Part 6).

**Strategic node types (non-landable):** Gate Spires (lane control), Relay Stations (intel range), Depot Stations (supply range extension), Asteroid Fields (mining ops + fleet ambush terrain), the Hub (neutral trade station, diplomacy scene hub).

---

## 3.2 Planet Design Overview

Each planet is a **World Partition open zone of 2–6 km²** representing its key theater — not a whole globe. Additional story missions use separate smaller levels within the planet's biome set. Each planet must deliver: a distinct visual identity readable in one screenshot, a distinct gameplay pressure, a distinct culture/faction, and a distinct strategic role. Ten planets, no two alike on any axis.

**Planet summary table:**

| # | Planet | Archetype | Gameplay pressure | Strategic role | Act focus |
|---|---|---|---|---|---|
| 1 | Kessara | Industrial forge world | Urban stealth, verticality | Starting theater; manufacturing | 1 |
| 2 | Tarsis | Desert world | Open-terrain warfare, heat, vehicles | First liberation; recruits | 1–2 |
| 3 | Krad-9 | Mining world (airless) | Tunnels, zero-atmo hazards, explosives | Materials engine | 2 |
| 4 | Vel'Naar | Frozen prison world | Cold survival, sieges | Prison breaks = elite recruits | 2 |
| 5 | The Shroud | Gas-giant moons | Smuggling, fleet play intro | Black market; fleet staging | 2 |
| 6 | Meridia | Ocean trade world | Social stealth, politics, boat/platform combat | Economy capital; the Fall beat | 2–3 |
| 7 | Sylvaris | Jungle world | Guerrilla warfare, sightline chaos | Bioresearch; Vale's past | 3 |
| 8 | Elystra | Agri/garden world | Defense missions, hearts-and-minds | Food supply; propaganda war | 3 |
| 9 | Vorn | Volcanic forge/shipyard world | Assault warfare, environmental kill-zones | Dominion fleet production | 3–4 |
| 10 | Aurelion | Capital ecumenopolis | Combined-arms urban war | Endgame | 4 |

---

## 3.3 Planet Details

### 1. KESSARA — The Forge That Feeds the War
- **Environment:** Continent-scale foundry sprawl under a permanent amber smog. Layer-cake verticality: Spire Levels (Dominion admin, clean light) → Mid-Works (factories, worker housing) → the Underworks (geothermal vaults, the resistance's home). Rain is warm and gray.
- **Population:** ~900M. Foundry-caste culture: shift-clans, tool-inheritance rites, hand-sign dialect evolved on deafening factory floors (used diegetically for squad stealth commands).
- **Resources:** Materials (very high), Energy (high), Credits (low — wages are scrip).
- **Strategic importance:** the Dominion's third-largest arms producer. Liberating Kessara converts enemy production into player production (factory capture mechanic, Part 6).
- **Enemies:** Veil surveillance webs, Enforcer patrols, urban pacification walkers; low armor presence early (police, not army — escalates).
- **Missions:** Prologue + Act 1 arc; sabotage/heist/rescue types; late-game "Homecoming" return arc when Kessara fully rises.
- **Visual identity:** amber smog, silhouetted crane forests, sodium-orange vs. Dominion white-gold light. Palette: rust, amber, graphite.

### 2. TARSIS — The Sea of Rust
- **Environment:** Ochre deserts strewn with the wrecked megaships of the Reunification Wars — dune seas broken by kilometer-long hull carcasses that form natural fortresses, towns, and dungeons. Day heat 55°C (gameplay: heat timers off-vehicle at noon), freezing nights, static-lightning storms.
- **Population:** ~40M scavenger-nomads in wreck-towns; fiercely independent salvage clans with a debt-and-honor culture ("hull-right" salvage law).
- **Resources:** Materials (salvage — unique cheap source of ship-grade alloy), Intelligence (wreck datacores).
- **Strategic importance:** first liberable planet; its salvage yards jump-start the player's vehicle and (later) ship programs; nomad recruits are natural Scouts/Pilots.
- **Enemies:** Dominion salvage-tax garrisons, gunship patrols, and the collaborationist Ashline Cartel (human antagonist faction — moral-fork content).
- **Missions:** vehicle convoy warfare, wreck-dungeon delves, water-rights defense, the Act 2 liberation campaign template's debut.
- **Visual identity:** ochre/teal contrast, ship-carcass horizons, heat shimmer. Palette: rust-orange, bone-white, storm-violet.

### 3. KRAD-9 — The Hollow World
- **Environment:** An airless cratered planetoid honeycombed by centuries of mining. Surface: vacuum, stark shadows, low gravity (0.4g — jump/fall rules change). Interior: pressurized tunnel-cities, ore cathedrals, unstable deep shafts. Breach hazards make every firefight a physics question (venting atmosphere, explosive decompression as tactic and threat).
- **Population:** ~15M pit-clans. Union-brotherhood culture forged by shared air: "You breathe what your crew breathes." Brick's homeworld.
- **Resources:** Materials (extreme — the Expanse's ore heart), Energy (fusion fuel isotopes).
- **Strategic importance:** whoever holds Krad-9 out-produces the other side long-term. The miners' strike arc decides if the player gains it as ally (sustainable, slower) or armed uprising (fast, bloodier, Dominion reprisals).
- **Enemies:** corporate security forces, tunnel-war specialist Dominion units, automated mining sentinels repurposed as weapons.
- **Missions:** tunnel warfare, breach/pressure puzzles, strike protection, deep-delve salvage of a pre-Dominion generation-ship reactor (research macguffin).
- **Visual identity:** black rock, hard white key light, warm mine-lamp interiors. Palette: coal, brass, oxygen-blue.

### 4. VEL'NAAR — The White Silence
- **Environment:** Glacial world of frozen fjords and aurora storms. Home of the Dominion's prison archipelago: blacksite fortresses sunk into ice shelves. Cold exposure system (heat sources as tactical resources); blizzards that blind AI and player alike (dynamic weather as stealth tool).
- **Population:** ~2M — prisoners, guards, and the hardy fisher-villages that predate the prisons and quietly feed escapees.
- **Resources:** Intelligence (the prisons hold the Expanse's dissident elite), Technology (cryo-archives).
- **Strategic importance:** every prison break is a recruitment jackpot: scientists, officers, organizers — named elite recruits with unique perks. Vel'Naar is how the player buys skill they can't train.
- **Enemies:** elite Veil wardens, thermal-scan drones, aurora-hardened garrison troops, the ice itself.
- **Missions:** infiltration/exfiltration set-pieces, blizzard sieges, the "Long Walk" escort across the ice, Act 2's Petra-adjacent prison arc.
- **Visual identity:** white/deep-teal, aurora greens, brutalist black prison geometry. Palette: snow, ink, aurora.

### 5. THE SHROUD — The Smuggler's Sky
- **Environment:** A banded gas giant and its five habitable-domed moons + drift-station swarms. Playspace: the moon Nym's dome-city and docking sprawl, plus station interiors. Low-orbit skybox dominated by the giant's storm bands.
- **Population:** ~30M spacers, lane-runners, exiles. Culture of contracts and reputation: your word is collateral. Kaya's home.
- **Resources:** Credits (very high — grey markets), Intelligence (everyone's cargo tells a story).
- **Strategic importance:** the black-market lane network moves what Gate Spires block. Alliance with the Shroud syndicates = strategic mobility the Dominion can't fully see. Also the player's first fleet anchorage.
- **Enemies:** Dominion customs fleets, bounty hunters, rival syndicates; unique social-space danger (armed neutral zones where drawing a weapon has faction consequences).
- **Missions:** heists, smuggling runs (strategy-layer minigame), reputation intrigue, the *Loyal Ghost* corvette theft, fleet tutorial operations.
- **Visual identity:** neon dockyards against the vast amber storm-face of the giant. Palette: neon-cyan, amber, void-black.

### 6. MERIDIA — The Drowned Marketplace
- **Environment:** Ocean world; civilization lives on tessellated float-cities, tidal platforms, and the Blight-memorial coasts of the single archipelago. Combat across platform networks, boats, and under-deck maintenance labyrinths. Social-stealth zones: high-society trade quarters where weapons are impossible and words are ammunition.
- **Population:** ~600M. Trade-house culture — the old Concordat's merchant aristocracy, chafing under Bursary rationing. The Blight began here; memorial politics run deep.
- **Resources:** Credits (the Expanse's financial heart), Energy (tidal), Intelligence (trade ledgers).
- **Strategic importance:** the coalition's banker — if won. Also the authored **Fall of Meridia** beat (Act 3): the planet the Dominion takes *back*, teaching that expansion without garrisons is vanity.
- **Enemies:** Bursary revenue fleets, house mercenaries, Veil economic police; assassination intrigue.
- **Missions:** social infiltration, ledger heists, naval-platform battles, famine-archive investigation (Blight twist), the Fall and the Retaking.
- **Visual identity:** white platforms, jade sea, gold trade-house banners; rain-slick nights. Palette: jade, pearl, gold.

### 7. SYLVARIS — The Green Cathedral
- **Environment:** Megaflora jungle — trees 300m tall forming three vertical biomes (canopy light, mid-trunk civilization, bioluminescent floor). Sightlines are short and vertical; combat is ambush-first. Home to pre-Dominion biotech research vaults overgrown into ruin-dungeons.
- **Population:** ~80M in trunk-cities; botanist-clans and the survivors of the **Sylvaris Reprisals** (Kaine's regiment burned three trunk-cities pursuing insurgents — Torren Vale's defection moment; the game's darkest backstory site, handled as memorial space, not spectacle).
- **Resources:** Technology (biotech vaults — medical research accelerants), Materials (structural bio-fiber).
- **Strategic importance:** medical tech tree acceleration; guerrilla-war veteran recruits; the moral heart of the anti-Dominion case.
- **Enemies:** Dominion defoliation units, canopy gunship sweeps, vault security systems; environmental (falls, predatory flora — light touch, not a monster game).
- **Missions:** guerrilla campaign (ambush/trap systems tutorialized), vault delves, Reprisals investigation (Vale loyalty arc), canopy artillery duels.
- **Visual identity:** god-rays through 300m canopy, bioluminescent night floor. Palette: deep green, gold light, ghost-blue night.

### 8. ELYSTRA — The Garden of the Radiance
- **Environment:** Terraformed agri-paradise — the Dominion's propaganda showcase. Endless grain seas, orchard arcologies, spotless white towns. The horror is administrative: this beauty is rationed *away* from the worlds that grow nothing. Combat here is politically expensive — every crater is a propaganda gift to the Dominion.
- **Population:** ~200M agri-workers under the softest, most total surveillance in the Expanse; the hardest population to radicalize (comfort collaborates).
- **Resources:** Food (unique strategic resource entering the economy in Act 3 — liberated worlds must eat), Credits.
- **Strategic importance:** whoever feeds the Expanse rules its loyalty. Elystra flips the game from "destroy things" to "protect and persuade" — defense missions, minimal-collateral ops, hearts-and-minds systems get their showcase.
- **Enemies:** pristine ceremonial-but-lethal Radiant Guard units, saturation surveillance, informant networks (social-threat gameplay).
- **Missions:** defense of defecting granary towns, zero-casualty infiltrations (optional-objective structure), broadcast-station seizures, the granary campaign.
- **Visual identity:** golden wheat to the horizon, white arcologies, unsettling perfection. Palette: gold, white, sky-blue — Dominion colors, deliberately.

### 9. VORN — The Anvil
- **Environment:** Volcanic world of ash plains, lava fissures, and the continent-sized **Vorn Yards** — the Dominion's fleet foundry. Environmental combat: ash-storms (visibility cycling), lava kill-zones, industrial machinery as dynamic hazard/weapon. The most militarized playspace before Aurelion.
- **Population:** ~50M — conscripted labor (Tithe of Hands destination; Dex's brother is here). The liberation of Vorn's labor camps is the campaign's emotional engine for Act 3.
- **Resources:** Materials (high), Technology (shipwright expertise — unique Fleet research boosts).
- **Strategic importance:** the Dominion's fleet regenerates from Vorn. The Act 3 shipyard raid is the strategic hinge of the war: fail it and Act 4's space battles are materially harder (persistent fleet-production consequence).
- **Enemies:** full Dominion military: armor columns, mech walkers, orbital fire support, Kaine's own 1st Armada garrison.
- **Missions:** combined-arms assaults, labor-camp liberations, the Shipyard Raid set-piece, Gate Spire Delta siege (Act 4 opener).
- **Visual identity:** black ash, ember-orange fissure light, colossal ship skeletons in gantries. Palette: ash-grey, ember, arc-weld blue.

### 10. AURELION — The Radiant Throne
- **Environment:** Ecumenopolis core district — the Radiant Spire (Directorate seat), the AEGIS Vault, the sun-mirror plazas, and ring after ring of the most ordered city humanity has built. Endgame playspace: multi-district urban warfare that transforms as liberation spreads (district states: Dominion / contested / risen — populace joins the fight).
- **Population:** ~4B. The core caste believes; the outer rings only obey. The finale's uprising system makes the population itself a gameplay force.
- **Resources:** n/a (endgame theater — Aurelion consumes your economy rather than feeding it).
- **Strategic importance:** the head of the machine: the Directorate, AEGIS, and the master Gate Spire pair. The war ends here or not at all.
- **Enemies:** everything the Dominion has left, tuned by campaign state: Radiant Guard, veteran Armada formations, AEGIS-directed automated defenses, and — path-dependent — Kaine.
- **Missions:** M4.4–M4.7: the Eclipse fleet set-piece, three-front ground campaign, the Radiant Spire ascent, the Vex confrontation.
- **Visual identity:** white-gold monumentalism under the artificial twilight of the eclipse — the game's poster image. Palette: white, gold, and the deep blue shadow of your fleet.

---

## 3.4 World-Design Production Rules

1. **One biome kit per planet + shared industrial/Dominion kit.** Dominion architecture is deliberately identical everywhere (fiction: standardization as oppression; production: maximal asset reuse).
2. **Playspace budget:** 2–6 km² per planet, PCG-dressed (UE 5.7 PCG is production-ready) over hand-authored macro layouts. Interior kits shared across planets.
3. **Landmark rule:** every playspace has one horizon-scale landmark (ship carcass, prison fortress, gas giant, the Spire) for orientation and identity.
4. **Culture through systems:** each planet's culture must surface in at least one mechanic (Kessara hand-signs → stealth UI; Shroud reputation → trade prices; Krad-9 air-brotherhood → morale events), not just codex text.
5. **State layers:** every planet supports 3 visual states per region (Occupied / Contested / Liberated) via lighting, propaganda dressing, NPC population sets, and audio — the world must *show* the war's progress.

---

*Next document: [04_core_gameplay.md](04_core_gameplay.md) — player and squad systems.*
