# BEAT-SHEET — M1.4 *The Quartermaster*
*L1 | story-architect | 2026-07-31 | act 1, beweging II*
*Canon: `02_story_bible.md` §2.5 (Brick), §2.9 · `phase0/specs/SPEC-P2-04` decision 11 + resolved point 4 · `19_voice_production.md` §19.3 (Bricks signature-regel)*

---

## 1. Dramatische functie

Bijbel: *"First real weapons; recruit Brick."* Verhaalfunctie:

> **De missie waarin de cel ophoudt te schrapen — en waarin de vijand voor het eerst een gezicht heeft dat niet van de vijand is.**

Twee dingen die elkaar dragen:
1. **Het arsenaal** maakt van dertien kogels een voorraad. Mechanisch is dat een loadout-unlock; dramatisch is het het moment waarop Ember iets bezit dat verdedigd moet worden. Bezit is het begin van kwetsbaarheid, en dat is de kiem van het hele act-2-materiaal (Siege of Hollow Point).
2. **Brick** komt binnen door een deur die hij zelf opendoet. Hij is geen buit en geen redding: hij is een man die al drie maanden wacht op iemand met een reden. Zijn hele karakter (§18.4: minste woorden, antwoordt met een dode naam) staat in zijn eerste drie woorden.

En één ding dat pas over honderd uur uitbetaalt: **na deze nacht heeft Ember een voorraadadministratie.** Dat is een baan. Iemand gaat die baan invullen. Dat is P3-a.

**Zichtbaarheid na M1.4: 4/8.** Een leeggehaald arsenaal is geen incident meer; het is een patroon met een dossier.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS**, **MARA** | veld |
| **BRICK** (Oram Bex) | het holdingblok van het arsenaal — **debuut** |
| twee Ember-vechters | veld |
| **DEX** | Hollow Point, radio |
| Dominion-magazijnofficier | PA-omroep tijdens lockdown — **castingsleutel ontbreekt, Q-5** |
| Dominion-conscripten (garnizoen) | barks |
| **REYES** | Hollow Point, S99 — **haar grootste scène van de act** |

### Brick — de canon die je moet kennen voor je hem schrijft
- 34, ex-mijnwerker uit de put op **Krad-9** (§2.5). Zwaar wapen. Moreel anker; "the soldiers' soldier".
- **Fingerprint (§18.4):** minste woorden van iedereen. Vaak één zelfstandig naamwoord. Tic: beantwoordt moeilijke vragen door een dode soldaat te noemen. Nooit: liegen; als eerste spreken in een groep.
- **Signature-regel (§19.3):** *"Tam. Oyelaran. Vic."* — die drie namen zijn canon en horen bij hem.
- **Hoe hij in een Kessaraans arsenaal terechtkomt zonder dat ik iets verzin:** de **Tithe of Hands** (§2.3) is een arbeidsloterij die mensen tussen werelden verplaatst. Een putmijnwerker van Krad-9 die door de Tithe naar Kessara is gehaald en daarna in een strafcel eindigde, gebruikt uitsluitend bestaande canon. **Dit is ruling AR-11**, en het levert gratis een tweede betaling op: Brick heeft een persoonlijke reden om in M1.6 (*The Tithe Train*) mee te gaan, en die missie raakt hem harder dan wie ook.
- **SPEC-P2-04 decision 11:** in de slice is Brick een rosterrecord, geen companion-systeem. Zijn dialoog wordt nu geschreven en later afgespeeld (zie C-7).

---

## 3. Site & runtime-haak

