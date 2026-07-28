# Prompt: schietgevoel bouwen zoals de demo's

*Dit bestand is kort gehouden, en dat is de hele pointe. De demo's die je zag
draaiden op ongeveer 60 woorden instructie. Alles wat je hier extra bijplakt
gaat af van de aandacht voor het probleem zelf.*

---

## Waarom de demo's lukten en Eclipse niet

De demo's zijn **geen Unreal**. Bewijs staat in de screenshots zelf: "built with
Three.js", een Chrome-tab op `localhost:8080/fps.html`, en "a painterly world in
one HTML file". Het zijn losse HTML-bestanden in een browser.

Dat is geen detail — het is het hele verschil:

| | HTML-demo | Eclipse (UE5) |
|---|---|---|
| Wie schrijft de wereld | het model, 100% | het model schrijft ~5%, de rest is engine + assets |
| Wapenpositie | drie getallen in de code die het net schreef | een socket op een skeleton in een editor |
| Textuur | procedureel, in code | een `.uasset` |
| Terugkoppeling | screenshot in 10 seconden, zelf bekijken | build (minuten) → editor → jij moet kijken |
| Aantal iteraties per uur | tientallen | een handvol |

Schietgevoel is puur een terugkoppel-probleem. Het is niet uit te denken, alleen
uit te proberen. In de demo's zit die lus dicht; in Eclipse is hij open aan beide
kanten. **Daarom lukt het daar in één prompt en hier niet in 24 uur.**

## Wat er in Eclipse feitelijk ontbreekt

Uit `EclipseHitscanWeaponComponent.cpp` (202 regels) en een zoekactie over alle
39.424 regels source:

- **Geen wapenmesh.** Geen `WeaponMesh`, geen muzzle-socket, geen attach naar
  `hand_r`. De trace vertrekt vanaf `ViewLocation` — de camera.
- **Geen tracer, geen mondingsvuur, geen camera-shake.** Nergens in de source.
- Uit je eigen `GEVECHT_AUDIT.md`: geen terugslag (9), geen hitmarker (4), geen
  richtingsindicator (14), geen munitie (10).

Je audit concludeert het zelf: *"vier van de vijf omissies gingen over FEEDBACK,
niet over mechaniek."* Het gevecht rékent goed. Er is alleen niets te zien.

**Dat is dus geen promptprobleem.** Het is een lijst bouwbare dingen, en die
lijst heb je al.

---

## PROMPT A — bouw de referentie in de browser (doe deze eerst)

Plak dit in een verse chat. Buiten de Eclipse-repo, in een lege map.

```
Bouw één HTML-bestand: een first-person schietbaan in Three.js, geen externe
assets, alles procedureel in code.

Doel: het SCHIETGEVOEL, niets anders. Een gang, wat dekking, drie doelen die
omvallen. Geen menu, geen missie, geen AI.

Deze getallen liggen vast (uit mijn eigen ontwerp):
- vuursnelheid 7 schoten/s
- speler doodt vijand in ~0,6 s goedgericht, kopschot ×2,5
- vijand doodt speler in ~2,5 s
- FOV 90, mikken maakt je 2,86× trager in het kijken
- looptempo 4,2 m/s, mikken zet dat op 1,45 m/s
- input naar beeld onder 100 ms

Alles wat gevoel maakt, wil ik als benoemde constanten bovenaan het bestand,
met een commentaarregel per constante over wat hij doet:
wapenpositie t.o.v. de camera, ADS-positie, terugslag (verticaal, horizontaal,
hersteltijd), spreiding heup vs. mikken, camera-shake, mondingsvuur-duur,
tracer-snelheid, hitmarker-vorm en -duur, hit-reactie van het doel,
hulzenuitworp, ademhaling-sway.

Werkwijze, en hier ligt het echte werk:
1. Bouw het.
2. Open het in een headless browser, maak screenshots van: heupvuur, ADS,
   het frame van het schot, het frame van de treffer.
3. KIJK naar je eigen screenshots. Benoem wat er goedkoop uitziet.
4. Stel de constanten bij en herhaal.

Doe stap 2-4 minstens acht keer voordat je me iets laat zien. Rapporteer per
ronde in één regel wat je zag en wat je veranderde. Ik wil de eindtabel met
alle constanten en hun uiteindelijke waarde.
```

