# HTML-kwaliteit in Unreal krijgen

*Herzien nadat de owner corrigeerde: er is een wapen, en de agent opent de game
zelf elke 5-10 minuten, kijkt en speelt. De lus is dus dicht. Dit document gaat
over waarom dat nog niet genoeg is.*

---

## Wat een HTML-bestand eigenlijk geeft

De demo's die de vraag opriepen draaien in de browser — "built with Three.js",
`localhost:8080/fps.html`, "one HTML file". Het verleidelijke antwoord is "het
model kan zijn resultaat zien". Dat klopt hier niet meer: jouw agent ziet zijn
resultaat ook.

Een HTML-bestand geeft twee dingen die daar los van staan:

| | HTML | Eclipse nu |
|---|---|---|
| Eén iteratie kost | ~3 seconden (bewerk, F5) | 5-10 minuten (bewerk, build, start, speel) |
| Iteraties per uur | honderden | zes tot twaalf |
| Waar de staat leeft | tekst, in het bestand dat hij net schreef | binaire `.uasset` + een C++-struct |
| Welk frame hij ziet | elk frame dat hij wil | wat er toevallig op het scherm staat |

Schietgevoel is een zoektocht door ~25 gekoppelde continue parameters:
terugslag verticaal, horizontaal, hersteltijd en -curve, spreiding heup vs. ADS
vs. lopend, shake-amplitude en -frequentie, mondingsvuur-duur, tracer-snelheid en
-dikte, hitmarker-vorm/-duur/-vertraging, hit-reactie, hulzen, sway. Die
parameters zijn niet los af te stellen — terugslag zonder shake voelt anders dan
terugslag mét.

Zo'n zoektocht heeft honderden rondes nodig, ongeacht de engine. Bij zes rondes
per uur is dat geen langzame convergentie, het is **geen** convergentie: de
sessie loopt af voordat het gevoel er is. Dat is het hele verschil. Niet
modelkwaliteit, niet Unreal-zwaarte — het aantal rondes.

---

## De drie concrete gaten

### 1. De binnenlus kost nog steeds een compile

Er staan negentien console-commando's geregistreerd:

```
Eclipse.Base.Build          Eclipse.Feel.Dump          Eclipse.Mission.RaiseAlarm
Eclipse.Base.Report         Eclipse.Gauntlet.Summary   Eclipse.Prep.AutoLaunch
Eclipse.Base.Vault          Eclipse.Campaign.*         Eclipse.Roster.Kill
Eclipse.Command.Dump        Eclipse.Economy.Report     Eclipse.Squad.DumpOrders
Eclipse.Events.Dump         Eclipse.Mission.*          Eclipse.Strategy.*
```

**Geen enkele daarvan schrijft een gevoelswaarde.** `Eclipse.Feel.Dump` leest.
De rest zijn toestandsacties. En de terugslag- en spreidingsvelden staan in
`EclipseCharacterTypes.h` — een C++-struct. Eén getal veranderen kost een build.

Dat is de 5-10 minuten. Die zit niet in Unreal, die zit hier.

**Wat het moet worden:**

```
Eclipse.Feel.Set   Recoil.VerticalDeg 1.4     — zet live, dit frame nog
Eclipse.Feel.List                             — alle padden + huidige waarde
Eclipse.Feel.Save                             — schrijf terug naar DT_WeaponFeel
Eclipse.Feel.Revert                           — terug naar de tabel
```

Implementatie: één `UEclipseFeelTuningSubsystem` met een `TMap<FName, float*>`
die naar de live struct-velden wijst, gevuld bij startup uit de DataTable.
`Set` schrijft door het pointer, dus het volgende schot gebruikt de nieuwe
waarde. `Save` serialiseert terug zodat de sessie niet verdampt.

Effect: de agent start de game **één keer** en doet er driehonderd rondes in.
Dat is de HTML-lus, in Unreal. De 5-10 minuten worden 5-10 seconden.

Dit is verreweg de belangrijkste van de drie. De andere twee zijn de moeite
alleen waard nádat deze er ligt.

### 2. De screenshot landt op het verkeerde frame

Schietgevoel leeft in ongeveer twaalf frames na de trekker:

