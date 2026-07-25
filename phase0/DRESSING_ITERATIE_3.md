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
| 3 | **Zet de lamp aan** | klein nieuw | De kop meet 0.0997 lin terwijl zijn pool 0.3867 is: de bron is **3.9× donkerder dan zijn eigen licht**. De `GlowPlane` (~r1220) staat met de lamp-yaw mee en dus vanaf de speelcamera's op zijn kant → vervang door kruis-billboard/tweezijdige quad, of zet de Glow-MID `(2.2,1.0,0.3)` op de bulb-slot. Doel: kop is het helderste object (≥1.0 lin) zodat bloom 0.45 een halo geeft. Dit alleen converteert elke pool van "geschilderde schijf" naar "licht". |
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
