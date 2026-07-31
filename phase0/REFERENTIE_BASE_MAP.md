# REFERENTIE — BASE en MAP, de twee andere hoogtes

*Maatstaf vóór code (owner-regel 27-07). Dit document zegt wat er op het scherm
hoort te staan en waaróm, zodat er iets is om tegen af te keuren. Zonder maatstaf
wordt dit twintig props in een lege hoek, precies wat `20_world_dressing_standard.md`
verbiedt.*

*Elke claim is gelabeld: **[SPEC]** = staat in de bijbel · **[GEMETEN]** = uit de code
gelezen op 31-07 · **[OPEN]** = nog niet beslist, hoort bij de owner.*

---

## 0. Waarom dit document bestaat

`EXECUTION_PLAN.md` §1b mat **boots** en vond daar één blokkade: de schermlaag zit
achter `IsDebugHudAllowed()` en staat dus per constructie niet op een opname. §1c mat
**base** en **map** en vond iets anders:

- **[GEMETEN]** Geen van beide zit achter de debug-poort — nul treffers op
  `IsDebugHudAllowed()` in `EclipseBaseHubWidget.cpp` en `EclipseStrategyMapWidget.cpp`.
  Die blokkade is uniek voor boots.
- **[GEMETEN]** Maar het zijn allebei **kale tekstlijsten**. Functioneel bedraad,
  visueel niet begonnen.

Boots had een *poort* nodig — de data was er, hij was onzichtbaar. Base en map hebben
een *maatstaf* nodig, want er is niets om tegen af te keuren. Dat is dit document.

---

## 1. MAP — de strategische laag

### 1.1 Wat de spec vraagt

**[SPEC — `03_world_design.md` §3.1]** De Vantara Expanse is een **jump-lane-graaf**:
10 bespeelbare planeten en 14 niet-landbare strategische knopen (Gate Spires,
relaisstations, depotstations, asteroïdevelden, de Hub), verbonden door jump-lanes.

De vier kaartregels, en let op wat ze gemeen hebben:

1. **Alleen via lanes.** Geen lane, geen beweging.
2. **Spires poorten lanes.** Een vijandige Gate Spire blokkeert militair transit;
   smokkelaarsverkeer kan nog wel, tegen kosten en risico (Kaya's specialiteit).
3. **Controle is per regio, niet per planeet.** Elke planeet heeft 4–8 regio's.
4. **Afstand = tijd = risico.** Bevoorradingslijnen zijn echt; een doorgesneden lane
   wurgt een economie.

**Alle vier leunen op topologie.** Wie naast wie ligt, wat wat blokkeert, hoe ver iets
is. Dat is de hele strategische laag.

### 1.2 Wat er nu staat

**[GEMETEN — `EclipseStrategyMapWidget.cpp`, 129 regels]** Eén `UVerticalBox` met
`UTextBlock`-regels:

```
DISTRICT BOARD — Day 1
  regionId — PLAYER | garrison 3 | unrest 12
  regionId — DOMINION | garrison 7 | unrest 40
```

Enige visuele codering is regelkleur per eigenaar (groen/geel/rood). Het commentaar in
het bestand noemt dat zelf eerlijk *"debug-grade, still readable"* — dat is een
eerlijke zelfbeoordeling en geen verwijt.

### 1.3 Het gat, in één zin

**Er is geen topologie op het scherm.** Een platte lijst regio's kan geen van de vier
kaartregels tonen: je ziet niet wat naast wat ligt, dus niet welke lane er is, dus niet
wat een Spire blokkeert, dus niet wat afstand kost. De strategische laag is precies
het deel dat een lijst *niet* kan zijn.

Dit is een ander soort tekort dan "er missen wat velden". Er missen geen velden — er
mist een **vorm**.

**En let op waar het gat NIET zit.** **[GEMETEN]** De graaf bestaat wél als data:
`FEclipseRegionDefinition.ConnectedRegionIds` in `EclipseRegionGraphAsset.h`, met een
validator die symmetrie afdwingt. De widget leest alleen de verkeerde helft — de
*mutabele* `FEclipseRegionState` (vier velden, geen kanten) in plaats van de
*statische* definitie waar de kanten in zitten. Zie §4.1; dat maakt de eerste stap
aanzienlijk kleiner dan "bouw een strategische laag".

### 1.4 Wat er dus moet staan

Onderverdeeld naar wat het bewijst, niet naar wat het toont:

| Moet tonen | Omdat | Nu |
|---|---|---|
| **De graaf zelf** — knopen op posities, lanes als verbindingen | Kaartregel 1: zonder zichtbare lane is "je kunt hier niet heen" willekeur in plaats van geografie | ontbreekt |
| **Lane-status per verbinding** — open, geblokkeerd door Spire, smokkelroute | Kaartregel 2. Een geblokkeerde lane die er hetzelfde uitziet als een open lane, leert je niets | ontbreekt |
| **Regio's binnen een planeet**, met eigenaar, garnizoen, onrust | Kaartregel 3: liberatie is regio's omklappen, dus de regio is de speleenheid | staat er als tekst, zonder plaats |
| **Reistijd en risico per route** | Kaartregel 4: afstand = tijd = risico moet afleesbaar zijn vóór je kiest, niet erna | ontbreekt |
| **Dominion Response Tier** | De strategische tegenspeler (`09_ai_systems.md`); zonder dit is er geen zichtbare vijand op deze hoogte | ontbreekt |
| **Missie-aanbod als aanbod** — wat, waar, wat het kost, wat het oplevert | Anders is de kaart een rapport in plaats van een keuzescherm | staat er als tekstregels |
| **Kaartbereik naar Command-Center-niveau** — lokaal → cluster → hele Expanse | **[SPEC — `05_base_building.md` §5.3.1]** Het Command Center poort het bereik met opzet, zodat de complexiteit meegroeit met de speler | ontbreekt |

### 1.5 De niet-onderhandelbare eis

**Een lijst mag blijven bestaan náást de graaf, nooit ervoor in de plaats.** Tekst is
beter in exacte getallen; een graaf is beter in verhoudingen. Wie de lijst weggooit
verliest de precisie, wie de graaf weglaat heeft nog steeds geen strategische laag.

---

## 2. BASE — Hollow Point

### 2.1 Wat de spec vraagt

**[SPEC — `05_base_building.md` §5.2–5.4]**

- Hollow Point is een buiten gebruik geraakte geothermische onderhoudskluis. Act 1:
  één gang, een generator die hoest, 11 kooien, een gestolen kaarttafel.
- **Slots zijn schaars met opzet:** 4 → 8 → 12 → 16 over de campagne. Een volledige
  campagne bouwt ~80% van alle L3's, dus een bouwvolgorde ís een strategische
  identiteit.
- **Dertien faciliteiten**, elk met **drie niveaus**, elk niveau **zichtbaar anders**
  (nieuwe machines, licht, personeelsdichtheid).
- **Bemanning:** soldaten en specialisten toewijzen verhoogt de opbrengst. Rooster en
  basis grijpen in elkaar.
- **Bouwen kost strategische tijd** (uren tot dagen op de campagneklok). Engineers
  bekorten het. **Rush kost Credits** — geld tegen tijd, altijd beschikbaar, nooit
  gratis.
- **Faciliteiten kunnen beschadigd raken** en staan dan offline tot reparatie. De basis
  is een plek die je verdédigt, geen menu.
- **Energie is een band:** elke faciliteit heeft upkeep, de Power Plant levert +10/+25/+50.
- **Memorial Hall groeit vanzelf en kost niets.** *Verdriet is nooit een aankoop.*

### 2.2 Wat er nu staat

**[GEMETEN — `EclipseBaseHubWidget.cpp`, 442 regels]** Meer structuur dan de kaart: een
`UWidgetSwitcher` met vier tabs (Command / Workshop / Barracks / Memorial). Maar de
inhoud is dezelfde vorm — tekstregels in een verticale doos — en de kop is één lange
`printf`-regel:

```
BASE — Hollow Point · day %d · pick a mission below to start (walking is disabled here)  |  C %d  M %d  I %d
```

### 2.3 Het gat

De basis toont **wat je hebt**, niet **wat je aan het doen bent**. Dat verschil is
precies waar een basisscherm voor bestaat.

| Moet tonen | Omdat | Nu |
|---|---|---|
| **Het slotraster** — bezet, vrij, nog vergrendeld | Schaarste is de kern van de bouwvolgorde. Vrije slots die je niet ziet, bestaan niet | ontbreekt |
| **Bouw- en upgrade-ETA als voortgang** | Bouwen kost dagen; een ETA zonder voortgang is een belofte zonder bewijs | ontbreekt |
| **De rush-keuze mét prijs** | Geld tegen tijd is een terugkerend dilemma en hoort zichtbaar te zijn op het moment dat het speelt | ontbreekt |
| **Bemanning per faciliteit** | Medbay tegen Academy om hetzelfde personeel is een bewust dilemma (§5.3.2); onzichtbaar is het geen dilemma | ontbreekt |
| **Energiebalans** — opgeteld verbruik tegen opwekking | Je kunt jezelf in het donker bouwen. Dat hoort te blijken vóór het gebeurt | ontbreekt |
| **Voorraadplafonds** (Storehouse) | Grondstoffen die je stil verliest omdat de kast vol is, is de vervelendste vorm van straf | drie losse getallen in de kop |
| **Beschadigd/offline** | De basis is een plek die je verdedigt; schade die niet leest, leert niets van een aanval | ontbreekt |
| **De strategische klok** | Alles hierboven loopt op dagen. Zonder klok heeft geen enkele ETA betekenis | staat als `day %d` in de kop |

### 2.4 Twee dingen die uit de spec volgen en makkelijk sneuvelen

1. **Elk faciliteitsniveau moet er anders uitzien.** **[SPEC §5.3]** Dat is een
   art-eis, geen nice-to-have: de Act 1-kluis en het Act 4-HQ moeten onherkenbaar zijn
   als hetzelfde systeem. Een UI die alleen "Workshop L2" schrijft waar eerst
   "Workshop L1" stond, voldoet daar niet aan.
2. **De Memorial Hall is geen faciliteit als de rest.** Hij groeit vanzelf, kost niets
   en heeft geen upgradeknop. Wie hem in hetzelfde raster met dezelfde bouwknop zet,
   heeft de bedoeling gemist.

---

## 3. Wat dit document NIET beslist

**[OPEN — owner-punt O-6]** De **visuele behandeling** staat hier bewust niet in.
Alles hierboven gaat over *welke informatie* op het scherm hoort en waarom — dat ligt
vast in de bijbel en is dus van mij. Hóé het eruitziet hangt aan de stijlvraag:

- **Lezing A (Borderlands-lock blijft):** de kaart wordt een gestileerde graaf met
  inktlijnen, de basis een gestileerde doorsnede.
- **Lezing B (fotorealisme):** beide worden een ander soort scherm, en `15.5` moet
  eerst herzien.

**Dit document is in beide lezingen geldig.** De informatie-eisen veranderen niet van
de stijl; alleen de vormgeving. Daarom kan de datalaag eronder nú gebouwd worden
zonder op O-6 te wachten — en daarom moet de vormgeving er wél op wachten.

**[OPEN]** Ook niet beslist: of base en map dezelfde interactiegrammatica krijgen als
boots (Command Mode is hold-to-enter). Dat is een ontwerpvraag die pas zinvol is als
er iets staat om mee te interacteren.

---

## 4. Volgorde die hieruit volgt

Per bouwvolgorde 14.5 — dataschema, pure logica met tests, subsystem en events,
debug-UI, echte UI als laatste:

1. **De topologie bestáát al — de UI gooit hem weg.** *Ik schreef hier eerst dat het
   graaf-schema nergens bestond. Dat was fout, en het nameten kostte één grep.*

   **[GEMETEN]** `Strategy/EclipseRegionGraphAsset.h` definieert
   `FEclipseRegionDefinition` mét `ConnectedRegionIds` — ongerichte kanten, en de
   validator dwingt symmetrie af. Het commentaar erbij zegt het met zoveel woorden:
   *"GDD 3.1 rule 1: it is a graph, edges matter"*. `EclipseStrategyLogic.cpp` loopt
   die buren al af.

   **Het gat zit tussen de twee structuren.** De widget leest
   `Campaign->GetState().Regions`, en dat is `FEclipseRegionState`: **[GEMETEN]** vier
   velden — `RegionId`, `Owner`, `Unrest`, `GarrisonStrength`. **Geen kanten.** De
   kanten zitten in de *statische* definitie, en die raadpleegt de widget nooit. De
   kaart toont daarom geen topologie, niet omdat die ontbreekt, maar omdat de UI de
   helft leest die hem niet heeft.

   Dat maakt de eerste stap kleiner en zekerder dan hij leek: **de widget de
   graaf-asset naast de toestand laten lezen**. Falsifieerbaar zonder één pixel —
   *"welke regio's grenzen aan deze, en klopt dat met de asset"* is een testbare vraag,
   en een test die de buren uit de UI-laag opvraagt gaat vandaag rood.

   **Wat er wél nog niet is** (en dus echt gebouwd moet worden): lane-**status**
   (open / door een Spire geblokkeerd / smokkelroute), afstand of reistijd per kant, en
   de Dominion Response Tier. De kant bestaat; wat de kant kóst, niet.
2. **Dan de basistoestand als data** — slots, ETA's, bemanning, energiebalans,
   schade — met dezelfde toets: elke rij hierboven moet te beantwoorden zijn door een
   test vóór er een widget is.
3. **Dan pas beeld**, en dat wacht op O-6.

**De valkuil om te vermijden:** de huidige tekstlijsten mooier maken. Dat voelt als
vooruitgang en lost niets op — het gat is geen opmaak maar een ontbrekende vorm.
