# Dressing-iteratie 3 — de metering eerst, dan pas kleur

*Spec, 2026-07-25 na de art-review van iteratie 2 (shots 00064-00069, 00063 = warm-up).
Verdict iteratie 2: het decal-recept is BEWEZEN, maar de "stickers"-diagnose is slechts
~25% opgelost — en niet in de laag waar het om ging. Alle zes frames: AAA-ready = nee.*

## 0. De vondst die de hele vorige iteratie herschrijft

De review mat de frames terug naar authored waarden en vond dat het beeld de
palet-albedo **×1.79** rendert (vloer-mid authored 0.1198 → 0.2145 lineair in
frame). Daaruit volgen twee dingen die we niet wisten toen iteratie 2 gebouwd werd:

1. **De "dusk"-retint maakte de vloer 16% LICHTER, niet donkerder.**
   Oud Lit `(0.165,0.150,0.160)` = Rec709-lum 0.1539; nieuw `(0.166,0.180,0.209)` =
   **0.1791**. Het anker `x0.72 onder Wall_` was fout gekozen: het bindt de grond aan
   het *helderste grote oppervlak* van het district in plaats van aan de lucht.
2. **Zelfs een donkerdere waarde was weggemeten.** Met histogram-auto-exposure en een
   vloer die 60-80% van elk frame vult, is de schermwaarde van die vloer een **fixed
   point** — het histogram trekt het dominante oppervlak altijd naar midden-grijs.

**Conclusie: zolang de metering vrij loopt, is de vloerwaarde niet art-directable.**
Elke kleurcorrectie op een dominant oppervlak is dan een no-op. Dit is de reden dat
iteratie 2 zoveel losse verbeteringen kon landen met zo weinig leeswinst.

## 1. De ladder (volgorde is niet vrij — 1 is de voorwaarde voor 2-9)

