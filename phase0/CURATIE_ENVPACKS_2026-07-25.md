# Curatieronde env-packs (2026-07-25) — inventaris + wat nog niet kan

*Vier packs kwamen binnen tussen 16:26 en 16:45. Deze ronde is BEWUST half: alles wat
op bestandsniveau kan is gedaan, en alles wat de editor nodig heeft (tri-counts,
materiaal-slots, of de meshes door de toon-pijplijn kunnen) staat als werklijst
klaar. Reden: de owner-editor staat open sinds 16:27 en Live Coding blokkeert elke
build én elke commandlet (memory `eclipse-editor-buildlock`).*

## 1. Wat er binnen is

| Pack | Grootte | Assets | Meshes | Textures | Materials | BP's | Maps |
|---|---|---|---|---|---|---|---|
| **Uniblocks** | 3777 MB | 3803 | ~3200 | ~204 | ~293 | 25 | 12 |
| **Factory_Pack_V1** | 2130 MB | 408 | ~151 | ~138 | ~115 | 3 | 2 |
| **Sci_fi_hallway** | 252 MB | 108 | ~35 | ~47 | ~17 | 3 | 2 |
| **IBuilding_49** | 66 MB | 7 | 1 (in `/Mesh`) | 5 | 2 | 0 | 1 |

**Eerste indruk per pack, op samenstelling:**

- **Uniblocks is de kit-kandidaat voor P2-08.** 3200 meshes met 12 demo-maps is het
  profiel van een modulaire bouwkit, en dat is precies wat de kit-pass nodig heeft:
  ruimtes die al vastliggen (plaza, perimeter, warehouse-yard) kunnen blok-voor-blok
  worden vervangen. Het is ook meteen het grootste risico: 3200 meshes is te veel om
  te curateren, dus dit moet per *ruimte* gaan, niet per asset.
- **Factory_Pack_V1** is een props-/industrieel pack (151 meshes, 115 materials) —
  kandidaat voor de machine-banken, tanks en leidingwerk waar nu graybox staat.
- **Sci_fi_hallway** is interieur. Relevant voor de **walkable vault** (die net
  landde in c6cf7fe en nu nog uit graybox-blokken bestaat), niet voor het district.
- **IBuilding_49** is één gebouw met een volledige PBR-set (AO/BaseColor/Metallic/
  Normal/Roughness). Klein, gericht, makkelijk te beoordelen.

## 2. Wishlist-check — vier van de zes zijn NIET binnen

Gecontroleerd op mapnaam-patronen in `Eclipse/Content`:

| Pack | Status |
|---|---|
| Sci-Fi Hallway | **aanwezig** (`Sci_fi_hallway`) |
| Sci-Fi Light Pack | **niet gezien** |
| Auto Footsteps Utility | **niet gezien** |
| Niagara Footstep VFX | **niet gezien** |
| FPS Weapon Bundle | **niet gezien** |
| Free Muzzle Flash | **niet gezien** |

Die vier zijn dus nog te klikken. Twee ervan zijn géén blokkade voor de kit-pass
(footsteps/muzzle zijn gameplay-feedback, geen environment), maar het **Sci-Fi Light
Pack** is wél relevant: de art-review vraagt om zichtbare lichtbronnen, en op een
unlit district zijn dat emissieve fixtures — precies wat zo'n pack levert.

## 3. Licentie/provenance en de opslagbeslissing

Alle vier zijn Fab/Epic-marketplace-content, dus dezelfde licentiefamilie als
`/Game/Art/Imported`: bruikbaar in dit project, **niet** herdistribueerbaar, en de
repo moet privé blijven (staat al zo in `Content/Art/Imported/SOURCES.md`).

**Aanbeveling: deze 6,2 GB NIET committen.** Volg het patroon van de pack-slim-ronde
(ASSET_CLEANUP §7-10): packs blijven machine-lokaal en untracked, en alleen de
*curatie-accepts* worden via een migratiescript naar `/Game/Art/Imported`
overgebracht — repo-tracked, met provenance-regel in SOURCES.md. Dat hield de repo
vorige keer klein terwijl de gebruikte assets wél reisden, en het voorkomt dat een
verworpen pack voorgoed in de git-historie zit.

## 4. Wat de editor nodig heeft (werklijst, mechanisch zodra het slot vrij is)

Dit zijn de échte curatiecriteria en geen ervan is leesbaar zonder de editor:

1. **Tri-counts per kandidaat-mesh** (12.4-budget). Bestandsgrootte is geen proxy.
2. **Materiaal-slots per mesh** — bepaalt of een mesh door de toon-master kan; een
   mesh met één slot voor alles is onbruikbaar voor palet-tinting per zone.
3. **Textuur-resoluties en gemeten lineaire gemiddelden** voor de albedo's die we
   als luminantie-detail willen gebruiken. **Gains meten, nooit schatten** — dat is
   precies waar de magenta-container en de vloer-omkering op stukliepen.
4. **Kan de mesh door de toon-pijplijn?** UV's aanwezig of world-aligned nodig,
   en of de authored pivot bruikbaar is. Let op de les uit de lamp-episode:
   *authored transform is niet verscheepte transform* — verifieer wat na import
   overblijft, niet wat het bronbestand beweert.
5. **Per ruimte een accept-lijst** voor Uniblocks, niet per asset.

Draai daarna: `Tools/measure_albedo_gain.py` voor de gains en de bestaande
`migrate_curation_accepts.py`-route voor de accepts.

## 5. Waarom de kit-pass nog niet begonnen is

De kit-pass (backlog #10, SPEC-P2-08) vraagt per iteratie: bouwen → shotronde →
art-review → commit op een groene bar. Alle vier die stappen hebben het build-slot
of de editor nodig. Daarnaast geldt de prioriteitsladder van de **tweede**
art-review, die zwaarder weegt dan de oude: het defect is nu het **waardePLAFOND**
(niet-emissieve dressing staat op 0.28-0.65 lum boven een grond van 0.03-0.06,
middenband leeg) en niet meer licht+vloer — zie `phase0/DRESSING_ITERATIE_3.md` §1d.
Nieuwe kits in de scène zetten vóórdat dat plafond zakt, zou betekenen dat elke kit
meteen op de verkeerde waarde staat en straks opnieuw moet.

**Volgorde die ik aanhoud zodra het slot vrij is:** plafond omlaag (het gemeten
defect) → pool herderiveren → dan kits plaatsen op een gezond waardebereik.
