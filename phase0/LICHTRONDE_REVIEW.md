# Lichtronde 1 — vaste-camera review

*26-07-2026, laat. Zeven frames op 1920×1080 via `-EclipseShot`, in
`Eclipse/Saved/Screenshots/WindowsEditor/` (HighresScreenshot00227 t/m 00233).
De owner: "ik wil één review-ronde zien voor je er honderd neerzet."*

## Wat ik zie

**De cel-shading werkt.** Contourlijnen, vlakke kleurbanden, een schemerlucht met
een oranje horizon. Het district leest als één stijl, ook op commando-afstand.

**De armaturen lezen niet als lampen.** Op het overzichtsframe staan vijftien tot
twintig amberkleurige blokken verspreid over het plein. Ze gloeien, maar er ligt
**geen lichtplas onder** en er is geen afval naar de randen. Ze lezen daardoor als
zelflichtende kratten, niet als verlichting.

**En daarmee leest de hiërarchie niet.** Het plan zet helderheid als enige
rangorde — extractie 16, doelsites 10, routes 6 — maar op commando-afstand zien de
ambervlakken er allemaal ongeveer even helder uit. Wat wél opvalt is de gele
schijf bij het extractiepunt: die leest als een baken, en die is niet met
helderheid gemaakt maar met VORM.

## Wat ik NIET kan zeggen

Welke van die ambervlakken de dertien armaturen zijn. Er staan er meer in beeld
dan er geplaatst zijn, dus een deel is dressing of dekkingsblokken. Een oordeel
"de armaturen zijn te helder" zou dus over de verkeerde objecten kunnen gaan —
dezelfde valkuil als het meetvak dat voor 95% een ander oppervlak was.

**Om dat op te lossen is één ronde nodig waarin alleen de armaturen zichtbaar
zijn**, net zoals de isolatie-opname in de speelronde de speler aanwijsbaar maakte.
Dat is de eerste stap voor lichtronde 2, vóór er meer bijkomt.

## Mijn aanbeveling aan de owner

1. **Geen armaturen bijplaatsen** tot de vorige vraag beantwoord is. Meer van iets
   dat niet als lamp leest, maakt het district niet leesbaarder — alleen drukker.
2. **De rangorde niet alleen op helderheid bouwen.** De gele schijf bewijst dat
   vorm op commando-afstand harder werkt dan intensiteit; helderheid alleen is een
   as die op een SM5-laptop bovendien snel dichtklapt.
3. **Overwegen of een armatuur licht hoort te WERPEN.** Nu zijn het emissieve
   vlakken. Een lichtplas op de grond is wat een lamp tot lamp maakt, en het is
   ook wat een route 's nachts leesbaar maakt.

Punt 2 en 3 zijn ontwerpkeuzes en geen bugs, dus ze liggen bij de owner. Punt 1
is een pauze, en die geldt tot hij ernaar gekeken heeft.

---

## CORRECTIE, een uur later: ik beoordeelde de verkeerde objecten

Hierboven staat dat "de armaturen niet als lampen lezen", op grond van vijftien tot
twintig ambervlakken op het plein. **Dat waren de armaturen niet.**

De isolatieronde (`-EclipseShot -EclipseShotFixtures`) houdt alleen de dertien
armaturen over en verbergt 423 andere actoren. Wat er dan overblijft:

- op commando-afstand: **een handvol kleine donkere stipjes** bij de horizon;
- van dichtbij: **een donkere doos zonder enige gloed**.

De ambervlakken uit mijn eerste oordeel zijn dus iets anders — dekkingsblokken of
dressing met hun eigen emissive. Precies de fout waar ik hierboven zelf voor
waarschuwde, en toch gemaakt: een oordeel geveld over vlakken waarvan ik niet had
vastgesteld dát het de armaturen waren.

### En de isolatie zelf vond eerst niets

