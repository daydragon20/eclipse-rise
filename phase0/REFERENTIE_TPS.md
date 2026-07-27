# REFERENTIE — de basis van een third-person shooter

*Geschreven 27-07-2026 op owner-opdracht, vóór er nog één regel code aan de
basis verandert. Reden: tot nu toe reageerde ik op losse symptoommeldingen, en
dan komt het volgende gat pas boven als de owner er weer tegenaan loopt.*

## Hoe dit document gelezen moet worden

Per onderdeel staan er drie dingen:

- **REFERENTIE** — wat Borderlands / Gears / The Division / Destiny doen, en
  waaróm. Dit is een beschrijving van het MECHANISME.
- **WIJ NU** — wat er op dit moment in de code staat, met het getal erbij en de
  vindplaats. Alles in deze kolom is nagekeken of gemeten, niet herinnerd.
- **KEUZE OF GAT** — het enige oordeel dat dit document velt.

### Wat hier NIET staat, en waarom dat opzet is

**Geen verzonnen getallen uit die spellen.** Boomlengtes, FOV-waarden en
blendtijden van Borderlands of The Division zijn niet gepubliceerd. Ik kan het
mechanisme beschrijven — dat is waarneembaar en breed gedocumenteerd — maar niet
"Gears gebruikt 250 cm". Waar ik geen bron heb staat dat er, en dan is ONS getal
het uitgangspunt en de referentie alleen de RICHTING. De projectregel is: research
is opschrijven wat je overneemt en waarom, niet zelf een getal verzinnen.

**Waar een getal ontbreekt, staat een meetopdracht in plaats van een gok.**

---

## 1. CAMERA

### REFERENTIE

De derde-persoonscamera hangt aan een arm achter en boven de schouder, met een
**laterale offset** zodat het personage niet het midden van het beeld — en dus
niet je vizierlijn — blokkeert. Dat is de reden dat de offset bestaat; hij is
geen smaakkeuze maar maakt ruimte voor het kruis.

Drie dingen die de referentie consequent doet:

1. **De camera volgt met vertraging, maar het personage schuift niet uit
   positie.** De lag zit op de POSITIE van de armbasis, en hij is begrensd. Zonder
   grens groeit de achterstand met je snelheid en verschuift je personage in beeld
   naarmate je harder loopt — precies de klacht "bij stilstand plakt het wapen
   tegen mijn lichaam, tijdens rennen gaat het naar de zijkant".
2. **Versnellen en remmen mogen de camera even laten achterlopen** — dat is wat
   snelheid voelbaar maakt — maar het herstel is snel en de uitslag klein.
3. **De hoogte-offset is groter dan de laterale offset.** Je kijkt over de
   schouder, niet erlangs.

Gears legde de norm vast (dichtbij, hoog, sterk naar één schouder), The Division
zit verder weg, Borderlands zit dichter op het personage met een bredere FOV.

### WIJ NU

| | Waarde | Vindplaats |
|---|---|---|
| Armlengte derde persoon | **300 cm** | `EclipseCharacterTypes.h:294` |
| FOV derde persoon | **80°** | `EclipseCharacterTypes.h:289` |
| Socket-offset | **(0, 55, 65)** — 55 cm naar rechts, 65 omhoog | `EclipseCharacter.cpp:92` |
| Camera-lagsnelheid | **12** | `EclipseCharacterTypes.h:302` |
| Klem op de achterstand | **6 cm** | `EclipseCharacterTypes.h:326` |
| Gemeten camera→pawn | **311,8 cm** stilstand → **317,6 cm** op snelheid | aanlooptest 27-07 |
| Gemeten schijnbare hoogte | **31,52°** stilstand → **30,97°** op snelheid (1,76% spreiding) | idem |

### KEUZE OF GAT

- **De klem werkt en is een KEUZE.** 5,8 cm achterstand op volle snelheid, tegen
  een klem van 6. Zonder die klem zou de achterstand `snelheid / 12` zijn — 35 cm
  bij rennen. Dat is eerder gerepareerd en het houdt.
