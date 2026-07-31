# BEAT-SHEET — M1.7 *Under the Ice*
*L1 | story-architect | 2026-07-31 | act 1, beweging IV*
*Canon: `02_story_bible.md` §2.9 (M1.7), §2.6 (Threx), §2.8 twists 2 en 4 · `03_world_design.md` §3.3 (Kessara-hoogtes) · architect-rulings AR-2, AR-6*

---

## 1. Dramatische functie

Bijbel: *"Discover Threx knows Ember's sector; countdown pressure."* Verhaalfunctie:

> **De missie waarin de jager zich omdraait — en waarin de speler voor het eerst meer weet dan zijn mentor en dat hardop zegt.**

M1.7 is de scharniermissie van act 1 en hij doet vier dingen tegelijk:

1. **De omkering.** Zes missies lang keek Ember naar de Dominion. Vanaf nu kijkt de Dominion terug, en het bewijs is een logboek met Embers eigen operaties erin, op datum.
2. **De grootste plant van het spel.** Threx' afgewezen interdictie-aanvraag (P4-c) is het document waarop twist 4 rust. In act 1 leest het als bureaucratische zuinigheid. In act 4 is het het bewijs dat AEGIS de opstand in leven hield.
3. **De tweede jager.** Iemand die niet van de Veil is, heeft eerder een query op Embers sector gedraaid (P2-b). Geen naam, geen gezicht, geen uitleg. Whisper bestaat pas in act 2 — maar zijn schaduw ligt hier.
4. **De keuze die Mara doodt.** K-77 wordt in S99 *gekozen*, met tegenspraak, met een alternatief op tafel. Als de speler in M1.8 het gevoel heeft dat de val hem overkwam, is act 1 mislukt. Hij moet weten dat hij erin gelopen is, en waarom, en dat hij het opnieuw zou doen.

**Zichtbaarheid na M1.7: 8/8.** Threx heeft een naam op zijn bureau en het is die van Ember.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS** | veld — dit is zijn missie |
| **MARA** | veld |
| **DEX** | veld óf radio (zie S02) |
| **BRICK** | veld, drie regels |
| **THREX** | live Veil-verkeer, trede 3 |
| Veil-operatives | de post, barks |
| **REYES**, **SELA** | Hollow Point, S99 — beiden nodig, want S99 is de scène waarin iedereen gelijk heeft |

---

## 3. Site & runtime-haak — AR-2

**Bevinding C-3:** de missie heet *Under the Ice* en Kessara heeft geen ijs (`03_world_design.md` §3.3: permanente amber smog, warme grijze regen).

**Architect-ruling AR-2:** *"the Ice"* is Kessaraans jargon voor de **koelkelder** van de Underworks — de laag waar de geothermische hitte van de Mid-Works wordt weggekoeld met condensorbanken. Het is het enige echt koude punt op een gloeiende planeet, en daarom noemt iedereen het zo.

**En daarom heeft de Veil er zijn luisterpost neergezet:** een serverbank produceert warmte, en warmte is op Kessara het enige wat je niet kunt verbergen. In een koelkelder is hij onzichtbaar.

Dit doet drie dingen tegelijk: het redt de canon-titel, het geeft Voss een technicusregel die niets uitlegt (hij is derde-generatie gieterijtechnicus — dit is letterlijk zijn vak), en het verzint geen locatie buiten de bestaande drie hoogtes.

- **`location`-string:** `Kessara / Underworks / Coolant Sublevel`
- **Regio-pin:** `CommsRelay` — grenst aan `SupplyDepot` en `FoundryRow`, en `SupplyDepot` is na M1.3's Foothold van de speler. **Lane-legaal, nagerekend.**
- **Runtime:** infiltratie. `CollectItem`/`InteractTarget` (het log) + `ExtractSquad`; alarmsubfase; optionals: het requisitiedocument (P4-c) en het geredigeerde personeelsrecord (P2-d).
- **Buiten SPEC-P2-04.** Geen nieuwe primitieven nodig.

---

## 4. Scènelijst

### S01 — *A Friend Of A Friend* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Brick, Reyes, Sela

- **want** — Mara wants to know how much the Veil has on Ember, tonight, before it grows.
- **obstacle** — There is no way to know that without walking into the place where it is written down.
- **turn** — Voss asks where the tip came from, and Mara answers in a way that ends the conversation without answering.