De eerste run meldde **226 armaturen zichtbaar** terwijl de graybox er dertien
plaatst. De gedeelde tag was in `SpawnBlock` beland — de generieke blokkenspawner —
in plaats van in `SpawnFixture`, dus 226 districtblokken heetten "armatuur" en het
halve district bleef staan.

**Het beeld zag er plausibel uit.** Alleen het GETAL klopte niet. Daar staat nu een
controle op: wijkt het aantal af van dertien, dan is dat een fout en geen frame om
te beoordelen.

### Wat er nu wél vaststaat

Alle dertien armaturen krijgen hun ingangen — nagemeten per stuk:

```
Beacon_Extraction   mesh=ok  basis=ok  emissive=ok  gain=16.0
Beacon_*  (4x)      mesh=ok  basis=ok  emissive=ok  gain=10.0
RouteLight (8x)     mesh=ok  basis=ok  emissive=ok  gain=6.0
```

Mesh, basistextuur, emissive-masker en gain zijn er allemaal, met de bedoelde
hiërarchie 16 / 10 / 6. **Toch gloeit er niets.** Daarmee is dit geen ontbrekend
asset en geen verkeerde waarde, maar iets in de materiaalgraaf of in het masker
zelf — bijvoorbeeld een `EmissiveMaskTex` die voor déze meshes zwart is, of een
UV-modus die het masker verkeerd uitleest.

Dat is de eerste taak van lichtronde 2, en hij is nu scherp gesteld in plaats van
een vermoeden.

## Herziene aanbeveling

De twee ontwerpvragen hieronder (licht werpen, rangorde op vorm) blijven staan,
maar ze zijn **niet urgent**: zolang de armaturen helemaal niet oplichten, valt er
over hun helderheid niets te beslissen. Eerst uitzoeken waarom het emissive-vlak
donker blijft.

---

## Vier hypotheses uitgesloten, één over

Nadat vaststond dát de armaturen donker zijn, is de oorzaak stap voor stap
ingeperkt. Elke stap is een meting, geen redenering:

| # | Hypothese | Proef | Uitkomst |
|---|---|---|---|
| 1 | een asset ontbreekt | per armatuur mesh / basis / emissive gelogd | **alle dertien ok** |
| 2 | de master kent de parameters niet | `GetTextureParameterValue` / `GetScalarParameterValue` bevragen | `EmissiveMaskTex=ja`, `GlowGain=ja` |
| 3 | de gain staat te laag | alle armaturen op **200** (12,5× de hoogste geauthorde waarde) | **geen zichtbaar verschil** |
| 4 | de UV-modus leest het masker verkeerd | `UVMode` van 1 naar **0** | **geen zichtbaar verschil** |

Hypothese 2 was de meest waarschijnlijke en de goedkoopste: `SetTextureParameterValue`
doet **stil niets** als de naam niet op de master staat — geen fout, geen
waarschuwing. Dan "slaagt" elke regel terwijl er niets aankomt. Die staat nu
permanent in de log-regel, want het is het soort fout dat zich anders herhaalt.

**Wat overblijft: de materiaalgraaf zelf.** `EmissiveMaskTex` bestaat als
parameter, maar niets bewijst dat hij ook op de emissive-uitgang is aangesloten.
`M_EclipseToonFixture` is door een script gemaakt (`Tools/create_fixture_material.py`);
een parameter aanmaken en hem daadwerkelijk verbinden zijn twee dingen, en alleen
het eerste is hier aantoonbaar gebeurd.

Dat is dezelfde vorm als het koppelscript van vanochtend, dat vijf skeletkoppelingen
meldde en er nul opsloeg: **een script dat zegt dat het iets deed, zonder na te
kijken.**

**Volgende stap:** de graaf van `M_EclipseToonFixture` inspecteren — is de
`EmissiveMaskTex`-sample verbonden met Emissive, of hangt hij los in de graaf? Eén
detail uit de proeven wijst die kant op: op twee armaturen is bij hoge gain een
flinterdun amber randje te zien. Er komt dús iets door, maar op een minuscuul deel
van de mesh.

