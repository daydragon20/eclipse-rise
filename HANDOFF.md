# ECLIPSE — PROJECT HANDOFF & PROGRESS
*Single "start here" page for a new machine or a new Claude session. Last updated: 2026-07-26 (dagopdracht + drie audits: locomotie, gevecht, squad).*

---

# AVONDRAPPORT — 26 juli 2026

**Bar: build ✓ (-NoUba) · 143/143 tests · validatie 5 validators / 0 fouten.**

## Geland vanavond

| | Gemeten |
|---|---|
| **Terugslag.** Het schot duwt je kruis omhoog en het zakt vanzelf terug met de stabiliteit uit het profiel. Het herstel stopt zodra je zelf kijkt — anders vecht het spel tegen je eigen correctie in. | Klim **0,500°** (data zegt 0,50), na 1 s rust **0,000°** |
| **Elk wapen klinkt anders, en niet als een loop.** Drie schotvarianten per familie, keuze is "nooit dezelfde als net". | **0 herhalingen** in 1000 trekkingen, verdeling 348/332/320 |
| **Een schot heeft ruimte.** De nagalm klinkt bij het openende schot van een serie, niet bij elke kogel. | 14 schoten in 2 s → **4 nagalmen** |
| **De demper is een keuze geworden.** De sidearm alarmeert tot 1200 cm in plaats van 2500 — onder de waarnemingsafstand van een vijand, dus hij verraadt je alleen aan wie je toch al zag. | Validator bewaakt de eis, niet het getal |
| **Voetstappen weten waar je op staat.** Het district hád geen oppervlaktetypes; die zijn er nu, met physical materials en een trace op het moment van de stap. | Plein **beton**, bovenop een dekkingsblok **metaal** (op hoogte 210 cm) |

## Correctie op mezelf: er ZIJN draai-animaties

Vanmorgen schreef ik bij punt 7 (turn-in-place) dat er geen bruikbare
draai-animatie in de packs zat, en dat het daarom niet te bouwen was. Dat klopt
niet, en het is het soort fout dat een hele taak ten onrechte doodverklaart.

Ik keek in SciFiCharacter. Maar **de speler is Belica** (ParagonLtBelica), en die
pack levert precies wat punt 7 vraagt:

    Idle_Turn_90_Left / Idle_Turn_90_Right
    Idle_Turn_180_Left / Idle_Turn_180_Right
    RMB_TurnInPlace_Fast / _Slow / _Zero

Vier draaitakes op de juiste hoeken, plus drie gemikte varianten. Voor het lichaam
dat er het meest toe doet, want turn-in-place gaat over de speler die zijn camera
draait terwijl hij stilstaat.

**Gevolg:** punt 7 is niet geblokkeerd, en de drempel van 90 graden in de
kijkcode (die er staat *omdat* ik dacht dat er geen animatie was) kan omlaag zodra
de takes zijn aangesloten. Dat staat als volgende taak.

De squadleden en vijanden gebruiken de SciFi-packs en hebben die takes níet — voor
hen blijft de huidige drempel gelden. Dat is geen probleem: je ziet hun voeten
zelden van dichtbij, en de speler is degene die zijn eigen lichaam de hele tijd
in beeld heeft.

## Wat ik van jou wil horen — het LICHTPLAN

Je vroeg één review-ronde voor ik honderd armaturen neerzet. Dit is die ronde.

**Eerst het antwoord op je vraag over emissive, want die verandert alles.**

De toon-pipeline heeft twee banden en dat is geen detail:

- Bijna alles in het district draait op **M_EclipseToonLit** met `EmissiveScale = 1`.
  BaseColor is albedo; de echte lichten leveren de energie.
- De **Glow**-familie draait op **M_EclipseToon** (unlit) met `EmissiveScale = 10`,
  en is bovendien boven 1.0 geauthord (2.2 / 1.0 / 0.3). Dat is de *lichtbron-band*
  en hij ligt daarmee ruwweg een factor 22 boven de albedoband.

**Wat dat betekent voor de 17 armaturen:** hun emissieve delen horen in de
Glow-band, niet in de gewone toon-band. Een fixture die op `EmissiveScale = 1`
staat, is in dit district geen lamp maar een lichtgekleurd blok — hij zit dan in
dezelfde luminantieband als de muur ernaast. De behuizing hoort juist wél in de
gewone lit-band, anders gloeit het metaal mee en verdwijnt de vorm.

Dus per armatuur **twee materiaalslots**: behuizing door de toon-master (15.5:
basistexture wordt luminantie-detail, palet bepaalt de kleur), emissief deel in de
Glow-band. Dat is precies de één-stijl-wet toepassen op een PBR-pack, niet ernaast.

**De opbouw, uit de referentie (RayTracedCinematicLightin) — de OPBOUW, niet de techniek:**

1. **Poelen, geen dekking.** Het leerproject verlicht ruimtes met een handvol sterke
   bronnen en laat het donker ertussen staan. Kessara heeft nu één zon en een
   uniforme dressing; wat ontbreekt is dus niet "meer licht" maar **contrast**.
2. **Hiërarchie in helderheid.** Eén bron is de sterkste in beeld, de rest zit er
   meetbaar onder. Dat is dezelfde regel als het palet al volgt.
3. **Gemotiveerde bronnen.** Elk licht heeft een zichtbare armatuur. Dat is wat dit
   pack levert en wat het district nu mist.

**Wat ik wil doen, in deze volgorde:**

| | Waar | Hoeveel | Waarom |
|---|---|---|---|
| 1 | **Doelsites** (ControlPost, AlarmRelay, Crane, Pens, Extraction) | 5 bakens | Een doel dat oplicht is leesbaar vanaf de andere kant van het plein. Dit is navigatie, geen dressing |
| 2 | **De routes ertussen** | ~8 wandarmaturen | De poelen die de referentie maakt; ertussen mag het donker blijven |
| 3 | **De extractiezone** | 1 sterkste bron | De hiërarchie: dit is het helderste punt in het district |
| 4 | Rest van het district | pas na jouw oordeel | — |

Dat zijn **14 armaturen, niet 100.** Eerst één ronde vaste-camera screenshots door
de art-review, dan pas breed.

**Wat ik van je nodig heb:** ja/nee op deze volgorde, en of je de bakens bij de
doelsites een eigen kleur wilt (mijn advies: nee — één palet, en de hiërarchie
komt uit helderheid, zoals de referentie het doet).

## Wat er klaarstaat maar nog niet geland is

Terwijl jij speelde stond de build vast — Live Coding houdt het slot vast zolang
de editor open is. Dat is geen klacht: ik heb ondertussen doorgewerkt aan wat
geen compiler nodig heeft. Maar het betekent dat het volgende **geschreven en
nagelezen is, en nog niet bewezen**:

- **Loadouts bereiken je wapen.** Elke loadout noemt nu een primair wapen en een
  sidearm; RB wisselt buiten Command Mode; elk slot houdt zijn eigen magazijn;
  `ReadySeconds` (handling) blokkeert het vuren vlak na een wissel. Plus een
  validator die eist dat een loadout bestaande, verschillende wapens noemt.
- **De hermeting van de kopschot-test**, die sinds het magazijn in een
  herlaadbeurt kon vallen en dan nul schade mat.

Zodra de editor dicht is: compileren, suite, landen. Het staat als klikje in het
dashboard.

## Ontwerpen die klaarliggen (geen build nodig, wel jouw oordeel)

- **[SQUAD_DOCTRINE.md](phase0/SQUAD_DOCTRINE.md)** — punt 1 volledig uitgewerkt.
  Twee lagen: basiscompetentie die er altijd is, en doctrine die hem inperkt of
  loslaat. Vier waarden op de bestaande `stance`. Zes bouwlagen, elk apart
  meetbaar, met de hermeting van het gevecht als laatste.
- **[TESTGIDS_HERBOUW.md](phase0/TESTGIDS_HERBOUW.md)** — de F3-gids van 23
  stappen naar 6, langs jouw regel. Van de 23 waren er 17 dingen die ik zelf had
  moeten vaststellen.

## Wat nog steeds op je wacht

### Voorstel: elementaire schade — mijn advies is NEE, en hier is waarom

Je zei: niet bouwen, zet een voorstel neer en ik beslis. Dit is het voorstel.

**Wat het in Borderlands is.** Vuur/zuur/schok/ontploffing zijn daar geen extra
schadegetal maar een *counter-systeem*: elk vijandtype heeft een gezondheidsbalk
van een soort (vlees, pantser, schild) en elk element telt anders tegen elke
soort. Vuur verbrandt vlees, zuur vreet pantser, schok breekt schilden. Daar komt
de hele wapenjacht van dat spel uit voort: je draagt drie wapens omdat je drie
soorten tegenkomt.

**Wat het kost om het echt te doen.** Drie dingen die ECLIPSE geen van drieën
heeft:
1. **Soorten gezondheid.** Nu is er één balk. Zonder pantser en schilden telt een
   element tegen niets — dan is vuur gewoon "meer schade" met een kleurtje.
2. **Statuseffecten over tijd.** Branden, corroderen, verlamd zijn. Dat is een
   effect-systeem per personage plus HUD ervoor, en het raakt de squad-AI (loopt
   een brandende vijand nog naar dekking?).
3. **Zichtbaarheid.** In Borderlands zie je aan de vijand wat er gebeurt. Bij ons
   is dat vier extra materiaal-toestanden door de toon-pipeline.

**Waarom ik nee adviseer.** Niet omdat het te veel werk is, maar omdat het het
verkeerde probleem oplost. De wapens verschillen sinds vandaag al op negen assen
(schade, tempo, magazijn, afval, spreiding heup/mik/beweging, terugslag,
stabiliteit, handling, kopschot) — en er staat nu een demper tegenover die je
alarmradius meer dan halveert. Dat is al meer keuze dan je in een gevecht kunt
gebruiken. Een tiende as toevoegen maakt de negen niet scherper.

**Wat ik in plaats daarvan zou doen als je die kant op wilt:** geef vijanden
*pantser* (één extra soort, geen vier), en laat één wapenveld dat doorboren. Dan
heb je de counter-gedachte van Borderlands met één as in plaats van vier, en het
past in DT_Weapons als één veld. Dat is een dag werk, niet een week.

**Beslis: nee (mijn advies) / ja, volledig / het pantser-alternatief.**
- **Spelen.** Er is sinds vanmorgen terugslag, spreiding, kopschoten, nieuwe
  vijandopstelling en drie lagen geluid bij gekomen. Ik kan meten dát ze werken;
  of het *lekker* is, kan ik niet meten.

---

# DAGRAPPORT — 26 juli 2026, bijgewerkt 12:25

**Bar: build ✓ (-NoUba) · 137/137 tests · validatie 4 validators / 0 fouten · catalog 31/31.**

*Alle getallen hieronder zijn om 12:05 tegen verse metingen gelegd en klopten alle
acht. Wil je het zelf nakijken: `python Eclipse/Tools/show_measurements.py [woord]`
leest ze uit het laatste testlog, en dat is de bron — dit document houdt geen eigen
kopie bij.*

## Wat werkt er nu dat gisteren niet werkte

| | Gemeten |
|---|---|
| **Achteruit lopen draait je niet meer om.** Het personage blijft naar de camera kijken en loopt achteruit — de third-person-shooter-conventie (Gears, Division, Borderlands) in plaats van het adventure-model. | Draaiing bij puur achteruit duwen: **180° → 0,0°**. Looprichting t.o.v. kijkrichting: **+1,00 → −1,00** |
| **Kopschoten doen echt meer.** Er zit een hitbox op de hoofd-socket, met terugval bovenin de capsule voor lichamen zonder mesh. | Romp **44 hp**, hoofd **110 hp** → verhouding **2,50×**, exact wat DT_Weapons zegt. Was 22 tegen 22 |
| **Je eerste schot verraadt je.** Vijanden binnen gehoorsafstand lopen naar de plek waar geschoten werd — niet naar waar je nú bent. Ook een gemist schot. | Vanaf **4200 cm** (buiten élk waarnemingsbereik): alle vier komen in beweging, samen **10.871 cm** dichterbij. Radius per wapen: **5000 cm** |
| **Vuren en geraakt worden veranderen de pose.** Schietanimatie en hit-reactie, allebei op één overlay-slot. | Klap heeft op 0,1 s gewicht **0,995** en dooft uit; schietpose past binnen het vuurinterval |
| **Landen heeft gewicht.** Camera-dip geschaald met de valsnelheid. | **4,3 cm** bij een normale sprong, komt exact terug op 65 |
| **Hurken heeft een overgang.** De take zat al in de pack en werd nooit gespeeld. | 0,3 s op half gewicht; hoofd-hitbox zakt mee |
| **Missies plaatsen hun eigen vijanden.** De vaste lus van vier is weg. | Assault **6**, Rescue **5**, Sabotage **4** (was overal 4). M1.1 houdt zijn 4. In een echte `-game`-run bevestigd: *"5 vijanden uit 2 geauthorde batch(es)"* |
| **Je squad praat.** Acht order-antwoorden met een rem van 2 s per soldaat. | **8 van de 8** aangesloten zinnen hebben een clip; een gevallen soldaat zegt nu *"I'm hit - can't move."* in plaats van *"No route, boss."* |
| **De +20 voor een ronde zonder gewonden betaalt uit.** | **20 materiaal** gemeten; was 0 |
| **Geen dode knoppen meer op RB en X.** RB wisselt buiten Command Mode het camerastandpunt (de pad kón dat helemaal niet meer), X geeft een snelle hergroepeer-order. LT is weer alleen mikken. | — |
| **Het alarm gaat af bij de eerste waarneming.** | Zie waarschuwing hieronder |
| **Je squad houdt een order vol.** "Richt op dat doelwit" vuurde precies één kogel en zweeg daarna — de order bleef staan, maar niets voerde hem opnieuw uit. Vijanden hadden wél een denkbeurt, squadmates niet. | **14 schoten in 2 seconden** waar het er **1** was |
| **Je hoort het gevecht.** Schot, inslag en voetstappen — alle drie lagen de cues ongebruikt in de repo. | Schot 0,7 · inslag 0,85 (luider, want dat is het signaal dat je *raakt*) · **een kopschot klinkt 1,35× harder** · voetstap elke 140 cm |

## Wat wacht er op jou

**Spelen en oordelen — dat kan ik niet meten:**
1. **De nieuwe vijandopstelling.** Niet meer vier op een hoop bij het hoofddoel, maar groepen op het site dat de missie noemt. Het aantal schuift nauwelijks (+2, +1, +0); de **vorm** verandert wel. **Start hem met `-EclipseStartMission=WorkerHousing`** — dat is de rescue, en op dag 1 de enige missie mét eigen opstelling die je kunt bereiken. Assault en Sabotage vragen eerst een aangrenzende regio (GDD 3.1 regel 1); de snelstart meldt dat en zet je in de hub.
2. **Kopschoten halveren je time-to-kill** van 4 kogels naar 2. Dat was de bedoeling, maar je moet het voelen.
3. **Het schot-alarm.** Sluipen blijft mogelijk, precies één schot lang.

**Beslissingen:**

| Vraag | Mijn aanbeveling |
|---|---|
| **Kost mikken snelheid?** Vandaag niet — je sprint even hard met je vizier op. In dit genre is 0,6× standaard. | **Doen.** Zonder straf is er geen reden om ooit níét te mikken. Maar het raakt elk gevecht, dus jouw call. |
| **Y (stance) blijft dood.** Stance wisselt alleen de HUD-regel; er is geen gedrag om buiten Command Mode aan te hangen. | **Laten tot de klasse-verbs.** Er iets anders op zetten verbergt dat stance zelf nog niet af is. |
| **Het alarm kost je vandaag niets.** Nagemeten uit de assets: **0** optionals eisen stilte, 1 eist nul gewonden. | Speel er niet omheen alsof het al straf oplevert. Het staat klaar voor je eerste stealth-optional. |