| frame | wat er hoort te gebeuren |
|---|---|
| +0 | mondingsvuur aan, shake begint |
| +1..2 | wapen kickt, kruis loopt omhoog |
| +2..4 | tracer onderweg, mondingsvuur uit |
| +4 | inslag, hitmarker aan, hit-reactie start |
| +6..10 | herstel, kruis zakt terug |
| +12 | hitmarker uit |

Een vaste-camera screenshot, of "de agent speelt even", landt daar nooit op. Hij
levert een plaatje van de kamer. Dat is prima voor de art-review — waar
`art-reviewer` ook precies voor bedoeld is — en het zegt niets over of het schot
leest.

In HTML bestaat dit probleem niet omdat het model de render-lus zelf schrijft en
frame N kan pakken.

**Wat het moet worden:** koppel capture aan de event-bus die je al hebt.
`Event.Combat.ShotFired` en `Event.Combat.HitLanded` triggeren een opname van
frame +0 t/m +12, en die worden getegeld tot één PNG-strip.

De agent kijkt naar die strip. Niet naar de kamer — naar het schot. Dat is het
enige artefact waarop schietgevoel te beoordelen valt, en er is er nu geen.

Bijvangst: de strip legt dode frames meteen bloot. Twaalf identieke plaatjes
betekent dat er niets gebeurt, en dat zie je in één blik.

### 3. De engine-staat is niet leesbaar

`Eclipse.Feel.Dump` is precies het goede idee en het gaat niet ver genoeg. Als
een shot er goedkoop uitziet kan het model in HTML de shader herschrijven — hij
schreef hem zelf. In Unreal moet het weten welke van honderden engine-defaults
het doet, en het kan ze niet lézen, want `.uasset` is binair.

Daarom is een HTML-bestand zo sterk: **staat is tekst.** Dat is te reproduceren.

**Wat het moet worden:** `Eclipse.Look.Dump` dat post-process-volume-waarden,
de parameters van het toon-materiaal, `PP_EclipseOutline`-instellingen, exposure,
Lumen-kwaliteit en de camera als platte tekst uitschrijft. Plus `Eclipse.Look.Set`
langs dezelfde weg als 1.

Je hebt hier al de helft van: `author_toon_material.py`, `setup_look_tuning.py`,
`measure_frame_values.py`. Wat ontbreekt is dat de agent het tijdens het spelen
kan lezen en zetten.

---

## De referenties

De beelden liggen in `Eclipse/Docs/FeelReferences/` — dat sluit meteen het
openstaande punt uit `graybox_feel_targets.md` §5, dat om referentiemateriaal
vroeg en tot nu toe leeg stond.

Lees de README daar vóór je een referentie doorgeeft. Kern ervan:

**Vier van de zeven overtreden de ÉÉN-STIJL-WET.** Ze zijn fotorealistisch of
first-person; Eclipse is toon en third-person (15.5, locked). Ze liggen er voor
hun *compositie, HUD-indeling, wapenplaatsing en timing* — niet voor hun
rendering. Geef een referentie daarom **nooit los**, altijd met beide kolommen:

```
Referentie: Eclipse/Docs/FeelReferences/REF-05_shooter_hipfire.png
Neem hieruit: de wapenpositie in het kader en de HUD-ankers in de hoeken.
Neem NIET over: de first-person camera en de fotoreal-materialen — wij zijn
third-person en toon. De positie is een startwaarde, geen eindwaarde.
```

Zonder die tweede regel krijg je een fotoreal-inslag in een toon-game terug, en
`art-reviewer` keurt dat af als blokkerend. Terecht.

De vier die er voor het schieten toe doen:

- **REF-05 / REF-06** (heupvuur naast gemikt) — het verschil tússen die twee
  beelden ís het antwoord op de vraag over positie. Ze leveren de twee offsets
  voor `DT_WeaponFeel`. Wat ze níet geven is de overgang ertussen; daar zijn de
  frame-strips voor.
- **REF-03** (subway-HUD) — de indeling voor punten 4, 5 en 14 van de
  `GEVECHT_AUDIT`. Let vooral op de accuracy-teller rechtsboven: die maakt
  trefferfeedback meetbaar over een hele sessie in plaats van per schot.
