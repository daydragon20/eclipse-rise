# BEVINDINGEN UIT SCREENSHOTS
*Eigenaar: `screenshot-inspector`. Vul aan, verwijder nooit — een afgehandelde bevinding krijgt `opgelost` in de laatste kolom.*
*Doel: Nathan hoeft niet zelf te melden wat er op zijn scherm mis is.*

**Beoordeeld tot en met:**
- Nathans map: t/m `Schermafbeelding 2026-07-31 195356.png` — alle vijf van de avond zijn nu ook echt bekéken, niet alleen overgenomen.
- `Eclipse/Saved/Screenshots/HUD_volledig/`: de zeven `HUD_wapen_*`-frames van 20:20.
- Nog niet aan de beurt: `HUD_volledig/HUD_1e_persoon`, `_3e_persoon`, `_wissel_midden`, `_3e_persoon_terug` (de 20:20-herschoten van al beoordeelde standen) en de `WindowsEditor/HighresScreenshot*`-serie.

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
| 2026-07-31 20:20 | `HUD_wapen_A_ingebouwd`, `_C_alleen_wapensectie`, `_D_los_aan_hand`, `_E_na_wissel`, `_G_BEIDE_tegenproef` | blokkeert | Een roodbruin vlak — huid/romp van het personage — vult 60 tot 95% van het beeld. Alleen aan de linkerkant blijft een strook wereld over. In `_G` staat de romp rechts, in de andere vier in het midden. | De camera staat ín de karaktermesh. Vermoedelijk een te korte spring-arm, of camerabotsing die niet uitduwt. **Gevolg voor het werk: vijf van de zeven wapen-testframes bewijzen niets** — je kunt het wapen er domweg niet op zien. Alleen `_B` en `_F` zijn bruikbaar bewijs. |
| 2026-07-31 20:20 | `HUD_wapen_A/_B/_C/_D/_G` tegenover `_F` | fout | Tijdens herladen staat rechtsonder alléén `HERLADEN` in oranje. De teller `12 / 12` die in `_F` op diezelfde plek staat is dan wég — je ziet tijdens het herladen niet meer hoeveel je had, noch hoe groot het magazijn is. | **Bevestigd in code:** `EclipseMissionHudWidget.cpp:181` zet de héle tekst van `AmmoReadout` op "HERLADEN" en keert terug. Eén tekstveld voor twee dingen die je tegelijk wilt weten. |
| 2026-07-31 20:20 | `HUD_wapen_F_eerste_persoon.png`, `HUD_wapen_E_na_wissel.png` | fout | Rechtsonder staat `Sidearm_Scrap   12 / 12`. De speler leest een asset-id mét underscore, geen wapennaam. | **Bevestigd:** `EclipseMissionHudWidget.cpp:190-193` drukt `GetActiveWeaponName()` af, en dat is de rijnaam uit de wapentabel. Zelfde soort lek als `45434C53` in de squadlijst: een intern id dat op het scherm belandt. |
| 2026-07-31 20:20 | `HUD_wapen_E_na_wissel.png` | fout | De wapenregel loopt van het scherm af: `Sidearm_Scrap` loopt door tot in de allerlaatste beeldkolom en de `12 / 12` erachter is niet meer in beeld. In `_F` past diezelfde regel wél, en eindigt hij ~34 px vóór de rand. | Vermoedelijk rechts uitgelijnde tekst zonder rechtermarge of clip, zodat een langere string over de rand schuift. Dat het uitgerekend in het frame *na de wissel* gebeurt maakt het erger: precies op het moment dat je moet zien wát je nu vasthebt. |
| 2026-07-31 20:20 | alle zeven `HUD_wapen_*`-frames | fout | Taalmix in dezelfde schermlaag: `HERLADEN` (Nederlands) staat naast `MISSION ACTIVE`, `Spring the ambush`, `Extract the squad`, `squad orders`, `command: idle`, `doctrine: ready` (Engels). | Géén smaakvraag: de brontaal is Engels — `13_roadmap.md` r45 noemt NL juist als een van de **doel**talen van de lokalisatie. `HERLADEN` staat als Engelse brontekst hardgecodeerd in `NSLOCTEXT("Eclipse", "Reloading", "HERLADEN")`. |
| 2026-07-31 20:20 | alle zeven `HUD_wapen_*`-frames | fout | Geen veilige marge aan de randen. `== MISSION ACTIVE` begint op ~2 px van de linkerrand, de kopregel raakt de bovenrand, de wapenregel zit ~34 px van de onderrand. Niets staat binnen een title-safe zone. | HUD-elementen op anker 0 zonder inset. Op een tv met overscan, of in een andere beeldverhouding, vallen de eerste tekens weg. Dit is nu al te zien: in `_E` is de wapenregel er feitelijk al deels af. |
| 2026-07-31 20:20 | `HUD_wapen_F` (op het gele bord), `_A/_C/_D/_G` (op de romp) | stijl | Het richtkruis is een wit plusje van een paar pixels. Op het gele waarschuwingsbord in `_F` en op het roodbruine lichaam in de andere frames is het nauwelijks terug te vinden. | Geen omlijning of contrastrand onder het richtkruis. Op een lichte of drukke achtergrond verdwijnt wit in wit — en het richtkruis is het enige element dat je élke seconde nodig hebt. |
| 2026-07-31 20:20 | `HUD_wapen_F_eerste_persoon.png`, `HUD_wapen_B_zonder_wapensectie.png` | fout | Losse lichaamsgeometrie aan de beeldranden in 1e persoon: in `_F` twee oranje/tan vormen in de linkeronderhoek, in `_B` twee tan vormen rechtsboven tégen de lucht, nergens aan vast. Zelfde kleur als de romp in de 3e-persoonsframes. | Vermoedelijk delen van de karaktermesh die wél gerenderd worden, maar op de verkeerde plek rond de 1e-persoonscamera staan. Hangt vermoedelijk samen met de camera-in-lichaam-regel hierboven: het lichaam wordt niet uitgezet, het staat verkeerd. |
| 2026-07-31 20:20 | alle zeven `HUD_wapen_*`-frames | stijl | De squad heet `Sef Voss`, `Sef Chen`, `Anke Stahl` — twee van de drie hebben dezelfde voornaam. | `EclipseRosterLogic.cpp:21` trekt per soldaat een willekeurige voornaam uit de pool zónder te kijken wat de vorige soldaat al kreeg. Bij een squad van drie valt een dubbele voornaam meteen op en leest als een bug, ook als hij statistisch gewoon kan. |
| 2026-07-31 20:20 | `HUD_wapen_F_eerste_persoon.png`, `HUD_wapen_B_zonder_wapensectie.png` | stijl | Vier identieke donkere pijlers naast elkaar in de poort, plus twee identieke rotsblokken rechts in beeld. | `20_world_dressing_standard.md` §20.7: dezelfde mesh drie keer in beeld. Bij een poortcolonnade is herhaling deels bedoeld — maar de pijlers zijn onderling ook niet gedraaid of verschillend verweerd, dus het leest als kopieerwerk in plaats van als architectuur. |

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

---

## Hoe dit werkt

`screenshot-inspector` draait **aan het begin van elke werkcyclus**, kijkt naar de beelden die nog niet in de tabel staan, en vult aan. Twee mappen:

- `Eclipse/Saved/Screenshots/WindowsEditor` — wat de agents zelf maken
- `C:\Users\natha\Pictures\Screenshots` — wat Nathan met Win+PrtSc vastlegt, dus waar de **foutmeldingen** staan

De eerste acht regels hierboven zijn met de hand ingevuld door de hoofdsessie op 31-07, als startpunt. Vanaf nu gaat het vanzelf.
