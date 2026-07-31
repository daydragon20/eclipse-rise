# BEAT-SHEET — M1.2 *The Dead Drop*
*L1 | story-architect | 2026-07-31 | act 1, beweging I*
*Canon: `02_story_bible.md` §2.9 · `01_game_vision.md` §1.7 moment 1 (de badge) · `phase0/specs/SPEC-P2-04` (ACCEPTED)*

---

## 1. Dramatische functie

Bijbel: *"Teach stealth + intel currency."* Dat is het lesdoel. De verhaalfunctie:

> **De missie waarin Voss ontdekt dat hij intel niet alleen kan uitgeven maar ook kan máken — en waarin de speler voor het eerst iets ziet dat hij pas over honderd uur begrijpt.**

Twee dingen gebeuren hier tegelijk, en ze horen bij elkaar:

1. **De badge betaalt.** Wat de speler in de rantsoenrij onthield (of niet — `01_game_vision.md` §1.7: *"No prompt tells them the third option exists"*), wordt hier een patrouilleroulatie. Dat is de mooiste beloning die een game kan geven: iets waarvoor niemand je een quest gaf.
2. **De cel loopt langs twee dingen heen die er niet toe lijken te doen.** Een blok dat de Veil markeerde en nooit uitkamde (P4-a). Een staande order over een gestolen custodian-key (P2-a). Beide worden in act 1 weggeredeneerd. Beide zijn waar.

M1.2 is dus de **planterij van de act**. Hij ziet er rustig uit en hij is het drukst.

---

## 2. Positie in de motor

Beweging I, tweede stap. **Zichtbaarheid na M1.2: 1/8** — bij een ghost-run zelfs 0/8, wat het punt is: dit is de laatste missie waarna de Dominion niets weet. Alles daarna kost zichtbaarheid.

Vertel dat de speler niet. Laat het achteraf blijken, in M1.7, als het log laat zien wanneer Ember voor het eerst gemarkeerd werd — en die datum ligt ná M1.2, niet erna.

---

## 3. Cast

| Wie | Waar |
|---|---|
| **VOSS** | veld |
| **MARA** | veld |
| **DEX** | Hollow Point, radio — dit is zijn missie. Hij praat het meest |
| één Ember-vechter | veld, barks |
| Kessaraanse burger (arbeidersblok) | veld — **castingsleutel ontbreekt, Q-5** |
| AEGIS | omroep, twee zinnen uit een blokluidspreker |
| **REYES** | Hollow Point, S01 en S99, kort |

---

## 4. Site & runtime-haak

- **Fictie:** de arbeidersblokken van de Mid-Works. Gestapelde wooncellen, natte betonnen galerijen, wasgoed onder afdakjes, een blokwarden-kantoortje per trappenhuis. Sodium-oranje tegen het Dominion-witgoud van de bewakingsmasten.
- **`location`-string:** `Kessara / Mid-Works / Worker Housing`
- **Regio-pin:** `WorkerHousing` (`DT_StoryMissions` rij MT_M12, gebouwd, achter `Story.Beat.M11_ThirteenBullets`)
- **Runtime:** 2× `CollectItem` in gescheiden patrouillezones + `ExtractSquad`; alarmsubfase spawnt een hunter-set en laat de ghost-optional vervallen (`bRequiresNoAlarm`); riool-insertie omzeilt zone 1.
- **Briefingtekst in data:** *"Two caches, two blocks, and a patrol that never learned our faces. Keep it that way."* — Mara.
- **Prep:** 5 Intel koopt de exacte dropplekken; zonder aankoop krijgt de speler alleen *gebieden*. Dat is `11_missions.md` §11.1's eerlijkheidslus, en S01 moet hem **spelen**, niet uitleggen.

---

## 5. Scènelijst

### S01 — *The Number He Kept* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes

- **want** — Dex needs a patrol rotation he cannot buy, cannot steal, and does not have.
- **obstacle** — Ember's entire intel holding is what four people happen to remember.
- **turn** — Voss produces a badge number he has been carrying since the ration line, and the room's arithmetic changes.

**Beats:** (1) Dex legt uit wat hij niet weet, in hardware-termen. (2) Mara weegt wachten tegen gaan. (3) **Voss noemt het plaatnummer.** (4) Dex draait het en het levert een roulatie op — hij is verrast en vindt dat vervelend. (5) Mara benoemt niet dat het slim was; ze verandert gewoon het plan, wat een groter compliment is.
**Als Q-2 rood is:** de badge bestaat niet en deze scène valt terug op "Dex heeft een naam in de administratie". Dat werkt, maar het kost 01.7 moment 1 én §2.11's Enforcer-draad. **Ik adviseer met klem Q-2 groen.**
**Speler:** de prep-keuze (5 Intel uitgeven of blind gaan) hangt aan deze scène.

### S02 — *Block Nine* · eerste cache
`Kessara / Mid-Works / Worker Housing` · walk-and-talk · tier 2
**Aanwezig:** Voss, Mara, vechter; Dex op de radio; één burger; AEGIS-omroep

- **want** — Voss wants to walk through a block of nine thousand people without one of them looking twice.
- **obstacle** — A woman on the third gallery looks twice.
- **turn** — She turns her radio up instead of shouting, and Voss learns the district is already choosing sides in ways nobody counts.

**Beats:** (1) De galerijen; het geluid van te veel mensen op te weinig ruimte. (2) De AEGIS-omroep — twee zinnen, tegenwoordige tijd, een kans in plaats van een mening (§18.4). (3) De vrouw ziet hen. (4) Ze doet het enige wat veilig is: harder geluid. (5) Mara's reactie, die de speler leert wat dat waard is. (6) De cache.
**T1-plant (dressing, P1-c):** een Bursary-rantsoenplakkaat over Meridia-hulp op de galerijmuur. Niemand noemt het. Het staat er.
**Valkuil:** de burger spreekt niet, of hooguit vier woorden. Een burger die een speech geeft over hoop is precies de slop die §18.9 D beschrijft.

### S03 — *The Sweep That Never Came* · de plant
`Kessara / Mid-Works / Worker Housing` · in-mission-radio · tier 2
**Aanwezig:** Voss, Mara, Dex (radio)

- **want** — The second cache sits in a block the Veil flagged eight days ago, and Mara wants it lifted tonight anyway.
- **obstacle** — A flagged block should be crawling with Veil, and it is empty, and empty is worse than crowded.
- **turn** — Mara names it as the third time this has happened, decides it is luck, and moves — and Voss lets her.

**Beats:** (1) Het merkteken op de deurpost — Veil-notatie, acht dagen oud. (2) Dex bevestigt over de radio dat er niets is gebeurd sinds. (3) Voss aarzelt. (4) **Mara: "dat is de derde."** (5) Ze legt het uit: de Veil is politie, niet leger, en te dun uitgesmeerd over negen districten. (6) Verder.
**Dit is P4-a en het is de belangrijkste zestig seconden van de act voor twist 4.** Regels:
- Het moet **verplicht** zijn en gesproken (AR-9).
- De verklaring moet **overtuigend** zijn. De speler moet hem geloven. Een plant die verdacht klinkt is geen plant, het is een aankondiging.
- Niemand mag het woord "vreemd" of "raar" gebruiken. Mara *lost het op*; ze verwondert zich niet.
**Systeem:** zet geen vlag. Deze plant leeft in het geheugen van de speler, niet in de save. *(Overweging: een `Story.Clue.ThinResponse_1` zou act 4 laten weten dat de speler erbij was — maar de scène is verplicht, dus dat is altijd waar. Geen vlag = geen ruis.)*

