# BEAT-SHEET — M2.5 *The Relay*
*L1 | story-architect | 2026-08-02 | act 2, beweging II*
*Canon: `02_story_bible.md` §2.9 (*"the Meridia relay heist that first contacts Whisper"*), §2.5 (Whisper — *"voice only until Act 3"*), §2.8 twist 1 en twist 2, §2.1 (de Meridia Blight) · `03_world_design.md` §3.2, §3.3 (MERIDIA) · `11_missions.md` §11.2 (Wire-tap, Infiltration, social infiltration)*
*Erft: `ACT2_OVERVIEW.md` §4 (T2 volledig, P1-A2), §7 (Meridia-register), §8 (AR-A2-4, AR-A2-5) · `ACT1_OVERVIEW.md` §4 (P2-a t/m P2-d) · `RULINGS_L1.md` L1-R17, L1-R19, L1-R37, L1-R38*

---

## 1. Dramatische functie

Bijbel: *"the Meridia relay heist that first contacts Whisper."*

> **De missie waarin iemand terugpraat die er niet hoort te zijn — en waarin de speler ontdekt dat het angstaanjagendste feit uit act 1 zijn beste bondgenoot is geworden.**

Act 1 heeft in `M1.7.S03` één regel achtergelaten die de speler koud maakte: er liep al eerder een query op Embers sector, en **die was niet van de Veil** (`Story.Clue.OutsideQuery`, plant P2-b). Er was dus een tweede jager. In deze missie blijkt die tweede jager een makelaar te zijn die Eclipse al twee akten in de gaten houdt en die nu wil praten.

**Dat is de goede vorm van een eerste contact:** de speler heeft geen enkele reden om hem te vertrouwen, en het spel geeft er geen. Whisper wordt niet gepresenteerd als een geschenk. Hij wordt gepresenteerd als een lek dat vriendelijk doet.

Daarnaast doet deze missie het werk dat act 3 nodig heeft en dat geen andere act-2-missie kan doen: **hij geeft de Blight een plaats.** `03_world_design.md` §3.3: *"The Blight began here; memorial politics run deep."* De act-3-onthulling (twist 1) is de beste wending van het spel en hij landt op een document. Deze missie zorgt dat hij ook op mensen landt.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS** | overal |
| **KAYA** | overal — sociale stealth is haar terrein en **zij draagt de enige gepolijste aanraking van de akte** |
| **TORREN** Vale | overal. **Zijn ingesproken nullijn ligt in S02 (AR-A2-4)** |
| **DEX** | The Carcass, radio. Hij bouwt de tap en hij is er niet bij |
| **WHISPER** | **S05 — zijn eerste regels van het spel, en uitsluitend via de relay** |
| **SELA** | S01, radio. Eén bezwaar |
| Meridiaanse handelshuis-figuren | S03 — **fingerprint en casting ontbreken (C-A2-2 / Q-A2-3)** |
| Bursary-inkomstenpersoneel, huismercenairs | S04, S06, S07 — barks + één spreekregel |
| Kustbewoners bij de gedenkplaats | S02 — `CIVILIAN_A`/`_B` |

**Spreekt niet:** Threx, Kaine, Vex, AEGIS (behalve één omroep in S03 — zie daar), Brick, Reyes, Petra, Mara.

> **Whisper is een STEM en niets anders** (§2.5, woordelijk: *"voice only until Act 3"*). Geen beeld, geen `shot:` die hem toont, geen beschrijving van waar hij zit. Wie hem lichamelijk maakt, verbrandt de act-3-onthulling.

---

## 3. Site & runtime-haak

