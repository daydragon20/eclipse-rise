# BEVINDINGEN UIT SCREENSHOTS
*Eigenaar: `screenshot-inspector`. Vul aan, verwijder nooit — een afgehandelde bevinding krijgt `opgelost` in de laatste kolom.*
*Doel: Nathan hoeft niet zelf te melden wat er op zijn scherm mis is.*

**Beoordeeld tot en met:**
- Nathans map: t/m `Schermafbeelding 2026-07-31 195356.png` — alle vijf van de avond zijn nu ook echt bekéken, niet alleen overgenomen. Niets nieuwer dan 19:53 in die map.
- `Eclipse/Saved/Screenshots/HUD_volledig/`: de zeven `HUD_wapen_*`-frames van 20:20.
- **Ronde 01-08 00:30 — per bestand geopend (GEZIEN):**
  - `HUD_volledig/` (alles 23:34): `HUD_1e_persoon`, `HUD_3e_persoon`, `HUD_3e_persoon_terug`,
    `HUD_wissel_midden`, `HUD_herladen_0900ms`, `HUD_herladen_klaar_5000ms`,
    `HUD_kruis_ondergrond_0`, `HUD_kruis_ondergrond_2`, `HUD_wapen_D_los_aan_hand`,
    `HUD_wapen_F_eerste_persoon` — 10 van de 17.
  - `GLYPH_20_2/`: `NA_cam1_compound`, `NA_cam4_wide_overview`, `NA_cam5_gate_west_stencil`,
    `NA_cam6_crossing`, `NA_cam7_poster`, `_c4_VOOR/_c4_NA`, `_c5_VOOR/_c5_NA`, `_w6_NA` — 10 van de 36.
  - `WindowsEditor/`: `00027`, `00036`, `00037`, `00049`. Alle 50 zijn wél **gemeten**
    (zwartfractie per frame) — daar komt de 22-frames-regel hieronder uit.
- **Alleen gemeten, niet visueel beoordeeld:** `GRONDPROEF/` (46) en `SPOORLADDER/` (12). Dit zijn
  instrumentframes van de grondronde; alle 58 zijn onderling uniek (md5), dus geen VOOR/NA-paar dat
  stiekem hetzelfde bestand is.
- **Nog niet aan de beurt (volgende ronde begint hier):** `HUD_volledig/HUD_kruis_ondergrond_1`,
  `HUD_herladen_1900ms`, `HUD_wapen_A/_B/_C/_E/_G` (23:34-herschoten); `GLYPH_20_2/NA_cam0`, `NA_cam2`,
  `NA_cam3`, alle `VOOR_cam*`, `_c1_*`, `_c2_*`, `_c3_*`, `_w4_*`, `_grammar_poster_vs_signs`,
  `_poster_op_afstand`, `_crop_*`; `WindowsEditor` 00000–00026, 00028–00035, 00038–00048, 00050–00057.
  Let op: de map **hernummert zichzelf** — tussen begin en eind van deze ronde verdwenen 00018–00024
  en verschenen 00016/00017/00038/00050–00052/00056/00057 (00:47). Een nummer is geen identiteit.

---

## De vault, 01-08 — het licht is de zwakke schakel, en het is gemeten

*Frames: `Eclipse/Saved/Screenshots/WindowsEditor/HighresScreenshot00033.png` (commandopost),
`00034.png` (barakken), `00035.png` (werkplaats). Ik heb alle drie geopend — GEZIEN — en
daarna gemeten met `Tools/measure_frame_values.py` (Rec.709-luminantie, lineair), want een
indruk is geen bevinding.*

**De vault-agent mat onderscheidbaarheid met een dieptebeeld dat expliciet KLEURENBLIND is
(een hertint mag niet slagen). Precies daardoor kon zijn 16/16 niets zeggen over licht.**
Twee defecten, allebei met een getal:

### 1. Het plafond rendert als absoluut zwart

| gebied | lum_lin |
|---|---|
| plafond (barakken, 144.000 px) | **0,0000** — max 0, geen enkele pixel boven nul |
| plafond (commandopost) | 0,0002 |
| plafond (**inlichtingenkamer, `00036`**, 90.000 px) | **0,0000** — max 0. *Nagemeten 01-08; hetzelfde patroon, vierde ruimte.* |
| vloer (barakken) | 0,0642 |

Het plafond **bestaat** (`EclipseVaultBuilder.cpp:733` — één vloer- en één plafondplaat over
de hele footprint), er valt alleen niets op. Ter vergelijking: de districtsgrond zit op
0,03–0,06. Een ruimte van 4,2 m onder de grond leest daarmee als **open naar de nacht**.

### 2. De lichtbronnen zijn donkerder dan wat ze verlichten

| gebied | lum_lin | lineaire RGB |
|---|---|---|
| GlowStrip-balk (barakken) | 0,0563 | (0,0999 · 0,0481 · 0,0093) — **wél amber** |
| vloer eronder | **0,0642** | — |
| plafondlamp links (commandopost) | 0,0093 | (0,0075 · 0,0094 · **0,0144**) — **blauw** |
| plafondlamp rechts | 0,0118 | (0,0095 · 0,0118 · **0,0181**) — **blauw** |
| donkere wand ernaast | 0,0163 | — |
| kaarttafel | **0,1933** | — |

Twee dingen staan hier los van elkaar:

- **De balk in de barakken is correct amber** (R:G:B ≈ 10,7 : 5,2 : 1 tegen de geauthorde
  `GlowStrip` (2,2 · 1,0 · 0,3) ≈ 7,3 : 3,3 : 1) maar **haalt de vloer niet**: 0,0563 tegen
  0,0642. Een werklamp die donkerder is dan de vloer die hij verlicht.
