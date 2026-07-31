# DEBUG DISCIPLINE — hoe een bug in uren wordt opgelost in plaats van in sessies
*Werkdocument | aangemaakt 2026-07-31 | owner-klacht: "VS Claude werkt veel te lang aan één ding"*
*Bindend voor: main, element-builder, hud-builder*

---

## 0. De klacht, en waarom hij terecht is

De eigenaar merkte op dat bugs zoals het **zwevende wapen** en het **trillen bij het schieten** onevenredig lang duren. Het bewijs staat in de repo zelf:

> *"Het zichtbare INSLAGSPOOR rendert niet. **Twaalf oorzaken uitgesloten**… Ik heb er **drie conclusies over teruggenomen**."*
> *"Het TRILLEN is de schietpose die per schot pulseert… **Twee reparaties geprobeerd en allebei teruggedraaid** omdat ze hun eigen meting niet haalden."*

Dat is geen pech. Dat is een **methodefout met een naam**: hypothesen serieel elimineren door code te lezen, in plaats van het draaiende systeem te observeren. Twaalf uitgesloten oorzaken zonder diagnose betekent twaalf iteraties die elk niets uitsloten wat ertoe deed.

**En de duurste fout van allemaal:** beide bugs zijn **bekende Unreal-problemen met gedocumenteerde oorzaken**. Ze stonden de hele tijd in de Epic-forums en in de Epic-documentatie. Tien minuten zoeken had meerdere sessies bespaard. Zie §4.

---

## 1. Het protocol — in deze volgorde, altijd

### Stap 0 — Is dit bekend engine-gedrag? *(max 10 minuten)*

**Doe dit vóór je één regel code leest.** Zoek de bug op met engine-termen, niet met projecttermen:

- ✗ "Eclipse weapon floating" — dit vindt niets, het is jouw project
- ✓ "Unreal Engine weapon socket lag behind animation"
- ✓ "UE5 layered blend per bone jitter"
- ✓ "Unreal decal not rendering deferred"

Zoek in: Epic Developer Community forums, de officiële UE-documentatie, UDN, GitHub-issues op de UE-mirror, en de Epic-changelogs van jouw versie.

**Waarom dit eerst komt:** je bouwt op een engine van twintig jaar oud met miljoenen gebruikers. De kans dat jouw symptoom uniek is, is klein. De kans dat het een bekend gedrag is met een bekende oorzaak, is groot. Deze stap kost tien minuten en heeft in dit project aantoonbaar meerdere sessies gekost door hem over te slaan.

**Uitkomst:** een bekende oorzaak (ga naar stap 4) of niets (ga naar stap 1).

### Stap 1 — Reproduceer minimaal *(max 20 minuten)*

Nooit debuggen in de volledige game. Bouw de kleinst mogelijke reproductie:

- Lege testmap
- Eén personage, default skeleton als het kan
- Eén wapen
- Geen AI, geen missielogica, geen HUD

**Verdwijnt de bug in de minimale opstelling?** Dan zit hij in wat je weghaalde — en je hebt zojuist 90% van de zoekruimte geëlimineerd in twintig minuten. **Blijft hij?** Dan heb je nu een reproductie die in twee seconden draait in plaats van in twee minuten, en elke volgende test is tien keer sneller.

### Stap 2 — Observeer, lees geen code

Code lezen levert *hypothesen*. Observatie levert *feiten*. Twaalf hypothesen zijn minder waard dan één meting.

**De UE-observatiegereedschapskist** (gebruik deze — dit is waar het misging):

| Vraag | Gereedschap |
|---|---|
| Wat doet de animatie echt? | **Animation Debug**: `ShowDebug ANIMATION` — toont actieve nodes, blend weights, montages live |
| Welke bone staat waar? | Skeleton-viewer + `a.AnimNode.*` cvars; Rewind Debugger (UE5) om frame-voor-frame terug te spoelen |
| Hoe blenden de gewichten? | **Rewind Debugger** op de AnimBP — zie de gewichten oscilleren in plaats van erover te speculeren |
| Wat tickt wanneer? | `stat game`, tick-groepen, `-LogCmds="LogTick Verbose"` |
| Rendert het wel en zie ik het niet? | Console `Vis`, `r.ShowMaterialDrawEvents 1`, RenderDoc-capture |
| Verandert een waarde per frame? | `stat unit`, custom `stat` counters, of gewoon een `UE_LOG` per frame gedurende 60 frames |
| Wat gebeurde er precies? | **Unreal Insights**-trace |

