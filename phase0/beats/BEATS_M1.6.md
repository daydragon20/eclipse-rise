# BEAT-SHEET — M1.6 *The Tithe Train*
*L1 | story-architect | 2026-07-31 | act 1, beweging III*
*Canon: `02_story_bible.md` §2.9 (M1.6), §2.3 (Tithe of Hands), §2.5 (Sela), §2.7 (The Cost of Speed), §2.11 (The Conscript Letters) · `19_voice_production.md` §19.3 (Sela's signature-regel)*

---

## 1. Dramatische functie

Bijbel: *"Free conscripts → first recruitment wave; meet Sela."* Verhaalfunctie:

> **De missie waarin de rebellie ophoudt een inbrekersbende te zijn en een zaak wordt — en de missie waarin Ember voor het eerst meer mensen heeft dan Mara bij naam kan kennen.**

Dat tweede is het scharnier van beweging III en de deur waar twist 3 doorheen loopt. Tot nu toe was Ember elf mensen die elkaar in het donker herkenden aan hun ademhaling. Na deze nacht zijn het er tientallen, en niemand van hen is nagetrokken. Iron Chorus waarschuwde daar in M1.5.S02 al voor. **Dat is geen toeval; het is de opzet.**

Structureel is dit de missie die de zwaarste morele last van act 1 draagt, en de bijbel geeft er twee archetypen aan (§2.7): *The Cost of Speed* en de vraag wat je doet met mensen die je bevrijdt zonder dat ze erom vroegen.

**Zichtbaarheid na M1.6: 7/8.** Een treinaanval op de Tithe is geen politiezaak meer. Vanaf hier is Ember een dossier op Threx' bureau, en M1.7 laat de speler dat dossier lezen.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS**, **MARA** | veld |
| **BRICK** | veld — **dit is zijn missie**, ook al zegt hij bijna niets. De Tithe heeft hem van Krad-9 gehaald (AR-11) |
| **SELA VANN** | in de wagons — **debuut** |
| Iron Chorus-vechters | veld, **alleen als `Story.Choice.M15_IronChorusPact` waar is** |
| Dominion-conscriptbewaking | barks |
| bevrijde conscripten (3–4 sprekende bijrollen) | wagons + instroom — **castingsleutels ontbreken, Q-5** |
| **DEX**, **REYES** | Hollow Point; Reyes krijgt S06 en S99 |

### Sela — de canon die je moet kennen voor je haar schrijft
- 23, Kessaraanse arbeidsorganisator, later coalitiepoliticus (§2.5). Bindt reputatie, allianties, burgerbeleid. *"The movement's conscience."*
- **Fingerprint (§18.4):** retorische structuur, ook in privé. Tweede persoon meervoud. Tic: herformuleert een persoonlijke vraag als een collectieve. Nooit: meer dan één regel over zichzelf.
- **Signature-regel (§19.3):** *"They didn't take your brother. We let them."* — **die hoort in S03 en nergens anders.** Het is het eerste wat de speler haar hoort zeggen, en ze zegt het niet tegen hem.
- **AR-7:** geen oration in act 1. Sela blijft binnen de hub-band (12–35 woorden per regel). Haar oration komt in act 3.

---

## 3. Site & runtime-haak

- **Fictie:** de Tithe-lijn is een goederenspoor dat de Mid-Works met de haven verbindt; de conscriptenwagons zijn omgebouwde erts-containers met roosterluiken. De trein rijdt naar het **muster yard** bij het Supply Depot, waar de loting wordt "uitgevoerd" — mensen worden geteld, gemerkt en verscheept.
- **`location`-strings:** `Kessara / Mid-Works / Tithe Line` en `Kessara / Mid-Works / Supply Depot Yard`
- **Regio-pin:** `SupplyDepot` (na M1.3's Foothold in spelershanden — fictie: de trein rijdt dóór spelersgebied, en dat is precies waarom de aanval mogelijk is en waarom hij gezien wordt)
- **Runtime:** konvooi-aanval. `ReachLocation` (boarden) → `CollectItem`/`InteractTarget` (wagonsluiten) → keuzefase → `ExtractSquad` (of `HoldPosition` in de doorrijd-tak). Optional: geen bevrijde conscript verloren.
- **Buiten SPEC-P2-04.** Een rijdend voertuig is een systeemvraag. **Fallback zonder nieuwe systemen:** de trein staat stil op een wisselplaats en de druk komt van een klok in plaats van van beweging. Dat verandert niets aan de scènes hieronder. Melden, niet omzeilen.

---

## 4. Scènelijst

### S01 — *Not For Supplies* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes, Brick, de cel

- **want** — Mara wants to hit a train that carries nothing Ember can use.
- **obstacle** — Every previous target paid for itself, and this one costs materials, ammunition and probably people.
- **turn** — Brick speaks first, for the only time in the act, and after that nobody argues.

**Beats:** (1) Wat de Tithe is, verteld door wat hij elke maand meeneemt — nooit uitgelegd (§18.8). (2) Dex' bezwaar, en het is een goed bezwaar: dit levert niets op. (3) Reyes rekent uit hoeveel gewonden een treinaanval kost. (4) **Brick spreekt als eerste.** §18.4 zegt: *"Never speaks first in a group."* Deze ene keer wel, en de schrijver mag daar niets bij uitleggen. Twee zinnen maximaal. (5) Stilte. (6) Mara zet het op de kaart.
**AR-11 betaalt hier voor het eerst.** Brick is door de Tithe van Krad-9 gehaald. Hij zegt dat niet. Hij zegt iets anders, en de speler weet het pas zeker in S03.

### S02 — *Boarding* · de aanval
`Kessara / Mid-Works / Tithe Line` · in-mission-radio + callout · tier 2
**Aanwezig:** allen; Iron Chorus indien pact

- **want** — Get onto a moving train without hitting the cars that have people in them.
- **obstacle** — Every car looks the same from outside, and the guards know which ones do not.
- **turn** — The guards defend the empty cars, which tells Ember exactly where everyone is.

**Beats:** (1) Aanloop; de trein. (2) Boarden. (3) De bewaking verdedigt de verkeerde wagons — **een tactisch inzicht dat uit vijandelijk gedrag komt, niet uit een marker.** Dat is `11_missions.md` §11.3's causaliteitsregel toegepast op een gevecht. (4) Bij pact: de Iron Chorus doet zijn deel, efficiënt en zonder zorg voor de bewaking. Eén beeld. (5) De sluiten van wagon één.
**Bark-vereiste:** conscriptbewaking die weet dat er mensen achter haar zitten klinkt anders dan een garnizoen. Nieuwe variant binnen het Dominion-vocabulaire.

### S03 — *The Cars* · Sela
`Kessara / Mid-Works / Tithe Line` · cutscene · tier 2
**Aanwezig:** Voss, Brick, Mara; **Sela**; drie conscripten

- **want** — Voss wants the cars open and everybody out.
- **obstacle** — A third of the people inside do not want to be out, because their families are on a list and the list has an address.
- **turn** — Sela stops arguing with Voss and starts arguing with them, and wins — and Ember has just found the only weapon it does not know how to build.

**Beats:** (1) Het luik open. De geur, het licht, te veel mensen die niet bewegen. (2) De eerste conscript die zegt dat hij blijft. Zijn reden is goed. (3) Voss probeert het. Het werkt niet — **en dat is belangrijk: Voss kan dit niet.** (4) **Sela.** Ze spreekt niet tegen Voss. Ze spreekt tegen de wagon. Haar signature-regel valt hier: *"They didn't take your brother. We let them."* (5) De wagon komt in beweging. (6) **Brick.** Hij staat in het luik en zegt drie of vier woorden en ze gaan over Krad-9, en de speler snapt nu waarom hij als eerste sprak in S01.
**Dit is de beste scène van beweging III en waarschijnlijk de op één na beste van de act.** Zij bewijst pillar 3 (PEOPLE, NOT UNITS) zonder één statistiek.
**Systeem:** `Story.Char.SelaMet`.
**Valkuil:** Sela mag geen toespraak houden (AR-7). Ze overtuigt met vier tot zes korte beurten, waarvan minstens twee vragen zijn. Retoriek is structuur, geen volume.

### S04 — *The Cost of Speed* · de keuze
`Kessara / Mid-Works / Tithe Line` · cutscene · tier 2
**Aanwezig:** Voss, Mara, Sela, Brick

- **want** — Voss needs every person on this train somewhere the Dominion cannot reach tonight.
- **obstacle** — There is one train, two routes, and no version where everybody is safe.
- **turn** — He chooses, and Sela tells him the price of the choice before he has finished making it — which is the beginning of the only relationship in the game that never becomes comfortable.

**De keuze (§2.7 *The Cost of Speed*):**
- **Doorrijden naar de Underworks-aftakking** — langzamer, iedereen mee, maar de trein is een doelwit en het gevecht in S05 wordt zwaarder. Risico op burgerdoden.
- **Stoppen en legen** — sneller, veiliger voor Ember, maar een deel van de mensen wordt op het emplacement opnieuw opgepakt.
- **Splitsen** (alleen bij Iron Chorus-pact) — de Iron Chorus neemt de tweede wagon en Ember weet niet waar die mensen heen gaan.

**Beats:** (1) De wissel komt eraan. (2) Mara geeft de keuze aan Voss — **derde keer** (na M1.1.S05 en M1.5.S04) en de speler moet het patroon nu bewust voelen. (3) Sela's rekening: wie er betaalt bij elke optie, in haar vocabulaire (tweede persoon meervoud). (4) De keuze. (5) Eén regel van Brick.
**Systeem:** `Story.Choice.M16_TrainRun`.

### S05 — *The Yard* · de respons
`Kessara / Mid-Works / Supply Depot Yard` · in-mission-radio + callout · tier 2
**Aanwezig:** allen

- **want** — Get several dozen unarmed people through a rail yard.
- **obstacle** — Unarmed people do not move like a squad and cannot be ordered.
- **turn** — Sela moves them, using her voice where Ember would have used cover.

**Beats:** (1) Het emplacement; zoeklichten; de respons. (2) **Het tactische probleem is nieuw:** burgers zijn geen eenheden. Dat is pillar 3 in mechaniek. (3) Sela organiseert ze — geroep, korte instructies, namen. (4) Ember dekt. (5) Verliezen, als die er zijn, zijn zichtbaar en hebben gezichten.
**Bij doorrijden:** zwaarder gevecht, meer druk, meer kans op burgerverlies. **Bij stoppen:** korter gevecht, en op de terugweg ziet de speler de mensen die niet meekwamen, gemerkt en geteld. **Beide takken doen pijn en dat is de bedoeling** (§2.7 regel 2).

### S06 — *What We Do With Them* · de instroom
`Kessara / Underworks / Hollow Point` · hub · tier 2
**Aanwezig:** Mara, Voss, Sela, Reyes, Dex, Brick, tientallen nieuwkomers

- **want** — Mara wants forty strangers fed, bunked and useful by morning.
- **obstacle** — Ember has no way to know who any of them are, and the only person who could check them all is standing in the room asking her not to.
- **turn** — She admits out loud that they cannot vet anybody, decides to take them all anyway, and Sela wins the second argument of the night.

> **Dit is de belangrijkste scène van act 1 voor twist 3, en hij mag nergens naar ruiken.**

**Beats:** (1) De vault, te vol. Dekens, tellingen, iemand die huilt en iemand die slaapt. (2) Dex' logistieke paniek — de voorraadadministratie uit M1.4.S99 is nu een echte baan, en hij wijst iemand aan. **Eén regel, terloops, geen naam die de camera vasthoudt.** (P3-a → P3-c, de deur staat open.) (3) **Mara zegt hardop dat ze niemand kan natrekken.** Ze weegt het tegen wat de Iron Chorus zou doen. Ze kiest voor mensen. **Dat is geen fout — het is haar karakter, en het is de reden dat de speler in act 3 niet weet wie hij moet haten.** (4) Reyes: bedden, water, ziektes. Klinisch, en de enige die de aantallen zegt. (5) **P5-c:** een bevrijde conscript vraagt of hij naar huis mag schrijven — zijn moeder denkt dat hij dood is. Mara zegt nee (opsec). Sela zegt ja. **De speler beslist.** (6) **De shift-baas-betaling:** Sela weet wat er in M1.5.S04 gebeurd is — de wijk weet het — en ze zegt er precies één ding over. Bij sparen: een compliment dat niet als compliment klinkt. Bij doden: geen verwijt, maar een feit dat ze onthoudt.
**Systeem:** `Story.Flag.IntakeUnvetted` (stil), `Story.Choice.M16_LettersAllowed`, `Story.Thread.ConscriptLetters_Open`.
**Valkuil, en dit is de grootste van de act:** géén enkele nieuwkomer krijgt een naam, een close-up of een verdachte regel. Wie hier de mol "voorbereidt" met een blikwisseling, verpest twist 3. **De deur staat open. Er loopt niemand doorheen die de camera volgt.**

### S99 — *More Mouths* · debrief
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes, Sela, Brick

- **want** — Mara wants to know what Ember is now, since it is plainly not a cell any more.
- **obstacle** — Nobody in the room has a word for it, and Sela has three.
- **turn** — Voss picks the smallest of her three words, and it sticks.

**Beats:** (1) De cijfers: hoeveel mensen, hoeveel eten, hoeveel dagen. Dex' realiteit. (2) Reyes' lijst met wat ze nodig heeft en niet krijgt. (3) Sela's plaats in de kamer: ze is er nog, en niemand heeft haar gevraagd te blijven. (4) **Voss vraagt haar te blijven** — of niet; laat dat een keuze zijn. (5) Het woord voor wat Ember nu is. **Nog niet "Eclipse"** — dat komt in M1.8. Iets kleiners en menselijkers. (6) Mara, alleen bij de kaart, laatste beeld. Geen regel.
**Systeem:** `Story.Beat.M16_TitheTrain`, `Story.Char.SelaMet`, wervingsinstroom (+recruits), dag +1.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.M15_Cells` · `Story.Choice.M15_IronChorusPact` · `Story.Choice.M15_ShiftBossSpared` | `Story.Beat.M16_TitheTrain` ✚ · `Story.Char.SelaMet` ✚ · `Story.Choice.M16_TrainRun` ✚ · `Story.Choice.M16_LettersAllowed` ✚ · `Story.Thread.ConscriptLetters_Open` ✚ · `Story.Flag.IntakeUnvetted` ✚ |

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T3** de mol | **geplant, dragend** (S06, P3-c). De ongescreende instroom. Onzichtbaar. |
| **T5** Kaine | **geplant, dragend** (S04 burgerrisico P5-d; S06 de brieven P5-c). De brievendraad loopt rechtstreeks naar het muiterijpad in act 4. |
| **T1** Blight | **geplant** (S03, P1-b): Sela's retoriek noemt de Blight als de wond waaruit dit voortkomt. Eén verwijzing, geen uitleg (§18.8). |
| T2, T4 | niet aangeraakt. |

---

## 7. Draden

- **The Conscript Letters** — **GEOPEND** (S06). Betaald in act 4, muiterijpad. §2.11.
- **De shift-baas** — **GESLOTEN** (S06, Sela's oordeel).
- **Iron Chorus** — doorlopend; eerste gezamenlijke operatie.

---

## 8. Groei

- **Sela** — komt binnen op het hoogtepunt van haar kracht en op het dieptepunt van haar macht: ze kan een wagon overtuigen en heeft geen bed. Haar arc (§2.5: *"learns power's compromises"*) begint bij iemand die nog nooit heeft moeten kiezen tussen twee goede dingen. In S06 wint ze; in act 2 zal ze verliezen.
- **Brick** — spreekt als eerste (S01), zegt vier woorden over Krad-9 (S03), en zwijgt de rest van de missie. Zijn hele arc in dertig woorden.
- **Mara** — kiest mensen boven veiligheid, hardop, met redenen. **De schrijver moet haar gelijk geven.** Als de speler in act 3 denkt "dat was dom", is de scène mislukt; hij moet denken "ik had hetzelfde gedaan".
- **Voss** — leert dat er problemen zijn die hij niet kan oplossen (S03) en dat leiderschap deels bestaat uit het herkennen van de persoon die het wél kan.
- **Dex** — van ingenieur naar kwartiermeester-tegen-wil-en-dank. Zijn paniek is komisch en zijn oplossing is de opening van twist 3.

---

## 9. Instructies voor de dialogue-writer

1. **S06 is de gevaarlijkste scène van de act.** Lees de valkuil twee keer. Geen enkele nieuwkomer krijgt aandacht.
2. **Sela houdt geen toespraak.** AR-7 verbiedt een oration in act 1. Ze overtuigt met korte beurten en vragen.
3. **Haar signature-regel valt in S03 en nergens anders.**
4. **Brick spreekt als eerste in S01 en dat is de enige keer.** Er komt geen uitleg bij.
5. **Mara heeft gelijk in S06.** Schrijf haar besluit als het beste beschikbare besluit, niet als een fout met vooruitwijzing.
6. **Beide takken van S04/S05 volledig.** Beide moeten pijn doen.
7. **De conscripten die willen blijven, hebben goede redenen.** Wie ze bang of dom schrijft, maakt Sela's overwinning waardeloos.
8. **Voss-varianten:** S03 (zijn mislukte poging), S04 (de keuze), S06 (de brieven), S99 (Sela vragen te blijven). Vier scènes — dit is samen met M1.5 en M1.8 de duurste missie van de act. Zie C-6/Q-7.

## 10. Barks

Nieuw: **burgers onder vuur** (paniek, geen commando's), **conscriptbewaking die weet dat er mensen achter haar zitten**, en de eerste **naam-barks van nieuwe rekruten** — waarmee §18.5 regel 6 (*"Reyes is down" beats "Man down"*) voor het eerst echt gaat werken, want de speler kent nu meer namen dan hij kan onthouden. Dat is precies de bedoeling.
</content>
