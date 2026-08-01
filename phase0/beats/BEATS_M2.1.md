# BEAT-SHEET — M2.1 *The Carcass*
*L1 | story-architect | 2026-08-02 | act 2, beweging I*
*Canon: `02_story_bible.md` §2.9 (act 2, *"establishing the Tarsis desert stronghold"*), §2.4 · `03_world_design.md` §3.3 (TARSIS), §3.2, productieregel 4 · `05_base_building.md` §5.2 (*"Act 2: Tarsis wreck-fortress 'The Carcass'"*) · `11_missions.md` §11.2 (liberation-template fase 1: Foothold)*
*Erft: `ACT2_OVERVIEW.md` §7 (Tarsis-register), §8 (AR-A2-2, AR-A2-5, AR-A2-10) · `ACT1_OVERVIEW.md` §8 · `RULINGS_L1.md`*

---

## 1. Dramatische functie

Bijbel: *"establishing the Tarsis desert stronghold."* `05_base_building.md` §5.2 geeft die vesting een naam die al canon is: **The Carcass**.

> **De missie waarin Eclipse voor het eerst grond nodig heeft die niet van hem is — en ontdekt dat gedeelde vijandschap geen betaalmiddel is.**

Op Kessara werkte Voss' hele repertoire omdat iedereen dezelfde honger had. Tarsis heeft die honger niet. `03_world_design.md` §3.3: *"~40M scavenger-nomads in wreck-towns; fiercely independent salvage clans with a debt-and-honour culture ('hull-right' salvage law)."* Deze mensen zijn niet onderdrukt zoals Kessara onderdrukt is; ze zijn belást — er is een *salvage-tax-garnizoen*, geen bezetting. Ze hebben geen bevrijder nodig. Ze hebben een schuldenaar nodig die betaalt.

Dat is de hele missie: **Voss moet iets aanbieden in plaats van iets delen**, en hij is daar slecht in, en de speler ziet dat.

**Dit is ook de missie die act 2's motor start.** Aan het eind bezit Eclipse een tweede plek. Vijf missies later is dat de enige plek die hij nog heeft.

---

## 2. Cast

| Wie | Waar |
|---|---|
| **VOSS** | overal — dit is zijn eerste missie als commandant buiten Kessara |
| **DEX** | mee in het veld (hij moet de vesting beoordelen) en op de radio |
| **BRICK** | mee. Hij zegt in de hele missie hoogstens tien woorden en twee ervan tellen |
| **SELA** | mee — dit is een politieke missie en dat is haar vak |
| **REYES** | Hollow Point, radio en briefing. Zij gaat niet mee, en dat is een beslissing die zij uitlegt |
| **PETRA** | Hollow Point, S01. Twee regels, allebei taken |
| Een **salvage-clanoudste** | S03, S04, S06 — de tegenspeler. **Fingerprint en casting ontbreken (C-A2-2 / Q-A2-3)** |
| Een **Ashline Cartel-spreker** | S05 — de collaborateurs. **Fingerprint en casting ontbreken (C-A2-2 / Q-A2-3)** |
| Dominion salvage-tax-garnizoen | S05, S06 — barks. Géén Veil (dat is de plant) |
| Eclipse-vechters | rolsprekers `FIGHTER_A`/`_B` |

**Spreekt niet:** Kaya, Torren, Whisper, Threx, Kaine, Mara live. Mara's opname in S99 is de uitzondering en staat vast in AR-A2-5.

---

## 3. Site & runtime-haak

