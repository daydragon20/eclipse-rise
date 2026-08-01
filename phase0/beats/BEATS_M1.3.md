# BEAT-SHEET — M1.3 *Signal Fire*
*L1 | story-architect | 2026-07-31 | act 1, beweging II*
*Canon: `02_story_bible.md` §2.9 · `phase0/specs/SPEC-P2-04` decision 6 · `phase0/specs/SPEC-P2-05` (liberation instance)*

---

## 1. Dramatische functie

Bijbel: *"First district-level world-state change."* Dat is de systeemfunctie. De verhaalfunctie is dubbel, en de tweede helft is de belangrijkste:

> **De eerste keer dat de wereld verandert omdat de speler bestaat — en de eerste keer dat de wereld terugpraat.**

M1.1 en M1.2 waren diefstal. Dit is een **verklaring**. Een jammer-mast neerhalen is niet stiekem: het district merkt het, de Dominion merkt het, en vanaf nu is er iemand die zoekt. Dat is de prijs die de motor van act 1 int (zichtbaarheid → Mara's dood), en M1.3 is de eerste betaling.

Daarnaast draagt deze missie twee dingen die verderop zwaar wegen:
- **Vex' stem**, uit een luidspreker, zestig uur voordat de speler hem ontmoet (P1-a).
- **Threx' stem**, op een kanaal dat de cel niet had moeten horen (AR-6, trede 2).

Dat is geen toeval maar de opzet: op het moment dat de cel de Dominion het zwijgen oplegt, hoort ze hem voor het eerst écht.

**Zichtbaarheid na M1.3: 3/8.** De grootste sprong van de act.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS**, **MARA** | veld |
| **DEX** | **veld** — voor het eerst. Hij zet de lading zelf; niemand anders kan het |
| twee Ember-vechters | veld |
| **VEX** | opname uit de mastluidsprekers, drie regels |
| **THREX** | stem op een Veil-kanaal, vier tot zes regels |
| De Enforcer (alleen plaatnummer) | veld, respons |
| **REYES** | Hollow Point, S01/S99 |

---

## 3. Site & runtime-haak

- **Fictie:** de relay-mast staat op het plein bóven het transitcheckpoint — een Dominion-witgouden paal in een amberkleurige wijk, met luidsprekerkransen op drie hoogtes. Eronder loopt een servicetunnel van de oude geothermie (de Underworks-verbinding), en dat is de stille route.
- **`location`-strings:** `Kessara / Foundry District / Relay Mast` en `Kessara / Foundry District / Transit Checkpoint`
- **Regio-pin:** `TransitCheckpoint` (gemeten keuze, zie de commentaren in `setup_story_missions.py` — `CommsRelay` is thematisch verleidelijk en lane-technisch onbereikbaar)
- **Runtime:** `DestroyTarget` (mastvoet) + `ExtractSquad`; getimede responssubfase ná de knal; optional = SupplyDepot-voorraden intact (+60 M)
- **Briefingtekst in data:** *"Take the tower and the district goes deaf. It will answer - be gone before it does."* — Mara.
- **P2-05-naad:** M1.3's `Event.Mission.Completed` is de trigger voor de Foothold-liberation (`TransitCheckpoint` + `WorkerHousing` + `SupplyDepot` klappen om naar de speler). **Deze missie commit zelf geen regio-flip** (decision 6). Voor het schrijven betekent dat: S99 mag de omslag *vieren*, maar de omslag gebeurt buiten deze missie.

---

## 4. Scènelijst

### S01 — *Deaf* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes, de cel

- **want** — Mara wants the district's ears shut for one night, and she wants everyone in this room to know it is not a robbery.
- **obstacle** — Every previous cell that made a noise this loud on Kessara is a place and a count Dex can recite, and not one of them is a name he can give.
- **turn** — Dex volunteers to set the charge himself, which is the first thing he has ever asked to do.

**Beats:** (1) Wat een jammer-mast doet, verteld door wat hij de cel kost (nooit uitgelegd — §18.8). (2) **Dex' lijst met cellen die dit probeerden — plaatsen, aantallen en één leeftijd, géén namen.** Het is een Brick-achtige beat en mag daarom precies één keer. (3) De insertie-keuze: plein of tunnel — de **speler** kiest. (4) Reyes vraagt wat er gebeurt met de gewonden als de extractie onder vuur ligt. Mara antwoordt eerlijk. (5) **Dex meldt zich.**

> **RULING L1-R48 — de drie celnamen komen er niet, en de schrijver had gelijk. `.080`/`.090` blijven zoals ze staan.**
>
> Beat 2 vroeg om namen. De schrijver heeft geweigerd ze te verzinnen, geëscaleerd in plaats van stil op te lossen, en plaatsen-plus-aantallen geleverd met een leeftijd op het eind (*"The overpass, four, and two of the four were fifteen."*). **Dat is beter dan wat ik vroeg, en het is dezelfde vorm als L1-R34: mijn eis was een proxy en de tekst levert waar de proxy voor stond.**
>
> Drie gronden, op volgorde van gewicht:
> 1. **De canon heeft ze niet.** `00_INDEX.md` kent op Kessara precies twee cellen: Ember en de Iron Chorus, en die tweede leeft en is M1.5's introductie. Drie dode cellen dopen is canon verzinnen, en dat is een owner-beslissing die ik niet neem en die niets oplevert dat de scène nodig heeft.
> 2. **Naamloosheid ís het onderwerp van de akte.** Wat een naam krijgt overleeft: Ember heet Ember, de cel ruilt in M1.8.S99 een familienaam voor een symbool, en AR-10 zet **de eerste naam op de muur** in M1.8.S90. Dex' lijst is de reden dat die muur bestaat — hij kan zijn doden alleen tellen. Namen in `.080`/`.090` maken van die lijst een eregalerij en van M1.8.S90 een herhaling in plaats van een antwoord.
> 3. **De leeftijd op het eind doet het werk dat de naam moest doen.** *"two of the four were fifteen"* is Bricks apparaat één stap verschoven, en het kost nul canon.
>
> **Gevolg dat de melding niet voorzag: `.080` en `.090` worden dus NIET herschreven.** De generatiewaarschuwing (L1-R18's les) vervalt daarmee — er staat niets in deze scène op het punt te veranderen behalve één toegevoegde regel.

> **RULING L1-R49 — de REGISTER-bevinding sluit met één toegevoegde regel, `.095`, en die regel is een karakterbeat en geen vulling.**
>
> De criticus stelde voor de bevinding rood te laten met een herpunte reden. **Ik neem dat niet over, want er is een reparatie die de scène beter maakt, en hij zat vast aan dezelfde escalatie.**
>
> **Wat gemeten is en wat het betekent.** De conditie waarvoor L1-R1 de registercontrole schreef — *"een cutscene waarin niemand ooit een volle zin zegt"* — is hier **niet** vervuld: `.230` (13 w), `.270` (14 w) en `.360` (13 w) zijn volle ondergeschikte zinnen. De twintig-woordsdrempel is een **proxy**, en hij faalt hier terwijl het ding waar hij voor staat standhoudt. Dat is een argument om de bevinding niet als stijlfout te lezen — het is **geen** argument dat de scène niets mist.
>
> **Het tegenbewijs staat in dezelfde missie en het is doorslaggevend.** `M1.3.S99` is dezelfde kamer, dezelfde vier sprekers, dezelfde schrijver, en haalt **29 woorden** op `.055` — toegevoegd omdat beat 2 daar geknepen werd. De kamer produceert dus lengte zodra een beat erom vraagt. **S01 heeft precies één beat die erom vraagt, en dat is de beat die op deze escalatie stond te wachten.**
>
> **`.095` — nieuwe regel, DEX, tussen `.090` en `.100`.** ID-ruimte bestaat (het corpus gebruikt al `.045` en `.262`); niets wordt hernummerd.
>
> | Eis | Waarom |
> |---|---|
> | **≥20 woorden, één volle zin met een bijzin** | dit is de regel die de registerdrempel haalt, en hij haalt hem omdat de beat hem verdient — niet andersom |
> | **Onderwerp: waarom de lijst plaatsen, aantallen en één leeftijd heeft en geen namen** | het antwoord op beat 2, zonder één naam te verzinnen |
> | **Zijn langste regel in de scène, en zijn enige volle zin** | §18.4 geeft Dex fragmenten. **De man die in fragmenten praat produceert één hele zin, één keer, over het enige dat hem bang maakt.** Dat is de beat, en hij is machinaal te controleren |
> | **Geen maxime (L1-R40), geen triade (§18.9 B), geen benoemd gevoel (§18.9 A)** | een citeerbare zin hier maakt van zijn angst een uitspraak |
> | **Geen celnaam, geen persoonsnaam, geen plaats buiten het locatieregister** | L1-R48 |
> | **`.100` *"There's more. I can keep going."* en `.110` *"Don't."* blijven de knop** | daarom staat de regel vóór `.100` en niet erna |
>
> **De begrenzing, en die is van mij omdat alleen ik hem kan zien.** Dit is de **derde** gereserveerde grammaticale breuk in de cast: Mara's ene *"jij"* (AR-5), Petra's ene vráág (L1-R9), en nu Dex' ene volle zin. Drie is een systeem; vier is de auteur (§18.9 D). **Geen vierde personage krijgt een gereserveerde breuk**, en de drie die er zijn liggen in verschillende grammaticale categorieën (voornaamwoord, taalhandeling, zinslengte) en in verschillende akten. Dat staat hier zodat de volgende schrijver die dit apparaat mooi vindt, weet dat het op is.
>
> **Kosten: nul.** Niets van M1.3 is gegenereerd, geen bestaande regel verandert, en `words:` gaat met de nieuwe regel mee omhoog — de schrijver telt bij.
**Waarom Dex mee moet:** dit is de missie waarin de ingenieur ophoudt de stem in het oor te zijn. Hij loopt mee, hij is bang, hij is de beste. Vanaf M1.4 blijft hij weer thuis, en dan mist de speler hem — precies zoals het hoort.

### S02 — *The Anniversary* · de nadering
`Kessara / Foundry District / Relay Mast` · ambient + walk-and-talk · tier 2
**Aanwezig:** Voss, Mara, Dex, vechters · **VEX (opname)**

- **want** — Four people want to cross a lit plaza that is currently broadcasting to itself.
- **obstacle** — The mast is mid-address and the plaza is full of people standing still to listen, because standing still is what you do.
- **turn** — The address ends, the plaza empties on the curfew tone, and the cell walks in behind the last of the crowd.

**Beats:** (1) Het plein: witgoud licht, mensen die luisteren zoals je luistert als niet-luisteren opvalt. (2) **De opname.** Vex, zacht, lange zinnen, "one" in plaats van "ik", en een sterftecijfer van vóór de Dominion als troost (§18.4). Drie regels. Onder de 60 woorden per regel — dit is géén oration (AR-7). (3) Eén reactie uit de cel — kort, laconiek, iemand die dit zijn hele leven hoort. (4) De curfew-toon; het plein leegt. (5) Erin.

> **P1-a. Dit is de plant voor twist 1 en hij is gratis.** De speler hoort de eindbaas zijn eigen misdaad verdedigen zestig uur voor hij weet dát het een misdaad is. Regels voor de schrijver:
> - Vex verdedigt de **Dominion**, niet de Blight. Hij noemt de Blight als *ramp die overkwam*, precies zoals iedereen dat gelooft.
> - Geen enkel personage reageert op de inhoud. Ze reageren op het volume.
> - Zijn signature-regel uit §19.3 (*"Before us, one in nine children on Meridia did not reach eleven..."*) hoort thuis in act 4 en wordt hier **niet** verbruikt. Schrijf hem nieuw, in dezelfde toon.

**Systeem:** `Story.Clue.BlightBroadcast`.

### S03 — *The Tunnel* / *The Plaza* · de route
`Kessara / Foundry District / Relay Mast` · walk-and-talk · tier 2
**Aanwezig:** afhankelijk van de route

- **want** — Get Dex within arm's reach of the mast footing with the charge intact.
- **obstacle** — The quiet way is long and floods, the loud way is short and lit.
- **turn** — Whichever they take, Dex talks the entire distance, and the cell lets him, because that is how they know he is frightened.

**Twee volledige takken.** Tunnel: oude geothermie, water, Voss die het systeem herkent (technicus-vocabulaire, en het is *zijn* wereld — hij is derde generatie). Plein: te veel licht, te weinig dekking, en een Enforcer-patrouille die op tijd moet passeren.
**Karakterbeat die in beide takken staat:** Dex praat zonder ophouden en niemand kapt hem af. Eén regel van Mara die dat benoemt zonder het te benoemen.

### S04 — *Sixty Seconds* · de lading
`Kessara / Foundry District / Relay Mast` · in-mission-radio · tier 2
**Aanwezig:** Dex, Voss, Mara

- **want** — Dex wants sixty seconds and nobody standing over him.
- **obstacle** — The relief patrol is early, and Voss has to decide whether to tell him.
- **turn** — Voss tells him, and Dex works faster and worse, and it holds anyway.

**Beats:** (1) Dex aan de mastvoet; techniek, geen uitleg. (2) De patrouille komt vroeg. (3) **Voss' keuze:** zeggen of stilhouden. (4) Dex' reactie — bij "zeggen" een grap die op de helft breekt; bij "stilhouden" een grap achteraf die harder aankomt. (5) De knal.
**Speler-keuze zonder systeemgevolg** — hij verandert alleen hoe de scène klinkt. Dat is `21_quality_mandate.md` §21.2 in de praktijk: variatie die zich verantwoordt.

### S05 — *The District Answers* · de respons
`Kessara / Foundry District / Transit Checkpoint` · in-mission-radio + callout · tier 2
**Aanwezig:** allen; Enforcer-respons

- **want** — Get out through a district that has just discovered it is being attacked.
- **obstacle** — For the first time the Dominion is not policing them; it is hunting them.
- **turn** — Voss recognises a shoulder plate in the response line and keeps moving anyway.

**Beats:** (1) Alarmtonen, sluitende poorten, mensen die naar binnen rennen. (2) De respons — Enforcers, walkers in de verte (`03_world_design.md` §3.3: urban pacification walkers). (3) **De badge.** Eén regel, hoogstens twee: Voss ziet de plaat die hij uit de rij kent. Hij zegt niets tegen de anderen, of hij zegt precies vier woorden. (4) De aftocht.
**Enforcer-draad:** dit is de enige keer in act 1 dat de speler hem levend ziet. Hij overleeft. Wie hem hier doodschrijft, sluit een draad die §2.11 openhoudt tot het einde van de campagne — **verboden**.

### S06 — *A Name On The Air* · de jager
`Kessara / Foundry District / Transit Checkpoint` · in-mission-radio · tier 2
**Aanwezig:** Voss, Mara, Dex · **THREX (stem)**

- **want** — With the jammer down, Dex wants to know what the district's own channels sound like.
- **obstacle** — One of those channels is not meant for a district.
- **turn** — A man on a Veil frequency says a first name, warmly, about somebody Ember has never heard of — and Mara stops walking.

**Beats:** (1) Dex scant; de lucht is voor het eerst leeg. (2) Een kanaal dat er niet hoort te zijn. (3) **Threx.** Warm, intiem, vraag na vraag, en hij gebruikt een **voornaam van iemand buiten Ember** (AR-6, trede 2 — een lid van een andere cel; dit maakt tegelijk M1.5 mogelijk). Hij complimenteert die persoon oprecht terwijl hij hem pijn doet (§18.4). Vier tot zes regels. (4) Het kanaal valt weg. (5) Mara zegt één ding, en het is niet geruststellend. (6) Dex maakt géén grap. Dat is zijn karakterbeat: de enige plek in act 1 waar hij een opening laat liggen.

> **AR-6, trede 2.** Threx mag hier **geen Ember-naam** noemen. Doet hij dat wel, dan is M1.7's hele reden van bestaan weg. De ladder is: vreemde naam (hier) → Ember-naam (M1.7.S04) → Voss' naam (M1.8.S05).

**Systeem:** `Story.Clue.ThrexVoice_1`.

### S99 — *First Light* · debrief
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 2
**Aanwezig:** Mara, Voss, Dex, Reyes, de cel

- **want** — Mara wants to mark the map, because it is the first time in five years there is anything to mark.
- **obstacle** — The thing they did tonight cannot be undone, and one of them keeps saying so.
- **turn** — She marks it anyway, and asks Voss where the next one goes.

**Beats:** (1) Terugkomst; wie er gewond is. (2) Dex, die niet kan stoppen met praten over de mast en daarmee iets anders wegduwt. (3) De kaart. Mara's markering. **Dit is de emotionele piek van beweging II.** (4) Reyes' klinische regel over wat de nacht kostte. (5) **Mara vraagt Voss waar de volgende komt.** Dat is de tweede keer dat ze zijn oordeel boven het hare zet (na M1.1.S04) en de speler moet het patroon nu net gaan voelen. (6) Threx' stem hangt onbesproken in de kamer. Niemand noemt hem. Eén regel die eromheen loopt.
**Systeem:** `Story.Beat.M13_SignalFire`, +40 M / +60 C / +4 I, dag +1, optional +60 M. Daarna neemt SPEC-P2-05 het over: Foothold-flips.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.M12_DeadDrop` | `Story.Beat.M13_SignalFire` ✔ · `Story.Clue.BlightBroadcast` ✚ · `Story.Clue.ThrexVoice_1` ✚ |

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T1** Blight | **geplant, dragend** (S02, P1-a). Vex' stem, drie regels, geen oration. |
| **T4** | niet hier. M1.3 is juist de missie waar de Dominion **wél** volledig reageert — anders wordt de dunne respons in M1.2/M1.5/M1.7 een patroon in plaats van een detail. Het contrast is de camouflage. |
| Threx-ladder AR-6 | **trede 2** (S06). |
| Enforcer-draad | **aangeraakt** (S05), levend gelaten. |
| T2, T3, T5 | niet aangeraakt. |

---

## 7. Groei

- **Dex** — de hele missie is zijn arc-in-het-klein: hij meldt zich (S01), hij is bang (S03), hij is uitstekend (S04), en hij verliest zijn grap (S06). §2.5 zegt: *"learns his machines are only as good as the people in them."* Dit is de eerste helft van die les — de machine werkte, en het voelde niet zoals hij dacht.
- **Voss** — van uitvoerder naar planner. Hij kiest de insertie, hij kiest of Dex de waarheid krijgt, en hij krijgt aan het eind de vraag waar de volgende komt.
- **Mara** — de scheur wordt zichtbaar. Ze wilde dit vijf jaar geleden en kon het niet; nu gebeurt het en het maakt haar niet blij, het maakt haar **haastig**. Die haast doodt haar in M1.8. Plant hem hier, in één regel, zonder uitleg.

---

## 8. Instructies voor de dialogue-writer

1. **Vex krijgt drie regels en geen woord meer.** Hij is behang met een stem. Als je hem meer geeft, is hij in uur acht al een personage, en dan heeft act 4 niets meer.
2. **Threx krijgt vier tot zes regels en noemt geen Ember-naam** (AR-6). Hij is het engst als hij aardig is.
3. **Dex' grap in S06 bestaat niet.** Schrijf de stilte. §18.7: stilte is inhoud.
4. **Niemand zegt dat dit een keerpunt is.** §18.9 A verbiedt de thesis-zin. De kaartmarkering in S99 doet het werk; een personage dat "vanaf nu is alles anders" zegt, maakt het kleiner.
5. **De Enforcer sterft niet.** Zie S05.
6. **Twee volledige routetakken in S03.** Beide met dezelfde Dex-beat, andere omgeving.
7. **Voss-varianten:** S01 (insertiekeuze), S04 (wel/niet waarschuwen), S99 (waar de volgende komt). Drie scènes, en dat is voor deze missie het maximum — zie C-6.

## 9. Barks

Nieuw ten opzichte van M1.1–M1.2: **alarm/stealth-broken op districtsniveau**, walker-nadering, evacuerende burgers, objective complete op de mastknal. Dominion-conscriptvocabulaire onder paniek: procedureel wordt kortademig. **Veil-vocabulaire nog steeds niet** — Threx' stem in S06 is scriptdialoog, geen bark, en er staan geen Veil-operatives op straat tot M1.7.
</content>
