# BEAT-SHEET — M1.5 *Cells*
*L1 | story-architect | 2026-07-31 | act 1, beweging III*
*Canon: `02_story_bible.md` §2.9 (M1.5), §2.11 (Iron Chorus), §2.3 (gefragmenteerde cellen), §2.7 (morele archetypen) · `11_missions.md` §11.2 (Negotiation summits — "armed-social spaces")*

---

## 1. Dramatische functie

Bijbel: *"Contact rival cell 'Iron Chorus'; first alliance mechanic."* Verhaalfunctie:

> **De missie waarin de speler ontdekt dat "de andere kant" niet één ding is — en dat de moeilijkste gesprekken van deze oorlog met bondgenoten worden gevoerd.**

`02_story_bible.md` §2.3 is hier de sleutel: *"Dozens of resistance cells exist at game start. They are fragmented, mutually distrustful, infiltrated, and dying."* En: *"The player's story is not the start of resistance — it is the start of resistance working."* M1.5 is de eerste keer dat het spel die zin waarmaakt. Niet door een gevecht te winnen, maar door een ruzie te overleven.

De Iron Chorus is geen slechte versie van Ember. Ze zijn de **oudere** versie: harder, voorzichtiger, met betere veiligheid en minder illusies. Ze hebben overleefd waar Ember bijna doodging, en ze hebben daar een prijs voor betaald die de speler pas in act 3 helemaal kan lezen.

Dat is meteen de belangrijkste schrijfopdracht van deze missie: **de Iron Chorus moet in minstens één opzicht gelijk hebben, en de speler moet dat weten.**

**Zichtbaarheid na M1.5: 5/8.** Twee cellen die met elkaar praten, is precies waar de Veil op wacht.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS**, **MARA** | veld |
| **BRICK** | veld — zijn eerste missie als lid. Hij zegt in de hele missie misschien acht woorden en één daarvan telt |
| **Iron Chorus-emissaris** | de ontmoeting — **naam en casting ontbreken, Q-4** |
| twee Iron Chorus-vechters | de ontmoeting, de sweep |
| een Kessaraanse shift-baas | S04 — het onderwerp, spreekt hooguit twee regels |
| **DEX** | Hollow Point, radio |
| Veil-sweepploeg | S05 — **de eerste keer dat de Veil fysiek in beeld is** |

---

## 3. Site & runtime-haak