- **Fictie:** een duinzee bezaaid met kilometers lange scheepskarkassen uit de Reunification Wars. Één ervan — **The Carcass** — is groot genoeg om een basis te zijn en leeg genoeg om nog van niemand te zijn, en dat is precies waarom er ruzie over is. Daghitte 55 °C: buiten wordt er 's nachts gewerkt.
- **`location`-strings** (allemaal in `ACT2_OVERVIEW` §7): `Kessara / Underworks / Hollow Point — Map Table` · `Tarsis / Dune Sea / Wreck-Town` · `Tarsis / Dune Sea / The Carcass` · `Tarsis / Dune Sea / Salvage Yard`
- **Regio-pin:** **bestaat niet** — de gebouwde graaf kent alleen Kessara-regio's. Bevinding **C-A2-3**. Niet omzeilen.
- **Runtime:** Foothold. Objectives: `ReachLocation` (het wrak) → gespreksfase → `ClearArea`/`HoldPosition` (het garnizoen en de Cartel) → basis-claim. Optional: geen enkele clan-doding; het bergingsdok intact.
- **Nieuw ten opzichte van act 1:** hitte-timers buiten voertuigen op het middaguur (§3.3) en een tweede basislocatie. Beide zijn systeemtaken en beide staan in C-A2-3.

---

## 4. Scènelijst

### S01 — *Somewhere That Isn't Here* · briefing
`Kessara / Underworks / Hollow Point — Map Table` · cutscene · tier 5
**Aanwezig:** Voss, Dex, Reyes, Sela, Brick, Petra

- **want** — Voss wants a second place to put people, before the first one runs out of room.
- **obstacle** — Nobody in the room has ever left Kessara, and the one who is loudest about going is the one who is staying.
- **turn** — Reyes refuses to go, and her reason makes the trip more urgent instead of less.

**Beats:** (1) De kaart is voor het eerst groter dan een district. Voss leest hem als een technicus: doorvoer, luchtwissel, afstand tot water. (2) **Waarom Tarsis en niet ergens anders** — bergingslegeringen, en Dex wil ze. Zijn enthousiasme is de eerste keer sinds M1.8 dat iemand ergens zin in heeft. (3) Sela's bezwaar en het is een goed bezwaar: je gaat mensen om iets vragen dat je hun niet kunt teruggeven. (4) **Reyes blijft.** Ze legt uit waarom in volledige klinische zinnen: de gewonden van K-77 zijn nog niet gelopen. Dat is geen sentiment, het is een rooster. (5) Petra deelt twee taken uit en beantwoordt geen enkele vraag. (6) Brick zegt niets over Tarsis en één ding over water.
**Vlaggen:** leest `Story.Beat.Act1Complete` (act-2-pin, muziekstaat). **Zet niets.**
**Registerwaarschuwing:** deze scène speelt op Kessara en mag dus tellen. **Vanaf S02 telt er niemand meer** — zie §9.3.

### S02 — *The Crossing* · walk-and-talk
`Tarsis / Dune Sea / Wreck-Town` · walk-and-talk · tier 5
**Aanwezig:** Voss, Dex, Sela, Brick + twee Eclipse-vechters

- **want** — Voss wants to arrive looking like a partner and not like refugees.
- **obstacle** — They arrived in whatever would fly, wearing Kessara, and the heat has already beaten them.
- **turn** — Sela tells him the thing he is about to do wrong, and he does it anyway, and she was right.

**Beats:** (1) Aankomst. Ochre, hittegolven, een scheepsromp aan de horizon die groter is dan de Underworks. **Eén beeld, geen reisverslag.** (2) De ploeg is verkeerd gekleed en dat is zichtbaar voor iedereen behalve henzelf. (3) **Wie er mee is, hangt af van M1.6:** de tak `run` bracht iedereen mee, `emptied`/`split` niet — één regel, geen scène. (4) Sela's waarschuwing: *hier vraag je niet om hulp, hier bied je aan.* (5) Voss vraagt om hulp.
**Vlaggen:** **leest `Story.Choice.M16_Train.{Run,Emptied,Split}`** — drie takken, één regel elk, verder niets. **Zet niets.**
**Registerwissel:** dit is de scène waarin het register kantelt. Van hier af **claimen en verschuldigd zijn**, nooit meer tellen (`ACT2_OVERVIEW` §7).

