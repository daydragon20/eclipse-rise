# 15.8 Dressing-iteratie 2 — licht + vloer (art-review 25-07, prioriteit 1)

*Implementatie-spec, geschreven 25-07 terwijl het build-slot dicht was. Uitvoeren NA de landing
van de [Art]-dressing-commit (EclipseGrayboxBuilder.cpp is tot dan bevroren onder review-GO).
Bron-verdicten: art-review shotronde 00056-00062 (samenvatting in progress_data.js, taakregel
"15.8-dressing-ronde 1").*

## Architectuur-anker (bepaalt de hele aanpak)

Het district rendert overwegend **unlit** (`M_EclipseToon`, banden in-shader via `LightDir` =
`SunRotation` [EclipseGrayboxBuilder.cpp:243]; SM5-devbox-limiet + bewuste stijlkeuze — zie de
lit-A/B-uitkomst: unlit blijft default op command-afstand). Consequenties:

- **Echte lichten/contactschaduwen bereiken de meeste oppervlakken niet.** De review-wens
  "licht-pools onder lampposts + contactschaduwen" moet dus in het toon-regime zelf:
  1. **Lamp-pools** = luminantie-decals (no-collision planes, patroon van de bestaande
     decals ~r580-620): warme natrium-discs (hue uit palet, alleen luminantie omhoog) onder de
     8 sodium-lamps (posities: `Lamps[]` ~r995 + crossing-paar r1057). Generator:
     `Tools/generate_decals.py` uitbreiden met een radial-falloff-disc.
  2. **Contactschaduw-vervanger** = blob-shadow-decals: donkere zachte discs (luminantie ×0,4,
     geen hue) onder de dressing-masses (boulders, containers, machine-banks, bunker) — leest
     in cel-stijl als bewuste grounding, kost geen VSM-budget (12.4).
- **De vloer is de uitzondering**: grootste niet-toon-oppervlak van het spel (fotoreal
  betonplaat, leest vol-daglicht op een schemerlucht — de kern van het "stickers"-probleem).
  Fix: vloer door de toon-master — world-aligned albedo (bestaand UVMode-0-pad), dusk-tint
  één stap onder Wall_, gemeten gain (exposure-invariantie-les: gain = 1/lineair-gemiddelde,
  klem 2.5). Let op de DecoPlaza/decals/lane-markering: die liggen óp de vloer en moeten hun
  waarde-hiërarchie houden (DecoLine onderaan de geel-hiërarchie, r155-157).

## Werkvolgorde (één iteratie, één shotronde per stap-cluster)

1. **Vloer → toon-master** (grootste winst, alle 7 cams): tint + gemeten gain; daarna
   shotronde en pas verder als de dusk-grade staat (de auto-exposure-les van 22-07 geldt).
2. **Lamp-pools + blob-shadows** (decal-tier, deterministisch, no-collision).
3. **Coördinaat-nudges uit beide reviews** (allemaal klein):
   - trim-band Z 350→365 (poster-clash, code-review #2);
   - buttresses flush op de muur (-9890→-9900 / 9890→9900, #4) én dieper/donkerder
     (art-review: silhouet-offset + één waardestap onder de band — anders onzichtbaar);
   - tread-pad 2 y-scale 2.0→1.7 (lampvoet-doorboring, #5);
   - dock: ramp-naad sluiten op de plinths (zweefspleet, art-review) + boulder-clip
     Wall-voet (00061) — bedding in de grond, niet door het muurvlak;
   - boulder-stain 30-40% lichter (leest nu als silhouet-zwart);
   - machine-face SciFi10_6 2-3× opschalen, body donker houden, emissive selectief;
   - magenta-verificatie Dominion-container (mix staat al op 0.45 in de gecommitte ronde —
     alleen shot-check).
4. **Graybox-restanten in gedresste frames** (stijl-wet-hygiëne): geel barrier-blok (cam 1),
   grijze vorken (cam 7) — toon-MID of verplaatsen uit het review-frame.
5. **Shotronde + art-review** (vaste cams; **eerste PNG = warm-up-offerframe, overslaan** —
   rig-comment 15.9) + stub-verwijdering pack-slim (na cold-DDC-check; ASSET_CLEANUP §10).

## Wat bewust NIET in deze iteratie

Lit-toon-migratie van het district (blijft A/B-uitkomst: unlit default; lit wint pas bij
interieurs/dag/characters — 15.7-keuze owner staat open) · civilian-wiring (eigen [Art]-ronde
na de import, figurentabel ~r797) · district-verticaliteit/backdrop-laag (art-review nice-to-have,
03.3-lijst) · alles wat P2-08/RTX-werk is (Lumen/Nanite/VSM-stack).

## Definition of done

Shotronde waarin (a) de vloer in het schemerpalet zit op alle cams, (b) elke dressing-mass
gegrond leest (pool/blob of bedding), (c) de zes nudges geverifieerd zijn, en (d) de
art-reviewer de ÉÉN-STIJL-WET-vraag ("verraadt iets zijn bron?") met nee beantwoordt voor
de eerder geflagde items. Daarna pas de zwakste-schakel-lijst vernieuwen.