| # | Wat | Soort | Concreet |
|---|---|---|---|
| 1 | **Pin de metering** | waarde | `AutoExposureBias -0.7 → -1.55` (Δ = −log2(1.79) = −0.84 EV) in `EclipseGrayboxBuilder.cpp` ~r1949, of `AutoExposureMinBrightness == MaxBrightness`. **Verificatie: hershoot alleen cam 7 en meet de vloer terug op 0.1198 ±0.01 lineair.** Niet blind district-breed doorvoeren — passes 20-27 waarschuwen daar terecht voor. Bijeffect: BldgA (0.560) en Cover (0.850) clippen niet meer, dus oxide stopt met naar zalm desatureren. |
| 2 | **Vloer twee stappen omlaag** | waarde | `Floor` Lit ×0.518 (=0.72²) → `(0.086,0.093,0.108)`; Shade → `(0.028,0.031,0.049)`. Mid 0.1198 → 0.0621, in frame ~1.9× de horizonlucht = schemerasfalt. `Wall_` blijft staan, zodat de grond/muur-stap eindelijk bestaat. TexGain 12.41/16.8 NIET aanraken. |
| 3 | **Zet de lamp aan** | **GELAND — zie §1c** | Emissieve bulb in de hoedmond op het Glow-palet; de lamp is nu het helderste object in cam 7 (rgb 255,238,44). Twee diagnoses onderweg weerlegd, zie §1b. |
| 4 | **Pool: 3 → 5 waardestappen** | derivatie | `PoolMid = 0.0621/0.72^5 = 0.3209` — exact dezelfde schermwaarde als nu, op de nieuwe donkere vloer. Lit `(0.876,0.398,0.119)`, opacity 0.70 en de Glow-hue-factor ×0.398 blijven allemaal geldig; alleen de comment-derivatie ("three banked value steps") moet mee. |
| 5 | **Blob herrekenen** | waarde | Op een vloer van 0.0621 zou de huidige ×0.4 bij opacity 0.85 uitkomen op 0.0304 — ónder de horizonlucht = silhouet-zwart, precies het defect waarvoor de boulder-stain al geflagd werd. Hou de kern op ~0.62×: vloertint **×0.55** bij opacity 0.85 → `BlobMid` Lit `(0.047,0.051,0.059)`, Shade `(0.015,0.017,0.027)`. **En: leid de skirt af van de HOOGTE, niet de footprint** — `footprint×1.25` geeft 76 units onder een 260 hoge container en verdwijnt vanaf ooghoogte (daarom was in 00065 geen enkele container-blob vindbaar). Voorstel `skirt = 0.35 × hoogte`. |
| 6 | **Grounding-dekking** | nieuw, klein | Blobs komen nu uit 4 plekken (rubble, machine-bodies, containers, bunker) = ~10% van de massa's; in het overzichtsframe zweeft de rest. `SpawnGroundDecal` toevoegen voor `CoverPoints[]`, de prop-tabel (vaten/kratten/barrières), lamppaal-voeten (~120-unit blob) en de poortpilaren. Patroon staat er al. |
| 7 | **Waardeplafond op dressing** | waarde + regel | **Te bankken regel: niets dat geen lichtbron is mag boven de pool-kernwaarde uitkomen.** Nu: `Prop_Barrier (0.26,0.27,0.30)` meet 0.7269 = **1.88× de pool-kern** → ×0.518 → `(0.135,0.140,0.155)`; tread-pad en hazardpad één stap onder de pool. |
| 8 | **Magenta/violet echt oplossen** | waarde | **De vorige hypothese was fout.** Mix 0.75→0.45 was de verkeerde variabele: `OxideShade (0.200,0.045,0.085)` is zélf authored op hue 344.5° met B ≈ 1.9× G, en `ColorSaturation 1.38` schreeuwt dat uit. Gemeten containerzijde: **hue 336.6°, sat 0.77-0.81** op drie losse patches. **Regel voor Kessara: elke donkere/shade-tint houdt B ≤ G.** `OxideShade → (0.200,0.062,0.038)` = hue 9.3° (BldgA deelt deze shade). `RubbleLit (0.095,0.084,0.101)` = hue 279° violet → `(0.098,0.088,0.078)`. Vat-tint idem. |
| 9 | ~~**Luchtperspectief via FogDensity**~~ | **GEPROBEERD EN VERWORPEN** | `FogDensity 0.006 → 0.02` is uitgevoerd, gemeten en teruggedraaid. Zie §1a. |
| 10 | **Outline-discipline** | nieuw (PP) | (a) grondvlakke dressing (pads, ramps, decals) uit de inkpass — anders wordt elk pad een omlijnde sticker (tread-pad 00064 meet 0.408 = helderder dan de pool-kern, mét gesloten outline = glasplaat); (b) lijndikte laten afnemen met afstand, want nu is elke prop op overzichtsafstand een dikomlijnde vorm op een vlakke plaat; (c) getrapte inklijnen nakijken (TSR-kwaliteit). |
| 11 | **`SM_AssetPlatform` eruit** | nieuw, klein | Zie §2 — blokkerend. Geschaalde kubus met dockrand is strikt beter. |
| 12 | **Liners: diepte of weg** | nieuw, klein | 5 units dik leest als losse fin (je ziet vanaf cam 3 het *einde* van de plaat); ≥40 units, of het paneelgraphic direct als albedo op de wandslab. |
| 13 | **Restanten + camera-dekking** | opruimen | Grijze vorken staan nog in 00067 én 00069. En: **de machine-faces zijn vanaf geen enkele gebankte camera zichtbaar** — cam 2 kijkt er vanaf de achterkant door een muur naar. Voeg een camera toe die de bank écht ziet, of accepteer dat die dressing nooit leest. |

## 1a. Uitgevoerd op 2026-07-25: stap 1 + 2 GELAND, stap 9 verworpen

**Stap 1 + 2 zijn gebouwd en gemeten (shots 00070-00076).** De metering-fix doet
precies wat hij moet doen: de vloer *reageert* nu op wat er geauthord staat.

| meting (cam 4) | vóór (00066) | ná (00073) |
|---|---|---|
| vloer voorgrond ÷ lucht horizon | 3,41× | **1,52×** |
| vloer voorgrond (lin) | 0.2026 | 0.0525 |
| lucht horizon (lin) | 0.0595 | 0.0346 |
| geclipt in de vloer | — | 0% (max 114) |

De vloer is 3,9× donkerder en niets is dichtgeknepen. De ratio landt op 1,52× waar
~1,9× het doel was — twee stappen (×0.518) is dus een fractie te ver. Dat is een
bewuste openstaande keuze, geen fout: **één stap (×0.72 → `(0.120,0.130,0.150)`)
zou op ~2,1× landen.** Kies dat pas ná stap 3 (de lamp), want een zichtbare bron
verandert hoe donker de vloer mág zijn.