### S04 — *Standing Order* · het kantoortje
`Kessara / Mid-Works / Worker Housing` · in-mission-radio · tier 2
**Aanwezig:** Voss (terminal), Dex (radio), Mara (op de uitkijk)

- **want** — Dex wants to know what the Veil is actually spending its people on this quarter.
- **obstacle** — The block warden's terminal answers a question nobody asked.
- **turn** — Recovery of a stolen custodian key-token outranks cell suppression, and Ember hears it as good news.

**Beats:** (1) Het wardenkantoortje: een bureau, een stempelkussen, een lijst met absenten. (2) Voss opent de terminal — technicus-vocabulaire, geen uitleg (§18.8). (3) **Dex leest de staande order hardop.** (4) Een grap over wat de Veil kwijt is. (5) Mara's reactie: dat betekent dat ze naar iemand anders kijken. Opluchting.
**P2-a.** De schrijver moet drie dingen halen en niet meer: *custodian*, *key-token*, *staat boven celbestrijding*. Geen enkel personage vraagt wat een custodian is. Dat is §18.8 in één beat, en het is de reden dat de onthulling in act 2 werkt.
**P2-d (optioneel):** een tweede record met een weggelakte naam in het huishouden van de Arbiter. Alleen als de speler doorzoekt. Geen dialoog nodig — één regel van Dex die het niet begrijpt en verder gaat.
**Systeem:** `Story.Clue.CustodianKey`.

### S05 — *Two Ways Out* · extractie
`Kessara / Mid-Works / Worker Housing` · in-mission-radio · tier 2
**Aanwezig:** Voss, Mara, vechter, Dex

- **want** — Get two caches out of a residential block without anyone in it paying for the visit.
- **obstacle** — Either nobody saw them, or everybody did, and the two exits are not the same exit.
- **turn** — In the quiet branch the block covers for them without being asked; in the loud branch it does not, and Voss carries that out with the intel.

**Beats, ghost-tak:** (1) Weg door de galerijen. (2) Iemand zet een deur open die dicht had moeten zijn. (3) Niemand zegt iets. (4) Dex' route.
**Beats, alarm-tak:** (1) De hunter-set. (2) Burgers binnen, deuren dicht. (3) Iemand op de galerij die er niet bij hoorde. (4) Mara's harde beslissing over wie ze niet meenemen.
**Schrijf beide takken volledig.** Dit is precies wat `21_quality_mandate.md` §21.2 bedoelt: liever twee takken van twaalf regels dan één van zes met een `condition`-regel eraan geplakt. `condition: story.m12_alarm == false` / `== true` op de regelblokken.
**T5, licht:** in de alarm-tak is er een burger die geraakt kan worden. De telling van burgerslachtoffers begint hier stil.

### S99 — *What It's Worth* · debrief
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes

- **want** — Dex wants to spend the new intel immediately on the thing he has wanted for a year.
- **obstacle** — Mara weighs intel the way she weighs food, and there are eleven mouths.
- **turn** — Voss proposes a target that is neither of theirs, and it is the one that gets written on the map.

**Beats:** (1) Wat de caches bevatten — namen, roosters, een frequentietabel. (2) Dex' wensenlijst. (3) Mara's afweging. (4) **Voss noemt de jammer-toren.** Dit is hoe M1.3 ontstaat: uit een idee van de speler, niet uit een opdracht. (5) Reyes' enige regel: een vraag over hoeveel gewonden een toren kost. (6) Mara zet het op de kaart.
**Systeem:** `Story.Beat.M12_DeadDrop`, +15 M / +30 C / +8 I, dag +1, ghost-optional +10 M/+4 I.

---

## 6. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.M11_ThirteenBullets` (pin-poort) · `Story.Thread.Enforcer_BadgeHeld` (S01, als Q-2 groen) | `Story.Beat.M12_DeadDrop` ✔ · `Story.Choice.M12_Ghost` ✚ · `Story.Clue.CustodianKey` ✚ |

---

## 7. Wendingen