### S03 — *Wreck-Town* · eerste contact
`Tarsis / Dune Sea / Wreck-Town` · cutscene · tier 5
**Aanwezig:** Voss, Sela, Brick · een salvage-clanoudste + clanleden

- **want** — Voss wants permission to occupy a hull nobody is using.
- **obstacle** — Under hull-right, "nobody is using it" is not a thing that exists — every wreck is owed to someone.
- **turn** — The elder addresses him by the name the cells gave him, and it turns out his reputation arrived before he did and is worth less than he hoped.

**Beats:** (1) De wreck-town: mensen leven ín een schip, niet ernaast. (2) **De oudste noemt hem "Cinder"** — de eerste keer dat een vreemde dat doet. Hij is niet onder de indruk; hij weet alleen wie er komt. (3) **Hull-right uitgelegd zonder college:** de oudste zegt niet wat de wet is, hij zegt wat hij van wie tegoed heeft, en de speler leidt de wet daaruit af (§18.8). (4) De Dominion is hier geen bezetter maar een **belastinginner** — het salvage-tax-garnizoen. Dat is de eerste keer dat het spel laat zien dat onderdrukking niet overal hetzelfde gezicht heeft. (5) De Cartel wordt genoemd, één keer, zonder uitleg. (6) De oudste noemt zijn voorwaarde niet. Hij zegt dat hij erover nadenkt, en dat is erger.
**Vlaggen:** **leest `Story.Char.CinderNamed`** (M1.8.S99) en **`Story.Char.PetraRescued`** (één regel — de oudste weet dat er een gevangenis is opengebroken; dat is wat er van Kessara is overgewaaid). **Zet niets.**

### S04 — *What Is Owed* · de onderhandeling
`Tarsis / Dune Sea / Salvage Yard` · hub · tier 5
**Aanwezig:** Voss, Sela, Dex · de oudste

- **want** — The elder wants to know what Eclipse will still owe when it stops needing Tarsis.
- **obstacle** — Voss has nothing to trade except a future, and the last three people who offered these clans a future were the Dominion, the Cartel, and a war.
- **turn** — He names a debt instead of a price, and the clan takes it — which means Eclipse is now inside a law it does not understand.

**Beats:** (1) De werf: gedemonteerde rompen, een cultuur die uit ontmanteling leeft. (2) **De oudste telt niet, hij claimt.** Elk ding in de scène heeft een eigenaar en een geschiedenis. (3) De speler krijgt het woord: **drie manieren om te betalen**, en alle drie zijn verdedigbaar in één zin (§2.7 regel 2). Materiaal nu, bescherming later, of een schuld die de clan zelf mag invullen. (4) De laatste optie is de gevaarlijkste en de oudste zegt dat hardop, en hij is niet aan het waarschuwen — hij is aan het aanbieden. (5) Dex ziet de vesting en rekent hardop, en het is de eerste keer sinds M1.8 dat hij een grap maakt. (6) De handdruk bestaat niet; er wordt iets *gezegd*, en dat is bindend. Shroud-cultuur maakt van je woord onderpand; Tarsis-cultuur maakt van je schuld je adres.
**Systeem:** `run.m21_hullright_offer` = `materials` | `protection` | `open` — **`run.`, want act 3 leest hem niet en de gevolgen worden binnen deze akte in M2.8 uitbetaald** (L1-R50: *"leest act 3 dit? nee → `run.`"*). Gelezen in **S04 zelf (3)** en **S05 (1)**.
**Sub-draad geopend:** *wat Eclipse verschuldigd is.* **Gesloten in M2.8.S03**, binnen dezelfde akte.
**Dit is de langste dialoogscène van de missie** en dus waar §18.3's variantieregel en §18.9 B het hardst bijten. Zet er een interruptie in. Laat de oudste één vraag ontwijken en er nooit op terugkomen.

### S05 — *The Ashline* · het gevecht begint
`Tarsis / Dune Sea / Salvage Yard` · in-mission-radio + callout · tier 5
**Aanwezig:** Voss, Dex, Brick, Sela · Ashline Cartel-spreker · Cartel-eenheden