**Assetwerk — hier houdt aansluitwerk op:**
- **Geen enkele pack heeft een draai-animatie.** Punt 7 is daarom half: het lichaam draait mee boven 90° (zoals Gears en Division), maar de voeten schuiven. Zoals afgesproken stop ik daar — dit is assetwerk.
- **Vijf van je negen lichamen hebben geen zijcycli.** De vier richtingen zitten in één pack; je lichamen komen uit vijf. De speler krijgt er 3 van de 6. Er is een kruislingse terugval, maar voor die vijf is er niets om op terug te vallen: die moonwalken bij achteruit en strafen.

**Na 10:40 kwam daar dit bij, en het hing allemaal aan één vraag.** Ik heb de
audit-methode ook op het gevecht losgelaten
([phase0/GEVECHT_AUDIT.md](phase0/GEVECHT_AUDIT.md), zestien punten), en de rode
draad daar was scherp: **vier van de vijf omissies gingen over FEEDBACK, niet over
mechaniek.** Het gevecht rékende goed en deed dat in stilte. Drie van die vier
bleken bij natrekken geen keuze maar een dóód asset — de cues lagen al in de repo
en er was niets dat ze afspeelde. Die zijn aangesloten. Wat overblijft is de
hitmarker: je hóórt sinds vandaag dat je raakt, je ziet het nog niet.

Daar zijn twee gereedschappen uit voortgekomen, want ik vond die geluiden bij
toeval en toeval is geen methode: `find_dead_fields.py` (tuningvelden die niets
leest) en `find_dead_assets.py` (audiocues die niemand afspeelt). Die laatste heb
ik twee keer moeten versmallen — de eerste versie trapte op een comment, de tweede
meldde 24 valse doden. De scope staat nu in de docstring.

**Er ligt nu één vraag die groter is dan alle andere.** De squad-audit
([phase0/SQUAD_AUDIT.md](phase0/SQUAD_AUDIT.md)) vond dat je squad *niets* uit
zichzelf doet: geen autonoom vuren, geen meelopen, geen dekking. Dat zijn drie
punten maar één beslissing, en hij is groter dan kopschoten of de
vijandopstelling — een squad die alles zelf doet maakt je commando-systeem
overbodig. Mijn advies staat erbij; ik heb hem bewust niet aangezet.

**De volledige locomotie-audit staat in [phase0/LOCOMOTIE_AUDIT.md](phase0/LOCOMOTIE_AUDIT.md)** — vijftien onderdelen, elk getal gemeten, met per punt of het een keuze of een omissie is. Die ronde hoort aan het eind van elke dag opnieuw te draaien.

---

# OCHTENDRAPPORT — nacht 25→26 juli 2026

## Wat je om 07:55 vroeg, en wat ervan geland is

Zes opdrachten in één regel, alle zes af — build groen, 127/127 tests, validatie 0, catalog 29/29.

| # | opdracht | wat er nu gebeurt | wat je moet weten |
|---|---|---|---|
| 1 | **bat bijwerken** | Sprint staat per apparaat (Shift = hold, L3 = toggle met vier uitstappen), hurken zegt 52 cm, F9 staat erbij, en de vier Command Mode-regels zeggen dat ze alleen binnen de hold werken | Verder niets aan jouw bestand aangeraakt |
| 2 | **alarm bij eerste waarneming** | De latch gaat af zodra een vijand je ziet. De haak bestond al en deed niets | **Correctie op wat ik eerst schreef:** ik waarschuwde dat elke detectie je de spook-optionals kost. Nagemeten uit de assets: **er is vandaag geen enkele optional die stilte eist** (0 met `bRequiresNoAlarm`, 1 met `bRequiresNoCasualties`). Het alarm kost je dus nu nog niets — het is de latch die klaarstaat voor het moment dat je een stealth-optional authordt. Speel er dus niet omheen alsof het al straf oplevert |
| 3 | **spawns aansluiten** | De vaste lus van vier is weg; missies plaatsen hun eigen vijanden. Nagemeten in een echte ronde: het sjabloon vraagt 5, er staan er 5 | **Correctie op mijn eerste melding.** Ik schreef "15 vijanden in plaats van 4", en dat leest als bijna vier keer zoveel per missie. Het is het TOTAAL over drie missies. Per missie: **Assault 6 (+2), Rescue 5 (+1), Sabotage 4 (+0)**. De aantalsprong is dus klein. Wat wél echt verandert is de **vorm**: de vijanden staan nu op het site dat de missie noemt in plaats van alle vier naast het hoofddoel |
| 4 | **stemmen met rem van 2 s** | Twee abonnementen op de bus; rem per soldaat, niet globaal | Zinnen die nooit gegenereerd zijn blijven stil — bewust: laten genereren kost een betaalde API-aanroep |
| 5 | **downed-bark optie 1** | Eigen pool: *"I'm hit - can't move."*, *"I'm down, boss."*, *"Need a medic, not an order."* | Zinnen zijn van mij; herschrijf ze gerust, de test pint ze niet vast |
| 6 | **no-casualties als voorwaarde** | Betaalt uit. Gemeten: **20 materiaal** | **Raakt je economie:** elke ronde zonder gewonden levert nu 20 meer op. In de HUD blijft de regel tijdens de missie onafgevinkt — je kunt hem tot het eind verliezen |

**Twee dingen die ik onderweg fout had en zelf heb gevonden.** De bark-rem hing aan het geluidsbestand in plaats van aan de soldaat, dus hij remde alleen op machines waar de audio al gegenereerd was. En bij de +20 bleek de **meting** net zo stuk als het mechanisme: de speelronde telde alleen uitbetalingen met reden `MissionReward`, terwijl de bonus onder `OptionalObjective` binnenkomt — die teller had hem dus ook niet gezien als hij al gewerkt had. Twee fouten die naar dezelfde conclusie wezen; precies het patroon waar de hele nacht over ging.

---

## Als je één ding doet: start de build en druk **F3**

De game loopt je dan zelf door alle controls, en elke stap zegt waaraan je ziet dat het klopt. Dit is wat er sinds jouw laatste sessie veranderd is, op volgorde van wat je het eerst merkt:

1. **De camera doet dingen die je nog nooit gezien hebt.** 1e persoon (C), mikken (RMB/LT) en de Command Mode-uitzoom waren alle drie **dood** — het doel werd gezet, maar de camera bewoog nooit. Eén regel in de constructor. Nu: 1e persoon schuift in 0,1 s naar je ogen, mikken trekt in van 300 naar 165 cm, Command Mode zoomt uit naar 520.
2. **Kijken is 2,5× trager**, en dat is een reparatie: de engine vermenigvuldigde je getunede waarde stil met 2,5.
3. **Sprint op L3 is een toggle** met vier uitstappen; Shift blijft hold.
4. **Hurken werkt** (de toets was dood) en je zakt nu 52 cm in plaats van 96 — 80 cm was kruiphoogte.
5. **Je squad loopt mee** in plaats van elke order te weigeren.
6. **Vier knoppen werken alleen ín Command Mode** (stance, volgende, vorige, onder-kruis). Dat stond nergens; nu wel.

En je **meetgereedschap** is meegerepareerd, want daar hing je verdict aan: drie van de vier panelen zeiden niet welke toets ze bedient (F4/F5 boeken een pick, F6/F7 je oordeel, 6–0 de 13.2-vragen), en zonder die toetsen blijven drie R3-criteria op *"nog niet gemeten"* staan hoe lang je ook speelt. Dat staat nu in de koppen zelf én in `BESTURING.md`.

**De tweede helft van de nacht ging over iets anders, en dat verandert hoe je §4 moet lezen.** Toen de reparaties op waren ben ik gaan zoeken naar het omgekeerde: niet "wat is kapot" maar **"wat doet stil niets terwijl het eruitziet alsof het werkt"**. Dat bleek de rijkere vraag. Zesentwintig gevallen: **zeven mechanismen** die bestaan maar tijdens spelen onbereikbaar zijn (de camera was daar de eerste van, en de duurste), en **drieëntwintig tuningvelden** die je kunt verdraaien zonder dat er iets gebeurt. Eén daarvan raakt je direct: **drie van je vier missies beschrijven een vijandopstelling die er nooit komt** (§4 rij 21, en op het dashboard). Ik heb ze geen van alle aangesloten — dat is telkens een ontwerp- of balansbeslissing — maar ze staan nu allemaal met een `NIET GELEZEN`-regel in de code, zodat geen comment meer gedrag belooft dat er niet is. **Het gaat dus niet slechter dan je dacht; het was alleen onzichtbaar.**

Wat ik níét kan meten is hoe het **voelt**. Daarvoor staat de lijst in §4, elk punt met mijn aanbeveling erbij — maar er is er één die alleen jij kunt beantwoorden: **is de schaal-bug weg?**

---

**Bar bij elke commit: build ✓ (-NoUba) · de VOLLEDIGE testsuite groen (0 fail) · EclipseValidateData 4 validators / 0 fouten · catalog gedocumenteerd = geïmplementeerd.** Alles tussen `26edd65` en `HEAD` is van deze nacht en alles is gepusht — `git log 26edd65..HEAD` is de lijst, en met opzet de enige. In thema's: het harnas (laag 1 + 2) · zes speelrondes · S1/S2/S3 · de feel-audit · de camera die nooit bewoog · de navigatie · bewakers op de beschrijvingen en op de strategische laag.

### Hoe zeker is dit rapport?

Vier dingen, want "de tests zijn groen" bewijst niet dat een document klopt.

- **De build start ook echt.** Twee keer gedraaid met `-game -EclipseStartMission=TransitCheckpoint` — om 03:45 en nog eens om 05:55, na ruim twintig commits meer, dus jouw weg en niet die van het harnas: district gebouwd (13 blokken, 20 dekkingen, 10 sites, 3 ingangen), navigatiegrens 28000 × 28000 × 4000, missieactoren gespawnd, `Mission 'MT_M11' started`, navmesh onder de speler NEE bij start en JA vijf seconden later — en **beide keren nul fouten, nul waarschuwingen** van Eclipse.
- **De metingen zijn reproduceerbaar, geen benaderingen.** De suite drie keer achter elkaar gedraaid en de tijdgevoelige getallen kwamen er identiek uit tot op drie decimalen. Dat is constructie, geen toeval: het harnas tikt met een vaste stap. **Verandert er morgen een getal, dan is dat een echte verandering en geen ruis.**
- **De getallen in dit document zijn tegen verse metingen gelegd** — vijf keer, laatst om 07:05, toen alle kopgetallen stuk voor stuk zijn teruggezocht in de gemeten waarden. De eerste twee controles vonden een fout, de laatste drie niets. (Er stond hier "174 gemeten waarden"; inmiddels zijn het er meer, want de nieuwe spawn-bewaker meet ook. Vandaar geen getal — het overzicht telt zelf.) Wil je het zelf nakijken: `python Eclipse/Tools/show_measurements.py [woord]`.
- **De suite kost 38 seconden**, inclusief engine-start, met zes speelrondes die echte missies starten erin. Er is dus geen reden om hem over te slaan.

*Wat die controles telkens vonden, was hetzelfde: met de hand onderhouden tellingen. **Vijf keer** liep er een getal achter — testaantallen, een commit-opsomming, het aantal openstaande vragen — en één ervan stond in de alinea die uitlegt waarom tellingen verouderen. Daarom staan ze er nu niet meer in. Het exacte testaantal staat in de commit-message van elke ronde, waar het niet kán verlopen; de commitlijst is `git log 26edd65..HEAD` en niets anders.*

## 1. Wat is af en gemeten — met de getallen

**De testharnas draait.** Laag 1 leest na het spawnen de daadwerkelijk toegepaste waarden van het movement component en de camera en legt ze naast `DA_CharacterTuning` — élk veld dat wordt toegepast, allemaal gelijk. (Geen aantal hier: de lijst groeide vannacht mee met de tuning, en dit stond op 19 terwijl het er inmiddels meer zijn. De test is de bron.) Laag 2 injecteert input via Enhanced Input, op dezelfde `UInputAction`-objecten die jouw controller aanstuurt, en meet over tijd. Alles headless, in de suite, elke ronde mee.

| Meting | Gemeten | Was | Referentie |
|---|---|---|---|
| Tijd tot topsnelheid (rennen) | **0,300 s** | — | 420 / 1400 = 0,300 |
| Stoptijd vanaf rennen | **0,150 s** | 0,083 s | Bijlage B bij 4×1/2000 |
| Glijafstand vanaf rennen | **26,6 cm** | 12,0 cm | 28 cm |
| 180-omkering (weer op topsnelheid) | **0,400 s** | — | — |
| Springhoogte | **127,5 cm** | — | v²/2g = 128 |
| Airtime | **1,008 s** | — | 2v/g = 1,020 |
| Seconden per 360 kijken | **1,500 s** | **0,600 s** | 360 / 240 |
| Kantelsnelheid | **+180 gr/s** (omhoog) | — | StickPitchSpeed 180 |
| Topsnelheid vooruit / zijwaarts / achteruit | **420 / 420 / 357** | 420 / 420 / **420** | 1,00 / 1,00 / 0,85 (Gears 5 TU3) |
| Stick op 0,05, 1 s | **0,00 cm en 0,00 gr** | — | 0 |
| Stick op 0,45, 1 s | **144,3 cm** | — | > 0 |
| Coyote-venster / inputbuffer | **110 ms / 150 ms** | bestonden niet | UE levert ze niet |
| Stap over een stoeprand van 20 cm | **+19,98 cm** | +19,98 | blijft een stap |
| Stap op kniehoogte (50 cm) | **0,00 cm**, stopt na 116 cm | klom er geruisloos overheen | wordt een vault (GDD-verb) |
| Kijksnelheid 2° vóór de pitch-limiet | **52,8 gr/s** (midden: 180) | 180 — volle snelheid tot de klem | Nesky #47 |
| Meshwortel boven de grond | **0,15 cm** | 0,15 — was al goed | Bijlage D beweerde −2 cm, dat klopt niet |

**S1 — "personage schaalt met snelheid": oorzaak gevonden, gefixt, gepind.** Van de vier kandidaten bewegen er drie niet mee: mesh-schaal (1,000), boomlengte (300,0) en FOV (80,0) zijn identiek bij stilstand, rennen en sprinten. Wat wél meebewoog was de **gemeten camera-tot-pawn-afstand**: 312,07 cm stil tegen **342,26 cm rennend**, oftewel de schijnbare hoogte zakte van 31,50° naar 28,84° — **8,4% kleiner van gaan rennen**. Oorzaak is `bEnableCameraLag`: de achterstand is exact `snelheid / CameraLagSpeed`, de enige speed-gekoppelde term in de rig. Gefixt met `CameraLagMaxDistance = 6` uu. Na de fix: rennen→sprinten **0,00%**, stilstand→sprinten **1,67%**.

**S2 — sprint is een toggle op L3, hold op Shift.** Gemeten: L3-toggle levert 650 cm/s. Alle vier de uitstappen (niet meer vooruit duwen, mikken, vuren, nogmaals L3) staan als losse assert; schuin sturen beëindigt hem níét.

**S3 — de dump is betrouwbaar op een toets.** F9 is nu een echte Enhanced-Input-binding in code en heeft géén configlaag nodig. `Saved/Config/` was de verkeerde plek: dat is de gegenereerde laag, van de engine, niet in de repo, teruggeschreven bij afsluiten. De regels staan nu óók in `Config/DefaultInput.ini`, en bij missiestart logt de game hoeveel debug-bindings er werkelijk geladen zijn. De dump zet zijn regel ook op het scherm.