- **De laterale offset van 55 cm is een GAT in zijn onderbouwing.** Hij staat als
  vast getal naast een armlengte van 300. Wat de referentie doet is de offset
  koppelen aan de armlengte, want de offset bepaalt hoeveel graden het personage
  uit het midden staat — en dat verandert zodra de arm korter wordt. Zie hoofdstuk
  2: bij het mikken krimpt onze arm wél en blijft de offset staan.
- **"Schuift mijn personage naar de zijkant als ik ren" is nog NIET gemeten — en
  de meting die ernaar lijkt te kijken KAN het niet vinden.** De opnameronde drukt
  per moment `scherm=(x,y)` af, en dat leek de meetopdracht al te dekken. Nagemeten
  over alle negen momenten van de ronde van 08:49 — stilstand, rennen, draaien over
  118° en 238°, herladen:

  ```
  opname 1..9:  scherm x=500  y=525   afstand=312cm     (negen keer identiek)
  ```

  **Een waarde die onder geen enkele omstandigheid beweegt, meet niet wat ze
  belooft.** De oorzaak is constructie, geen toeval: deze regel projecteert
  `Body->GetActorLocation()`, en de camera hangt daar via de spring arm star aan
  vast. Die projectie is dus per definitie constant, hoe hard je ook loopt. Dat is
  dezelfde vorm als de rest van deze dag — een controle die niet rood kán worden.

  De 500 is trouwens wel informatief: op een frame van 1280 ligt het midden op 640,
  dus het personage staat **140 px links van het midden**. Dat is de socket-offset
  van 55 cm naar rechts, en dat klopt met de bedoeling.

  **DE JUISTE MEETOPDRACHT is de horizontale positie van het SILHOUET, niet van de
  actor.** De ronde berekent al een silhouethoogte in pixels (`[PLAYSHOT n
  SILHOUET] 482 px hoog`), dus er is al een silhouetbepaling; die moet ook zijn
  x-midden afgeven. Dat meet wél wat de owner ziet, want een personage dat
  overhelt of achterblijft verschuift met zijn silhouet en niet met zijn
  actor-oorsprong.

---

## 2. MIKKEN — het zwaarste punt

### REFERENTIE

Wat er gebeurt als je de mikknop indrukt, in alle vier de referenties:

1. **De camera trekt in en de FOV vernauwt.** Beide, en tegelijk. De FOV-vernauwing
   levert het vergrotende effect; de kortere arm zet je schouder in beeld.
2. **De camera wisselt of versterkt de schouderoffset**, zodat het vizier vrij komt
   te liggen van het personage.
3. **De overgang is kort** — een fractie van een seconde — en heeft een curve, geen
   lineaire lerp. Te traag voelt als stroop, direct voelt als een knip.
4. **DE LAG GAAT ER BIJNA HELEMAAL UIT.** Dit is het punt. Camera-lag is precies
   het mechanisme dat een vizier laat zwemmen: het kruis staat vast in het midden
   van het scherm, maar de camera zelf loopt achter op het personage, dus de
   WERELD schuift onder je kruis door. Bij het lopen is dat gewicht en dus prettig;
   bij het richten is het onbruikbaar. Elke referentie zet de lag bij ADS op nul of
   vrijwel nul, en dát is de reden dat mikken daar "vastklikt".

**Het vizier staat stil omdat de camera stil staat.** Niet omdat het kruis extra
verankerd is — het kruis stond altijd al in het midden.

### WIJ NU

```cpp
if (bAiming)
{
    TargetArmLength *= 0.55f;   // 300 -> 165 cm
    TargetFOV       *= 0.80f;   // 80  -> 64 graden
}
```
`EclipseCharacter.cpp`, `RefreshCameraTargets()`

- Er is **geen schouderwissel** en de socket-offset verandert **niet**: hij blijft
  (0, 55, 65).
