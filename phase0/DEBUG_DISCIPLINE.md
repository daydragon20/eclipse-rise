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

> ## ⚠️ OORZAAK 1 IS GEMETEN — 31-07. De diagnose klopt, het mechanisme níét.
>
> **Wat bevestigd is:** het gewicht oscilleert echt, en 1:1 met het aantal schoten.
> Gemeten pieken bij 10 / 20 / 27 schoten: **10 / 20 / 27**, identiek bij 120 Hz,
> 60 Hz **en 77 Hz**. Dat derde raster is er met opzet: 120 en 60 zijn allebei een
> geheel veelvoud van het vuurinterval van 0,15 s, dus die twee kunnen het samen eens
> zijn over een rasterartefact. 77 Hz valt nergens op een schot en geeft hetzelfde.
>
> **Controleproef vóór de meting**, want een teller die altijd "het aantal schoten"
> zegt meet niets: één doorlopende pose over hetzelfde venster geeft **1 piek**, een
> vlak signaal **0**. De teller kan dus wél iets anders zeggen.
>
> **Wat NIET klopt aan de tekst hieronder — en dat is de les.** De speler draait
> **geen AnimBP**: `EclipseCharacter.cpp` zet `UEclipseAnimInstance` als
> anim-instance-class, een C++-proxy die gewogen poses optelt. Er is dus geen
> *Layered Blend Per Bone*-node en geen aim-offset die om bones vecht, en de
> **Rewind Debugger heeft niets om terug te spoelen**. Die aanbeveling stuurde naar
> gereedschap dat hier niet bestaat.
>
> **Het echte mechanisme:** `PlayOneShot` zet `OneShotTime = 0.0f` — de envelope
> **herstart bij elk schot**, en loopt dus per schot 0 → piek → 0.
>
> **Twee dingen die de voor de hand liggende verklaring uitsluiten.** De geleverde
> config is niet het zaagtandregime: posetijd 0,12 s < vuurinterval 0,15 s, dus elke
> puls loopt áf (104 stille frames tussen de schoten) — er wordt niets afgekapt. En
> dat maakt niet uit: bij snelvuur (interval 0,08 s < posetijd) zijn er 0 stille
> frames en zijn het **exact dezelfde** 27 pieken. Het afkappen maakt de omkeringen
> dus niet; de herstart doet dat, in beide regimes.
>
> **Vondst onderweg:** de formule stond **twee keer** — in de proxy die de speler ziet
> én in de game-thread-spiegel die de testlaag uitleest. Beide roepen nu
> `EclipseLocomotion::OneShotEnvelope()` aan. Zonder die samenvoeging bewees een
> meting aan de ene helft niets over de andere.
>
> **Fixrichting (aparte iteratie, bewust nog niet gedaan):** een doorlopende envelope
> die bij een nieuw schot vanaf het *huidige* gewicht verder loopt in plaats van vanaf
> 0. De falsificatie ligt klaar — dezelfde test moet dan pieken ≪ N geven bij
> ongewijzigde bemonstering. Test: `Tests/EclipseAnimOneShotWeightTests.cpp`.

**Bekende oorzaken, in volgorde van waarschijnlijkheid:**
1. ~~**Blendgewicht oscilleert** — twee nodes (bovenlichaamslaag en aim-offset) vechten om dezelfde bones. Kijk met de **Rewind Debugger** naar het gewicht per frame.~~ **Gemeten en gecorrigeerd, zie het kader hierboven:** het gewicht oscilleert wél, maar er zijn geen vechtende nodes en geen Rewind Debugger — het is de herstart per schot in de C++-proxy.
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

### 4.3 Inslagspoor rendert niet — **OPEN, maar de transform is nu met een meting UITGESLOTEN**

*Stand 31-07. Deze regel vervangt de vorige conclusie ("transform-bug, geen rendering-bug"); die is weerlegd.*