- **Fictie:** een Dominion-magazijn in Foundry Row — een omgebouwde gieterijhal, kratten in stellingen tot vier hoog, een kantoor op een bordes, en achterin een holdingblok van drie cellen waar de wacht mensen zet die niet in een rapport passen.
- **`location`-string:** `Kessara / Foundry District / Foundry Row Armory`
- **Regio-pin:** `FoundryRow` (gemeten: na M1.3's Foothold is dit de enige overgebleven aangrenzende Dominion-regio; zie `setup_story_missions.py`)
- **Runtime:** 2× `CollectItem` (wapenkratten) + `ExtractSquad`; escalatiesubfases op alarm én op krat 1; krat 2 triggert altijd lockdown; breach-punt óf zijrooster (decision 8); optional = niemand neergegaan (+15 M)
- **Briefingtekst in data:** *"Their armory has been feeding this district for years. Tonight it feeds us."* — Mara.
- **Commit:** +200 M / +100 C, Brick op het roster, `UnlockedLoadoutTags += real-rifle tier`, `Story.Beat.M14_Quartermaster` + `Story.Beat.BrickRecruited`.

---

## 4. Scènelijst

### S01 — *Their Armory* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes

- **want** — Mara wants the armory emptied in one night, because there will not be a second one.
- **obstacle** — Nobody in the room has ever carried more than they could run with.
- **turn** — Dex points out that the problem is not getting in, it is that eleven people cannot lift what is inside.

**Beats:** (1) Wat er in het arsenaal ligt, verteld als gewicht en niet als vuurkracht — dat is Dex' hoofd. (2) De insertie: breach of rooster (speler kiest). (3) Reyes' vraag: wat doen we met de bewaking. **Dit is de eerste keer dat iemand die vraag stelt**, en hij hangt boven de hele rest van de act. (4) Mara's antwoord is niet mooi. (5) Dex' rekensom over draagkracht — het probleem dat M1.4's ontknoping oplost, want Brick kan dragen.
**Prep:** IC-intel onthult de wachtroulatie.

### S02 — *The Grate* / *The Breach* · binnenkomst
`Kessara / Foundry District / Foundry Row Armory` · walk-and-talk / in-mission-radio · tier 2
**Aanwezig:** Voss, Mara, vechters; Dex op de radio

- **want** — Get four people inside a working armory without starting the fight in the doorway.
- **obstacle** — The quiet way is a coolant grate built for one person at a time; the loud way announces itself.
- **turn** — Inside, the place is bigger than the plan, and the plan quietly becomes Voss's.

**Twee volledige takken.** Rooster: nauw, één voor één, kwetsbaar; Voss herkent het koelsysteem (technicus — en dit bereidt AR-2 voor, want M1.7 speelt in precies zo'n systeem). Breach: luid, snel, en de eerste conscripten die vallen zijn jong.
**Gedeelde beat:** de eerste blik op de hal. Iets in de trant van "hier had de hele Foundry Row een jaar van kunnen leven" — maar dan geschreven, niet als thesis.

### S03 — *The Man In The Holding Cell* · Bricks debuut
`Kessara / Foundry District / Foundry Row Armory` · in-mission-radio · tier 2
**Aanwezig:** Voss, Brick, één vechter; Mara op de radio

- **want** — Voss wants a route to the second crate that does not cross the office bridge.
- **obstacle** — The only person who knows this building is locked in the third cell and has no reason to help anybody.
- **turn** — He gives Voss the route in nine words, and asks for nothing, and that is what makes Voss come back for him.

**Beats:** (1) Het holdingblok. Drie cellen. Twee leeg. (2) **Bricks eerste regel is drie namen.** Niet als antwoord op "wie ben jij" — als antwoord op "waarom sta jij hier nog". Dat is zijn hele personage in één beat en de speler snapt het pas de tweede keer. (3) Voss vraagt de route. (4) Brick geeft hem. Kort, exact, geen voorwaarde. (5) Voss biedt de deur aan. (6) Brick zegt nog niet ja.
**Waarom hij nog niet ja zegt:** een gevangene die meteen dankbaar meeloopt, is een quest-item. Een man die eerst zijn werk afmaakt en pas in S05 meeloopt, is een personage. Zijn ja komt in S05, en het is één woord.
**Valkuil:** Brick spreekt nooit als eerste in een groep (§18.4). In deze scène is hij alleen met Voss, dus hij mag antwoorden — maar hij begint nooit.

### S04 — *Lockdown* · de escalatie
`Kessara / Foundry District / Foundry Row Armory` · in-mission-radio + callout · tier 2
**Aanwezig:** allen; magazijnofficier (PA); conscript-garnizoen

- **want** — The quartermaster wants his building sealed and his inventory intact, in that order.
- **obstacle** — He is announcing his procedure over the public address while he executes it, because that is what the manual says.
- **turn** — The garrison that comes down the aisles is nineteen years old, and Ember has to decide what winning costs tonight.

**Beats:** (1) Krat 2 opent; de lockdown begint. (2) **De officier op de PA** — procedureel, bureaucratisch, en hij noemt kratnummers terwijl er mensen doodgaan. Hij is niet wreed; hij is een man die zijn formulier afwerkt. (`02_story_bible.md` §2.2: *"Every atrocity has bureaucratic logic."*) (3) Het garnizoen: conscripten. Barks in het jonge, procedurele vocabulaire (§18.5 regel 5). (4) Brick opent de achterroute. (5) Eén Ember-regel over hoe oud ze zijn — **één**, en niemand reageert erop.
**T5-plant P5-b.** De telling loopt door: wie hier zonder onderscheid alles neermaait, betaalt in act 4.
**Valkuil:** géén personage houdt een pleidooi over conscripten. Eén regel, geen antwoord, doorlopen. Dat doet meer dan een discussie.

### S05 — *Carrying* · eruit
`Kessara / Foundry District / Foundry Row Armory` · in-mission-radio · tier 2
**Aanwezig:** allen incl. Brick

- **want** — Get out with more than they can carry.
- **obstacle** — Every kilo they leave is somebody who does not get a rifle.
- **turn** — Brick picks up the load that was going to be left behind, and answers the offer of the door with one word.

**Beats:** (1) Wat blijft liggen — en wie dat besluit. (2) De achterroute. (3) **Brick tilt.** Hij draagt meer dan twee anderen samen en zegt er niets over. (4) Voss herhaalt het aanbod. (5) **Bricks ja.** Eén woord. (6) Weg, in het licht van de brandende laadperrons of juist in volledige stilte (afhankelijk van luid/stil).
**Dit is het moment waar de missie voor bestaat**, en het bevat vier gesproken woorden. Weersta alles.

### S99 — *Real Rifles* · debrief
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, **Reyes**, Brick, de cel

- **want** — Reyes wants to examine the man who has been in a Dominion holding cell for three months.
- **obstacle** — Brick answers medical questions the way he answers all questions: with a name.
- **turn** — She stops asking about his body and asks about the names, and he tells her, and that is the first thing anyone in Ember has given her to treat.

**Beats:** (1) De buit uitgestald; Dex die er niet vanaf kan blijven. (2) De rekening: wat het kostte. (3) **Reyes onderzoekt Brick.** Ex-Dominion-medicus tegenover ex-Dominion-gevangene. Zij weet precies hoe zijn cel eruitzag, want zij heeft in dat systeem gewerkt. Ze zegt dat niet. (4) De namen. (5) Mara zet Brick op de lijst. (6) **P3-a:** Dex merkt op dat iemand dit spul moet bijhouden — er is nu een voorraad, dus er is een administratie, dus er is een baan. Eén grap, en de baan bestaat.
**Reyes' arc:** §2.5 zegt *"from penance to purpose"*. Dit is de eerste steen: ze behandelt iemand die zij vroeger had moeten triageren op loyaliteitsscore. Niet uitleggen. De scène weet het; zij zegt het niet.
**Systeem:** `Story.Beat.M14_Quartermaster`, `Story.Beat.BrickRecruited`, +200 M/+100 C, loadout-unlock, dag +1, optional +15 M.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.M13_SignalFire` | `Story.Beat.M14_Quartermaster` ✔ · `Story.Beat.BrickRecruited` ✔ |

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T3** de mol | **geplant, dragend** (S99, P3-a): de functie kwartiermeester ontstaat. Niemand vult hem in deze missie. |
| **T5** Kaine | **geplant** (S04, P5-b): conscriptgarnizoen, telling loopt. |
| **T4** | niet hier. Het arsenaal wordt volledig verdedigd. Zie de opmerking bij M1.3: contrast is de camouflage. |
| T1, T2 | niet aangeraakt. |

---

## 7. Groei

- **Brick** — komt binnen als een man die al drie maanden namen telt in het donker. Zijn arc (§2.5: *"quiet grief — he remembers every name on the wall"*) begint dus vóór de muur bestaat. Dat is belangrijk: **de muur in M1.8 is niet zijn idee, maar hij is de enige die er meteen thuis is.**
- **Reyes** — haar eerste echte scène. Van klinische bijrol naar iemand met een verleden dat de speler kan ruiken maar niet kan lezen.
- **Voss** — commandeert voor het eerst een vreemde, en doet het door iets aan te bieden in plaats van te vragen.
- **Dex** — krijgt gereedschap en is er ongelukkig mee, want gereedschap betekent dat er gevochten gaat worden op een schaal waar hij niet om vroeg. Eén regel, verstopt in een grap.
- **Mara** — zet een naam op een lijst en de lijst is te lang aan het worden. Plant voor M1.6.

---

## 8. Keuzes

| Keuze | Waar | Gevolg |
|---|---|---|
| Rooster of breach | S01/S02 | twee takken; conscript-doden verschillen sterk |
| Wat blijft liggen | S05 | flavor + kleine materiaalvariatie |
| Hoe hard het garnizoen wordt aangepakt | S04, systemisch | T5-telling |

---

## 9. Instructies voor de dialogue-writer

1. **Brick spreekt in deze hele missie minder dan dertig woorden.** Tel ze. Als je over de dertig komt, ben je iemand anders aan het schrijven.
2. **Zijn eerste regel is drie namen** en er komt geen uitleg achteraan, nu niet en nooit.
3. **De magazijnofficier is geen schurk.** Hij leest zijn procedure voor terwijl er mensen sterven. Dat is enger en het is `02_story_bible.md` §2.2.
4. **Eén regel over de leeftijd van het garnizoen, en niemand antwoordt.** Een discussie hierover verplaatst de scène van de speler naar de personages.
5. **Reyes zegt nooit dat ze bij de Dominion heeft gediend.** De speler weet het uit haar handen: ze weet welke cel het was zonder te vragen.
6. **P3-a moet als grap landen.** "Iemand moet dit bijhouden" mag onder geen beding gewichtig klinken. Als het gewichtig klinkt, weet de speler in act 3 al wie de mol is.
7. **Voss-varianten:** S03 (hoe hij de deur aanbiedt) en S99. Elders neutraal.

## 10. Barks

Eerste inzet van **interieur-gevecht**: breaching, corridors, lockdown-sirenes. Dominion-conscriptvocabulaire onder paniek in een gebouw dat ze kennen en de speler niet. Bricks eigen barkset (P2-01) debuteert — §18.5 regel 6: squad-barks gebruiken namen, dus vanaf nu klinkt *"{name} is down"* met een naam die de speler kent.
</content>