- **Fictie:** een oceaanwereld waarvan de beschaving op geschakelde drijfsteden en getijdenplatforms leeft. De relay staat op een getijdenplatform en is bereikbaar via de onderdeklabyrinten. Boven water: handelskwartieren waar wapens onmogelijk zijn en woorden munitie.
- **`location`-strings** (`ACT2_OVERVIEW` §7): `Tarsis / Dune Sea / The Carcass — Map Table` · `Meridia / Memorial Coast` · `Meridia / Float City / Trade Quarter` · `Meridia / Float City / Under-Deck` · `Meridia / Tidal Platform`
- **Regio-pin:** **bestaat niet.** En Meridia staat volgens §2.9 zelfs niet op de act-2-strategiekaart — zie **C-A2-1**: mijn lezing is dat dit een authored *"separate smaller level"* is (§3.2) en geen open zone. **Dat is een gemelde lezing, geen stille beslissing.**
- **Runtime:** Wire-tap met een sociale-stealthaanloop. Objectives: sociale fase (toegang kopen zonder wapen) → `Infiltrate` (onderdek) → `HackTerminal`/tap → keuzefase → `ExtractSquad`. Optional: het grootboek intact meenemen; nul burgerdoden in het handelskwartier.
- **Nieuw:** sociale stealth (wapens onmogelijk, reputatie als resource). **C-A2-3.**

---

## 4. Scènelijst

### S01 — *Everything Goes Through Here* · briefing
`Tarsis / Dune Sea / The Carcass — Map Table` · cutscene · tier 5
**Aanwezig:** Voss, Dex, Kaya, Torren + Sela op de radio

- **want** — Voss wants to hear what the Dominion says to itself, and one relay carries most of it.
- **obstacle** — The relay sits on a world Eclipse has no business on, in a society where a rifle is worse than useless.
- **turn** — Torren plans it, competently, in front of everyone — and Sela says out loud what that means.

**Beats:** (1) De relay uitgelegd als infrastructuur en niet als doelwit: geld, handel en bevelen lopen door dezelfde draad. (2) **Torrens eerste bijdrage als lid.** Hij plant het, en het is beter dan wat Eclipse zelf had bedacht. Bevelen als suggesties (§18.4). (3) **P3-A2-a, tweede beat:** Sela merkt op dat de man die net binnen is nu de operaties plant. **Ze beschuldigt hem nergens van.** Ze noemt een feit en laat het staan. Torren spreekt haar niet tegen. **Twee regels, geen scène.** (4) Kaya's inbreng: op Meridia koop je toegang, je breekt hem niet — en dat kost geld dat Eclipse na M2.4 misschien niet heeft.
**Vlaggen:** **leest `Story.Char.TorrenJoined`** en `Story.Beat.M24_LoyalGhost`. **Zet niets.**

### S02 — *The Memorial Coast* · aankomst, en Torrens nullijn
`Meridia / Memorial Coast` · walk-and-talk · tier 5
**Aanwezig:** Voss, Torren, Kaya · kustbewoners

- **want** — Voss wants to be inside the trade quarter before the tide turns.
- **obstacle** — The route runs along a coast full of people who are not going anywhere, and one of them talks to him.
- **turn** — A local explains what the memorial is *for* these days, and it is not the dead.

> **DIT IS TORRENS INGESPROKEN NULLIJN (AR-A2-4 / L1-R19 / L1-R37) EN DAT IS EEN HARDE OPDRACHT.**
> §18.4 geeft hem *"longest silences in the game — his beats are marked `[pause]`"*. Dat is een **differentiële** tell: hij werkt alleen als de speler weet hoe hij normaal getimed praat. Zonder nullijn wordt "zwijgzaam" gewoon zijn stem, en dan landt zijn zwaarste beat in act 3 op niets.
> **In deze scène praat Torren normaal.** Volledige zinnen, gewoon tempo, geen `[pause]`, geen gewicht. Hij is een man die naast iemand loopt.
> **En de naald slaat precies één keer uit** (L1-R37: het beste ingesproken niet-X bevat één X): **één** `[pause]`, en die valt op de zin over Sylvaris die niemand gesteld heeft. Eén. Wie er twee schrijft, heeft geen nullijn geschreven.