**De missie speelt zichzelf uit.** `Eclipse.Playthrough.M11PlaysItselfFromLaunchToDebrief` start M1.1 via het échte laadpad (`SelectMission` + `AutoLaunch`), loopt met geïnjecteerde input naar het controlepost, vecht, haalt het doelwit neer, loopt naar extractie en bereikt de debrief. Uitkomst van de groene ronde: **geslaagd, 2/3 objectives, dag +1, 25 materialen en 50 credits als commit-feit** (`Event.Economy.ResourcesChanged`, reden `MissionReward`), **regio niet geflipt** (correct per SPEC-P2-04 besluit 6), order beantwoord binnen **0,001 s**, game-thread **0,80 ms gemiddeld / 9,77 ms slechtste** over 2736 ticks, 44,8 s gesimuleerde speeltijd.

**Je squad werkt.** Dat was de belangrijkste reparatie van het EERSTE deel van de nacht, en het waren twee oorzaken die op elkaar leken. Er was **geen navmesh** (nul navigatiegrenzen — de config zei dat dat mocht en dat was een misverstand), en toen die er wel was bleef de squad weigeren omdat hij **93 meter van je vandaan spawnde**: de game mode las je positie vóórdat je naar het insertiepunt verplaatst was. Elke order leverde daardoor een half pad op, en een half pad telt als geen pad — hij weigerde dus terecht. **Gemeten na de fixes: verste squadmate 9282 → 317 cm, pad 48% → 100%, weigeringen 3 → 0, en 2 van de 3 soldaten komen binnen 2,5 s daadwerkelijk dichterbij.** De keten order → antwoord → beweging is nu end-to-end bewaakt; daarvoor controleerde de suite alleen dát er antwoord kwam.

**Drie defecten die de harnas vond en die niemand had kunnen zien door code te lezen:**

1. **Kijken liep 2,5× te snel.** De tuning zegt 240 gr/s en het gidspaneel toont "1,50 s per 360"; het was 600 gr/s en 0,60 s. `bEnableLegacyInputScales` stond aan (engine-default), dus `AddYawInput`/`AddPitchInput` vermenigvuldigden nog met `InputYawScale = 2.5` en `InputPitchScale = **-**2.5` uit `BaseGame.ini`. Het negatieve teken betekende dat de handler een verborgen omkering compenseerde — daarom was "invert Y" niet te beredeneren.
2. **"Take out the patrol leader" was af door erlangs te lopen.** De overlap-trigger voltooide objectives zonder naar het type te kijken, en `DestroyTarget` had zelf géén voltooiingspad. De missie eindigde in een keurige geslaagde debrief zonder dat er iets gebeurd was, en de hele suite stond groen.
3. **Achteruitlopen was even snel als vooruit rennen.** UE kent geen richtingsstraf; `GetMaxSpeed()` is de enige plek waar dat kan.

**De grootste vondst kwam als laatste, en hij was al die tijd onzichtbaar: de camera-blend heeft NOOIT gedraaid.** De gids belooft dat de camera in ~0,2 s naar je ogen schuift bij 1e persoon, en dat mikken hem intrekt. Gemeten bleef alles staan waar het stond — 300,00 cm en 80,0°, onveranderd.

De oorzaak is één regel in de constructor: `bCanEverTick = false`, terwijl de blend zichzelf aanvraagt met `SetActorTickEnabled(true)`. Dat is een **no-op** op een actor waarvan de tickfunctie nooit geregistreerd is. De code las volkomen correct — er stond zelfs een nette 12.4-onderbouwing bij waarom de tick uit hoort te staan — en het dóél werd elke keer keurig gezet. Alleen bewoog er nooit iets naartoe.

**Drie dingen die jij ziet waren daardoor dood:** 1e/3e persoon (C), mikken (RMB/LT) en de Command Mode-uitzoom. Na de fix (tickfunctie bestaat, staat uit tot een blend loopt, zet zichzelf weer uit — de perf-bedoeling blijft exact): 1e persoon boom **300 → 0,00** en FOV **80 → 90**, terug naar 3e persoon exact **300,00**, mikken boom **300 → 165** (0,55×) en FOV **80 → 64** (0,80×).

Dat dit pas om half drie 's nachts boven kwam, komt doordat geen enkele test ooit vroeg wat er ná een druk op de knop *gebeurt* — alleen of de binding bestond. Het is de scherpste versie van de les die vannacht vier keer terugkwam.

**Het hele project is daarna op dit patroon nagelopen** (elke plek die een tick aan- of uitzet tegen de vlag die bepaalt of dat kán): vier plekken, en het personage was de enige foute. Het wapen en de objective-trigger staan bewust en consequent op "nooit tikken", en de Command Mode-component gebruikt exact het juiste patroon — `bCanEverTick = true` mét `bStartWithTickEnabled = false`. Dat is wrang: **het goede voorbeeld stond al in de component die op ditzelfde personage zit**, twee bestanden verderop.

**En toen het wapen zelf, waar drie getallen stonden die niemand had nagemeten.** Het vuurtempo klopt: 14 schoten in 2 s tegen de geschreven 0,15 s interval — het eerste schot is gratis, daarna dertien op tempo. De andere twee niet:

- **Je bereik is 47 m, niet de 50 die in de data staat.** Gemeten raak je tot 4700 cm en mis je op 4900. De kogel vertrekt vanaf de **camera**, en die staat 300 cm achter je, dus een deel van het bereikbudget gaat op achter je rug. Bijkomend: tijdens het mikken trekt de camera in tot 165 cm, dus dan reik je ~135 cm **verder**. Mikken verlengt je wapen, en dat staat in geen enkel getal.
- **Kopschoten doen niets.** 22 hp op de borst, 22 hp op het hoofd. De regel vermenigvuldigt met 2,5 als de geraakte bone "head" heet, maar de trace loopt op `ECC_Pawn` en raakt dus de capsule — en die heeft geen bones. Het code-commentaar noemt dat een graybox-beperking; dat is te mild, want ook met echte skeletten blokkeert de capsule eerst. **Structureel, niet tijdelijk.**

Allebei bewust **niet** gerepareerd: het zijn precies de getallen waar jouw openstaande balansvraag over gaat, en die verplaatsen terwijl jij hem beantwoordt is onbehoorlijk. Eerst kiezen, dan repareren.

**En de vraag één laag hoger: welke EVENTS vuren er ooit?** De catalogus bewaakt dat elk gedocumenteerd event geïmplementeerd is, maar niet dat het afgaat — een event dat alleen uit een console-commando vertrekt telt daar mee als af. De speelronde telt wat er langskomt: **13 in een volledige M1.1**, van de 31 in de catalogus. En de vraag die daaronder lag is 26-07 beantwoord: met een regel per event bij zijn eerste afvuring gemeten dat **elk gecatalogiseerd event ergens afgaat** — de twee die dat gisteren niet deden (`ClassAbilityUsed` en `SoldierStabilized`) vuren inmiddels wel. De meeste ontbrekende horen bij de basis en de campagne en draaien buiten een missie, maar één spoor liep door: van je drie squad-klassen heeft alleen de **Medic** een signature verb met een code-pad. **Momentum** (Assault) en **Killzone** (Sniper) bestaan als tag-identiteit plus getal in `DT_ClassDefs` — de Sniper heeft zelfs een lane-bereik van 6000 cm — maar geen enkele regel vuurt ze af. Eerlijk gelabeld in de code als aanstaand specwerk; van buiten zie je alleen dat twee van je drie klassen zich als een gewone soldaat gedragen.

**En de laatste keer dat die vraag raak was, leverde hij het goedkoopste openstaande werk op: er staan acht ingesproken squad-zinnen klaar die nooit worden afgespeeld.** `Bark_Ack_Move_A/B`, `Ack_Hold`, `Ack_Regroup`, `Down`, en drie weigeringen (`NoRoute`, `NoShot`, `Pinned`). Het stemsysteem bestaat, de opnames bestaan — maar de enige verwijzingen buiten de tests komen van de commandlet die ze *genereert*. Niets speelt ze af. Je squad antwoordt dus in tekst terwijl de stem er letterlijk al is.

**Wat die sweeps opleverden is daarna vooruit vertaald in drie bewakers**, want de vondsten zijn eenmalig en de vraag is herbruikbaar: elke regio is ooit te bereiken (6 van 6, gemeten met een golf-voor-golf-uitbreiding vanaf jouw startgebied), elk regiotype heeft een missie-aanbod én elk aanbod hangt aan een bestaand regiotype (3 van 3, beide richtingen), en elke vastgepinde story-missie wijst naar een regio die bestaat. Alle drie klopten vandaag al — dat is het punt: een bewaker die je pas bouwt nádat het misging, had de fout niet voorkomen.

**Alle taken die op "klaar" staan zijn tegen diezelfde lat gelegd — belooft "klaar" wat het zegt?** Eén had een voorbehoud nodig (P2-01 squad-classes: twee van de drie signature verbs hebben geen code-pad). De rest houdt stand, en dat is expliciet gecontroleerd in plaats van aangenomen: de **audio** is niet alleen bedraad maar hoorbaar — 12 voltooide missies in de laatste suiteronde, nul "sting ontbreekt"-waarschuwingen, dus het bestand laadt echt; de **verloren debrief** rekent netjes af (geen beloning, geen story-beat, de dag kost wél, regio onaangeroerd); en **gezondheid** komt uit data (speler uit de tuning, vijanden uit hun archetype) en niet uit een hardgecodeerd getal.

**Alle console-commando's zijn daarna langs één vraag gelegd: welk mechanisme is ALLEEN via de console bereikbaar?** Van de negentien is er precies één die geen speelbaar pad heeft, en dat is het **alarm** — gemeten in de ronde waarin vier vijanden je zien en neerschieten: de latch blijft uit. De rest heeft wél een echte weg: een regio bevrijden gebeurt bij een geslaagde debrief (`bProgressRegionOnSuccess`), de dag draait via die debrief, en de overige commando's zijn dumps en rapporten die niets veranderen. Dat staat hier zodat niemand die sweep overdoet. **Dezelfde vraag is ook aan alle tien de subsystemen gesteld** — heeft elk een echte aanroeper buiten de tests? Ja: de laagste zijn `EclipseBaseSubsystem` (aangeroepen door economy) en `EclipseEconomySubsystem` (door de basis-hub-widget), en `EclipseAudioSubsystem` staat op nul aanroepen omdat hij per ontwerp een bus-consument is — niemand hóórt hem aan te roepen. Geen dood subsysteem. **En dezelfde vraag op DATANIVEAU** — welk getunede veld wordt nergens gelezen? Achttien velden nagelopen; alle hebben een lezer, met precies één uitzondering die de eerdere vondst bevestigt in plaats van een nieuwe toe te voegen: `KillzoneRangeCm` (6000 cm) wordt gekopieerd naar de klasse-kit en daar door niemand gelezen — het lane-bereik van de Sniper reist mee zonder ergens aan te komen, net als het verb zelf. Eén veld leverde wél iets nieuws op: **`WalkSpeed` wordt alleen door de animatie gelezen**, waardoor wandelen op toetsenbord niet bestaat (§4 punt 20).

**Drie stiltes, en de sweep die erop volgde.** Je opdracht vroeg te noteren of er iets stils gebeurt dat luid had moeten zijn. Dat bleek geen bijvangst maar een categorie:

1. **Een missie logde haar start wel en haar einde niet.** Zie hieronder — dit vond de speelronde.
2. **Neergaan was volledig stil, ook voor jou.** Wie na een sessie `Saved/Logs` opende, kon nergens zien dát hij was doodgegaan, laat staan waaraan. Nu één regel per lichaam, eenmalig: `Player-side DOWN: … (cause: EnemyFire)`.
3. **Een geweigerde order zei niets** — terwijl dat juist de tak is die verklaart waarom een order niets deed. Alleen weigeringen loggen; elke bevestiging loggen zou ze bedelven.

Bij het controleren van punt 3 kwam er iets groters uit dan de logregel: **de weigerregels zijn goed getest, maar alleen als losse functie.** De weg erboven — order geven → beoordelen → bark kiezen → event versturen — had **nog nooit gedraaid**, want een volledige M1.1 levert nul weigeringen op; je squad kan alles wat je vraagt. De bark, de reden en de nieuwe logregel waren dus onbewezen. Dat is nu een test die er één afdwingt en controleert wat jíj merkt: precies één antwoord, geen bevestiging, een reden die niet leeg is, en een hoorbare zin (*"Can't see the target."*).

**Vierde defect, gevonden nadat de rest al groen stond: een missie die eindigde zei niet dát ze eindigde.** Het *starten* werd gelogd (`Mission 'MT_M11' started …`), het *eindigen* niet. Toen mijn speelronde het checkpoint in liep, neerging en de missie faalde op **31,48 s**, was de eventbus de enige getuige — geen logregel, geen reden, en de vier vijanden verdwenen door het opruimen. `ResolveDebrief` logt nu uitkomst, objectives, gevallen squad en alarm (GDD 14.3.5, luid degraderen). Dit is precies het "iets stils dat luid had moeten zijn" uit je opdracht, en het was alleen te vinden door de missie echt te spélen.

**En een vierde soort stilte, die drie keer toesloeg: de tekst klopte over wát werkt en zweeg over wáár het niet werkt.** Bukken was een dode toets (inmiddels gerepareerd). Daarna bleek **stance** alleen te werken ín Command Mode — en de sweep daarna dat het om **vier** controls gaat: stance, volgende, vorige en onder-kruis gaan alle vier door een handler die meteen terugkeert als je Command Mode niet vasthoudt. In de F2-tabel die jij tijdens het spelen leest, stond die voorwaarde bij precies één kolom van één rij. Je drukt dus RB of X in het veld en er gebeurt stil niets.

Alle vier de rijen noemen nu hun context, in beide kolommen. **Het gedrag is niet gewijzigd** — of ze buiten Command Mode zouden móéten werken is een ontwerpvraag, en de tabel eerlijk maken lost het directe probleem op zonder die vraag voor je te beantwoorden. Wat wél nieuw is: een test die het gedrag vastpint in plaats van de tekst, zodat de tabel meeverandert als iemand het gat later dicht maakt.

Dit is het lastigste soort fout van de nacht, omdat **gedragstests hem per definitie niet vinden**: het gedrag ís correct.

**De beschrijvingen hebben nu hun eigen bewakers.** Zes keer vannacht klopte een *beschrijving* niet met de code, en elke keer was het de beschrijving die de volgende ronde de verkeerde kant op stuurde — de 107 gedragstests zagen er geen één. Twee nieuwe tests sluiten dat gat voor de twee plekken die jij tijdens het spelen leest: de F2-controletabel mag geen binding claimen die niet bestaat, en de getallen in de testgids moeten die van `DA_CharacterTuning` zijn. **Allebei gefalsifieerd** — ik heb de historische fout teruggezet en gecontroleerd dat ze rood gaan, want een assert die niet rood kan worden geeft dekking die er niet is.

Wat nog *niet* bewaakt is: de losse documenten (dit bestand, `BESTURING.md`, de feel-referentie). Die zijn vannacht met de hand nagelopen — vijf fouten gevonden, waaronder één in de referentie die tot een verkeerde "fix" had geleid — maar er is geen mechanisme dat ze vasthoudt.

## 2. Wat is niet gelukt, en waarom

