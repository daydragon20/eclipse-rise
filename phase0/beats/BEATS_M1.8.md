# BEAT-SHEET — M1.8 *Blacksite K-77*
*L1 | story-architect | 2026-07-31 | act 1, beweging IV | **act-climax***
*Canon: `02_story_bible.md` §2.9 (M1.8 — vast punt), §2.5 (Mara sterft aan het eind van act 1), §2.6 (Threx executeert Mara persoonlijk), §2.11 · `01_game_vision.md` §1.7 moment 3 · `19_voice_production.md` §19.3 (Threx' signature-regel)*

---

## 1. Dramatische functie

De bijbel geeft deze missie vier vaste feiten en ik verplaats er geen:

1. Petra en de gevangenen worden gered.
2. **Het is een val — Threx verwachtte hen.**
3. **Mara wordt geëxecuteerd terwijl ze de ontsnapping dekt.** Persoonlijk, door Threx.
4. Voss leidt de overlevenden eruit, en in de as stemmen de cellen om **"Cinder"** te volgen.

Verhaalfunctie:

> **De rekening voor zeven overwinningen — en de enige promotie die dit spel kent: iemand anders houdt op.**

Structureel is dit de scène waar de motor van act 1 zijn cirkel sluit. Elke overwinning maakte de cel zichtbaarder; de zichtbaarheid zette Ember op Threx' bureau (M1.7); het manifest dat de raid mogelijk maakte lag op dat bureau. **De speler heeft zichzelf hierheen gewerkt.** Dat moet in S99 nawerken zonder dat iemand het uitspreekt.

**Omvang:** M1.8 is bewust ongeveer tweemaal de gemiddelde act-1-missie (twaalf scènes). Dat is geen scope-creep maar de act-climax; de gemiddelden van M1.1–M1.7 dragen het. Zie C-6.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS** | overal |
| **MARA** | tot S08 |
| **PETRA VOSS** | vanaf S04 — **castingrij en fingerprint ontbreken, Q-3 / C-4 / C-5** |
| **THREX** | S05 (PA), S06 (persoonlijk), S08 (de executie) |
| **BRICK**, **DEX**, **REYES**, **SELA** | veld en/of Hollow Point |
| Iron Chorus-emissaris | S99 (de stemming) — **alleen bij pact** |
| Veil, Radiant Guard-achtige bewaking | barks |
| Gevangenen (3–4 sprekende bijrollen) | S03, S09 |

### Petra Voss — fingerprint VASTGESTELD *(ruling L1-R9, 2026-07-31)*
Staat nu in `18_writing_standard.md` §18.4, sectie *Recurring non-companion voices*. Signature-regel in §19.3: ***"Sit. Eat. Then tell me who died."*** **Schrijfbaar. Nog niet gecast** — dat is de owner-keuze die overblijft van Q-3.

| | |
|---|---|
| **Syntax** | Imperatieven **met weggelaten voornaamwoord**. Huishoudelijk/keukenvocabulaire waar iedereen om haar heen operationeel praat. Op één na kortste regels van het spel. |
| **Tic** | Beantwoordt een vraag die ze niet wil beantwoorden door een taak uit te delen. |
| **Nooit** | **Zegt nooit "je" of "jouw".** Laat zich nooit bedanken. Noemt nooit wat haar is aangedaan. |

Waarom dit werkt: het maakt haar het **tegengif voor de hele act**. Iedereen om haar heen praat in operaties; zij praat in maaltijden. En het maakt haar latere rol (§2.11: *"the hideout's quiet heart"*) mechanisch: ze deelt taken uit, en dat is precies wat een basis nodig heeft.

**Twee harde scheidingen, en ze zijn allebei controleerbaar:**
- **Tegen Brick:** hij geeft je een zelfstandig naamwoord, zij een werkwoord. Nodig voor de strip-test (§18.9 C1) — twee laconieke personages in één cast.
- **Tegen Mara, en dit is de dragende:** Mara spendeert vier scènes later in S08 de enige "jij" van het spel, en dat woord ís de commando-overdracht (AR-5b). Petra's imperatieven laten het voornaamwoord weg, dus het woord blijft schaars over de hele climax en Mara's ene gebruik landt in een vacuüm. **Een regex over S04 moet nul treffers geven op "you"/"your".**

**Ze noemt hem ook nergens bij naam** (ruling L1-R10). Zie §2b.

### 2b. De gedeelde achternaam — wat vastligt *(ruling L1-R10)*

Canon (§2.4): de ouders stierven bij de **Foundry Collapse van 484 AE**, die AEGIS boekte als *optimal loss allocation* — ontdekbaar in act 3. Voss was toen **zeven**; Petra was **36** toen ze hem overnam.

1. Ze draagt de naam **door bloed**. Wiens zus ze is blijft open — dat invullen kost canon en levert niets op.
2. **Zij is de laatste volwassene die de naam draagt.**
3. **De achternaam is de draad tussen de Foundry Collapse en de AEGIS-onthulling.** De doden die AEGIS als optimale verliesallocatie boekte, zijn de mensen wier naam de speler de hele game draagt. Die draad lag ongebruikt in de canon.
4. **Petra noemt de hoofdpersoon nergens iets.** Geen naam, geen "je" — alleen taken. §2.4 zet de achternaam vast *"for voiced dialogue"*, dus de voornaam wordt nooit ingesproken; iedereen zegt "Voss", vanaf M1.8 "Cinder". Zij is de enige levende die hem kende voordat een van beide woorden iets betekende. **Zo wordt de hardste VO-beperking van het spel een eigenschap in plaats van een gat.**
5. **De beat waar dit samenvalt is S99**, niet S04. Zie daar.

---

## 3. Site & runtime-haak — AR-3

**Blacksite K-77 ligt onder Kessara's Spire Levels**, bereikt via een servicelift vanaf Foundry Row. De glossary noemt K-77 zonder plaats; act 1 speelt op Kessara; `03_world_design.md` §3.3 geeft Kessara zijn laagjestaart. Thematisch: **het schone licht staat bovenop de cellen.** Productie: een eigen klein level, precies zoals §3.2 voorschrijft voor extra verhaalmissies.

- **`location`-strings:** `Kessara / Spire Levels / Service Lift` en `Kessara / Spire Levels / Blacksite K-77`
- **Regio-pin:** eigen level, ontgrendeld door `Story.Intel.K77Manifest`
- **Runtime:** breach/liberation. `InteractTarget` ×N (cellen) → `EscortGroup`/`ExtractSquad` onder oplopende druk; geauthorde subfases voor de val en de laatste deur. Optionals: alle gevangenen levend; geen Ember-verlies (**mag niet gegarandeerd haalbaar zijn** — zie §7).
- **Buiten SPEC-P2-04.**

---

## 4. Scènelijst

### S01 — *Who Stays* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2

- **want** — Mara wants a plan that puts the fewest people inside the building.
- **obstacle** — The fewest people is still more than Ember can afford to lose, and everyone in the room volunteers.
- **turn** — She assigns Voss the extraction route instead of the entry, which is the first time she has ever kept him out of a door.

**Beats:** (1) Dex' plattegrond, gebouwd uit het manifest en giswerk. (2) Wie gaat, wie blijft — en Sela blijft, met de veertig nieuwen. Dat is een besluit dat later telt. (3) **Mara geeft Voss de extractieroute.** Ze legt niet uit waarom. De speler voelt het pas terug in S08. (4) Reyes gaat mee; het is de eerste keer, en ze zegt waarom in één klinische zin. (5) Brick: één zin, en het is een naam.
**Kijk uit:** niemand neemt afscheid. §18.7 — begin laat, eindig vroeg. Een afscheidsscène vóór de dood is de goedkoopste truc die er is.

### S02 — *The Descent* · de lift
`Kessara / Spire Levels / Service Lift` · walk-and-talk · tier 2

- **want** — Get eight people down a service shaft into the cleanest part of the city.
- **obstacle** — Nobody in this squad has ever been above the Mid-Works, and the light up here is wrong.
- **turn** — The lift stops at an administrative floor by accident, the doors open on a corridor of people going home from work, and nobody looks up.

**Beats:** (1) De lift; het licht verandert van amber naar witgoud. (2) **De deuren openen op de verkeerde verdieping.** Kantoormensen. Jassen. Iemand met een lunchtrommel. Ze kijken niet op. (3) De deuren sluiten. (4) Eén regel, van wie dan ook. **Één.** (5) Verder omlaag.
**Dit beeld is de hele Dominion in tien seconden** en het kost één scène-beat. `21_quality_mandate` §21.2: dit is een graphic point.

### S03 — *The Cells* · het blok
`Kessara / Spire Levels / Blacksite K-77` · in-mission-radio · tier 2

- **want** — Open every door on the manifest before the shift changes.
- **obstacle** — Some of the people inside have been there long enough that an open door means nothing to them.
- **turn** — Voss counts the guards, and the number is too small, and he says so — and Mara tells him to keep opening doors.

**Beats:** (1) Het blok. Licht, schoon, stil. Geen kerker — een **kantoor met cellen erin**, en dat is erger. (2) Deuren open. Sommigen komen niet. (3) Eén gevangene die vraagt of het over is, en de vraag betekent iets anders dan het lijkt. (4) **P4-d:** Voss telt de bewaking. Voor wat Threx wist, staan hier veel te weinig mensen. **Hij zegt het hardop.** (5) **Mara wil het niet horen.** Ze zegt de derde variant van haar M1.2-verklaring, en dit keer klinkt hij dun.
**Dit is de derde tik van T4** en het is de laatste keer dat de speler eraan herinnerd wordt vóór act 4.

### S04 — *Petra* · de hereniging
`Kessara / Spire Levels / Blacksite K-77` · cutscene · tier 2

- **want** — Voss wants his aunt to walk out of a room she has been in for months.
- **obstacle** — She is not the person the prologue remembers, and she starts by giving orders.
- **turn** — He stops trying to comfort her and does what she says, and she lets him take her arm.

**Beats:** (1) De cel. Ze zit. Ze staat op voordat hij iets zegt. (2) **Haar eerste regel is een taak** ("pak die deken", "help hem eerst"). Ze bedankt niet en ze huilt niet. (3) Voss probeert het emotionele register. Het werkt niet. (4) Ze wijst naar iemand anders die hulp nodig heeft. (5) **Pas als hij doet wat ze zegt, laat ze zich vasthouden.** (6) Eén regel die de proloog aanraakt zonder hem te citeren.
**Dit is de betaling van de hele proloog** en hij duurt negentig seconden. Geen enkele regel benoemt wat er gebeurd is.

### S05 — *The Door Closes* · de val
`Kessara / Spire Levels / Blacksite K-77` · cutscene → in-mission-radio · tier 2

- **want** — Get everyone to the extraction route Mara assigned Voss.
- **obstacle** — The route is already closed, and the man closing it says so politely over the public address.
- **turn** — Threx uses Voss's first name, and Ember learns in one word how long he has been watching.

**Beats:** (1) De eerste deur die niet opengaat. (2) Dex, op de radio, die begrijpt wat het betekent voordat iemand anders het zegt. (3) **Threx op de PA.** Rustig, vriendelijk, en hij noemt drie mensen bij hun voornaam: één gevangene, één Ember-vechter (dezelfde als M1.7.S04) en **Voss**. (4) **AR-6, trede 4.** (5) De containment begint.
**De schrijver mag Threx hier niet laten opscheppen.** Hij feliciteert hen. Hij meent het.

### S06 — *Threx* · de confrontatie
`Kessara / Spire Levels / Blacksite K-77` · cutscene · tier 2

- **want** — Voss wants a door open and there is a man standing on the other side of the glass who can open it.
- **obstacle** — He is not going to, and he would like to talk first, and he is in no hurry at all.
- **turn** — He tells Voss something true about Petra, kindly, and it works — Voss loses ten seconds, and ten seconds is what the containment needed.

**Beats:** (1) Glas of een dichte deur met een spreeknet. Threx aan de andere kant, alleen, ongewapend. (2) Hij begint met een compliment over de operatie, en het is verdiend. (3) **De signature-regel van §19.3 valt hier:** *"You have your aunt's hands. Did anyone ever tell you that?"* — dit is de canonieke plaats. (4) Hij stelt drie vragen en beantwoordt er geen. (5) **Voss krijgt spreekopties**, en geen ervan wint. Dat is essentieel: dit is een dialoogconfrontatie die je niet kunt winnen, tien uur voordat het spel er een geeft die je wél kunt winnen (Vex, act 4). De vorm rijmt; de uitkomst niet. (6) Threx loopt weg omdat hij ergens anders moet zijn. **Dat is de belediging.**
**Valkuil:** geen dreigementen, geen monoloog, geen ideologie. Threx doet pijn met warmte. Als hij één keer dreigt, is hij een gewone schurk en verliest act 3 zijn catharsis.

### S07 — *The Corridor* · de terugtocht
`Kessara / Spire Levels / Blacksite K-77` · in-mission-radio + callout · tier 2

- **want** — Move forty people who cannot run down a corridor that is being closed from both ends.
- **obstacle** — Ember is a squad; this is a crowd; and Sela — the one person who can move a crowd — is at Hollow Point.
- **turn** — Somebody from Ember's original eleven dies holding a door, and the crowd moves because they saw it.

**Beats:** (1) Het gevecht, in korte callouts. (2) Gevangenen die niet kunnen rennen. Reyes onder druk — **haar samentrekkingen verdwijnen** (§18.4: training reasserts). Dit is het duidelijkste fingerprint-moment van de hele act; laat het niemand benoemen. (3) Brick draagt. (4) **De eerste naam op de muur:** iemand van de acht sterft, en het is dezelfde naam die Threx in M1.7.S04 uitsprak (AR-10). Systemisch mag ook een rekruut sterven — `{name}`-slot — maar **deze ene dood is authored.** (5) Verder.

### S08 — *Mara* · het vaste punt
`Kessara / Spire Levels / Blacksite K-77` · cutscene · tier 2

- **want** — Mara wants the last door held long enough for forty people to get past it.
- **obstacle** — It only holds from this side.
- **turn** — She says "you" for the first time in the game, and Voss understands the whole sentence before she has finished it.

**Beats:**
(1) De laatste deur. Het mechanisme — **Dex heeft het in S01 uitgelegd zonder dat iemand luisterde.** Dat is de opzet: de speler wist het al.
(2) Er is geen discussie. Mara zegt niet "ga". Ze doet iets praktisch en het is te laat om het terug te draaien.
(3) **AR-5 — de enige "jij" in het spel.** Mara zegt het hele spel "wij". Hier zegt ze één zin met "jij" erin, en die zin is de overdracht van het commando. Ze zegt niet dat hij de leider is; ze **gebruikt een woord dat ze nooit gebruikt**, en dat is genoeg.
(4) Voss' antwoord. **Varianten op beide assen** — dit is de duurste en belangrijkste variantset van de act.
(5) De deur sluit.
(6) **De executie.** Threx doet het persoonlijk (§2.6). De speler ziet het door glas, of ziet het niet en hoort het over de radio die aan blijft staan. **Aanbeveling: het tweede.** Het is erger, het is goedkoper, en het respecteert §18.9 — je toont geen gevoel, je laat een kanaal open staan.
(7) **Geen laatste woorden na de schoten.** Geen sterfscène-monoloog.

> **Harde regels voor deze scène:**
> - Mara benoemt geen enkel gevoel. Nul `direct_beat` (§18.9 A).
> - Ze zegt niet "ik ben trots op je", niet "leid ze", niet "dit is jouw taak nu".
> - **De hele overdracht zit in één voornaamwoord.** Als de schrijver dat niet vertrouwt, is de scène al kapot.
> - Voss krijgt geen afscheidsregel die werkt. Hij wordt onderbroken, of de deur is al dicht.
> - Maximaal veertien regels in de hele scène.

**Systeem:** `Story.Char.MaraDead`. **Vanaf hier speelt geen enkele live Mara-regel meer af, ooit.** Alleen opnames (S91, en acts 2–4).

### S09 — *The Ash* · buiten
`Kessara / Spire Levels / Service Lift` → `Kessara / Underworks / Hollow Point` · in-mission-radio · tier 2

- **want** — Voss wants a head count.
- **obstacle** — Nobody wants to give him one.
- **turn** — Petra gives it, out loud, with the numbers, because somebody has to and she has done it before.

**Beats:** (1) Boven; de lift; veertig mensen die niet weten waar ze heen moeten. (2) Stilte. (3) Voss vraagt de telling. (4) Niemand antwoordt. (5) **Petra telt.** Namen en aantallen, plat, zonder troost. Haar fingerprint doet hier al het werk. (6) Reyes begint te werken. (7) **P4-d, laatste tik:** iemand — Dex, of Brick — merkt op dat er niemand achter hen aan kwam. Niemand heeft er een antwoord op. **Deze vraag blijft open tot act 4.** Dat is geen wees; het staat in de plant/payoff-tabel met een betaalplek.

### S90 — *The Wall* · de plaat
`Kessara / Underworks / Hollow Point — Bunk Row` · cutscene · tier 2

- **want** — Brick wants a name where people will see it every day.
- **obstacle** — There is nowhere in a geothermal vault that is meant for that.
- **turn** — He welds a bunk plate to the wall and etches the name, and nobody asked him to, and by morning there are three more.

**Beats:** (1) Het geluid van lassen, 's nachts, in een basis die niet slaapt. (2) Brick, alleen. (3) Voss komt kijken en zegt niets. (4) **Brick noemt de naam.** Eén keer. (5) Anderen komen erbij. Geen ceremonie.
**Dit is `01_game_vision.md` §1.7 moment 3, letterlijk**, en het is de geboorte van de Memorial Hall (`05_base_building.md` §5.3: *"Grief is never a purchase"*).
**Systeem:** `Story.Thread.WallOpen`.

### S91 — *The Letter* · Mara's stem
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2

- **want** — Dex wants to shut down Mara's console before somebody else opens it.
- **obstacle** — There are eleven recordings on it, dated, addressed to "the next of us".
- **turn** — He plays the first one, because he is the only person in the vault who could not stand to leave it unheard, and it is not addressed to Voss.

**Beats:** (1) De console. Dex alleen, dan Voss. (2) De lijst met opnames. **Elf.** Zoveel als de cel groot was. (3) Dex' grap, en hij haalt hem niet. (4) **De eerste opname speelt.** Mara, kort, praktisch, zonder pathos — en hij is gericht aan iemand anders dan Voss, wat hem oneindig veel beter maakt. (5) Voss laat hem afspelen.
**Dit is de canon-plant voor §2.5:** *"Her recorded 'letters to the next of us' appear through Act 4."* **Zonder deze scène komen die brieven in act 2 uit het niets.**
**Als HUB.A1.mara_letters gespeeld is** (waar Voss haar er één zag opnemen), landt dit tienvoudig. **Voorwaardelijke regel toevoegen** — dat is precies waar `condition:` voor bestaat.
**Systeem:** `Story.Thread.MaraLetters_Open`.

### S99 — *Cinder* · de stemming
`Kessara / Underworks / Hollow Point` · cutscene · tier 2

- **want** — Three cells' worth of survivors need to know who they are following before morning.
- **obstacle** — Nobody in the room wants the job, and the only person who ever wanted it is dead.
- **turn** — Somebody uses a name that was not a name an hour ago, and it goes around the room, and by the end of it Voss answers to it.

**Beats:**
(1) De vault, vol. Ember, de Iron Chorus (bij pact), de bevrijden, de veertig.

> **DE DRIE CELLEN — ruling L1-R24 (01-08). Dit stond hierboven al impliciet en is één keer verkeerd gelezen.** Canon §2.9 eist *"the loyalty of three fused cells"* aan het eind van de act. Dat zijn **Ember**, **de veertig** (Sela's instroom van de Tithe-trein, M1.6) en **de bevrijden** (de gevangenen uit K-77, Petra erbij). Alle drie onvoorwaardelijk, in alle drie de pact-takken. **De Iron Chorus is er geen** — de enige groep in beat 1 met een voorwaarde eraan, en de scène zegt het in zijn eigen regel (`.040`: *"Three cells and one door out. The Chorus sleeps somewhere else."*). Fusie met de Chorus is een act-2-vraag (`ACT1_OVERVIEW` §5), en L1-R13 hangt de hele act-2-vertakking op aan full-vs-limited. **Een Chorus die in act 1 versmelt, sloopt L1-R13.**
(2) **Sela spreekt.** Kort — geen oration (AR-7). Zij stelt de vraag die niemand durft: en nu?
(3) De ruzie. Iron Chorus wil hun eigen lijn; sommigen willen weglopen; sommigen willen wraak.
(4) **P3-d:** iemand vraagt hoe Threx het wist. **Er komen drie antwoorden en geen conclusie:** de instroom, de informantennetten van de Veil, of pech. Het gesprek gaat verder. **Niemand lost het op.** Dat is de plant voor twist 3 én twist 4, en de speler moet met woede blijven zitten die geen adres heeft.
(5) **De naam.** Iemand uit de menigte — geen hoofdpersoon — gebruikt het woord *Cinder*. Het komt uit hoe hij eruitzag toen hij bovenkwam, of uit iets wat Mara ooit zei. **Voss noemt zichzelf nooit zo.**
(6) **Brick herhaalt het.** Daardoor blijft het plakken: de man met de minste woorden gebruikt er één, en dat is het zwaarste woord in de kamer.
(7) De stemming — expliciet, met handen of stemmen. Canon: *"the surviving cells vote to follow 'Cinder'."*
(8) **Het sigil.** Iemand heeft het op de muur geschilderd. Niemand weet wie. De beweging heeft een naam. Laatste beeld.

**Systeem:** `Story.Beat.M18_BlacksiteK77`, `Story.Char.CinderNamed`, `Story.Char.PetraRescued`, `Story.Beat.Act1Complete`.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Intel.K77Manifest` · `Story.Char.PetraLocated` · `Story.Choice.M15_IronChorusPact` · `Story.Thread.MaraLetters_Open` (hub) | `Story.Beat.M18_BlacksiteK77` ✚ · `Story.Char.MaraDead` ✚ · `Story.Char.PetraRescued` ✚ · `Story.Char.CinderNamed` ✚ · `Story.Thread.WallOpen` ✚ · `Story.Thread.MaraLetters_Open` ✚ · `Story.Beat.Act1Complete` ✚ |

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T4** AEGIS | **geplant, dragend** (S03 P4-d, S09 laatste tik). De vraag "waarom kwam er niemand" verlaat act 1 onbeantwoord — met betaalplek in act 4. |
| **T3** de mol | **geplant, dragend** (S99 P3-d): drie verklaringen, geen conclusie. |
| Threx-ladder AR-6 | **trede 4, twee bewegingen — L1-R23.** 4a in het openbaar over de omroep (S05), 4b in persoon en alleen (S06). **De ladder voltooit in `S06.040`** — *"You have your aunt's hands."* De as is afstand, niet naamvorm: de naamvorm loopt vast na trede 3, want Voss' voornaam is permanent onspeelbaar (L1-R10 punt 4). |
| **T2** Whisper | niet aangeraakt. Act 1's Whisper-planten liggen in M1.2 en M1.7. |
| **T5** Kaine | niet aangeraakt. |

## 7. Draden

| Draad | Handeling |
|---|---|
| **Petra Voss** | **BETAALD** (S04). Loopt door als het stille hart van de basis. |
| **Letters from the Wall** | **GEOPEND** (S90). |
| **Mara's brieven** | **GEOPEND** (S91) — canon §2.5, loopt tot act 4. |
| **Iron Chorus** | doorlopend (S99). |
| **The Enforcer** | niet aangeraakt. Hij leeft. |

---

## 8. Ontwerpnoot — de dood die het spel niet garandeert

`01_game_vision.md` §1.7 moment 3 zegt: *"scripted-systemic design means at least one recruited (non-plot) soldier likely dies."* **Likely, niet certainly.** Dat is de goede keuze en het script moet er tegen kunnen:

- **S07's authored dood** (een van de acht, de naam die Threx uitsprak) is **gegarandeerd** en draagt S90.
- **Systemische doden** daarbovenop zijn optioneel en gebruiken het `{name}`-slot (§18.5 regel 6).
- **Niemand mag een regel schrijven die een specifiek aantal doden veronderstelt.** S09's telling is dus geen getal in de dialoog maar een handeling: Petra telt, de camera niet.

Dit is precies het soort ding dat een schrijver stilletjes kapotmaakt door "we hebben er vier verloren" te schrijven.

---

## 9. Groei

- **Voss → Cinder.** De promotie is geen kroning maar een leegte die hij invult. Hij vraagt er niet om, hij krijgt hem niet, en hij **antwoordt op een naam** — dat is de hele beat.
- **Mara** — voltooit haar arc met één voornaamwoord. Zij is het bewijs van de stelling waar het spel over gaat: mensen volgen mensen, niet plannen.
- **Petra** — komt terug als iemand die de speler niet kende. Haar arc is niet herstel, het is **nut**: ze begint meteen te werken.
- **Brick** — de muur is zijn idee en niemand vroeg erom. §2.5: *"he remembers every name on the wall."* Vanaf nu is dat letterlijk waar en het begon met zijn lasapparaat.
- **Reyes** — verliest haar samentrekkingen in S07 en krijgt ze in S09 terug. Dat is haar hele karakter in twee scènes.
- **Dex** — kan de grap niet halen (S91). Twee keer in de act laat hij een opening liggen (M1.3.S06 en hier), en de tweede keer weet de speler wat dat betekent.
- **Sela** — houdt de kamer bij elkaar in S99 zonder een toespraak te houden. Dat is de opzet voor haar act-3-oration: dan mag ze wél.

---

## 10. Instructies voor de dialogue-writer

1. **Lees §18.9 A twee keer voor je S08 schrijft.** Elke verboden constructie in dat lijstje is uitgevonden voor precies deze scène. Geen enkele ervan komt erin.
2. **S08 heeft maximaal veertien regels** en de belangrijkste is een voornaamwoord.
3. **Threx dreigt nooit.** Ook niet in S06. Ook niet als het verleidelijk is.
4. **Niemand neemt afscheid in S01.**
5. **Petra bedankt niemand, ooit.**
6. **De executie is beter gehoord dan gezien.**
7. **Niemand noemt Voss "Cinder" behalve in S99**, en Voss zelf nooit.
8. **P3-d blijft onopgelost.** Wie in S99 een verklaring laat winnen, sloopt twist 3.
9. **De ~30 `direct_beat`-regels van het spel:** ik geef er **één** uit in act 1, en de plek is **S09 of S90** — de schrijver kiest welke, met een `note:` erbij. Nergens anders in de act, en niet in S08. In de scène waar iedereen een gevoel zou benoemen, benoemt niemand er een; in de stille scène erna mag één personage één keer plat zeggen wat er is. Dat is het rantsoen goed besteed.
10. **Voss-varianten:** S06 (de opties tegen Threx) en S08 (het antwoord) krijgen volledige dekking op beide assen. S04 en S99 krijgen een halve set. Dit is de duurste missie van de act en dat is hier terecht.

## 11. Barks

**Veil binnen** (kalm, coördinerend, geen versterkingsroepen — de regel uit M1.7 blijft staan en betaalt hier), **gevangenen** (verward, niet dankbaar), **containment-tellers**. Squad-barks draaien in S07 naar hun donkerste variantenset: *mate down* met een naam die de speler zeven missies lang gehoord heeft. Dit is de missie waarvoor §18.5 regel 6 geschreven is.
</content>
