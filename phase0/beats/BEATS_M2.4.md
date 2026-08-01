# BEAT-SHEET — M2.4 *Loyal Ghost*
*L1 | story-architect | 2026-08-02 | act 2, beweging II*
*Canon: `02_story_bible.md` §2.9 (*"first space asset (stolen corvette Loyal Ghost)"*), §2.5 (Torren Vale, Kaya Renn), §2.6 (Kaine — *"Torren Vale's former commander and estranged sister-in-arms"*) · `00_INDEX.md` glossary (*Loyal Ghost*) · `03_world_design.md` §3.3 (THE SHROUD — *"the Loyal Ghost corvette theft, fleet tutorial operations"*) · `11_missions.md` §11.2 (Heist, Boarding Action)*
*Erft: `ACT2_OVERVIEW.md` §7 (Shroud-register), §8 (AR-A2-1, AR-A2-2, AR-A2-4) · `18_writing_standard.md` §18.4 · `RULINGS_L1.md` L1-R19, L1-R37*

---

## 1. Dramatische functie

Bijbel: *"first space asset (stolen corvette Loyal Ghost)."* `03_world_design.md` §3.3 zet de diefstal expliciet op de Shroud.

> **De missie waarin Eclipse zijn eerste schip steelt — en waarin blijkt dat een schip stelen niet het probleem is. Het uitklaren wel.**

Een korvet vliegen kan Kaya. Een korvet *wegkrijgen* kan niemand: een Dominion-vaartuig verlaat een dok op gezag, en gezag is een stem die weet hoe de Armada praat. Daarom is dit de missie waarin **Torren Vale** binnenkomt — niet als aanwinst, maar als het ontbrekende gereedschap.

**En dat is precies waarom hij in act 3 hangt.** §2.8 twist 3: de Veil voert de coalitie vervalst bewijs dat Torren Kaine's mol is. Die vervalsing werkt alleen als de speler zich kan herinneren dat Torren binnenkwam **omdat Eclipse iets van hem nodig had**, en niet omdat iemand hem vertrouwde. Deze missie moet dat feit hard op tafel leggen en het daarna laten liggen.

De tweede helft van de dramatische functie is stiller: **dit is de missie waarin Voss voor het eerst een individu in de tweede persoon aanspreekt.** Zie AR-A2-2 — die ladder loopt sinds `M1.1` en dit is trede drie.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS** | overal |
| **KAYA** | overal. Zij vliegt, en zij is de enige die weet waar Torren is |
| **DEX** | mee — een korvet is de grootste machine die hij ooit heeft aangeraakt |
| **TORREN** Vale | **vanaf S03.** Zijn introductie. **Zijn lange stiltes vuren hier NIET — zie §9.4** |
| **SELA** | The Carcass, radio. Eén scène, één bezwaar, en het is het juiste |
| **BRICK** | The Carcass. Niet mee |
| Dominion-dokpersoneel / douane | S04, S05, S06 — barks + één officier-spreekrol |
| Shroud-dokwerkers | `CIVILIAN_A`/`_B` |

**Spreekt niet:** Whisper, Threx, **Kaine** (AR-A2-1: haar eerste regel valt in M2.9.S08 en geen seconde eerder — ook niet in een opname, ook niet in een clearance-protocol), Petra, Reyes, Mara.

> **De grootste verleiding van deze missie is een Kaine-stem in het klaringsprotocol.** Niet doen. Torren die haar *beschrijft* is het hele punt van zijn introductie; Kaine die zichzelf laat horen, is de act-9-beat vijf missies te vroeg.

---

## 3. Site & runtime-haak

- **Fictie:** de *Loyal Ghost* ligt aan een drift-station in de baan om de gasreus — een Dominion-korvet in onderhoud, met een halve bemanning en een klaringsvenster. Buiten het station: de stormvlakte van de reus.
- **`location`-strings** (`ACT2_OVERVIEW` §7): `Tarsis / Dune Sea / The Carcass — Map Table` · `The Shroud / Nym / Docking Sprawl` · `The Shroud / Nym / Dome City` · `The Shroud / Drift Station` · `Loyal Ghost / Bridge`
- **Regio-pin:** **bestaat niet** — C-A2-3.
- **Runtime:** Heist met een boarding-staart. Objectives: `ReachLocation` (drift station) → `Infiltrate` → klaringsfase (dialoog onder tijdsdruk) → `ExtractSquad` per schip. Optional: geen alarm; het onderhoudspersoneel ongedeerd.
- **Nieuw:** een ruimte-asset en een schip als speelruimte. **C-A2-3.** De dialoog beschrijft het schip; ze vraagt geen vlootsysteem.

