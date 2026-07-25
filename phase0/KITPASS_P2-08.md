# Kit-pass P2-08 — mapping op gemeten meshes

*Plan, 2026-07-25. Gegrond op de gemeten curatie
(`phase0/CURATIE_ENVPACKS_2026-07-25.md` §3a, data in
`Saved/CurationStaging/envpacks_2026_07_25.json`), niet op mesh-namen die mooi
klinken. Prioriteitsladder komt uit de tweede art-review
(`phase0/DRESSING_ITERATIE_3.md` §1d).*

## 0. Harde voorwaarde: eerst het waardeplafond af

**Niet met kits beginnen voordat de pool en de barrière op hun doel staan.** De
reden is gemeten, niet stilistisch: het frame heeft nu een lege middenband
(middentonen 66,5% → 9,3%), en elke kit die je nú plaatst krijgt een tint die tegen
een verkeerd bereik is afgestemd. Dan moet de hele pass opnieuw. De geparkeerde
patch (`pending_pool_barrier.patch`) is die laatste stap.

## 1. Welk pack waarvoor — en waarom niet het grootste

Uniblocks heeft 3200 meshes maar **2506 met één materiaalslot** (78%). Eén slot kan
geen palet-scheiding per zone dragen, en dat is precies wat een wandpaneel nodig
heeft. **Factory_Pack_V1** heeft 151 meshes, 146 op dressing-gewicht, nul boven
budget en slechts 23% één-slot — dat is het pack dat de toon-pijplijn draagt.

Factory Pack blijkt bovendien een échte kit: zware hero-props mét slot-scheiding
*en* ultralichte modulaire stukken.

| Categorie | Meshes (gemeten) | Bestemming in het district |
|---|---|---|
| **Hero-props** (7-17k tris, 4-7 slots) | `SM_barrel_2` 17128/4, `SM_gas_bottle_packfbx` 16780/5, `SM_Controller_1` 7654/7, `SM_dare` 7132/5 | **Eén per frame, dichtbij de camera.** De controller (7 slots!) is de kandidaat voor de machine-bank-voorkant die de art-review "niet leesbaar" noemde. |
| **Industriële massa** (4-6k tris, 2-5 slots) | `SM_Tank` 5666/4, `SM_pipe_5` 5712/2, `SM_Power_Pole` 5532/3, `SM_crane_2/3` ~5500/3-5 | Vervangt de graybox machine-banken en het leidingwerk. Crane + power pole geven de **verticaliteit** die de skyline-audit vraagt. |
| **Skyline/backdrop** (~4k tris, 3-4 slots) | `SM_Tower_9_A` 4414/3, `SM_Tower_11_C` 3980/4 | Achter de perimeter, waar nu graybox-massa's staan. Let op: die staan op de `Skyline`-tint, niet op de district-tints. |
| **Modulaire bouwstukken** (2-22 tris!) | `SM_wall_2` 8/2, `SM_wall_3` 16/2, `SM_Floor_1/2` 2/1, `SM_roof_1/6`, `SM_pillar_2`, `SM_fence_2_c`, `SM_Sign_1b` 12/2 | Dit is de eigenlijke kit. **`SM_wall_2` en `SM_wall_3` hebben 2 slots** — die kunnen wand + trim scheiden en zijn dus bruikbaar; de 1-slot varianten alleen waar één tint klopt. |

**Uniblocks-gebruik blijft beperkt en per stuk gecontroleerd:** als massa en kleine
props waar één tint volstaat. De 13 hero- en 8 te-zware meshes vallen af (zwaarste:
een bush op 52k tris — het district is geen tuin).

## 2. Werkorde

1. **Waardeplafond af** (§0). Meet met `Tools/measure_frame_values.py`.
2. **Eén ruimte per iteratie**, en alleen ruimtes die al vastliggen: warehouse-yard →
   compound → gate. **Missie-sites uit P2-04 NIET** — die liggen nog niet vast, dus
   kits daar zijn weggegooid werk.
3. **Per mesh vóór plaatsing:** slot-aantal checken in de curatie-JSON, en de gain
   van elke albedo **meten** met `Tools/measure_albedo_gain.py`. 254 van de 416
   textures zijn 4K+; een geschatte gain is exact waar de magenta-container en de
   vloer-omkering op stukliepen.