- **Wat er aan MIJN kant misging, want dat hoort in deze sectie.** Ik heb één bevinding gepubliceerd die fout was en zelf moeten intrekken ("de vijanden bewegen niet" — ze hadden nooit iemand gezien). Vijf keer bleek een meting fout in plaats van de code: despawnde vijanden, een vallende pawn, de staart van een ADS-blok, een hurkend personage waardoor ik een responscurve verzon die niet bestaat, en twee regio's die op dag 1 niet te kiezen zijn. En zes keer liep een tekst achter op de code, waarvan vijf keer in dít rapport. Geen daarvan heeft iets kapot gemaakt — alles is gemeten, gecorrigeerd en vastgelegd — maar het is wel het eerlijke antwoord op "wat ging er mis".

  **En wat ik ermee gedaan heb, want reparaties tellen alleen als de volgende niet volgt.** Elke fout die ik drie keer of vaker maakte, is daarna systematisch afgezocht in plaats van afgewacht: verouderde tellingen (negen, waarvan de laatste twee alleen door de sweep gevonden), bewakers die ik groen liet zonder ze rood te zien (vijf, allemaal alsnog gefalsifieerd), controls die niet op alle vier de plekken stonden waar jij ze zoekt (twee gaten), metingen die begonnen in een toestand die ik niet gecontroleerd had (de grootste bewegingstest bleek er nog een, en hij klopte toevallig), en codecommentaar dat iets belooft wat de code niet doet (die sweep kwam schoon terug — de vier foute waren de vier die er waren). Die werkregel staat nu ook in mijn geheugen: **na de derde keer dezelfde fout, stop met repareren en ga zoeken.**

- **Niets is definitief mislukt**, en drie audit-items zijn bewust níét gebouwd omdat het bouwopdrachten zijn en geen tuningrondes: turn-in-place (ROT-03, vraagt een draai-animatie of je krijgt voetslip), de sprint-camerastack (CAM-11), en camera-shake/recoil/hitmarkers (§8 FEEDBACK bestaat volledig niet). Zie §4 voor mijn aanbeveling per stuk.
- **De squad weigert ELKE verplaatsingsorder met `NoRoute`** (mét bark: "No route, boss.", "Can't get there from here.", "That path's blocked."). Systeemtechnisch is dat precies goed — elke order krijgt exact één antwoord en de weigering is beredeneerd, nooit stil — maar het betekent dat de squad nergens heen kan. **Ik heb dit uitgezocht en de oorzaak gemeten, en hij is groter dan hij leek.**

  De speelronde rapporteerde: navigatiesysteem aanwezig, één nav-data-actor, en **nul navigatiegrenzen**. Nul grenzen is nul navmesh, en dan faalt `MoveToLocation` altijd — in elke run, niet alleen in de test. `DefaultEngine.ini` zette `bGenerateNavigationOnlyAroundNavigationInvokers=True` met het commentaar *"no authored bounds volume needed"*, en dat is een misverstand: invoker-modus bepaalt wélke tegels binnen de grenzen gebouwd worden, hij vervángt de grenzen niet.

  **Geland:** het district spawnt nu zijn eigen `ANavMeshBoundsVolume` (het bouwt zichzelf uit code, dus de grens hoort daar ook vandaan te komen). Gemeten na de fix: nav-grenzen **0 → 1**, nav-data-actoren 1 → 2.

  **En daarna is het uitgezocht tot het einde.** Ik heb de game écht gestart — `-game`, met de missie draaiend en de frames tikkend — en de navmesh-stand twee keer laten loggen: bij missiestart én vijf seconden later.

  **INMIDDELS OPGELOST, en dat is later op de nacht opnieuw gemeten.** Toen ik dit schreef stond er beide keren "geen navmesh onder de speler", en dat las als een squad die in de echte build nergens heen kan. De rookproef van 03:45 zegt iets anders: *"Navigatie (bij missiestart): 1 grens, navmesh onder de speler = NEE"* en *"Navigatie (vijf seconden later): 1 grens, navmesh onder de speler = JA"*. **De navmesh bouwt dus wél, hij heeft alleen een paar seconden nodig** — precies wat invoker-gestuurde Recast-generatie hoort te doen. Deze alinea beschreef een tussenstand en is blijven staan; dat is de zesde keer vannacht dat een tekst achterliep op de code.

  Drie eerdere diagnostische runs liepen hierop stuk, en de reden was mijn eigen instrumentatie: de nav-regel vuurde één keer, bij missiestart — precies het moment waarop Recast per definitie nog niets gebouwd kán hebben. Een meting op t=0 zegt altijd "nee", ongeacht of het goed komt. Er wordt nu twee keer gemeten, en dat is wat de vraag beantwoordde.

  **En daarna is hij gevonden en gefixt.** De twee logregels maakten de volgende stap goedkoop, dus die is meteen gezet: ik heb de GROOTTE van die ene grens laten meebrengen, en die was **0 × 0 × 0 uu**. Een runtime-gespawnde `ANavMeshBoundsVolume` krijgt zijn brush namelijk van de brush-*builder*, en die draait alleen in de editor — de volume registreert zich dus keurig als grens en is leeg. Dat verklaart precies waarom er "1 grens" stond en tóch nooit een tegel gebouwd werd.

  Opgelost met een gewone box-component op de volume: die telt wél mee in de bounding box die het navigatiesysteem uitleest, kost niets en heeft geen editor-code nodig. **Gemeten in een echte `-game`-run: grens 0×0×0 → 28000 × 28000 × 4000 uu, en vijf seconden na missiestart ligt er navmesh onder de speler (was: nooit).** Bij missiestart zelf staat er nog "nee", en dat hoort — Recast bouwt asynchroon.

  **Wat nog open staat, en waar de jacht precies eindigde.** De squad weigert in het harnas nog steeds, óók nu er navmesh is. Ik heb drie hypotheses gemeten en alle drie sneuvelden:

  | Hypothese | Meting | Uitkomst |
  |---|---|---|
  | De squad valt nog en kan daarom niet paden | tijd tot de squad staat | **0,000 s** — ze stonden al |
  | Het orderdoel ligt buiten de navmesh | doel projecteren | **ligt erop**, en alle 3 de soldaten ook |
  | Het gekozen DEKKINGSpunt ligt achter geometrie, buiten de mesh | punt op de mesh projecteren met terugval | **nog steeds drie weigeringen** |

  Daarna nog twee, en die pellen het verder af:

  | Hypothese | Meting | Uitkomst |
  |---|---|---|
  | De nav-agent van de squadpawn vindt geen nav data | agent opzoeken | **`RecastNavMesh-Default`** — hij resolvet gewoon |
  | Er ís geen pad naar het doel | padzoeker rechtstreeks bevragen | **pad gevonden** (gedeeltelijk) |

  **Dus: er is navmesh, er is een nav-agent, er is een pad — en `MoveToLocation` faalt toch.** Dat is een scherpe, ongemakkelijke stand, en het is precies het soort plek waar plausibel redeneren geld kost. Ik heb nog één ding geprobeerd (`bProjectDestinationToNavigation` aanzetten, want een gedeeltelijk pad wijst op een doel net naast de mesh); dat veranderde niets en is **teruggedraaid** — een wijziging die niets aantoonbaar oplost hoort niet in de boom, hoe redelijk hij ook klinkt.

  **GEVONDEN.** De weigering komt niet uit `MoveToLocation` maar uit de pure beslistabel dáárvoor. `GatherFacts` zet:

  ```
  Facts.bHasPathToTarget = Path != nullptr && Path->IsValid() && !Path->IsPartial();
  ```

  en `DecideOrder` weigert met `NoRoute` zodra dat false is. **Een GEDEELTELIJK pad telt dus als geen pad.** Mijn eigen meting van twee ronden eerder zei het al letterlijk — *"padzoeker = pad gevonden (gedeeltelijk)"* — en ik heb er overheen gelezen omdat ik op dat moment naar `MoveToLocation` zocht. Dat is de duurste soort fout van deze nacht: het antwoord stond al in mijn eigen log.

  **En het is geen bug maar een ontwerpkeuze die zich tegen zichzelf keert.** "Orders zijn beloftes" (GDD 8.4) rechtvaardigt weigeren als je niet volledig kunt voldoen. Maar de gemeten praktijk is dat vrijwel elk pad naar een aangewezen punt gedeeltelijk is — je wijst een plek aan op een muur, in dekking, of een halve meter naast de mesh — dus wordt élke order geweigerd, en dan betekent de belofte niets meer.

  **Dat is jouw beslissing, niet de mijne**, want het gaat over wat een order-belofte betékent. De twee opties staan in §4.

  **Waar de volgende sessie begint, met de aanwijzing erbij.** Vijf hypotheses liggen achter de rug; de resterende ruimte is wat er gebeurt tussen "pad gevonden" en "aanvraag geaccepteerd". Twee concrete aanknopingspunten uit het log:

  - **De engine meldt zelf niets** bij de afwijzing, en dat is geen toeval: `AAIController::MoveTo` schrijft zijn faalreden naar de **visual log** (`UE_VLOG`), niet naar het tekstlog. Vandaar de stilte. Eerste stap is dus die reden zichtbaar maken — de teruggegeven `EPathFollowingRequestResult` meelogen in de weigering, of visual logging aanzetten.
  - Het navmesh-build-blok logt **`agent radius 35.0`** terwijl de capsule van het personage **34** is. Waarschijnlijk onschuldig, maar het is het enige getal in de hele keten dat niet klopt met wat eromheen staat, en het is goedkoop na te kijken.

  De proef eromheen is klaar: de speelronde meet navmesh, nav-agent, pad én weigeringen in één run, dus een poging is binnen een minuut beoordeeld.

  De dekkingspunt-projectie is blijven staan: hij loste dit niet op, maar hij is op eigen merites juist. Een dekkingspunt dat buiten de navmesh valt hoort terug te vallen op het bevolen punt — dat is precies het principe dat in de functie zelf staat: *het order naar de letter uitvoeren wint van het optimaliseren van de geest*.

- **CORRECTIE OP MEZELF: "de vijanden bewegen niet" was fout.** Ik meldde een uur geleden dat 0 van de 4 vijanden van zijn spawnplek komt en noemde dat een defect. Dat was te snel geconcludeerd uit één getal. Een extra logregel — "meld één keer dát je iemand ziet" — leverde **nul meldingen** op: **de vijanden hebben tijdens de hele missie nooit iemand waargenomen.** Ze bewegen niet omdat ze niets weten, niet omdat hun beweging kapot is. Het was een artefact van mijn eigen speelronde, geen gamedefect. Excuses voor de valse melding; hij stond een uur in dit rapport.

- **Wat er WEL onder zat, en dat is een echt punt voor je balans.** Mijn speelronde schakelt de vijanden uit vanaf **4200 cm**, en hun waarnemingsbereik is **2500–3000 cm**. Het spelerwapen (AR_Foundry) staat op **5000 cm** in de data — maar effectief raak je tot **4700 cm**, want de kogel vertrekt vanaf de camera en die staat 300 cm achter je (zie §1). Reken dus met 4700 als je deze balansvraag beantwoordt. Je kunt dus elke vijand in deze missie neerhalen van buiten zijn waarnemingsbereik, zonder dat hij ooit merkt dat er gevochten wordt — en mijn geautomatiseerde speler doet dat ook, want het is de enige manier om met 100 HP tegen vier schutters te winnen.

  Dat is geen bug maar een **bereikasymmetrie**: het spelerwapen overtreft de vijandelijke waarneming met 60–100%. Zolang dat zo staat is out-ranging de dominante, risicoloze strategie en is dekking zoeken zinloos. Of dat erg is, is een balanskeuze — de getallen staan in `DT_Weapons` en `DT_EnemyArchetypes` en het zijn er twee.

  **Gevolg voor de dekking, en dat is het punt dat blijft staan tot jij kiest.** De speelronde meet nu zelf hoe dicht hij ooit bij een vijand kwam: **4169,67 cm**. Zolang dat getal boven hun waarnemingsbereik ligt, worden naderen, dekking zoeken en terugvuren in die ronde **nooit uitgeoefend** — de test is groen en las als "gevecht werkt", maar dekt alleen de helft waarin de vijand niets doet. Dat staat nu als meetregel in de uitvoer in plaats van als aanname. Bewust **geen** assert erop: welke kant het op moet is jouw balanskeuze, en een assert zou het huidige gedrag vastzetten voordat je gekozen hebt.

  **Die helft is inmiddels wél gedekt, en de uitkomst verandert de balansvraag.** Een tweede ronde (`Eclipse.Playthrough.EnemiesEngageWhenYouWalkIntoTheirRange`) probeert niet te winnen maar loopt bewust naar binnen. Gemeten: eerste treffer op **2442 cm**, de vijand komt tot **1097 cm** van zijn spawn naar je toe, en je gaat van **100 hp naar neer in 2,50 s**. **De vijand-AI is dus niet kapot** — hij nam alleen nooit deel. En dit is de keerzijde van de asymmetrie hierboven: buiten hun bereik is het risicoloos, binnen hun bereik heb je tweeënhalve seconde. Dat zijn twee getallen die dezelfde ene keuze beschrijven.

## 3. Beslissingen die ik zelf genomen heb, en waarop

| Beslissing | Grond |
|---|---|
| **De gehurkte capsule krijgt een getunede waarde (124 cm), de engine-default van 80 cm niet.** | 80 cm is kruiphoogte: tegen een staande capsule van 176 cm zak je 55%, terwijl een mens door de knieën ~30% verliest. Die waarde stond nergens tegenover een keuze — dezelfde soort vondst als de rest van het bewegingscomponent bij de audit. Gemeten na: 124 cm, je zakt 52 cm. |
| **De overgebleven engine-defaults heb ik NIET aangeraakt**, maar wel zichtbaar gemaakt — de test `Eclipse.Feel.Layer1.WhichEngineDefaultsAreStillUnchosen` somt ze op met hun default ernaast, dus dat lijstje veroudert niet in dit document. | Een default kán de juiste waarde zijn — hij is alleen niet gekozen, en dat verschil hoort leesbaar te zijn in plaats van dat "nooit bekeken" als "bewust zo" leest. Van de zes op het BEWEGINGScomponent zou ik er vijf laten staan (de spring arm heeft zijn eigen regels hieronder): `WalkableFloorAngle` (44,8° is een normale helling), `GravityScale` (1,0, en de sprongboog is al getuned via `JumpZVelocity`), `LedgeCheckThreshold` (4 cm), en de twee `AirControlBoost`-waarden (die verdubbeling onder 25 cm/s helpt je van een richel af stappen en is engine-ontwerp, geen ongeluk). De zesde, **`JumpMaxHoldTime = 0`, is een echte keuze**: langer drukken springt niet hoger. Voor een aardse tactische shooter is dat verdedigbaar — Gears en The Division hebben ook geen vrije variabele sprong — dus mijn advies is laten staan, maar je moet weten dát het zo staat. |
| **Geen camera-rotatielag** (`bEnableCameraRotationLag` blijft uit), en dat is nu een keuze in plaats van een default. | Rotatielag maakt de camera zachter maar het richten mushy: je kruis loopt achter je stick aan. Voor een shooter wint precisie van vloeiendheid — de POSITIE-lag (die er wél is, geklemd op 6 uu) geeft het gewicht, zonder je vizier te vertragen. |
| **`bInheritRoll` blijft aan, maar staat genoteerd als val voor §8.** | Vandaag onschadelijk: de control rotation heeft geen roll, dus de boom rolt nergens van. Zodra camera-shake landt en die roll gebruikt, roteert de hele boom mee in plaats van alleen het beeld — dat is het soort verrassing dat je bij het bouwen wilt weten, niet erna. |
| **Hold of toggle hangt aan de DUUR van de toestand**, niet aan het apparaat alleen. Momentaan op een schouderknop → hold op beide; momentaan op een stickknop → hold op toetsenbord, toggle op de pad; aanhoudend → toggle op beide. | De hele bindingslijst is er langs gelegd (`BESTURING.md`). Alleen sprint wijzigde; mikken, Command Mode en stance bleven, nu mét reden. |
| **Hurken blijft een toggle op beide apparaten.** | Hurken is de stealth-default (GDD 04) en die houd je minuten aan — dat is "aanhoudend", niet "momentaan". Een hele infiltratie op Ctrl is een handkramp, geen feel. |
| **`bEnableLegacyInputScales` uit**, en de stick wordt daarmee 2,5× trager. | De getunede waarde is de waarheid, of het getal betekent niets. De muis blijft exact even snel (`MouseLookScale` 1,0 → 2,5, een kale schaal zonder eenheid — daar telt gedrag). |
| **De camera-lag geklemd op 6 uu in plaats van uitgezet.** | Camera-lag bestaat om kleine, schokkerige correcties glad te strijken, niet om een halve meter afstand te kopen. Boven ~72 cm/s zit hij op zijn klem en is hij dus constant. |
| **"Patrol leader" is één doelwit**, niet de hele patrouille van vier. | Het objective zegt enkelvoud. Met "alle vier" hield één schutter achter dekking het objective 94 seconden open terwijl de missie inhoudelijk klaar was — de speelronde liet dat meteen zien. |
| **Beloningen worden gemeten aan commit-feiten, niet aan wallet-delta's.** | De dagtick boekt legitiem eigen inkomsten en uitgaven; het saldo ging −84 credits terwijl de missie er 50 uitkeerde. Zelfde discipline als de M1.1-Gauntlet. |