---

## 4. Scènelijst

### S01 — *A Thing That Moves* · briefing
`Tarsis / Dune Sea / The Carcass — Map Table` · cutscene · tier 5
**Aanwezig:** Voss, Dex, Kaya + Sela en Brick op de radio

- **want** — Voss wants a ship, because two planets and a moon cannot be run out of somebody else's cargo hold.
- **obstacle** — The syndicate will rent him lift, but renting is how the syndicate owns you.
- **turn** — Kaya names a target she should not know about, and does not say how she knows.

**Beats:** (1) De lane-kaart uit M2.3 in gebruik; de kosten van gehuurde lift zijn zichtbaar en pijnlijk. (2) Dex' opsomming van wat een korvet is en wat het niet is. Hij is hier het gelukkigst dat hij in twee akten is geweest en dat maakt hem in S05 het bangst. (3) **Kaya noemt de *Loyal Ghost*.** Ze weet welk schip, welk dok, welk venster. Ze legt niet uit hoe. **Eén regel, en er wordt niet doorgevraagd** — dat is Shroud-cultuur en het is stil plantwerk: informatie is daar handel en iemand verkocht dit. (4) Sela's bezwaar over de radio: een gestolen oorlogsschip maakt van Eclipse een marine, en marines hebben regeringen nodig. Ze heeft gelijk en ze wordt overstemd.
**Vlaggen:** **leest `Story.Char.KayaJoined`** en `Story.Beat.M23_Nym`. **Zet niets.**

### S02 — *The Contract* · de rekening
`The Shroud / Nym / Docking Sprawl` · walk-and-talk · tier 5
**Aanwezig:** Voss, Kaya, Dex

- **want** — Kaya wants the crew paid before anyone climbs into a Dominion hull.
- **obstacle** — How Eclipse pays depends on a promise Voss made on this moon two days ago.
- **turn** — She tells him what he is still missing, and it is not money and it is not people.

**Beats:** (1) **De sub-draad uit `M2.3.S04` wordt hier gesloten** — het contract wordt uitgevoerd, en het voelt anders per tak:
- **`paid`:** Eclipse levert legering en erts. Het dok is beleefd en de basis is armer. Dex zegt er iets over dat pijn doet.
- **`owed`:** Eclipse betaalt niets en het dok is *vriendelijker*, en dat is erger. Iemand noteert iets in een boek.
(2) De crew: Shroud-mensen die dit voor geld doen en dat niet verbergen. (3) **Kaya benoemt het echte probleem: het schip komt niet los zonder gezag.** Een Dominion-romp verlaat een dok op een stem die weet hoe de Armada praat. (4) Ze zegt dat er zo iemand op deze maan is, en dat niemand hem koopt of verkoopt **omdat hij te heet is**. Eén regel over wie hij was; geen dossier. (5) Dex' grap over wat er nog meer op deze maan te koop is.
**Vlaggen:** **leest `Story.Choice.M23_Terms.{Paid,Owed}`** — twee takken, gescheiden dialoog. **Zet niets.**
**Sub-draad gesloten:** *wat de syndicaten aan Eclipse verkocht hebben* (geopend `M2.3.S04`).

### S03 — *The Exile* · Torren
`The Shroud / Nym / Dome City` · cutscene · tier 5
**Aanwezig:** Voss, Kaya · **Torren Vale**

- **want** — Voss wants a voice that can talk a Dominion warship out of a dock.
- **obstacle** — The man who owns that voice has spent four years making sure nobody needs him for anything.
- **turn** — He agrees, and he says the price out loud, and the price is that Eclipse will never be able to say he asked to come.

**Beats:** (1) Waar een gedefecteerde kolonel woont als niemand hem durft aan te geven: in het volle zicht, in een neutrale zone, met een rekening die hij betaalt. (2) **Torrens eerste regels van het spel.** §18.4: militaire economie, bevelen als suggesties, verheft nooit zijn stem, gebruikt nooit een callsign die hij niet verdiend heeft. **Hij noemt Voss geen "Cinder".** (3) De Sylvaris Reprisals worden **niet uitgelegd** (§18.8). Ze worden één keer genoemd, door Kaya, als een ding dat iedereen weet, en Torren reageert er niet op. **Dat niet-reageren is zijn introductie.** (4) Hij vraagt niet wat Eclipse gelooft. Hij vraagt wat Eclipse kan verliezen — een beroepsvraag, en het is de eerste keer dat iemand in dit spel Voss zo aankijkt. (5) **Hij zegt ja, en hij zegt waarom niet.** Hij komt niet mee omdat hij gelooft; hij komt mee omdat het schip er anders niet uitkomt. **Dit moet de speler onthouden en het moet niet onderstreept worden** — het is de plant waarop twist 3 in act 3 rust (P3-A2-a).
**Vlaggen:** **leest `Story.Char.KayaJoined`**. **Zet niets** — hij is nog niet aangenomen.