**Regel:** je mag pas een oorzaak *noemen* als je een observatie hebt die hem aantoont. "Waarschijnlijk komt het door X" is geen diagnose, het is een gok — en gokken die je vervolgens repareert, produceren precies de teruggedraaide fixes uit §0.

### Stap 3 — Halveer, tel niet af

Twaalf oorzaken serieel uitsluiten = 12 iteraties. Binair zoeken = 4.

- **Werkte het ooit?** → `git bisect`. Dit is het krachtigste en meest onderbenutte gereedschap dat er is. Bij 200 commits vind je de dader in 8 stappen, gegarandeerd, zonder één hypothese te bedenken.
- **Zit het in de data of in de code?** → default asset inladen. Eén test, halve zoekruimte weg.
- **Zit het in de AnimBP of in de mesh?** → AnimBP loskoppelen. Eén test.
- **Zit het in het spelerpad of in het squadpad?** → zelfde bug bij een AI-personage?

Elke test moet **minstens de helft** van de resterende mogelijkheden wegnemen. Een test die maar één mogelijkheid uitsluit, is de verkeerde test.

### Stap 4 — Diagnose vóór reparatie

**Geen enkele fix zonder benoemde oorzaak.** Schrijf hem letterlijk op:

> *"Oorzaak: het wapen tickt in TG_PrePhysics, vóór de skeletal mesh zijn pose evalueert, dus het leest de pose van de vorige frame. Bewijs: gelogde socket-transform loopt exact één frame achter op de bone-transform (meting: 60 frames, 60/60 afwijking)."*

Kun je dat niet in één alinea met een meting erbij, dan weet je de oorzaak niet en is elke fix een gok. **Een gok die de test haalt is erger dan een mislukking** — hij verstopt de bug tot hij later terugkomt op een duurder moment.

Dit is precies waarom de twee trilling-reparaties zijn teruggedraaid: ze kwamen vóór de diagnose.

### Stap 5 — Eén meting die je ongelijk zou geven

Voor je de fix commit: **welke meting zou aantonen dat mijn diagnose fout is?** Draai die. Haalt hij de verwachting niet, dan klopt de diagnose niet — hoe mooi de fix ook oogt.

Dit is dezelfde falsificatiediscipline die `EXECUTION_PLAN.md` §3 op risico's toepast. Hij hoort net zo goed op bugs.

### Stap 6 — Time-box en escaleer

**Drie iteraties, of 45 minuten zonder diagnose: STOP.** Niet doorgaan, niet nog een hypothese.

Schrijf op:
- wat het symptoom exact is, met een meting
- wat je hebt uitgesloten, en met welke observatie (niet: met welke redenering)
- wat de drie meest waarschijnlijke resterende oorzaken zijn
- welke observatie of owner-actie de volgende stap zou deblokkeren

en **ga aan iets anders werken.** Een geblokkeerde bug die netjes gedocumenteerd op de rij staat, kost het project niets. Een agent die zes uur in een lus zit, kost het project zes uur.

---

## 2. Anti-patronen — met de voorbeelden uit dit project

| Anti-patroon | Hoe het er hier uitzag | Wat het had moeten zijn |
|---|---|---|
| **Hypothesen tellen** | "Twaalf oorzaken uitgesloten" | Bisectie: 12 → 4 stappen |
| **Code lezen i.p.v. kijken** | Conclusies over het inslagspoor zonder in de editor te kijken | `ShowDebug`, RenderDoc, Rewind Debugger |
| **Repareren vóór diagnose** | Twee trilling-fixes, allebei teruggedraaid | Eerst de oorzaak benoemen mét meting |
| **Conclusies publiceren en terugnemen** | Drie ingetrokken conclusies over het inslagspoor | Een conclusie is een *meting*, geen redenering. Geen meting = geen conclusie. |
| **Debuggen in de volledige game** | Beide dossiers | Minimale reproductie in een lege map |
| **Niet zoeken of het bekend is** | Beide dossiers zijn bekende UE-problemen | Stap 0, tien minuten |
| **Geen time-box** | Dossiers die sessies overleven | Drie iteraties, dan escaleren |