**Wat hier stond, en waarom het weg moest.** De conclusie luidde: het enige zichtbare object stond *bij het personage* in plaats van op de inslagplek, dus de transform is fout en het renderen werkt prima. Dat feit kwam uit een controleproef die het niet kan dragen — het was een magenta blok dat **vastgemaakt was aan het personage** en bij BeginPlay was neergezet. Een object dat per constructie bij het personage staat, zegt niets over waar een *gespawnd* spoor terechtkomt; het kón nergens anders staan. Dezelfde aantekening meldde bovendien dat de echte sporen gemeten op **8,4–8,5 m vóór de camera** stonden, wat de tegenovergestelde kant op wijst. En de logregel die het had kunnen beslissen, logde wel `Spot` (waar het spoor *naartoe* ging) maar nooit `Mark->GetActorLocation()` (waar het *staat*). Dit is anti-patroon 1 uit §2 in zuivere vorm: een conclusie zonder observatie die hem aantoont.

**De meting die nu wel is gedaan.** `Eclipse.Combat.ImpactMarkLandsOnTheHitAndNotOnTheShooter` (`Eclipse/Source/Eclipse/Tests/EclipseImpactMarkTests.cpp`), volledig headless, 20 schoten gevarieerd over afstand (250–3955 cm), hoek (360° yaw, pitch −35…+35), oppervlaknormaal (vloer / muur / schuin) en schutterpositie — die laatste staat nooit op de oorsprong, want daar zijn wereld- en lokale ruimte identiek en is juist de vermoede fout onzichtbaar. Twee eisen per schot **tegelijk**, want elk van de twee alleen laat de andere verklaring in leven.

| Gemeten | Uitkomst |
|---|---|
| spoor ≤ 1 cm van de inslagplek uit de trace | **20/20** — slechtste afwijking 1,000 cm, en dat is exact de bewuste lift van 1 cm langs de normaal; van de *bedoelde* plek 0,000 cm bij alle twintig |
| spoor ≥ 100 cm van de schutter | **20/20** — dichtstbijzijnde 249,4 cm |
| staan ze er ná alle twintig spawns nog steeds | 20/20, slechtste afwijking onveranderd 1,000 cm |
| **controleproef**: een spoor met de hand OP de schutter gezet, langs hetzelfde zoek- en meetpad | 250,0 cm van de inslag, 0,0 cm van de schutter — **beide eisen keuren het af**, dus deze meting kán rood worden |

**En dezelfde meting in het echte spel.** De uitgebreide logregel in `SpawnImpactMark` drukt sinds 31-07 naast `bedoeld` ook de **werkelijke actor-locatie na spawnen**, de **schutterspositie** en `Hit.GetActor()` af. In de opnameronde van 31-07 (11 wereldtreffers, 11 sporen):

- `verschil 0.00 cm` tussen bedoeld en werkelijk — **bij alle elf**;
- `1.00 cm` van het inslagpunt — de lift, bij alle elf;
- **627,8 tot 848,5 cm** van de schutter;
- `[PLAYSHOT 3 SPOREN] 22 levend` — 22 en niet 11, want de game mode zet er via `OnWorldImpact` een tweede naast — geprojecteerd op **scherm (592,497)** in een frame van 1280×720, `inbeeld=1`, **884 cm VÓÓR** de camera.

Dus niet alleen in een synthetische wereld: in de draaiende game staan de sporen op de inslagplek, meters van de schutter, midden in beeld, vóór de camera — en ze zijn niet te zien.

**Weerlegd:** de transform-diagnose. `SpawnImpactMark` zet het spoor in wereldruimte precies waar de `FHitResult` het wil hebben, op elke afstand en elke hoek, headless én in het spel. Geen lokale-ruimte-fout, geen niet-losgemaakte attach, geen vergeten offset.

**Niet bewezen, en dus geen nieuwe aanname:** de testwereld tickt nooit, dus over frame-tijd zegt de headless meting niets. En `inbeeld=1` blijft zeggen wat het altijd zei — niets over wat er vóór het vlak staat.

**Het dossier gaat terug naar de renderkant**, met een kleinere zoekruimte dan in juli: er staan inmiddels **twee onafhankelijke spawners** van hetzelfde spoor in de code — `UEclipseHitscanWeaponComponent::SpawnImpactMark` (M_EclipseToonDecal + masker, rotatie uit de normaal) en `AEclipseGameMode::OnWorldImpact` (M_EclipseToon, geen rotatie) — en ze zijn allebei onzichtbaar. Dat sluit "het ligt aan wie hem neerzet" én "het ligt aan dit ene materiaal" in één klap uit.

