# ACT 2 — THE SPREADING DARK · architectuur, plant/payoff, registers
*Laag L1 (verhaalfundering) + L4 (wereldbeschrijving) | eigenaar: story-architect | 2026-08-02*
*Bindend voor: `dialogue-writer` ×N, `dialogue-critic`, `voice-director`*
*Canon boven dit document: `02_story_bible.md` → `11_missions.md` → `03_world_design.md` → `18_writing_standard.md` → `phase0/SCRIPT_FORMAT.md`*
*Erft: `phase0/beats/ACT1_OVERVIEW.md` (AR-1 t/m AR-12) en `phase0/beats/RULINGS_L1.md` (L1-R1 t/m L1-R56). **Dit document herschrijft geen van beide.***

---

## 0. Wat dit is, en wat het niet is

Dit is het **skelet van act 2**: negen missies, uiteengelegd in scènes met een want/obstacle/turn, met de vlaggen die ze zetten en lezen, en met de tabellen die bijhouden welke wending waar geplant en waar betaald wordt.

**Het bevat geen dialoog.** Geen enkele regel hier is een regel die iemand zegt.

**Het verzint geen canon.** Elke naam komt uit de glossary in `00_INDEX.md` of uit `03_world_design.md` §3.3. Waar act 2 iets nodig had dat niet bestaat, staat dat in §9 als **owner-vraag**. Waar de canon een gat had dat ik met bestaande canon kon dichten, staat dat in §8 als **architect-ruling**, met de redenering erbij, zodat de owner hem kan omkeren.

**Waar de rulings staan, en waarom ze niet in `RULINGS_L1.md` staan.** Act 1 draagt twee sporen: `ACT1_OVERVIEW.md` §8 houdt de architect-rulings van die akte (AR-1…AR-12), `RULINGS_L1.md` houdt het log van escalatie-uitspraken (L1-R…). Act 2 volgt exact diezelfde splitsing: **§8 hieronder houdt AR-A2-1…**, genummerd in een eigen naamruimte zodat hij nooit met AR-1…AR-12 kan botsen. Uitspraken die het L1-log, `SCRIPT_FORMAT`, `18_writing_standard.md` of de validator raken staan **niet** in dit bestand — die zijn aangevraagd en landen serieel, zodat er nooit twee schrijvers in één log zitten. Zie §9, tabel *Aangevraagde L1-rulings*.

**Volgorde van gezag bij conflict:** `02_story_bible.md` wint van alles. Wint de bijbel niet duidelijk, dan beslist story-architect en noteert het hier. Raakt de beslissing de bijbel zélf, dan is het een owner-beslissing en staat het in §9.

### Wat ik NIET heb kunnen meten, en dat hoort op de statuskaart

**Deze ronde had geen shell.** `python Eclipse/Tools/validate_script.py --no-voice` en `python Eclipse/Tools/check_spoken_numbers.py` zijn **niet gedraaid**. Wat ik in plaats daarvan heb gedaan: beide tools gelezen en de structuur van act 2 zo gelegd dat ze bij de eerste draai iets kunnen zeggen in plaats van niets — zie §9 bevindingen **C-A2-4** en **C-A2-5**, die allebei gaan over checks die op act 2 per constructie stil blijven. Elke telling in dit document die ik niet zelf kon meten is als zodanig gemarkeerd. Er staat geen enkel getal in dit document dat een tool had moeten leveren en dat ik met de hand heb geschat.

---

## 1. De dramatische functie van act 2

De bijbel geeft act 2 één opdracht (§2.9): *"Building power, becoming a threat"* — en drie vaste eindpunten: **de eerste volledige planetaire bevrijding (Tarsis)**, **Kaine neemt persoonlijk het commando**, en **de Dominion-propaganda noemt "Cinder" bij naam.**

Act 1's motor was: *elke overwinning maakt de cel zichtbaarder, en zichtbaarheid is wat Mara doodt.* Die motor is op. Voss is zichtbaar; dat is niet meer de vraag. De motor van act 2 is de volgende stap in dezelfde beweging:

> **Elke overwinning maakt de beweging een PLAATS. En een plaats kun je aanvallen.**

Dat is de spine, en hij is om vier redenen de goedkoopste die act 2 kan draaien:

1. **Hij maakt de Siege of Hollow Point een gevolg in plaats van een gebeurtenis.** `05_base_building.md` §5.2 zegt het al met zoveel woorden: *"the Siege of Hollow Point makes the reason for moving playable."* De basis valt niet omdat het script het zegt. Hij valt omdat de speler hem heeft opgebouwd tot iets dat een adres heeft. Dezelfde vorm als Mara's dood, één laag hoger: de speler wint zich in de val.
2. **Hij levert de bijbel zijn eigen relocatie.** §5.2 laat de HQ in act 2 verhuizen naar de Tarsis-wrakvesting **The Carcass**. Als M2.1 die vesting vestigt en M2.6 Hollow Point onbruikbaar maakt, is de verhuizing geen menu-optie maar de derde akte van dit verhaal. De bijbel legt het eindpunt vast; deze spine is de enige lezing waarin het eindpunt een oorzaak heeft.
3. **Hij draagt twist 4 zonder hem af te maken.** AEGIS liet de opstand toe als drukventiel. Act 1 liet dat zien als *"de Dominion reageert te dun"*. Act 2 mag dat niet herhalen — dan wordt de vorm de tell (L1-R17). Dus verandert de anomalie van soort: **niet meer "ze zijn te dun", maar "de verkeerde dienst doet het werk".** De Veil komt niet mee van Kessara af; de Armada wel, en pas laat. Zie §4, plantengroep P4-A2.
4. **Hij geeft twist 2 een reden om nu te vallen.** Een inlichtingenmakelaar is pas iets waard voor iemand die op meer dan één planeet iets te verliezen heeft. Whisper wordt gecontacteerd op het moment dat Eclipse voor het eerst niet meer alles zelf kan zien.

### De vier bewegingen

| Beweging | Missies | Wat er verandert |
|---|---|---|
| **I — Grond kopen** | M2.1, M2.2 | Eclipse verlaat Kessara. Op Tarsis moet Voss grond *kopen* in plaats van delen; op Krad-9 moet hij een staking winnen die niet van hem is. Twee culturen die anders praten dan de zijne. |
| **II — Bereik kopen** | M2.3, M2.4, M2.5 | Kaya, de *Loyal Ghost*, Torren, en de eerste stem die niet van Eclipse is en toch voor Eclipse werkt. De beweging krijgt een lane-net en een blinde vlek. |
| **III — Betalen** | M2.6, M2.7 | Hollow Point valt. Wie Whisper is, wordt de duurste vraag van de akte. Eclipse verhuist naar The Carcass. |
| **IV — Een planeet** | M2.8, M2.9 | Tarsis komt overeind. Kaine neemt het commando. De Dominion spreekt de naam "Cinder" hardop uit — en op datzelfde moment breekt het model dat de speler nooit gezien heeft. |

### Het einde van de act (canon, §2.9)
De eerste volledige **planetaire bevrijding** (Tarsis) · het bevrijdingssjabloon uit `11_missions.md` §11.2 draait voor het eerst authored · de Expanse merkt het · **Kaine neemt persoonlijk het commando** · de Dominion bestempelt Eclipse als *state-level enemy* en **de propaganda noemt "Cinder".**

---

## 2. De cast van act 2 — wie spreekt, wie niet

Harde lijst. Een schrijver die hier iemand aan toevoegt, escaleert eerst.

### Spreekt in act 2

| Personage | Vanaf | Rol in de act | Fingerprint |
|---|---|---|---|
| **VOSS / "CINDER"** | M2.1 | de speler; commandant sinds M1.8.S99 | §18.4 — techniekvocabulaire; claimt nooit krediet voor andermans werk. **Nieuw in act 2: hij erft de grammatica van het bevel. Zie AR-A2-2.** |
| **DEX** Callum | M2.1 | ingenieur; de basis is nu zijn levenswerk, en die valt | fragmenten; technische zelfstandige naamwoorden als scheldwoord |
| **REYES**, Dr. Elin | M2.1 | medic; in act 2 voor het eerst met een infirmerie die vol kan lopen | volledige klinische zinnen; laat samentrekkingen vallen onder stress (nullijn ligt in M1.4.S99) |
| **BRICK** (Oram Bex) | M2.1 | zwaar wapen, moreel anker. **M2.2 speelt op zijn thuiswereld.** | minste woorden van iedereen; antwoordt met een dode naam |
| **SELA** Vann | M2.1 | organisator; in act 2 begint haar politieke arc echt | retorische structuur, ook privé; tweede persoon meervoud. **§18.9 B maximes-cap geldt ook voor haar (L1-R56)** |
| **PETRA** Voss | M2.1 | het stille hart van de basis; **kan sterven in M2.6** (§2.11) | §18.4 *Recurring non-companion voices* — imperatieven zonder voornaamwoord; noemt de hoofdpersoon nergens iets |
| **KAYA** Renn | **M2.3** | smokkelaarspiloot; bindt vloot en lanes | run-ons, onderbreekt zichzelf; ondermijnt elke serieuze zin. **Haar nullijn ligt in M2.3 — AR-A2-4** |
| **TORREN** Vale | **M2.4** | ex-Armada-kolonel; bindt het militaire systeem | militaire economie; bevelen als suggesties; de langste stiltes van het spel. **Zijn nullijn ligt in M2.5 — AR-A2-4** |
| **WHISPER** | **M2.5** | inlichtingenmakelaar; **stem-only tot act 3** (§2.5) | geen samentrekkingen; conditionalis; passief; *"it is known"*; **gebruikt nooit namen, zegt nooit "I"** |
| **THREX**, Inquisitor Dahl | M2.6 | de jager die het persoonlijke duel verloor en het institutioneel maakt | warm, intiem, vraag na vraag; voornamen als wapen. **AR-A2-3 begrenst hem hard** |
| **KAINE**, Grand Marshal Sera | **M2.9, en niet eerder** | de Armada krijgt een gezicht | precies, eerlijk, geen eufemisme; **noemt haar voornemen vóór ze het uitvoert**; liegt nooit; gebruikt nooit Veil-taal |
| **AEGIS** | M2.5, M2.9 (omroep) | publieke aankondigingen, geen gesprek | tegenwoordige tijd; noemt kansen waar mensen meningen noemen |
| **MARA** Sovann | M2.1, M2.6, M2.7 — **uitsluitend als opname** | de brieven (§2.5) | korte declaratieven; "wij". **Zie AR-A2-5: drie brieven, drie adressen, en geen enkele beantwoordt de vraag van zijn scène** |
| Iron Chorus-emissaris | M2.6 | de pact-betaling | §18.4 — telt waar een ander een bijvoeglijk naamwoord pakt; **gebruikt nooit een naam**. Naam/casting nog steeds open (Q-4) |
| Ashline Cartel-spreker | M2.1, M2.9 | de collaborateurs van Tarsis (`03_world_design.md` §3.3) | **fingerprint en casting ontbreken — C-A2-2 / Q-A2-3** |
| Salvage-clanoudste (Tarsis) | M2.1, M2.8 | hull-right; schuld-en-eer | **fingerprint en casting ontbreken — C-A2-2 / Q-A2-3** |
| Krad-9-stakingsleider | M2.2 | de staking die niet van Voss is | **fingerprint en casting ontbreken — C-A2-2 / Q-A2-3** |
| Rolsprekers | overal | `FIGHTER_`, `CONSCRIPT_`, `VEIL_`, `CIVILIAN_`, `OFFICER_`, `PRISONER_`, `CHORUS_` + `EMISSARY` | §18.5 / L1-R6 / L1-R6b / L1-R30 |