---

## 3. De harde regels

1. **Stap 0 altijd.** Tien minuten zoeken vóór elke bug. Geen uitzonderingen.
2. **Geen fix zonder benoemde oorzaak plus meting.**
3. **Geen conclusie zonder observatie.** Een redenering is een hypothese, ook als hij overtuigend is.
4. **Elke test halveert.** Kan een test dat niet, bedenk een betere test.
5. **Drie iteraties, dan escaleren.** Met een geschreven stand, niet met "ik ben er nog mee bezig".
6. **Minimale reproductie vóór de derde iteratie.** Zit je bij poging drie nog in de volledige game, dan bouw je nu eerst de testmap.
7. **Elke afgesloten bug levert één regel op in §4** — zodat dezelfde bug nooit twee keer wordt onderzocht.

---

## 4. Bekende-oorzaken-catalogus

*Groeit met elke opgeloste bug. Raadpleeg dit vóór stap 0 — misschien staat het antwoord er al.*

### 4.1 Wapen zweeft / loopt achter tijdens bewegen — **BEKEND UE-GEDRAG**

**Symptoom:** een aan een socket bevestigd wapen hangt naast of achter de hand tijdens beweging.

**Bekende oorzaak:** gesocketde objecten lopen **0,5–1,5 frame achter** op de animatie. Het wapen leest de pose van de vorige frame omdat het tickt vóór de skeletal mesh zijn nieuwe pose heeft geëvalueerd. Dit is standaard engine-gedrag, geen projectbug.

**Bekende oplossing:** tick-volgorde expliciet vastleggen. Zet een **tick-prerequisite** zodat de wapencomponent pas tickt nádat de character-mesh zijn animatie heeft bijgewerkt, of verplaats hem naar een latere tick-groep (`TG_PostPhysics` / `TG_PostUpdateWork`). Voor de hand-op-wapen-kant is de omgekeerde koppeling de juiste: wapen aan de hand, en de linkerhand via **hand-IK** naar een socket op het wápen (de standaardoplossing voor tweehandige wapens).