### S04 — *Maintenance Window* · de infiltratie
`The Shroud / Drift Station` · in-mission-radio + callout · tier 5
**Aanwezig:** Voss, Dex, Kaya, Torren + twee vechters · Dominion-onderhoudspersoneel

- **want** — Eclipse wants to be aboard before the maintenance shift rotates.
- **obstacle** — There are people on that ship who are not soldiers and who will die if this goes loud.
- **turn** — Torren gives an order phrased as a suggestion, everyone obeys it instantly, and Voss notices that.

**Beats:** (1) Het station: een dok in de baan, met de stormvlakte van de reus als plafond. (2) **Torren commandeert voor het eerst** — en het is de §18.4-vorm: hij formuleert het als een voorstel en niemand hoort er een voorstel in. (3) Voss merkt het. Eén regel of geen regel; het mag ook alleen een `shot:` zijn. (4) Het onderhoudspersoneel is burgerpersoneel en de speler kan ze sparen. **Dit telt mee voor twist 5** (§2.8): burgerslachtoffers laag. (5) Kaya op het schip: ze praat tegen het korvet zoals andere mensen tegen een dier praten, en ze ondermijnt haar eigen laatste zin. (6) Dex ziet de machinekamer en zegt niets, en dat is de eerste keer.
**Band:** `in-mission-radio` met `band: callout` op de gevechtsregels.
**Vlaggen:** zet niets.

### S05 — *Clearance* · de brug
`Loyal Ghost / Bridge` · cutscene · tier 5
**Aanwezig:** Voss, Torren, Kaya, Dex · een Dominion-havenofficier (alleen stem, over de klaring)

- **want** — Voss wants the ship cleared out of the dock in the next ninety seconds.
- **obstacle** — Clearance is a conversation, and the only man who can have it is the one nobody in the room has decided to trust.
- **turn** — Voss gives Torren the ship, in the second person, out loud — and that is the first time Voss has commanded a single human being that way.

**Beats:** (1) De brug: koud, Dominion-standaard, iemand anders' schip. (2) **De klaring.** Torren praat met de havenautoriteit in een register dat de speler nog nooit gehoord heeft: precies, beleefd, volstrekt ongehaast. **De havenofficier is een rolspreker en geen personage** — en het is uitdrukkelijk **niet Kaine** (AR-A2-1). (3) Er gaat iets bijna mis en Torren repareert het door **niets te zeggen** — één beat stilte, en de autoriteit vult hem zelf in. Dit is de enige plek in de missie waar zijn stilte-instrument mag vuren, en hij is **kort**, want zijn eigen beats komen pas in act 3. (4) **DE BEAT VAN AR-A2-2.** Voss draagt het schip aan Torren over in de **tweede persoon**, tegen één man. Mara spendeerde dat woord één keer, in `M1.8.S08`, en dat was de commando-overdracht; act 1's slotscène gaf het aan niemand. **Hier geeft Voss het door.** Eén regel. Er wordt niet over gepraat. (5) Torren neemt het aan en zegt precies wat hij gaat doen voordat hij het doet — en dat is Kaine's fingerprint (§18.4), in de mond van de man die onder haar diende. **Niemand merkt het op.** Act 3 doet dat.
**Systeem:** zet **`Story.Char.TorrenJoined`** ✚ — **hier, waar hij instapt, en niet in de debrief.** Dezelfde redenering als L1-R38 voor clues: een debrief speelt onvoorwaardelijk, en dit is het moment waarop het waar wordt.
**Gelezen door:** `M2.5.S01` · **`M2.6.S03`** (aanraking A2-4) · **`M2.9.S07`** (aanraking A2-5) · **act 3, twist 3 — de dragende lezer.** Volledig register: `ACT2_OVERVIEW` §6.
**Voss-varianten:** volledige dekking op beide assen. **De personal/strategic-as draagt hier het meeste**: geef je een man een schip of geef je een schip een man.

### S06 — *Out* · de ontsnapping
`Loyal Ghost / Bridge` · in-mission-radio + callout · tier 5
**Aanwezig:** Voss, Kaya, Torren, Dex · Dominion-douanevloot