### Spreekt NIET in act 2 — AR-A2-1, hard

**MARA** live (zij is dood; `Story.Char.MaraDead` — alleen opnames, en alleen op de drie adressen van AR-A2-5) · **VEX** (blijft opname/omroep tot act 3–4; §2.6 maakt hem de eindantagonist en zijn eerste gesprek is de finale) · **CALLIS** (systemische antagonist; act 3) · **KAINE vóór M2.9** (§2.9: zij neemt *aan het eind* van act 2 het commando — één regel eerder en de beat is weg).

**Whisper spreekt alleen via de relay/comms, nooit in beeld** (§2.5: *"voice only until Act 3"*). Wie deze regel breekt, verbrandt de act-3-onthulling.

### Wat er van Ember over is
Act 1 eindigt met *"~15 soldiers and the loyalty of three fused cells"* (§2.9). **Act 2 spreekt geen totaal uit.** Zie §7, nummerregister, rij *"hoeveel zijn we"* — dat is een ruling, geen stijladvies.

---

## 3. De negen missies in één tabel

**Waarom negen, en waar dat staat.** `02_story_bible.md` §2.9 schrijft *"Main missions (M2.1–M2.9)"* en `11_missions.md` §11.1 telt 34 verhaalmissies als **8+9+10+7**. Act 2 levert er dus **negen**, en dat is expliciet vastgelegd, geen aanname.