**Beats:** (1) Wat er de afgelopen twee weken veranderd is: patrouilles die de goede straten lopen, een dead drop die verschoven moest worden. (2) Dex heeft de warmte gevonden — de koelkelder gebruikt te veel vermogen. **Techniek, geen uitleg.** (3) **P2-c:** Voss vraagt wie de tip gaf. Mara: *een vriend van een vriend op de lanes.* Meer niet. Dex kijkt op; niemand vraagt door. **Eén regel. De schrijver mag hem niet verdedigen en niet benadrukken.** (4) Sela's eerste briefing — ze zit er nu bij, en haar aanwezigheid verandert wat er gezegd kan worden. (5) Reyes stelt de vraag over terugweg en gewonden. Mara heeft er geen goed antwoord op en zegt dat.
**Systeem:** — (de vlag komt in S03).

### S02 — *Into The Cold* · de nadering
`Kessara / Underworks / Coolant Sublevel` · walk-and-talk · tier 2
**Aanwezig:** Voss, Mara, Brick; Dex

- **want** — Get into a Veil listening post through the only door that is not a door.
- **obstacle** — The condenser galleries are loud, wet and cold, and the Veil chose them for exactly that reason.
- **turn** — Voss reads the building the way he was trained to read a foundry, and takes the lead without being given it.

**Beats:** (1) De koelkelder: condensorbanken, damp, ijsaanslag op leidingen in een stad waar het nooit koud is. **Het beste beeld van de act** — zet er de camera op (§21.2: graphic points). (2) Voss ziet waar het vermogen heen gaat. Hij zegt het in vaktaal en niemand vraagt wat het betekent (§18.8). (3) Mara laat hem voorgaan. Ze zegt er niets over. (4) Bricks eerste regel: iets over kou, en het gaat niet over kou. (5) Binnen.
**Keuze:** Dex meelopen (hij kan sneller met het log, maar hij is kwetsbaar) of thuislaten (veiliger, langzamer, en de speler moet in S03 zelf lezen). Kleine keuze, echte gevolgen in S03 en S06.

### S03 — *The Log* · de vondst
`Kessara / Underworks / Coolant Sublevel` · in-mission-radio · tier 2
**Aanwezig:** Voss, Mara, Dex (aanwezig of op de radio)

- **want** — Voss wants a number: how much does the Veil know.
- **obstacle** — The answer is on the screen in front of him and it is bigger than the question.
- **turn** — Ember's own operations are in the log by date — and so is a query nobody in this room ran.

**Drie vondsten, in deze volgorde:**

**1. De sector is gemarkeerd** *(verplicht)*
Ember staat erin. Met datums. De vroegste ligt ná M1.2 — dus M1.1 en M1.2 zagen ze niet, en alles daarna wel. **Dat is de motor van act 1 op een scherm**, en de speler leest zijn eigen voetsporen.

**2. De buitenstaander** *(verplicht — P2-b)*
Onder de Veil-queries staat er één die niet van de Veil is. Andere autorisatieketen, ouder, en er is nooit een rapport op gevolgd. Dex kan zeggen wat het *niet* is; niemand kan zeggen wat het wél is. **Twee regels, geen theorie.** Whisper bestaat in act 2; hier bestaat alleen zijn schaduw.
→ `Story.Clue.OutsideQuery`

**3. De afgewezen aanvraag** *(optioneel document, verplichte dialoogdrager — P4-c)*
Threx heeft een interdictiepakket aangevraagd voor deze sector: extra ploegen, permanente surveillance, de hele hamer. **Afgewezen.** Niet door een commandant — de afwijzing draagt een toewijzingsstempel. Middelen zijn elders nodig.
- **In act 1:** Dex maakt er een grap over dat zelfs de Veil een budget heeft. Mara ziet er ruimte in. Iedereen loopt door.
- **In act 4:** dit is het bewijsstuk. AEGIS gaf de jager geen middelen omdat een gecontroleerde opstand als drukventiel modelleerde.
- **AR-9-regel:** het document is optioneel, **de constatering is verplicht.** Dex leest de afwijzing hardop zodra de speler het scherm aanraakt.
→ `Story.Clue.AegisDenial`

**4. Het geredigeerde record** *(volledig optioneel — P2-d)*
Een personeelsrecord uit het huishouden van de Arbiter, ontpersoond en weggelakt. Eén regel van Dex die het niet begrijpt en doorgaat.
**Systeem:** `Story.Clue.ThrexKnowsSector`.

### S04 — *He Says A Name* · Threx, trede 3
`Kessara / Underworks / Coolant Sublevel` · cutscene · tier 2
**Aanwezig:** Voss, Mara · **THREX** (live verkeer)

- **want** — Voss wants to be gone before the post's watch rotates.
- **obstacle** — The live channel is still open and the man on it is talking about somebody they know.
- **turn** — He says an Ember name, kindly, and pronounces it correctly.