**De volgende meting, niet de volgende hypothese.** De zwakke plek staat al in het dossier: *"inbeeld=1 zegt niets over wat er vóór het vlak staat"*, en geen enkele rig-stand kijkt naar onbelemmerd wegdek. De goedkoopste test die dat halveert is een **zichtlijn-trace van de camera naar elk levend spoor** in de `[PLAYSHOT n SPOREN]`-regel: die maakt van "inbeeld" een "vrij zicht ja/nee". Is het zicht vrij en staat er niets, dán is het een render-vraag en gaat RenderDoc open. Is het zicht nooit vrij, dan meet dit harnas de vraag helemaal niet en is een rig-stand die wél naar de grond kijkt de eerste stap. Exact dezelfde vraag staat open voor de 38 grondvlakken van de bouwer, waarvan nooit is vastgesteld dát ze renderen.

*Geen dertiende hypothese. Eén meting — en die is nu gedaan.*

### 4.5 GPU-crash "Device Removed" — **GEEN TDR-TIMEOUT, een PAGE FAULT**

**Symptoom (31-07, 19:20):** `GPU crash detected — Device 0 Removed: DXGI_ERROR_DEVICE_HUNG`,
midden in een automatische opnameronde (`unrealeditor-cmd.exe`, 1280×720, frame 259,
vlak na `HighresScreenshot00002`).

**De eerste diagnose was TDR-timeout. Die is weerlegd door te kijken in plaats van te
redeneren.** Er lag een NVIDIA **Aftermath-dump** naast het log, en die decodeert de
crash letterlijk:

```
Device Info:
    Status       : PageFault
    Adapter Reset: False
    Engine Reset : True
Page Fault Info:
    GPU VA  : 0x00007fff00000000
    Type    : AddressTranslationError
    Access  : Read
    Engine  : Graphics
    Client  : GraphicsProcessingCluster
Active Shaders: 2 total, beide Type = Compute
```

**Een page fault is geen timeout.** De GPU wachtte niet te lang — hij **las een adres dat
er niet was**. Dat zijn twee verschillende defecten met twee verschillende oplossingen:
een timeout los je op door de renderbelasting te verlagen, een page fault door de
ongeldige toegang te vinden.

**Drie metingen die de timeout-lezing uitsluiten:**

1. **Nul** timeout- of hang-duurmeldingen in het hele log (`grep -c` op "timed out",
   "GPU hang", "took too long" → 0).
2. De regel `TDR settings OK - Level: Recover, Delay: 2` die de eerste diagnose aanhaalde,
   staat op **regel 958, bij het opstarten**. Dat is de engine die zijn *instellingen*
   rapporteert, geen gebeurtenis. Een instelling aflezen is geen meting van wat er gebeurde.
3. `Adapter Reset: False` — bij een driver-reset door tijdsoverschrijding verwacht je juist
   wél een adapter-reset.

**Twee onderdelen van de eerste diagnose bestaan in dit project niet:**

| Claim | Gemeten |
|---|---|
| "het project zet Nanite-cvars aan" | **geen enkele** `r.Nanite`-instelling in `Config/`, `Source/`, de bats of de tools |
| "`r.TSR.History.ScreenPercentage:200`" | **nergens gezet** — niet in config, niet in `SPEEL_ECLIPSE.bat` (die geeft alleen `-ExecCmds="Eclipse.Guide.Overlay 1"`), niet in code |

Die naar 100 zetten in een falsificatietest doet dus **niets**, en als de run dan niet
crasht zou TSR ten onrechte de eer krijgen.

**Wat er wél aan staat** (`Eclipse/Config/DefaultEngine.ini`): Lumen GI
(`r.DynamicGlobalIlluminationMethod=1`), Lumen-reflecties (`r.ReflectionMethod=1`),
Virtual Shadow Maps (`r.Shadow.Virtual.Enable=1`), mesh distance fields — op **Feature
Level SM5**, waar Lumen terugvalt op software-raytracing. Uit het log bevestigd: GTX 1080 Ti,
D3D12, draait SM5, kaart kan Feature Level 12_1. Lumen blijft daarmee de hoofdverdachte,
maar als **bron van de foute lezing**, niet als traagheid.

