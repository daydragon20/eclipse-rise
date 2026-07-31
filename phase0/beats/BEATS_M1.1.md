# BEAT-SHEET — M1.1 *Thirteen Bullets*
*L1 | story-architect | 2026-07-31 | act 1, beweging I | **ijkmissie L0***
*Canon: `02_story_bible.md` §2.9 · `01_game_vision.md` §1.7 moment 2 · `phase0/specs/SPEC-P2-04` (ACCEPTED) · `phase0/SCRIPT_FORMAT.md` §3*

---

## 1. Dramatische functie

De bijbel geeft M1.1 één didactisch doel: *"Teach combat-as-problem-solving."* De vision-doc scherpt aan (§1.7 moment 2): *"scarcity as drama; combat as problem-solving."*

De verhaalfunctie is groter dan het lesdoel, en die is:

> **De eerste keer dat iemand Voss een getal geeft dat te klein is, en hem laat zien dat het genoeg is.**

Dat is de hele game in acht minuten. Eclipse gaat over een beweging die structureel te weinig heeft — te weinig mensen, te weinig schepen, te weinig tijd — en toch wint, door beter te kijken dan de tegenstander. M1.1 leert dat niet met een tutorialtekst maar met dertien kogels en een vrouw die zegt dat dat vroeger zes waren.

**Dit is bovendien de ijkmissie (L0).** Wat hier geschreven wordt, is de maat voor 132.000 woorden. Als de personages hier niet als personages klinken, klinkt niemand ergens als iemand. De schrijver van deze missie schrijft niet één missie; hij schrijft de mal.

---

## 2. Positie in de motor

Beweging I, eerste stap. De cel is op zijn kleinst en het meest onzichtbaar. De Dominion weet niet dat Ember bestaat — en de eerste plant van T4 zit al in wat er *niet* gebeurt: er komt geen leger achter een supply-patrouille aan, want Kessara heeft in act 1 "police, not army" (`03_world_design.md` §3.3). In act 1 leest dat als wereldopbouw. In act 4 leest het als toewijzing.

**Zichtbaarheid na M1.1: 1/8.** Eén patrouille die niet terugkomt. De Dominion noteert het als een overval.

---

## 3. Cast

| Wie | Waar | Waarom hij er is |
|---|---|---|
| **VOSS** | veld | de speler |
| **MARA** | veld, squadleider | zij leidt nog. Dit is de laatste missie waarin ze onbetwist de baas is zonder dat iemand het merkt |
| **DEX** | Hollow Point, radio | de stem in het oor. Zijn eerste karakterbeat gaat over apparatuur die hij niet heeft |
| **REYES** | Hollow Point, off-mic in S01 en S99 | één klinische regel per kant. Zij is de enige die vooraf zegt hoeveel gewonden ze aankan |
| 2 Ember-vechters uit de acht (AR-1) | veld | barks, namen. Uit deze acht komt later de eerste naam op de muur (AR-10) |
| Dominion-conscripten (supply-patrouille) | veld | barks. Jong en procedureel (§18.5 regel 5) |

**Niet aanwezig:** iedereen anders. Zie `ACT1_OVERVIEW.md` §2, AR-8.

---

## 4. Site & runtime-haak

- **Fictie:** een verkeersoverbrugging in het Foundry District, boven het transitcheckpoint waar de bevoorradingsroute doorheen loopt. Amber smog, sodium-oranje, warme grijze regen (`03_world_design.md` §3.3). De cel ligt hoog; de patrouille loopt laag. Verticaliteit is Kessara's handtekening en meteen het tactische lesje.
- **`location`-string:** `Kessara / Foundry District / Overpass` — **letterlijk zoals `SCRIPT_FORMAT` §3 hem gebruikt**, niet aanpassen.
- **Regio-pin:** `TransitCheckpoint` (`DT_StoryMissions` rij MT_M11, al gebouwd).
- **Runtime:** `DestroyTarget` (patrouilleleider) + `ExtractSquad`; hinderlaag armt wanneer de squad positie houdt; versterkingssubfase op `bAlarmRaised`; optional = zero-casualty (+20 M).
- **Briefingtekst die al in data staat:** *"Thirteen bullets between us and the first real strike. Make every one of them a promise."* — spreker Mara. **Dat is de kiem van S01, niet de vervanging ervan.** De regel mag letterlijk terugkomen als slotregel van de briefingscène.

