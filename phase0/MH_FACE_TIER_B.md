# MetaHuman gezichts-tier — implementatie van 15.7 keuze B (hybride)

*Implementatie-spec, geschreven 2026-07-25 direct na het owner-besluit "15.7: B".
Bron: `phase0/metahuman_recipes.md` §Beslispunt 15.7 (nu met BESLOTEN-blok),
`15_visual_quality_charter.md` §15.5/15.7/15.8. Code-anker: `AEclipseCharacter::ApplyBodyDef`
in `Eclipse/Source/Eclipse/Characters/EclipseCharacter.cpp:82-141`.
Deblokkeert: de named-character-koppen (Kaya/Brick/Vale/Dex/Petra/…) en daarmee de
dialoog-close-ups van SPEC-P2-04.*

## 1. Waarom dit niet "gewoon bToonRestyle aanzetten" is

De bestaande restyle-lus is **slot-blind**: hij loopt van `SlotIndex = 0` tot
`GetNumMaterials()` en vervangt *elk* slot door een toon-MID
(`EclipseCharacter.cpp:120-139`). Op een pack-body is dat precies goed. Op een
MetaHuman-hoofd is dat **optie A** — het overschrijft ook oog-, lacrimaal- en
tandmaterialen, en dat is exact wat B niet wil.

B vraagt dus één echte nieuwe capaciteit: **de lus moet weten wát een slot is.**
De sleutel is de materiaalslot-*naam* op de MetaHuman-mesh, niet de index.

## 2. Slot-classificatie (de kern van B)

Drie families, met de slotnamen die de wave-2-probe van 25-07 op de echte assets
zag (`MI_EyeL/R_Baked`, `SSP_skin_unified`, `SSP_eye_unified`, `SSP_teeth_unified`):

| Familie | Slotnaam-signalen | Behandeling |
|---|---|---|
| **Huid** | `head`, `body`, `skin`, `cartilage`, `saliva` | Door de toon-master (zelfde weg als de body-restyle) |
| **Tanden** | `teeth`, `tongue`, `gums` | Door de toon-master (owner-keuze B benoemt teeth expliciet) |
| **Oog-familie — UITGEZONDERD** | `eye`, `eyeLeft`, `eyeRight`, `eyeshell`, `eyelash`, `lacrimal`, `cornea` | Eigen shader behouden + exposure-compensatie |

Implementatie-eis: match op *lowercase substring* van de slotnaam, en de
oog-familie wordt **eerst** getest (een slot `eyeshell` mag nooit als huid
eindigen). Onbekende slotnaam = huid-behandeling met één `Verbose`-log van de
naam: de toon-weg is de veilige default (15.5 blijft dan gelden) en de log
levert de naam aan voor de volgende iteratie. Nooit stil overslaan — een
overgeslagen slot rendert fotoreëel en breekt de stijl-wet onzichtbaar.

**Grooms zijn geen materiaalslot.** Haar, wenkbrauwen en snor hangen als
`UGroomComponent` naast de mesh (probe: 6 GroomAssets + 6 GroomBindingAssets).
Die worden dus niet door deze lus geraakt en krijgen hun compensatie via hun
eigen componentpad — apart afhandelen, niet vergeten.

## 3. Exposure-compensatie voor de uitgezonderde familie

Het probleem dat B moet oplossen: het district is **unlit**, dus lit-PBR
onderbelicht tot silhouet (dezelfde forensiek die in de body-comment op
`EclipseCharacter.cpp:104-108` staat). Ogen die hun eigen shader houden, moeten
dus omhoog getild worden naar de toon-tier, anders zijn ze zwarte gaten in een
verder correct hoofd.

- Compensatie = een scalar op een MID van het *bestaande* oogmateriaal (niet de
  toon-master): behoud de shader, verschuif alleen het niveau.
- **Waarde niet verzinnen.** De factor volgt uit één meting: render het Kaya-slot
  op vaste camera, meet de gemiddelde luminantie van het huid-gebied (toon-tier,
  bekend) en van het oog-gebied (lit-tier), en de verhouding *is* de
  compensatie. Leg de meting vast in de commit-message zoals de dressing-gains
  in `SOURCES.md` staan. Tot die meting bestaat: geen compensatie toepassen en
  het als bekend-open loggen — een geschatte factor die als gemeten leest is
  precies wat 15.5 verbiedt.

## 4. De gain-val die de bestaande code zelf blootlegt

`EclipseCharacter.cpp:132-136` zet `AlbedoGain` op `3.2f` met deze
rechtvaardiging in het comment: *"bodies are small on screen, so the unmeasured
normalization is acceptable at this tier (the measured discipline applies to big
surfaces)."*

Voor een **dialoog-close-up van een gezicht valt die rechtvaardiging weg** — dat
is per definitie een groot oppervlak. De gezichts-tier moet daarom zijn eigen,
**gemeten** gain krijgen (1/lineair-gemiddelde van de MetaHuman-huidtextuur),
niet de body-waarde 3.2 overerven. Dit is geen detail: het is dezelfde fout die
de magenta-container in dressingronde 1 opleverde, één tier hoger.

## 5. Volgorde

1. **Assembly-run eerst.** `/Game/MetaHumans` is nog leeg; de probe (25-07 13:16)
   vond de bronnen wél binnen (Frey/Hannah/Mason/Advika + garderobe + 6 grooms,
   904 assets). Zonder samengestelde MetaHuman is er geen mesh om slots op te
   classificeren.
2. Slot-classificatie + huid/teeth door de toon-master + de gemeten gezichts-gain.
3. Groom-compensatiepad.
4. Oog-compensatie ná de meting uit §3.
5. **Harde poort:** één 15.8-shotronde op de vaste camera's, art-review op (a) de
   naad huid↔oog, (b) of het hoofd op dezelfde exposure-tier leest als het
   district, (c) of de kop nog als Borderlands-karakterkop leest en niet als
   fotoreëel masker. Faalt die ronde → terugval is **A**, niet C (owner-besluit).
   Eerste PNG van de rig = warm-up-offerframe, overslaan.

## 6. Wat NIET in deze changeset hoort

- Named-character-koppeling per slot in `DT_NamedCharacters` (bestaat al, data-only).
- De MetaHuman-garderobe voor figuranten — gewogen en afgewezen op 12.4-grond;
  figuranten draaien op de 11 CC0-bodies (`Content/Art/Characters/SOURCES.md`).
- Fallback-gedrag wijzigen: zolang een `MH_*` ontbreekt draait het slot op de
  `DT_BodyDefs`-fallback. Dat blijft, en de spec verandert er niets aan.