**Beats:** (1) Het live kanaal. (2) **Threx.** Vier tot zes regels, warm, vragend, en hij gebruikt de voornaam van **een van Embers acht** (AR-1) — de vechter die in M1.8 als eerste op de muur komt (AR-10). (3) Hij zegt iets aardigs over die persoon, en het is oprecht, en het is ondraaglijk (§18.4). (4) Hij noemt Voss **niet**. Dat is trede 4 en die is van M1.8. (5) Mara's reactie: de eerste keer in het spel dat ze bang klinkt. Eén regel. (6) Voss zegt hardop wat er nu moet gebeuren, en het is niet wat Mara denkt.
**AR-6, trede 3.** Verboden: Voss' naam, K-77, of enige toespeling op de val.
**Systeem:** `Story.Clue.ThrexVoice_1` → `Story.Clue.ThrexKnowsSector` bevestigd.

### S05 — *The Manifest* · Petra
`Kessara / Underworks / Coolant Sublevel` · in-mission-radio · tier 2
**Aanwezig:** Voss, Mara, Brick

- **want** — Mara wants to leave with the log and nothing else.
- **obstacle** — The post also holds a Veil transfer manifest, and it is a list of people, and one of the names is Petra Voss.
- **turn** — Voss stops being a technician reading a system and becomes a man reading a list.

**Beats:** (1) Het manifest — een overplaatsingslijst naar Blacksite K-77. (2) Namen. Veel namen. (3) **Petra.** (4) Voss zegt niets, of hij zegt haar naam. Beide werken; laat het een variant zijn. (5) **Brick leest mee en noemt geen enkele naam** — de enige keer in de act dat hij een lijst ziet en zwijgt, want dit is niet zijn lijst. Dat is een karakterbeat die alleen werkt als de speler zijn tic al kent. (6) Mara ziet Voss' gezicht en weet dat de missie zojuist veranderd is.
**Systeem:** `Story.Intel.K77Manifest`, `Story.Char.PetraLocated`.

### S06 — *Out Under Pressure* · extractie
`Kessara / Underworks / Coolant Sublevel` · in-mission-radio + callout · tier 2
**Aanwezig:** allen; Veil

- **want** — Get out of a Veil facility that now knows it has been entered.
- **obstacle** — The Veil fights in a way Ember has not seen: quietly, patiently, and without reinforcements.
- **turn** — They get out, and afterwards nobody can say why it was that easy.

**Beats:** (1) De post ontwaakt. (2) **Veil-vocabulaire onder druk:** ze schreeuwen niet, ze raken niet in paniek, ze coördineren. Contrast met M1.4's conscripten. (3) Damp, condens, slecht zicht — het beeld werkt hier ook mechanisch. (4) Eruit. (5) **P4-d-opmaat:** Voss zegt hardop dat er geen versterking kwam. Mara wuift het weg met dezelfde verklaring als in M1.2.S03. **Nu voor de tweede keer, en de speler herinnert zich de eerste.** (6) Dex, als hij mee is, zegt de zin die het dichtst bij de waarheid komt — en het is een grap.
**Dit is de dubbele tik van T4.** Eén keer is wereldopbouw. Twee keer is een patroon. Drie keer (M1.8) is een vraag. Vier keer (act 4) is het antwoord.

### S99 — *The Box With One Door* · debrief
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes, Sela, Brick

- **want** — Mara wants to raid Blacksite K-77 within the week, before the transfers ship.
- **obstacle** — Every single person in the room has a correct reason not to.
- **turn** — She overrules all of them, and the terrible thing is that she is not wrong either.

> **Deze scène bepaalt of Mara's dood tragedie is of plot.** Als de speler K-77 ondergaat, is act 1 mislukt. Hij moet erheen willen.

**Beats:**
(1) Het log op tafel. Wat de Veil weet. De datums.
(2) **Dex:** het is een doos met één deur. Hij heeft gelijk, en hij is de enige die het gebouw begrijpt.
(3) **Reyes:** hoeveel gewonden, hoeveel bedden, hoeveel er niet terugkomen. Klinisch, in cijfers. Zij heeft gelijk.
(4) **Sela:** de veertig nieuwe mensen zijn niet klaar; als Ember daar sterft, sterft er meer dan Ember. Zij heeft gelijk.
(5) **Brick:** één zin. Hij is de enige die vóór is, en zijn reden is dat er mensen in zitten. Hij heeft gelijk.
(6) **Voss** krijgt het woord — en dit is de eerste keer dat de speler een strategisch besluit *tegen* zijn mentor in kan nemen. Zijn keuze verandert de toon van M1.8, niet het feit ervan: Mara gaat hoe dan ook. Als Voss tegen is, gaat hij mee omdat zij gaat. Als hij vóór is, gaat hij omdat hij het wil. **Dat verschil draagt de hele nasleep.**
(7) **Mara sluit de discussie.** Haar argument: over drie dagen zijn die mensen weg, en de cel heeft nooit iets gered dat de moeite waard was. Dit is de haast die in M1.3.S99 geplant is, nu volgroeid.
(8) Laatste beeld: de kaart, K-77 erop gezet.