Waarom dit werkt: elke knop is een getal in een bestand dat het model zelf
schreef, en het kan het resultaat bekijken. Reken op een half uur tot een uur.

## PROMPT B — de getallen naar Unreal brengen

Pas nadat A een tabel heeft opgeleverd. In de Eclipse-repo.

```
Lees ALLEEN deze drie: phase0/GEVECHT_AUDIT.md, phase0/graybox_feel_targets.md,
Eclipse/Source/Eclipse/Combat/EclipseHitscanWeaponComponent.cpp.
Niet HANDOFF.md, niet de GDD-delen. Ik heb ze gelezen, jij hebt de ruimte nodig.

Hieronder staat een tabel met afgestelde gevoelsconstanten uit een werkende
browser-prototype. [PLAK TABEL UIT PROMPT A]

Opdracht: breng punt 4, 5, 7, 9 en 14 van GEVECHT_AUDIT.md naar de build, met
deze getallen als startwaarde. Dus: hitmarker, kopschot-signaal, terugslag,
schutter-reactie, richting van de klap.

Harde eis: elk van deze constanten wordt een veld in DT_Weapons of een nieuwe
DT_WeaponFeel — geen enkele hardcoded (14.2). Ik moet ze in de editor kunnen
draaien zonder build.

Tweede harde eis, en dit is de belangrijkste: breid EclipseFeelHarness.cpp uit
zodat het deze vijf MEET, net zoals het nu locomotie meet. Terugslag in graden
na drie schoten, spreidingsdiameter op 20 m heup vs. ADS, hitmarker-duur in
frames, tijd tussen trekker en zichtbaar signaal. Draai het en zet de gemeten
waarden naast de doelwaarden.

Bouw met -NoUba. Eindig groen. Commit per systeem.
```

## Wat er nog vóór B moet gebeuren — de wapenmesh

Punt 9 (terugslag) en jouw vraag over "positie en textuur" lopen allebei stuk op
hetzelfde: **er is geen wapen in de wereld.** Geen mesh, geen socket, geen
muzzle. Een agent kan die twee niet plaatsen, want plaatsen gebeurt in de editor.

Dat is een owner-actie, en het is een kwartier werk:

1. Zet één wapenmesh op de `hand_r`-socket van het speler-skeleton.
2. Voeg een socket `Muzzle` toe op de loop.
3. Maak `DT_WeaponFeel` met twee rijen offsets: heup en ADS (locatie + rotatie).

Daarna kan de agent de positie wél afstellen — want dan zijn het zes getallen in
een data-table in plaats van een handeling in een venster dat hij niet ziet.

Textuur is hetzelfde verhaal, één laag dieper: in de demo is de textuur code
(canvas-ruis naar roughness/normal). In UE is het een asset. Laat de agent het
materiaal procedureel opzetten via `Tools/author_toon_material.py` — je hebt dat
script al — zodat parameters weer getallen worden.

## De maat van de rode draad

Buiten `Eclipse/` staat ~800 KB markdown. `HANDOFF.md` alleen is 101 KB, en er
liggen 41 documenten in `phase0/`. Een agent die volgens `NIEUWE_CHAT_PROMPT.txt`
begint, leest eerst over de economie-ledger en de story-bible en komt daarna pas
bij terugslag.

Een rode draad van die lengte geeft geen richting meer, hij verdunt. Voor een
gevoelstaak is `GEVECHT_AUDIT.md` in zijn eentje meer waard dan alle 17
GDD-delen samen — want die staat vol benoemde ontbrekende dingen, en de GDD staat
vol intentie. Intentie is al binnen. Zeg voor gevoelstaken expliciet wélke drie
bestanden gelezen mogen worden, zoals in prompt B hierboven.

---

## Samengevat

1. De demo's zijn browser-HTML. Eén prompt lukt daar omdat het model zijn eigen
   resultaat kan zien.
2. Geef Opus diezelfde lus: bouw het schietgevoel eerst in de browser, met
   screenshots als terugkoppeling.
3. Neem alleen de getallen mee naar Unreal, en meet ze daar met het harnas dat je
   al hebt.
4. Zet zelf één wapenmesh en één muzzle-socket klaar. Zonder dat kan geen enkele
   prompt het schietgevoel raken.
5. Beperk de leeslijst per taak tot drie bestanden.