- **REF-07** (painterly wereld) — het bewijs dat gestileerd niet arm hoeft te
  zijn. Zet dit ernaast als iemand detail wil weghalen "omdat het toch toon is".

## De prompt

Pas te gebruiken nadat gat 1 dicht is. Ervoor is het zinloos — dan vraag je om
honderd rondes in een lus die er zes toelaat.

```
Je gaat het schietgevoel afstellen. Eén sessie, niet herstarten.

Start de game één keer. Gebruik Eclipse.Feel.Set om waarden te veranderen —
NOOIT een rebuild, NOOIT de editor. Als een waarde niet via Set bereikbaar is,
meld dat en sla hem over; hem alsnog via een build willen doen is de fout die
deze hele opdracht moet voorkomen.

Bekijk eerst deze drie, en lees de annotatie in de README ernaast:
- Eclipse/Docs/FeelReferences/REF-05_shooter_hipfire.png  (heupvuur)
- Eclipse/Docs/FeelReferences/REF-06_shooter_ads.png      (gemikt)
- Eclipse/Docs/FeelReferences/REF-03_subway_fps_hud.png   (feedback-HUD)

Uit REF-05/06 haal je de wapenpositie en het verschil heup->ADS. Uit REF-03 de
plek van de feedback in het kader. Neem uit geen van drieën de renderstijl of
het first-person perspectief over — wij zijn third-person en toon (15.5).
De offsets zijn startwaarden die over-shoulder opnieuw afgesteld moeten worden.

Per ronde:
1. Vuur drie schoten uit de heup en drie uit ADS.
2. Pak de frame-strips (Eclipse.Feel.Set Capture.Strip 1 zet ze aan).
3. KIJK naar de strips, met REF-05/06 ernaast. Schrijf in één regel wat er
   dood of goedkoop uitziet.
4. Eén parameter tegelijk bijstellen. Herhaal.

Minimaal veertig rondes voordat je iets aan mij laat zien.

Wat ik terug wil:
- het logboek van veertig regels, één per ronde
- de eindtabel: parameter, beginwaarde, eindwaarde, waarom
- de frame-strip van ronde 1 en van ronde 40 naast elkaar
- per referentie één regel: wat je ervan overnam en wat je bewust liet liggen
- Eclipse.Feel.Save gedraaid, en de DataTable-diff in de commit

Leesregel: alleen phase0/GEVECHT_AUDIT.md, phase0/graybox_feel_targets.md en
Eclipse/Docs/FeelReferences/README.md. Niet HANDOFF.md, niet de GDD-delen. Die
staan vol intentie en die is al binnen; je hebt de ruimte nodig voor de strips.
```

Die laatste alinea is geen bijzaak. `HANDOFF.md` is 101 KB, er liggen 41
documenten in `phase0/`, en buiten `Eclipse/` staat ~800 KB markdown. Een
gevoelssessie die daarmee begint, begint met een halfvolle context. De demo's
kregen 100% van de ruimte voor het probleem zelf.

---

## Waarom hier geen code in zit

Deze repo loopt achter op je werkkopie — er is een wapen dat hier niet in staat.
`Eclipse.Feel.Set` schrijven tegen deze versie van `EclipseCharacterTypes.h`
levert een conflict met wat je lokaal al hebt. De spec hierboven is precies
genoeg om hem lokaal te laten bouwen; push eerst, dan kan het hier ook.

## Samengevat

De lus is dicht, dat klopt. Hij is alleen **traag, grof en gericht op het
verkeerde frame.**

1. `Eclipse.Feel.Set/List/Save` — één sessie, honderden rondes in plaats van zes.
2. Frame-strips op `ShotFired`/`HitLanded` — kijk naar het schot, niet naar de kamer.
3. `Eclipse.Look.Dump/Set` — maak de engine-staat leesbaar zoals een HTML-bestand.
4. Referenties uit `Eclipse/Docs/FeelReferences/`, altijd mét de "neem NIET over"-regel.
5. Drie leesbare bestanden per gevoelstaak, verder niets.

Punt 1 alleen al haalt het grootste deel van het verschil weg.