**Eén ding wil ik expliciet noemen, want het is een correctie op jouw melding.** Je meldde dat het personage GROTER wordt naarmate je sneller loopt. De meting zegt het omgekeerde: kleiner, 8,4% tussen stilstand en rennen. Eén defect, twee lezingen — wat vaststaat is de koppeling en de grootte, en die is nu weg. Wat "nagenoeg onzichtbaar bij langzaam lopen" was, weet ik dus nog niet zeker; als je dat na deze build nog steeds ziet, is er een tweede oorzaak en dan zoek ik verder met een meting.

## 4. Wat op jou wacht — met mijn aanbeveling, zodat ja of nee volstaat

| # | Vraag | Mijn aanbeveling |
|---|---|---|
| 1 | **Speel de build en zeg of S1 weg is.** Druk F9 terwijl je langzaam loopt en nog eens terwijl je sprint; de regel staat op je scherm. | Doe dit eerst — het is de enige open vraag uit jouw sessie waar ik geen meting voor heb. |
| 2 | **Kijken is nu 2,5× trager (1,50 s per 360 in plaats van 0,60 s).** Goed zo, of te traag? | **Eerst zo laten.** 240 gr/s is jouw eigen getunede waarde en 600 gr/s is fors boven de genre-band. Te traag? Dan is het één getal: `StickYawSpeed` in `DA_CharacterTuning`. |
| 3 | **AFGEHANDELD 26-07.** De 73% blijft en de GDD-regel 4.1.1 is meegecorrigeerd naar 520 cm. De dode knop `CameraPullbackPercent` is verwijderd, dus er is nu één waarheid in plaats van twee. <br><sub>Oorspronkelijk: ~~**Command Mode trekt de camera 73% terug (300→520), maar `DA_CommandModeTuning` zegt 15% en de GDD 4.1.1 ook.** **LET OP — deze vraag is veranderd:** ~~</sub> | Niets meer te beslissen. |
| 4 | **De navmesh bestaat nu** — gemeten in een echte `-game`-run: 28000 × 28000 × 4000 uu grens, en vijf seconden na missiestart ligt er navmesh onder de speler. De oorzaak was een lege grens (0×0×0): een runtime-gespawnde volume krijgt zijn brush niet buiten de editor. | **Kijk of je squad je nu volgt — maar de meting is inmiddels positief.** Toen ik dit schreef weigerden ze in het harnas nog; later op de nacht bleek de oorzaak dat ze 93 meter verderop spawnden, en na die fix accepteert de squad **alle vier de ordertypes** (elk met precies één antwoord en een eigen zin) en komen twee van de drie soldaten binnen 2,5 s daadwerkelijk dichterbij. Wat ik niet kan zien is of het er in beeld ook naar uitziet. |
| 5 | **Hurken ook als hold aanbieden, als optie naast de toggle?** | **Ja, maar later** — het vraagt een instellingenmenu, en dat is SPEC-P2-07. |
| 6 | ~~Coyote time + sprong-inputbuffer bouwen?~~ **Gedaan** — 110 ms en 150 ms, beide gemeten en gepind. Ze voegen alleen vergeving toe: een sprong die eerst mislukte lukt nu, nooit andersom. | Niets te beslissen; speel het en zeg of het te toegeeflijk voelt. |
| 7 | **AFGEHANDELD 26-07.** Beantwoord zoals je vroeg: er zit in **geen enkele pack** een `Turn_*`-take, dus dit is assetwerk en geen tuningronde. Wat er wél is: het lichaam draait mee boven 90° (waar Gears en Division het ook leggen), zodat je niet meer 180° van je eigen lichaam kunt wegkijken. De voeten schuiven daarbij — dat is precies het gat dat de animatie zou vullen. <br><sub>Oorspronkelijk: ~~**Turn-in-place bouwen** (nu blijft je rug bevroren staan als je alleen de camera draait)?~~</sub> | Niets meer te beslissen. |
| 7b | **`AdsLookMultiplier`: 0.35 (nu) of 0.60?** **Nu in seconden, want zo voel je het:** een volle 360° kost tijdens het mikken **4,29 s**, tegen 1,50 s vanaf de heup — **2,86× trager**. Met 0.60 zou het ~2,5 s zijn. Het commentaar redeneerde naar 0.60 terwijl het veld op 0.35 stond — het commentaar is gecorrigeerd, de waarde niet aangeraakt. | **Laat 0.35 staan tot je hebt gemikt.** Beide zijn onderbouwd (0.35 = wat CoD/Apex verschepen, 0.60 = richting de meetkundig neutrale 0.745). Dit is smaak, en jij hebt ADS nog niet beoordeeld. |
| 7c | ~~Elke missie begon met een val van 1,6 meter die 0,35 s duurde~~ — **opgelost**. De pawn werd 100 cm bóven het insertiepunt gezet, en dat punt staat zelf al boven de vloer. Nu wordt er naar de grond getraceerd en staat hij meteen. Gemeten: **160,7 → 0,0 cm** en 0,35 → 0,017 s. | Niets te beslissen. Ik had dit eerst als "vraagt beeld" op jouw lijst gezet en dat was te snel geoordeeld — het harnas kan het perfect meten, en de geslaagde speelronde bewijst dat hij niet in de vloer spawnt. |
| ~~4b~~ | ~~Telt een gedeeltelijk pad als "ik kan er komen"?~~ **Vervallen** — die conditie was nooit het probleem. Hij weigerde terecht een onmogelijke order, want de squad stond 93 meter verderop. Nu de squad naast je staat is het pad 100% en wordt de order aangenomen. Geen keuze meer nodig. | ~~Telt een GEDEELTELIJK pad als "ik kan er komen"?~~ Dat is de hele oorzaak van de weigerende squad: één conditie, `!Path->IsPartial()`. Nu geldt: gedeeltelijk = geen route = weigeren. | **Ja, laat een gedeeltelijk pad tellen — mits de soldaat dan zegt hoe ver hij komt.** "Orders zijn beloftes" pleit voor weigeren, maar in de praktijk is bijna elk pad naar een aangewezen punt gedeeltelijk (je wijst een muur aan, dekking, of net naast de mesh), dus weigert hij álles en betekent de belofte niets. Een soldaat die zegt *"tot daar kom ik"* en dan gaat, houdt de belofte beter dan een die "no route" zegt en blijft staan. Maar dit gaat over wat een order bij ons betékent, dus het is jouw call. |
| 8 | **Camera-shake, recoil, hitmarkers, sprint-camerastack** — §8 FEEDBACK bestaat volledig niet. | **Na de vorige punten.** Dit is de grootste feel-winst die er nog ligt (Gears koopt met 1,2× sprint méér snelheidsgevoel dan wij met 1,55×), maar het is een bouwopdracht van meerdere sessies. |
| 9 | **AFGEHANDELD 26-07.** Opgelost door je eigen beslissing: het schot verraadt je nu binnen 50 m, dus de asymmetrie is weg zonder aan één getal te komen. Gemeten: vanaf 42 m komen alle vier de vijanden in beweging naar de plek waar je schoot. <br><sub>Oorspronkelijk: ~~**Bereikasymmetrie.** Jouw wapen staat op 5000 cm maar raakt effectief tot **4700** (de kogel vertrekt vanaf de camera); de vijand ziet je pas op 2500~~</sub> | Niets meer te beslissen. |
| 10 | **Omhoog kijken duwt de camera in je rug** — van 300 cm naar 84 cm vanaf ongeveer +25°. Omlaag kijken doet niets. | **Kijk eerst zelf omhoog in de build.** Het getal is groot, maar of het stóórt hangt af van hoe vaak je omhoog kijkt. Wil je het weg: `ViewPitchMax` naar ~+45 is één waarde; de camera hoger aanhaken is netter maar bouwwerk. |
| 11 | **Je valt 3,5× sneller uit de sprint dan je erin komt** (0,042 s tegen 0,150 s). | **Laten staan tot je gesprongen en gerend hebt.** Snel je controle terugkrijgen is verdedigbaar; het verschil is alleen nooit gekozen — versnellen loopt via de acceleratie, terugzakken via de wrijving. Voelt het abrupt, dan trek ik de uitstap naar ~0,10 s. |
| 12 | **In de lucht kun je ~213 cm opzij sturen** met aanloop — ruim een halve renseconde. *(Correctie op mezelf: ik meldde eerst 287 cm, maar die sprong begon uit stilstand en daar verdubbelt UE de luchtsturing via `AirControlBoostMultiplier`. 213 cm is het getal dat je werkelijk voelt.)* | **Laten staan tot je gesprongen hebt.** 0,35 komt uit Epic's template en is verdedigbaar, maar is nooit tegen dit spel afgewogen. Voelt het zwevend, dan is 0,20 de logische tussenstap. |
| 13 | **AFGEHANDELD 26-07.** Drie van de vier hebben werk gekregen: RB wisselt het camerastandpunt, X geeft een hergroepeer-order, en LT is geen moduskeuze meer maar altijd mikken. **Y (stance) blijft met opzet dood** — zie rij 18. <br><sub>Oorspronkelijk: ~~**Vier knoppen doen niets buiten Command Mode** (stance, volgende, vorige, onder-kruis). De tabel zegt dat nu; het gedrag is niet aangeraakt.~~</sub> | Niets meer te beslissen. |
| 14 | **AFGEHANDELD 26-07.** Optie 1 gebouwd: eigen zinnenpool voor Downed. Hij zegt nu *"I'm hit - can't move."*, *"I'm down, boss."* of *"Need a medic, not an order."* <br><sub>Oorspronkelijk: ~~**Een neergeschoten soldaat zegt "No route, boss."** De reden klopt in de data (Downed), maar de zin komt uit de pool van het ordertype en wijst je du~~</sub> | Niets meer te beslissen. |
| 14b | **AFGEHANDELD 26-07.** Betaalt uit. Gemeten: **20 materiaal**. Een voorwaarde-objective (conditie-vlag, geen doelwit) vinkt zichzelf af bij de debrief. <br><sub>Oorspronkelijk: ~~**De +20-bonus voor een ronde zonder gewonden kan niet uitbetaald worden.** Hij hangt aan een optioneel objective dat *voltooid* moet zijn, en niets v~~</sub> | Niets meer te beslissen. |
| 15 | **AFGEHANDELD 26-07.** Doen 2,5×. Gemeten: **44 hp romp tegen 110 hp hoofd**. Hitbox op de hoofd-socket, met terugval bovenin de capsule voor lichamen zonder mesh. <br><sub>Oorspronkelijk: ~~**Kopschoten doen niets extra** — 22 hp op borst én hoofd. De ×2,5 staat in `DT_Weapons` maar de trace raakt de capsule, en die heeft geen bones. Stru~~</sub> | Niets meer te beslissen. |
| 16 | **AFGEHANDELD 26-07.** Gaat af bij de eerste waarneming. **Let op:** nagemeten uit de assets eist vandaag géén enkele optional stilte, dus het kost je nog niets — het staat klaar voor je eerste stealth-optional. <br><sub>Oorspronkelijk: ~~**Het alarm kan tijdens spelen niet afgaan.** Vier vijanden zien je en schieten je neer — de latch blijft uit. De enige aanroep is een console-command~~</sub> | Niets meer te beslissen. |
| 17 | **Twee van je drie squad-klassen doen niets bijzonders.** Alleen de Medic (Stabilize) heeft een code-pad; Momentum en Killzone bestaan als tag plus getal. | **Na de feedback-laag**, net als de stance-splitsing — een klasse die anders vecht is alleen te beoordelen als je ziet en hoort dat er gevochten wordt. Killzone hangt bovendien aan Command Mode, en die uitzoom werkt pas sinds vannacht. |
| 18 | **Stance wisselt zichtbaar en doet niets.** De soldaat onthoudt hem en leest hem nergens; de HUD-regel springt wel om. | **Laat het staan tot de klasse-verbs aan de beurt zijn** — het is dezelfde beslissing. De gidsstap zegt nu hardop dat je de régel controleert en niet het gedrag. |
| 19 | **AFGEHANDELD 26-07.** Alle acht klinken, met een rem van 2 s per soldaat. De woordenlijst-mismatch is opgelost: `NoShot` hoort bij `NoLineOfSight`, de wees `Pinned` is uit de seed gehaald (hij beloofde suppressie, en die mechaniek bestaat niet), en de twee ontbrekende zinnen zijn geschreven en ingesproken — een ack op `FocusTarget` en een weigering op `InvalidTarget`. Gemeten: **8 van de 8** aangesloten zinnen hebben een clip. <br><sub>Oorspronkelijk: ~~acht ingesproken squad-zinnen worden nooit afgespeeld~~</sub> | Niets meer te beslissen. |
| 20 | **Op toetsenbord kun je niet wandelen.** `WalkSpeed` (180) wordt alleen door de animatie gelezen; het bewegingscomponent mikt altijd op `RunSpeed`. WASD zijn digitaal, dus daar loop je altijd 420. Op de stick werkt het wel — de respons is lineair, dus 180 zit op ~43% uitslag. | **Laat het tot je met beide apparaten gespeeld hebt.** Het valt pas op als je op toetsenbord ergens doorheen wilt sluipen. Wil je het: een aparte wandeltoets, en Alt is bezet door stance — dus dat is een bindingskeuze van jou. |
| 21 | ~~**Drie van je vier missies beschrijven een vijandopstelling die nooit gebeurt.**~~ **AFGEHANDELD 26-07 op jouw opdracht.** `EnemySpawns` wordt gelezen: Assault plaatst er 6, Rescue 5, Sabotage 4, en M1.1 houdt zijn vier omdat hij geen batches authordt. In een echte run bevestigd (*"5 vijanden uit 2 geauthorde batch(es)"*). Een onbekend archetype wordt luid gemeld en overgeslagen in plaats van stil vervangen. | Niets meer te beslissen. Wat je nog moet doen is het **spelen**: het aantal schuift nauwelijks (+2, +1, +0) maar de vórm verandert — groepen op hun eigen site in plaats van vier op een hoop. |

## 5. De eerlijke stand: speelt het beter?

**Ja, en dat is meetbaar — maar niet overal waar jij het gemeld hebt.**

Wat aantoonbaar beter is: het personage verandert niet meer van grootte als je van tempo wisselt (8,4% → 0,00% tussen rennen en sprinten). Stoppen kost nu 150 ms en 27 cm in plaats van 83 ms en 12 cm, dus er is massa die tot stilstand komt in plaats van een figuur die op zijn plek staat. Achteruitlopen is 15% trager dan vooruit rennen in plaats van even snel. De sprint zit op L3 waar je hem verwacht en blijft aan tot je iets doet wat hem hoort te beëindigen. En de camera draait op de snelheid die in de tuning staat, wat hij eerder aantoonbaar niet deed.

