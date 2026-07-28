# Feel-referenties

*Sluit het openstaande punt uit `phase0/graybox_feel_targets.md` §5: "Lock the
targets against footage, not memory." Elke referentie krijgt daar de eis mee van
één annotatie —* wat we nemen, wat we bewust niet nemen. *Die staat hier per
beeld, want zonder die tweede helft is een referentie gevaarlijk.*

---

## De waarschuwing die vooraf gaat

**Vier van de zes referenties hieronder overtreden de ÉÉN-STIJL-WET.** Ze zijn
fotorealistisch of first-person; Eclipse is Borderlands-leaning toon en
third-person over-shoulder (15.5, locked).

Ze liggen hier omdat ze iets anders bewijzen: **compositie, HUD-indeling,
wapenplaatsing en gevoelstiming.** Dat zijn stuk voor stuk zaken die los staan
van de renderstijl.

Wie een van deze beelden aan een agent geeft zonder de "NIET"-kolom erbij, krijgt
een fotoreal-inslag in een toon-game terug — en `art-reviewer` keurt dat terecht
af als blokkerend. **Geef altijd beide kolommen mee.**

---

## De bestanden

Leg ze hier neer onder exact deze namen; de prompts in `PROMPT_SCHIETGEVOEL.md`
verwijzen ernaar.

| Bestand | Bron |
|---|---|
| `REF-01_snowboarder_thirdperson.png` | Opus-demo, snowboarder-physics |
| `REF-02_threejs_interior.png` | Opus-demo, 3D-woning in Three.js |
| `REF-03_subway_fps_hud.png` | Opus-demo, subway-FPS |
| `REF-04_strategy_map_ui.png` | Opus-demo, "Perilous Ways" spoorwegspel |
| `REF-05_shooter_hipfire.png` | Opus-demo, marktshooter, heupvuur |
| `REF-06_shooter_ads.png` | Opus-demo, marktshooter, gemikt |
| `REF-07_painterly_world.png` | Opus-demo, painterly wereld in één HTML |

---

## REF-01 — snowboarder, third-person

**WEL nemen.** De cameraplaatsing. Dit is de enige referentie in de reeks die
Eclipse' eigen camera heeft: third-person, personage links-van-midden en laag in
het kader, met tweederde van het beeld voor de wereld. Vergelijk met de
`FOV 90 / over-shoulder`-regel uit `graybox_feel_targets.md` §2 — dit is hoe die
eruit hoort te zien.

Ook: dit is gestileerd zónder low-poly te zijn. De bergen zijn geband en
geschilderd, niet gefacetteerd. Dat is precies de scheidslijn die 15.5 trekt.

**NIET nemen.** Het sneeuwpalet en de zachte, ronde silhouetten. Eclipse is een
verzetsoorlog; deze toon is vriendelijk.

## REF-02 — Three.js-interieur

**WEL nemen.** Twee dingen. Ten eerste de lichtplaatsing: praktische lampen in
het beeld (afzuigkap, onderkastverlichting) doen al het werk, en de rest van de
ruimte mag donker blijven. Dat is goedkoper én leest beter dan een gelijkmatig
verlichte kamer — relevant voor Hollow Point.

Ten tweede, en belangrijker voor de werkwijze: **linksonder staat de
knoppenlegenda, rechtsonder `60 fps · 1 draws`.** Die perf-uitlezing hoort
permanent in beeld te staan tijdens elke agent-sessie. Zonder dat cijfer op het
scherm kan de agent een screenshot niet naast de 12.4-budgetten leggen.

**NIET nemen.** De fotoreal-materialen. Tegels, roestvrij staal en de koelkast
zijn PBR-realistisch; die moeten bij ons door `author_toon_material.py`.

## REF-03 — subway-FPS, HUD-indeling

**WEL nemen.** De volledige feedback-indeling, want dit is het antwoord op de
drie openstaande punten uit `GEVECHT_AUDIT.md` (4, 5, 14):

- middenboven: golf + resterende vijanden — leest zonder de blik te verplaatsen
- rechtsboven: kills / headshots / **accuracy in procenten**
- linksonder: conditie-balk met het cijfer eronder
- linksboven: live instelbare sliders
- daaronder: `66 fps · 907 draws · 6 active`

Die accuracy-teller is de interessantste. Hij maakt trefferfeedback *meetbaar
over een hele sessie* in plaats van per schot — precies wat je harnas mist. Een
agent die veertig tuning-rondes draait kan daarop sturen.

En de sliders linksboven zijn visueel wat `Eclipse.Feel.Set` functioneel moet
worden: waarden bijstellen zonder de game te verlaten.