- **De twee plafondlampen in de commandopost zijn niet eens amber.** Ze zijn blauw-dominant
  (B > R), dezelfde tintfamilie als de donkere wand, en **donkerder dan die wand**. In dit
  frame is het helderste ding het **meubilair** (kaarttafel 0,1933) — twintig keer de lamp.

**Het is niet de tonemapper.** In hetzelfde frame, op dezelfde hoogte, meet de amberwand
0,0781 met (0,1366 · 0,0671 · 0,0146) — R ≫ B. Amber komt er dus wel degelijk doorheen; een
horizontale scan op lamphoogte bevestigt het (x=1440 → 0,0667, R ≫ B).

**Twee kandidaat-oorzaken, geen van beide bewezen** — dit is een symptoombeschrijving:
(a) die twee boxen krijgen het `GlowStrip`-materiaal niet toegewezen zoals de balk dat wel
krijgt, of (b) wat ik zie is een niet-emissieve behuizing en het emissieve vlak wijst van de
camera af. Wie dit oppakt: meet eerst welke van de twee, met een tweede frame vanuit een
andere hoek — dat scheidt ze door constructie.

> **De regel eronder is de oude:** `authored ≠ verscheept`. `GlowStrip` staat in de palettabel
> op (2,2 · 1,0 · 0,3) met `EmissiveScale` 10, en één van de twee plaatsingen levert dat en
> de andere niet.

*Nagemeten op 01-08: het plafond van `00036` (inlichtingenkamer) meet óók 0,0000 over 90.000 px,
max 0. Vier ruimtes, vier keer nul — dit is niet één verkeerd geplaatste plaat.*

---

## De ronde van 01-08 — het scherm, de glyphronde en de gang

*Dit blok gaat over de 23:34-HUD-serie, de 22:15-glyphserie en de `WindowsEditor`-rondes van
23:17–23:29. Alles hieronder is per bestand geopend; waar een getal staat komt het uit
`Tools/measure_frame_values.py` of uit een pixeltelling op het frame zelf.*

### 1. De schermlaag is fors vooruitgegaan — en heeft één nieuw, meetbaar leesbaarheidsgat

Wat er sinds 20:20 **echt** anders is, en op welk bestand het staat, staat verderop in "Al bekend".
Het nieuwe probleem is dit:

De statusregel `command: idle (hold Q / pad LB) · doctrine: ready` is **wit zonder rand, zonder
schaduw en zonder achtergrondvlak**. In `HUD_1e_persoon`, `HUD_3e_persoon_terug` en
`HUD_herladen_klaar_5000ms` valt het staartstuk `…trine: ready` precies op een **fel gele balk in de
wereld** (een platte gele strook op de muur). Gemeten in `HUD_3e_persoon_terug`:

| gebied | lum_lin | opmerking |
|---|---|---|
| gele balk achter de tekst (880 px) | **0,7065** | **53,0% van de pixels zit geclipt op 255** |
| muur direct ernaast (1045 px) | 0,2864 | — |

Wit (relatieve luminantie 1,0) op die balk geeft een contrastverhouding van
(1,0+0,05)/(0,7065+0,05) = **1,39 : 1**. Op de muur ernaast is het 3,12 : 1. WCAG AA vraagt 4,5 : 1
voor lopende tekst, 3 : 1 voor grote tekst. **De HUD haalt de norm nergens in dit frame, en waar een
fel object achter de tekst staat is hij feitelijk weg.** Dit is precies de eis "leesbaar in beide
perspectieven": het gaat hier niet om smaak maar om een getal.

### 2. De glyphronde heeft de zwarte achtergrondplaat gesloopt — en de kruising staat er nog aards bij

**Wat gelukt is (VOOR/NA is hier echt bewijs):** in `_c4_VOOR_stencil` en `_c5_VOOR_stencil` staat de
Eclipse-sigil op een **ondoorzichtige zwarte rechthoek** die als een postzegel op de muur plakt. In
`_c4_NA_stencil`, `_c5_NA_stencil` en `_w6_NA` is die plaat weg en spuit de sigil dóór op de muur,
mét druipsporen. Dat is een echte fix, en de VOOR/NA-opzet bewijst hem.

**Wat níet gelukt is:** het zwaartepunt van deze ronde was §20.2, en op `NA_cam6_crossing` staat aan
weerszijden van de kruising nog steeds een installatie die je zonder aarzeling als aards aanwijst.
Zie de regels in de tabel — ik heb ze bewust gesplitst in *de vorm van het bord* en *de paal waar hij
op zit*, want dat zijn twee verschillende reparaties.

> **Waar ik géén overtreding zag, en dat expliciet meld:** de twéé pictogrammen zelf
> (het rode zon/turbine-embleem en het amberen "gieten in een bak"-teken) zijn op §20.2 gebouwd rond
> de Radiance-zon, met een eigen glyphrij eronder. Dáár is de asset-naam misleidend geweest, niet het
> beeld. Ik heb ze op 6× uitvergroot bekeken voor ik dit schreef.

> **Klein voorbehoud bij de bewijsmap zelf, geen defect in de game:** `_c4_VOOR_stencil.png` en
> `_w4_VOOR.png` zijn **byte-identiek** (zelfde md5, 368.876 bytes). Wie de map doorloopt telt daar
> twee onafhankelijke voor-opnames waar er één is. De andere 34 bestanden in `GLYPH_20_2` zijn wél
> uniek.

### 3. Twee-en-twintig frames uit de rondes van 23:19–23:24 bevatten geen grond