**Stap 9 (FogDensity) is geprobeerd en teruggedraaid — de hypothese was fout, en
de fout was mijn meting.** Ik las "de grond wordt 2,08× lichter naar de horizon"
uit een probe-rechthoek in de horizonband die geclipte niet-vloer-pixels bevatte
(0,5% op 255). Een schone dieptescan met vijf rechthoeken van dichtbij naar ver
geeft 0.0489 / 0.0495 / 0.0553 / 0.0536 / 0.0523 lineair — **vlak binnen ~7%**.
`FogDensity` van 0.006 naar 0.02 verschoof die verhouding met 0,02× (niets), dus
de waarde staat terug op 0.006 mét de bevinding in het commentaar. Les voor
volgende rondes: **meet nooit een oppervlak in een band waar iets anders in staat**
— controleer altijd eerst `geclipt` en `max` van de probe-uitvoer.

Dat maakt de frame-probe (`Tools/measure_frame_values.py`, §4) meteen zijn eigen
rechtvaardiging: hij ving zowel de oorspronkelijke vloer-omkering als mijn eigen
verkeerde conclusie.

## 1d. Herziene ladder na de tweede art-review (shots 00092-00097) — LEES DIT EERST

De tweede review kalibreerde eerst zijn eigen meetvakken tegen de ontwerpbaseline
(reproduceerde 2.170× in 00069) en corrigeerde daarna drie dingen die hieronder in
§1c nog fout staan. **De review heeft voorrang op §1c.**

**1. Mijn pool-meting was fout: ring in plaats van kern.** Echte `pool_core` in
00097 = **0.2907**, dus de ratio is **6.25×** de vloer, niet 3.68×. De correctie
op de pool is daarmee **×0.35-0.40** (naar 0.10-0.12 lum), niet ×0.59 — en dat
getal heeft nu drie onafhankelijke bewijzen: pool÷bulb moet van 40% naar 14-16%;
de baanmarkering moet ≥2× haar asfalt houden; en 2.18 × vloer 0.0465 = 0.101.
De ontworpen 2.18× was dus gewoon goed.

**2. De metering-pin raakte ALLEEN de vloer.** Alles boven ~0.25 lum zit op de
shoulder van de tonemapper en gaf maar 20-45% van de verwachte daling terug:
vloer ÷4.21, maar barrière ÷1.11. Gevolg: **barrière ÷ vloer ging van 3.72× naar
14.1×** — het plafond zakte niet mee, dus dressing staat nu *verder* van zijn
ondergrond dan vóór de ingreep. Het histogram van de onderste beeldhelft is nu
bimodaal met een **lege middenband** (middentonen 66.5% → 9.3% in cam 7, 2.0% in
cam 4). Dat ís "stickers op een plaat", numeriek — het spiegelbeeld van het oude
"alles middengrijs"-defect. **Daarom is stap 7 (waardeplafond) nu #1, en `Wall_`
hoort erbij** (0.238-0.356 = 4.1-4.8× de vloer, grootste oppervlak in élk frame;
"Wall_ blijft staan" uit stap 2 was in retrospectief fout). Let op: authored→frame
is niet-lineair (2.4-2.7× op barrièreniveau, 0.75-0.92× op vloerniveau), dus
**×0.518 werkt daar niet** — zet een frame-doel en bisecteer met de probe.

**3. De vloer is AF — doe de stap terug naar ×0.72 NIET.** Mijn 1.52× was weer een
probe-artefact; schone luchtmetingen geven een bandbreedte van 1.51-2.32×, met het
~1.9×-doel er middenin. Naar ×0.72 zou pool÷vloer gratis van 6.25× naar 3.7×
brengen *zonder de pool aan te raken* — een cosmetische no-op die het echte defect
verbergt, en de derde instantie van precies de val die deze ronde al twee
terugdraaiingen kostte. Het echte vloerdefect is **vlakheid** (18% variatie over
55% van cam 4), niet het gemiddelde → nieuw item **2b: lokale vloervariatie**.

**Nieuwe, gemeten defecten die zwaarder wegen dan de rest van de oude ladder:**
- **De baanmarkering kantelt binnen de pool**: buiten 5.65× haar asfalt, binnen
  **0.79×** — de gele lijn is in de pool *donkerder* dan het wegdek, en wat je nog
  ziet is alleen zijn zwarte ink-outline. De sticker houdt het frame bijeen in
  plaats van het licht. Daarom moet de pool **als multiply/add over de ondergrond**
  composeren, niet eroverheen: ×0.40 alléén levert een koelere sticker.