#### En het belangrijkste, voor wie hier verder gaat: de crash is GRILLIG

**Gemeten over de logs van 31-07: 1 crash op 9 opnamerondes.** De crashende run heeft 28
shot-regels tegen 220 in de geslaagde — hij stierf vroeg.

**Daarmee is de voorgestelde falsificatietest ongeldig, en dat is een les die breder geldt.**
Eén run "met Lumen uit" die niet crasht bewijst niets: bij een crashkans van ~11% gaat een
willekeurige run in 89% van de gevallen vanzelf goed. Om met redelijke zekerheid te zeggen
dat een wijziging de crash wegneemt, heb je grofweg **26 achtereenvolgende schone runs**
nodig (0,89²⁶ ≈ 0,05). Een test die niet kan onderscheiden tussen "gerepareerd" en "geluk
gehad", is geen test.

#### DE PASS IS GEVONDEN — en het is niet Lumen

**De GPU-breadcrumbs stonden al in hetzelfde log**, twintig regels onder de crashmelding.
Ze noemen precies wat er liep toen de kaart viel:

```
Frame 1257 / SceneRender / RenderGraphExecute / Scene
    HZB                          [ Finished ]
    ComputeLightGrid             [ Finished ]
    LightFunctionAtlasGeneration [ Finished ]
    SkyAtmosphereLUTs            [ ACTIEF ]
        SkyAtmosphere::DistantSkyLightLut  [ ACTIEF ]
        SkyAtmosphere::SkyViewLut          [ ACTIEF ]
        SkyAtmosphere::CameraVolumeLut     [ ACTIEF ]
    BasePass                     [ ACTIEF ]
        ParallelDraw (0 van 3)   [ ACTIEF ]
        ParallelDraw (1 van 3)   [ Niet gestart ]
    ... alles hierna               [ Niet gestart ]
```

**Dat wijst één ding aan.** De SkyAtmosphere-LUT's zijn **compute**-passes, en de
Aftermath-dump meldde exact **twee actieve compute-shaders**. `BasePass/ParallelDraw` is
graphics, geen compute. De enige actieve compute in de hele boom is SkyAtmosphere.

**En het weerlegt de Lumen-hypothese — inclusief mijn eigen.** Kijk naar wat er *niet*
gelopen heeft:

| Verdachte | Breadcrumb-status |
|---|---|
| Lumen GI / reflecties | `DiffuseIndirectAndAO`, `RenderDeferredLighting` — **Niet gestart** |
| TSR | `TemporalSuperResolution` — **Niet gestart** |
| Screen-space reflecties | `ScreenSpaceReflections(Quality=2)` — **Niet gestart** |
| Nanite | bestaat niet in dit project |

De GPU is die frame **nooit bij Lumen aangekomen**. Lumen uitzetten had de crash dus niet
weggenomen — en was de proefrun toevallig schoon geweest (89% kans, zie hierboven), dan had
Lumen ten onrechte de schuld gekregen én de eer van de "fix". Dat is precies de val die
deze twee metingen samen dichttimmeren.

**Stand van de diagnose:** een page fault bij het lezen, in een SkyAtmosphere-LUT
compute-pass, gelijktijdig met de eerste BasePass-ParallelDraw. Volgende stap is dáár
halveren — `r.SkyAtmosphere 0` als eerste falsificatie, met de kanttekening dat de
grilligheid (1 op 9) ook die test veel runs kost, tenzij er eerst een deterministische
reproductie komt.

**Waarom dit hier staat:** dit is §2 en §5 in één geval. De diagnose klonk sluitend, noemde
een echt getal (Delay: 2) en wees een echte zwaarte aan (Lumen op SM5) — en was toch fout,
omdat het getal een *instelling* was en niet een *gebeurtenis*. Het gereedschap dat het
antwoord wél had, had de hele tijd naast het log gelegen.
