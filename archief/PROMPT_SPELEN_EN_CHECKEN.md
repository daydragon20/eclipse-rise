# Prompts om te plakken

Twee kant-en-klare prompts voor de dev-sessie (VS Claude of een verse chat).
Kopieer de hele blok tussen de lijnen.

---

## 1 · "Is de code écht in orde, en geef me een speelbare build"

Gebruik deze als je wil wéten dat alles klopt én daarna gewoon wil spelen.

```
Doe een volledige gezondheidscheck van ECLIPSE en lever me daarna een speelbare build op.

DEEL A — bewijs dat de code klopt (geen samenvattingen, echte output):
1. Build de editor-target (-NoUba) en toon me de laatste 15 regels.
2. Draai de volledige headless testsuite. Noem het exacte aantal: X/Y geslaagd, en
   als er iets faalt: de naam van elke gefaalde test + waarom.
3. Draai EclipseValidateData (alle validators) en het event-catalog-script.
   Rapporteer per validator het aantal fouten — 0 of het echte getal.
4. Toon `git status --porcelain`. Als er ongecommit werk staat: zeg per bestand
   of het af is of halverwege, en of het veilig is om mee te spelen.
5. Eén eerlijke conclusie: is de huidige staat speelbaar, ja of nee, en wat is
   het zwakste punt dat ik ga merken tijdens het spelen?

DEEL B — zet hem speelklaar:
6. Zorg dat GrayboxDistrict laadt met alles wat er nu in zit (squad van 4,
   Command Mode Stage A, de gedresste omgeving).
7. Start hem één keer zelf op zoals de fotoronde dat doet:
   -game -windowed -resx=1280 -resy=720 -EclipseShot
   en toon me 4 verse screenshots, zodat ik zie wat ik ga krijgen.
8. Bevestig dat SPEEL_ECLIPSE.bat in de repo-root werkt met de huidige build.
   Werkt hij niet, fix hem dan.

REGELS:
- Als iets niet werkt: zeg het plat, verzin geen "waarschijnlijk werkt het wel".
- Geen enkele stap overslaan omdat hij "vorige keer groen was".
- Ik wil de echte getallen zien, niet "alles staat groen".
```

---

## 2 · "Maak het openstaande werk af" (bij een vastgelopen wachtlus)

Gebruik deze als de sessie blijft herhalen *"Wacht op de workflow / Nog in afwachting"*.
Druk eerst **Esc** om de lus te onderbreken.

```
STOP met wachten op de workflow — er draait niets meer. De agents waar je op wacht
zijn gestorven op een limiet; er staat geen build- of editorproces meer open.

Doe dit in plaats daarvan, zelf, serieel, zonder subagents:
1. Inventariseer het ongecommitte werk in de werkboom (26 bestanden, waaronder de
   nieuwe EclipseVaultBuilder.cpp/.h, setup_liberation_data.py en MH_FACE_TIER_B.md).
   Zeg per changeset: af, halverwege, of kapot.
2. Maak de halfafgemaakte stukken af — walkable vault (P2-03 stap 4-5B) en de
   P2-05-liberation-wiring — of zet ze terug naar een compileerbare staat.
3. Groene bar vóór elke commit: build ✓, volledige tests ✓, validatie 0 ✓, catalog ✓.
4. Commit per systeem met de gebruikelijke prefix en push.
5. Werk HANDOFF.md en progress_data.js bij met de echte stand.

Als een stap niet lukt: stop, zeg wat er misging, en wacht op mij. Ga niet opnieuw
in een poll-lus zitten.
```

---

## 3 · "De env-packs staan erin" — de graphics-fase vrijgeven

Dit is de prompt die P2-08 (het fidelity-district) losmaakt. Plak hem zodra de
downloads klaar zijn.