- **Fictie:** een pompgalerij onder de arbeidersblokken — een oude waterkelder van de Mid-Works met drie uitgangen, waar twee groepen elkaar kunnen ontmoeten zonder elkaars route te leren. Het soort ruimte dat `11_missions.md` §11.2 een *armed-social space* noemt: iedereen praat, iedereen staat verkeerd.
- **`location`-strings:** `Kessara / Mid-Works / Worker Housing — Pump Gallery` en `Kessara / Mid-Works / Worker Housing`
- **Regio-pin:** `WorkerHousing` (na M1.3's Foothold in spelershanden — dat maakt de ontmoeting fictioneel mogelijk: Ember biedt de veilige grond aan, en dat is precies waarom de Iron Chorus komt)
- **Runtime:** diplomatie + escorte. Objectives: `ReachLocation` (de galerij) → gespreksfase → escorte-`ExtractSquad` met beschermde eenheid. Optional: geen enkele Iron Chorus-vechter verloren.
- **Nieuw ten opzichte van M1.1–M1.4:** dit is de eerste missie van act 1 die **niet in SPEC-P2-04 staat**. Zij vraagt geen nieuwe objective-primitieven (11.4-conform), maar wel een escorte-doel. Als dat er niet is, is dat een systeemtaak — **niet** iets wat een schrijver omzeilt.

---

## 4. Scènelijst

### S01 — *Three Cells, One District* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Brick, Reyes

- **want** — Mara wants a meeting with the only other cell on Kessara that has lasted longer than Ember.
- **obstacle** — She has asked them twice before and been refused twice, and everyone in the room knows it.
- **turn** — She admits the third invitation was not hers: it was theirs, and that is what frightens her.

**Beats:** (1) Wie de Iron Chorus zijn, verteld als reputatie en niet als dossier. (2) Dex' opmerking dat hun radiodiscipline beter is dan die van Ember. Zonder afgunst — hij bewondert het. (3) Mara's twee eerdere weigeringen. **Ze noemt ze zelf**, want Mara liegt niet en zeker niet naar boven. (4) De omkering: nu vragen zíj. (5) Brick vraagt één ding, en het is praktisch, en het is de goede vraag.
**Verband met M1.3:** de naam die Threx op de radio noemde (S06 van M1.3) was iemand van de Iron Chorus. Dát is waarom ze nu wel praten. **Eén regel — nooit twee.** Als de speler het verband zelf legt, is het een van de mooiste momenten van de act; als een personage het uitlegt, is het een samenvatting.

### S02 — *The Pump Gallery* · de ontmoeting
`Kessara / Mid-Works / Worker Housing — Pump Gallery` · hub · tier 2
**Aanwezig:** Voss, Mara, Brick · emissaris + twee vechters

- **want** — The emissary wants to know whether Ember is a cell or a crowd.
- **obstacle** — Ember has just taken forty new rifles and has no idea what that looks like from outside.
- **turn** — He offers an alliance on one condition, and the condition is a name on a list.

**Beats:** (1) Aankomst; drie uitgangen; niemand gaat zitten. (2) De emissaris toetst — geen dreiging, alleen vragen die niemand goed kan beantwoorden. (3) **P3-b:** hij vraagt hoe Ember zijn mensen natrekt. Ember trekt niemand na. Hij lacht, en het is geen vriendelijke lach. **Dit is de plant voor twist 3 en hij moet als arrogantie klinken.** (4) Hun leer: de Dominion valt niet zonder dat de mensen die hem draaiende houden een prijs betalen. Verdedigbaar in één zin (§2.7 regel 2) — de shift-bazen leveren de Tithe-lijsten, en zonder die lijsten stopt de Tithe. (5) **Het aanbod: gezamenlijke operatie, hun voorwaarden, één naam vooraf.** (6) De speler krijgt het woord.
**Dit is de langste dialoogscène van act 1.** Het is ook de enige plek waar de speler écht kan onderhandelen. Geef hem drie tot vier gespreksopties met werkelijk verschillende uitkomsten, niet drie smaken van ja.

### S03 — *Walking Them Out* · de escorte
`Kessara / Mid-Works / Worker Housing` · walk-and-talk · tier 2
**Aanwezig:** Voss, emissaris, Brick, Mara achteraan

- **want** — Voss wants the emissary out of Ember's ground alive, whatever was agreed downstairs.
- **obstacle** — The emissary keeps testing him, and the tests are about Mara.
- **turn** — He tells Voss something about Mara's past refusals that Mara did not tell the room.

**Beats:** (1) Lopen door de blokken; curfew; niemand buiten. (2) De emissaris' vragen — waarom Ember Voss vertrouwt, wat er gebeurt als Mara er niet meer is. **Dat is een vraag die niemand nog gesteld heeft en hij zaait M1.8.** (3) **Wat hij over Mara vertelt.** Iets waars, iets kleins, iets dat haar niet slechter maakt maar wel menselijker: bijvoorbeeld dat zij drie jaar geleden zelf om hulp vroeg en dat ze nee kregen omdat Ember toen te klein was om waard te zijn. (4) Bricks acht woorden. Hij noemt een naam, en de emissaris herkent hem. **Dat is het moment waarop de twee cellen elkaar voor het eerst iets echt geven.**
**Waarom Brick hier zit:** hij is de enige die niet onderhandelt. §18.4 — hij liegt nooit. Voor een cel die van wantrouwen leeft, is een man die niet kan liegen de beste diplomaat aan tafel.

### S04 — *The Shift Boss* · de keuze
`Kessara / Mid-Works / Worker Housing` · cutscene · tier 2
**Aanwezig:** Voss, emissaris, Brick, Mara; de shift-baas

- **want** — The Iron Chorus wants the name on their list closed tonight, with Ember watching.
- **obstacle** — The name is a foundry shift boss who files the absentee lists the Tithe draws from — and who also fed two galleries through last winter.
- **turn** — Voss decides, and whichever way he decides, one of the two cells learns exactly what Ember is.

**Beats:** (1) Het huis; de man; zijn keuken. Hij is niet dapper en niet monsterlijk. (2) De emissaris legt uit wat hij aanlevert. Feitelijk, verifieerbaar. (3) De shift-baas krijgt hoogstens twee regels. Hij ontkent niets. (4) **De keuze: laten gebeuren / voorkomen / uitstellen (hem waarschuwen en laten verdwijnen).** (5) De reactie van de emissaris. (6) Mara zegt niets — opnieuw. Dit is de tweede keer in de act dat ze een beslissing bij Voss laat (na M1.1.S05), en de derde komt in M1.7.
**§2.7 regel 2 — beide kanten in één zin:**
- *Doden:* zonder de lijsten stopt de Tithe, en de Tithe kost elke maand mensen.
- *Sparen:* wie beslist wie er sterft omdat hij een formulier invult, is de administratie geworden waar hij tegen vecht.
**Systeem:** `Story.Choice.M15_ShiftBossSpared`. **Betaald in M1.6.S06** door Sela — binnen dezelfde act. Geen wees.

### S05 — *The Late Sweep* · het gevecht
`Kessara / Mid-Works / Worker Housing` · in-mission-radio + callout · tier 2
**Aanwezig:** allen; **Veil-sweepploeg**

- **want** — Both cells want to be somewhere else before the sweep closes the block.
- **obstacle** — They are two groups who have never fought in the same street and do not use the same words.
- **turn** — The Iron Chorus fights the way they have survived — and Ember sees the price of surviving that long.

**Beats:** (1) De sweep sluit. (2) **De Veil, fysiek, voor het eerst.** Kalm, klinisch vocabulaire (§18.5 regel 5) — het contrast met de conscripten van M1.4 moet hoorbaar zijn binnen tien seconden. (3) De twee groepen vechten langs elkaar heen; verschillende commando's, verschillende reflexen. (4) **De Iron Chorus-methode:** ze breken contact door een derde partij als scherm te gebruiken — een portiek vol burgers, een gesloten deur die ze niet openen. Eén beeld, geen betoog. (5) **P4-b:** de sweep is laat en te klein, en het is de **emissaris** die dat spottend opmerkt — *"jullie hebben geluk of jullie hebben iets dat wij niet hebben"*. Ember lacht het weg. (6) Eruit.
**Waarom P4-b uit de mond van de rivaal komt:** een buitenstaander die het opmerkt, maakt het feit hard zonder het tot mysterie te maken. Ember kan het wegwuiven; de speler onthoudt het.

### S99 — *Terms* · debrief
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Brick, Reyes

- **want** — Mara wants to know what Ember agreed to, in words, out loud, in front of everyone.
- **obstacle** — What was agreed depends on what Voss did at a kitchen table, and half the room disagrees with it.
- **turn** — She ratifies his decision without endorsing it, and tells the room it was his to make — which is the first public transfer of authority in the game.

**Beats:** (1) De uitkomst: pact of weigering. (2) Dex' bezwaar of instemming — hij denkt in systemen en ziet meteen wat een alliantie betekent voor radiodiscipline en veiligheid. (3) Reyes' vraag over de burgers in het portiek. (4) Brick zegt één ding over de emissaris. (5) **Mara bekrachtigt Voss' besluit en zegt hardop dat het zijn besluit was.** Dit is beweging III's structurele beat: gezag verschuift in het openbaar. (6) Als er een pact is: de eerste gezamenlijke operatie staat op de kaart — en die operatie is M1.6.
**Systeem:** `Story.Beat.M15_Cells`, `Story.Choice.M15_IronChorusPact`, `Story.Choice.M15_ShiftBossSpared`. Rewards: bescheiden materiaal + intel; de echte beloning is de alliantie.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.M14_Quartermaster` · `Story.Clue.ThrexVoice_1` (S01, één regel) | `Story.Beat.M15_Cells` ✚ · `Story.Choice.M15_IronChorusPact` ✚ · `Story.Choice.M15_ShiftBossSpared` ✚ |

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T3** de mol | **geplant, dragend** (S02, P3-b): de Iron Chorus lacht Ember uit omdat het niemand natrekt. Klinkt als arrogantie. Is een waarschuwing. |
| **T4** | **geplant, dragend** (S05, P4-b): de late, te kleine sweep, opgemerkt door een buitenstaander. |
| Threx-ladder AR-6 | **geen trede.** Threx komt niet voor in M1.5 — de naam uit M1.3 wordt alleen genoemd. Een trede overslaan is verboden; een trede *herhalen* ook. |
| T1, T2, T5 | niet aangeraakt (behalve de burgers in het portiek, die stil in de burgertelling landen). |

---

## 7. Draden

- **Iron Chorus** — **GEOPEND**. Loopt door tot act 2–4 (fusie, schisma of tragedie, §2.11).
- **De shift-baas** — geopend in S04, **gesloten in M1.6.S06** door Sela's oordeel. Optionele echo in act 2.

---

## 8. Groei

- **Voss** — van planner naar onderhandelaar. Belangrijker: dit is de missie waarin hij voor het eerst een besluit neemt dat níét over tactiek gaat, en waarin het publiek daarvan een andere cel is. §2.4 traits 2 en 3 (*"people follow them"*, *"refuses to waste lives"*) staan hier tegenover elkaar en de speler kiest welke wint.
- **Mara** — geeft gezag weg in het openbaar. Ze doet dat kalm, en dat maakt het erger: ze is iets aan het regelen. De speler weet nog niet wat.
- **Brick** — acht woorden. Eén ervan opent een deur die drie scènes met praten niet openkreeg.
- **Dex** — ziet de alliantie als een systeem met een lek, en heeft gelijk. Zijn grap in S99 is de bitterste van de act.

---

## 9. Instructies voor de dialogue-writer

1. **De Iron Chorus heeft in minstens één opzicht gelijk, en de speler moet dat merken.** Als ze alleen maar hard zijn, is de missie een strohalm en de act-3-betaling waardeloos.
2. **De emissaris heeft geen naam tot Q-4 beantwoord is.** Schrijf hem als "de emissaris"; het past bij een cel die zijn namen niet weggeeft, en het is omkeerbaar zodra er een naam is. **Verzin er geen.**
3. **De shift-baas krijgt twee regels en geen verdediging.** Wie hem laat pleiten, maakt de keuze makkelijk. Hij moet gewoon een man in zijn keuken zijn.
4. **Mara zegt niets in S04.** Haar zwijgen is de scène. §18.7: stilte is inhoud.
5. **Veil-vocabulaire debuteert in S05** en moet binnen tien seconden hoorbaar anders zijn dan Dominion-conscripten. Kalm, klinisch, weinig woorden, geen paniek (§18.5 regel 5).
6. **P4-b komt uit de mond van de rivaal, niet uit die van Ember.** Ember wuift het weg.
7. **S02 is de langste dialoogscène van act 1** en dus de scène waar de variantieregel (§18.3, factor 3) en de symmetriecheck (§18.9 B) het hardst bijten. Zet er interrupties in. Laat iemand een vraag ontwijken en er nooit op terugkomen.
8. **Voss-varianten:** S02 (de onderhandeling) en S04 (de keuze) krijgen volledige dekking op beide assen. Dit is samen met M1.8 de plek waar het budget voor varianten heen moet.

## 10. Barks

**Nieuw: Veil-vocabulaire** (kalm, klinisch, doelgericht) en **Iron Chorus-vocabulaire.** Voor de Iron Chorus is de goedkope oplossing hergebruik van de Eclipse-fighterstem met andere regels; de betere oplossing is een eigen registerkeuze. Dat is een `voice-director`-afweging binnen tier 1 en hij hoort hier gemeld te worden, niet stil opgelost.
</content>