- **want** — The Cartel wants Eclipse gone before the clans learn there was a choice.
- **obstacle** — They are not soldiers and they are not fanatics; they are a business with a contract, and they say so.
- **turn** — They fight to be paid, not to win — and the moment it stops paying they leave, which teaches Eclipse something about this planet it will need in M2.9.

**Beats:** (1) De Cartel arriveert vóór het garnizoen. Dat is de tell: zij houden de wacht die de Dominion niet houdt. (2) **De spreker onderhandelt tijdens het vuurgevecht.** Hij dreigt niet en hij scheldt niet; hij noemt bedragen. Dat is een derde vijandvocabulaire naast conscripten en Veil (§18.5 regel 5). (3) De clans doen **niets**. Ze kijken. Dat is hull-right: dit is nog niemands zaak. (4) Brick zegt vier woorden en ze gaan over lucht, niet over de vijand. (5) Zodra het te duur wordt, breekt de Cartel contact — netjes, zonder paniek, met een laatste regel die geen dreigement is.
**Vlaggen:** **leest `run.m21_hullright_offer`** (1 — de Cartel weet wat er is afgesproken en zegt het, en dat is de eerste aanwijzing dat er een lek in de wreck-town zit). **Zet niets.**
**Band:** `in-mission-radio`, met `band: callout` op de gevechtsregels.

### S06 — *Taking the Hull* · de vesting
`Tarsis / Dune Sea / The Carcass` · in-mission-radio · tier 5
**Aanwezig:** Voss, Dex, Brick + vechters · het salvage-tax-garnizoen · de oudste (aan het eind)

- **want** — Voss wants the wreck cleared before the heat comes back at noon.
- **obstacle** — The garrison inside is a tax detachment that has never been attacked and does not know how to be.
- **turn** — It is over far too quickly, and Dex says so.

**Beats:** (1) Binnen in een schip dat honderd jaar geleden op de grond is gevallen: gangen die op hun kant liggen, zwaartekracht die niet klopt met de architectuur. (2) Het garnizoen is klein, slecht en verrast. **Dit hoort onbevredigend te zijn.** (3) Dex' opmerking dat dit geen gevecht was maar een verhuizing. (4) De vesting is groot — te groot voor wat Eclipse is. Eén regel over lege ruimte, en het is een belofte en een dreiging tegelijk. (5) De oudste komt kijken en zegt niets over de doden. Hij zegt iets over het schip.
**Vlaggen:** zet niets. **Voss spreekt hier groepen aan, nooit een individu in de tweede persoon** — AR-A2-2.

### S99 — *Two Doors* · debrief
`Tarsis / Dune Sea / The Carcass` · cutscene · tier 5
**Aanwezig:** Voss, Dex, Sela, Brick · Reyes op de radio vanaf Hollow Point

- **want** — Voss wants to know what they have actually gained.
- **obstacle** — What they have gained is a second address, and nobody in the room has said the word "second" out loud yet.
- **turn** — A recording of Mara's plays, and it does not answer anything.

**Beats:** (1) De opsomming van wat er ligt: bergingslegering, een dok, ruimte. **Geen totaal aantal mensen** — AR-A2-10. (2) **P3-A2-b:** iemand moet de lijsten van twee plekken tegelijk bijhouden. De kwartiermeesterspost uit `M1.4.S99` wordt hier een **post met toegang**, en het klinkt als groeipijn. Eén regel, terloops, en er wordt geen naam genoemd (Q-6 blijft open). (3) **AANRAKING A2-1 — P4-A2-a, en hij is VLAK.** Dex merkt op dat er geen Veil op deze planeet is. Hij verklaart het weg: de Veil is een planetaire dienst, de Veil reist niet. Niemand spreekt hem tegen. **Fragmenten, geen figuur, geen slotbeat, geen citeerbare zin.** Hij is niet gerustgesteld; hij is bezig, en dat is waarom het niet opvalt. (4) Reyes op de radio, kort, met slecht nieuws over één gewonde — de scène gaat door. (5) **De Mara-opname (AR-A2-5, brief 2).** Adres: de eerste nacht weg van Kessara. **Ze beantwoordt de vraag van deze scène niet** en er wordt na afloop niet over gepraat. Er is geen keuze in deze scène en dat is de voorwaarde.
**Systeem:** zet **`Story.Beat.M21_Carcass`** ✚. Zet **`Story.Clue.*` niet** — de aanraking is een aanraking en geen clue; de dragende T4-clue van act 2 valt in M2.6.S03 (L1-R38: clues worden gezet waar ze klinken, en deze regel is een wegverklaring, geen clue).
**Leest:** `Story.Thread.MaraLetters_Open`.

