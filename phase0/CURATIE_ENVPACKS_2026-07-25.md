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

## 3a. GEMETEN in de editor (17:17) — en het weerlegt mijn eigen eerste indruk

Gedraaid met `Tools/inventory_envpacks_2026_07_25.py` (report-only, raakt geen asset
aan). Beslisregels: ≤6000 tris = dressing-gewicht, 6000-20000 = alleen hero-prop,
>20000 = afwijzen op deze tier; ≥2 materiaalslots gewenst omdat de toon-master **per
zone** tint.

| Pack | meshes | dressing | hero | te zwaar | **1-slot** | textures (4K+) |
|---|---|---|---|---|---|---|
| Uniblocks | 3200 | 3179 | 13 | 8 | **2506 (78%)** | 203 (121) |
| Factory_Pack_V1 | 151 | 146 | 5 | 0 | **35 (23%)** | 139 (110) |
| Sci_fi_hallway | 19 | 17 | 2 | 0 | **18 (95%)** | 69 (23) |
| IBuilding_49 | 1 | 0 | 0 | **1** | 1 | 5 (5) |

**§1 zei dat Uniblocks "de kit-kandidaat" was. Dat was een gok op mesh-aantal, en de
meting spreekt hem tegen.** 78% van die 3200 meshes heeft één materiaalslot, en één
slot kan geen palet-scheiding dragen. Voor een klein prop is dat prima — een vat mag
één tint zijn — maar voor architectuur is het fataal: een wandpaneel heeft wand,
trim en lijst nodig in verschillende waarden, en dat is precies wat de tweede
art-review als "lege middenband" aanwees. **Factory_Pack_V1 heeft vier keer betere
slot-discipline (23% 1-slot) en nul meshes boven budget** — dat is het pack dat de
toon-pijplijn wél kan dragen, ondanks dat het kleiner is.

Per pack:
- **Factory_Pack_V1 — de sterkste kandidaat.** 146 van 151 meshes op dressing-gewicht,
  geen enkele te zwaar, zwaarste is een vat op 17k tris met 4 slots (hero-prop).
  Dit is het pack voor de machine-banken en het leidingwerk waar nu graybox staat.
- **Uniblocks — selectief, en alleen waar één tint klopt.** De 13 hero- en 8
  te-zware meshes vallen af (zwaarste: een bush op 52k tris). De rest is bruikbaar
  als *massa* en als kleine props, niet als getinte architectuur. Cureer per ruimte,
  en check per stuk het slot-aantal vóór je hem als wandpaneel gebruikt.
- **Sci_fi_hallway — 18 van 19 meshes 1-slot.** Als interieurkit voor de vault is dat
  een probleem in dezelfde richting. Parkeren tot de vault een art-pass krijgt.
- **IBuilding_49 — afgewezen op deze tier.** Eén mesh van 26.385 tris met één slot.
  Te zwaar én niet tintbaar; het district draait op een SM5-laptop (12.4).

**Textuur-waarschuwing:** 254 van de 416 textures zijn 4K of groter. Bruikbaar als
luminantie-albedo, maar elke gain MOET gemeten worden met
`Tools/measure_albedo_gain.py`. Geschatte gains zijn precies waar de
magenta-container en de vloer-omkering op stukliepen.

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