---

## 5. Scènelijst

### S01 — *Thirteen Between Us* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes, twee vechters (non-speaking of één bark)

- **want** — Mara needs eleven frightened people to spend their last ammunition on a supply patrol instead of hoarding it.
- **obstacle** — Everyone in the room has already done the arithmetic, and the arithmetic says wait.
- **turn** — She stops arguing the odds and hands them the count as an inheritance instead of a shortage.

**Beats:** (1) De telling wordt hardop gedaan — dertien, verdeeld over vier mensen. (2) Dex somt op wat hij níét heeft; zijn eerste grap, en hij is bang. (3) Reyes stelt één vraag in plaats van een bezwaar: hoeveel gewonden verwacht Mara. (4) Voss ziet de route op de kaart en zegt iets technisch dat niemand vroeg — de eerste keer dat de speler merkt dat zijn personage anders kijkt. (5) Mara sluit met de belofte-regel.
**Speler:** één interjectie of keuze in de planning (de overbrugging of de onderdoorgang) — §18.7: de speler is deelnemer, geen publiek.

### S02 — *The Overpass* · nadering
`Kessara / Foundry District / Overpass` · walk-and-talk · tier 2
**Aanwezig:** Mara, Voss, twee vechters; Dex op de radio

- **want** — Voss wants to know what happens if the plan fails, without asking it out loud.
- **obstacle** — Mara answers questions about failure by talking about the weather, the route, anything else.
- **turn** — She tells him what she actually expects to lose tonight, and it is a smaller number than he feared and a worse one than he wanted.

**Beats:** (1) De stad van bovenaf: kraanwouden, het Dominion-witgoud van de Spire Levels ver weg. (2) Dex' eerste radio-lijn — hardware-humor over de zender. (3) Een vechter vraagt naar een naam die de speler nog niet kent; Mara antwoordt met "wij". (4) Voss' vraag en Mara's ontwijking. (5) Het antwoord.
**Speler:** beweging; de dialoog loopt door onder het lopen.

### S03 — *The Ammo Count* · het scharnier
`Kessara / Foundry District / Overpass` · in-mission-radio · tier 2
**Aanwezig:** Mara, Voss

> **Deze scène staat al in `SCRIPT_FORMAT` §3 als het canonieke voorbeeld.** Want/obstacle/turn zijn daar letterlijk gegeven en worden **niet** herschreven. De drie voorbeeldregels (`.010` Mara "Thirteen." / `.020` Voss met varianten / `.030` Mara "It was enough for the ones who had six.") zijn **gereserveerd op hun ID's**. De schrijver mag ze houden — mijn advies is: houden. Ze zijn beter dan wat een tweede poging oplevert, en ze zijn de reden dat het formaat-document bestaat.

- **want** — Mara needs Voss moving before the patrol cycles back.
- **obstacle** — Voss has counted his rounds and the number has him.
- **turn** — She hands him the number back as a promise instead of a limit.

**Beats:** (1) Dertien. (2) Dat is niet genoeg. (3) Het was genoeg voor wie er zes had. Meer heeft de scène niet nodig; alles wat je toevoegt haalt hem omlaag.
**Ruimte voor uitbreiding:** ná `.030` mag er nog één beat bij (bijv. Voss die het getal opnieuw gebruikt, nu als plan). ID's `.040`+. Vóór `.010` niets.

### S04 — *One Volley* · de hinderlaag
`Kessara / Foundry District / Overpass` · in-mission-radio · tier 2
**Aanwezig:** Mara, Voss, twee vechters, Dominion-conscripten (barks)

- **want** — The cell wants the whole patrol down inside the free volley, before anyone can key a radio.
- **obstacle** — The patrol walks the chokepoint in the wrong order and the leader is last.
- **turn** — Voss re-solves the ambush out loud, and Mara follows his call instead of her own.

**Beats:** (1) Wachten; ademen; de patrouille komt. (2) De volgorde klopt niet. (3) Voss' oplossing — technisch, kort, over de radio. (4) Mara's toestemming in twee woorden. (5) De volley. (6) Wat er misgaat.
**Dit is de eerste keer dat iemand Voss volgt.** Zo klein dat de speler het bijna mist, en dat is precies goed — §2.4: "people follow them" moet uit spel volgen, niet uit een toespraak.
**Regellengte:** callout-band waar het onder vuur is (2–6 woorden). Deze scène draagt de test uit `SCRIPT_PRODUCTION_PLAN` §3: *praten ze over het schieten heen?*