**Waarom déze negen.** §2.9 noemt zeven beats bij naam (Tarsis-bolwerk; Krad-9-staking; Kaya's smokkelnet; *Loyal Ghost*; de Meridia-relay-heist; de Siege of Hollow Point; de Whisper-identiteitsclimax) en zet er één eindpunt achter (*"first full planetary liberation (Tarsis)"*). Dat is acht. De negende volgt uit `11_missions.md` §11.2: het bevrijdingssjabloon is **drie verplichte fasemissies** — Foothold → Momentum → Capital Push — en §2.9 zegt dat het sjabloon hier *"runs authored for the first time"*. M2.1 is de Foothold; de bevrijding kost dus nog twee missies, niet één. **Negen, exact, zonder er een bij te verzinnen of een weg te laten.**

| # | Werktitel | Type (§11.2) | Planeet | Dramatische functie | Beat-vlag |
|---|---|---|---|---|---|
| **M2.1** | *The Carcass* | Foothold (liberation-template fase 1) | Tarsis | Eclipse koopt voor het eerst grond die niet van hem is, in een cultuur die niet deelt maar *verschuldigt* | `Story.Beat.M21_Carcass` ✚ |
| **M2.2** | *The Strike* | Strike protection / uprising — **morele vork** | Krad-9 | een overwinning die niet van Voss is, en de eerste keer dat hij iemand anders' beweging kan verpesten door te winnen | `Story.Beat.M22_Strike` ✚ |
| **M2.3** | *Nym* | Negotiation summit / reputation intrigue | The Shroud | bereik kopen van mensen wier woord onderpand is; Kaya | `Story.Beat.M23_Nym` ✚ |
| **M2.4** | *Loyal Ghost* | Heist (eerste ruimte-asset) | The Shroud | het eerste schip — en de man die weet hoe een Armada-schip vanbinnen praat | `Story.Beat.M24_LoyalGhost` ✚ |
| **M2.5** | *The Relay* | Wire-tap / infiltration | Meridia | de eerste stem die niet van Eclipse is en toch voor Eclipse werkt | `Story.Beat.M25_Relay` ✚ |
| **M2.6** | *The Siege of Hollow Point* | Base Defense — **act-scharnier** | Kessara | de basis valt omdat hij een basis werd; Petra's leven staat op het spel | `Story.Beat.M26_Siege` ✚ |
| **M2.7** | *The Custodian* | Extraction / dialoogclimax | The Shroud → The Carcass | **twist 2 betaalt**: Whisper is Ilan Vex | `Story.Beat.M27_Custodian` ✚ |
| **M2.8** | *Hull-Right* | Momentum-fase (liberation-template fase 2) | Tarsis | de clans kiezen partij, en de rekening van M2.1 wordt gepresenteerd | `Story.Beat.M28_HullRight` ✚ |
| **M2.9** | *The Sea of Rust* | Capital Push / hybrid battle (fase 3) | Tarsis | een planeet komt vrij; Kaine neemt het commando; de Dominion spreekt "Cinder" uit | `Story.Beat.M29_SeaOfRust` ✚ · `Story.Beat.Act2Complete` ✚ |

**De werktitels zijn werktitels.** `02_story_bible.md` geeft act-1-missies canonieke namen en act 2 geen enkele. Vier van de negen zijn woordelijk canon (*The Carcass*, *Loyal Ghost*, *The Siege of Hollow Point*, *The Sea of Rust* als epitheton van Tarsis) en *Hull-Right* is de canonieke naam van het Tarsische bergingsrecht; de andere vier zijn beschrijvend en bewust saai. **Owner-vraag Q-A2-1.** Ze zitten in geen enkele `text:` en zijn dus gratis te wijzigen zolang ze niet uitgesproken worden — dat is de reden dat ze zo gekozen zijn.

**42-telling.** 34 verhaalmissies (8+9+10+7) + 8 loyaliteitsmissies. **Act 2 levert er negen van de 34.** Loopstand na act 2: 8 (act 1) + 9 = **17 van de 42**. Welke loyaliteitsmissies in act 2 vallen is **niet vastgelegd** door de canon en is een owner-vraag (**Q-A2-2**) — zie ook AR-A2-6 over `LOY.kaya`.

---

## 4. Plant/payoff-tabel — de tabel die moet sluiten

De regel is act 1's regel en verandert niet: **elke wending is geplant vóór hij betaald wordt**, en **elke plant heeft een verplichte gesproken drager** (AR-9). Documenten en optionele objectives verdiepen; ze dragen nooit alleen.

Act 2 doet drie dingen tegelijk en de tabel is per soort gesplitst:
**(a)** het **betaalt** wat act 1 heeft geplant (T2, en de act-1-draden),
**(b)** het **plant** wat act 3 en 4 betalen (T1, T3, T5),
**(c)** het **trekt de T4-curve door zonder hem af te maken.**

### T2 — Whisper is Ilan Vex · **BETAALD IN ACT 2 (M2.7)**

Dit is de wending die act 2 moet leveren, en act 1 heeft er vier plants voor achtergelaten. **Alle vier worden hier ingelost, en dat is de eerste keer dat een act-1-plant-groep sluit.**

| Act-1-plant | Waar geplant | Vlag | Waar act 2 hem betaalt | Hoe |
|---|---|---|---|---|
| **P2-a** — Veil-staande order: het terugkrijgen van een gestolen **custodian-key-token** staat boven celbestrijding | `M1.2.S04` (`.120` spreekt hem uit) | `Story.Clue.CustodianKey` | **M2.7.S05** — verplicht, gesproken | De bijbel zegt dat Ilan Vex *"fled with root-access fragments"* (§2.8 twist 2). Het token dat de Veil een heel kwartaal terugwilde **is zo'n fragment.** De speler heeft het bewijs van zijn identiteit al zestig uur in zijn logboek en heeft het gelezen als politiebureaucratie. |
| **P2-b** — in het luisterpostlog liep al eerder een query op Embers sector, **niet van de Veil** | `M1.7.S03` | `Story.Clue.OutsideQuery` | **M2.5.S04** — verplicht, gesproken | De tweede jager is de man aan de lijn. Het angstaanjagendste feit van act 1 wordt de bondgenoot van act 2, en dat is precies waarom de speler hem niet vertrouwt. |
| **P2-c** — Mara legt nooit uit waar de tip vandaan kwam: *"een vriend van een vriend op de lanes"* | `M1.7.S01` + `M1.7.S99` | — | **M2.7.S02, in een Mara-opname (AR-A2-5, brief 4)** | **Mara's bron was Whisper, en zij wist zelf niet wie hij was.** Dat is de enige betaling waarin die regel een plant is in plaats van mentor-geheimzinnigheid. En het kost: het antwoord komt in de stem van de vrouw die er niet meer is, en het is geen antwoord. |
| **P2-d** — Dominion-personeelsrecord met een weggelakte, ontpersoonde naam in het **huishouden van de Arbiter** | `M1.7.S03`, optioneel (`run.m17_record`) | — | **NIET GEADRESSEERD — en dat is een gevonden defect, geen keuze. Zie de waarschuwing hieronder** | — |

> **P2-d KAN IN ACT 2 NIET GELEZEN WORDEN, EN DAT IS EEN FOUT DIE IK IN MIJN EIGEN TABEL HEB GEVONDEN.** De eerste versie van deze rij zette de betaling op `M2.7.S05` *"als optionele verdieping, gegate op `run.m17_record`"*. **Dat kan niet.** L1-R4 en `SCRIPT_FORMAT` §4: `run.`-feiten leven in `FEclipseMissionOutcome` en **overleven de missie niet**. Een act-2-scène die op `run.m17_record` conditioneert, hangt aan een feit dat op dat moment niet bestaat — en `validate_script.py`'s `CONDITION`-controle zou hem waarschijnlijk stil laten passeren, want de fact wordt in `SCRIPT_FORMAT` §4 gedeclareerd en telt dus als "iets zet hem".
>
> **Drie uitwegen, en twee ervan zijn niet van mij:**
> **(a)** act 1 krijgt alsnog een `Story.Clue.*` op die vondst — **dat is een wijziging aan een act-1-bestand en aan `ACT1_OVERVIEW` §6, en die raak ik deze ronde niet aan;**
> **(b)** de regel speelt in M2.7 **onvoorwaardelijk**, voor iedereen — dan is het geen verdieping meer maar gewoon een regel, en dat mag, want AR-9 zegt dat P2-b en P2-a de wending al dragen;
> **(c)** de regel vervalt. Kost niets: P2-d heeft nooit iets gedragen.
>
> **Mijn advies is (c) boven (b) boven (a)**, en de reden is prijs: (a) heropent een gegenereerde akte voor één optionele regel. **Ik heb (b) noch (c) doorgevoerd — er staat geen P2-d-regel in `BEATS_M2.7`, en dat blijft zo tot dit beslist is.** Aangevraagd als **RQ-6**.

> **De keuze zelf is canon en heeft twee bladeren, geen drie.** §2.8: *"Trust him and gain unmatched intel access (and Act 4's AEGIS options); refuse and he leaves (harder endgame, but immunity to twist #4)."* → `Story.Choice.M27_Whisper.{Trusted,Refused}`. **Dit is de vlag met de verste lezer in het spel** (act 4's AEGIS-beslissing en de *free*-optie van §2.10). Zie het vlaggenregister, §6.

### T1 — De Blight was gemaakt *(betaald: act 3 midpoint)*

Act 1 plantte de stem (P1-a: Vex' opgenomen herdenkingstoespraak) en de mening (P1-b: Sela). **Act 2 plant de plaats.** `03_world_design.md` §3.3: *"The Blight began here; memorial politics run deep."* Een act-3-onthulling die op een document landt in plaats van op mensen, is de goedkoopste versie van de beste wending van het spel.

| ID | Plant | Waar | Drager | Hoe het in act 2 leest |
|---|---|---|---|---|
| **P1-A2-a** | De Blight-gedenkkust is een politiek instrument: de handelshuizen gebruiken hun doden als hefboom tegen de Bursary-rantsoenering | **M2.5.S02** | **verplicht** (het is de sociale-stealth-route naar de relay) | cynische lokale politiek |
| **P1-A2-b** | De relay-heist levert onderweg een Bursary-grootboek waarin de Blight-noodhulp een **toewijzingskolom** is, niet een ramp | **M2.5.S04** | **verplicht** (gesproken, één regel, door Kaya) | boekhouding. Dezelfde herkenningsvorm als `M1.4.S04.220`: geef de speler het woord voordat het iets betekent |
| **P1-A2-c** | Whisper's eerste contact gaat over Meridia en hij weet er **te veel** van | **M2.5.S05** | **verplicht** | een makelaar die zijn waar aanprijst |

### T3 — De Vale-kwestie / de echte mol *(betaald: act 3)*

Canon (§2.8): de Veil voert de coalitie vervalst bewijs dat **Torren Vale** Kaine's mol is; de echte mol is *"a minor quartermaster NPC recruited in Act 1"*. Act 2 heeft drie verplichtingen en geen ervan mag de mol aanwijzen.

| ID | Plant | Waar | Drager | Hoe het in act 2 leest |
|---|---|---|---|---|
| **P3-A2-a** | **Torren komt binnen** — er is iemand om te beschuldigen, en zijn binnenkomst is van begin af aan omstreden | **M2.4.S05** + **M2.5.S01** | **verplicht** (`Story.Char.TorrenJoined`) | een aanwinst met een prijs |
| **P3-A2-b** | De kwartiermeesterspost uit `M1.4.S99` wordt in act 2 een **echte post met toegang**: de basis groeit naar tier 2–3 en iemand houdt de lijsten bij | **M2.1.S99** + **M2.6.S01** | **verplicht** (twee keer, allebei terloops) | logistieke groei |
| **P3-A2-c** | De Siege komt bij een basis waarvan het adres alleen intern bekend was — en **niemand vraagt wie het verteld heeft**, omdat het antwoord voor de hand ligt en fout is | **M2.6.S02** + **M2.6.S99** | **verplicht** (de niet-gestelde vraag is de plant) | *"we waren te groot geworden om te verstoppen"* |
| **P3-A2-d** | Threx werkt in act 2 niet meer aan Voss maar aan **de mensen om Voss heen** — informantennetten, geen jacht | **M2.6.S05** | **verplicht** (één keer, en hij spreekt niet tegen Voss) | een verslagen jager die van tactiek verandert |

> **P3-A2-c is met opzet de omkering van P3-d en niet de herhaling ervan.** Act 1 eindigde met drie hardop uitgesproken verklaringen voor het K-77-verraad die geen van drieën klopten (`M1.8.S99`). Diezelfde vorm nog een keer draaien is §18.9 D's laatste bullet — dezelfde constructie draagt drie opeenvolgende beats — één akte hoger. **Dus: bij de Siege stelt niemand de vraag.** De speler stelt hem misschien wel, en dat is het hele punt.
>
> **Continuïteitswaarschuwing voor act 3, en die is hier geregistreerd zodat hij niet zoekraakt:** wie de mol in act 3 aanwijst, moet naar `M1.6.S06` (de ongescreende instroom, `Story.Flag.IntakeUnvetted`) **en** naar P3-A2-b kunnen terugwijzen. Act 2 sluit die deur niet en mag hem niet dichtdoen.

### T4 — AEGIS voorspelde de opstand en liet hem toe *(betaald: act 4)*

Act 1 plaatste de aanrakingen als *"de Dominion reageert te dun, en iemand verklaart dat weg"*. Act 2 mag dat **niet herhalen** — vier keer dezelfde vorm en de speler leert de vorm in plaats van het feit (L1-R17). Daarom verandert de anomalie van soort:

> **Act 1: ze zijn te dun. Act 2: de verkéérde dienst doet het werk.**
> De Veil volgt Eclipse niet van Kessara af. De Armada wel — en pas laat, en in één keer.

Dat is retroactief exact wat twist 4 nodig heeft: §2.8 zegt dat de speler zijn vroege overleven te danken had aan **AEGIS die Veil-middelen afknijpt**. De Armada is niet afgeknepen. Zodra de Armada het overneemt, houdt de bescherming op — en dat gebeurt op de plek waar de bijbel het zelf zet: *"The rebellion outgrew the model at 'hour 60' — the moment (retroactively shown) matching the player's first planetary liberation."* **Dat is M2.9.**

| ID | Plant | Waar | Drager | Verklaring in act 2 | Waarheid in act 4 |
|---|---|---|---|---|---|
| **P4-A2-a** | De Veil komt niet mee van Kessara af. Op Tarsis, Krad-9 en The Shroud is er geen Veil, alleen lokale beveiliging | **M2.1.S99** | **verplicht** (Torren nog niet aanwezig — dus Dex, en vlak) | *"de Veil is een planetaire dienst"* | de Veil krijgt de middelen niet; AEGIS deelt ze niet uit |
| **P4-A2-b** | De ertshart van de Expanse wordt bewaakt door bedrijfsbeveiliging, niet door de Armada | **M2.2.S03** | **verplicht** | *"niemand verwacht dat een staking een oorlog wordt"* | idem |
| **P4-A2-c** | De Meridia-relay is onderbewaakt voor wat er doorheen gaat | **M2.5.S03** | **verplicht** — **en dit is act 2's ENIGE gepolijste aanraking, en hij is van Kaya** | *"geld bewaakt geld, geen draden"* | idem |
| **P4-A2-d** | De Siege komt met de **Armada** en niet met de Veil, en hij komt laat en in één keer | **M2.6.S03** | **verplicht** (Torren, vlak, professioneel, en hij heeft ongelijk) | *"Kaine wachtte tot ze het één keer kon doen"* | **de dragende.** De Veil vroeg en kreeg niet; de Armada vroeg niet en kreeg. → `Story.Clue.ArmadaAllocation` |
| **P4-A2-e** | Bij de bevrijding **verandert de reactie van karakter** — sneller, zwaarder, en zonder de zuinigheid van twee akten | **M2.9.S07** | **verplicht** | *"we hebben ze eindelijk bang gemaakt"* — **half geslikt, en Torren maakt de zin niet af** | **uur 60.** Het model brak; dit is de enige keer dat de speler de breuk *ziet* zonder hem te kunnen benoemen |

#### De WEGVERKLARINGEN van act 2 — de aanrakingsinventaris

**De definitie is act 1's definitie en verandert niet** (`ACT1_OVERVIEW` §4). Drie delen, alle drie nodig: **(1)** er schiet iets tekort dat de speler kan opvallen; **(2)** een personage verklaart het weg in een gesproken, verplichte regel; **(3)** niemand spreekt het tegen. Niet-tellend: de Dominion of AEGIS die zichzelf beschrijft, een document of optionele terminal, en een regel die de anomalie *benoemt zonder te verklaren*.

**De tabel is niet T4-gescoped** — L1-R52 heeft dat afgeschaft en die ruling erf ik. Een aanraking telt hier, ongeacht welke wending hij bedient.

**De curve van act 2 is een andere curve dan die van act 1, en dat is het belangrijkste dat op deze pagina staat.**

> Act 1's curve was **Mara's verklaringen die opraken**: gepolijst → zeker → de eerste barst → een letterlijke herhaling → twee onafgemaakte redenen → stilte. Dat was een karakterfeit, geen auteursregel.
>
> **Act 2's curve is de omgekeerde, en de mond is Torren.** Zijn verklaringen raken nooit op. Hij is een beroepsofficier: hij heeft altijd een correcte, controleerbare, professioneel klinkende reden, en hij zegt hem zonder één keer te aarzelen. **Dat is wat hem in act 3 aan de galg brengt** (twist 3: de Veil voert bewijs dat híj de mol is) — een man die alles kan verklaren is een man wiens verklaringen niets meer bewijzen. De speler heeft dat een hele akte lang als competentie gehoord.
>
> **Eén enkele barst, en hij zit op het laatste adres:** `M2.9.S07`, waar Torren de zin niet afmaakt. Dat is de rijm met `M1.4.S99.040` en `M1.8.S99.080` (Dex' wáre verklaring twee keer midden in een woord afgekapt) — **hier is het de ónware verklaring die afbreekt, in een andere mond.** Niet opruimen, en niet uitleggen.

| # | Aanraking | Adres | Wending | Plant | Vorm — **en dit is een EIS, geen observatie** |
|---|---|---|---|---|---|
| **A2-1** | DEX over de afwezige Veil | `M2.1.S99` | T4 | P4-A2-a | **vlak.** Fragmenten, geen figuur, geen slotbeat. Hij is niet gerustgesteld, hij is bezig |
| **A2-2** | De stakingsleider over de bedrijfsbeveiliging | `M2.2.S03` | T4 | P4-A2-b | **vlak.** Uit de mond van een buitenstaander (de M1.5.S05-vorm: een derde partij maakt het feit hard zonder er een mysterie van te maken) |
| **A2-3** | KAYA over de onderbewaakte relay | `M2.5.S03` | T4 | P4-A2-c | **gepolijst — en dit is de ENIGE gepolijste van act 2.** Eén per akte, en het is de mond van iemand die van gevatheid haar beroep heeft gemaakt. Dat is karakter, niet auteurschap |
| **A2-4** | TORREN over waarom Kaine wachtte | `M2.6.S03` | T4 | P4-A2-d | **vlak en volledig.** Zijn langste regel in de scène en zijn zekerste. Dit is de top van zijn curve |
| **A2-5** | TORREN over de veranderde reactie, en hij maakt hem niet af | `M2.9.S07` | T4 | P4-A2-e | **vlak en afgebroken.** De enige barst van de akte |
| **A2-6** | Een salvage-clanoudste over waarom de Dominion Tarsis nooit echt bezet heeft | `M2.8.S02` | T4 | P4-A2-a (echo) | **vlak.** Eén regel, geen tweede. Een lokale die het al veertig jaar zo ziet |

**Zes aanrakingen over negen missies. Eén gepolijst.** Wie een zevende schrijft, zet hem in deze tabel of schrijft hem niet — act 1's regel, ongewijzigd. **Maximaal één aanraking per missie, en nooit twee in één scène.**

### T5 — Kaine's geweten *(betaald: act 4, voorwaardelijk)*

De telling loopt door: burgerslachtoffers laag **én** conscripten gespaard (§2.8 twist 5). Act 2 voegt er drie beslissingen aan toe, en één ervan is de zwaarste van de akte omdat hij niet over vijanden gaat.

| ID | Plant | Waar | Drager |
|---|---|---|---|
| **P5-A2-a** | De Krad-9-vork: de staking **winnen** (langzaam, houdbaar) of **bewapenen** (snel, bloediger, Dominion-represailles) — `03_world_design.md` §3.3 | **M2.2.S05** | **verplicht** (keuze) |
| **P5-A2-b** | De evacuatie van Hollow Point: wat gaat er mee als er niet alles mee kan | **M2.6.S07** | **verplicht** (keuze) |
| **P5-A2-c** | De Ashline Cartel bij de bevrijding: breken, kopen of sparen — collaborateurs zijn geen soldaten | **M2.9.S06** | **verplicht** (keuze) |
| **P5-A2-d** | Bij de Siege wordt voor het eerst een **Armada**-conscript gevangengenomen in plaats van een Veil-operative. Het is niet dezelfde soort mens | **M2.6.S08** | **verplicht** (barks + één regel van Brick) |

> **Waarom Brick P5-A2-d draagt en niet Reyes.** Reyes heeft de conscripten-arc al twee keer gedragen (M1.1.S05, M1.6.S06) en §18.9 D waarschuwt precies voor een cast waarin elk thema altijd dezelfde mond krijgt. Brick komt uit Krad-9, waar de cultuur *"you breathe what your crew breathes"* is (`03_world_design.md` §3.3) — een man voor wie een bemanning een bemanning is, ongeacht welk uniform. Hij zegt er drie woorden over. Dat is genoeg.

### Wat act 2 betaalt in plaats van plant

| Betaling | Geplant in | Waar betaald |
|---|---|---|
| De custodian-key-order | `M1.2.S04` | **M2.7.S05** |
| De query die niet van de Veil was | `M1.7.S03` | **M2.5.S04** |
| Mara's onverklaarde bron | `M1.7.S01`/`.S99` | **M2.7.S02** (opname) |
| Het pact met de Iron Chorus | `M1.5.S99` | **M2.6.S04** — drie takken, en de tak die niets speelt krijgt een `silence:` |
| Petra's redding | `M1.8.S04` | **M2.6** — zij kan hier sterven (§2.11). De redding krijgt gewicht doordat hij verloren kan gaan |
| De naam "Cinder", door de cellen gegeven | `M1.8.S99` | **M2.9.S08** — de Dominion spreekt hem uit door een luidspreker. Hetzelfde woord, andere mond, act uit elkaar |
| De muur (`Story.Thread.WallOpen`) | `M1.8.S90` | **M2.6.S99** — de doden van de Siege; en de muur verhuist mee naar The Carcass |

---

## 5. Dradenregister (§2.11) — status na act 2

| Draad | Geopend | Aangeraakt in act 2 | Gesloten | Wees? |
|---|---|---|---|---|
| **Letters from the Wall** | `M1.8.S90` | **M2.6.S99** (de Siege-doden) + de verhuizing van de muur naar The Carcass in M2.7 | act 4 | nee — loopt door, bewust |
| **The Enforcer** | proloog kaart 3 (afhankelijk van Q-2) | **niet aangeraakt, en dat is een besluit** — zie AR-A2-7 | act 3/4, persoonlijke keuze | **nee, mits AR-A2-7 gelezen wordt.** Act 2 speelt bijna niet op Kessara; hem daar toch inbrengen zou een reis kosten die de act niet heeft |
| **Petra Voss** | proloog | **M2.6 — hier kan ze sterven** (§2.11, woordelijk). M2.1/M2.7: het stille hart van een basis die verhuist | act 3–4 (of hier) | nee |
| **Iron Chorus** | `M1.5` | **M2.6.S04** — het pact betaalt; fusie, schisma of niets | act 3–4 | nee |
| **The Conscript Letters** | `M1.6.S06` | **M2.2.S99** — de eerste brieven komen terug, en ze komen uit Krad-9 en Vorn | act 4 muiterijpad | nee |
| **Mara's brieven** (§2.5) | `HUB.A1.mara_letters` / `M1.8.S91` | **drie opnames, drie adressen — AR-A2-5** | act 4 | nee |

**Sub-draden die ik in act 2 open, en dus in act 2 sluit:**

| Sub-draad | Geopend | Gesloten |
|---|---|---|
| Wat Eclipse aan de salvage-clans verschuldigd is (hull-right) | `M2.1.S04` | **M2.8.S03** — de rekening wordt gepresenteerd binnen dezelfde akte. Een schuldcultuur waarin nooit iemand komt innen is decor |
| Wat de Shroud-syndicaten aan Eclipse verkocht hebben | `M2.3.S04` | **M2.4.S02** — het contract wordt in de eerstvolgende missie uitgevoerd |
| Hollow Point als adres | `M2.6.S02` | **M2.7.S01** — de verhuizing. `Story.Beat.HollowPointLost` |

**Sub-draden die act 2 uit gaan, met een genoemde betaalplek** (dat is het verschil tussen een opgezette vraag en een wees):

| Sub-draad | Betaalplek | Vlag die hem draagt |
|---|---|---|
| Wie het adres van Hollow Point verkocht heeft | act 3, twist 3 | `Story.Flag.IntakeUnvetted` (act 1) + `Story.Clue.ThrexNetwork` |
| Of Torren de rebellie leert winnen of herbouwt wat hij ontvluchtte (§2.5) | act 3, twist 3 | `Story.Char.TorrenJoined` |
| Wat Whisper met root-toegang doet | act 4, AEGIS-beslissing | `Story.Choice.M27_Whisper.*` + `Story.Clue.RootFragment` |
| Waarom de reactie op uur 60 van karakter veranderde | act 4, twist 4 | `Story.Clue.ArmadaAllocation` |

---

## 6. Vlaggenregister

**De kolom *gelezen door* is geen documentatie — hij is de veiligheidscontrole (L1-R12), en hij is bij het schrijven ingevuld, niet achteraf.** Een lege of vage cel is zelf een bevinding. Een cel die een ongebouwd systeem noemt in plaats van de scènes die er staan, is een lege cel met een jasje aan (L1-R39).

**Alle onderstaande tags zijn nieuw en bestaan nog niet in `EclipseGameplayTags.cpp`.** Dat is een systeemtaak, geen schrijftaak: **geen enkele act-2-scène gebruikt een vlag in `condition:` voordat de tag bestaat.**

### Beat-vlaggen

| Tag | Gezet door | Gelezen door |
|---|---|---|
| `Story.Beat.M21_Carcass` | M2.1.S99 (debrief) | **M2.2.S01** · **M2.3.S01** · **M2.6.S07** (is er een plek om heen te gaan) · M2.7.S01 (de verhuizing) · M2.8-pin |
| `Story.Beat.M22_Strike` | M2.2.S99 | **M2.3.S01** · act 3 coalitiepolitiek (§2.9: *"unify miners"*) |
| `Story.Beat.M23_Nym` | M2.3.S99 | **M2.4.S01** |
| `Story.Beat.M24_LoyalGhost` | M2.4.S99 | M2.5-pin · **M2.7.S01** (het schip vervoert de verhuizing) · act 3 vlootlaag |
| `Story.Beat.M25_Relay` | M2.5.S99 | M2.6-pin · M2.7.S02 |
| `Story.Beat.M26_Siege` | M2.6.S99 | M2.7-pin · act 3 |
| `Story.Beat.M27_Custodian` | M2.7.S99 | M2.8-pin · act 3 |
| `Story.Beat.M28_HullRight` | M2.8.S99 | M2.9-pin |
| `Story.Beat.M29_SeaOfRust` | M2.9.S99 | act 3-pin |
| `Story.Beat.Act2Complete` | M2.9.S99 | act 3, hub-set, muziekstaat, strategiekaart |
| `Story.Beat.HollowPointLost` | **M2.6.S99** | **elke scène vanaf M2.7 — als locatie-gate.** Geen enkele scène na M2.6 speelt op `Kessara / Underworks / Hollow Point`. Zie AR-A2-8 |

### Personages

| Tag | Gezet door | Gelezen door |
|---|---|---|
| `Story.Char.KayaJoined` | **M2.3.S99** | **M2.4.S01** · **M2.4.S03** (zij weet waar Torren is) · **M2.5.S03** (aanraking A2-3) · M2.7 · act 3 vlootlaag · barks |
| `Story.Char.TorrenJoined` | **M2.4.S05** (waar hij instapt, niet de debrief) | **M2.5.S01** · **M2.6.S03** (aanraking A2-4) · **M2.9.S07** (aanraking A2-5) · **act 3, twist 3 — de dragende lezer** |
| `Story.Char.WhisperContact` | **M2.5.S05** (waar hij spreekt) | M2.6.S01 · **M2.7 (alle scènes)** · act 3 espionage |
| `Story.Char.PetraSiege.{Survived,Died}` | **M2.6.S08** | **M2.6.S99** · **M2.7.S03** · `Story.Thread.WallOpen` (bij `.Died`) · Brick, Reyes en Voss in act 3–4 · §2.11 |
| `Story.Char.KaineCommand` | **M2.9.S08** | act 3 Dominion Response · act 3–4 alle Kaine-scènes · twist 5-telling |
| `Story.Char.CinderNamedByDominion` | **M2.9.S08** | **Dominion- en Veil-barks vanaf act 3** (callsign-gebruik door de vijand) · act 3 propaganda · act 4 |

> **`Story.Char.CinderNamedByDominion` is niet hetzelfde als act 1's `Story.Char.CinderNamed`, en de scheiding is opzettelijk.** Act 1's vlag zegt *"de cellen hebben hem een naam gegeven"*; deze zegt *"de vijand gebruikt hem"*. Twee gebeurtenissen, een akte uit elkaar, met verschillende lezers: de eerste stuurt Eclipse-barks, de tweede Dominion-barks. Ze in één vlag samenvouwen zou de act-2-slotbeat onzichtbaar maken. **De normalisatie in `validate_script.py` (`fact_key`) laat ze uit elkaar** — `cindernamed` tegenover `cindernamedbydominion` — maar dat is krap, en wie een derde naamvlag toevoegt, controleert dat eerst met `--explain`.

### Keuzes — alle meerwaardige keuzes zijn tag-bladeren (L1-R3)

| Tag | Script-zijde | Gezet door | Gelezen door |
|---|---|---|---|
| `Story.Choice.M22_Krad.{Won,Armed}` | `story.m22_krad` = `won` \| `armed` | **M2.2.S05** | **M2.2.S06 (2 takken — de uitvoering)** · **M2.2.S99 (2 takken)** · **M2.9.S04** (wat Krad-9 stuurt) · **Brick in M2.2.S99 en act 3** · Sela-reputatie · act 3 coalitiepolitiek |
| `Story.Choice.M23_Terms.{Paid,Owed}` | `story.m23_terms` | **M2.3.S04** | **M2.4.S02** (waarmee je de crew betaalt) · **M2.7.S01** · `LOY.kaya` · act 3 vlootkosten |
| `Story.Choice.M25_Relay.{Kept,Burned}` | `story.m25_relay` | **M2.5.S06** | **M2.5.S99 (2)** · **M2.7.S02** (heeft Whisper nog een kanaal) · act 3 intel |
| `Story.Choice.M26_Carry.{Wounded,Records,Cache}` | `story.m26_carry` | **M2.6.S07** | **M2.6.S99 (3 takken)** · **M2.7.S03** · **M2.8.S01** (wat de Momentum-fase kost) · act 3 |
| `Story.Choice.M27_Whisper.{Trusted,Refused}` | `story.m27_whisper` | **M2.7.S06** | **M2.7.S99 (2)** · **M2.9.S02** · act 3 espionage-arc · **act 4 AEGIS-beslissing en de *free*-optie van §2.10 — de verste lezer in het spel** |
| `Story.Choice.M29_Ashline.{Broken,Bought,Spared}` | `story.m29_ashline` | **M2.9.S06** | **M2.9.S99 (3 takken)** · act 3 Tarsis-onrust · Sela-reputatie |

> **`M27_Whisper` heeft twee bladeren en geen derde, en dat is canon en geen bezuiniging.** §2.8 twist 2 kent precies twee uitkomsten met precies twee gevolgketens (*"trust ... and gain"* / *"refuse and he leaves ... immunity to twist #4"*). Een derde, verzoenende optie zou de immuniteit-tak onbepaald maken, en die tak is de enige plek in het spel waar de speler zich een wending kan besparen. **Wie een derde wil, wijzigt de bijbel — owner-beslissing.**

### Clues, intel en draden

| Tag | Gezet door | Gelezen door |
|---|---|---|
| `Story.Clue.BlightLedger` | **M2.5.S04** (waar de regel klinkt — L1-R38) | **act 3, twist 1** — de Blight-archiefonthulling |
| `Story.Clue.ArmadaAllocation` | **M2.6.S03** (waar de regel klinkt) | **act 4, twist 4 — de dragende van act 2**, naast act 1's `Story.Clue.AegisDenial` |
| `Story.Clue.RootFragment` | **M2.7.S05** | **act 4, AEGIS-opties** · act 3 espionage |
| `Story.Clue.ThrexNetwork` | **M2.6.S05** | **act 3, twist 3** — de Veil-vervalsing tegen Vale |
| `Story.Intel.LoyalGhost` | **M2.4.S99** | act 3 vlootlaag · M2.7.S01 · M2.9 (orbital drop / insertie) |
| `Story.Thread.CarcassHome` | **M2.7.S01** | **elke scène vanaf M2.7 — als locatie-gate**, tegenhanger van `HollowPointLost` |

**Regel (erf van act 1):** een `Story.Clue.*` verandert nooit gameplay in de akte waarin hij gezet wordt. Hij bestaat om latere akten te laten weten wat de speler heeft **gehoord**, zodat de betaling zich naar hem voegt. En: **een clue wordt gezet in de scène waarin hij wordt uitgesproken, nooit in de debrief** (L1-R38). Alle vier de clues hierboven doen dat.

### Vlaggen uit act 1 die act 2 LEEST — en waar

Dit is de andere helft van de veiligheidscontrole en hij ontbrak in act 1: het register van act 1 zegt *"gelezen door: act 2"*, en dat is een cel met een jasje aan tot iemand er een adres in zet. **Hier staan de adressen.**

| Act-1-vlag | Gezet in | **Adres in act 2** |
|---|---|---|
| `Story.Clue.CustodianKey` | `M1.2.S04` | **M2.7.S05** — verplicht, gesproken, dragend voor twist 2 |
| `Story.Clue.OutsideQuery` | `M1.7.S03` | **M2.5.S04** — verplicht, gesproken |
| `Story.Char.MaraDead` | `M1.8.S08` | **elke scène in act 2** — geen enkele live Mara-regel; de drie opnames zijn de uitzondering en staan in AR-A2-5 |
| `Story.Char.PetraRescued` | `M1.8.S04` | **M2.1.S03**, **M2.6.S02/.S08**, **M2.7.S03** — en hij is de voorwaarde voor `Story.Char.PetraSiege.*` |
| `Story.Char.CinderNamed` | `M1.8.S99` | **M2.1.S03** (de eerste keer dat een vreemde hem zo aanspreekt) · **M2.9.S08** |
| `Story.Choice.M15_Pact.{Full,Limited,None}` | `M1.5.S99` | **M2.6.S04 — twee gespeelde takken (`full`, `limited`); de `none`-tak speelt niets en krijgt een `silence:` met een reden van ≥40 tekens** |
| `Story.Choice.M16_LettersAllowed` | `M1.6.S06` | **M2.2.S99** — de eerste brieven komen terug (draad *Conscript Letters*, sluit in act 4) |
| `Story.Choice.M16_Train.{Run,Emptied,Split}` | `M1.6.S04` | **M2.1.S02** — wie er meekomt naar Tarsis hangt af van wie er toen meekwam |
| `Story.Flag.IntakeUnvetted` | `M1.6.S06`, stil | **act 3, twist 3.** Act 2 leest hem **niet** en mag hem niet resetten — AR-A2-9 |
| `Story.Thread.WallOpen` | `M1.8.S90` | **M2.6.S99** (Siege-doden) · **M2.7.S01** (de muur verhuist) |
| `Story.Thread.MaraLetters_Open` | `HUB.A1.mara_letters` | **M2.1.S99**, **M2.6.S99**, **M2.7.S02** — de drie adressen van AR-A2-5 |
| `Story.Thread.Enforcer_BadgeHeld` | proloog (Q-2) | **niet gelezen in act 2 — AR-A2-7, expliciet besluit met reden** |
| `Story.Beat.M18_BlacksiteK77` · `Story.Beat.Act1Complete` | `M1.8.S99` | **M2.1.S01** (act-2-pin en muziekstaat) |

> **Twee cellen in deze tabel zeggen "niet gelezen", en dat is met opzet een uitspraak in plaats van een leemte.** `IntakeUnvetted` en `Enforcer_BadgeHeld` gaan act 2 door zonder aanraking. Beide hebben een genoemde betaalplek en een ruling die zegt waarom. Een vlag die in een akte niet gelezen wordt is geen defect; een vlag waarvan niemand heeft opgeschreven dat dat een keuze was, wél.

### Run-feiten van act 2 (`run.` — overleeft de missie niet, L1-R4/L1-R26/L1-R50)

**De vuistregel is L1-R50's vuistregel: *leest act 3 dit? Nee → `run.`*.**

| Fact | Soort | Gezet / gelezen |
|---|---|---|
| `run.zero_casualty`, `run.alarm_raised`, `run.ghost` | latch | bestaand; act-2-debriefs lezen ze |
| `run.m26_defence_held` | **latch — te bouwen** | de Siege-uitkomst die `Story.Char.PetraSiege.*` bepaalt (§2.11: *"her death is possible ... if defenses fail"*). **Dit is de enige `run.`-fact van act 2 die een systeem moet leveren; de rest zijn gesprekstakken.** Zie C-A2-3 |
| `run.m21_hullright_offer` | choice | M2.1.S04, gelezen in M2.1.S04/.S05 |
| `run.m23_pitch` | choice | M2.3.S03, gelezen in M2.3.S03/.S04 |
| `run.m27_probe` | choice | M2.7.S04, gelezen in M2.7.S04/.S05 — hoe je een man ondervraagt die geen namen gebruikt |
| `run.m29_broadcast` | choice | M2.9.S08, gelezen in M2.9.S08 — wat Eclipse terugzegt tegen de omroep |

---

## 7. Locatieregister (L4) en nummerregister

### Locatieregister

`SCRIPT_FORMAT` §6 valideert `location` tegen het locatieregister van de akte. **Voor act 1 is dat `ACT1_OVERVIEW` §7; voor act 2 is dat onderstaande tabel.** `03_world_design.md` §3.3 geeft geen enkele districtnaam voor Tarsis, Krad-9, The Shroud of Meridia — precies dezelfde leemte als bevinding C-1 voor Kessara, één akte groter. **Elke string hieronder is opgebouwd uit zelfstandige naamwoorden die letterlijk in §3.3 of in de canon-glossary staan**, en de bronkolom zegt welke. Er staat geen enkele verzonnen plaatsnaam in.

| Canonieke `location`-string | Planeet | Waar de woorden vandaan komen |
|---|---|---|
| `Tarsis / Dune Sea / The Carcass` | Tarsis | **`00_INDEX.md` glossary**: *"The Carcass (Tarsis HQ)"* · `05_base_building.md` §5.2 · §3.3 *"dune seas"* |
| `Tarsis / Dune Sea / The Carcass — Map Table` | Tarsis | idem; spiegelt Hollow Points sub-strings uit `ACT1_OVERVIEW` §7 |
| `Tarsis / Dune Sea / The Carcass — Workshop` | Tarsis | idem |
| `Tarsis / Dune Sea / Wreck-Town` | Tarsis | §3.3 *"~40M scavenger-nomads in wreck-towns"* |
| `Tarsis / Dune Sea / Salvage Yard` | Tarsis | §3.3 *"its salvage yards jump-start the player's vehicle programs"* |
| `Tarsis / Dune Sea / Hull Carcass` | Tarsis | §3.3 *"kilometer-long hull carcasses that form natural fortresses, towns, and dungeons"* — een ánder wrak dan The Carcass |
| `Krad-9 / Surface` | Krad-9 | §3.3 *"Surface: vacuum, stark shadows, low gravity"* |
| `Krad-9 / Tunnel City` | Krad-9 | §3.3 *"pressurized tunnel-cities"* |
| `Krad-9 / Tunnel City — Ore Cathedral` | Krad-9 | §3.3 *"ore cathedrals"* |
| `Krad-9 / Deep Shaft` | Krad-9 | §3.3 *"unstable deep shafts"* |
| `The Shroud / Nym / Dome City` | The Shroud | §3.3 *"the moon Nym's dome-city"* · glossary *"The Shroud (moon Nym)"* |
| `The Shroud / Nym / Docking Sprawl` | The Shroud | §3.3 *"and docking sprawl"* |
| `The Shroud / Drift Station` | The Shroud | §3.3 *"drift-station swarms ... plus station interiors"* |
| `Meridia / Float City / Trade Quarter` | Meridia | §3.3 *"tessellated float-cities"* + *"high-society trade quarters"* |
| `Meridia / Float City / Under-Deck` | Meridia | §3.3 *"under-deck maintenance labyrinths"* |
| `Meridia / Tidal Platform` | Meridia | §3.3 *"tidal platforms"* |
| `Meridia / Memorial Coast` | Meridia | §3.3 *"the Blight-memorial coasts of the single archipelago"* |
| `Loyal Ghost / Bridge` | (schip) | glossary *"Loyal Ghost (first corvette)"* |
| `Kessara / Underworks / Hollow Point` | Kessara | **bestaat al** — `ACT1_OVERVIEW` §7 |
| `Kessara / Underworks / Hollow Point — Map Table` | Kessara | **bestaat al** |
| `Kessara / Underworks / Hollow Point — Workshop` | Kessara | **bestaat al** |
| `Kessara / Underworks / Hollow Point — Bunk Row` | Kessara | **bestaat al** |
| `Kessara / Underworks / Coolant Sublevel` | Kessara | **bestaat al** — de Siege gebruikt hem als aanvalsroute |

**Regio-pins.** De gebouwde Phase-1-graaf (`Eclipse/Tools/create_phase1_content.py`) kent zes regio's en die liggen **allemaal op Kessara**: `Underworks`, `TransitCheckpoint`, `FoundryRow`, `WorkerHousing`, `SupplyDepot`, `CommsRelay`. **Er is geen enkele regio voor Tarsis, Krad-9, The Shroud of Meridia.** Acht van de negen act-2-missies hebben dus geen pin om op te landen. Dat is geen schrijfprobleem en het wordt niet omzeild — het staat als bevinding **C-A2-3** in §9 en het hoort in de EXECUTION_PLAN-backlog.

### Het register van act 2 is niet tellen

**Erf van L1-R30/ronde 1, en dit is de belangrijkste stijlbeslissing van de akte.** Kessara telt: een gieterijcultuur met ploegclans, scrip-lonen en rantsoenen, en vier beroepen die elk in hun eigen eenheid tellen. **Dat register verlaat Kessara niet.**

| Planeet | Register | Waar het vandaan komt | Wat het in dialoog doet |
|---|---|---|---|
| **Kessara** | **tellen** | `03_world_design.md` §3.3; L1-R30 ronde 1 | blijft in M2.6 (de Siege), en **alleen** daar |
| **Tarsis** | **claimen en verschuldigd zijn** | §3.3: *"fiercely independent salvage clans with a debt-and-honour culture ('hull-right' salvage law)"* | mensen zeggen wat iets van hen is en wat ze iemand schuldig zijn, in plaats van hoeveel er is |
| **Krad-9** | **delen** | §3.3: *"Union-brotherhood culture forged by shared air: 'You breathe what your crew breathes.'"* | de eerste persoon meervoud is er letterlijk lucht; niemand zegt "ik" over iets dat een ploeg raakt |
| **The Shroud** | **beloven** | §3.3: *"Culture of contracts and reputation: your word is collateral."* | zinnen zijn toezeggingen met een prijs; niemand zegt iets vrijblijvend, ook niet als grap |
| **Meridia** | **bezitten** | §3.3: trade-house-cultuur, *"words are ammunition"*, handelsgrootboeken als grondstof | mensen praten in eigendom, aandeel en aanspraak |

**Dit is de act-2-invulling van `03_world_design.md` productieregel 4 (*"culture through systems"*), en het is meteen de goedkoopste bescherming tegen §18.9 D.** Vijf planeten, vijf grammatica's; een regel die op de verkeerde planeet staat is mechanisch hoorbaar. **Maximaal één registerbeat per scène draagt de wending** — dezelfde begrenzing als act 1's telbeat, en om dezelfde reden.

### Nummerregister — dragende getallen die meer dan één scène raken

**Uitgangspunt, en het is gemeten door anderen en niet door mij:** na act 1 is **geen enkel getal onder de twintig nog vrij**. Vier agents kozen op 01-08 elk onafhankelijk *"het enige vrije getal"* en alle vier hadden gelijk op het moment dat ze maten. `Eclipse/Tools/check_spoken_numbers.py` bestaat om dat antwoord uit een tool te halen in plaats van uit een notitie. **Ik heb hem deze ronde niet kunnen draaien (geen shell) en dit document claimt daarom nergens dat een getal vrij is.**

**De architectuurbeslissing die daaruit volgt, en die is van mij:**

> **Act 2 bouwt geen enkel dragend getal onder de twintig.** Een dragend getal van act 2 is **≥ 20**, of het is een **getal met een eenheid** (L1-R55: wat een register beschermt is het getal *in zijn eenheid*, zodra de eenheid de betekenis draagt). Een kaal cijferwoord onder de twintig mag vallen zoals het valt — herhaling is geen defect (L1-R33) — maar het mag nooit iets *dragen*.

Dat kost act 2 niets en het maakt de kruisakte-botsing structureel onmogelijk in plaats van telkens opnieuw te controleren.

| Getal | Betekent in act 2 | Dragers | Mag niets anders meten |
|---|---|---|---|
| **elf** | **niets nieuws.** Het blijft de omvang van Ember Cell (AR-1/L1-R14) en het staat in act 2 uitsluitend in **verleden tijd**, als iemand het over vroeger heeft | hoogstens één act-2-gebruik, in M2.7 | het is een herinnering, geen telling |
| **eenenveertig** | **niets nieuws.** De instroom van de Tithe-trein, campagneconstante (L1-R32/L1-R42) | erven; act 2 telt hem niet opnieuw | nooit *"de veertig"* |
| **veertig** | **niets nieuws.** De bevrijden uit K-77 (L1-R42) | erven | act 2 spreekt hem niet uit |
| **"hoeveel zijn we"** | **wordt in act 2 nooit uitgesproken.** Zie de ruling hieronder | — | — |
| **vijfenvijftig graden** | de daghitte van Tarsis; de reden dat er buiten de nacht niet gewerkt wordt | M2.1, M2.8, M2.9 | `03_world_design.md` §3.3 letterlijk. Getal **met eenheid** |
| **nul komma vier** | de zwaartekracht van Krad-9; verandert hoe mensen vallen en gooien | M2.2 | §3.3 letterlijk. Getal met eenheid, en niet uit te spreken als kaal cijferwoord |
| **vijf manen** | The Shroud | M2.3, M2.4 | §3.3 *"five habitable-domed moons"*. **Vijf is verzadigd als kaal woord** — dit gebruik heeft zijn eenheid nodig of het wijkt |
| **uur zestig** | het moment waarop het model brak (twist 4) | **nergens uitgesproken in act 2** | §2.8 noemt het als auteursbegrip. Het is een *ontwerp*getal en geen dialooggetal. **Wie het uitspreekt, verraadt twist 4** |

> **RULING AR-A2-10 — act 2 spreekt geen totale sterkte uit.** §2.9 zet act 1 op *"~15 soldiers and the loyalty of three fused cells"*, terwijl act 1's eigen registers eenenveertig (instroom) en veertig (K-77) dragen. Die drie getallen zijn niet met elkaar te verzoenen in één gesproken zin, en act 1 heeft ze bewust uit elkaar gehouden. **Act 2 mag ze niet samenvoegen.** Er is geen enkele beat in deze akte die een totaal nodig heeft: waar een scène de groei wil laten voelen, doet hij dat met ruimte, geluid, drukte en een tekort — nooit met een som. **Een schrijver die een totaal nodig denkt te hebben, escaleert.**
>
> **Wanneer je hier een rij bij zet:** zodra een getal een tweede scène raakt. Niet als het één keer valt. **En je claimt nooit in een `note:` dat een getal vrij is** — je draait `python Eclipse/Tools/check_spoken_numbers.py` en je verwijst naar de tool. Een notitie bevriest een telling die doorloopt; dat is L1-R33 en het heeft één keer een veegbeurt over vijftien bestanden gekost.

### Cijferconventie
Ongewijzigd overgenomen (L1-R36): cijfers die één voor één worden uitgesproken zijn **losse woorden zonder koppeltekens** (*"Two six one."*). Getallen die als getal klinken blijven één woord (*"Forty-one"*, *"Seventeen"*).

---

## 8. Architect-rulings van act 2

Beslissingen die ik neem omdat de bedoeling al vastligt en alleen de invulling ontbrak. Elk is omkeerbaar door de owner; elk staat hier mét de redenering, zodat omkeren goedkoop is. **De nummering is `AR-A2-*` zodat hij nooit met act 1's `AR-1…AR-12` kan botsen.**

| # | Ruling | Waarom dit de goedkoopste lezing is |
|---|---|---|
| **AR-A2-1** | **Kaine spreekt haar eerste regel in `M2.9.S08` en geen seconde eerder.** Vex blijft opname/omroep. Callis spreekt niet in act 2. Mara spreekt uitsluitend als opname, op drie adressen. | §2.9 zegt letterlijk *"Kaine takes personal command"* aan het **eind** van act 2. Een Kaine-regel in M2.6 maakt van de act-9-beat een terugkeer in plaats van een aankomst, en §18.4 geeft haar *"states her intention before acting on it"* — een fingerprint die alleen werkt als haar eerste optreden een aankondiging ís. |
| **AR-A2-2** | **Voss erft de tweede persoon, en hij erft hem langzaam.** Mara spendeerde "you" precies één keer, in `M1.8.S08`, en de slotscène van act 1 gaf hem aan niemand. In act 2 gebruikt Voss de tweede persoon **in bevelen aan individuen vanaf M2.6 en niet eerder**; in M2.1–M2.5 spreekt hij groepen aan. De ene keer dat hij hem vóór M2.6 gebruikt, is tegen **Torren** in `M2.4.S05`, op het moment dat hij hem aanneemt. | Dit is de enige act-1-draad die de akte uit gaat zonder een geadresseerde erfgenaam, en hij kost niets om te sluiten. De ladder wordt: M1.1 derde persoon aan de squad (AR-5b) → M1.8 tweede persoon aan hem (AR-5) → **act 2: hij geeft hem door, aan één man, en pas als hij een basis moet verdedigen.** Dat is een grammaticale beweging van drie treden over zeventien missies en hij is met een regex te controleren. Geen enkele schrijver mag Voss vóór M2.4 een individu in de tweede persoon bevelen. |
| **AR-A2-3** | **Threx escaleert niet in act 2.** AR-6's ladder is in `M1.8.S06` op zijn top geëindigd (4b, in persoon, alleen tegen hem). Act 2 voegt **geen vijfde trede** toe. Threx komt twee keer voor, allebei in M2.6, en **allebei sprekend over Voss tegen iemand anders** — een daling op de afstandsas. | AR-6's as is AFSTAND en die is op: verder dan "in persoon, alleen tegen jou" is er niets. Een vijfde trede zou een trede verdubbelen en dat is bij AR-6 uitdrukkelijk verboden. **De daling is bovendien het verhaal**: een jager die het persoonlijke duel verloor en het institutioneel maakt, is precies de man die in act 3 een dossier vervalst (twist 3). Hij wordt niet minder gevaarlijk; hij wordt minder aanwezig, en dat is erger. |
| **AR-A2-4** | **De ingesproken nullijnen van Torren en Kaya liggen in act 2 en hebben een adres: Kaya in `M2.3.S02`, Torren in `M2.5.S02`.** Elk bevat **precies één** uitslag van de eigen tell. | §18.4 eist het woordelijk: *"Torren's and Kaya's fall in Act 2 and are owed the same treatment"* (als Reyes' `M1.4.S99`, ruling L1-R19). En L1-R37's generalisatie: het beste ingesproken niet-X bevat één X, anders is het geen nullijn maar een ander personage. Zonder adres kiezen vier schrijvers vier plekken en krijgt geen van beiden er een. **Kaya**: haar tell is de ondermijning van een serieuze zin, dus haar nullijn is een scène waarin niets serieus gezegd wordt en de run-ons gewoon haar snelheid zijn — één keer slaat de naald uit. **Torren**: zijn tell is de langste stilte van het spel op zijn eigen beat, dus zijn nullijn is een scène waarin hij normaal getimed praat — en één keer niet. |
| **AR-A2-5** | **Act 2 speelt drie Mara-opnames, op drie adressen, en geen enkele beantwoordt de vraag van zijn scène.** `M2.1.S99` (de eerste keer weg van Kessara) · `M2.6.S99` (de basis is weg) · **`M2.7.S02` (de bron die ze nooit noemde — P2-c's betaling)**. Geen enkele opname speelt in een scène waarin de speler een keuze maakt. | §2.5 maakt de brieven canon *"through Act 4"*, en `M1.8.S91` legt er elf vast die de cel spiegelen (L1-R14). Zonder adressen grijpt elke schrijver naar dezelfde troostende brief op zijn zwaarste moment en is Mara na drie missies goedkoop. **De keuze-uitsluiting is de belangrijkste helft**: een dode mentor die meepraat op het moment dat de speler beslist, draait `M1.8.S08` terug — dat was de promotie van de speler en die wordt niet twee akten lang teruggegeven. |
| **AR-A2-6** | **M2.3 is de hoofdmissie die contact legt en Kaya werft; `LOY.kaya` is de latere, aparte missie die van contact een *alliantie* maakt.** | §2.9 noemt *"Kaya's smuggler network"* een hoofdmissie van act 2 en §2.5 noemt haar loyaliteitsmissie *"unlocks the smuggler network alliance"*. Beide kunnen alleen waar zijn als het twee dingen zijn, en dat is ook de enige lezing waarin de telling 8+**9**+10+7 klopt. Bovendien is het de logische volgorde: je ontgrendelt geen alliantie met mensen die je nog niet ontmoet hebt. **Welke acht loyaliteitsmissies in welke akte vallen, is niet vastgelegd — Q-A2-2.** |
| **AR-A2-7** | **De draad *The Enforcer* wordt in act 2 niet aangeraakt, en dat is een besluit met een reden.** | De draad hangt aan een badge uit de Kessara-rantsoenrij en aan één man met kenteken `two six one`. Act 2 speelt één missie op Kessara (M2.6, en dat is een belegering waarin niemand tijd heeft om een badge na te trekken). Hem er toch in schuiven kost een scène die de akte niet heeft, en de betaling ligt in act 3/4 als *persoonlijke keuze*. **Een draad die je bewust laat liggen met een genoemde betaalplek is geen wees; een draad die je aanraakt en niet afmaakt, wel.** Bovendien staat de wortel nog steeds op Q-2 — er is niets aan te raken tot die vraag beantwoord is. |
| **AR-A2-8** | **Hollow Point wordt niet vernietigd. Hij wordt gebrand als adres.** Na `M2.6.S99` is de plek bekend bij de Dominion en daarmee onbruikbaar; hij staat er nog. Geen enkele scène na M2.6 speelt er. | `05_base_building.md` §5.2 vraagt een *reden om te verhuizen* en geen puinhoop; het zegt bovendien dat relocatie *"a set-piece, not a reset"* is en dat upgrades meegaan. En `03_world_design.md` §3.3 belooft Kessara een late *"Homecoming"*-terugkeerarc — die is onmogelijk als de Underworks-vault in act 2 instort. **Verlies door kennis in plaats van door puin is bovendien de these van de akte**: de beweging werd een plaats, en een plaats is een adres. |
| **AR-A2-9** | **Act 2 raakt `Story.Flag.IntakeUnvetted` niet aan en lost het K-77-verraad niet op.** Wie in act 2 een verklaring voor de Siege wil geven, geeft er precies één en niemand vraagt door (P3-A2-c). | Act 1 heeft drie onopgeloste verklaringen achtergelaten (P3-d) en die worden in act 3 betaald. Een vierde verklaring toevoegen maakt van een wond een opsomming (L1-R12's slotalinea, woordelijk). En de vorm herhalen is §18.9 D's laatste bullet. **Act 2's bijdrage aan twist 3 is dat de vraag niet gesteld wordt** — niet dat hij nog een keer gesteld wordt. |
| **AR-A2-10** | **Act 2 spreekt geen totale sterkte uit.** Zie §7, nummerregister. | Drie onverenigbare canon-getallen (≈15 soldaten, eenenveertig, veertig). Er is geen beat die een som nodig heeft. |
| **AR-A2-11** | **Act 2 verbruikt geen oration.** Er zijn er vier in het hele spel (§18.3); AR-7 reserveert ze voor act 3 (Sela, oprichting Concord) en act 4 (Vex, Voss, één reserve). Ook de bevrijdingstoespraak in M2.9 blijft binnen de `cutscene`-band. | Een oration op uur 65 verbrandt een van vier schaarste-momenten op een overwinning die act 3 nog moet overtreffen. AR-7 is act 1's ruling en de redenering geldt hier onverkort. |
| **AR-A2-12** | **De Siege of Hollow Point is één missie met twee helften en het wordt niet in twee missies gesplitst.** De basislay-out ís het slagveld (§2.9), en de evacuatie is de tweede helft van diezelfde missie, niet een nieuwe. | §2.9 noemt de Siege één keer, in één adem, als één van de zeven beats. Splitsen zou de negentelling breken (§3) en het zou de enige beat van de akte waarin verlies onvermijdelijk is, in twee draaglijke helften knippen. **M2.6 is daarom act 2's grootste missie**, zoals M1.8 dat voor act 1 was. |

---

## 9. Canon-conflicten en owner-vragen

### Conflicten (gevonden, niet stil opgelost)

| # | Conflict | Impact | Mijn voorstel |
|---|---|---|---|
| **C-A2-1** | `02_story_bible.md` §2.9 zegt dat act 2's strategiekaart **Kessara + Tarsis, Krad-9 en The Shroud** is. Maar §2.9 zet in diezelfde alinea de **Meridia**-relay-heist als act-2-hoofdmissie, en `03_world_design.md` §3.3 zet **Vel'Naar** op *"Act focus 2"* met *"Act 2's Petra-adjacent prison arc"*. Twee planeten die in act 2 spelen staan niet op de act-2-kaart. | M2.5 speelt op een planeet die volgens de bijbel niet bereikbaar is; en er staat een Vel'Naar-arc in de canon die in geen enkele M2.x past. | **Mijn lezing, en ik heb hem niet stil doorgevoerd:** de "3 reachable planets" beschrijven de **strategiekaart** (de systemische laag), terwijl `03_world_design.md` §3.2 authored verhaalmissies expliciet toestaat op *"separate smaller levels"*. M2.5 is zo'n level: een heist op één platform, geen open zone. Voor Vel'Naar geldt hetzelfde niet — daar staat een hele **arc**. **Mijn voorstel: de Vel'Naar-prison-arc is campagne-sjabloonmateriaal (`11_missions.md` §11.2 *Breach*), geen authored M2.x.** Dat past bij de negentelling en bij §3.3's eigen zin dat Vel'Naar *"how the player buys skill they can't train"* is — een systeem, geen verhaalbeat. **Als de owner er een authored missie van wil, dan verschuift de negentelling en dat is een bijbelwijziging.** |
| **C-A2-2** | `19_voice_production.md` §19.3 mist castingrijen voor **elke niet-companion-stem van act 2**: een Ashline Cartel-spreker, een Tarsische salvage-clanoudste, een Krad-9-stakingsleider, en Meridiaanse handelshuis-burgers. §18.4 mist hun fingerprints. | **Blokkerend voor generatie van act 2** — precies dezelfde vorm als bevinding C-4 voor act 1, die nog steeds openstaat. | Vier rijen erbij, in dezelfde vorm als L1-R9/L1-R6b: fingerprint eerst (die kan ik leveren zodra de owner de rollen bevestigt), castingrij daarna. **Ik heb ze niet verzonnen** — er staat in geen enkel act-2-stub een naam voor deze mensen. |
| **C-A2-3** | De gebouwde regiograaf kent zes regio's en die liggen alle zes op Kessara. **Acht van de negen act-2-missies hebben geen regio-pin.** Bovendien vraagt M2.6 een `run.m26_defence_held`-latch die niet bestaat, en M2.4/M2.9 vragen een ruimte-asset en een hybride veldslag. | Act 2 is schrijfbaar en op dit moment niet instantieerbaar. | Melden, niet omzeilen — exact wat `ACT1_OVERVIEW` §10 met de M1.5–M1.8-systeemnaad deed. Hoort in de EXECUTION_PLAN-backlog. **Geen enkele act-2-scène doet alsof het bestaat.** |
| **C-A2-4** | `validate_script.py` doet de **LOCATION**-controle alleen voor `act == 1` (`if act == 1 and loc not in ...`), en `load_locations()` leest uitsluitend `ACT1_OVERVIEW` §7. Act-2-scènes worden dus **niet** op locatie gecontroleerd. | Het locatieregister hierboven is documentatie tot de tool hem leest. Een typefout in een act-2-`location:` is per constructie onzichtbaar. | Eén wijziging: `load_locations()` per akte laden en de act-gate laten vallen. **Ik heb de tool niet aangeraakt.** |
| **C-A2-5** | `load_flag_register()` leest uitsluitend `ACT1_OVERVIEW` §6 en werpt `SourceShapeError` bij minder dan 15 rijen. Elke act-2-vlag die een scène zet of leest, zal daardoor als *onbekend* of *setterloos* gemeld worden — of erger, stil blijven. | **De eerste act-2-scène die op schijf komt, maakt de bar rood of blind.** Dit is de duurste van de vijf, want hij treft het instrument dat act 1's fouten heeft gevangen. | `load_flag_register()` moet §6 van beide overzichten samenvoegen. **Dit moet gebeuren vóór de eerste act-2-`.yaml`**, niet erna. |
| **C-A2-6** | `SCRIPT_FORMAT` §4 staat als `mission`-waarde `M1.1`–`M4.7` toe, dus M2.1–M2.9 passen. Maar de act-2-hub zou `HUB.A2` heten en dat staat er niet; en `credit_tier` is gedocumenteerd als *"0–5 per §19.2"* terwijl §19.2's ladder bij 4 ophoudt en act 2 er niet op staat. | Act-2-scènes hebben geen gedefinieerde tier. | **Mijn voorstel: act 2 = `credit_tier: 5`**, met de betekenis *"na de 131k-maand, op de Starter-cadans van §19.2"*. Dat is de enige vrije waarde die het formaat al toestaat en het sorteert act 2 correct achter tier 4. **`HUB.A2` toevoegen is een tweede regel in hetzelfde document.** Beide raken een bestand dat ik deze ronde niet bewerk — zie de tabel hieronder. |
| | **CORRECTIE 02-08, gemeten door de coördinator en niet door mij: dit is een SPECIFICATIEGAT, geen TOOLGAT.** Ik had aangenomen dat `validate_script.py` de `mission`-waarde afdwingt. **Dat doet hij niet** — er is geen hardcoded missiepatroon en geen `HUB.A`-vorm in de tool, op beide gegrepd. Een act-2-hubscène zákt er dus niet op. | De urgentie zakt van *blokkerend* naar *aarzeling*. | Het risico verandert van soort: een schrijver die `HUB.A2` maakt krijgt geen foutmelding, maar ook geen bevestiging uit het formaatdocument — en dat wordt een escalatie naar mij die één regel in §4 nu al voorkomt. **En de les is generiek en hoort hier genoteerd: weet per escalatie of het de tool is of het document.** Act 1 ging hier één keer precies andersom mis (iemand nam aan dat de tool iets afdwong wat alleen in proza stond, en bouwde eromheen). Dit is dezelfde fout in spiegelbeeld, en hij was van mij. |
| **C-A2-7** | `02_story_bible.md` geeft act-1-missies canonieke titels en act 2 geen enkele. | Negen missies zonder naam; filenamen en beat-vlaggen hangen eraan. | Werktitels in §3, vier ervan woordelijk canon. **Q-A2-1.** Ze staan in geen enkele `text:` en zijn daarom gratis te wijzigen tot generatie. |

### Aangevraagde L1-rulings — *voor seriële landing, door de architect die `RULINGS_L1.md` bezit*

**Ik heb `RULINGS_L1.md` niet aangeraakt en er geen tweede van gemaakt.** Deze zeven hebben een uitspraak nodig in dat log of in een bestand dat ik deze ronde niet bewerk. Ze staan hier zodat ze niet zoekraken; ze staan **niet** als besluit geformuleerd.

| # | Waar het landt | Wat er beslist moet worden |
|---|---|---|
| **RQ-1** | `SCRIPT_FORMAT` §3/§4 | `credit_tier: 5` = act 2, en wat het betekent. Zonder dit heeft geen enkele act-2-stub een geldige tier (C-A2-6). |
| **RQ-2** | `validate_script.py` + `SCRIPT_FORMAT` §6 | Vlaggenregister en locatieregister per akte laden in plaats van act-1-only (C-A2-4, C-A2-5). **Dit is de enige van de vijf die geld kan kosten als hij te laat komt.** |
| **RQ-3** | `18_writing_standard.md` §18.4 | Fingerprints voor de vier act-2-rolstemmen (C-A2-2). Zelfde vorm als L1-R9 (Petra) en L1-R6b (de emissaris): fingerprint eerst, casting daarna. |
| **RQ-4** | `SCRIPT_FORMAT` §4 | `HUB.A2` toevoegen aan de toegestane `mission`-waarden, óf vaststellen dat act 2 geen hub krijgt zolang de hub-tier buiten het budget valt (L1-R11). **Niet blokkerend — gemeten: de validator dwingt de `mission`-waarde niet af** (zie de correctie bij C-A2-6). Dit kost één regel en voorkomt één escalatie; het houdt geen enkele schrijver tegen. |
| **RQ-5** | `RULINGS_L1.md` | Bevestiging dat act-2-architect-rulings in `ACT2_OVERVIEW` §8 horen met het `AR-A2-*`-prefix, en niet in het L1-log. Dit is de afspraak die ik heb aangenomen; hij hoort ergens bevestigd te staan zodat de derde akte hem niet opnieuw hoeft te bedenken. |
| **RQ-6** | `ACT1_OVERVIEW` §4 (P2-d-rij) **of** niets | **P2-d is niet betaalbaar in act 2** omdat `run.m17_record` de missie niet overleeft (zie de waarschuwing in §4, T2). Kiezen tussen: act 1 een `Story.Clue.*` geven (duur, heropent een gegenereerde akte), de regel onvoorwaardelijk laten spelen, of hem laten vervallen. **Mijn advies: laten vervallen.** Ik heb geen van drieën doorgevoerd. |
| **RQ-7** | `validate_script.py` | **Een generieke variant van RQ-6:** een `condition` op een `run.`-fact die in een *andere missie* gezet wordt, is altijd fout, en de huidige `CONDITION`-controle vangt hem niet — de fact staat in `SCRIPT_FORMAT` §4 gedeclareerd en telt dus als "iets zet hem". Over 42 missies gebeurt dit nog een keer. **Ik heb dit gevonden doordat ik het zelf schreef**, niet doordat een tool het meldde. |

### Owner-vragen

| # | Vraag | Blokkeert | Kosten van uitstel |
|---|---|---|---|
| **Q-A2-1** | Akkoord met de negen **werktitels** in §3, of komen er canonieke namen? | niets nu | **Nul vandaag, hoog na generatie** — maar alleen als een titel ooit uitgesproken wordt. Ze staan in geen enkele `text:`. |
| **Q-A2-2** | **Welke van de acht loyaliteitsmissies vallen in act 2?** De canon pint alleen Dex' broer op Vorn (act 3) en Kaya's netwerk op act 2. | de 42-telling en de schrijfplanning van acts 2–3 | Middel. Elke loyaliteitsmissie is ~1.800 woorden (§18.1) en ze concurreren met M3.x om dezelfde schrijvers. |
| **Q-A2-3** | Vier nieuwe stemrollen voor act 2 (C-A2-2). Bevestigt de owner dat deze rollen bestaan, en met of zonder naam? | generatie van M2.1, M2.2, M2.5, M2.9 | Laag in geld, **hoog in volgorde**: casting wordt vóór tier 1 gelockt en is daarna permanent. |
| **Q-A2-4** | **Vel'Naar** (C-A2-1): sjabloonmateriaal, of een authored act-2-missie? Het tweede breekt de negentelling en is een bijbelwijziging. | de structuur van act 2 | Laag nu, hoog zodra M2.x-bestanden op schijf staan. |
| **Q-A2-5** | **Petra's dood in M2.6.** §2.11 maakt hem mogelijk *"if defenses fail"*. Bevestigt de owner dat dit een systemische uitkomst is (de basisverdediging bepaalt het) en geen dialoogkeuze? | M2.6, M2.7 en elke Petra-scène in acts 3–4 | **Hoog.** Als het een keuze wordt in plaats van een uitkomst, verandert de hele scèneopzet van M2.6.S07/S08 én moet elke latere Petra-regel dubbel geschreven worden. |
| **Q-A2-6** | Erft act 2 een **hub**? De act-1-hub viel uit het budget (L1-R11) en blijft geschreven-maar-ongegenereerd. Krijgt The Carcass twaalf gesprekken, of wacht act 2 op dezelfde manier? | schrijfplanning, niet de missies | Laag nu. Maar als het antwoord ja is, is het ~12 scènes en die concurreren met M3.x. |

---

## 10. Scèneregister act 2 — planning

**Scènenummering ongewijzigd** (`ACT1_OVERVIEW` §10): `S01` = briefing, `S02`–`S0n` = uitvoering in speelvolgorde, `S90`/`S91` = na-de-missie-beats die geen debrief zijn, `S99` = debrief. Regel-ID's in tientallen. Scènenummers zijn opeenvolgend en **permanent**.

| Missie | Scènes | Map | Beat-sheet | Status |
|---|---|---|---|---|
| M2.1 *The Carcass* | 7 | `act2/M2.1_the_carcass/` | `BEATS_M2.1.md` | **geleverd** |
| M2.2 *The Strike* | 7 | `act2/M2.2_the_strike/` | `BEATS_M2.2.md` | **geleverd** |
| M2.3 *Nym* | 6 | `act2/M2.3_nym/` | `BEATS_M2.3.md` | **geleverd** |
| M2.4 *Loyal Ghost* | 7 | `act2/M2.4_loyal_ghost/` | `BEATS_M2.4.md` | **geleverd** |
| M2.5 *The Relay* | 8 | `act2/M2.5_the_relay/` | `BEATS_M2.5.md` | **geleverd** |
| M2.6 *The Siege of Hollow Point* | 11 | `act2/M2.6_the_siege/` | `BEATS_M2.6.md` | **te schrijven** — adressen staan vast in §4/§5/§6 |
| M2.7 *The Custodian* | 8 | `act2/M2.7_the_custodian/` | `BEATS_M2.7.md` | **te schrijven** — adressen staan vast |
| M2.8 *Hull-Right* | 6 | `act2/M2.8_hull_right/` | `BEATS_M2.8.md` | **te schrijven** — adressen staan vast |
| M2.9 *The Sea of Rust* | 10 | `act2/M2.9_sea_of_rust/` | `BEATS_M2.9.md` | **te schrijven** — adressen staan vast |
| **Totaal** | **70** | | | |

**M2.6 en M2.9 zijn met opzet ongeveer tweemaal de gemiddelde missie**, zoals M1.8 dat voor act 1 was: de één is het scharnier, de ander de act-climax. De overige zeven liggen daaronder en het gemiddelde komt uit op de ~1.600 woorden per missie uit §18.1.

**De vier nog niet geleverde beat-sheets zijn niet ongedekt.** Hun scèneadressen staan in dit document vast — elke plant, elke aanraking, elke vlag en elke draad hierboven noemt een `M2.6.S0x`/`M2.7.S0x`/`M2.8.S0x`/`M2.9.S0x`. Wie ze schrijft, vult in wat hier al besloten is; hij beslist het niet opnieuw. Zie de laatste alinea van §12.

---

## 11. Wat de dialogue-writer als eerste doet

1. **Lees je missie-beat-sheet volledig** (`BEATS_M2.x.md`), plus §2, §7 (het register van jouw planeet) en §8 hierboven. Niet de hele bijbel — dat is mijn werk en het is gedaan.
2. **Lees ook `ACT1_OVERVIEW.md` §8 en `RULINGS_L1.md`.** Act 2 begint niet op nul: 44+ rulings uit act 1 gelden onverkort. De vier die je het vaakst raken: **L1-R1** (banden zijn plafonds), **L1-R3** (meerwaardige keuzes zijn tag-bladeren), **L1-R4/L1-R50** (`run.` versus `story.`), **L1-R17** (wegverklaringen zijn vlak, niet gevat).
3. **Open de stubs van je missie.** Header, `want`/`obstacle`/`turn` en `type` staan er al. **Je verandert die niet.** Vind je ze fout, dan is dat een escalatie naar story-architect, geen stille herschrijving.
4. **Vul alleen `lines:`.** ID's in tientallen, `.010` als eerste. Nooit hernummeren.
5. **Schrijf in je `note:` waar je gekeken hebt, nooit wat je geteld hebt.** Een notitie die een telling bevriest, is over drie dagen een instructie die de volgende schrijver verkeerd stuurt — dat is tien keer gebeurd in één nacht van act 1. Voor getallen: **draai `python Eclipse/Tools/check_spoken_numbers.py`** en verwijs naar de tool.
6. **Controleer je scène tegen §18.9 vóór je hem indient.** De criticus is een poort, geen redacteur.
7. **Kom je een canon-conflict tegen**, dan escaleer je naar story-architect. Jij ziet één missie; ik zie er 42.

### Volgorde waarin de missies vrijgegeven worden

**M2.1 gaat eerst en alleen** — hij is de eerste scène van een nieuwe planeet, een nieuw register en een nieuwe basis, en zijn ervaring hoort in de andere acht te landen voordat die beginnen.

Daarna mogen **M2.2, M2.3 en M2.5 parallel**. **M2.4 wacht op M2.3** (Kaya moet aangenomen zijn voordat ze vliegt, en `Story.Char.KayaJoined` wordt in M2.3.S99 gezet). **M2.6 en M2.7 wachten op M2.4 en M2.5** — die twee dragen de ketens waar de Siege en de identiteitsclimax op betalen (Torren, Whisper-contact). **M2.8 en M2.9 gaan als laatste**, want M2.9 leest zes vlaggen uit vijf eerdere missies.

**Parallel schrijven aan beide kanten van een ketting is hoe continuïteit breekt** — dat is act 1's les, woordelijk.

---

## 12. Definition of done — act 2, laag L1

- [x] Het aantal missies is uit de bron afgeleid en niet aangenomen: **negen** (§2.9 + `11_missions.md` §11.1), met de herleiding erbij (§3)
- [x] Locatieregister, met per string de canon-bron waaruit de woorden komen (§7)
- [x] Vlaggenregister **met een ingevulde *gelezen door*-kolom**, inclusief de act-1-vlaggen die act 2 leest — met adres, niet met "act 2" (§6)
- [x] Nummerregister, met een regel die de kruisakte-botsing structureel onmogelijk maakt in plaats van hem telkens opnieuw te controleren (§7)
- [x] Aanrakingsinventaris met een **vorm-eis per rij**, en één gepolijste in de hele akte (§4)
- [x] De act-1-draden die hier betaald worden, met adres: T2 volledig, AR-5's tweede persoon, `CustodianKey`, `OutsideQuery`, het pact, de brieven, de muur (§4, §5, §6)
- [x] Alle canon-conflicten gemeld in plaats van stil opgelost (§9); geen verzonnen personage, plaats of factie
- [x] `RULINGS_L1.md` niet aangeraakt; geen tweede rulings-opslagplek aangemaakt; benodigde rulings als aanvraag in §9
- [x] Beat-sheets **M2.1 t/m M2.5** geleverd
- [ ] Beat-sheets **M2.6 t/m M2.9** — **openstaand.** Adressen liggen vast in §4/§5/§6; wat ontbreekt is de scèneopbouw
- [ ] `SCRIPT_FORMAT`-valide `.yaml`-stubs op schijf — **openstaand, en geblokkeerd door C-A2-5**: de eerste act-2-`.yaml` maakt de validator blind of rood tot het vlaggenregister per akte geladen wordt
- [ ] Owner-vragen Q-A2-1 t/m Q-A2-6 beantwoord — **openstaand**
- [ ] `validate_script.py --no-voice` en `check_spoken_numbers.py` gedraaid — **niet gedaan, geen shell deze ronde.** Dit is de vijfde L1-ronde op rij die blind sluit en het hoort op de statuskaart

### Wat een volgende ronde als eerste doet

1. **RQ-2 laten landen** (vlaggen- en locatieregister per akte in `validate_script.py`). Alles daarna is goedkoper en niets ervoor is meetbaar.
2. **`BEATS_M2.6.md`** — de Siege. Het is de grootste missie van de akte, het scharnier van de spine, en drie andere beat-sheets betalen erop.
3. **`BEATS_M2.7.md`** — de twist-2-climax. Het is de enige plek in de campagne waar een act-1-plantgroep volledig sluit, en de enige keuze met een lezer in act 4.
4. Dan M2.9, dan M2.8. **M2.8 als laatste met opzet**: hij is de kortste, hij leest het minst, en hij is de enige die geen enkele wending draagt.