4. **Waardeplafond respecteren:** niets niet-emissief boven de pool-kern. Nieuwe kits
   krijgen dus een tint in de middenband die nu leeg is — dat is het hele punt.
5. **Shotronde + art-review vóór elke commit** (15.8). Eerste PNG is het
   warm-up-offerframe. Cam 4 heeft een camera-probleem (45% lucht + 45% vloer) en cam
   6 valt af als reviewframe — beide staan in DRESSING_ITERATIE_3 §1d.
6. **Provenance:** accepts via `Tools/migrate_curation_accepts.py` naar
   `/Game/Art/Imported` met een regel in SOURCES.md. De packs zelf blijven
   machine-lokaal en untracked; commit die 6,2 GB niet.

## 2a. Werkorde stap 3 UITGEVOERD — de kandidaten zijn gemeten (2026-07-25)

`Tools/inspect_kit_candidates.py` (report-and-export only) heeft alle 12 kandidaten
opgezocht, hun slots uitgelezen en elke base-colour geëxporteerd; de gains zijn
daarna gemeten met `measure_albedo_gain.py`. **Alle 12 gevonden, en de
driehoek/slot-getallen bevestigen §1 exact** (SM_Controller_1 7654/7, SM_Tank 5666/4,
SM_barrel_2 17128/4, SM_wall_2 8/2, SM_wall_3 16/2).

| Albedo | lin. mean | gain (1/mean) | geklemd | eff. multiplier |
|---|---|---|---|---|
| Plaster002_4K | 0.6708 | 1.49 | 0.0% | 1.00 |
| Wood034_4K | 0.3882 | 2.58 | 0.0% | 1.00 |
| Concrete008_4K | 0.3540 | 2.82 | 0.0% | 1.00 |
| Planks001_4K | 0.2776 | 3.60 | 0.0% | 1.00 |
| Metal032_4K | 0.2417 | 4.14 | 0.0% | 1.00 |
| Gravel022_4K | 0.2105 | 4.75 | 1.3% | 1.00 |
| Concrete030_4K | 0.1182 | 8.46 | 0.0% | 1.00 |
| Fabric012_4K | 0.0422 | 23.70 | 0.0% | 1.00 |
| Metal029_4K | 0.0154 | 65.11 | 0.0% | 1.00 |

**Uitkomst: alle negen zijn bruikbaar.** De effectieve gemiddelde multiplier is voor
elke textuur 1.00, en hooguit 1,3% van de pixels loopt tegen de 2.5-clamp. Texturing
hermetert de auto-exposure dus niet — de eis uit 15.5 waarop de vloer eerder omviel.

**Twee eigen fouten die deze meting opleverde, allebei het noteren waard:**

1. Mijn eerste kleurfilter matchte `_d` en exporteerde daardoor 11
   **displacement-maps als albedo**. Een gain gemeten op een hoogtekaart is een getal
   dat er geloofwaardig uitziet en nergens op slaat. Er staat nu een expliciete
   uitsluitlijst (displacement/normal/roughness/AO/height/...) in het script.
2. Ik vlagde eerst elke gain boven 2.5 als "boven de clamp". Dat is een
   categoriefout: **de 2.5-clamp zit op de per-pixel multiplier (albedo × gain), niet
   op de gain zelf** — de vloer draait al jaren op 12.41. Het getal dat wél beslist is
   de geklemde fractie plus de effectieve multiplier, en die staan nu in de tabel.

Metal029 met gain 65 is dus geen alarm maar een simpel feit: het is een zeer donkere
textuur, en 65 × 0.0154 = 1.00 zoals bedoeld.

## 3. Wat dit oplost uit de art-review

- **Machine-faces onleesbaar** → `SM_Controller_1` (7 slots) op de bank-voorkant.
- **Geen verticaliteit / vlakke kaart** → crane + power pole + towers.
- **`SM_AssetPlatform` als dok** is al weg (blokken); Factory Pack heeft nu echte
  dock-geometrie als vervanging wanneer we die willen.
- **Lege middenband** → nieuwe kits landen bewust in dat gat.

Wat het **niet** oplost, en dus apart blijft: bloom/halo, contactschaduwen op alle
massaklassen, de pool-compositing (multiply in plaats van over), en de grijze vorken.
Die staan in DRESSING_ITERATIE_3 en zijn geen kit-werk.