**Systeem:** `Story.Beat.M17_UnderTheIce`, `Story.Intel.K77Manifest` ontgrendelt M1.8, dag +1.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.M16_TitheTrain` · `Story.Clue.ThrexVoice_1` | `Story.Beat.M17_UnderTheIce` ✚ · `Story.Clue.ThrexKnowsSector` ✚ · `Story.Clue.OutsideQuery` ✚ · `Story.Clue.AegisDenial` ✚ · `Story.Intel.K77Manifest` ✚ · `Story.Char.PetraLocated` ✚ |

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T4** AEGIS liet het toe | **geplant, DRAGEND — de belangrijkste van het spel** (S03 P4-c, S06 de tweede dunne respons). |
| **T2** Whisper | **geplant, dragend** (S01 P2-c, S03 P2-b). Optioneel P2-d. |
| Threx-ladder AR-6 | **trede 3** (S04). |
| **T3** de mol | indirect: het log toont dat de Veil ná M1.2 begon te kijken. In act 3 wordt die tijdlijn tegen de M1.6-instroom gelegd. Geen dialoog nodig. |
| T1, T5 | niet aangeraakt. |

---

## 7. Groei

- **Voss** — weet voor het eerst meer dan Mara (S03), leidt zonder toestemming (S02), en neemt in S99 stelling. Aan het eind van deze missie is hij in alles behalve titel al medeleider, en de cel weet dat voordat hij het weet.
- **Mara** — haar tragische fout wordt zichtbaar en moet **sympathiek** zijn. Ze kiest hoop boven bewijs, en de reden is dat vijf jaar voorzichtigheid haar niets heeft opgeleverd. §18.4: korte declaratieven, "wij". In S04 klinkt ze één keer bang, en dat is de enige keer in de hele game.
- **Dex** — het gebouw begrijpen en niet gehoord worden. Zijn arc (*"machines are only as good as the people in them"*) draait hier de andere kant op: hij heeft gelijk over de machine en het maakt niets uit.
- **Reyes / Sela / Brick** — drie manieren om gelijk te hebben. Deze scène is hun eerste ensemblemoment en de reden dat de speler ze alle drie mist als het misgaat.

---

## 8. Instructies voor de dialogue-writer

1. **S99 is de belangrijkste scène die je schrijft.** Iedereen heeft gelijk. Niemand is dom. Als één personage een zwak argument krijgt, wordt Mara's besluit een fout in plaats van een tragedie.
2. **De afgewezen aanvraag (P4-c) wordt hardop gelezen en meteen weggelachen.** Niemand kijkt er twee keer naar. Het is de belangrijkste zin van act 1 en hij moet klinken als bijzaak.
3. **De buitenstaander-query krijgt twee regels en geen theorie.** Geen enkel personage speculeert wie het is.
4. **Threx noemt Voss' naam niet** (AR-6). Ook niet bijna.
5. **Mara's "een vriend van een vriend op de lanes" wordt niet verdedigd en niet benadrukt.** Eén regel, gesprek voorbij.
6. **Bricks stilte bij het manifest is zijn beat.** Schrijf de afwezigheid.
7. **De koelkelder is het mooiste beeld van de act.** Schrijf `shot:`-noten die dat pakken — dat is de haak waar de cinematic-pass en `21_quality_mandate` §21.2 aan hangen.
8. **Voss-varianten:** S05 (Petra's naam) en S99 (stelling nemen) krijgen volledige dekking. S02 en S03 neutraal.

## 9. Barks

**Veil onder druk** is nieuw en het belangrijkste vocabulaire van deze missie: kalm, coördinerend, geen paniek, korte codes, en — cruciaal — **geen versterkingsroepen.** Dat laatste is een bark-ontwerpkeuze met verhaalgevolgen: de speler moet later kunnen terughalen dat er nooit iemand om hulp riep. Dit hoort als expliciete instructie in de tier-1-barkset van de Veil.
</content>