```
De env-packs staan erin. Geverifieerd op schijf in Eclipse/Content:

- Factory_Pack_V1      2,1 GB, 408 assets
- Uniblocks            3,7 GB, 3803 assets
- IBuilding_49         compleet (mesh + AO/BaseColor/Metallic/Normal/Roughness)
- LPCharacters_FREE    16 MB, 25 assets (bonus character-pack)

DOE DIT, in deze volgorde:

1. EERST AFMAKEN WAT OPEN STAAT. Er staat ongecommit werk in de werkboom
   (o.a. EclipseVaultBuilder.cpp/.h, setup_liberation_data.py, MH_FACE_TIER_B.md).
   Inventariseer per changeset: af, halverwege of kapot. Maak af of zet terug naar
   een compileerbare staat. Commit per systeem met groene bar. Geen nieuw werk
   starten voordat de boom schoon is.

2. INVENTARISEER DE NIEUWE PACKS zoals bij de vorige curatierondes:
   tri-counts, materiaal-slots, licentie/provenance in SOURCES.md, en of de meshes
   door de toon-pijplijn kunnen. Rapporteer wat bruikbaar is en wat niet — geen
   enkel pack raw in de scène.
   Controleer ook of Sci-Fi Hallway, Sci-Fi Light Pack, Auto Footsteps Utility,
   Niagara Footstep VFX, FPS Weapon Bundle en Free Muzzle Flash binnen zijn; ik zie
   ze niet als aparte Content-map. Zo niet: zeg het, dan klik ik ze alsnog.

3. START DE KIT-PASS (backlog #10, SPEC-P2-08). De owner-afhankelijkheid bij P2-08
   is hiermee vervuld. Prioriteitsladder uit de laatste art-review geldt nog:
   (1) licht+vloer-pass, (2) toon-master over de plaza-vloer, (3) dock-naad,
   boulder-clip, machine-faces, buttresses.
   Vervang graybox-blokken door echte kits waar de ruimtes al vastliggen; waar ze
   nog niet vastliggen (missie-sites uit P2-04), wacht je.

4. SHOTRONDE + ART-REVIEW vóór elke commit, per 15.8. Toon mij de shots op de vaste
   camera's — eerste PNG overslaan (warm-up-offerframe).

5. Werk HANDOFF.md en progress_data.js bij. In progress_data.js mag de Old West-taak
   uit ownerActies: die is klaar (vault-cache opgeruimd, ~5 GB vrij).

REGELS: groene bar vóór elke commit (build ✓, volledige tests ✓, validatie 0 ✓,
catalog ✓). Geen poll-lus — als iets niet lukt, stop en zeg het.
```

---

## 4 · Objectives testen (missies nalopen zonder de hele loop te spelen)

```
Ik wil de missie-objectives kunnen testen zonder telkens de hele campagne te spelen.

1. Zet me een snelle testroute op: welke console-commando's of debug-flags bestaan
   er al om een missie direct te starten, objectives te forceren en een debrief te
   triggeren? (Eclipse.Command.Dump bestaat al — wat is er nog meer?)
2. Documenteer die in phase0/TESTROUTE_OBJECTIVES.md: per missie welke stappen,
   welk commando, en wat ik hoor te zien als het klopt.
3. Als er nog geen snelle route is, bouw er dan één als debug-commando — in de
   bestaande debug-laag (14.5), geen nieuwe systemen.
4. Zeg me daarna in gewone taal hoe ik M1.1 "Thirteen Bullets" van start tot debrief
   naloop met SPEEL_ECLIPSE.bat.
```

---

## 5 · Zelf spelen zonder agent

Dubbelklik **`SPEEL_ECLIPSE.bat`** in `C:\Dev\ECLIPSE_GDD`.

Dat start de game als echte standalone build, direct op Kessara — geen editor,
geen lege Entry-map, muis meteen gevangen. Dat was precies waarom je controls
eerder niets deden: de editor opende de lege startmap.

| Toets | Actie |
|---|---|
| WASD / muis | lopen / rondkijken |
| Shift / Ctrl | sprint / hurken |
| Linkermuis | vuren |
| **Q vasthouden** | Command Mode — wereld naar 30% |
| 1-4 | orders geven |
| Tab of scroll | soldaat kiezen |
| E | soldaat onder je richtkruis |
| Shift+F1 | muis vrijgeven |
| Esc | afsluiten |

Werkt de .bat niet, dan is de build verouderd → gebruik prompt 1, deel B.