---

## 5. Vlaggen

| In | Uit |
|---|---|
| `Story.Beat.Act1Complete` (S01) · `Story.Choice.M16_Train.{Run,Emptied,Split}` (S02, 3 takken) · `Story.Char.CinderNamed` (S03) · `Story.Char.PetraRescued` (S03, 1) · `Story.Thread.MaraLetters_Open` (S99) | **`Story.Beat.M21_Carcass`** ✚ (S99) |
| `run.m21_hullright_offer` — gezet S04, gelezen S04 (3) + S05 (1) | — |

**Kanonieke spelling en de kolom *gelezen door*: `ACT2_OVERVIEW` §6.** Geen enkele tag hierboven bestaat al in `EclipseGameplayTags.cpp`; **geen `condition:` op `Story.Beat.M21_Carcass` voordat de tag er is.**

---

## 6. Wendingen

| Wending | Handeling |
|---|---|
| **T4** | **geplant, dragend** (S99, P4-A2-a, aanraking **A2-1**): de Veil komt niet mee van Kessara af, en Dex verklaart het weg. **Vlak.** Dit is act 2's eerste aanraking en hij zet de vorm voor de andere vijf |
| **T3** | **geplant, niet-dragend** (S99, P3-A2-b): de kwartiermeesterspost krijgt toegang. Eén regel, terloops, geen naam |
| **T1, T2, T5** | niet aangeraakt. Whisper bestaat nog niet; de Blight ligt op Meridia; er zijn hier geen conscripten en geen burgerdoden — de Cartel is betaald personeel en dat is een ander moreel object |
| **Threx-ladder AR-6** | **geen trede.** Threx komt niet voor. AR-A2-3: hij escaleert in act 2 helemaal niet |

---

## 7. Draden

- **Wat Eclipse verschuldigd is (hull-right)** — **GEOPEND** in S04. **Gesloten in M2.8.S03**, binnen dezelfde akte. Een schuldcultuur waarin nooit iemand komt innen is decor.
- **Mara's brieven** — brief 2 op zijn adres (AR-A2-5). Loopt door tot act 4.
- **The Enforcer** — **niet aangeraakt en dat is een besluit** (AR-A2-7).

---

## 8. Groei

- **Voss** — van iemand die deelt naar iemand die aanbiedt. Hij is er slecht in, en de scène waarin hij het fout doet (S03) is de scène die de missie draagt. **Hij spreekt hier nog geen enkel individu in de tweede persoon aan** (AR-A2-2).
- **Sela** — voor het eerst adviseert zij vóór in plaats van te oordelen ná. Ze heeft gelijk en er wordt niet naar haar geluisterd; onthoud dat voor M2.9.
- **Dex** — de eerste keer sinds Mara's dood dat hij ergens zin in heeft, en het is metaal. Zijn grap in S04 is de eerste van act 2 (§18.6: nul komische beats in nasleep, en dit is geen nasleep meer).
- **Brick** — hij praat op deze planeet over water en lucht. Dat is Krad-9 dat door hem heen praat, en het is de opzet voor M2.2.
- **Reyes** — blijft achter. Dat is een besluit met een rooster erachter en het maakt haar in M2.6 duurder.

---