### S05 — *The One Who Is Still Moving* · de keuze
`Kessara / Foundry District / Overpass` · in-mission-radio · tier 2
**Aanwezig:** Voss, Mara, één gewonde Dominion-conscript, één Ember-vechter

- **want** — Voss wants to leave, and the wounded conscript on the deck will not stop making noise.
- **obstacle** — Mara will not make this call for him, and the patrol's relief cycles in minutes.
- **turn** — Whatever Voss decides, the cell watches him decide it, and that is the thing that changes.

**Beats:** (1) De conscript leeft, is jong, en praat procedureel omdat hij het zo geleerd heeft — hij noemt zijn eenheidsnummer, niet zijn naam. (2) De vechter wil doorlopen. (3) Mara zegt niets. Bewust — zij bewaart haar "wij" hier. (4) Voss kiest: **afmaken / laten liggen / verbinden en achterlaten**. (5) Eén reactie per keuze van de vechter, één van Mara.
**Systeem:** zet `Story.Choice.M11_ConscriptSpared`. Eerste invoer in de T5-telling (Kaine's geweten, act 4).
**Valkuil:** geen personage mag hier de moraal uitspreken. §18.9 A verbiedt het en de scène heeft het niet nodig — de speler weet zelf wel wat hij deed.

### S06 — *Off The Deck* · extractie
`Kessara / Foundry District / Transit Checkpoint` · in-mission-radio · tier 2
**Aanwezig:** Mara, Voss, vechters, Dex op de radio

- **want** — Get four people off an elevated position with a response cycle closing.
- **obstacle** — They are carrying more than they came with and one of them is slow.
- **turn** — Dex talks them through a route he has never walked, and is right, and cannot take the compliment.

**Beats:** (1) Wat ze meenemen (kratten, magazijnen — het *waarom* van de hele missie). (2) De respons komt, en hij is dun. (3) Dex' route. (4) Iemand bedankt Dex; hij **redirect vlak naar het werk** — géén grap hier (L1-R7: deze scène grenst aan de nasleep). Dat hij de opening laat liggen, is de karakterbeat. (5) Weg.
**T4-plant, licht:** één regel van een vechter over hoe weinig er kwam. Nog geen mysterie — opluchting.

### S99 — *What Thirteen Bought* · debrief
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes, de cel

- **want** — Mara wants the cell to understand what they now own, before they start celebrating what they survived.
- **obstacle** — They came back with more ammunition than they left with, and nobody can stop counting it.
- **turn** — She gives the night's credit to Voss in front of everyone, and he gives it straight back to the two who walked the low route.

**Beats:** (1) De telling opnieuw — dit keer omhoog. (2) Reyes behandelt wat er te behandelen is; haar eerste volledige klinische zin. (3) Dex ontleedt wat ze buit hebben. (4) Mara noemt wat Voss deed. (5) Voss geeft het door — §18.4: *"never claims credit in front of the people who did the work."* Dit is de scène waar die eigenschap voor het eerst zichtbaar is. (6) Als de conscript gespaard is: één regel die daarnaar terugwijst, zonder oordeel.
**Systeem:** `Story.Beat.M11_ThirteenBullets`, +25 M / +50 C, dag +1, optional zero-casualty +20 M.

---

## 6. Vlaggen

| In | Uit |
|---|---|
| — (openingsmissie, `UnlockBeatTag` leeg) | `Story.Beat.M11_ThirteenBullets` ✔ bestaat · `Story.Choice.M11_ConscriptSpared` ✚ nieuw |

---

## 7. Wendingen

| Wending | Handeling |
|---|---|
| **T4** AEGIS liet het toe | **geplant, licht** (S06): de respons is dun, en dat voelt als geluk. Nog geen aandacht op vestigen. |
| **T5** Kaine's geweten | **geplant, dragend** (S05, P5-a): de telling begint. |
| Enforcer-draad | **aangeraakt** (S02 of S04, één regel): Voss herkent de eenheidsmarkering van de patrouille als dezelfde detachering als de badge uit de rij. Bereidt M1.2.S01 voor. Alleen schrijven als Q-2 groen is. |
| T1, T2, T3 | niet aangeraakt. Te vroeg. |

---

## 8. Groei

- **Voss** — van iemand die telt wat hij heeft naar iemand die telt wat hij ermee kan. De as-verandering zit in S04 (hij herrekent de hinderlaag) en S99 (hij geeft krediet door). Aan het eind van deze missie is hij nog geen leider; hij is iemand die één keer gelijk had, en dat is precies genoeg.
- **Mara** — gevestigd als de gelovige. Eén scheurtje, en niet meer dan één: in S02 zegt ze hoeveel ze verwacht te verliezen, en dat getal is niet nul. Zij heeft dit al eerder gedaan en er mensen bij verloren. Niet uitleggen. Eén getal.
- **Dex** — gevestigd als de man die bouwt in plaats van hoopt. Zijn grap in S01 is bang; zijn route in S06 is goed; zijn deflectie is automatisch.
- **Reyes** — twee regels, allebei klinisch, allebei een vraag in plaats van een verwijt (§18.4).

---

## 9. Keuzes

| Keuze | Waar | Gevolg |
|---|---|---|
| Overbrugging of onderdoorgang | S01 | tactische opzet; kleine variant in S02/S04 |
| De gewonde conscript | S05 | `Story.Choice.M11_ConscriptSpared` → T5-telling, act 4 |
| Zero-casualty (systemisch, geen dialoogkeuze) | uitvoering | +20 M; één extra regel van Reyes in S99 |

---

## 10. Instructies voor de dialogue-writer

1. **Dit is de mal.** Alles wat je hier doet, wordt zeven keer nagedaan. Schrijf langzaam.
2. **Raak S03 nauwelijks aan.** Drie regels staan er al en ze zijn goed. Voeg hooguit één beat toe ná `.030`. Niets vóór `.010`.
3. **De variantieregel bijt hier het hardst** (§18.3): binnen elke scène minstens factor 3 verschil in regellengte. S04 is bijna helemaal callout-band; zorg dat S01 en S99 dat compenseren.
4. **Mara zegt nooit "ik" over de zaak** en zegt in deze hele missie geen enkele keer "jij" tegen Voss over de zaak (AR-5 — dat woord is gereserveerd voor M1.8.S08). Ze mag wel "jij" zeggen over praktische dingen ("hou je links"). Het verbod geldt de zaak, niet de grammatica.
5. **Dex' grappen — gecorrigeerd, ruling L1-R7.** Dit punt sprak §5 tegen (samen één te veel) en de schrijver van de ijkmissie ving dat. Vastgesteld: **S01, S02, en S99 — de derde alleen in de schone run.** Ligt er iemand gewond, dan is S99 nasleep en zijn het er twee (§18.6: nul komische beats in nasleep). **S06 is een vlakke redirect, geen grap** — hij grenst aan de nasleep. De bangste grap blijft de eerste.
6. **Geen enkel personage benoemt een gevoel** (§18.9 A). Er is één rantsoen van ~30 `direct_beat`-regels in het hele spel; **M1.1 krijgt er nul.** De eerste geef ik uit in M1.8.
7. **De conscript in S05 praat procedureel, niet zielig.** Hij noemt zijn eenheidsnummer omdat hij dat geleerd heeft. Dat is erger dan smeken en het is §18.5 regel 5.
8. **Voss-varianten:** S03 heeft ze al (pragmatist/idealist in het voorbeeld). Verder alleen op S05 (de keuze) en S99 (het doorgeven van krediet). Elders één neutrale Voss-regel — zie C-6/Q-7, het budget hangt hieraan.

## 11. Barks die deze missie nodig heeft

Uit de 16 prioriteitstriggers (§18.5): contact · taking fire · reloading · out of ammo · enemy down · squadmate down · order acknowledged · target lost · low health · objective complete · extraction called. **Drie vocabulaires** (Eclipse geïmproviseerd en persoonlijk; Dominion-conscript jong en procedureel; Veil komt hier nog niet voor). Deze zijn tier 1 en worden gegenereerd vóór deze scènes — ze zijn dus **niet** van deze schrijver, maar M1.1 is wel de missie waar ze voor het eerst allemaal tegelijk klinken. *"Out of ammo"* draagt in deze missie meer gewicht dan waar ook in het spel.
</content>
