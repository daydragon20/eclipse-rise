# Assets en plugins in wacht — staat klaar, nog niet in gebruik

*Alles hieronder staat al op deze pc en kan zonder downloaden aan. Het is bewust
nog niet aangezet of in `Eclipse/Content` gezet, met per item de reden en het
moment waarop het wél aan de beurt is.*

*Twee soorten, met een heel verschillende drempel:*
- **Assets** (§1-4, 8) — die komen in de repo en dus permanent in de
  git-historie. Criterium: gaat het volgende stuk werk dit gebruiken?
- **Engine-plugins** (§5) — die staan al bij Unreal geïnstalleerd; aanzetten is
  één vinkje en volledig terug te draaien. Kosten: een tragere editor, en bij
  experimentele plugins een instabieler project.

**Ophalen:**
```
powershell -File Tools\fab_doorgeven.ps1 -Alleen <MapNaam> -Kopieer
```
Zonder `-Kopieer` is het een proefronde. Meerdere tegelijk mag: `-Alleen A,B,C`.

**Laatst nagelopen:** 2026-07-26

---

## 1. Klaar om te gebruiken zodra de fase het vraagt

| Map | Grootte | Wat | Wanneer relevant |
|---|---|---|---|
| `MilitaryRanks1` | 128 MB | Insignes, rangonderscheidingen, decals | Fase 2, als de squad visuele identiteit krijgt. Nu nog graybox (GDD 15.0), dus zinloos |
| `Maxtree` | 844 MB | Planten met wind-controller | **Sylvaris** (Fase 3). Kessara is industrieel — geen begroeiing |
| `Lava_Material` | 280 MB | Één lavamateriaal + textures | **Vorn** of een vulkanische planeet (Fase 3) |
| `Snow_Material_Pack` | 97 MB | Sneeuw- en ijsmaterialen | Een koude planeet (Fase 3) |
| `Planet385CY` | 1,2 GB | **Geen planeet** — gerigde insecten/creatures | Als er niet-humanoide vijanden komen. Nu geen missieontwerp dat ze vraagt |
| `Northwood` | 7,4 GB | Environment-pack uit DemonstratingAnimationBlu | Als het bij een planeet past. Schijfruimte is geen bezwaar meer; het gaat wel permanent in de git-historie (`.git` is al 3,8 GB) |

## 2. Bewust nooit ophalen

| Map | Waarom niet |
|---|---|
| `EnhancedThirdPerson` (383 MB) | Character-template. Je hebt eigen C++-besturing die verder is |
| `AT3n3`, `Movies`, `Splash` | Eigen content van het ArchViz-template, geen Fab-pack |

## 3. Referentie — blijft buiten de repo

**`RayTracedCinematicLightin`** (4,2 GB)
`C:\Users\natha\Documents\Unreal Projects\RayTracedCinematicLightin`

Epic-leerproject over cinematische verlichting. Gebruiken voor de **opbouw** van
licht (waar bronnen staan, felheid ten opzichte van elkaar, hoe een ruimte
leesbaar wordt), niet voor de techniek — ECLIPSE is cel-shaded, ray-tracing hoort
er niet in. Open het project apart; niet kopiëren.

## 4. Staat in de Fab-bibliotheek, nog niet gedownload

*Claimen kost niets en is permanent — doe dat altijd meteen bij een gratis
asset, want gratis aanbiedingen verlopen. Downloaden pas als er een plek is waar
het landt.*