- Er is **geen aparte blendtijd** — de blend loopt via de bestaande camera-blend.
- **De camera-lag blijft AAN tijdens het mikken.** In `RefreshCameraTargets` staat
  niets over lag, en de enige plek die `bEnableCameraLag` zet doet dat op
  `!bSuspended && TunedCameraLagSpeed > 0`. Mikken raakt geen van beide.
- Bewegen tijdens het mikken gaat op **145 cm/s** (`AimSpeed`).

### KEUZE OF GAT

- **DE LAG DIE AAN BLIJFT TIJDENS HET MIKKEN IS HET GAT.** Dit is de beste
  kandidaat voor de owner-klacht "nu beweegt alles als ik mik en kan ik niet eens
  deftig kijken", en hij is te onderbouwen zonder te spelen: het kruis staat
  schermvast in het midden, de camera loopt achter op de pawn, dus de wereld
  schuift onder het kruis door zolang je beweegt. **Dit is punt 1 van de
  bouwvolgorde.**
- **De 0,55 en 0,80 zijn een KEUZE met een goede reden** (relatief, dus ADS werkt
  gelijk in eerste en derde persoon), maar **de laterale offset die NIET meekrimpt
  is een GAT**: bij een arm van 165 cm weegt 55 cm zijwaarts bijna twee keer zo
  zwaar in graden als bij 300 cm. Het personage kruipt bij het mikken dus juist
  verder naar het midden van je vizierlijn in plaats van eruit.
- **Er is geen gemeten blendtijd.** Meetopdracht: hoe lang duurt de overgang van
  300→165 en 80→64 nu feitelijk.
- **Hoe het kruis zich verhoudt tot waar de kogel gaat is NIET geverifieerd.** De
  hitscan vuurt vanaf een oorsprong met een richting; of dat exact door het
  schermmidden loopt is nooit gemeten. Meetopdracht, en die hoort vóór elke
  spreiding-cosmetica.

---

## 3. RICHTKRUIS

### REFERENTIE

- **Vanaf de heup**: een open kruis of stippenkrans die de werkelijke spreiding
  toont. Hij is groter dan bij ADS en dat is informatie, geen decoratie.
- **Tijdens het mikken**: strakker, kleiner, vaak een andere vorm (punt of stip).
- **Spreiding is zichtbaar**: het kruis opent bij bewegen, springen en aanhoudend
  vuren, en sluit weer als je stilstaat. Zo lees je je eigen nauwkeurigheid zonder
  een getal.
- **Treffer**: een korte markering over het kruis heen. **Kopschot**: dezelfde
  markering in een andere kleur of vorm, plus een eigen geluid. Beide zijn kort —
  ze bevestigen zonder je blik weg te trekken.
- Borderlands zet er schadegetallen bovenop; Gears en Division houden het bij de
  markering.

### WIJ NU

- **Er stond tot vandaag geen enkel richtkruis in het project.** Het enige
  voorkomen van het woord was `EMouseCursor::Crosshairs` — de VORM van de
  muisaanwijzer, en die staat in het veld uit. De startbat vroeg er wel al naar
  ("zie je een crosshair?"), en dat was dus nooit waar.
- Sinds vandaag: een `+` in tekst, grootte 18, schermmidden, `HitTestInvisible`,
  lichte koele tint. Eén vorm, **geen verschil tussen heup en mikken, geen
  spreiding**.
- De hitmarker bestond al: `+` of een variant bij kopschot, kort zichtbaar.

### KEUZE OF GAT

- **De hitmarker zat op een kruis dat niet bestond** — dat was een echt gat en het
  is nu gesloten.
- **DAT HET KRUIS ER IS, IS DOOR MIJ NIET VISUEEL BEVESTIGD, en de owner ziet het
  niet.** Ik heb het met een groene suite afgevinkt en "klaar" genoemd. Dat was te
  sterk. De opnameronde slaat de HUD-laag bewust over (`IsDebugHudAllowed()` is
  false in een `-EclipseShot`-ronde), dus het beeldbewijs kón er niet zijn en ik
  had dat moeten melden in plaats van eromheen te praten. **Dit is punt 2 van de
  bouwvolgorde, en het is niet af tot het op een frame staat.**