- **Bloom is de facto uit**: 1.07× op 12px van een bulb met 61% geclipte pixels.
  En de inkpass zet een **zwarte rand** om de bulb (0.0223 < verre lucht 0.0270).
  Een sodiumlamp zonder halo mét zwarte rand is per definitie plastic. → stap 3
  splitst in **3b bloom/halo**, **3c bulb uit de inkpass**, **3d warme overdracht
  op verticalen** (mast ín de pool R/B 1.05 tegen 0.97 buiten = nul overdracht).
- **Nul contactverankering** in alle zes frames; de barrière heeft outline op zijn
  silhouet maar niets op het grondcontact, dus zijn onderkant lost op in het
  asfalt. → stap 6 (grounding) schuift naar de top-3.
- **Outline-regel herformuleren**: ink op silhouetten, ink UIT op grondvlakke
  dressing, ink NOOIT op emissives — en juist WEL (of een blob) op grondcontacten.
- **`SM_AssetPlatform` staat er twee keer** in 00093, met maatstreepjes en
  checker-schaalramp leesbaar op 1080p. Blokkerend en erger dan gedacht.
- **Magenta is een MID, geen shade**: hue 336.7° op val 0.51 met B/G = **3.17**
  (niet 1.9×). De regel "shade houdt B ≤ G" moet dus ook over de **midtinten**.
- **Cam 4 heeft een camera-probleem, geen dressing-probleem**: 45% lege lucht +
  45% lege vloer + 10% contentstrook kan nooit lezen. Kantel de pitch of zet een
  lamp in het nabije veld. **Cam 6 valt af als reviewframe** (placeholder-pop).
- Geparkeerd: **liners** (op geen enkele camera-afstand leesbaar als defect) en
  **FogDensity** (met de grond op 0.03-0.06 is er niets om op te tillen; pas
  hertesten nadat het plafond zakt).

**Stand "stickers"-diagnose: ~35% opgelost** (was 25%). Winst: zichtbare bron,
decal-recept houdt stand, vloer is art-directable, ladder is numeriek. Maar twee
van de vier resterende tells zijn deze ronde *verslechterd*.

**Referenties gebankt:** 00069 blijft de pool-recept-referentie; 00097 ernaast als
"bron zichtbaar, plafond nog niet gezakt" (het frame waartegen de plafond-fix
gemeten wordt); 00095 als dark-value-referentie voor commandoafstand.

## 1c. Stap 3 GELAND — en het stap-4-getal dat §1d corrigeert

**De bulb werkt.** De helderste pixel in cam 7 was in elke eerdere ronde de
neutrale witte barrière (rgb ~226,225,226). Nu is het de lamp: **rgb (255,238,44)**
op kophoogte, met 8,3% geclipte pixels in het bulb-vlak — sodium-oranje, rood
geklipt, dus bloom 0.45 heeft eindelijk iets om een halo van te maken. De bron van
het enige licht in het district is zichtbaar.

**Direct gevolg, gemeten in dezelfde shot: de pool moet mee.** De pool-MID hield
zijn absolute waarden terwijl de vloer twee stappen zakte, dus de verhouding liep
op van de ontworpen 2,18× naar **3,68×** (pool-kern 0.1781 ÷ vloer 0.0484). De pool
is nu relatief te heet ten opzichte van zijn eigen vloer. Dat is stap 4, en het is
nu een getal in plaats van een gevoel: **×0,59 om 2,18× te herstellen** — maar leg
eerst vast of 2,18× nog het juiste doel is nu er een *zichtbare* bron boven hangt,
want dat was de aanname waaronder die ratio is gekozen.

Ook nog steeds open en nu ook gemeten: de barrière staat op 0.3203 = **1,8× de
pool-kern** (stap 7, waardeplafond op niet-lichtgevende dressing).

## 1b. Stap 3: twee weerlegde diagnoses onderweg (bewaard zodat niemand ze herhaalt)

De review dacht dat de `GlowPlane` vanaf de gebankte camera's op zijn kant staat,
omdat hij op de lamp-yaw wordt gespawnd. Dat is gebouwd als kruis-billboard
(dezelfde plaat nog eens op `Yaw+90`), geshoot en gemeten:

- helderste pixel in cam 7: **225.3 → 215.6** — niet omhoog, en in *beide* shots is
  dat de neutrale witte barrière (rgb ~226,225,226), niet de lamp;
- geen enkele pixel ≥250 in beide frames;
- image-diff tegen de vorige cam-7-shot: alleen verspreide grain-samples, **nergens
  een nieuw helder cluster**.

Conclusie: **oriëntatie is niet de oorzaak**, en de wijziging is teruggedraaid
(twee keer zoveel actors voor nul meetbare winst is 12.4-kosten zonder baten).

**De sterkere hypothese**, uit het lezen van de aanroep: de glow-plaat wordt
gespawnd op `FVector(Post.X, Post.Y, 0.0f)` — **grondniveau** — terwijl de bulb die
hij voorstelt op paalhoogte hangt, ~0.6-0.9 m langs de arm. Tenzij
`GlowPlane.GlowPlane` die hoogte in zijn eigen geometrie bakt, ligt de emissive
quad dus in de straat *binnen de pool*. Dat verklaart in één keer zowel de donkere
kop als een deel van waarom de pool vlak leest. **Volgende stap: controleer eerst de
authored pivot/extent van die mesh (`Tools/blender/gen_street_props.py`), plaats hem
dan op de kop — en raak oriëntatie of helderheid niet aan.** `EmissiveScale` staat al
op 10 op de unlit-master met een >1 Glow-tint (2.2,1.0,0.3), dus helderheid is
vrijwel zeker niet het ontbrekende stuk.

Wél gehouden uit deze poging: `SpawnGen` slikte een ontbrekende generated mesh
**stil** (14.3.5-gat). Er komt nu één warning met het aantal overgeslagen
plaatsingen en de scripts die je moet draaien. Geverifieerd dat hij niet afgaat op
de huidige boom — alle generated meshes zijn aanwezig.

## 2. ÉÉN-STIJL-WET-overtredingen die nu blokkeren

1. **`SM_AssetPlatform` als dock-plinth** (~r1463-1468, zichtbaar onderaan 00065): dat is de marketplace asset-showcase-**draaischijf**, met radiale spaken, herkenbaar als precies wat het is, in een gebankt reviewframe. Dit is de vraag "verraadt het zijn bron?" letterlijk met ja beantwoord.
2. **Palet-drift buiten Kessara**: magenta container (hue 336°, 30% van 00065), violet-mauve boulders (hue 279°) en vaten. Palet is de enige kleur-autoriteit.
3. **Neon-cyaan tech-krat** in 00065 — dat is het palet van The Shroud. Eén planeet per frame.

Niet-blokkerend maar gemeld: de vloer *leest* fotografisch in een cel/ink-frame (4K-asfalt op mix 0.5, salt-and-pepper vooraan, vlakke mip-lavendel in het midden) — iteratie 3+ wil hand-geauthored grondkarakter, niet meer noise-dichtheid. Borden/posters zijn platte quads waarvan de emissive hard bij de quadrand stopt. **00068 valt af als referentieshot** omdat het gedomineerd wordt door een placeholder-pop.

## 3. Wat NIET herbouwd moet worden

Het decal-pad zelf is de juiste keuze op een unlit district, en het recept is bewezen:
falloff zonder quad-rand, het asfaltkorrel loopt herkenbaar door in het warme gebied,
de Z-stack warmt de baanmarkering mee, en de ratio landt exact op ontwerp (gemeten
2.25× links en 2.17× rechts tegen 2.18× ontworpen). **Gebankt als pool-recept-referentie:
`HighresScreenshot00069.png`** — niet als kwaliteitsreferentie, maar als de bar die
iteratie 3 moet vasthouden terwijl de vloer eronder wegzakt.

Op commandoafstand (00067) werken de pools al: kleine warme lichtpunten in de straat,
de enige warmte in dat frame. Dat is de winst van iteratie 2 en die blijft staan.

## 4. Meetmethode bankken (15.8 stap 5)

De review kon "te heet/te zwak" in getallen uitdrukken door de frames terug te meten
(sRGB-EOTF + Rec.709 op benoemde pixelrechthoeken). Maak daar een frame-probe van naast
`Eclipse/Tools/measure_albedo_gain.py`, zodat elke volgende ronde met getallen werkt in
plaats van met gevoel — en zodat een claim als "de vloer is nu donkerder" niet meer
ongemerkt het tegendeel kan zijn.