| Wending | Handeling |
|---|---|
| **T2** Whisper = Ilan Vex | **geplant, dragend** (S04, P2-a). Optionele verdieping P2-d. |
| **T4** AEGIS liet het toe | **geplant, dragend** (S03, P4-a). De verklaring moet overtuigen. |
| **T1** Blight | geplant als dressing (S02, P1-c). Geen dialoog. |
| **T5** Kaine | telling loopt door in de alarm-tak van S05. |
| T3 | niet aangeraakt. |

**Betaald in deze missie:** de Enforcer-badge uit de proloog (S01). Dat is `01_game_vision.md` §1.7's *"intel seed used in Act 1"*, letterlijk.

---

## 8. Groei

- **Dex** — van gereedschapskist naar geheugen. Hij is de enige die zich herinnert wat de cel eerder probeerde en waarom het mislukte. Zijn grap in S04 is zijn beste van de act; zijn wensenlijst in S99 laat zien dat hij al twee jaar aan iets denkt dat niemand hem vroeg.
- **Voss** — leert dat hij intel kan *maken*. Structureel: hij begint de missie door iets te geven wat niemand vroeg (de badge) en eindigt hem door iets voor te stellen wat niemand vroeg (de toren). Dat is dezelfde beweging, twee keer, en het is hoe leiderschap eruitziet voordat iemand het zo noemt.
- **Mara** — haar patiënte kant, en haar blinde vlek. Ze legt de dunne Veil-aanwezigheid uit in plaats van hem te onderzoeken. Dat is geen domheid; het is wat vijf jaar overleven met je doet. Het is ook waarom ze in M1.7 K-77 doorzet.
- **Reyes** — één vraag per scène. Beide keren over gewonden. Ze bouwt haar reputatie op als de enige die vooruit rekent in mensen.

---

## 9. Keuzes

| Keuze | Waar | Gevolg |
|---|---|---|
| 5 Intel uitgeven in prep | prep, opgezet in S01 | precieze dropplekken vs. zoekgebieden |
| Ghost of luid | uitvoering | `Story.Choice.M12_Ghost`; twee volledige takken in S05; +10 M/+4 I |
| Doorzoeken van het wardenkantoortje | S04 | `Story.Clue.CustodianKey` verdieping (P2-d) |

---

## 10. Instructies voor de dialogue-writer

1. **S03 en S04 zijn de reden dat deze missie bestaat.** Als je tijd tekortkomt, schrijf die twee drie keer over en de rest één keer.
2. **Niemand verwondert zich.** Niet over de lege blok, niet over de custodian-order. Verwondering is een aankondiging, en een aangekondigde wending is geen wending. Personages *verklaren* wat ze zien en lopen door.
3. **Geen enkel personage legt uit wat intel is, wat een custodian is, of hoe de Veil werkt** (§18.8). De speler leert het uit gebruik en herhaling.
4. **Dex praat het meest van iedereen in deze missie**, en dat is de enige missie in act 1 waarin dat zo is. Gebruik het: dit is waar de speler zijn stem leert kennen voordat hij hem twintig uur in zijn oor heeft.
5. **De burger in S02 zegt hoogstens vier woorden, of niets.** Wat ze doet is de regel.
6. **Beide takken van S05 volledig schrijven.** Geen gedeelde regels met een `condition` eraan geplakt.
7. **Voss-varianten:** S01 (hoe hij de badge aanbiedt — terloops of berekend) en S99 (hoe hij de toren voorstelt). Elders neutraal.

## 11. Barks

Stealth-vocabulaire: stealth-broken · target lost · contact · order acknowledged. **Veil-vocabulaire debuteert hier niet** — de hunter-set in de alarm-tak zijn Enforcers, geen Veil. Dat is bewust: de Veil arriveert pas in M1.7 als stem, na Threx' introductie in M1.3. Een Veil-bark in M1.2 breekt AR-6.
</content>