**Beats:** (1) De kust: gedenkstenen, water, mensen die er wonen. **Het register van deze planeet is BEZITTEN** — een bewoner praat over de gedenkplaats in termen van aandeel en aanspraak. (2) **P1-A2-a, en het is de plant die act 3 nodig heeft:** de Blight-gedenkkust is een **politiek instrument**. De handelshuizen gebruiken hun doden als hefboom tegen de Bursary-rantsoenering. Dat is cynisch, het is waar, en het is wat een bewoner erover zegt zonder er cynisch bij te klinken. **Verplicht, gesproken, en de speler hoort het als lokale politiek.** (3) **Torren, normaal getimed.** Hij praat over de plek als over terrein, en het is precies zo saai als het moet zijn. (4) Kaya vindt de gedenkplaats ongemakkelijk en zegt dat op haar manier. (5) De ene `[pause]`.
**Vlaggen:** **zet niets, leest niets.** Deze scène draagt de nullijn en de T1-plant en verder niets — dat is de reden dat er niets op het spel staat.

### S03 — *Words Are Ammunition* · het handelskwartier
`Meridia / Float City / Trade Quarter` · hub · tier 5
**Aanwezig:** Voss, Kaya, Torren · handelshuis-figuren

- **want** — Kaya wants an access token from a house that has no reason to give one.
- **obstacle** — Nobody here can be threatened, bribed in the open, or rushed, and Voss's entire skill set is threat, trade and speed.
- **turn** — She gets it by selling something Eclipse has and did not know was worth anything.