- **[FREE] Planet Project** (Arghanion's Puzzlebox, 5.6–5.7, ~1 GB) —
  procedurele planeet met orbit-systeem, volumetrische atmosfeer en wolken,
  8k planeettexturen, 16k star dome, nachtelijke stadslichten.
  **Wanneer:** eerder dan je zou denken. Niet pas bij de strategische kaart
  (Fase 3), maar bij de staande taak *"wereld zonder einde"* — een planeet aan
  de hemel van Kessara geeft horizonschaal die je met geometrie niet haalt.
  **Let op:** fotorealistisch geauthord. Vorm en compositie bruikbaar, shading
  niet — zelfde vertaalslag als het lightpack (GDD 15.5).
- **Muzzle Flash (Niagara System)** — mondingsvlam. Hoort bij de feedback-laag,
  maar het schot heeft nu al geluid en een hitmarker. Volgende ronde.
- **Footsteps: Essential SFX / Mini Sound Pack / Surface Footstep System** —
  `Footsteps_Volume_02` is al binnen en dekt Metal/Mud/Sand/Grass/Wood. Alleen
  ophalen als een oppervlak ontbreekt.

Ophalen gaat via de Epic Launcher → Fab → "Add to Project" → **ArchVizTemplateLite**
(5.7). Daarna `fab_doorgeven.ps1`. Zie §7 waarom niet rechtstreeks.

## 5. MetaHuman-plugins in de engine — staan klaar, uit gelaten

**Deze hoef je niet te downloaden.** Ze zitten al in UE 5.8 (1,9 GB aan
MetaHuman-plugins). Aanzetten: **Edit → Plugins** → zoek `metahuman` → vinkje →
Restart. Uitzetten gaat net zo makkelijk.

**Nu aan:** MetaHuman SDK (hulpmiddelen) · MetaHuman Creator + Core Tech
(de editor waarin je de gezichten maakt).

| Plugin | Wat het doet | Wanneer aanzetten |
|---|---|---|
| **MetaHuman Animator** | Gezichtsanimatie uit opnames | Als er dialoogscènes met gezichtsanimatie komen. Niet in Fase 2 |
| MetaHuman Animator Calibration Diagnostics/Processing | Hoort bij Animator | Tegelijk met Animator, niet los |
| MetaHuman CoreML | Inferentie op Apple-platforms | Nooit — je bouwt voor Windows |
| MetaHuman Creator – UAF support | Experimenteel, Unreal Animation Framework | Alleen als de animatie-pipeline naar UAF gaat |
| **MetaHuman Crowd** | Ondersteuning voor menigten | Als Kessara bevolkt moet worden (Fase 2/3). Experimenteel — eerst de performance-vraag beantwoorden |
| **MetaHuman Generator** | AI-gestuurde character-creatie | Interessant als er véél gezichten nodig zijn. Experimenteel en onvoorspelbaar; niet voor de zes vaste recepten |
| MetaHuman Live Link | Realtime streaming vanaf iPhone | Alleen met een iPhone met TrueDepth |
| MetaHumanRuntime | **Deprecated**, vervangen door SDK | Nooit |

*Regel: zet niets experimenteels aan zonder een concrete taak die het vraagt.
Een tragere editor en een instabieler project zijn geen theorie bij deze
categorie.*

## 6. MetaHuman-downloads van de website — beoordeeld, meeste niet bruikbaar

Van `metahuman.com/download`:

| Onderdeel | Oordeel |
|---|---|
| MetaHuman for Maya / Houdini / Marvelous Designer | **Nee** — vereist DCC-software die er niet is |
| Groom Starter/Advanced Kits | **Nee** — idem (Maya/Houdini) |
| Performance Capture Toolkit | **Nee** — studio-workflow |
| Live Link Face | **Alleen** met een iPhone met TrueDepth |
| **MetaHuman Animator Markerless Motion Capture** | **Later** — gezichtsanimatie uit gewone video, geen hardware nodig. Pas nuttig als er dialoogscènes met gezichtsanimatie komen |
| **MetaHuman Crowd Sample** | **Later** — als Kessara bevolkt moet worden. Let op performance; er zijn al negen character-packs |

**Waarom dit minder oplevert dan het lijkt:** deze tools bestaan voor
fotorealistisch gezichtsdetail — poriën, subsurface, haarsimulatie. De
toon-shader gooit dat weg. Besluit van 25-07 (variant B): alleen de ogen houden
hun eigen shader. Investeren in realisme dat daarna wegrendert is verspilde tijd.

## 7. Waarom alles via een doorgeefproject gaat

Fab weigert "Add to Project" als een pack 5.8 niet in zijn manifest noemt — ook
als de assets prima laden. De Launcher filtert op engine-versie, niet op of het
werkt. Meshes, texturen, materialen en animaties uit 5.3–5.7 laden zonder
problemen in 5.8.

Doelproject: **ArchVizTemplateLite** (5.7). Andere 5.7-projecten die ook werken:
MetaHumans, ModularUIHUDLiteBlueprin, ObjectiveTrackerSystemLit.

**Uitzondering:** packs met C++-code of plugins. Die moeten tegen 5.8
gecompileerd worden en zijn geen kopieerklus — `fab_doorgeven.ps1` waarschuwt
als het bronproject `Source/` of `Plugins/` heeft.

## 8. Al binnengehaald (niet opnieuw doen)

| Map | Wanneer | Wat |
|---|---|---|
| `FreeWeaponSounds` | 26-07 | 3 schoten + 3 gedempt + tails + volledige herlaadketen |
| `Footsteps_Volume_02` | 26-07 | Oppervlak-gebaseerd: Metal, Mud, Sand, Grass, Wood |
| `Sci_Fi_Light` | 26-07 | 17 emissieve armaturen — wand, plafond, laag, baken, props |