**En je squad doet iets.** Dat is geen tuning maar een systeem dat werkte-op-papier en niet-in-de-game: hij weigerde elke order met een keurige, beredeneerde weigering — het contract was intact, alleen kon hij nergens komen. Dat is precies het soort defect dat een groene testsuite niet vindt en een speler meteen ziet.

Waar ik terughoudend ben: de rest is vier tuningwaarden en één camera-klem. Ze halen het onnatuurlijke eraf; ze voegen nog niets toe. De vijf kanalen waarmee Gears zijn snelheidsgevoel koopt — camera zakken, shake, blur, ademhaling, niet kunnen vuren — hebben we er nul van. Zolang §8 FEEDBACK leeg is, blijft schieten "de vijand valt om" in plaats van "ik raakte hem". Dat is het volgende grote ding, en het is echt bouwen, geen afstellen.

**Dat oordeel moet ik corrigeren, en het was te bescheiden.** Ik schreef hierboven "de rest is vier tuningwaarden en één camera-klem". Daarna ging de audit van *beredeneerd* naar *gemeten*, en vier dingen veranderden het beeld:

- **De camera bewoog helemaal niet** bij 1e persoon, mikken en Command Mode. Drie dingen die je elke minuut gebruikt, allemaal dood, allemaal met een correct gezet doel. Geen afstelling maar een reparatie — en het eerste wat je gaat merken.
- **De vijand-AI is niet kapot; hij deed alleen nooit mee.** Loop je zijn bereik in, dan ziet hij je op 2442 cm, komt 1097 cm naar je toe en legt je in **2,50 s** van vol naar neer. Mijn eigen speelronde won juist dóór dat te vermijden, waardoor de hele vijandkant ongetest bleef terwijl alles groen stond.
- **Eén audit-rij was zelf fout** en had tot een verkeerde reparatie geleid: de pitch-limieten zouden "bij −70 in de grond duiken", maar gemeten gebeurt daar niets — het zit aan de andere kant, waar omhoog kijken de camera van 300 naar 84 cm duwt.
- **Drie dingen die stil gebeurden zijn nu luid** (missie-einde, neergaan, geweigerde orders). Het tweede is het vervelendste geweest: wie na een sessie de logs opende, kon niet eens zien dát hij was doodgegaan.

Daarnaast zijn er drie dingen bijgekomen die **niet** kapot zijn maar wel ontbreken, en die samen verklaren waarom vechten leeg aanvoelt: kopschoten doen niets (de trace raakt de capsule), het alarm kan tijdens spelen niet afgaan (de enige aanroep is een console-commando), en twee van je drie squad-klassen hebben geen signature verb. Alle drie zijn eerlijk gelabeld in de code als aanstaand werk — maar van buiten zie je alleen dat er niets gebeurt.

En één daarvan is goedkoper dan de rest bij elkaar: **de stemmen liggen klaar en worden niet afgespeeld.** Dat is geen bouwopdracht van sessies maar één consument op de eventbus — en het is precies het kanaal waarmee een gevecht ophoudt stil te zijn.

Wat daarmee nog steeds waar is, en scherper dan vanavond vroeg: **het onnatuurlijke is eraf en de camera doet eindelijk wat hij beloofde, maar aan het GEVECHT is nog niets toegevoegd.** §8 FEEDBACK blijft leeg, en die leegte is nu beter in kaart: het is niet alleen shake en recoil, het is ook dat raak schieten, betrapt worden en klassen die verschillen allemaal nog geen uitkomst hebben.

En één ding dat niet over feel gaat maar wel over vertrouwen: de missie speelt zichzelf nu uit, van start tot debrief, met asserts op de uitkomst. Dat vond in de eerste ronde meteen een objective dat afging zonder dat er geschoten was — met een volledig groene testsuite eromheen. Elke fix die vannacht landde is daarna opnieuw uitgespeeld als bewijs dat er niets brak.

---

> **Live progress dashboard:** open [`PROGRESS.html`](PROGRESS.html) in a browser and leave it open — it reloads itself every 60 s. Facts (commits, tests, newest screenshots) refresh automatically every 10 min via `start_progress_watcher.bat` (→ `Tools/update_progress.ps1`). Dev sessions update the judgment percentages by editing **`progress_data.js`** at milestones — never edit PROGRESS.html itself, and never hand-edit `progress_auto.js`. Owner instructions for the audio pipeline + studio working method are recorded verbatim in [`phase0/OWNER_MANDATE.md`](phase0/OWNER_MANDATE.md).

> **Nathan's simpele pagina = [`START_HIER.html`](START_HIER.html)** (kindertaal, leest live uit `progress_data.js`). Dit is de pagina die de owner opent — PROGRESS.html is de detailversie. **Elke dev-iteratie MOET `progress_data.js` bijwerken**: het veld `bijgewerkt` (tijd), de `taken`-statussen, `ownerActies` (weghalen wat af is), en een geverifieerde owner-actie verhuizen naar `jijGedaan` met tijd. Bewerk NOOIT START_HIER.html direct. Verse-chat startprompt staat in [`NIEUWE_CHAT_PROMPT.txt`](NIEUWE_CHAT_PROMPT.txt). Géén `/loop`/ScheduleWakeup gebruiken (vuurt onbetrouwbaar).

> **Read order for whoever picks this up:** this file → `DOCUMENTATION_README.md` → `00_INDEX.md` → `13_roadmap.md` (ACTIVE_MILESTONE) → `14_ai_dev_instructions.md`. Then the phase specs in `phase0/specs/`.

---

## 1. What this project is

**ECLIPSE: Rise of the Resistance** — a single-player third-person Action-Strategy RPG built in **Unreal Engine 5.8 (C++)**. Full design lives in the bible (docs `00`–`15`). It is a ~4.5-year, 6-phase game (roadmap `13`). We are in **Phase 1 — Prototype "The Loop"** (deliberately ugly graybox; prove the risks, not the graphics).

This is **not** a web app and does **not** deploy to Vercel — it is a native UE game that runs in the Unreal Editor / a packaged build.

---

## 2. Where everything lives (one repo)

As of the consolidation commit, the **design bible and the UE game are in ONE git repo** rooted at `C:\Dev\ECLIPSE_GDD\`, so a single `git clone` gets everything (docs + code + full history).

| What | Path (relative to repo root) |
|---|---|
| **Design bible** (docs 00–15, `phase0/` specs, this file, SETUP.md) | repo root |
| **The UE game** (code + content) | `Eclipse/` |
| **Save & push safety net** | `push-all.bat` / `push-all.ps1` |

Repo is local-only until pushed (see §7). Save-and-push any time with **`push-all.bat`** (double-click) — commits everything and pushes so the other machine can `git pull`.

**Progress is readable in two places:**
1. `git log` inside `Eclipse\` — each commit is one spec landed (the real build record).
2. `13_roadmap.md` → `ACTIVE_MILESTONE` block — the single source of "where we are."
3. This file's §4 (status) and §5 (what's next).

---

## 3. Hardware note (why a stronger machine matters)

This dev box is a **2017 laptop: NVIDIA GTX 1050 (4 GB), i7-7700HQ, 32 GB RAM**. It is a fine **graybox / logic / CI** box (Phase 1 runs on it), but it is **not** the fidelity target: no hardware ray tracing (no RT cores), Lumen only in reduced software mode, 4 GB VRAM can't stream 4K/8K. The **visual quality work (Phase 2+) belongs on a stronger RTX-class machine** — see the new `15_visual_quality_charter.md` (§15.2) for the dev-box-vs-target split. So handing the high-fidelity phases to a stronger PC is exactly the intended plan.

---

## 4. Current status (green bar re-verified 2026-07-25 18:40)

**Phase 1 landed and re-reviewed; Phase 2 specs are landing on a green bar every commit.**

- Build: `EclipseEditor Win64 Development` **Succeeded** (UE 5.8, always `-NoUba`).
- Automation tests: **95/95 pass** (`Automation RunTests Eclipse`, headless `-nullrhi`).
- Data validation: **clean** (`EclipseValidateData`, 4 validators / 0 errors).
- Event catalog: **29/29 in sync** (`Eclipse/Tools/check_event_catalog.py`).

**How to play it right now** (this is what the owner tests): launch the game and press
**F3** for the in-game test guide — 20 steps that walk you through every control, tick
themselves off as you use them, and end with the 13.2 questions. **F2** gives the
feel-gauntlet overlay with the five R3 criteria. Both write one shared summary block to
`Saved/Logs`. `-EclipseStartMission=<RegionId>` (e.g. `TransitCheckpoint`) skips the hub
and lands straight in a mission; `-EclipseShot` runs the fixed-camera review rig and
deliberately hides all debug UI.
- Voice pipeline: **live-verified** against the ElevenLabs PCM endpoint (8/8 lines generated + imported + assigned; re-run = 8 cache hits, 0 API calls).

**Systems in place (headless-proven):** event bus · campaign state + transaction API + save v0 · deterministic economy ledger · 6-node strategy mini-map · mission runtime + debrief consequences · squad of 2 with ordered actions and *reasoned refusals* (never-silent) · roster + permadeath + memorial stub · menu base hub + preparation flow · playable graybox layer (character/GAS health, hitscan combat, enemy AI, code-built district).

The full loop is **proven headless** (test `Eclipse.Base.Prep.FullCircleSmoke`: advance day → select → intel spend → launch → win → consequences → second loop) and is now **wired live** — playable in PIE with no console commands (menu base → launch → graybox mission with squad orders → extraction → debrief → base).

---

## 5. What's next

**Voor de owner: één speelsessie.** Die levert in één keer wat geen enkele
geautomatiseerde check kan geven — het **R3-verdict** over Command Mode en het
**13.2-antwoord** ("speel je vrijwillig een tweede ronde?"). De game begeleidt die
sessie zelf via F3. Het kliklijstje op het dashboard zegt in vijf stappen wat er
sinds gisteren veranderd is en waar je op moet letten.

**Voor de volgende sessie: de drie audits zijn de wachtrij.** Ze staan in
`phase0/` en vervangen de losse lijst die hier stond, want die dupliceerde §1 en
§4 en liep daardoor achter:

| document | wat er nog open staat |
|---|---|
| [LOCOMOTIE_AUDIT.md](phase0/LOCOMOTIE_AUDIT.md) | één omissie (mikken kost geen snelheid) en assetwerk: geen `Turn_*`-take in welke pack dan ook, en vijf van de negen lichamen zonder zijcycli |
| [GEVECHT_AUDIT.md](phase0/GEVECHT_AUDIT.md) | de hitmarker (de rest van de feedbacklaag is 26-07 gedaan), spreiding en terugslag, en de loadout die het wapen niet bereikt |
| [SQUAD_AUDIT.md](phase0/SQUAD_AUDIT.md) | één vraag in drie vormen: mag de squad iets doen zonder order? Dat is de grootste beslissing die er ligt |

**Herhaal die audits aan het eind van elke werkdag.** Dat is niet ceremonieel: op
26-07 waren twee van de vier locomotie-omissies pas zichtbaar dóór een fix van
diezelfde ochtend. Elke reparatie verandert wat de volgende ronde vindt.

**Twee gereedschappen die de vorige sessies hebben opgeleverd**, en die de vraag
"wat ligt hier dood?" beantwoorden zonder erover te struikelen:

- `python Eclipse/Tools/find_dead_fields.py` — tuningvelden die niets leest
- `python Eclipse/Tools/find_dead_assets.py` — audiocues die niemand afspeelt

**Bekende dekkingsgat, technisch:** de basis-hub-widget heeft geen enkele test,
terwijl dat de laag is waar elke missie start en elke aankoop gebeurt. De logica
eronder is wél gedekt (`EclipseBaseTests`, `EclipsePrepTests`,
`EclipseEconomyTests`); het is de bedrading die blind is. Op 26-07 zijn daar twee
bugs gevonden door te lezen — een genegeerde uitkomst en een ontbrekende refresh —
en structureel dichtgezet met één gedeelde afhandeling, maar een test is er niet.

## 6. How to build, test, and run (any Windows machine with the toolchain)

Prereqs: **UE 5.8** (Epic Launcher) + **Visual Studio 2022** with the "Game development with C++" workload. Set `UE_ROOT` to the engine, e.g. `C:\Program Files\Epic Games\UE_5.8`.

```powershell
# 1. Generate VS project files (right-click Eclipse.uproject → Generate, or:)
& "$env:UE_ROOT\Engine\Build\BatchFiles\Build.bat" EclipseEditor Win64 Development `
  -project="<repo>\Eclipse\Eclipse.uproject" -WaitMutex

# 2. Run the tests (same as CI)
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<repo>\Eclipse\Eclipse.uproject" -ExecCmds="Automation RunTests Eclipse; quit" `
  -unattended -nopause -nosplash -nullrhi -log -ReportExportPath="<repo>\Eclipse\Saved\Automation"

# 3. Data validation
& "$env:UE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "<repo>\Eclipse\Eclipse.uproject" -run=EclipseValidateData -unattended -nopause -nosplash

# 4. Play: open Eclipse.uproject in the editor → Play In Editor.
```

CI definition (needs a self-hosted Windows+UE5 runner): `Eclipse\.github\workflows\ci.yml`.

---

## 7. How another (stronger) computer takes over

The clean mechanism is **git**. The bible + code are already **one repo** (§2), local-only until pushed.

**One-time setup — private GitHub repo:**
1. `gh auth login --hostname github.com --git-protocol https --web` → authorize in the browser (kimi can drive it).
2. From the repo root: `gh repo create eclipse-rise --private --source . --remote origin --push` (creates the private repo AND pushes everything in one go).

**From then on — keep it synced (this is the auto-push command):**
- Double-click **`push-all.bat`** (or run `push-all.ps1`). It commits all current work and `git push`es. Use it before switching machines or whenever Claude nears a session limit.

**On the strong PC:** install the toolchain (`SETUP.md`), `git clone https://github.com/<owner>/eclipse-rise.git C:\Dev\ECLIPSE_GDD`, build (§6), read this file + `ACTIVE_MILESTONE`, continue at §5. **Full day-one migration guide — with UML/BPMN diagrams, the Max/Fable login, the secrets vault, and a paste-ready Fable prompt — is in [`MIGRATION_TO_STRONG_PC.md`](MIGRATION_TO_STRONG_PC.md).**

**Alternatives if you don't want GitHub yet:** `git bundle create eclipse.bundle --all` (one file, full history) transferred by USB/drive/server; or push to the existing Rocadelo server the project already came from.

**What the strong PC's Claude should be told:** *"Read `C:\...\ECLIPSE_GDD\HANDOFF.md`, then continue Phase 1 at §5. Stay inside the ACTIVE_MILESTONE. Fidelity work starts at Phase 2 per `15_visual_quality_charter.md`."*

---

## 8. Handy pointers