- **want** — Kaya wants to be in the lane before anyone reads the clearance twice.
- **obstacle** — Somebody reads it twice.
- **turn** — They get out, and the manner of getting out tells Voss who he has just hired.

**Beats:** (1) De douanevloot komt achter hen aan. **Dit is de eerste ruimtescène van het spel** en hij is kort, want het is geen vlootgevecht — het is een vlucht. (2) **Kaya vliegt en Torren vecht, en ze zijn het over alles oneens.** Twee vakmensen met tegengestelde reflexen; dat is de relatie voor drie akten. (3) Torren stelt één ding voor dat koud is en werkt. (4) Kaya doet iets roekeloos dat óók werkt en ze ondermijnt het meteen. (5) De lane. Stilte. Dex zegt eindelijk iets over de machinekamer en het is één woord.
**Band:** `in-mission-radio` met `band: callout`.
**Vlaggen:** zet niets.

### S99 — *Ours Now* · debrief
`Loyal Ghost / Bridge` · cutscene · tier 5
**Aanwezig:** Voss, Kaya, Dex, Torren · Sela en Brick op de radio

- **want** — Voss wants the room to agree that the man on the bridge belongs there.
- **obstacle** — Sela will not agree, and her objection is not paranoia, it is arithmetic.
- **turn** — Torren agrees with Sela, in front of everyone, and that makes it worse instead of better.