- **Één vorm zonder spreiding is voorlopig een KEUZE** — spreiding tonen die er
  mechanisch niet is zou liegen. Maar het is een gat zodra spreiding bestaat.

### Eerste verdenking bij "ik zie het kruis niet", te controleren vóór er iets
verandert

Het kruis hangt in `UEclipseMissionHudWidget` — dezelfde widget als de F3-gids en
de munitieteller. Als die widget in een `-game`-run niet gemonteerd is, zijn het
kruis, de gids EN de teller alle drie weg. Dat verklaart owner-punt 2 en punt 6
met **één** oorzaak in plaats van twee. Het log zegt het letterlijk:
`Mission mode: debug HUD mounted/…`. **Dat is het eerste dat ik nakijk.**

Tweede verdenking, even goedkoop: de owner startte via `SPEEL_ECLIPSE.bat`, die
de bestaande binaries gebruikt. Draaide die start vóór mijn build klaar was, dan
speelde hij oude code en kán het kruis er niet zijn.

---

## 4. WAPEN IN BEELD

### REFERENTIE

- **Stilstand**: het wapen hangt laag of tegen het lichaam, in beide handen,
  duidelijk zichtbaar in silhouet.
- **Lopen**: meebewegend maar rustig; het wapen blijft leesbaar.
- **Sprinten**: het wapen zakt of kantelt weg — dit is de universele visuele taal
  voor "ik kan nu niet schieten", en het maakt tegelijk het beeld vrij.
- **Wapenwissel**: een zichtbare animatie van één à twee seconden — wegsteken,
  pakken, opbrengen. De wissel is nooit instant, want de KOSTEN van wisselen zijn
  de hele reden dat een tweede wapen een keuze is.

### WIJ NU

> **HERSCHREVEN 27-07 NA EEN CORRECTIE VAN DE OWNER.** Hier stond "er hangt geen
> wapenmesh — er is alleen niets in de handen". **Dat was onjuist.** Ik had het
> afgeleid uit codelezing (geen `AttachToComponent` in de karakterlaag behalve de
> kop-hitbox) zonder naar een frame te kijken. Dat is exact de vorm die ik vandaag
> vier keer bij mezelf en bij anderen heb aangewezen: de conclusie klopte met de
> code en niet met de werkelijkheid. In een document dat de MAATSTAF is weegt die
> fout zwaarder dan in een losse commit, want elke volgende keuze gaat erop staan.

- **ER HANGT WÉL EEN WAPEN.** [GEZIEN — `HighresScreenshot00915`, en bevestigd op
  00909, 00910, 00914 en `ScreenShot00000`.] Een geweer, horizontaal voor de borst,
  in beide handen, met loop en greep herkenbaar.
- **Waarom de codezoektocht hem niet vond**: er is geen attachment omdat het wapen
  deel is van de karaktermesh of van de pose. Zoeken op `AttachToComponent` kón hem
  dus per definitie niet vinden.
- **Waarom hij op eerdere beelden niet opviel**: de toon-restyle geeft ELK
  materiaalslot dezelfde factietint (`TintLit`/`TintShade` per lichaam), dus het
  wapen heeft exact de kleur van het lichaam waar het tegenaan ligt. Het verdwijnt
  in het silhouet.
- **Bij stilstand ligt het wapen netjes tegen het lichaam** [GEZIEN — 00915]. Dat
  is de pose, en die is dus goed.
- De wapenlaag is compleet als DATA: `component=1 magazijn=30 munitie=30`, twee
  slots, RB wisselt, elk slot houdt zijn eigen magazijn. [uit code en log; de
  wissel is NIET visueel bevestigd.]

### KEUZE OF GAT

- **Het gat is niet "er is geen wapen" maar "het wapen is geen los object".**
  Dat verklaart drie waarnemingen met één oorzaak: er is geen attachment (klopt),
  er hángt een wapen (klopt ook), en de wapenwissel doet visueel niets — want er is
  niets om te wisselen zolang het wapen deel van het lichaam is.