## 9. Instructies voor de dialogue-writer

1. **Het register van deze planeet is CLAIMEN EN VERSCHULDIGD ZIJN, en het begint bij S02.** Op Kessara telt iedereen (act 1's ruling, en S01 speelt daar nog). Vanaf de duinzee zegt niemand meer *hoeveel*; ze zeggen *van wie* en *waarvoor*. Eén registerbeat per scène draagt de wending, niet meer. Volledige tabel: `ACT2_OVERVIEW` §7.
2. **De clan-oudste en de Cartel-spreker hebben geen naam** tot Q-A2-3 beantwoord is. **Verzin er geen.** Schrijf ze als "de oudste" en "de Cartel". Dat is precies wat er met de Iron Chorus-emissaris in act 1 gebeurde en het bleek een sterkte in plaats van een gat.
3. **De Ashline Cartel is geen fanatieke factie.** Ze zijn betaald. Ze dreigen niet, ze schelden niet, ze noemen bedragen, en ze gaan weg als het te duur wordt. Wie ze als schurken schrijft, verspeelt de moral fork in M2.9.
4. **De hull-right-wet wordt nergens uitgelegd.** §18.8: de speler mag dingen niet begrijpen. De oudste zegt wat hij van wie tegoed heeft; de wet is wat de speler daaruit afleidt. **Een regel die de wet uitlegt, is een codex-regel en zakt op §18.9 B (Explaining).**
5. **AANRAKING A2-1 in S99 is vlak.** Dex' verklaring voor de afwezige Veil mag niet citeerbaar zijn. Zou hij goed op een poster staan, dan herschrijf je hem (L1-R17). Hij is bovendien de **eerste** van zes in de akte, dus hij zet de norm: als deze gepolijst is, worden de volgende vijf het ook.
6. **De Mara-opname in S99 beantwoordt niets** en er wordt niet over nagepraat. Er staat geen keuze in die scène en dat is de voorwaarde (AR-A2-5).
7. **Geen enkel totaal aantal mensen** (AR-A2-10). Waar je de groei wilt laten voelen, doe je dat met ruimte, hitte, geluid en een tekort.
8. **Voss-varianten:** S04 (de onderhandeling) krijgt volledige dekking op beide assen. S03 krijgt alleen de personal/strategic-as. De rest draait op de basisregel — dit is de duurste knop van het spel (Q-7) en M2.1 is niet de missie om hem leeg te trekken.
9. **Getallen:** er is geen dragend getal in deze missie behalve **vijfenvijftig graden** (met eenheid, `03_world_design.md` §3.3 letterlijk). Draai `python Eclipse/Tools/check_spoken_numbers.py` vóór je een getal laat vallen en **schrijf de uitkomst niet in een `note:` als bewering** — verwijs naar de tool.

---

## 10. Barks

**Nieuw: het Ashline-Cartel-vocabulaire.** Vierde faction-vocabulaire naast Eclipse, Dominion-conscripten en de Veil (§18.5 regel 5). Register: **commercieel**. Ze roepen kosten, geen dreigementen — *"that's coming out of the fee"* is de vorm, niet *"you're dead"*. Zes tot twaalf varianten per trigger, en **de scheiding met de Dominion-conscripten moet binnen tien seconden hoorbaar zijn**, precies zoals de Veil in M1.5.S05 dat moest zijn.

**Nieuw: Tarsische omgevingsbarks** (clanleden die kijken en niet meedoen). Deze zijn goedkoop en ze dragen de hele planeet-identiteit: mensen die tijdens een vuurgevecht over eigendom praten.

> **Casting hiervoor bestaat niet** (C-A2-2 / Q-A2-3) en dat is een `voice-director`-afweging binnen tier 1, geen schrijfbeslissing. **Niet oplossen door een bestaande pool te lenen** — L1-R30 is daar expliciet over: een pool-sleutel noemt de factie die de spreker levert, nooit het werk dat hij doet, en lenen resolvet schoon en genereert de verkeerde stem.