**Beats:** (1) Het schip is van Eclipse. Dex geeft het geen nieuwe naam en zegt waarom niet — **het heet *Loyal Ghost* en dat blijft zo** (glossary). (2) **Sela's bezwaar over de radio**, en het is het scherpste van de akte: een ex-kolonel die de vijand van binnenuit kent, kent ook Eclipse van binnenuit zodra hij binnen is. **Ze noemt geen verraad.** Ze noemt toegang. (3) **Torren geeft haar gelijk**, kort, zonder zich te verdedigen — precies wat een man doet die niets te verbergen heeft en precies wat een man doet die veel te verbergen heeft. **Dit is de hele opzet van twist 3 en er wordt niets meer aan toegevoegd.** (4) Brick, één regel, over het schip en niet over de man. (5) Kaya vraagt wanneer ze betaald wordt en het is geen grap.
**Systeem:** zet **`Story.Beat.M24_LoyalGhost`** ✚ en **`Story.Intel.LoyalGhost`** ✚.
**Gelezen door (`Story.Intel.LoyalGhost`):** act 3 vlootlaag · `M2.7.S01` (het schip vervoert de verhuizing) · `M2.9` (insertie).

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Char.KayaJoined` (S01, S03) · `Story.Beat.M23_Nym` (S01) · `Story.Choice.M23_Terms.{Paid,Owed}` (S02, 2 takken) | **`Story.Char.TorrenJoined`** ✚ (**S05**) · **`Story.Beat.M24_LoyalGhost`** ✚ (S99) · **`Story.Intel.LoyalGhost`** ✚ (S99) |

**`Story.Char.TorrenJoined` wordt in S05 gezet en niet in S99.** Volledig register en de kolom *gelezen door*: `ACT2_OVERVIEW` §6.

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T3** de mol / de Vale-kwestie | **geplant, dragend** (S03 + S05 + S99, P3-A2-a): Torren komt binnen **omdat Eclipse iets van hem nodig heeft**, hij zegt dat zelf, en Sela's bezwaar gaat over toegang en niet over trouw. **Dit is de plant waar twist 3 op staat en hij mag niet verdiept worden** — één bezwaar, één instemming, geen tweede scène |
| **T5** | **aangeraakt, niet-dragend** (S04): het onderhoudspersoneel is burgerpersoneel en kan gespaard worden. Telt mee in de twist-5-telling |
| **T4** | **niet aangeraakt.** Geen aanraking in deze missie — de zes liggen vast in `ACT2_OVERVIEW` §4 |
| **T1, T2** | niet aangeraakt |
| **Threx-ladder AR-6** | **geen trede** (AR-A2-3) |

---

## 7. Draden

- **Wat de syndicaten verkocht hebben** — **GESLOTEN** in S02 (geopend `M2.3.S04`). Binnen één missie afstand, zoals afgesproken.
- **Is Torren de rebellie aan het leren winnen, of herbouwt hij wat hij ontvluchtte (§2.5)** — **GEOPEND** in S99. **Betaalplek: act 3, twist 3.** Draagvlag `Story.Char.TorrenJoined`. Act 2 raakt hem daarna nog twee keer aan (M2.6.S03, M2.9.S07) en maakt hem nergens af.
- **Mara's brieven** — **niet aangeraakt** (AR-A2-5).

---

## 8. Groei

- **Voss** — hij neemt iemand aan die hij niet vertrouwt omdat hij hem nodig heeft, en hij weet dat dat een andere soort beslissing is dan alles wat hij in act 1 nam. **En hij geeft in S05 de tweede persoon door** (AR-A2-2), wat betekent dat hij eindelijk iemands commandant is in plaats van iemands opvolger.
- **Torren** — introductie. Hij is competent, eerlijk, koel en volstrekt ondoorgrondelijk, en die vier dingen tegelijk zijn de reden dat de vervalsing in act 3 werkt.
- **Kaya** — tweede missie, en de eerste waarin ze iets doet wat haar geld kost. Ze zegt het niet.
- **Dex** — hij krijgt de grootste machine van zijn leven en wordt er stil van. Zijn ene woord aan het eind van S06 is de beste regel die hij in deze missie kan krijgen.
- **Sela** — ze verliest opnieuw, en ze verliest opnieuw correct. **Twee verloren argumenten in twee missies is een arc en geen toeval** — zorg dat de speler dat merkt.

---

## 9. Instructies voor de dialogue-writer

1. **Torren komt binnen als GEREEDSCHAP, niet als bondgenoot, en hij zegt dat zelf.** Dit is de dragende plant voor twist 3 (act 3). Wie hem er warm in schrijft — een handdruk, een gedeelde overtuiging, een moment van herkenning — maakt de vervalsing in act 3 onmogelijk, want dan heeft de speler een reden om hem te vertrouwen die niet uit bewijs komt.
2. **De Sylvaris Reprisals worden niet uitgelegd.** §18.8. Eén vermelding, door Kaya, terloops; Torren reageert niet. **Wie er een monoloog van maakt, verbrandt zijn act-3-loyaliteitsarc.**
3. **Kaine spreekt niet** — ook niet in het klaringsprotocol, ook niet als opname (AR-A2-1). De verleiding is groot en het antwoord is nee.
4. **TORRENS LANGE STILTES VUREN IN DEZE MISSIE NIET** — op één korte uitzondering in S05.3 na. §18.4 geeft hem *"longest silences in the game — his beats are marked `[pause]`"*, en dat is een **differentiële** tell: hij werkt alleen als de speler eerst weet hoe hij normaal getimed praat. **Zijn nullijn ligt in `M2.5.S02`** (AR-A2-4). Wie zijn stiltes hier al uitgeeft, geeft ze uit voordat er iets is om ze tegen af te zetten — precies de fout die L1-R19 bij Reyes vond, en dan één akte te vroeg in plaats van te laat.
5. **Het schip houdt zijn naam.** *Loyal Ghost* staat in de canon-glossary. Dex mag erover mopperen; hij mag hem niet hernoemen.
6. **Het Shroud-register is BELOVEN** en het geldt ook aan boord (`ACT2_OVERVIEW` §7): Kaya's crew praat in toezeggingen met een prijs. **Torren praat in geen enkel planeetregister** — hij is Armada, en zijn taal is de enige in act 2 die overal hetzelfde klinkt. Dat is een gratis strip-test-scheiding (§18.9 C1) en het is ook karakterisering: hij hoort nergens.
7. **Geen totaal aantal mensen** (AR-A2-10). En **negentig seconden** in S05's `want:` is proza, geen dialooggetal — spreek het niet uit tenzij je er een register-rij voor aanvraagt.
8. **Voss-varianten:** S05 krijgt volledige dekking op beide assen (het is de duurste beat van de missie en de enige die hem waard is). S03 krijgt idealist/pragmatist. De rest draait op de basisregel.

---

## 10. Barks

**Nieuw: Dominion-douane en havenpersoneel.** Register: **procedureel, en niet bang** — dit zijn geen conscripten in een vuurgevecht, dit zijn ambtenaren op een dok. Het is het goedkoopste onderscheid binnen het bestaande Dominion-vocabulaire en het maakt de Armada-escalatie in M2.6 hoorbaar groter.

**Nieuw: scheepsbarks** (Kaya's crew, de brug, de manoeuvres). **Dit is de eerste vlootlaag van het spel** en of hij een eigen pool krijgt is een `voice-director`-afweging (C-A2-2 / Q-A2-3). **Niet oplossen door de Eclipse-pool te lenen** — L1-R30: een pool-sleutel noemt de factie die de spreker levert, nooit het werk dat hij doet, en de Shroud-crew is geen Eclipse.