- **Owner-punt 5 verandert daarmee van "bouw een wapenmesh" naar "haal het wapen
  UIT de mesh".** Dat is een andere en grotere klus. **Wat het kost, vóór er iets
  begint:** de wapensectie uit de skeletal mesh isoleren of verbergen (per
  materiaalslot of per botsectie), een los mesh-asset per wapenfamilie krijgen,
  een socket op `hand_r` maken die op dit Paragon-rig bestaat, en de bestaande
  wisselogica daaraan hangen. Elk van die vier is een eigen falsificeerbare stap.
  **Dit hoort opnieuw gewogen te worden tegen de rest van de volgorde en dat is een
  owner-keuze, geen mijne.**
- **Een goedkopere tussenstap die het meeste oplost**: het wapen een EIGEN tint
  geven in plaats van de factietint van het lichaam. Dan is het leesbaar in
  silhouet zonder dat er iets uit de mesh hoeft. Dat lost "ik zie mijn wapen niet"
  op, maar niet "de wissel doet niets".

---

## 5. KOGELS EN TREFFERS

### REFERENTIE

- **Tracers**: niet elke kogel, maar genoeg om de baan te lezen. Ze vertrekken uit
  de loop, niet uit de camera — anders klopt de lijn niet met wat je ziet.
- **Op het inslagpunt**: een korte vonk of stofpluim mét een decal dat blijft
  liggen. De vonk zegt "nu", het gat zegt "hier".
- **Materiaalafhankelijk**: metaal vonkt, beton stuift, dat is dezelfde
  informatie als het oppervlaktegeluid dat wij al hebben.
- **Op de vijand**: een flinch of een kort oplichten. Borderlands zet er
  schadegetallen bij; Gears en Division doen het met impact en flinch.
- Zonder dit weet de speler niet of hij mist of dat de vijand veel leven heeft —
  letterlijk de owner-formulering.

### WIJ NU

> **OOK HERZIEN NA DE OWNER-CORRECTIE**, met de frames ernaast in plaats van
> alleen de code. Eén uitspraak hier was te absoluut.

- **Geluid bestaat**: `Cue_SFX_Impact_BulletMetal_01`, met oppervlaktetypes onder
  de voeten al geïmplementeerd (beton/metaal via physical materials en een trace).
  [uit code — NIET hoorbaar te verifiëren in een opnameronde.]
- **ER IS WÉL IETS ZICHTBAARS BIJ HET VUREN: hulzen.** [GEZIEN —
  `HighresScreenshot00911`, de opname "lopend en vurend": kleine gele objecten die
  uit het wapen wegvliegen.] Hier stond "visueel bestaat er niets", en dat klopte
  niet. Het is bovendien de enige reden dat ik op dat frame kon vaststellen DAT er
  geschoten werd.
- **Geen tracer zichtbaar** [GEZIEN op 00911 — geen baan tussen wapen en doel].
- **Geen inslag zichtbaar** — maar dit is **NIET geverifieerd**: op geen enkele
  opname staat een doel dat op dat moment geraakt wordt, dus het frame kan het
  verschil tussen "geen inslag-VFX" en "geen inslag in beeld" niet maken. Dit
  blijft dus een uitspraak uit codelezing, en zo staat hij hier.
- **Geen flinch en geen schadegetal** — eveneens uit code, niet visueel bevestigd
  om dezelfde reden.