- Governance / coding rules: `14_ai_dev_instructions.md` (naming `AEclipse*/UEclipse*/FEclipse*/EEclipse*`; commits `[System] Verb summary (GDD ref)`; event-bus-only cross-system; tunables in DataAssets; PLACEHOLDER tags).
- Event registry: `Eclipse\Docs\EventCatalog.md` (keep in sync with code, checked by `Tools/check_event_catalog.py`).
- Save contract: `12_technical_design.md` §12.3.
- Performance budgets: `12_technical_design.md` §12.4.
- Visual target + hardware reality: `15_visual_quality_charter.md`.
- **Alle gemeten feel-waarden opzoeken:** `python Eclipse/Tools/show_measurements.py [woord]` — toont de metingen per test, bijvoorbeeld `… sprint` of `… camera`. Het pakt het nieuwste log dat écht metingen bevat en zet de bestandsnaam erboven, dus draai je de hele groene bar (waarna validatie en catalogus het log overschrijven), dan vindt hij nog steeds de testronde. Het houdt geen eigen kopie van de getallen bij en kan dus niet verouderen ten opzichte van de tests.
- **Dode tuningvelden vinden:** `python Eclipse/Tools/find_dead_fields.py` — somt elk `UPROPERTY(EditAnywhere)`-veld op dat nergens buiten zijn eigen header gelezen wordt, dus elke knop die je kunt verdraaien zonder effect. Draai hem na het toevoegen van tuningvelden: een veld dat je authordt maar niet uitleest is onzichtbaar kapot. De docstring zegt wat hij níét ziet (Blueprints, dode code).

---

## GEPARKEERDE WIJZIGING (2026-07-25 17:25) — owner test de game

Twee palet-wijzigingen voor dressing-iteratie 3 staan **als patch geparkeerd**, niet
in de werkboom:
`<scratchpad>/pending_pool_barrier.patch` (3749 bytes), toepassen met `git apply`.

Inhoud: (1) stap 4 — de pool herderiveerd naar authored ×0.35-0.40 omdat hij op
6.25× zijn eigen vloer stond waar het ontwerp 2.18× zegt (drie onafhankelijke
bewijzen in de patch-comment); (2) stap 7 tweede bisectiestap — de barrière van
authored 0.0945 naar 0.046, want 0.0945 landde op frame 0.2909 waar het doel ≤0.15
is. Beide zijn NIET gebouwd en dus niet geverifieerd.

**Waarom geparkeerd in plaats van in de boom gelaten:** de owner ging de game testen.
Als Unreal hem vraagt de modules te herbouwen, zou hij ongeverifieerde palet-waarden
binnenkrijgen en een andere build testen dan wat er gecommit is. Boom staat nu exact
op de gecommitte staat. Na zijn testsessie: patch toepassen, build, shotronde,
meten tegen de doelen hierboven, dan committen.

## AFGEHANDELD (2026-07-25 16:45) — inmiddels geland als `a333013`, hier voor de historie

`Eclipse/Source/Eclipse/Core/EclipseGrayboxBuilder.cpp` bevatte één niet-gecommitte
wijziging: de `SpawnGen(LampGlow, ...)`-aanroep is verwijderd. Reden staat in het
commentaar ter plekke: de tweede art-review vond die emissieve plaat als een
letterlijke sticker plat in de straat aan de mastvoet (shot 00092, (1105,628)-
(1215,655), 0.2745 lum, hue 34.8°, harde zilveren rand + ink-outline). Mijn eerdere
afwijzing van die hypothese was fout — ik vertrouwde de authored transform in
`gen_street_props.py` in plaats van te controleren wat er na export/import
overblijft.

**Niet gecommit omdat de bar niet gedraaid kon worden:** de owner-editor staat open
sinds 16:27 (Live Coding blokkeert UBT, exit 6 — memory `eclipse-editor-buildlock`).
Zodra het build-slot vrij is: build → suite (verwacht 85/85) → shotronde →
committen. De emissieve bulb die de bron zichtbaar maakt is al geland (`865a77b`),
dus het district houdt zijn lichtbron ook zonder deze wijziging.

## LAATSTE STAND (2026-07-25 ~15:30 — CYCLUS 6 GELAND: vier bouwsporen + review-ronde)

**Bar bij de commits: build ✓ (-NoUba) · tests 85/85 (0 fail; 55 schoon + 30 met bekende fixture-warnings) · EclipseValidateData 4 validators/0 fouten · catalog 29/29.** Bar is bewust twee keer gedraaid: de tweede run dekt exact de gecommitte boom (een test-helper-guard landde ná de eerste build, dus die eerste run dekte hem niet).

1. **`e8bd5c8` [Quests]** — M1.1-Gauntlet op de geshipte keten (DA_CampaignSetup → DT_StoryMissions-pin → MT_M11.uasset), win- én verliespad. Rewards gemeten aan de commit-eigen ResourcesChanged-feiten i.p.v. wallet-delta's (de dagtick boekt legitiem extra inkomsten).
2. **`b068212` [Strategy]** — liberation-trigger: één writer, één transactie, state-derived idempotentie + vierde ValidateData-validator + `Tools/setup_liberation_data.py`.
3. **`67f46a6` [Quests]** — Taak-4-kern: Event.Mission.PhaseChanged, alarm als benoemde sub-fase, alarm/casualty-latch, optional-payouts atomair in de debrief-transactie. Catalog 29/29 + `bRequiresNoCasualties` als spec-amendement in SPEC-P2-04 §Data schema.
4. **`c6cf7fe` [Base]** — walkable vault (NIEUW `Base/EclipseVaultBuilder.*`): pure `PlanSlots` + parity-Gauntlet, her-render op de bestaande Event.Base.*-feiten, gecoalesceerd op plan-hash (1 rebuild per commit, geen tick).
5. **`966c800` [Art]** — dressing-iteratie 2: dusk-vloer, lamp-pools + blob-schaduwen als luminantie-decals (district is unlit), volledige nudge-lijst, `generate_decals.py`.

**Reviewronde:** 5 lenzen over de drie complete changesets, elke blocker/major langs een tegenlezer die hem moest wéérleggen. Uitkomst: 5× GO, 13 majors weerlegd, **1 bevestigd** — EB-2's wiring-tests bouwden de twee productiefeiten zélf in hun eigen orde, dus een omgedraaide commit/broadcast in `ResolveDebrief` zou de liberation stil doden terwijl alles groen bleef. Gefixt met `Eclipse.Strategy.Liberation.WiringDebriefSeamFlipsTrio` (draait het échte debrief-pad en pint commit-voor-broadcast + beide tabel-koppelingen) plus twee nieuwe validator-checks. Verder verwerkt: twee majors op de eigen Gauntlet (verlies-test stelde niet vast wélke missie liep; vier ongeguarde region-dereferences) en een liegende logregel bij een lege liberation-resolutie.

**Owner-besluit geland:** 15.7 = **B (hybride MetaHuman-shading)**. Vastgelegd in `phase0/metahuman_recipes.md`; implementatie-spec `phase0/MH_FACE_TIER_B.md` met twee vondsten: de bestaande restyle-lus is slot-blind (vervangt élk slot op index = optie A), en de body-gain 3.2 is bewust ongemeten omdat "bodies klein op het scherm zijn" — bij een dialoog-close-up valt die rechtvaardiging weg.

**Eerstvolgende stappen:** (1) commandlet-slot: `generate_decals.py` → `import_generated_decals.py` → shotronde (eerste PNG = warm-up, overslaan) → art-review op de pool/blob-vraag; (2) `setup_liberation_data.py` NIET vóór M1.3 authored is — de nieuwe validator zou dan correct melden dat de rij nooit kan vuren; (3) wave 2: M1.1 zero-casualty-optional (LET OP: `setup_story_missions.py` schrijft objectives alleen in `if created:`, en MT_M11 bestaat al — er is een eigen migratiestap nodig, staat als comment in het script), recap-materialisatie → cold-reader (harde poort vóór M1.2), civilian-wiring; (4) menu-hub-retirement als eigen mini nu de parity groen is.

## VORIGE STAND (2026-07-25 ~13:15 — CYCLUS 5: de vijf wacht-changesets)

**Trigger:** Nathan sloot de editor om 13:06 (Live-Coding-buildlock weg, memory eclipse-editor-buildlock) — de voorbereide landing rolde daarna in één run. **Unie-bar vóór de commits: build ✓ (-NoUba, 69 s) · tests 68/68 (50 schoon + 18 met bekende fixture-warnings, 0 fail) · EclipseValidateData 3 validators/0 fouten · catalog 28/28.** Alle vijf changesets hadden vóóraf review-GO (2 verse reviews, 1 herkeur, art-review, planner ×2).

1. **`6ac96c1` [Strategy]** — P2-04 stap 2: ResolveOfferForRegion als het ene offer-pad, 5 native Story.Beat.*-tags, per-tabel-validatiepass (luid, 1× per tabel), map her-rendert op BeatReached. (De StaffAssigned-comment-hunk reed bewust hier mee.)
2. **`6b08bb2` [Art]** — 15.8-dressingronde 1 op de Imported-accepts; magenta-container → mix 0.45; iteratie-2-spec staat in `phase0/DRESSING_ITERATIE_2.md` (kerninzicht: district is unlit → licht via luminantie-decals).
3. **`173d4ec` [Quests]** — completion-beat atomair in de debrief-transactie (skip-if-set load-bearing); verlies committeert nooit story-voortgang.
4. **`d503651` [Base]** — P2-03 stap 4-5A: casualty-releases, muster-gate, mutatie-laag-cap + 3 tests; catalog-rij eerlijk.
5. **`9be3786` [Strategy]** — P2-05 pure core: monotone liberation-resolve + beat-gate + 6 tests (veldnamen spec-conform na review-B1).

**Direct hierna (zelfde sessie):** commandlet-rij in de vrije slot — `setup_story_missions.py` (MT_M11 + DT_StoryMissions + DA_CampaignSetup, daarna her-bar + [Quests]-datacommit) → `inventory_metahumans_wave2.py` (report-only MH-probe → aftekening Nathans MetaHuman-taak) → `import_modular_civilians.py` (11 CC0-civilians + SOURCES.md + [Art]-datacommit). **Cyclus N+1 staat gepland in `phase0/CYCLUS_N1_PLAN.md`** (main M1.1-Gauntlet eerst, dan 4 element-builders per fence-matrix; wave 2 = optional-schema, recap-materialisatie + cold-reader, civilian-wiring; M1.2 = N+2, gated op cold-reader 4/4). Taak-4-spec: `phase0/TAAK4_STORY_SURFACE.md`; recap-copy: `phase0/RECAP_CARDS_M1.md`.

## VORIGE STAND (2026-07-24 — P2-01 GELAND)

**Waar gestopt:** Phase 2 "Thirteen Bullets" is de actieve milestone (owner-instructie 23-07; de 13.2-playtest van Phase 1 blijft een staande owner-actie). **SPEC-P2-01 (Squad van 4 + eerste 3 classes: Assault/Medic/Sniper) is geland en gepusht** na een volle groene bar én een onafhankelijke code-review (verdict GO, 0 blockers).

**Deze sessie (samengevat):**
- *Groene bar P2-01:* build ✓ (-NoUba) · **tests 38/38** (6 nieuwe, incl. v0-fixture-migratietest én een echte v2-fixture-test) · **EclipseValidateData 3 validators / 0 fouten** · catalog **21/21**.
- *Checkpoint 14.3.6 (R6) vervuld:* save-schema v2→v3 (roster-ClassId-tail) mét migratie-entry + fixture-tests in hetzélfde commit; pre-v3 saves landen deterministisch op classless (NAME_None).
- *DoD-gat gedicht:* EclipseValidateData dekte DT_ClassDefs niet — nieuwe derde validator toegevoegd (verb-familie, verb↔tunable-consistentie, weapon/body-cross-refs per campaign-setup, negatieve-tunable-vangnet voor script-gevulde tabellen).
- *Review-fixes in dezelfde changeset:* M1 — medic-her-dispatch: na een afgeronde triage-run krijgt een tweede casualty alsnog zijn poging (peek `CanStabilizeSoldier` voorkomt shuttle-loops op geredde/verlopen patiënten); m2 stilte-log; m3 struct-checks; m5 asset-telling. Follow-ups genoteerd: m4 (spawn-fan-offsets → SquadTuning), m6 (cover-scorerconstanten → tuning), m7 (catalog-formulering SoldierDowned-consumer).
- *Hygiëne:* `__pycache__/` ge-gitignored. `Eclipse/Content/MetaHumans|Atira_LODSettings|Locodrome` staan bewust untracked tot de MetaHuman-curatie (aparte taak) besluit wat reist.
- *Sessies 23-07 (niet eerder in dit bestand bijgeschreven):* zie de changelog in `progress_data.js` — o.a. body-pipeline (RAISOR/Belica via DT_BodyDefs), westgevel-B district-breed (412f14f), eerste audio-ronde gebankt (348b7f0), SPEC-P2-02/-P2-03 geschreven + gereviewd, MetaHumans-basis geland (908 MB, curatie in de rij).

**Vervolg dezelfde dag (middag/avond — drie sporen parallel gebouwd):**
- **P2-02 Stage A geland** ([Command]-commit): hold Q/pad-LB → 0.30-dilatatie via pure state-machine (`EclipseCommandLogic`) + wrapper-component met sluitende fail-safe (release/death/mission-end/travel → exact 1.0; tick alleen tijdens de hold), per-soldier-selectie (cycle + reticle-pick, range uit DA_CommandModeTuning) door het bestaande IssueOrder-contract, `Event.Command.ModeEntered/Exited` (catalog 23/23), debug-HUD, `Eclipse.Command.Dump`, 3 headless tests. Review GO; de MAJOR (pick-range was dode data) + alle minors in dezelfde changeset gefixt. **Feel-gauntlet (R3-verdict, ~20 min) staat nu in Nathans kliklijst — draaiboek: `phase0/FEEL_GAUNTLET_P2-02.md`.** Stage B start pas ná verdict "true"; bij "false" geldt de fallback-ladder uit de spec.
- **P2-03 stap 1-2 geland** ([Base]-commit, via element-builder + eigen review): DT_Facilities-schema, slot-graph-asset, DA_BaseTuning, `FEclipseBaseState` mét save v3→v4-migratie + byte-getrouwe v3-fixture in hetzelfde commit (14.3.6/R6 tweede keer bewezen); spec-startstand = type-default (gratis L1 Command Center op Slot_A); hash-dekking uitgebreid. Stap 3 (wrapper + Event.Base.*) = volgende iteratie.
- **Pack-slim-ronde af** ([Art]-commit): alle 11 curatie-accepts naar `/Game/Art/Imported` (repo-tracked, provenance in SOURCES.md), builder-refs via 3 mini-stubs pixel-gelijk gehouden (string-swap = eerste edit 15.8-ronde), Minions+SciFi10 van schijf — **~5,9 GB vrij**, scan-bewijs 0 refs (ASSET_CLEANUP §7-10).
- **m4/m6/m7-follow-ups geland** ([Squad]-commit): spawn-fan + cover-scorer-constanten naar DA_SquadTuning (gedrag-neutraal), catalog-formulering eerlijk gemaakt.
- **SPEC-P2-04 geschreven + main-review verwerkt** (concept ACCEPTED): geen prologue (recap-cards + cold-reader-falsificatie), R7-Gauntlet als bouwstap 1, nul nieuwe objective-primitieven, M1.3 als enige world-state-change via de P2-05-seam, Brick = Assault-rosterrecord; de 4 open punten beslist in de spec zelf.
- **Groene bar (unie, alle sporen samen): 47/47 tests · validatie 3 validators/0 fouten · catalog 23/23.**