**NIET nemen.** De first-person camera en het fotoreal-tunnelwerk. De HUD-indéling
is overdraagbaar naar third-person; het cameraperspectief niet.

## REF-04 — strategiekaart

**WEL nemen.** De paneelverdeling, en dit is de referentie voor je Command Mode
en de strategielaag, niet voor het gevecht:

- bovenbalk: alle harde getallen op één rij (week, crowns, prosperity, concord)
- links: staande orders bovenaan, daaronder de vloot, daaronder relaties
- rechts: het register, inklapbaar
- middenboven: **één regel die zegt wat je nu moet doen** — "Click a settlement,
  then another"

Die ene regel is de goedkoopste UX-ingreep in het hele beeld en de meest
overdraagbare. Command Mode heeft geen tutorial nodig als er altijd één regel
staat die de volgende handeling benoemt.

**NIET nemen.** Perkament, serif-typografie, de fantasy-toon. Eclipse' UI-taal
ligt vast in 15.5 en die is industrieel.

## REF-05 / REF-06 — marktshooter, heupvuur en gemikt

**Dit paar is de kern van de vraag over "positie en textuur".** Ze horen samen
bekeken te worden, want het verschil tússen de twee beelden is het antwoord.

**WEL nemen — de wapenplaatsing als getallen:**

| | REF-05 (heup) | REF-06 (gemikt) |
|---|---|---|
| Wapen in het kader | rechtsonder, ~35% van het beeld | naar het midden, kolf gecentreerd |
| Loop wijst | schuin omhoog-links | recht vooruit, vizier op het midden |
| Zichtbaarheid wereld | vrijwel volledig | onderste derde afgedekt |
| Handen zichtbaar | beide, ontspannen | beide, strak tegen het lichaam |

Dat zijn de twee offsets die `DT_WeaponFeel` nodig heeft. De overgang ertussen —
hoe lang hij duurt en welke curve hij volgt — is het enige wat een screenshot
níet geeft, en precies waarvoor de frame-strips uit `PROMPT_SCHIETGEVOEL.md` §2
bestaan.

**WEL nemen — de HUD-ankers:** munitie rechtsonder groot (`30`), gezondheid
linksonder (`100`), kompas middenboven. Alle drie in de hoeken, geen enkele in
het midden waar het kruis staat.

**WEL nemen — de materiaalgedachte, niet de uitvoering.** Het wapen heeft
slijtage op de randen: de greep is lichter waar hij aangeraakt wordt, de loop
donkerder. Dat randslijtage-idee is stijl-onafhankelijk en overleeft de vertaling
naar toon-shading prima. Een vlak wapen leest als een prop; een gesleten wapen
leest als gereedschap.

**NIET nemen.** De first-person camera — Eclipse is third-person, dus deze
offsets zijn een *startpunt* dat over-shoulder opnieuw afgesteld moet worden, geen
eindwaarde. En niet de fotoreal-rendering: zand, beton en metaal zijn hier PBR.

## REF-07 — painterly wereld

**WEL nemen.** Dit is het belangrijkste beeld in de map, want het weerlegt de
aanname waar 15.5 expliciet tegen waarschuwt: *"toon = cheap" is afgewezen.*

Hier is gestileerd én dicht. Het gras is volledig gesimuleerd met wind, de bomen
hebben volume, de heuvels lopen door tot de horizon. De stilering zit in
kleurbanding en silhouet — **niet** in minder geometrie. Dat is woordelijk de
combinatie die de ÉÉN-STIJL-WET vraagt: cel/toon-post plus Nanite-dichtheid.

Gebruik dit beeld als de scheidsrechter wanneer iemand voorstelt om detail weg te
halen "omdat het toch toon is". Het antwoord staat hier.

Ook nemen: de lichtbehandeling. Eén sterke bron, lage stand, alles eromheen in
warme banden. Dat is hoe de Eclipse-schemering uit 03 eruit kan zien zonder
Lumen-kosten.

**NIET nemen.** Het pastelpalet. Per-planeet paletten liggen vast in
`phase0/art_style_bible.md` §3.

---

## Hoe je ze aan een agent geeft

Nooit los. Altijd zo:

```
Referentie: Eclipse/Docs/FeelReferences/REF-05_shooter_hipfire.png
Neem hieruit: de wapenpositie in het kader en de HUD-ankers in de hoeken.
Neem NIET over: de first-person camera en de fotoreal-materialen — wij zijn
third-person en toon (15.5, locked). De positie is een startwaarde die
over-shoulder opnieuw afgesteld moet worden.
```

De tweede en derde regel zijn niet beleefdheid. Zonder die regels levert een
referentie een stijlbreuk op, en dat is in dit project een blokkerende bevinding.