**Bronnen:** [socket lagging behind animation — Epic forums](https://forums.unrealengine.com/t/socket-lagging-behind-animation/67625) · [Actor Ticking — UE-documentatie](https://dev.epicgames.com/documentation/en-us/unreal-engine/actor-ticking-in-unreal-engine) · [Left-hand weapon IK](https://zaggoth.wordpress.com/2019/01/26/ue4-tutorial-the-right-way-to-do-left-hand-weapon-ik/)

### 4.2 Trillen bij het schieten — **BEKEND UE-GEDRAG**

**Symptoom:** de schietpose pulseert per schot; gemeten 27 richtingsomkeringen van de hand tegen hoogstens 2 daarbuiten.

**Diagnose:** 27 richtingsomkeringen is geen animatiefout — dat is een **oscillerend blendgewicht**. Dit is het bekende *Layered Blend Per Bone*-jittersymptoom, breed gemeld op de Epic-forums, vaak in combinatie met een aim-offset.

**Bekende oorzaken, in volgorde van waarschijnlijkheid:**
1. **Blendgewicht oscilleert** — twee nodes (bovenlichaamslaag en aim-offset) vechten om dezelfde bones. Kijk met de **Rewind Debugger** naar het gewicht per frame; je ziet het heen en weer springen in plaats van te speculeren.
2. **Animatiecompressie** — geïmporteerde animaties gaan trillen door compressie. Zet de compressie op **Default Anim Bone Compression** en meet opnieuw. Goedkope test, veelvoorkomende oorzaak.
3. **Bone-uitlijning / retargeting** — verschillen tussen skeletons. Gebruik **Show Retargeting Debug** om de trillende bone aan te wijzen.
4. **Blend-in/blend-out per schot te kort** — bij hoge vuursnelheid herstart de montage voor hij is uitgeblend. Een additieve terugslag-take lost dit structureel op (staat al als voorstel in de repo).

**Bronnen:** [Layered Blend Per Bone Jitter — Epic forums](https://forums.unrealengine.com/t/layered-blend-per-bone-jitter-4-26/1999131) · [aiming animation shaking with Layered blend per bone](https://forums.unrealengine.com/t/aiming-animation-is-shaking-with-layered-blend-per-bone/1804108) · [layer blend per bone making animations jittery](https://forums.unrealengine.com/t/animation-bug-layer-blend-per-bone-making-animations-jittery/410566) · [Using Layered Animations — UE-documentatie](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-layered-animations-in-unreal-engine)

### 4.4 "The map specified on the commandline could not be found" — **GEEN GAMEBUG**

**Symptoom (31-07, 19:11):** de game start niet en toont een dialoog:

> The map specified on the commandline `C:/Users/natha/AppData/Local/Programs/Git/Game/Maps/GrayboxDistrict` could not be found. Would you like to load the default map instead?

**Oorzaak: Git Bash (MSYS2) verminkt het argument.** Unreal-mapnamen zijn *virtuele* paden en beginnen met `/Game/…`. MSYS ziet elk argument dat op een Unix-absoluut pad lijkt en vertaalt het naar Windows door de Git-installatiemap ervoor te plakken. Git staat hier in `C:\Users\natha\AppData\Local\Programs\Git`, en dat is exact het voorvoegsel in de melding.

**Bewijs (één commando, geen redenering):**
```bash
$ cmd //c echo /Game/Maps/GrayboxDistrict
C:/Users/natha/AppData/Local/Programs/Git/Game/Maps/GrayboxDistrict
```

**Er is dus niets mis met `SPEEL_ECLIPSE.bat`** — daar staat gewoon `set MAP=/Game/Maps/GrayboxDistrict`. De fout ontstaat alleen wanneer de game via de **Bash-tool** wordt gestart.

**Oplossing, in volgorde van voorkeur:**

1. **Start de game via PowerShell of cmd, niet via Bash.** `SPEEL_ECLIPSE.bat` is een batchbestand; dat hoort in de PowerShell-tool. Dit is de echte fix.
2. Moet het toch via Bash: zet de padvertaling uit voor dat commando —
   `MSYS_NO_PATHCONV=1 ./SPEEL_ECLIPSE.bat` of `MSYS2_ARG_CONV_EXCL='*' …`
3. Of ontsnap het pad met een dubbele slash: `//Game/Maps/GrayboxDistrict`.

**Algemene regel voor dit project:** elk argument dat met `/Game/`, `/Script/`, `/Engine/` of `-ExecCmds=` begint, gaat **nooit** door de Bash-tool. Dat geldt ook voor commandlets en `-EclipseStartMission=…`.

**Waarom dit hier staat:** dit is precies een §1-stap-0-geval. Het is bekend gedrag van het gereedschap, niet van de game, en het kost tien seconden om te herkennen zodra je het één keer hebt opgeschreven.

### 4.3 Inslagspoor rendert niet — **OPEN, maar verkeerd benaderd**

**Stand:** twaalf oorzaken "uitgesloten" door redenering, drie conclusies teruggenomen. Het enige dat ooit verscheen was een kubus **bij het personage**, niets op de inslagplek.

**Wat dat ene feit betekent — en het is het enige harde feit in het dossier:** als het gespawnde object bij het personage verschijnt in plaats van op de inslagplek, dan rendert het systeem prima en is de **transform fout**. Dat is geen rendering-bug, dat is een locatie-bug. Twaalf uitgesloten rendering-oorzaken waren dus twaalf tests in de verkeerde helft van de zoekruimte.

**Volgende stap volgens dit protocol:** log de hit-locatie uit de trace en de uiteindelijke spawn-transform naast elkaar, 20 schoten lang. Wijken ze af, dan zit het in de trace of de transform-berekening (waarschijnlijk lokale versus wereldruimte, of een relatieve attach die niet los is gemaakt). Zijn ze gelijk, dán pas is het een rendering-vraag en gaat RenderDoc open.

*Geen dertiende hypothese. Eén meting.*