**Beats:** (1) Sociale stealth: wapens zijn onmogelijk, en het is voelbaar hoe weerloos dat de ploeg maakt. (2) **Het handelshuisregister: bezit.** Mensen praten in aandeel, aanspraak en wie wat te vorderen heeft. Ze zijn niet vijandig; ze zijn **duur**. (3) Een **AEGIS-omroep** loopt door de ruimte — een rantsoenmededeling in de tegenwoordige tijd, met een kans in plaats van een mening (§18.4). Niemand luistert. **Dat is de enige AEGIS-regel van deze missie en hij is behang** (AR-8's vorm: AEGIS spreekt nooit tegen de speler). (4) **AANRAKING A2-3 — P4-A2-c, EN DIT IS DE ENIGE GEPOLIJSTE AANRAKING VAN DE HELE AKTE.** Kaya leest de bewakingsroosters en merkt op dat de relay onderbewaakt is voor wat er doorheen gaat. Ze verklaart het weg met een gevatte zin over dat geld geld bewaakt en geen draden. **Niemand spreekt haar tegen.** Het is één gepolijste zin, uit de mond van iemand die van gevatheid haar beroep heeft gemaakt — **dat is karakter, geen auteurschap**, en dat is precies waarom er in act 2 maar één van is (L1-R17). (5) Kaya's verkoop: ze biedt iets aan dat Eclipse heeft en niet als waarde herkende — **Tarsische bergingsdata of Krad-9-doorvoercijfers**, en dat maakt de vorige drie missies opeens geld waard. (6) Torren zegt in deze hele scène bijna niets en het is hier **niet** zijn tell: hij is buiten zijn vak en hij weet het.
**Vlaggen:** **leest `Story.Char.KayaJoined`**. **Zet niets.**

### S04 — *The Under-Deck* · de tap
`Meridia / Float City / Under-Deck` → `Meridia / Tidal Platform` · in-mission-radio · tier 5
**Aanwezig:** Voss, Kaya, Torren · Dex op de radio

- **want** — Dex wants his tap on the line before the tide window closes.
- **obstacle** — The relay's traffic is enormous and Eclipse has minutes, so somebody has to decide what to steal.
- **turn** — The trace on the line is one Voss has seen before, and it is eight months old.

**Beats:** (1) Het onderdek: onderhoudslabyrint, water aan drie kanten, Dominion-infrastructuur die niemand onderhoudt. (2) Dex over de radio, in fragmenten, tevreden. (3) **P1-A2-b:** in de doorvoer zit een **Bursary-grootboek** waarin de Blight-noodhulp een **toewijzingskolom** is en geen ramp. **Kaya zegt het hardop, één regel, als boekhouding.** Dat is dezelfde herkenningsvorm als `M1.4.S04.220` in act 1: geef de speler het woord voordat het iets betekent. **Het is geen aanraking** — de Bursary beschrijft zichzelf, en dat is per definitie plantmateriaal en geen wegverklaring (`ACT1_OVERVIEW` §4, L1-R44). → zet `Story.Clue.BlightLedger`. (4) **P2-b BETAALT.** Op de lijn staat een oud querysignatuur, en het is het signatuur uit `M1.7.S03` — de query die **niet van de Veil** was, **acht maanden** oud (het beschermde getal-met-eenheid uit act 1, L1-R55; act 2 verandert er niets aan). **Voss herkent het.** Eén regel. Er wordt niet uitgelegd wat het betekent. (5) De keuze wát je steelt is een gameplay-keuze en geen dialoogkeuze — noem hem niet in een `choice:`-blok.
**Systeem:** zet **`Story.Clue.BlightLedger`** ✚ — **hier, waar de regel klinkt** (L1-R38). **Leest `Story.Clue.OutsideQuery`** (act 1, `M1.7.S03`) — **verplicht, gesproken.**

### S05 — *It Is Known* · Whisper
`Meridia / Tidal Platform` · cutscene · tier 5
**Aanwezig:** Voss, Kaya, Torren · **WHISPER (stem, via de relay)**

- **want** — Voss wants to know who has been reading his sector for eight months.
- **obstacle** — The voice will answer any question except that one, and it answers the others too well.
- **turn** — It offers help, unasked, for nothing — which is the most expensive thing anyone has offered him.

**Beats:** (1) De lijn draagt iets terug. **Whisper spreekt.** §18.4: geen samentrekkingen, conditionalis, passieve constructies; *"it is known"* in plaats van *"I know"*; **hij gebruikt geen namen en hij zegt nooit "ik".** Dat maakt zijn eerste regels mechanisch onmiskenbaar en het is meteen de reden dat de speler hem niet kan lezen. (2) **Hij bevestigt de query niet en ontkent hem niet.** Hij impliceert. (3) **P1-A2-c:** hij weet te veel van Meridia. Niet dreigend veel — **precies** veel: welk huis welk aandeel houdt, welke gedenkplaats van wie is. Voor een makelaar is dat het aanprijzen van zijn waar; voor act 3 is het het bewijs dat hij ergens vandaan komt. (4) **Torrens reactie is de interessantste in de kamer.** Een inlichtingenofficier herkent een inlichtingenofficier, en hij zegt er precies één ding over. (5) Kaya vertrouwt hem meteen niet en zegt waarom, en haar reden is commercieel: **niemand geeft iets voor niets, en wie dat wel doet, is jou aan het kopen.** (6) Whisper biedt iets aan dat Eclipse over drie missies nodig heeft, en hij vraagt er niets voor. **De scène eindigt zonder dat er iets is afgesproken.**
**Systeem:** zet **`Story.Char.WhisperContact`** ✚ — **hier, waar hij spreekt.**
**Gelezen door:** `M2.6.S01` · **M2.7 (alle scènes)** · act 3 espionage-arc.
**Harde eis:** **geen `shot:` toont hem, geen regel beschrijft waar hij is.** §2.5.

### S06 — *Keep It or Kill It* · de keuze
`Meridia / Tidal Platform` · cutscene · tier 5
**Aanwezig:** Voss, Kaya, Torren · Dex op de radio

- **want** — Voss wants the relay to keep being useful to Eclipse.
- **obstacle** — A relay that is useful to Eclipse is a relay that is still useful to the Dominion, and to the voice that just called.
- **turn** — He decides what kind of intelligence service Eclipse is going to be, three missions before he knows it mattered.

**Beats:** (1) De vraag ligt op tafel en er is geen tijd. (2) **De twee opties, allebei in één zin verdedigbaar (§2.7 regel 2):**
- *Houden* — de tap blijft staan, Eclipse blijft meelezen, en de Dominion blijft óók meelezen. **En de vreemde stem houdt een kanaal.**
- *Branden* — de relay gaat kapot, de Dominion is een oog kwijt, Meridia's handel is een week lam, en Eclipse hoort niets meer. **En de vreemde stem is weg tot hij zelf terugkomt.**
(3) Torren adviseert **houden**, koel en overtuigend, en hij heeft professioneel gelijk. (4) Kaya adviseert **branden** en haar reden is dezelfde als in S05: een gift is een aankoop. (5) Dex over de radio: het derde argument, technisch, en het maakt beide opties zwaarder in plaats van één ervan makkelijker.
**Systeem:** `Story.Choice.M25_Relay.{Kept,Burned}` ✚ — script-zijde `story.m25_relay` = `kept` | `burned`.
**Gelezen door:** `M2.5.S99` (2) · **`M2.7.S02`** (heeft Whisper nog een kanaal, of moest hij een omweg vinden) · act 3 intel. Volledig register: `ACT2_OVERVIEW` §6.
**Voss-varianten:** volledige dekking op beide assen. **De idealist/pragmatist-as draagt hier het meeste.**

### S07 — *The Tide Window* · eruit
`Meridia / Tidal Platform` → `Meridia / Float City / Under-Deck` · in-mission-radio + callout · tier 5
**Aanwezig:** Voss, Kaya, Torren + vechters · Bursary-inkomstenpersoneel · huismercenairs

- **want** — Eclipse wants to be off the platform before the houses decide whose problem this is.
- **obstacle** — The mercenaries here are paid by people who are still negotiating, so the fight keeps pausing.
- **turn** — They get out clean, and the reason they get out clean is that somebody on the other side was told to look elsewhere.

**Beats:** (1) De achtervolging over platforms en onderdek; water als gevaar en als dekking. (2) **De huismercenairs vechten met tussenpozen** — hun opdrachtgevers onderhandelen nog. Dat is Meridia: ook een vuurgevecht is een transactie. (3) Torren vecht correct en zonder enige aarzeling en het is de tweede keer dat Voss dat ziet. (4) **De uitweg gaat te makkelijk**, en er is precies één regel over. **Dit is GEEN aanraking** — niemand verklaart het weg, het wordt alleen benoemd, en een regel die de anomalie benoemt zonder hem te verklaren telt per definitie niet mee (`ACT1_OVERVIEW` §4). Het is de zaadje-vorm van wat in M2.7 een vraag wordt: heeft de stem dat gedaan? (5) Weg.
**Band:** `in-mission-radio` met `band: callout`.
**Vlaggen:** zet niets.

### S99 — *What It Cost* · debrief
`Loyal Ghost / Bridge` · cutscene · tier 5
**Aanwezig:** Voss, Kaya, Torren, Dex + Sela en Brick op de radio

- **want** — Voss wants the room to tell him whether he just made an ally or opened a door.
- **obstacle** — The room does not agree, and for once the disagreement does not split the way it usually does.
- **turn** — Nobody decides anything, and that unresolved question is what the next two missions are actually about.

**Beats:** (1) De buit: het grootboek, de doorvoer, en per tak de tap of de puinhoop (`story.m25_relay`, 2 takken). (2) **De vreemde stem.** Vier mensen, vier posities, en ze lopen niet langs de vertrouwde breuklijnen: Torren is voorzichtig positief, Kaya is negatief, Dex is nieuwsgierig op een manier die hem bang maakt, Sela wil weten wie er verantwoording aflegt. **Er wordt niets besloten en dat is de scène.** (3) Brick, één regel, en hij gaat niet over de stem. (4) **Geen aanraking, geen clue, geen Mara-opname.** Deze debrief zet één beat-vlag en laat een vraag open, en dat is alles wat hij moet doen.
**Systeem:** zet **`Story.Beat.M25_Relay`** ✚. Leest `story.m25_relay` (2 takken).

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Char.TorrenJoined` (S01) · `Story.Beat.M24_LoyalGhost` (S01) · `Story.Char.KayaJoined` (S03) · **`Story.Clue.OutsideQuery`** (S04 — act-1-plant P2-b, verplicht en gesproken) | **`Story.Clue.BlightLedger`** ✚ (S04) · **`Story.Char.WhisperContact`** ✚ (S05) · **`Story.Choice.M25_Relay.{Kept,Burned}`** ✚ (S06) · **`Story.Beat.M25_Relay`** ✚ (S99) |
| `story.m25_relay` — gezet S06, gelezen S99 (2) | — |

**De twee clues worden gezet in de scène waarin ze klinken, nooit in de debrief** (L1-R38). Volledig register: `ACT2_OVERVIEW` §6.

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T2** Whisper is Ilan Vex | **eerste contact, en één act-1-plant BETAALT.** S04 lost P2-b in (`Story.Clue.OutsideQuery` — de query die niet van de Veil was). S05 opent de relatie. **De identiteit wordt hier niet aangeraakt** en `Story.Clue.CustodianKey` blijft ongelezen tot `M2.7.S05` |
| **T1** de Blight was gemaakt | **geplant, dragend ×2** (S02 P1-A2-a: de gedenkkust als politiek instrument; S04 P1-A2-b: de Blight als toewijzingskolom in een Bursary-grootboek) + **niet-dragend** (S05 P1-A2-c: Whisper weet te veel van Meridia). Betaald in act 3 midpoint |
| **T4** | **geplant, dragend** (S03, P4-A2-c, aanraking **A2-3**) — **de enige gepolijste aanraking van de akte** |
| **T3** | **aangeraakt, niet-dragend** (S01, P3-A2-a tweede beat): de nieuwe man plant de operaties en Sela benoemt het. Twee regels |
| **T5** | niet aangeraakt behalve via de burgertelling in het handelskwartier |
| **Threx-ladder AR-6** | **geen trede** (AR-A2-3) |

---

## 7. Draden

- **Whisper** — **GEOPEND** in S05. **Sluit in `M2.7`**, binnen dezelfde akte. Draagvlag `Story.Char.WhisperContact`.
- **Wat Whisper met root-toegang doet** — wordt hier nog niet geopend; dat gebeurt pas in M2.7 en betaalt in act 4.
- **De Blight als plaats** — **GEOPEND** in S02/S04. **Betaalplek: act 3 midpoint, twist 1.** Draagvlag `Story.Clue.BlightLedger`. Act 2 sluit hem niet en mag hem niet sluiten.
- **Mara's brieven** — **niet aangeraakt** (AR-A2-5). De volgende twee adressen zijn `M2.6.S99` en `M2.7.S02`.

---

## 8. Groei

- **Voss** — hij krijgt voor het eerst een bondgenoot aangeboden die hij niet kan doorgronden, en hij is daar in act 1 nooit voor opgeleid: alles wat hij tot nu toe vertrouwde, vertrouwde hij omdat hij het zag werken. **M2.3 heeft hem geleerd dat oprechtheid geen valuta is; deze missie laat zien wat dat kost.**
- **Torren** — nullijn in S02, en daarna twee scènes waarin hij professioneel gelijk heeft. **Elk van die momenten is later munitie tegen hem.**
- **Kaya** — ze doet in S03 het beste werk van haar leven en ze draagt de enige gepolijste regel van de akte. En ze is de enige die Whisper meteen niet vertrouwt, om een reden die niets met moraal te maken heeft.
- **Dex** — hij is er niet bij en hij hoort alles. Dat is de opzet voor M2.6, waar hij er wél is en niets kan doen.
- **Sela** — één bezwaar in S01 en één vraag in S99, en beide gaan over verantwoording. **Derde missie op rij dat ze gelijk heeft en niet gehoord wordt.** Dat is nu een arc.

---

## 9. Instructies voor de dialogue-writer

1. **WHISPER IS EEN STEM.** Geen beeld, geen locatie, geen `shot:` die hem toont (§2.5, woordelijk). Zijn fingerprint is bovendien mechanisch controleerbaar en **er is geen speelruimte**: geen samentrekkingen, conditionalis, passief, *"it is known"* in plaats van *"I know"*, **geen namen, nooit "ik"**. Hij bevestigt nooit; hij impliceert.
2. **S02 IS TORRENS NULLIJN. Hij praat daar normaal en er valt precies één `[pause]`.** Dit is de belangrijkste instructie in dit document, om dezelfde reden als Kaya's nullijn in M2.3.S02: een differentiële tell zonder nullijn vuurt nooit (§18.4, L1-R19, L1-R37).
3. **AANRAKING A2-3 in S03 is de ENIGE gepolijste van act 2, en hij is van Kaya.** Dat is geen vrijbrief maar een rantsoen: als jij hier een tweede gevatte wegverklaring schrijft, of als een andere missie er een schrijft, wordt de vórm de tell en lost de speler twist 4 op door proza te herkennen (L1-R17). **Eén. In de hele akte.**
4. **Het Bursary-grootboek in S04 is GEEN aanraking.** De Bursary beschrijft zichzelf, en dat is plantmateriaal (L1-R44). Kaya leest het hardop als boekhouding en verklaart niemands gedrag weg. **Zet het niet in de aanrakingstabel.**
5. **De query in S04 is acht maanden oud en dat getal komt uit act 1** (L1-R55: beschermd is *"acht maanden"* als duur). **Verander het niet, en gebruik geen tweede duur van acht maanden in deze missie.**
6. **De te makkelijke uitweg in S07 wordt benoemd en niet verklaard.** Eén regel. Zodra iemand hem wegverklaart, is het een aanraking en dan zijn het er zeven in de akte in plaats van zes.
7. **Het register van deze planeet is BEZITTEN.** Aandeel, aanspraak, vordering, wie wat van wie tegoed heeft — ook over doden, ook tijdens een vuurgevecht. Niemand citeert *"words are ammunition"*; dat is codex-tekst met een mond eromheen (§18.9 B, Explaining).
8. **Geen totaal aantal mensen** (AR-A2-10). **Getallen:** het enige beschermde is **acht maanden** (geërfd). Draai `python Eclipse/Tools/check_spoken_numbers.py` en **schrijf geen vrijheidsclaim in een `note:`**.
9. **Voss-varianten:** S06 (de keuze) krijgt volledige dekking op beide assen; S05 krijgt idealist/pragmatist. De rest draait op de basisregel.

---

## 10. Barks

**Nieuw: Meridiaanse huismercenairs.** Register: **contractueel met pauzes.** Ze roepen naar elkaar over wie er nog betaalt en of dit onder de opdracht valt. Ze zijn scherp te scheiden van de Ashline Cartel (die roept kósten) doordat zij **over de opdrachtgever** roepen, niet over het bedrag.

**Nieuw: Bursary-inkomstenpersoneel.** Klein pakket, hoogstens één triggerset, en het is de eerste keer dat de **Bursary** als organisatie hoorbaar is — §2.2 maakt Callis een systemische antagonist voor act 3, en dit is zijn stem in de wereld voordat hij zelf spreekt.

**Nieuw: handelskwartier-omgeving.** Sociale ruimte, geen gevecht. Mensen die over aandeel praten. **Dit is het goedkoopste pakket van de akte en het draagt de planeet volledig.**

> **Casting bestaat voor geen van drieën** (C-A2-2 / Q-A2-3). **Niet oplossen door een pool te lenen** (L1-R30).