**Cyclus 3 (avond — R7 groen, MetaHumans gecureerd, P2-03 stap 3 + P2-04 stap 2):**
- **R7 = GROEN, geland** (`60014e6`): de missie-runtime draagt aantoonbaar authored missies (M1.1-skelet op het echte laadpad; mandatory-set, rewards, regio-onaangetast) — M1.2-M1.4-authoring vrijgegeven. In dezelfde changeset de **debrief-dag-regel** (P2-03 locked decision 4: elke missie kost mechanisch een dag, win óf verlies, mét dag-asserts op beide paden).
- **MetaHuman-curatie geland** (`15dfe61`, review GO): SentinelC (908 MB) verplaatst naar zijn geauthorde root (dode kruisrefs gerepareerd), Kaya provisioneel bedraad via setup_metahuman_data.py, single-writer-fix op setup_character_data.py, .gitignore dekt de machine-lokale character-content, 15.7-beslisdocument (aanbeveling B) bij owner/art-review. Builder-string-swap naar de Imported-paden (`7036849`).
- **P2-03 stap 3 geland** (`69b5f4b`, review GO): transactie-API + Event.Base.* ×4 (uitsluitend commit-emissie) + UEclipseBaseSubsystem-wrapper + alle 5 stap-1-2-bevindingen mét tests + gematerialiseerde Hollow Point-data; zes stap-4-5-follow-ups verankerd in de taak. **P2-04 stap 2 Quests-deel geland** (`16ce2bd`, review GO): DT_StoryMissions-schema + pure story-sequencing. **Audio 16.12 geïmporteerd** (`937f35f`): bed/cues/sting als engine-assets; bed-plaatsing + sting-koppeling lopen. **Eindbar unie: 54/54 tests · validatie 0 · catalog 27/27.**
- **SPEC-P2-05 geschreven + geaccepteerd** (Foothold-trio; geen nieuwe events — LiberationPhaseAdvanced gedropt als P2-00-amendement; bouw ná StoryFlags v5).

**Cyclus 4 (avond — StoryFlags v5 + audio-runtime):** [Strategy] `fd38933` = StoryFlags geland als DERDE bewezen R6-migratie (v4→v5, byte-getrouwe fixtures, SetStoryFlag-mutatie met atomaire duplicate-reject, Event.Story.BeatReached, catalog 28/28); [Audio] `6b48ba4` = never-silent Kessara-bed (2D + attenuation-override + anti-stapel-cleanup) en de verzets-sting op Mission.Completed via UEclipseAudioSubsystem (pure bus-consumer, BusContract-test). Suite 57/57. M1.1-authoring-script (`Tools/setup_story_missions.py`) staat klaar voor de slot.

**Eerstvolgende stappen:** (1) owner: **feel-gauntlet** (R3, bovenaan de actielijst) + MH-kliks (Frey/Hannah/Mason) + 15.7-keuze; (2) code: Strategy-offer-precedence + native Story.Beat.*-tags + M1.1 authored (P2-04 bouwstap 2-afronding; script klaar) → daarna debrief-beat-koppeling en M1.2-authoring; Stage B wacht op het R3-verdict; (3) builder-rondes: 15.8-dressing (cold-DDC-check, stub-verwijdering, shotronde + 15.7-vergelijk) · P2-03 stap 4-5 (walkable vault, 6 verankerde follow-ups).

---

## VORIGE STAND (2026-07-22, ~21:40 CET — EERSTE SESSIE OP DE STERKE PC)

**Waar gestopt:** eerste Fable 5-sessie op de sterke PC (na de installateur-bootstrap). Werkboom schoon, alles gecommit en gepusht; **groene bar hier onafhankelijk herbevestigd: build ✓ (-NoUba), 31/31 tests ✓, validatie 0 fouten ✓, catalog 19/19 ✓.**

**Deze sessie (samengevat):**
- *Machine gemeten:* **GTX 1080 Ti, 11 GB VRAM** — volle SM6/DX12 (Nanite, VSM, TSR, software-Lumen), maar **géén RT-cores**: hardware ray tracing kan hier niet. Charter 15.2 heeft nu een aparte rij (C) voor deze machine; HWRT-validatie blijft RTX-klasse-werk.
- *Owner-revisie §15 verwerkt (bindend):* Borderlands-leunend blijft gelockt, **fidelity erbínnen omhoog** — Nanite-polygoondichtheid, opgeschroefde software-Lumen-GI, rijkere post (SSAO/bloom/film grain), meer particles, TSR hoog. Vastgelegd in 15.5 ("Fidelity revision").
- *Graphics-kalibratie (taak A) — inktlijnen LIVE:* beide materiaal-scripts gedraaid (`M_EclipseToon` banden 0.55/0.10; `PP_EclipseInk`). Eerste live-ronde onthulde een klassiek depth-Sobel-artefact: **de scherende vloer vloeide vol inkt** (heel middenveld zwart). Fix: edge-detectie omgebouwd naar **Laplaciaan (2e afgeleide)** in `author_outline_material.py` — vlak-interieurs schoon onder elke kijkhoek, silhouetten + gevel-vloer-naden krijgen wél lijn. Tweede shotronde geverifieerd: vloer loopt door tot de horizon, lijnen schoon, schemerpalet staat. Galerij (`progress_media/shot_01..04.jpg`) ververst.
- *Kleine carry-over:* rename `Entries`→`EntryPoints` van de installateur gecommit (gedragsneutraal, groen geverifieerd).

**Vervolg diezelfde avond (alles groen + gepusht):** (1) shot-rig gefixt — pawn vliegt tijdens de rig, camera 4 kadreert nu het hele district; (2) hatch-tuning — arcering leest als penseelstroken (25% duty, periode 120); (3) SM6-fidelitypad in de builder, feature-level-gated zodat de SM5-laptop identiek blijft: volumetric smog, zon-schaduwen (lichtschachten door de silhouetten), realtime skylight, film grain 0.07 + bloom 0.45 (15.5-revisie); (4) **Kessara-skyline** (03.3) als code-built placeholder: deterministische ring (seed 503) buiten de perimeter — 56 fabriekshulks, 18 schoorstenen, 12 kraanportalen, natrium-oranje raamstroken; het district staat nu visueel in een stad, zonder één nav/missie/test-raakvlak.

**Nog later die nacht — asset-pass gestart (taak C, owner gaf blanket-akkoord):** eerste echte textures in de game via het autonome CC0-spoor: 4×2K diffuse van Poly Haven (asfalt/betonblok/golfplaat/metaalplaat; herkomst + licentie in `Eclipse/Content/Art/Textures/SOURCES.md`, import via `Tools/import_polyhaven_textures.py`). `M_EclipseToon` heeft nu een world-aligned albedo-pad (default neutraal); de builder koppelt per palet-entry texture/schaal/gain/mix. **Kalibratieles:** texturen her-meterden de auto-exposure (schemer→dag); opgelost door per texture het lineaire gemiddelde te meten en gain = 1/gemiddelde te zetten (shader klemt op 2.5) — de dusk-grade is exposure-invariant onder texturing. Geverifieerd met shotronde; groene bar opnieuw volledig groen.

**FAB-WACHTRIJ (voor de eigenaar, ~10 min):** Epic Games Launcher is op deze pc nog nooit gestart/ingelogd. Stappen: (1) Launcher starten → inloggen met het Epic-account (2FA bij de hand); (2) in de browser op fab.com met hetzelfde account inloggen; (3) melden — dan volgt een concrete shortlist (industriële modulaire kit, straatprops, Paragon-characters) waarop alleen "Add to library" + licentie-akkoord geklikt hoeft te worden; import + toon-restyle gebeurt daarna autonoom. Ook nuttig: ElevenLabs-key scopes Music/SFX aanzetten voor taak D-liveruns (TTS werkt al).

**Loop-iteratie 2 (zelfde nacht, alles groen + gepusht):** texture-variatie luminantie-only gemaakt (palet = enige kleur-autoriteit; Dominion-post weer zalm-oxide) · straat-dressing gelegd (hoofdader + dwarsstraat + markering + vlekken, no-collision) · 12 natrium-checkpointstrips op de binnenmuren · **SPEC-P2-00** (Vertical Slice-overview) via subagent opgeleverd + gereviewd in `phase0/specs/` — de ACTIVE_MILESTONE-omzetting naar Phase 2 blijft de call van de eigenaar (na de 13.2-playtest).

**Loop-iteratie 3:** lit-toon-migratie gebouwd als vlag-gated A/B (`M_EclipseToonLit`, DefaultLit, alleen via `-EclipseLitToon` op SM6; Glow blijft unlit). A/B-verdict: bij schemer op command-afstand ~niet te onderscheiden — unlit blijft default; het lit-pad wint pas bij interieurs/dag/characters. Alles groen.

**Loop-iteratie 4:** eerste echte 3D-meshes geland — CC0-props (vat/wegbarrière/krat, Poly Haven-API) door de toon-pijplijn via een nieuw mesh-UV-pad (UVMode-param); alle slots MID; les: bijna-neutrale ×10-paletten tonemappen naar bleek → vat op donker roest. Tools: Pillow ✓, ffmpeg ✓, Blender in UAC-wachtrij (geen blokker). Eigenaar: fab.com-login ✓; nog open: Launcher-login + shortlist-kliks, ElevenLabs-scopes. Character-route gekozen per Bible: **CC0-stylized (Quaternius) nu** voor squad/vijand-placeholders (past bij gelockte 15.5 + volledig autonoom), **MetaHuman-evaluatie bij Phase 2→3** voor hero-companions; Mixamo-animaties = klik-lijst voor de eigenaar zodra de animatie-pass start (geen browser-besturing in deze omgeving), met Quaternius' CC0-animatiebibliotheek als autonoom alternatief.

**Loop-iteratie 5:** Pillow-decals live — generator `Tools/generate_decals.py` (abstract AEGIS-oog, hazard, verzets-eclips; luminantie-only zodat het palet de kleur bepaalt), import + plaatsing als no-collision planes (compound-posters, kruisings-pads, stencils bij Entry_Main/warehouse); gains gemeten (7.8/1.3/7.1). Eigenaar heeft fab.com klaargezet + shortlist ontvangen (Industry Props Pack, modulaire industriële kit, Paragon Lt. Belica, Quixel-decals) — wacht op "Add to library"-kliks + "packs staan erin"-sein.

**Loop-iteratie 6:** Quaternius-characters binnen (gdown; BlueSoldier M/V + 2 burgers, volledige animatiesets) en als Idle-figuren geplaatst (enforcers goud bij poort/checkpoint, burgers teal bij warehouse). Lessen: `used_with_skeletal_mesh`-flag verplicht (anders zwart in -game) en verzadigde paletten vereist op de ×10-range. Fab-monitor draait; `_fab_inbox` bestaat; eigenaar is ingelogd op Launcher + fab.com en heeft de klik-route voor "Add to Project".

**Bekende volgende zwakste punten (15.8-lijst):** (1) zodra Fab-packs landen: de kit-pass (graybox-gebouwen vervangen — monitor vuurt vanzelf); (2) squad/vijand-BODY-swap naar Quaternius-meshes in AEclipseCharacter (gameplay-pass mét squad-scenario-suite); (3) meer figuren + patrol-loop (Walk-anim langs de hoofdader); (4) particles (vonken/as); (5) district-verticaliteit (03.3); (6) lit-toon bij interieur/dag.

**Owner-afspraken die blijven gelden (elke sessie):** geen installaties/downloads/acceptatie-prompts zonder expliciet akkoord vooraf (eerst uitleggen waarvoor); bouwen met `-NoUba`; PROGRESS.html nooit bewerken (data in `progress_data.js`; `progress_auto.js` is van de watcher). `gh` is hier NIET geïnstalleerd — push werkt via de vault-token in Windows Credential Manager (username gepind in repo-config).

**Eerstvolgende stap:** (1) hatch-tuning + shot-rig-fix, nieuwe shotronde ter review; (2) eigenaar speelt de loop in PIE — gate-vraag 13.2 — met squad-barks en Xbox-controller; (3) taak B: fidelity-pass per herziene 15.5 op deze machine; (4) taak C (art-pass, Fab/Quixel) — **wacht op per-download akkoord**; Epic Games Launcher staat op deze PC (Fab-toegang loopt via het ingelogde Epic-account); (5) taak D: Music/SFX-endpoints + adaptieve muziek (16.7).

---

## VORIGE STAND (2026-07-22, ~17:00 CET — laptop)

**Waar gestopt:** einde van de derde Fable 5-sessie van vandaag (voice live + graphics-doorbraak + controller + migratiedraaiboek). Werkboom schoon, alles gecommit en gepusht; **build groen, 31/31 tests groen, validatie 0 fouten, catalog 19/19**.

**Deze sessie (samengevat):**
- *Voice-pipeline LIVE bewezen:* dialogue-seed (JSON → voice-assets, create-only) + eerste echte ElevenLabs-run: 8/8 barks gegenereerd, geïmporteerd, auto-toegewezen; herhaalrun = 8 cache-hits / 0 API-calls. Cache gecommit (`9e33eec`, `2a67a09`). Key werkt volledig — de eigenaar heeft de ontbrekende scope aangezet; 23 stemmen zichtbaar via `/v1/voices`.
- *Graphics-doorbraak (pass 20–27 forensics):* het oude `PP_EclipseOutline`-postmateriaal bleek het HELE beeld te overschilderen — elke "paarse waas" was dat materiaal, niet de scène. Nieuw cel/toon-materiaal `M_EclipseToon` (Unlit, licht-banden + hatching in-shader; `Tools/author_toon_material.py`) bewezen werkend (rood/groen-diagnose). Builder: toon-MID's per paletkleur met hue-shifted schaduwen, zon –25°, exposure-bias –1.0, atmosfeer weer zon-gekoppeld. Inktlijnen her-authored als **`PP_EclipseInk`** (`Tools/author_outline_material.py`, verified scene-passthrough) — **beide scripts nog één keer draaien** (band-defaults 0.55/0.10 + nieuw ink-asset) en dan één `-EclipseShot`-ronde ter verificatie; tot dan rendert het district zonder lijnen (netjes afgevangen).
- *Xbox-controller + muis (owner request):* zelfde acties, gamepad-keys erbij (linkerstick lopen, rechterstick kijken, RT vuren, LB agressieve stance, D-pad squad-orders 1–4, B hurken, stick-klik sprint). Feel-pass (curves/deadzones/deltatime) bewust Phase 2 — PLACEHOLDER getagd.
- *Migratiedraaiboek:* `MIGRATION_TO_STRONG_PC.md` volledig herschreven als dag-één-draaiboek: **consent-protocol** (uitleg → akkoord vóór elke installatie/download; bouwen met `-NoUba` tegen firewallprompts), installatietabel mét redenen, secrets-flow, het bindende **asset-beleid** (algemene assets downloaden via Fab/Quixel ná akkoord; hero-assets pas handgebouwd op gelijk kwaliteitsniveau), de eerlijke laptop↔RTX-werkverdeling en in **§7 de bootstrap-prompt** voor de nieuwe PC (eindstaat: Visual Studio open met de solution, Fable 5/max ingelogd, groene bar, screenshots getoond — dan pas ontwikkelwerk).

**Owner-afspraken die blijven gelden (elke sessie):** geen installaties/downloads/acceptatie-prompts zonder expliciet akkoord vooraf (eerst uitleggen waarvoor); bouwen met `-NoUba`; PROGRESS.html nooit bewerken (data in `progress_data.js`; `progress_auto.js` is van de watcher).

**Bekende SM5-devbox-limiet (gedocumenteerd in code):** directionele lichten verlichten op het D3D12-SM5-fallbackpad van de GTX 1050 geen horizontale vlakken betrouwbaar; het Unlit-toonmateriaal omzeilt dit structureel (banden in de shader).

**Eerstvolgende stap:** (1) de twee materiaal-scripts draaien + één screenshotronde (kalibratie-verificatie — kan op deze laptop, geeft alleen even het reviewvenster); (2) eigenaar speelt de loop in PIE — gate-vraag 13.2 — hoort daarbij de eerste squad-barks en kan de Xbox-controller meteen testen; (3) migratie: `C:\Dev\ECLIPSE_SECRETS` per USB meenemen en op de nieuwe PC de **bootstrap-prompt uit `MIGRATION_TO_STRONG_PC.md` §7** plakken; (4) Phase-2 forward-infra wanneer gewenst: Music/SFX-endpoints + adaptieve muziek (16.7).