Ik heb alle 50 frames in `WindowsEditor` doorgemeten op zwartfractie. In **22 ervan** staat de
onderste 480 beeldrijen op **max luminantie 0** — geen enkele pixel boven absoluut zwart:

`00000–00003`, `00025–00032`, `00040–00046`, `00053–00055`.

In `00027` meet de héle onderhelft (1.075.200 px) `lum_lin = 0,0000` met max 0, terwijl boven de
horizon een normale luchtgradiënt staat (0,0313). **Dat is dezelfde handtekening als het
vaultplafond**: niet donker, maar exact nul. Wat die rondes ook probeerden vast te leggen — op deze
frames staat het niet.

---

## Bevindingen

| Datum | Bestand | Ernst | Wat er te zien is | Waarschijnlijke oorzaak |
|---|---|---|---|---|
| 2026-07-31 19:11 | Schermafbeelding 191142 | blokkeert | Dialoog: map `C:/Users/natha/AppData/Local/Programs/Git/Game/Maps/GrayboxDistrict` niet gevonden | **Bevestigd:** Git Bash verminkt `/Game/...`-argumenten. `DEBUG_DISCIPLINE.md` §4.4. Start via PowerShell. |
| 2026-07-31 19:19 | Schermafbeelding 191919 | blokkeert | Zelfde dialoog, opnieuw | De agent gebruikte opnieuw de Bash-tool |
| 2026-07-31 19:20 | Schermafbeelding 192039 | blokkeert | "GPU Crashed or D3D Device Removed", mini-dump weggeschreven | Page fault in een compute-shader (Aftermath-dump). Zie owner-vraag O-7. |
| 2026-07-31 19:21 | Schermafbeelding 192113 | blokkeert | Crash Reporter, stack ×5 in `UnrealEditor_D3D12RHI` | Zelfde crash |
| 2026-07-31 19:53 | Schermafbeelding 195356 | blokkeert | "Missing Eclipse Modules — Eclipse" | **OPGELOST 20:15, en de eerste verklaring was fout.** Hier stond dat `UnrealEditor-Eclipse.dll` ontbrak. Nagemeten: die stond er gewoon (3,6 MB). Het probleem was dat hij om 20:05 opnieuw gebouwd was terwijl `UnrealEditor-EclipseEditor.dll` van 19:47 dateerde — **verschillende BuildIds**, en dan weigert UE te starten. De onderliggende oorzaak was een agent die `GetFlankStateLabel()` aanriep terwijl de declaratie nog in zijn vólgende stap zat, waardoor de build omviel. Eén geslaagde `-NoUba`-build bracht ze in sync; geverifieerd met een commandlet-run die beide modules laadt. **Les:** deze melding wijst naar een ontbrekend bestand terwijl het om een tijdstempel gaat. |
| 2026-07-31 avond | *(owner-waarneming)* | fout | Wapen wordt **omgekeerd** vastgehouden | Vermoedelijk socket-rotatie: forward-as van de mesh matcht niet met `hand_r`. Zie OBS-1. |
| 2026-07-31 avond | *(owner-waarneming)* | fout | Geen handen/armen zichtbaar in first-person | Lichaam wordt niet gerenderd in 1e persoon, dus wapen én handen verdwijnen mee. Ontwerpkeuze nodig — zie OBS-2. |
| 2026-07-31 avond | *(owner-waarneming)* | stijl | Schermlaag volgt de Borderlands-taal nog niet | Stijlronde staat gepland ná functionele compleetheid. Zie `REFERENTIE_HUD_BORDERLANDS.md` en O-8. |
| 2026-07-31 20:20 | `HUD_volledig/HUD_wapen_B_zonder_wapensectie.png` | fout | Het pistool hángt midden in beeld, los, zonder hand of arm eraan. De loop wijst schuin naar rechtsboven, de greep naar linksonder. Het wapen is ~300 van de 720 beeldlijnen hoog — even hoog als de poortpijlers erachter. | Vermoedelijk twee dingen tegelijk: een importschaal die een flinke factor te groot staat, én een attach op iets anders dan `hand_r`. **Openstaande tegenspraak, expres niet weggepoetst:** de camerastand in dit frame is niet van die in `HUD_wapen_F_eerste_persoon` te onderscheiden — zelfde weg, zelfde poort, dezelfde vier borden — en in `_F` is géén wapen te zien. Tegelijk staat in owner-vraag O-9 een méting dat het wapen in 1e persoon op (-2054, 3130) projecteert, dus ver buiten een beeld van 1280x720. Twee mogelijkheden en ik kan er met beeld alleen niet tussen kiezen: óf `_B` is géén 1e-persoonsframe (waarschijnlijker: "zonder wapensectie" betekende dat het wapen los in de wereld stond), óf het wapen kán er wél komen en de meting geldt niet voor alle standen. **Wie hieraan verder werkt: kijk eerst welke camera `_B` geschoten heeft, voor je op één van de twee doorbouwt.** |
| 2026-07-31 20:20 | `HUD_wapen_A_ingebouwd`, `_C_alleen_wapensectie`, `_D_los_aan_hand`, `_E_na_wissel`, `_G_BEIDE_tegenproef` | blokkeert | Een roodbruin vlak — huid/romp van het personage — vult 60 tot 95% van het beeld. Alleen aan de linkerkant blijft een strook wereld over. In `_G` staat de romp rechts, in de andere vier in het midden. | **OPGELOST — geverifieerd 01-08.** In alle zes geopende 3e-persoonsframes van 23:34 staat de camera achter en boven het personage; de hele figuur staat vrij in beeld en de wereld eromheen is zichtbaar. Daardoor is `HUD_wapen_D_los_aan_hand` nu wél bruikbaar bewijs — zie de nieuwe wapenregel. |
| 2026-07-31 20:20 | `HUD_wapen_A/_B/_C/_D/_G` tegenover `_F` | fout | Tijdens herladen staat rechtsonder alléén `HERLADEN` in oranje. De teller `12 / 12` die in `_F` op diezelfde plek staat is dan wég — je ziet tijdens het herladen niet meer hoeveel je had, noch hoe groot het magazijn is. | **OPGELOST — geverifieerd 01-08 op `HUD_herladen_0900ms` en `HUD_herladen_klaar_5000ms`.** Rechtsonder staat nu `RELOADING` (oranje, met onderstreping) **bóven** `AR Foundry   18 / 30`. Beide dingen tegelijk leesbaar. |
| 2026-07-31 20:20 | `HUD_wapen_F_eerste_persoon.png`, `HUD_wapen_E_na_wissel.png` | fout | Rechtsonder staat `Sidearm_Scrap   12 / 12`. De speler leest een asset-id mét underscore, geen wapennaam. | **Half opgelost — zie de nieuwe regel van 01-08.** Negen van de tien 23:34-frames tonen `AR Foundry`, maar `HUD_wapen_F_eerste_persoon` toont nog steeds `Sidearm_Scrap`. Niet als opgelost afvinken. |
| 2026-07-31 20:20 | `HUD_wapen_E_na_wissel.png` | fout | De wapenregel loopt van het scherm af: `Sidearm_Scrap` loopt door tot in de allerlaatste beeldkolom en de `12 / 12` erachter is niet meer in beeld. In `_F` past diezelfde regel wél, en eindigt hij ~34 px vóór de rand. | Vermoedelijk rechts uitgelijnde tekst zonder rechtermarge of clip, zodat een langere string over de rand schuift. Dat het uitgerekend in het frame *na de wissel* gebeurt maakt het erger: precies op het moment dat je moet zien wát je nu vasthebt. |
| 2026-07-31 20:20 | alle zeven `HUD_wapen_*`-frames | fout | Taalmix in dezelfde schermlaag: `HERLADEN` (Nederlands) staat naast `MISSION ACTIVE`, `Spring the ambush`, `Extract the squad`, `squad orders`, `command: idle`, `doctrine: ready` (Engels). | **OPGELOST — geverifieerd 01-08.** In `HUD_herladen_0900ms` en `_klaar_5000ms` staat er `RELOADING`. Alle zichtbare schermtekst in de tien geopende 23:34-frames is Engels. |
| 2026-07-31 20:20 | alle zeven `HUD_wapen_*`-frames | fout | Geen veilige marge aan de randen. `== MISSION ACTIVE` begint op ~2 px van de linkerrand, de kopregel raakt de bovenrand, de wapenregel zit ~34 px van de onderrand. Niets staat binnen een title-safe zone. | **Grotendeels opgelost — nagemeten 01-08 op `HUD_1e_persoon` (1280x720):** links 37 px, rechts 37 px, boven 38 px, onder 44 px. Was 2 px links. Verticaal zit alles nu binnen 5%; **horizontaal is 37 px = 2,9%, dus nog steeds binnen de 5%-title-safe-rand.** Restpunt, geen blokkade meer. |
| 2026-07-31 20:20 | `HUD_wapen_F` (op het gele bord), `_A/_C/_D/_G` (op de romp) | stijl | Het richtkruis is een wit plusje van een paar pixels. Op het gele waarschuwingsbord in `_F` en op het roodbruine lichaam in de andere frames is het nauwelijks terug te vinden. | Geen omlijning of contrastrand onder het richtkruis. Op een lichte of drukke achtergrond verdwijnt wit in wit — en het richtkruis is het enige element dat je élke seconde nodig hebt. |
| 2026-07-31 20:20 | `HUD_wapen_F_eerste_persoon.png`, `HUD_wapen_B_zonder_wapensectie.png` | fout | Losse lichaamsgeometrie aan de beeldranden in 1e persoon: in `_F` twee oranje/tan vormen in de linkeronderhoek, in `_B` twee tan vormen rechtsboven tégen de lucht, nergens aan vast. Zelfde kleur als de romp in de 3e-persoonsframes. | Vermoedelijk delen van de karaktermesh die wél gerenderd worden, maar op de verkeerde plek rond de 1e-persoonscamera staan. Hangt vermoedelijk samen met de camera-in-lichaam-regel hierboven: het lichaam wordt niet uitgezet, het staat verkeerd. |
| 2026-07-31 20:20 | alle zeven `HUD_wapen_*`-frames | stijl | De squad heet `Sef Voss`, `Sef Chen`, `Anke Stahl` — twee van de drie hebben dezelfde voornaam. | `EclipseRosterLogic.cpp:21` trekt per soldaat een willekeurige voornaam uit de pool zónder te kijken wat de vorige soldaat al kreeg. Bij een squad van drie valt een dubbele voornaam meteen op en leest als een bug, ook als hij statistisch gewoon kan. |
| 2026-07-31 20:20 | `HUD_wapen_F_eerste_persoon.png`, `HUD_wapen_B_zonder_wapensectie.png` | stijl | Vier identieke donkere pijlers naast elkaar in de poort, plus twee identieke rotsblokken rechts in beeld. | `20_world_dressing_standard.md` §20.7: dezelfde mesh drie keer in beeld. Bij een poortcolonnade is herhaling deels bedoeld — maar de pijlers zijn onderling ook niet gedraaid of verschillend verweerd, dus het leest als kopieerwerk in plaats van als architectuur. |
| **2026-08-01 23:34** | `HUD_1e_persoon`, `HUD_3e_persoon_terug`, `HUD_herladen_klaar_5000ms` | fout | **GEZIEN + gemeten.** De statusregel is wit zonder rand of achtergrondvlak. Waar een felgele wereldbalk erachter staat, wordt `…trine: ready` wit-op-geel. Balk: `lum_lin` 0,7065, **53,0% van 880 px geclipt op 255**; muur ernaast 0,2864. Contrast wit-op-balk **1,39 : 1**, wit-op-muur 3,12 : 1. | *Vermoeden:* HUD-tekst wordt zonder outline/drop-shadow/backplate getekend, dus de leesbaarheid hangt volledig af van wat er toevallig achter staat. Dit is de eis "leesbaar in beide perspectieven", nu met een getal. |
| **2026-08-01 23:34** | `HUD_wapen_F_eerste_persoon` tegenover de andere negen 23:34-frames | fout | **GEZIEN.** Rechtsonder staat `Sidearm_Scrap   12 / 12` — mét underscore. In `HUD_1e_persoon`, `_3e_persoon`, `_3e_persoon_terug`, `_wissel_midden`, `_herladen_0900ms`, `_herladen_klaar_5000ms`, `_kruis_ondergrond_0/2` en `_wapen_D` staat op diezelfde plek `AR Foundry   30 / 30` — netjes. | De asset-id-lek uit de 20:20-ronde is **half** gerepareerd: er is nu een weergavenaam, maar kennelijk maar voor één wapen. *Vermoeden:* de rij `Sidearm_Scrap` in de wapentabel heeft geen ingevuld display-name-veld en valt terug op de rijnaam. Eén wapen tonen is geen bewijs dat het lek dicht is. |
| **2026-08-01 23:34** | `HUD_wapen_D_los_aan_hand` (op 5× uitvergroot) | fout | **GEZIEN.** De silhouet van het wapen is van links naar rechts te lezen als: magazijn → pistoolgreep met trekkerbeugel → kast → **open skeletkolf**. De kolf steekt vrij de lucht in aan de van het lichaam afgekeerde kant; het uiteinde met de loop verdwijnt ín de romp van het personage. Er zit **geen hand om de greep** — greep en trekkerbeugel staan tegen open lucht. | Scherpere beeldbevestiging van OBS-1 dan de 20:20-ronde had (daar was er geen hand én geen leesbaar silhouet). *Vermoeden:* de forward-as van de mesh staat 180° gedraaid ten opzichte van de socket, óf hij hangt aan de verkeerde bot-socket. **Meet dit voor je fixt** — het frame laat de as zien, niet de oorzaak. |
| **2026-08-01 23:34** | `HUD_wissel_midden` tegenover `HUD_1e_persoon` en `HUD_wapen_F_eerste_persoon` | fout | **GEZIEN — en dit sluit de openstaande tegenspraak van de 20:20-ronde.** In `HUD_wissel_midden` (camera midden in de overgang 3e↔1e) staan **wél** een wapen en een **oranje arm**, linksonder in beeld. In de twee volledige 1e-persoonsframes staat op diezelfde plek helemaal niets. | De mesh, het materiaal en de draw call werken; in de volle 1e-persoonsstand valt het geheel **buiten het beeldvlak**. Dat sluit aan bij de meting in O-9 (projectie op (-2054, 3130) bij 1280x720). **De vorige ronde kon niet kiezen tussen "er is geen 1e-persoonsmesh" en "hij staat verkeerd"; dit frame kiest: hij staat verkeerd.** Geen ontwerpvraag meer — een offset. |
| **2026-08-01 23:34** | alle tien geopende `HUD_*`-frames | fout | **GEZIEN.** Er staat **geen gezondheidsbalk, geen schild, geen schade-indicatie en geen minimap** op het scherm. Wat er wél staat: objectives, squad-orders, statusregel, richtkruis, munitiepaneel. | Geen smaakvraag: `REFERENTIE_HUD_BORDERLANDS.md` r36-37 wijst gezondheid expliciet toe aan **linksonder**, en r57 zegt "Eerst compleet, dan mooi. Alle elementen functioneel aanwezig (gezondheid, minimap, squad-kaarten…)". Linksonder is in alle tien frames leeg. |
| **2026-08-01 23:34** | alle tien geopende `HUD_*`-frames | stijl | **GEZIEN.** Het spelerspersonage draagt niets: een egaal oranjebruine figuur met zwarte cel-omlijning, geen uniform, geen bepakking, geen laarzen — alleen een smal olijfkleurig bandje om de linkerpols. In `NA_cam5_gate_west_stencil` heeft dezelfde figuur bovendien een kop in een duidelijk andere kleur dan het lichaam. | Placeholder-personage. Staat hier één keer genoteerd zodat het niet als "nieuwe bug" terugkomt in elke volgende ronde. |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam6_crossing.png` | fout | **GEZIEN (6× uitvergroot).** Op een paal langs de weg zit een vierkante amberen plaat met daarop een **gele ruit-omlijning** (op de punt gezet vierkant) met een klein zonneteken in het midden. | `20_world_dressing_standard.md` §20.2 is categorisch. De *inhoud* (zon) is op standaard; de **draagvorm** — gele ruit op een paal aan de rand van een rijbaan — is een van de best herkenbare aardse verkeersbordvormen die er zijn. §20.2-test: "kan een speler dit aanwijzen en zeggen 'dat komt uit onze wereld'?" Ja. |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam6_crossing.png` | fout | **GEZIEN (2× en 6× uitvergroot).** Aan weerszijden van de kruising staat een **paal met ronde uitlopende voetplaat**, bovenaan een **zijwaartse arm met een kapvormige, naar beneden gerichte lamp met geel lensvlak**. Halverwege elke paal een verlicht paneel met een **klep erboven** — links **rood**, rechts **geel** — en tussen de twee palen hangt een **doorzakkende zwarte kabel**. | §20.2 verbiedt naast verkeersborden expliciet **stoplichten** en de "westerse straatmeubilairtraditie". Rood links, geel rechts, kleppen erboven, kabel ertussen, cobra-lampen erop: dit leest als een aardse verkeerslichtinstallatie, ongeacht wat er op de panelen staat. |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam6_crossing.png` (beide palen) | fout | **GEZIEN.** Op **allebei** de bordpanelen loopt de paal vóór het bordvlak langs en dekt de middenkolom af — bij het gele bord snijdt hij de ruit doormidden en verbergt hij het centrale teken, bij het rode bord dekt hij het midden van de glyph. | *Vermoeden:* het bord is aan de van de camera afgekeerde zijde van de paal bevestigd, of op de paal-as gecentreerd in plaats van opzij geplaatst. Los van §20.2 een echte fout: een bord waarvan de leesbare kant door zijn eigen paal wordt afgedekt. |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam6_crossing.png` | fout | **GEZIEN + gemeten.** Een **2–3 px brede oranje lijn** loopt kaarsrecht over het wegdek, van x≈245 tot x≈1420, en wordt daarbij nauwelijks dikker (2 px bij x=350, 3 px bij x=700). Hij eindigt op geen enkel object en volgt geen belijning. Hij staat **identiek in `VOOR_cam6_crossing`, op dezelfde pixelposities** (x=300→y=867, x=400→y=878, x=500→y=890, x=600→y=901). | Niet uit de glyphronde — hij stond er al. *Twee kandidaten, geen van beide bewezen:* (a) een blijvende debug-lijn in de opname, (b) de naad tussen twee vrijwel coplanaire grondvlakken (de `GRONDPROEF`-serie werkt met +0 cm/+2 cm-recepten). **Wie dit oppakt: één frame zonder debug-tekenen scheidt de twee door constructie.** |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam6_crossing.png` | stijl | **GEZIEN.** Op het wegdek ligt een plaat met **zwart-gele diagonale gevarenstrepen**. | §20.2-tabel wijst "Waarschuwing / gevaar" toe aan "Dominion-hazardtaal, gebouwd rond de **Radiance**-zon". Diagonale zwart-gele arcering is de aardse industriële conventie, niet die taal. Minder scherp dan het ruitbord — daarom `stijl` — maar het hoort in dezelfde vervangronde. |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam4_wide_overview.png` | stijl | **GEZIEN + geteld.** In de onderste helft (1.036.800 px) ligt **91,3% van de pixels binnen ±10 sRGB van één kleur** (44,45,55) — kale, kenmerkloze grond. Over het héle frame ligt **64,3%** binnen ±10 van één kleur. Alle inhoud zit geperst in een band van ~200 px bovenaan. | §20.4/§20.7: geen focuspunt, geen voorgrondlaag, geen schaal-anker. Een overzichtsbeeld dat voor bijna twee derde uit één egale kleur bestaat, laat niet zien wat er gebouwd is — ook niet als er wél iets gebouwd is. |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam4_wide_overview.png`, `NA_cam1_compound`, `NA_cam6_crossing` | stijl | **GEZIEN.** Dezelfde amberkleurige, halfdoorschijnende blokmesh staat in `NA_cam4` **ruim twintig keer** verspreid over de lege vlakte, zonder onderling verband en zonder dat één ervan een vraag beantwoordt. Dezelfde mesh staat in `NA_cam1` nog eens als een reusachtig exemplaar rechtsonder en in `NA_cam6` in minstens vier maten. | §20.7 "dezelfde mesh drie keer in beeld" én §20.3 "geen enkel object wordt geplaatst om ruimte te vullen". Twintig keer dezelfde mesh over een lege vlakte is de opvullingsreflex waar §20.3 over gaat. |
| **2026-08-01 22:16** | `GLYPH_20_2/NA_cam7_poster.png` | stijl | **GEZIEN.** Twee **identieke** grijsblauwe kioskkasten staan tegen elkaar aan pal vóór de propagandaposter en dekken de onderste helft daarvan af. De poster (donkere plaat met gouden Radiance-zon) is het enige verhalende element in het frame. | §20.7 (dezelfde mesh naast zichzelf) plus een compositiefout: het element waar de shot naar vernoemd is, wordt door rekwisieten afgedekt. |
| **2026-08-01 22:15** | `GLYPH_20_2/NA_cam5_gate_west_stencil.png` | stijl | **GEZIEN.** Twee identieke donkere poortpijlers staan midden in beeld tegen elkaar aan; de voorste loopt onderaan uit beeld. Ze markeren geen doorgang — ze staan in de zichtlijn. Verder: de drie borden op de muur hangen op exact dezelfde hoogte, even ver uit elkaar en even groot. | §20.7 (herhaalde mesh, gelijke slijtage) en §20.4 (de compositie wordt geblokkeerd in plaats van gekaderd). |
| **2026-08-01 23:29** | `WindowsEditor/HighresScreenshot00036.png` (inlichtingenkamer) | fout | **GEZIEN (3× uitvergroot).** De kratten in de kamer én de **vier wandpanelen** dragen een regelmatig licht/donker **schaakbordpatroon** over alle zichtbare vlakken; de celgrootte is constant in wereldmaat en tilet dus door over verschillende objecten. De vier wandpanelen zijn bovendien **doorzichtig** — de blokvoegen van de muur erachter zijn er dwars doorheen te zien. | *Vermoeden:* een standaard- of placeholder-schaakbordmateriaal in plaats van het bedoelde oppervlak, en op de panelen een alfakanaal dat als raster doorkomt. Zolang dit erop staat, zegt geen enkel oordeel over de aankleding van deze kamer iets. |
| **2026-08-01 23:29** | `WindowsEditor/HighresScreenshot00037.png`, `00049.png` (gang) | fout | **GEZIEN + gemeten.** Door het zwarte plafond loopt een **dunne blauwgrijze lijn** van (841, 0) schuin naar (868, 110) en verder tot y≈470. Gemeten waarde op de lijn ≈ (34, 39, 49) sRGB — blauw-dominant — tegen een plafond dat gemiddeld `lum_lin` 0,0033 meet. Het is het enige niet-zwarte in dat vlak. | *Vermoeden:* een naad tussen twee plafondplaten waar de ruimte erachter doorheen te zien is. Zie ook het vaultblok bovenaan: het plafond is daar het zwakke punt, hier lekt het. |
| **2026-08-01 23:19–23:24** | `WindowsEditor` `00000–00003`, `00025–00032`, `00040–00046`, `00053–00055` (22 frames) | fout | **GEMETEN op alle 50 frames.** In deze 22 staat de **onderste 480 beeldrijen op max luminantie 0** — geen enkele pixel boven absoluut zwart. In `00027` meet de héle onderhelft (1.075.200 px) `lum_lin = 0,0000`, max 0, terwijl de lucht erboven een normale gradiënt van 0,0313 heeft. | *Twee kandidaten:* (a) de camera staat in deze rondes op of onder het grondvlak, (b) het grondvlak rendert daar op exact nul, dezelfde handtekening als het vaultplafond. **Dit is een instrumentbevinding: wat die 22 frames ook moesten aantonen, de grond staat er niet op.** |

---

## Al bekend — status in de ronde van 20:20

Deze punten stonden al ergens. Hier alleen of ze er nog zijn, en op welk bestand.

| Bekend punt | Nog zichtbaar? | Waar |
|---|---|---|
| Verkeers-/waarschuwingsborden in het district (`20_world_dressing_standard.md` §20.2) | **Ja, onveranderd.** Vier stuks in één blik: een rood verbodsbord links, een zwart bord met een rode cirkel ernaast, een rood bord met stralingsembleem op de poortbalk, een geel waarschuwingsbord met figuur. | `HUD_wapen_B`, `HUD_wapen_F` |
| Geen wapen in 1e persoon | **Ja in `_F`.** In `_B` staat bij een niet te onderscheiden camerastand wél een wapen in beeld — zie de nieuwe regel voor de tegenspraak met de meting in O-9. Niet als opgelost lezen. | `HUD_wapen_F` vs `HUD_wapen_B` |
| Wapen wordt omgekeerd vastgehouden (owner, OBS-1) | **Beeldbevestiging:** in `_B` wijst de loop schuin omhoog naar rechts in plaats van vooruit. Kanttekening: er is in dat frame geen hand — "vastgehouden" is te genereus. | `HUD_wapen_B` |
| Camera door het lichaam bij de wissel 3e↔1e | **Ja, en breder dan gedacht.** Niet alleen bij de wissel: in vijf van de zeven frames staat de camera in het lichaam, ook in rust. | `HUD_wapen_A/_C/_D/_E/_G` |
| Squadlijst toont `45434C53` | **Nee — weg.** Er staan echte namen. Zie "Wat er goed is". | alle zeven frames |
| GPU-crash (SkyAtmosphere page fault) | Geen nieuw voorval in de 20:20-ronde. | — |
| "Missing Eclipse Modules" | Geen nieuw voorval; de dialoog van 19:53 zei letterlijk "missing **or built with a different engine version**" — de tweede helft van die zin was de juiste. | `Schermafbeelding 195356` |

### Stand in de ronde van 01-08 (23:34-HUD + 22:15-glyph)

| Bekend punt | Nog zichtbaar? | Waar |
|---|---|---|
| Camera in het lichaam bij 3e persoon | **Nee — weg.** Zes van zes geopende 3e-persoonsframes: camera achter en boven, hele figuur vrij in beeld. | `HUD_3e_persoon`, `_3e_persoon_terug`, `_herladen_0900ms`, `_herladen_klaar_5000ms`, `_kruis_ondergrond_0/2`, `_wapen_D` |
| Teller verdwijnt tijdens herladen | **Nee — weg.** `RELOADING` staat nu bóven `AR Foundry 18 / 30`. | `HUD_herladen_0900ms`, `_klaar_5000ms` |
| `HERLADEN` (NL) tussen Engelse tekst | **Nee — weg.** Alles Engels. | alle tien |
| Asset-id `Sidearm_Scrap` op het scherm | **Ja, maar nog in één frame van de tien.** De andere negen tonen `AR Foundry`. | `HUD_wapen_F_eerste_persoon` |
| Geen wapen/handen in 1e persoon | **Ja** in beide volledige 1e-persoonsframes — **maar** in de overgangsstand staan wapen én arm er wél. Zie de nieuwe regel; dit is nu een offset, geen ontbrekende mesh. | `HUD_1e_persoon`, `HUD_wapen_F` vs `HUD_wissel_midden` |
| Wapen omgekeerd vastgehouden (OBS-1) | **Ja, en nu leesbaar.** Kolf naar buiten, loopzijde de romp in, geen hand om de greep. | `HUD_wapen_D_los_aan_hand` |
| Richtkruis is een klein wit plusje zonder omlijning | **Ja, onveranderd.** Geteld: **129 pure witte pixels binnen een blok van 28×28 px — 0,014% van het beeld**, zonder rand of schaduw. | alle tien |
| Geen veilige marge aan de randen | **Grotendeels weg.** 37/37/38/44 px in plaats van 2 px links. Horizontaal nog binnen 5%. | `HUD_1e_persoon` |
| Aardse borden in het district (§20.2) | **Ja, maar verschoven van vlak naar vorm.** De *pictogrammen* zijn nu Radiance-zon-gebaseerd en op standaard; wat overblijft is de **draagvorm** (gele ruit op een paal) en de **installatie** (paallamp + rood/geel paneel + hangkabel). Drie aparte regels in de tabel. | `NA_cam6_crossing` |
| Zwarte ondergrondplaat achter de Eclipse-sigil | **Nee — weg.** Zie "Wat er goed is". | `_c4_NA_stencil`, `_c5_NA_stencil`, `_w6_NA` |
| Plafond rendert absoluut zwart (vault) | **Ja, en breder.** Vierde ruimte erbij: `00036` meet 0,0000 met max 0 over 90.000 px. | `HighresScreenshot00036` |

---

## Wat er goed is

Een lijst die alleen fouten bevat laat niet zien of er iets vooruitgaat. Vanavond ging er iets vooruit.

1. **De squadlijst toont echte namen.** `Sef Voss`, `Sef Chen`, `Anke Stahl` — de GUID-kop `45434C53` komt in geen van de zeven frames van 20:20 nog voor. Dit is de duidelijkste vooruitgang van de ronde.
2. **Er staat een munitieteller, en hij is leesbaar.** `Sidearm_Scrap   12 / 12`, wit op donker asfalt, in één oogopslag te vinden. De naam ernaast is fout (zie tabel), het getal niet.
3. **Herladen wordt gecommuniceerd**, en in een eigen kleur (oranje) die duidelijk verschilt van het wit van de teller. Je ziet zonder lezen dát er iets anders aan de hand is.
4. **De objectives-lijst is echte spelinformatie, geen placeholder.** Drie doelen, waarvan één expliciet `(optional)`, plus per squadlid de orderstand (`-> Hold`) en een statusregel `command: idle · doctrine: ready`. Dat is precies wat een squadgame moet tonen.
5. **Het richtkruis staat er, en staat dood in het midden**, in beide perspectieven. Klein en te wit (zie tabel), maar aanwezig en correct geplaatst.
6. **Het wapen bestaat en rendert.** `_B` bewijst dat de mesh, het materiaal en de draw call werken — waar dat wapen precies hoort te staan is nog open (zie de tabel), maar "er is niets" is het in elk geval niet.
7. **De scène leest in drie dieptelagen** (§20.7): wegmarkering op de voorgrond, muur met borden op middenafstand, poortbalk en luchtgradiënt op de achtergrond.

### Ronde 01-08 — wat er tussen 20:20 en 23:34 echt is opgelost

Vijf punten uit de vorige ronde zijn **weg**, geverifieerd op beeld, niet op belofte:

8. **De camera staat niet meer in het lichaam.** Dit was het punt dat vijf van de zeven testframes
   waardeloos maakte. Nu staat de figuur vrij in beeld — en pas dáárdoor kon ik het wapensilhouet
   überhaupt lezen. Een fix die andere metingen mogelijk maakt telt dubbel.
9. **Herladen en de teller staan er tegelijk.** `RELOADING` bovenop, `AR Foundry 18 / 30` eronder.
10. **De schermtekst is nu één taal.**
11. **De randmarges zijn van 2 px naar 37 px gegaan.**
12. **Het munitiepaneel heeft een eigen donkere achtergrondplaat** rechtsonder. Dat is precies de
    oplossing die de statusregel bovenaan nog míst — het middel bestaat dus al in dit bestand.

En uit de glyphronde:

13. **De ondoorzichtige zwarte plaat achter de Eclipse-sigil is weg.** In `_c4_VOOR_stencil` en
    `_c5_VOOR_stencil` plakt de glyph op een zwarte rechthoek; in `_c4_NA_stencil`, `_c5_NA_stencil`
    en `_w6_NA` spuit hij door op de muur, mét druipsporen. Een VOOR/NA-paar dat zijn werk doet.
14. **De pictogrammen zelf zijn op standaard.** Ik heb het rode en het amberen teken op 6× bekeken:
    beide zijn rond de Radiance-zon gebouwd, met een eigen glyphrij eronder. Er zit geen aards
    symbool in het beeldvlak van die twee borden. Dat is de moeite waard om te noteren, want de vorige
    ronde ging juist mis op assets die op naam onschuldig leken.

---

## Hoe dit werkt

`screenshot-inspector` draait **aan het begin van elke werkcyclus**, kijkt naar de beelden die nog niet in de tabel staan, en vult aan. Twee mappen:

- `Eclipse/Saved/Screenshots/WindowsEditor` — wat de agents zelf maken
- `C:\Users\natha\Pictures\Screenshots` — wat Nathan met Win+PrtSc vastlegt, dus waar de **foutmeldingen** staan

De eerste acht regels hierboven zijn met de hand ingevuld door de hoofdsessie op 31-07, als startpunt. Vanaf nu gaat het vanzelf.