> **HERZIEN 27-07 — DE OORZAAK ZIT DIEPER DAN DE VFX.** Nagekeken waar een inslag
> in de code landt, en het antwoord is: nergens. `EclipseHitscanWeaponComponent`
> traceert op `ECC_Pawn` en gooit daarna alles weg wat geen personage is
> (`if (HitCharacter == nullptr) return false;`). Een schot in een muur, een krat
> of de grond raakt dus wél iets en wordt vervolgens **stil verworpen** — geen
> feit, geen geluid, geen decal.
>
> Punt 4 is daarmee niet "voeg inslag-VFX toe" maar **"geef een schot dat de
> wereld raakt überhaupt een uitkomst"**. Een particle op het huidige pad blijft
> bij elke MIS onzichtbaar, en missen is juist het geval waarover de klacht gaat.
>
> Volgorde, elk apart falsifieerbaar: (1) het niet-personage-geval niet meer
> weggooien maar een eigen uitkomst geven met ImpactPoint, normaal en physical
> material; (2) het bestaande oppervlaktegeluid eraan hangen — dat systeem bestaat
> al voor voetstappen, dus aansluiten en niet bouwen; (3) pas dan het zichtbare.
- De hitmarker is de enige andere visuele terugkoppeling, en die zat tot vandaag op
  een kruis dat niet bestond.

### KEUZE OF GAT

- **Een GAT.** De eigen gevechts-audit noemde het al: vier van de vijf omissies
  gaan over feedback.
- De owner heeft een Muzzle Flash (Niagara) in zijn Fab-bibliotheek staan en
  aangeboden die op te halen. **Dat is een owner-klik en staat in het
  kliklijstje.** De inslag zelf hoeft daar niet op te wachten: een decal plus een
  korte particle kan uit wat er al is.

---

## Wat dit document voor de bouwvolgorde betekent

De owner-volgorde blijft leidend. Wat de referentie eraan toevoegt:

1. **Mikken** — de camera-lag die aan blijft is de concrete, aanwijsbare oorzaak.
   Daar begint het, en de laterale offset die niet meekrimpt hoort in dezelfde
   stap.
2. **Richtkruis** — eerst uitzoeken of de HUD-widget in een `-game`-run überhaupt
   gemonteerd is. Dat verklaart mogelijk óók punt 6.
3. **Camera** — meet de SCHERMPOSITIE. Elke bestaande meting gaat over grootte en
   afstand; de klacht gaat over positie.
4. **Kogels zichtbaar** — decal plus korte particle uit bestaand materiaal; de
   muzzle flash is een owner-klik.
5. **Wapenwissel** — geen aparte post: dit is "wapen in beeld".
6. **De gids** — niet nóg een toets. Uitzoeken waaróm er niets aankomt; zie de
   HUD-verdenking bij punt 2.

## De regel die dit document zichzelf oplegt (owner, 27-07)

**Kijk naar het beeld VOORDAT je een hoofdstuk over zichtbaarheid schrijft.**

De eerste versie van hoofdstuk 4 beweerde dat er geen wapen in beeld hing. Dat was
uit codelezing afgeleid en het was onjuist — de owner wees het aan op vier van mijn
eigen opnames. In een los commentaar is dat een fout; in de maatstaf stuurt het
elke volgende keuze de verkeerde kant op, want punt 5 verandert erdoor van "bouw
een wapenmesh" naar "haal het wapen uit de mesh".

Daarom draagt elke uitspraak over zichtbaarheid vanaf nu een merkteken:

- **[GEZIEN — bestandsnaam]** — met eigen ogen op dat frame vastgesteld.
- **[uit code, niet visueel bevestigd]** — gelezen, niet gezien. Mag nooit als
  grond dienen voor een bouwbeslissing zonder eerst een frame.

En waar een frame het verschil NIET kan maken (bijvoorbeeld: geen inslag-VFX
versus geen doel in beeld), staat dat er expliciet bij in plaats van dat de
afwezigheid als bewijs wordt gebruikt.

## Wat ik in dit document NIET heb geverifieerd

- Geen enkel getal uit Borderlands, Gears, The Division of Destiny. De mechanismen
  zijn beschreven, de getallen niet — die zijn niet gepubliceerd en ik verzin ze
  niet.
- De verdenking dat de HUD-widget niet gemonteerd is, is een HYPOTHESE uit
  codelezing. Niet gemeten.
- Dat de camera-lag de oorzaak is van "alles beweegt als ik mik" is een redenering
  uit de code (lag staat aan, kruis staat schermvast), niet uit een meting op een
  frame. Het is de eerste hypothese, niet de conclusie.
