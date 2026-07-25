# Feel-referentie — third-person shooter

*Werkdocument. Opgesteld 2026-07-25 op owner-verzoek: een uitputtende taxonomie van
alles wat de bewegings- en bedieningsfeel van een TPS bepaalt, zodat de agent zelf
systematisch kan auditen in plaats van dat de owner defect na defect meldt.*

**Hoofdreferentie: Borderlands.** Daarnaast Gears of War, The Division, Mass Effect,
Warframe. ECLIPSE = third-person action-strategy, UE 5.8, gestileerd, squad-commando's
op commando-afstand.

---

## 0. Hoe dit document werkt

Elke regel heeft een **ID** (`LOC-01`, `CAM-07`, …). Een audit rapporteert per ID:
groen / afwijkend / niet gebouwd. De ID's zijn stabiel; voeg toe aan het eind van een
blok, hernummer nooit.

**Auditvolgorde die het meeste oplevert:**

1. **Bijlage D** — de concrete voorstellenlijst. Elke rij is een enkel veld met een huidige
   en een voorgestelde waarde. Begin hier; dit is het snelst afvinkbaar.
2. **Bijlage C** — de stand per gebied, zodat je weet welke gebieden überhaupt bestaan.
   Vier van de tien gebieden zijn nu grotendeels leeg; daar heeft "afwijkend" geen betekenis.
3. **Bijlage C2** — de camera-checklist naar Nesky. Puur ja/nee, geen getallen nodig.
4. **De ⚠-rijen** — dat zijn conflicten tussen code, GDD en spec. Die horen niet door een
   audit opgelost te worden maar door change management.
5. Pas daarna de volledige tabellen per gebied.

De drie gebieden met het grootste gat tussen "gedocumenteerde conventie" en "gebouwd" zijn
**§8 FEEDBACK** (bestaat volledig niet), **§9 ANIMATIE** (geen richting, aim-offset, lean,
foot IK of turn-in-place) en **§10 TRAVERSAL** (alles engine-default).

Elke regel draagt een bronlabel:

| Label | Betekenis |
|---|---|
| **[OFFICIEEL]** | Uitspraak van de maker, of een verscheepte configwaarde uit het spel zelf |
| **[ENGINE]** | UE 5.8-broncode, lokaal geverifieerd in `C:\Program Files\Epic Games\UE_5.8\Engine\Source` — hard, en meteen implementeerbaar |
| **[GEMETEN]** | Reverse-engineering door derden; reproduceerbaar, niet bevestigd door de maker |
| **[REDENERING]** | Geen bron gevonden. Afgeleid uit engine-gedrag, uit de GDD, of uit vakconventie. **Wantrouw deze het eerst.** |

### De overdraagbaarheidsregel

Dit is de les uit de vorige onderzoeksronde (zie `CONTROLLER_FEEL.md`), en hij is hier
bindend:

> **Absolute getallen uit een spel met een eigen interne schaal zijn niet overdraagbaar.**
> Fortnite-percentages, Apex' 0–500-schuiven, "sensitivity 7", "aim assist 0.6" — die
> getallen betekenen niets buiten hun eigen build.

Wat wél overdraagt: **verhoudingen** (sprint/loop-ratio, ADS-multiplier, strafe-penalty),
**tijden** (seconden, milliseconden), **hoeksnelheden** (graden/seconde), **fysieke maten**
(meters, m/s) en **Unreal Units** (1 uu = 1 cm) uit een ander UE-project.

Waar hieronder een getal staat dat niet overdraagt, staat dat er expliciet bij.

### De hiërarchieregel

De Game Design Bible wint (`14_ai_dev_instructions.md` §14.1). Dit document is een
*referentie*, geen mandaat. Waar een aanbeveling hier afwijkt van `04_core_gameplay.md`
of van `graybox_feel_targets.md` (status: **LOCKED**), staat dat als **⚠ conflict**
gemarkeerd — dat gaat via change management, niet stilzwijgend.

Vaste ankers waar alles hieronder aan hangt:

| Anker | Waarde | Herkomst |
|---|---|---|
| Loop / ren / sprint | 1.8 / 4.2 / 6.5 m/s = **180 / 420 / 650 cm/s** | GDD 4.1.1, `graybox_feel_targets.md` (LOCKED) |
| Camera | over-shoulder, wisselbare schouder, ~15% pullback in commando-modus, push-in bij mikken | GDD 4.1.1 |
| Input-naar-beweging | **≤ 100 ms** voor alle verbs | `graybox_feel_targets.md` (LOCKED) |
| Eenheden in code | centimeters / uu | hele `Eclipse`-module |
| Zwaartekracht | −980 cm/s² | [ENGINE] `BaseEngine.ini:3407` `DefaultGravityZ=-980.0` |

---

## 0b. Wat de referentietitels werkelijk zijn — lees dit eerst

Drie bevindingen uit de bronstudie die de rest van dit document sturen. Ze zijn
belangrijk genoeg om ze niet in een tabelcel te verstoppen.

### 1. Gears' "gewicht" zit in de camera en de animatie, niet in de beweging

> *"The roadie run is, in fact, only **1.2 times faster** than regular movement… the
> perception of speed comes from the camera angle and the wobble effect."*
> — Cliff Bleszinski, GDC 2007 **[OFFICIEEL]**

Roadie run is **1.2×**. Niet 1.6×, niet 2×. De hele beleving van snelheid wordt gekocht
met **vijf kanalen tegelijk**: camera zakt naar de grond, camera schudt en bobt "alsof een
cameraman achter je aan rent", motion blur aan de schermranden, ademhaling en hartslag
worden luider (en doven ná de sprint pas uit), en je **kunt niet vuren**.
[GEMETEN] Gears-wiki, Giant Bomb, gamedeveloper.com

En de tegenhanger, van The Coalition zelf (GDC 2018) **[OFFICIEEL]**:

> *"Gears controls are very responsive (twitchy). Filter inputs to locomotion blend spaces.
> If filtered values are too far from actual values → snap to actual values, inertialize.
> Fluid pose even with twitchy inputs."*

**Dit is de bouwtekening van "zwaar maar responsief".** De besturingslaag is ongefilterd en
direct. Alleen de **animatie-blendspace-inputs** worden laagdoorlaat-gefilterd; loopt het
filter te ver van de waarheid, dan snapt het terug en verbergt inertialisatie de sprong.
Resultaat: een zwaar ogend personage met **nul extra inputlatentie**.

> **Consequentie voor ECLIPSE:** gewicht mag nooit in `MaxAcceleration` of
> `BrakingDeceleration` gezocht worden zodra het de *besturing* traag maakt. Zet de
> beweging responsief en koop het gewicht terug in camera, animatiefiltering en audio.
> Zie LOC-18, CAM-11, ANI-14 (inertialisatie) en ANI-15 (het filterrecept zelf).

### 2. Borderlands is géén third-person camera-referentie

Borderlands is in elke mainline-titel **first-person** voor de speler. Third person bestaat
alleen als debug-console (`camera 3rd`) en voor voertuigen. [GEMETEN] Borderlands-wiki

**Borderlands levert dus: art direction, wapenfeel (het spread/bloom-model, §4), en de
feedback-juice (§8). Niets voor §2 en §3.** Dat onderscheid is bewust hier vastgelegd
zodat een audit niet naar een niet-bestaande Borderlands-TPS-camera gaat zoeken.

Wat Borderlands wél levert, en het is goud, zijn **verscheepte UE-waarden in uu**
(1 uu = 1 cm, dus direct overdraagbaar):

| | BL2 (UE3) | BL3 / Wonderlands (UE4) | Ratio t.o.v. lopen |
|---|---|---|---|
| Loopsnelheid | `GroundSpeed` **440** | `MaxWalkSpeed` **470** | 1.000 |
| Sprint | via attribuut-modifier, waarde niet leesbaar | `MaxSprintSpeed` **720** | **1.532** |
| Gehurkt | `CrouchedPct` 0.5 → **220** | `MaxWalkSpeedCrouched` **275** | 0.50 → **0.585** |
| Luchtsnelheidscap | `AirSpeed` **500** | — | 1.136 (hoger dan grond!) |
| Sprongkracht | `JumpZ` **630** | — | — |
| Luchtcontrole | `PlayerAirControl` **0.11** | — | — |
| Neergehaald (FFYL) | **150** | **200** | 0.34 / 0.43 |

[GEMETEN] Datamined uit de spelobjecten zelf, via de BLCMM/BL3-mod-community
(`BL2 Movement Speed Cheats.blcm`, `movement_speed_cheats_normal.bl3hotfix`).

⚠ **BL2's sprint-multiplier is niet af te leiden.** De enige zichtbare waarde is
`BaseValueScaleConstant = 1` op `SprintDefinition_Default`, en dat is een schaal op een
onbekende bonus — **geen ratio**. Niet overnemen.

### 3. De twee polen van richtingsstraf

| Titel | Strafe | Achteruit | Bron |
|---|---|---|---|
| **Gears 5** (na TU3, dec 2019) | **1.00** (lateraal = voorwaarts) | **0.85** | [OFFICIEEL] patch notes |
| **Borderlands** (alle delen) | vermoedelijk **1.00** | vermoedelijk **1.00** | [REDENERING] — geen richtingseigenschap in de gedatamineerde defaults |

Gears kwam op 0.85 uit **nadat 0.739 te sloom bleek** — dat is een uitgespeelde,
competitief getunede waarde. En de rest van diezelfde patch is leerzaam: de basis-**versnelling
ging +50% omhoog terwijl de topsnelheid onaangeroerd bleef**, met als expliciet doel
*"improve responsiveness, reduce some of the delayed [feel]"*. **Versnelling is de
responsiviteitsknop, topsnelheid niet.**

### 4. Wat elke referentie feitelijk oplevert

Niet elke titel is overal bruikbaar voor. Dit voorkomt dat een audit in de verkeerde bron gaat graven.

| Titel | Levert wél | Levert niet |
|---|---|---|
| **Borderlands** (UE3/UE4) | verscheepte bewegings-uu, het spread/bloom-model in graden, het hit-feedbackmodel, crit-formule, art direction | **niets over TPS-camera** — het spel is first-person; ook geen acceleratie-, rem- of ADS-waarden |
| **Gears of War** (UE3/UE4/UE5) | de 1.2×-sprintratio, de sprint-camerastack, richtingsratio's, inertialisatie + fasematching + motion warping (drie gepubliceerde papers), aim-assist-terminologie, annuleervensters op dekking | **geen enkele boomlengte, schouderoffset, camera-lag of pitch-limiet** — nooit gepubliceerd |
| **Mass Effect** (UE3) | de enige gepubliceerde FOV-ladder in graden (70/80/90/84), de ADS-zoomladder met gevoeligheids- en assist-multipliers, de drie aim-assist-schakelaars | camerapositie-offsets (zitten in binaire `.pcc`), snelheden, dodge-timing |
| **Warframe** (Evolution) | het beste toegankelijkheidsverhaal dat er is (shake, recoil-types, deadzone-taxonomie), de FOV-invariantieregel, "shake is lokaal", per-intentie aim-assist-gating | bewegingsgetallen (parkour-schaal, niet overdraagbaar), camera-offsets in enige eenheid |
| **The Division** (Snowdrop) | de "additional FOV"-delta-aanpak, Motion Sickness Mode, de drieledige gevoeligheidssplitsing | ⚠ vrijwel alles. Ubisoft publiceert geen units, geen bereiken, geen defaults; er is geen Snowdrop-talk over beweging of camera gevonden. **De aanname dat Division 2 uitgebreide aim-opties verscheept is niet bevestigd** |

Eén structurele convergentie die opvalt: **drie gevoeligheidstrappen** (heup / ADS / scoped) is
het antwoord waar Division 2, Warframe én Mass Effect onafhankelijk op uitkomen. ECLIPSE heeft
er nu twee.

---

## 1. LOCOMOTIE

Kernpunt: in UE is grondversnelling **lineair** en zonder wrijvingsverlies. In
`CalcVelocity` wordt bij het versnellen alleen de *richting* van de snelheid naar de
inputrichting gedraaid (met `GroundFriction` als draaisnelheid); de *grootte* groeit
puur met `MaxAcceleration` tot `MaxWalkSpeed`. Daarom is de tijd-naar-topsnelheid exact
te berekenen, en is `GroundFriction` géén rem maar een **richtingswisselsnelheid**.

Bron voor die twee zinnen: [ENGINE] `CharacterMovementComponent.cpp`,
`UCharacterMovementComponent::CalcVelocity`.

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| LOC-01 | Tijd naar topsnelheid | `t = MaxWalkSpeed / MaxAcceleration`. UE-default 600/2048 = **293 ms**; TP-template 500/2048 = **244 ms**. Een "zware" shooter zit hoger, een arcade-shooter lager. | [ENGINE] CMC-ctor + `CalcVelocity` | **~230–300 ms naar ren-snelheid.** Met `MaxAcceleration = 1800`: 420/1800 = **233 ms**, sprint 650/1800 = **361 ms**. Huidige 1400 geeft 300/464 ms — sprint voelt daardoor traag op gang |
| LOC-02 | Acceleratie is lineair, niet exponentieel | UE kent geen ease-in op grondversnelling; de snelheidscurve is een rechte lijn tot de clamp | [ENGINE] `CalcVelocity` | Accepteren. Wie een ease-in wil, doet dat in de *animatie* (start-poses, distance matching), niet in de movementcomponent — anders verlies je de voorspelbaarheid |
| LOC-03 | Afremmen / uitglijden | `dv/dt = −(BrakingFriction × BrakingFrictionFactor)·v − BrakingDeceleration`. UE-default stopt in **~100 ms / ~21 cm** vanaf 600 — dat is *heel* abrupt, dichter bij een arena-shooter dan bij een TPS met gewicht | [ENGINE] `ApplyVelocityBraking` + gesimuleerd, zie Bijlage B | **Zet `bUseSeparateBrakingFriction = true`, `BrakingFriction = 4.0`, `BrakingFrictionFactor = 1.0`, `BrakingDecelerationWalking = 2000`.** Geeft sprint **200 ms / 57 cm**, ren **150 ms / 28 cm**. Dat is "zwaar maar responsief" zonder in Division-traagheid te vallen |
| LOC-04 | `BrakingFrictionFactor` = 2.0 is een historisch artefact | Epic's eigen comment in de broncode: *"Historical value, 1 would be more appropriate."* De factor vermenigvuldigt óók `BrakingFriction`, dus wie hem op 2 laat staan remt dubbel zo hard als het veld suggereert | [ENGINE] CMC-ctor, letterlijke comment | Op **1.0** zetten en de rem via `BrakingFriction` sturen. Anders is elke tuning van `BrakingFriction` misleidend |
| LOC-05 | Stopdrempel | Onder `BRAKE_TO_STOP_VELOCITY = 10 cm/s` klapt de snelheid naar nul zodra er geremd wordt | [ENGINE] `CharacterMovementComponent.cpp:100` | Accepteren. 10 cm/s is onzichtbaar; het voorkomt een eindeloos uitdovende staart |
| LOC-06 | Sprint-ratio | De band is **veel breder dan verwacht en de ondergrens is verrassend laag**. Gears' roadie run is **1.2×** [OFFICIEEL, CliffyB GDC 2007]; BL3/Wonderlands is **1.532×** (720/470) [GEMETEN, datamined]. ECLIPSE: 650/420 = **1.55×**, dus iets boven de Borderlands-pool | [OFFICIEEL] + [GEMETEN], zie §0b | **1.55× behouden** — hij ligt binnen de band en volgt uit de LOCKED GDD-ankers. Maar weet dat Gears met 1.2× méér snelheidsbeleving haalt dan wij met 1.55×, omdat zij vijf camerakanalen inzetten en wij nul. Zie CAM-11 |
| LOC-06b | Loopsnelheid absoluut | BL2 **440 uu/s**, BL3 **470 uu/s**. ECLIPSE's 420 zit net onder beide | [GEMETEN] datamined uit de spelobjecten | 420 is verdedigbaar en volgt uit de GDD. Geen wijziging nodig — dit is puur ter kalibratie dat we in het juiste getallenbereik zitten |
| LOC-07 | Wisselen tussen trappen | Twee scholen: (a) discrete toestandswissel met een snelheidsramp, (b) continue analoge schaal. UE ondersteunt (b) gratis: `AnalogInputModifier = Acceleration.Size() / MaxAcceleration` schaalt `MaxWalkSpeed` lineair, met `MinAnalogWalkSpeed` als bodem | [ENGINE] `ComputeAnalogInputModifier`, `CalcVelocity` | Gamepad: **analoge schaal loop↔ren** (gratis, en het is wat spelers verwachten). Toetsenbord: **expliciete loop-toets nodig** — een digitale toets geeft altijd magnitude 1.0, dus op KB bestaat "lopen" nu niet |
| LOC-08 | ⚠ Sprint als snelheidsoverschrijving | Sprint wordt in ECLIPSE gezet door `MaxWalkSpeed` te overschrijven. De pawn springt dan direct naar de nieuwe clamp zodra hij eroverheen kan accelereren — maar hij *valt ook direct terug* bij loslaten, want zodra `v > MaxSpeed` treedt braking in | [ENGINE] `CalcVelocity`, tak `bVelocityOverMax` | Werkt, maar weet dat de **sprint-uitstap remt met de volle brakingcurve**, niet met een zachte rol-uit. Wil je een uitloop, verlaag dan tijdelijk `BrakingDeceleration` gedurende ~0.2 s na loslaten |
| LOC-09 | Sprint-instap (wind-up) | Gears kent **geen** snelheids-wind-up maar wél een harde **rotatiesnap**: *"Snap character rotation when switching to sprint"* (Gears 3), en Gears 4 hield diezelfde snap en verborg hem met inertialisatie | [OFFICIEEL] The Coalition, GDC 2018 | Geen aparte snelheidsramp bouwen — ECLIPSE krijgt hem gratis uit LOC-01 (bij `MaxAcceleration = 1800` duurt 420 → 650 nog **128 ms**). Wel de **rotatiesnap** overnemen: bij sprintstart draait het lichaam direct naar de bewegingsrichting, en de animatie vangt dat op (ANI-14) |
| LOC-10 | Sprint-uitstap naar vuren | **De wrijving zit op het verlaten van sprint, niet op het ingaan.** Gears heeft een expliciete *"roadie run to shoot delay"*, en verkortte die in TU3 met **25%** in plaats van hem te schrappen. Vuren tijdens roadie run is onmogelijk | [OFFICIEEL] Gears 5 TU3; [GEMETEN] Gears-wiki | **0.25 s** wapen-opbrengtijd, en de trekker moet *gebufferd* worden (zie INP-09) zodat de speler niet twee keer hoeft te drukken. Behoud de vertraging als ontwerpkeuze — hij maakt de sprintbeslissing betekenisvol — maar houd hem kort |
| LOC-11 | Strafe-penalty | **Herzien op bronbewijs.** Gears 5 groepeert lateraal mét voorwaarts: strafe = **1.00**. Borderlands heeft geen richtingseigenschap in zijn defaults, dus vermoedelijk ook 1.00 | [OFFICIEEL] Gears 5 TU3 ("85% of forward / lateral speed"); [REDENERING] voor Borderlands | **Strafe = 1.00.** Mijn eerdere voorstel van 0.75 was niet onderbouwd en is in strijd met de enige titel die er een cijfer over publiceert. Gears investeerde in TU3 juist in *béter* strafen ("increase the opportunities for skilled strafing") |
| LOC-12 | Achteruitlopen | De enige gepubliceerde waarde in het hele veld: Gears 5 zette achteruit op **0.85× voorwaarts**, nadat **0.739 te sloom bleek** in de praktijk | [OFFICIEEL] Gears 5 TU3-patchnotes | **Achteruit = 0.85.** Dit is een uitgespeelde competitieve waarde uit precies het genre dat wij maken. Mijn eerdere 0.55 was fors te streng. Implementatie: `GetMaxSpeed()` overriden op de hoek tussen `Velocity` en `ControlRotation` — UE biedt dit **niet** ingebouwd |
| LOC-13 | Richtingwissel / momentum | `GroundFriction` is de draaisnelheid van de snelheidsvector naar de inputrichting: `V -= (V − AccelDir × V.Size()) × min(dt × Friction, 1)`. Tijdconstante = `1/GroundFriction` | [ENGINE] `CalcVelocity` | Bij UE-default `GroundFriction = 8` duurt een **90°-richtingwissel op sprint ~230 ms** (gesimuleerd). Dat is een goede TPS-waarde: merkbaar momentum, geen ijs. **Laat 8.0 staan** — 4 voelt als glijden (400 ms), 16 als schaatsen-op-klittenband (117 ms) |
| LOC-14 | `GroundFriction` wordt nu nooit gezet | Als je hem niet zet, krijg je 8.0. Dat is toevallig goed, maar het is niet *gekozen* | [ENGINE] CMC-ctor | Expliciet in het tuning-asset zetten op **8.0**, met de comment dat het de richtingswisseltijd is en niet de rem |
| LOC-15 | Analoge bodemsnelheid | `MinAnalogWalkSpeed` voorkomt dat een minimaal stickduwtje een onbruikbaar trage sluippas geeft | [ENGINE] default **0**; TP-template zet **20** | **20 cm/s** (= template). Onder die waarde is de animatieblend niet leesbaar |
| LOC-16 | Loopsnelheid bij schuine helling | UE behoudt standaard de horizontale snelheid op hellingen (`bMaintainHorizontalGroundVelocity = true`) — je loopt een helling op zonder snelheidsverlies | [ENGINE] CMC-ctor | Accepteren voor graybox. Wil je later gewicht op hellingen, dan is dít de vlag (op `false` volgt de snelheid het oppervlak en verlies je tempo bergop) |
| LOC-17 | Netwerk-smoothing zit in de weg bij feel-metingen | Gesimuleerde proxies worden 100 ms positie- / 50 ms rotatie-gesmoothd | [ENGINE] CMC-ctor `NetworkSimulatedSmoothLocationTime` | Feel altijd meten op de **lokaal bestuurde pawn**, nooit op een proxy. Anders meet je de smoothing |
| LOC-18 | **Versnelling is de responsiviteitsknop, topsnelheid niet** | Gears 5 TU3 verhoogde de basisversnelling met **+50%** en liet de topsnelheid ongemoeid. Hun eigen formulering: *"The top speed has been unaffected, only now the character will start moving and reach their top speed sooner than before"* | [OFFICIEEL] Gears 5 TU3-patchnotes | Als ECLIPSE ooit "sloom" voelt, is dit de eerste knop: `MaxAcceleration` omhoog, snelheden ongemoeid. Dat is precies waarom LOC-01 1400 → 1800 voorstelt en de 420/650 uit de GDD onaangeroerd laat |
| LOC-19 | Boven welke snelheid het uit elkaar valt | De BL2-modauteur meldt dat ~2.3× de standaard grondsnelheid *"jerky multiplayer visuals"* geeft — daar houden netcode en animatie het niet meer bij | [GEMETEN] modauteur-notitie | Praktisch plafond: houd de hoogste verplaatsingssnelheid (inclusief slide en dash) onder ~2× de rensnelheid, dus **onder ~840 cm/s** |

---

## 2. ROTATIE — lichaam versus camera

Dit is het gebied waar TPS-en het vaakst kapot gaan, en waar ECLIPSE nu een gat heeft.

UE geeft drie standen, en je hebt ze **alle drie** nodig op verschillende momenten:

| Stand | Vlaggen | Gedrag |
|---|---|---|
| **Vrij** (exploratie, hipfire) | `bOrientRotationToMovement = true`, `bUseControllerRotationYaw = false` | Lichaam draait naar de *loopsrichting*. Camera draait vrij eromheen. Stilstaand draait het lichaam **niet** mee met de camera |
| **Gericht** (ADS, vuren) | `bOrientRotationToMovement = false`, `bUseControllerRotationYaw = true` | Lichaam volgt de camera hard. Strafen/achteruit worden nu zichtbaar en vereisen een 2D-locomotieblend |
| **Gedempt gericht** | `bOrientRotationToMovement = false`, `bUseControllerDesiredRotation = true` | Lichaam volgt de camera, maar met `RotationRate` als demping in plaats van instant |

Bron: [ENGINE] `UCharacterMovementComponent::PhysicsRotation` + `ACharacter`-ctor.

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| ROT-01 | `ACharacter` staat standaard in de *verkeerde* stand voor een TPS | `ACharacter` zet `bUseControllerRotationYaw = true` in zijn ctor. De TP-template zet hem expliciet weer uit en zet `bOrientRotationToMovement = true` | [ENGINE] `Character.cpp` ctor; `TP_ThirdPersonCharacter.cpp` | Beide vlaggen **expliciet** zetten in de ECLIPSE-ctor, nooit op de default vertrouwen |
| ROT-02 | Draaisnelheid lichaam | `RotationRate.Yaw` in graden/seconde. UE-default **360°/s**; TP-template **500°/s**; Epic's eigen shooter-variant **600°/s** — Epic verhoogt hem dus zodra er geschoten wordt | [ENGINE] CMC-ctor, `TP_ThirdPersonCharacter.cpp`, `ShooterCharacter.cpp` | **500°/s vrij, 720°/s in gerichte stand.** Bij 500°/s kost een 180°-ommekeer 360 ms; dat is te traag voor een vuurgevecht. In ADS moet het lichaam de camera vrijwel direct volgen |
| ROT-03 | Alleen de camera draaien, stilstaand | Met `bOrientRotationToMovement` blijft het lichaam **volledig stilstaan** terwijl de camera eromheen zwenkt. Zonder turn-in-place kijk je dus tegen een bevroren rug die opeens 170° verkeerd staat | [ENGINE] `PhysicsRotation` (vroege return als er geen beweging is) | **Turn-in-place bouwen.** Zonder dat is dit een gegarandeerd zichtbaar defect zodra iemand met de rechterstick speelt zonder te lopen |
| ROT-04 | Turn-in-place: drempel | Conventie: het lichaam mag een yaw-afwijking t.o.v. de camera *tolereren* tot een drempel, en draait dan met een animatie bij. Typisch 45–90° | [REDENERING] op vakconventie; [ENGINE] `AnimNode_OrientationWarping` gebruikt `LocomotionAngleDeltaThreshold = 90°` als vergelijkbare grens | **Drempel 70°**, bijdraaien tot ~15° rest. Onder 70° lost de aim-offset het op (zie ANI-05) |
| ROT-05 | Turn-in-place: snelheid | Moet trager dan een loop-draai; anders wordt het een pirouette | [REDENERING] | **180–220°/s**, gedreven door root-motion uit de turn-animatie zodat de voeten niet slippen |
| ROT-06 | Yaw-afwijking moet ergens vandaan komen | De AnimInstance heeft een `YawOffset` nodig (camera-yaw minus lichaam-yaw, genormaliseerd naar −180..180) om zowel de aim-offset als turn-in-place te voeden | [REDENERING] | Toevoegen aan `FEclipseAnimSnapshot`. **Bestaat nu niet** — de anim-instance kent alleen `GroundSpeed`, `IdleWeight/WalkWeight/RunWeight`, `StrideRate`, `bIsInAir`, `bIsDowned` |
| ROT-07 | Pitch hoort nooit in de capsule | `ACharacter` zet `bUseControllerRotationPitch = false` en `Roll = false`; de capsule moet rechtop blijven | [ENGINE] `Character.cpp` ctor | Zo laten. Pitch is puur camera + aim-offset |
| ROT-08 | Overgang vrij ↔ gericht | Het omzetten van de vlaggen is *instant* en zichtbaar als een ruk als het lichaam op dat moment 90° verkeerd staat | [REDENERING] op ROT-01/ROT-02 | Bij het ingaan van ADS: eerst `bUseControllerDesiredRotation` met `RotationRate = 720°/s` gedurende de ADS-blend, dan pas de harde stand. **Deze omschakeling is in ECLIPSE nog niet gebouwd** |
| ROT-09 | Rotatiesprongen mag je *houden* en verbergen | Gears houdt de harde rotatiesnap bij sprintstart in de **gameplay** en verwijdert hem alleen uit de **presentatie**, met inertialisatie. *"Gears 3: snap character rotation when switching to sprint. Gears 4: snap character rotation when switching to sprint / inertialize away the discontinuity."* Het mechaniek werd niet zachter, de weergave wel | [OFFICIEEL] The Coalition, GDC 2018 | **Dit is de goedkoopste manier om ROT-08 op te lossen.** Snap de rotatie hard (voorspelbaar, netwerkbaar, nul latentie) en laat een inertialisatienode de sprong wegwerken. UE heeft een `Inertialization`-node in de AnimGraph — zie ANI-14 |

---

## 3. CAMERA

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| CAM-01 | Booms­lengte | UE `USpringArmComponent` default **300 uu**; TP-template **400 uu**; Epic's combat-variant **100 uu** (dichtbij, melee). TPS-band in de praktijk: 200–450 uu | [ENGINE] `SpringArmComponent.cpp`, `TP_ThirdPersonCharacter.cpp`, `CombatCharacter.cpp` | **300 uu** vrij lopen (huidige waarde is goed), **165 uu** in ADS (= ×0.55, huidige factor). Dit is een uu-waarde uit een ander UE-project en dus wél overdraagbaar |
| CAM-02 | Schouderoffset | UE geeft **geen** default schouderoffset — `SocketOffset` is (0,0,0). De enige gedocumenteerde referentieconfiguratie in het veld is Unity's Cinemachine `ThirdPersonFollow`: **offset (70, 30, −50) cm**, verticale armlengte **50 cm**, cameraafstand **200 cm**. Omgerekend is de laterale offset daar **0.35× de cameraafstand** | [ENGINE] `SpringArmComponent.cpp`; [OFFICIEEL] Cinemachine 3.1-documentatie | ECLIPSE: **Y = +55, Z = +65** op boom 300 → lateraal **0.18×**. Dat is de helft van Cinemachine's verhouding, dus een *centralere* framing dan de referentie. Verdedigbaar, maar weet dat een grotere offset het dradenkruis meer vrij zicht geeft. Als het beeld te "achter de rug" voelt, is dit de knop |
| CAM-02b | Offset breekt de vuurlijn | Met een schouderoffset lopen loop en schermmidden uiteen; schieten in de camera-voorwaartsrichting mist. De standaardoplossing is een **aim-locator** op de spring arm: richt loop → locator in plaats van loop → voorwaarts | [GEMETEN] Unreal-community, breed toegepast | ECLIPSE traceert al vanuit de camera (`AimReachCm = 10000`) en niet vanuit de loop — dat is de juiste kant op. Let er wel op dat de **visuele** tracer vanaf de loop naar het trefferpunt moet lopen, anders zie je kogels schuin wegschieten |
| CAM-03 | Schouderwissel | **Niet zo vanzelfsprekend als het lijkt.** Gears — de genre-definiërende strak-over-de-schouder TPS — heeft **in geen enkele titel ooit een schouderwissel gehad**. De gespiegelde camerapositie zit wél in de build en wordt automatisch getriggerd bij richten vanuit dekking onder bepaalde hoeken, maar er is geen knop voor | [GEMETEN] sterke community-consensus over alle Gears-delen | GDD 4.1.1 eist "wisselbare schouder", dus voor ECLIPSE bouwen we hem — maar het is een **bewuste keuze**, geen default. **0.2 s blend**, dezelfde constante-snelheid-blend als `ViewToggleBlendTime`. Overweeg Gears' truc erbij: automatisch spiegelen bij dekking, zodat de handmatige wissel zelden nodig is. **Nog niet gebouwd** |
| CAM-04 | Camera-lag (positie) | `bEnableCameraLag` staat **standaard uit**; `CameraLagSpeed` default **10**. De lag is `FMath::VInterpTo` — exponentiële smoothing, dus **tijdconstante τ = 1/CameraLagSpeed** en **halveringstijd = ln2 × τ**. Bij 10 is dat τ = 0.10 s, halvering **69 ms**; bij ECLIPSE's 12 is dat τ = 0.083 s, halvering **58 ms**. Substepping aan met max stap 1/60 s | [ENGINE] `SpringArmComponent.cpp` ctor + `VInterpTo` | **12** vrij lopen (huidig, goed — 58 ms halvering is een prettige "lazy follow"), **uitzetten of ≥ 25 in ADS**. **De ADS-uitzondering bestaat nu niet.** Denk in halveringstijden, niet in snelheidsgetallen: dat is de enige manier waarop dit getal overdraagbaar wordt |
| CAM-04b | De nettere demper zit al in 5.8 | `FMath::CriticallyDampedSmoothing(...)` neemt een **`SmoothingTime` in seconden**, gedocumenteerd als *"the time lag when tracking constant motion"*, en is kritisch gedempt (geen overshoot). Geldig zolang `DeltaTime < 0.5 × SmoothingTime` | [ENGINE] `FMath::CriticallyDampedSmoothing` | Bij een herbouw van de camerarig hierop overstappen: een parameter in seconden is voor een designer leesbaar, `CameraLagSpeed = 12` is dat niet |
| CAM-05 | Camera-rotatielag | `CameraRotationLagSpeed` default 10, `bEnableCameraRotationLag` standaard uit | [ENGINE] idem | **Uit laten.** Rotatielag op een speler-bestuurde camera vertaalt direct naar traag mikken; dat is precies de klacht die The Division-achtige rigs opleveren |
| CAM-06 | Botsing met muren | `bDoCollisionTest = true`, `ProbeSize = 12 uu`, `ProbeChannel = ECC_Camera`. De boom trekt hard in bij een treffer — er is **geen** demping op het intrekken en **geen** demping op het weer uitschuiven. Nesky heeft hier zes aparte fouten voor: obstakels die de zichtlijn van opzij breken (#5), de camera wegduwen terwijl de speler hem er juist heen draait (#6), de speler de camera in een obstakel laten duwen (#7), **onafhankelijke krachten die om de camera vechten (#8)**, smalle pilaren (#10), en een heuvel als muur behandelen (#11) | [ENGINE] `SpringArmComponent.cpp`; [OFFICIEEL] Nesky GDC 2014 | Probe **12** is te klein voor een 34–42 uu capsule; de camera schaaft langs hoeken. **Naar 20–25 uu.** Bouw een *asymmetrische* uitschuif: instant in, over ~0.3 s uit. En Nesky's #8 is een architectuurregel: laat één systeem de camerapositie bepalen, niet drie die tegen elkaar in duwen |
| CAM-06b | Het eigen lichaam is ook een occluder | Nesky's mistake **#33: "Letting the avatar's own body occlude targets ahead."** DE loste precies dit op in Warframe met drie camerapresets (**Default / Side Offset / Far Offset**), en de reden was expliciet niet cinematografisch: *"to prevent your fashion from getting in the way"* — het eigen kostuum blokkeerde het zicht | [OFFICIEEL] Nesky GDC 2014; [OFFICIEEL] DE Devstream #195 en Update 43.0 | Relevant voor ECLIPSE, want de speler draagt zware armor-silhouetten. De schouderoffset (CAM-02) is het primaire antwoord; een **Far Offset**-preset als toegankelijkheidsoptie is het secundaire |
| CAM-07 | Pitch-limieten | `APlayerCameraManager` default **−89.9° / +89.9°** — bijna recht omhoog en omlaag, voor een TPS onbruikbaar. **Maar Epic overschrijft dat in zijn eigen template naar `ViewPitchMin = −70.0f`, `ViewPitchMax = 80.0f`** — asymmetrisch, met meer ruimte omhoog dan omlaag | [ENGINE] `PlayerCameraManager.cpp:58-59` én `TP_FirstPersonCameraManager.cpp:9-10`, beide lokaal geverifieerd | ECLIPSE staat op **−70 / +70** en zit daarmee al goed. Overweeg Epic's asymmetrie (**−70 / +75**): omhoog kijken heb je nodig voor verhoogde vijanden en verdiepingen, ver omlaag kijken vrijwel nooit. De limiet hoort op de `PlayerCameraManager` — dat doet ECLIPSE correct — en niet als naklem achteraf |
| CAM-07b | Uitloop naar de pitch-limiet | Nesky's mistake **#47: "Maintaining pitch speed until hitting the pitch limit"** — de camera moet afremmen tegen de klem aan, niet er hard tegenaan slaan | [OFFICIEEL] John Nesky, "50 Game Camera Mistakes", GDC 2014 | Laatste ~10° van het pitch-bereik dempen naar nul. Kost weinig, en het verschil tussen "de camera stopt" en "de camera knalt tegen een muur" is direct voelbaar |
| CAM-08 | FOV | `UCameraComponent::FieldOfView` en `APlayerCameraManager::DefaultFOV` zijn beide **90°**. De referentietitels: Gears of War 1 verscheepte `FOVAngle=90` in `BaseEngine.ini`, **Gears 5 op PC heeft default 80**, Borderlands 2/3 default 90 (BL3-slider 90–110, console-performance-modus 70), **Mass Effect LE1 basis 70°**. En de vuistregel: *"TPS games tend to have a narrower FOV because the camera is further away from the pivot point"* — third person hoort **smaller** dan first person, niet breder | [ENGINE] `CameraComponent.cpp`; [GEMETEN] `BaseEngine.ini` GoW1, Gears 5 settings-rapportage, BL-wiki, mouse-sensitivity.com; [OFFICIEEL] LE1-configwaarden | ⚠ **conflict**: GDD 4.1.1 en `graybox_feel_targets.md` zeggen "FOV 90 default"; de code verscheept **80** third-person. **Gears 5's 80 ondersteunt de code**, niet de GDD — en Borderlands' 90 telt hier niet mee omdat dat een first-person FOV is. Advies: **80 behouden en de GDD-regel corrigeren** via change management |
| CAM-08b | Aparte FOV voor de aim-camera | Gears draait aantoonbaar **twee FOV's**: configwijzigingen veranderen *"world FOV but not aim FOV"* | [GEMETEN] Gears-community, Reloaded-forum | Bevestigt de architectuur die ECLIPSE al heeft (aim vermenigvuldigt de FOV). Zorg dat een toekomstige FOV-schuif **alleen de wereld-FOV** raakt en de ADS-factor apart laat |
| CAM-09 | FOV bij mikken | Versmallen is de conventie: het beeld "duwt in" en de effectieve gevoeligheid daalt mee | [REDENERING]; Epic's shooter-template heeft **geen** ADS, dus geen engine-bron | **×0.80** (huidige factor). Combineer met een look-multiplier van dezelfde orde zodat de hoeksnelheid *op het scherm* gelijk blijft — zie INP-06 |
| CAM-10 | FOV bij sprint | **Mass Effect LE1 verscheept een complete FOV-ladder**, en het is de enige die in graden gepubliceerd is: basis **70°**, sprint buiten gevecht **80°**, sprint in gevecht **90°**, dekking ingaan **83.97°**, voertuig 84°. Dus **+10° buiten gevecht en +20° in gevecht** | [OFFICIEEL] verscheepte LE1-waarden, getranscribeerd door modders | Fors meer dan mijn eerdere voorstel van +6..+8°. **Advies: +10°** op de ECLIPSE-basis van 80. De ME-gedachte dat de kick in gevecht *groter* is dan erbuiten is een goed idee — sprint onder vuur moet paniekeriger voelen |
| CAM-10b | ⚠ FOV-verschuiving is een misselijkheidsbron | DE (Warframe) verlaagde de FOV-verschuiving van Void Sling met 60% *"to address reports of base Field of View inducing nausea"*, en maakte Volt's snelheidsboost-FOV **lokaal** zodat hij andere spelers niet raakt. Nesky's mistake **#42: "Rapidly shifting field-of-view"** | [OFFICIEEL] DE-patchnotes; [OFFICIEEL] Nesky GDC 2014 | Elke FOV-kick moet **gesmoothd** zijn (nooit een sprong) en **uitzetbaar** (FDB-10). En: FOV-effecten van squadmate-abilities mogen de spelercamera nooit raken |
| CAM-10c | Beweeg pitch, afstand en FOV samen | Nesky's mistake **#16: "Shifting pitch, distance, and field-of-view independently"** | [OFFICIEEL] Nesky GDC 2014 | ECLIPSE verandert bij ADS al boom **én** FOV tegelijk, met dezelfde blend — dat is correct. Houd die koppeling vast bij de sprint-camera (CAM-11) en de commando-camera (CAM-13) |
| CAM-11 | **De sprint-camerastack** | Dit is de belangrijkste camerabevinding van de hele studie. Gears koopt met **1.2×** snelheid een compleet snelheidsgevoel, via vijf gelijktijdige kanalen: (1) camera **zakt richting de grond**, (2) camera **schudt/zwaait** "alsof een cameraman achter je aan rent", (3) **motion blur** aan de schermranden, (4) **ademhaling + hartslag** in de audio, die pas ná het einde van de sprint uitdooft, (5) **je kunt niet vuren** | [OFFICIEEL] CliffyB GDC 2007 voor de 1.2× en de camera-verklaring; [GEMETEN] Gears-wiki/Giant Bomb voor de stack | **De stack overnemen, niet de multiplier.** ECLIPSE heeft de snelheid (1.55×) maar nul van de vijf kanalen. Concreet: boom **+40 uu**, `SocketOffset.Z` **−10 uu**, lichte camera-shake, radiale blur, ademhaling-audiolaag met uitdoofstaart van ~1.5 s ná de sprint. Bob en shake **alleen additief op de camera**, nooit op de aim-richting (FDB-07) |
| CAM-11b | Sprint-camera moet uitzetbaar zijn | Gears 5 én Gears of War: Reloaded verschepen een **aparte toggle voor roadie-run camera shake**, zodat je de schud-camera kunt uitzetten met behoud van de sprintanimatie | [OFFICIEEL] Gears 5 accessibility-lijst; [GEMETEN] Reloaded-instellingen | Eén-op-één overnemen: aparte schuif voor sprint-shake, los van de algemene shake-schuif |
| CAM-12 | Screen shake | UE 5.8 heeft `UCameraShakeBase` (Perlin-noise en sinusoscillatie-nodes). **Alle amplitude- en frequentiedefaults zijn 0** — de engine geeft geen enkele richtwaarde | [ENGINE] `LegacyCameraShake.h`, `PerlinNoiseRotationShakeCameraNode.h` | Zie FDB-05..FDB-07. Belangrijk architectuurpunt: shake hoort op de **camera**, niet op de control rotation — anders verschuift het je aim en is het geen feedback maar een handicap |
| CAM-13 | Commando-modus | GDD 4.1.1 + SPEC-P2-02: camera trekt ~15% terug, "lift licht", 30% tijdvertraging | [OFFICIEEL] eigen GDD | ⚠ **conflict**: `DA_CommandModeTuning.CameraPullbackPercent = 15`, maar de character blendt de boom 300 → 520 (**+73%**) plus 120 uu hoogte, en leest die 15% nooit. Eén waarheid kiezen |
| CAM-14 | Nieuwe camerastack in 5.8 | Het `GameplayCameras`-plugin is in 5.8 uit Experimental gepromoveerd naar `Engine/Plugins/Cameras`, met o.a. `BoomArmCameraNode`, `DampenPositionCameraNode`, `DampenRotationCameraNode`, `CollisionPushCameraNode` en een critical-damper-interpolator | [ENGINE] `Engine/Plugins/Cameras/GameplayCameras/` | Niet nu migreren. Wel weten dat de nette oplossing voor CAM-06 (gedempt uitschuiven) daar kant-en-klaar zit als `CollisionPushCameraNode` + `PushInterpolator` |
| CAM-15 | Camerapivot-hoogte | `APawn::BaseEyeHeight = 64 uu`; gehurkt = `CrouchedHalfHeight × EyeHeightRatio` | [ENGINE] `Pawn.cpp:90`, `Character.cpp:533` | De boom hangt nu aan de capsule-root, niet op ooghoogte. Voor een over-shoulder rig hoort de pivot op **~60–70 uu** boven de capsule-basis; de huidige `SocketOffset.Z = 65` doet dat effectief al |

---

## 4. WAPEN

UE 5.8 heeft **geen wapensysteem**. De enige engine-bron is Epic's shooter-variant in de
first-person template, en die is minimaal: geen ADS, geen spreidingsmodel, geen recoil-herstel.

### Het Borderlands-nauwkeurigheidsmodel — overnemen zoals het is

Dit is het waardevolste overdraagbare systeem uit de hele bronstudie, want **het is volledig
in graden uitgedrukt** en dus schaal-onafhankelijk. [GEMETEN] Borderlands-wiki, gedatamineerd.

Twee onafhankelijke grootheden:

- **`spread`** — statische cone per wapen. Uit de kaartwaarde:
  `spread = (100 − kaartnauwkeurigheid) / 12` graden. Elke 12 punten kaartnauwkeurigheid = 1° cone.
- **`accuracyPool`** — dynamische bloom. Start op `accuracyMinimum`, **elk schot telt
  `recoil` op**, geklemd op `accuracyMaximum`. Na het stoppen met vuren dooft hij uit met
  `accuracyRegenRate`, ná een **`accuracyIdleRegenDelay` van 0.2 s die voor élk wapen in het
  spel identiek is**.

De twee regels die alles doen:

| | Kogelcone |
|---|---|
| **Hipfire** | `spread + accuracyPool` |
| **ADS** | **`spread` alleen** — de pool tikt door en stuurt de schermeffecten, maar draagt **nul** bij aan de kogelbaan |

En: **scope-sway, screen shake en muzzle climb zijn evenredig met `accuracyPool` en `recoil`,
maar beïnvloeden de kogelbaan niet.**

> **Dat is de hele truc: de visuele heftigheid van recoil staat los van de ballistische
> straf. ADS schrapt de ballistische straf volledig en houdt de visuele kick.** Precies wat
> een gestileerde shooter nodig heeft — je mag de camera flink laten stuiteren zonder de
> speler te straffen.

De verscheepte tabel (Borderlands 1-wapentypes; de architectuur is in BL2/BL3 identiek,
de getallen niet). Alles in graden, regen-tempo's zijn graden/seconde:

| Wapentype | spread | hip min | hip max | hip regen | ADS min | ADS max | ADS regen | recoil/schot |
|---|---|---|---|---|---|---|---|---|
| Combat Rifle | 1.30 | 1.50 | 12.00 | −8.00 | 1.00 | 8.00 | −6.00 | 3.50 |
| Support MG | 2.40 | 2.00 | 14.00 | −8.00 | 1.00 | 8.00 | −6.00 | 2.20 |
| Machine Pistol | 2.00 | 2.00 | 12.00 | −8.00 | 1.00 | 8.00 | −6.00 | 1.50 |
| Repeater Pistol | 2.00 | 1.50 | 8.00 | −12.00 | 1.00 | 4.00 | −12.00 | 2.00 |
| Revolver | 1.00 | 1.50 | 12.00 | −7.00 | 1.00 | 8.00 | −6.00 | 3.00 |
| SMG | 2.00 | 2.00 | 10.00 | −10.00 | 0.00 | 8.00 | −6.00 | 1.50 |
| Combat Shotgun | 6.50 | 2.00 | 12.00 | −8.00 | 1.00 | 12.00 | −8.00 | 8.00 |
| Pump Sniper | 0.40 | 5.00 | 12.00 | −6.00 | 0.00 | 12.00 | −6.00 | 15.00 |
| Semi-Auto Sniper | 0.60 | 5.00 | 12.00 | −6.00 | 0.00 | 12.00 | −2.00 | 12.00 |
| Rocket Launcher | 1.00 | 2.00 | 12.00 | −8.00 | 2.00 | 8.00 | −8.00 | 6.00 |

Regels die er direct uit vallen, en die overdraagbaar zijn:

1. **ADS halveert vrijwel altijd de maximale bloom** (12 → 8) én verlaagt de bodem naar ~1° of 0°.
2. **De sniper-identiteit is het hip/ADS-gat**: hipbodem 5°, ADS-bodem 0°.
3. **Recoil-per-schot spant 10×** (1.5 SMG → 15 pump sniper), `spread` spant 17× (0.40 → 7.00).
   Twee onafhankelijke assen — wapenkarakter komt uit de *combinatie*, niet uit één schuif.
4. **De semi-auto sniper heeft −2.00 ADS-regen tegen −6.00 voor de pump**: het snellere wapen
   wordt bewust gestraft op herhaalvuur.

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| WPN-01 | Houdingen | Drie: **hip** (wapen laag, lopen), **ready/high-ready** (wapen geheven, niet gericht), **ironsights/ADS** (gericht). Sommige TPS-en slaan "hip" over zodra er vijanden in de buurt zijn | [REDENERING] op vakconventie | Drie houdingen. Automatische wissel hip → ready bij `bHasHostileInView` of binnen 3 s na vuren; handmatig naar ADS |
| WPN-02 | Wanneer wisselen ze | Sprint forceert altijd naar hip (of lager). Vuren forceert naar minimaal ready | [REDENERING] | Sprint → hip, met de opbrengtijd uit LOC-10 |
| WPN-03 | Blendtijden houdingen | Conventie: hip↔ready ~0.20 s, ready↔ADS ~0.20–0.30 s. Snelle shooters zitten aan de onderkant, "zware" aan de bovenkant | [REDENERING]; [ENGINE] als anker: UE's default anim-blend is **0.2 s** (`FAlphaBlend`), state-machine-transitie **0.2 s**, montage-helper **0.25 s** | **hip↔ready 0.18 s, ready↔ADS 0.22 s.** Camera-blend (CAM-01/CAM-09) moet **dezelfde** duur hebben, anders lopen beeld en wapen uit de pas — dat is een klassiek "voelt niet lekker maar ik weet niet waarom"-defect |
| WPN-04 | ADS-transitietijd | De meest gevoelde wapenparameter die er is. Gemeten band uit Call of Duty's aanzetdata: **160 ms** voor de snelste opbouw tot **584 ms** voor de traagste | [GEMETEN] CoD:MW-aanzetdata; geen engine-bron (Epic's shooter-template heeft geen ADS) | **AR 0.22 s, sidearm 0.16 s** — precies op de snelle rand van de gemeten band, wat past bij een spel waar de squad-laag om aandacht vraagt. De tijd moet *symmetrisch* zijn in en uit, anders voelt loslaten plakkerig |
| WPN-05 | Aim offset | Additieve blendspace op yaw/pitch die het bovenlichaam naar de kijkrichting draait, bovenop de locomotie. Zonder aim offset richt je personage nooit waar je camera kijkt | [REDENERING] op vakconventie; UE levert `UAimOffsetBlendSpace` als asset-type | Bereik **±90° yaw, ±60° pitch**, buiten dat bereik neemt turn-in-place (ROT-04) over. **Bestaat nu niet in ECLIPSE** — dit is de reden dat de ADS-rotatiestand (ROT-08) nog niet aangezet kon worden |
| WPN-06 | Recoil-model | Twee scholen: (a) **patroon** — voorspelbare kick die je leert compenseren, (b) **spreiding/bloom** — cone die groeit per schot en uitdooft. Borderlands is volledig school (b), volledig in graden gemodelleerd | [GEMETEN] Borderlands-accuracy-model, zie hierboven | **School (b) overnemen.** Patroonleren past niet bij een spel met een squad-laag die de aandacht opeist, en bloom is precies wat bij de gestileerde toon past |
| WPN-07 | Recoil-implementatie | Epic's template past recoil toe als **`AddControllerPitchInput(Recoil)`** — direct op de camera, zonder herstel. Borderlands doet het omgekeerde: schermeffecten zijn **decoratief en beïnvloeden de kogelbaan niet** | [ENGINE] `ShooterCharacter.cpp:188-192`; [GEMETEN] BL-accuracy-model | **Volg Borderlands, niet de template.** Camera-kick is puur visueel en veert terug; de ballistische straf zit volledig in `spread + accuracyPool`. Dat maakt de kick vrij tuneerbaar voor gevoel zonder de wapenbalans te raken |
| WPN-08 | Recoil-herstel | Borderlands: **0.2 s idle-vertraging, identiek voor élk wapen**, daarna lineaire uitdemping met een per-wapen regen-tempo van **6–12 graden/seconde** | [GEMETEN] Borderlands-accuracy-model | **0.2 s dode tijd overnemen** (het is één constante voor het hele spel — simpel en het werkt), daarna uitdemping van **8°/s** hipfire en **6°/s** ADS |
| WPN-09 | Kick per schot | Overdraagbaar als **graden per schot**. Borderlands' band: 1.5° (SMG) tot 15° (pump sniper) opgeteld bij de bloompool per schot | [GEMETEN] Borderlands-accuracy-model | ECLIPSE-AR: **bloom +2.5° per schot** (tussen Machine Pistol 1.5 en Combat Rifle 3.5), plus een puur visuele camerakick van ~0.35° die terugveert |
| WPN-10 | Spreiding / cone | Epic's template modelleert dit als **`AimVariance` = cone-halfhoek in graden** (default 0) — dezelfde eenheid als Borderlands, dus de tabel hierboven is 1-op-1 bruikbaar | [ENGINE] `ShooterWeapon.h` `meta = (Units = "Degrees")`; [GEMETEN] BL-tabel | AR: **`spread` 1.3°**, hip-pool **1.5→12°**, ADS-pool **1.0→8°**, regen −8/−6, recoil +2.5. Sidearm: `spread` 2.0°, hip-pool 1.5→8°, ADS-pool 1.0→4°, regen −12 beide. Dat zijn Combat Rifle en Repeater Pistol, licht aangepast |
| WPN-10b | ADS schrapt de bloom, niet alleen halveert | In Borderlands draagt de pool in ADS **nul** bij aan de kogelcone. ADS is dus niet "iets nauwkeuriger" maar **categorisch** nauwkeurig | [GEMETEN] Borderlands-accuracy-model | Sterk aanbevolen voor ECLIPSE: het maakt de ADS-beslissing scherp en leesbaar, en het rechtvaardigt de bewegingsstraf van ADS |
| WPN-11 | Vuurcadans | `RefireRate` in seconden tussen schoten; Epic's default 0.5 s, `bFullAuto = false` | [ENGINE] `ShooterWeapon.h`, `meta = (Units = "s")` | ECLIPSE heeft `FireInterval = 0.15` (= 400 rpm). Dat is aan de trage kant voor een AR; **0.10 s (600 rpm)** leest als een AR, 0.15 s als een burst-wapen |
| WPN-12 | Muzzle flash | Conventie: 2–3 frames zichtbaar, dus **~35–50 ms** bij 60 fps. Langer leest als vuurwerk | [REDENERING] | **40 ms**, met een lichtbron van dezelfde duur. Bij gestileerde art mag de flash groter maar niet langer |
| WPN-13 | Tracers | Conventie: niet elke kogel, maar 1 op 3 à 5, en de tracer reist **langzamer dan de kogel** zodat hij leesbaar is | [REDENERING] op vakconventie | 1 op 3. Tracersnelheid **~10 000 cm/s (100 m/s)** — puur visueel, ontkoppeld van de hitscan-oplossing. Ter vergelijking: Epic's template-projectiel doet 3000 cm/s = 30 m/s, wat een *gameplay*-projectiel is, geen kogel |
| WPN-14 | Hulzen | Conventie: particle of pooled physics-actor, levensduur 2–4 s, met geluid bij landen. Nooit onbeperkt bewaren | [REDENERING] | Particle met collisie, **3 s** levensduur, uitfaden. Geen physics-actors — die kosten bij 40 agents onacceptabel veel |
| WPN-15 | Projectiel versus hitscan | GDD 8: hitscan < 50 m, projectiel ≥ 50 m | [OFFICIEEL] eigen GDD | Grens is er (`AimAssistRange = 5000`, `RangeCm = 5000`). Let op dat de *feedbacktiming* verschilt: bij projectielen komt de hitmarker later, en dat moet de audio ook doen |
| WPN-16 | Muzzle-offset | Projectielen spawnen niet op de loop maar iets ervoor, anders raken ze de eigen collisie | [ENGINE] `ShooterWeapon.h` `MuzzleOffset = 10 cm` | 10 cm overnemen |
| WPN-17 | Wapen-sway | Conventie: lichte, langzame idle-sway in ADS zodat het beeld leeft; verdwijnt bij het schieten | [REDENERING] | **±0.4°, periode ~4 s**, additief op de camera. Sterker maken is een moeilijkheidsknop, geen feelknop |
| WPN-18 | Kritieke treffers | BL2: `CritDamage = Damage × Multiplier × (100% + BaseBonus) / (100% + CritPenalty)`. Shotguns/AR's/pistolen/SMG's ×2, snipers ×2 met **+100% BaseBonus**; sommige klassen dragen een crit-*penalty* | [GEMETEN] BL2-wiki | GDD 8 heeft al **×2.5 kop**. Het bruikbare idee uit Borderlands is de *penalty*-term: wapens die makkelijk crits halen (hoge cadans, lage spread) mogen een crit-straf krijgen in plaats van een damage-nerf |

---

## 5. SPRONG

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| JMP-01 | Sprongkracht → hoogte | `apex = JumpZ² / (2g)`, `airtime = 2·JumpZ / g`, met g = 980 cm/s². UE-default 420 → **90 cm / 857 ms**; TP-template 500 → **128 cm / 1020 ms** | [ENGINE] CMC-ctor + `BaseEngine.ini` `DefaultGravityZ` | ECLIPSE staat op 500 (128 cm apex). Voor een tactische shooter is dat royaal — een mens springt ~40–50 cm. **Advies: 450 → 103 cm / 918 ms**, tenzij de leveldesign de 128 cm nodig heeft. Wel eerst controleren of graybox-geometrie erop gebouwd is |
| JMP-02 | Airtime is de feel, niet de hoogte | Boven ~1.0 s airtime voelt een sprong "maanachtig", onder ~0.6 s als een hupje | [REDENERING] op JMP-01 | Blijf in de band **0.75–0.95 s**. Wil je hoog springen zonder maangevoel, verhoog dan `GravityScale` *en* `JumpZVelocity` samen |
| JMP-03 | Luchtcontrole | `AirControl` default **0.05** (vrijwel geen); TP-template **0.35**. Borderlands 2 verscheept **`PlayerAirControl ≈ 0.11`** — dus fors dichter bij de engine-default dan bij de template. Er is een boost: onder `AirControlBoostVelocityThreshold = 25 cm/s` horizontale snelheid wordt `AirControl` × `AirControlBoostMultiplier = 2` | [ENGINE] CMC-ctor; [GEMETEN] BL2 `GD_Globals.General.Globals` | **0.20**, tussen Borderlands' 0.11 en de template-0.35. 0.35 (huidig) is arcade-achtig. Weet dat de ×2-boost bij stilstaand springen betekent dat je effectief 0.40 hebt precies wanneer het het meest opvalt |
| JMP-12 | Luchtsnelheidscap mag hóger zijn dan de grondsnelheid | Verrassend: BL2 heeft `GroundSpeed 440` maar `AirSpeed 500` — **1.136×**. Springen mag dus een klein snelheidsvoordeel geven | [GEMETEN] BL2 datamined | Bewust níét overnemen voor ECLIPSE: bij ons is springen geen verplaatsingsverb (zie LOC-19 en JMP-11). Wel het noteren, want het verklaart waarom bunnyhoppen in Borderlands loont en waarom wij dat moeten dichttimmeren |
| JMP-04 | Valsnelheid | `BrakingDecelerationFalling` default **0** (geen luchtweerstand, je valt tot de terminale snelheid van de simulatie); TP-template zet **1500**. `FallingLateralFriction` default **0** | [ENGINE] CMC-ctor | 1500 (huidig) is goed: het remt *horizontale* drift in de lucht af zonder de val te beïnvloeden |
| JMP-05 | Variabele spronghoogte | `JumpMaxHoldTime = 0` betekent: **geen** variabele hoogte. Loslaten van de knop doet niets | [ENGINE] `Character.cpp` ctor | Voor een shooter: **zo laten**. Variabele spronghoogte hoort bij platformers; hier maakt het de sprong onvoorspelbaar |
| JMP-06 | Dubbele sprong | `JumpMaxCount = 1` | [ENGINE] `Character.cpp` ctor | Zo laten |
| JMP-07 | Coyote time | **UE heeft dit niet.** Loop je van een rand af, dan is `Jump()` direct genegeerd | [ENGINE] geen enkele treffer op coyote/grace in `CharacterMovementComponent.cpp` of `Character.cpp` | **Bouwen: 100–120 ms venster** na het verlaten van de grond waarin `Jump()` nog werkt. Dit is een van de goedkoopste feel-winsten die er bestaan |
| JMP-08 | Sprong-inputbuffer | **UE heeft dit ook niet.** Druk je 50 ms vóór de landing, dan gebeurt er niets | [ENGINE] idem | **Bouwen: 150 ms buffer.** Zie INP-09 voor het algemene mechanisme |
| JMP-09 | Landing | Conventie: landingsanimatie geschaald op valsnelheid — zachte landing (< ~250 cm/s) zonder onderbreking, harde landing (> ~600 cm/s) met een korte recovery | [REDENERING] | Drie trappen op verticale impactsnelheid: **< 300 / 300–700 / > 700 cm/s**. Alleen de zwaarste onderbreekt de beweging, en dan maximaal 0.35 s |
| JMP-10 | Landingsdemping op de camera | Conventie: camera zakt kort in bij een landing (camera-dip), los van de animatie | [REDENERING] | **−8 uu over 0.06 s in, 0.20 s uit**, geschaald op valsnelheid. Dit doet meer voor het gewicht van een landing dan de animatie zelf |
| JMP-11 | Springen tijdens sprint | Conventie: sprint-sprong houdt de horizontale snelheid vast (dat gebeurt in UE automatisch) maar mag niet gratis extra afstand geven | [ENGINE] `bMaintainHorizontalGroundVelocity`, luchtsnelheid blijft behouden bij ontbreken van laterale wrijving | Accepteren, maar **sprint moet uitgaan bij landen** — anders is springen een gratis snelheidsbehoud over obstakels heen |

---

## 6. HURKEN

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| CRC-01 | Hurksnelheid | `MaxWalkSpeedCrouched` default = `MaxWalkSpeed × 0.5`. Borderlands bevestigt en versoepelt: BL2 `CrouchedPct = 0.5` (220 uu/s), en BL3 ging naar **0.585** (275/470) — de hurkstraf werd tussen twee generaties dus **milder**, niet strenger | [ENGINE] CMC-ctor; [GEMETEN] BL2/BL3 datamined | ECLIPSE: 150 / 420 = **0.36×**, ruim onder zowel de engine-conventie als beide Borderlands-generaties. **Advies: 0.50×, dus 210 cm/s.** Mijn eerdere voorstel van 0.40–0.45 was nog steeds te streng gemeten aan de enige harde bronnen die er zijn |
| CRC-02 | Hurkhoogte | `CrouchedHalfHeight` default **40 uu** tegen een staande 88–96 uu. Dat is een reductie tot **~45%** — heel laag, bijna kruipen | [ENGINE] CMC-ctor `SetCrouchedHalfHeight(40.0f)` | **60 uu** tegen de staande 88 (= 68%). 40 uu maakt de capsule korter dan een mens die hurkt en veroorzaakt zichtbare mesh-capsule-mismatch |
| CRC-03 | Overgangstijd | **UE heeft er geen.** `Crouch()` verandert de capsule in **één frame**; er is geen interpolatie. De vloeiendheid moet volledig uit de mesh-offset en de animatie komen | [ENGINE] `UCharacterMovementComponent::Crouch` — directe `SetCapsuleSize` | **0.25 s** visuele overgang via mesh-Z-interpolatie + animatieblend. Zonder dat teleporteert het personage verticaal |
| CRC-04 | Uit hurk komen kan geblokkeerd zijn | UE test op encroachment bij het uitrekken en annuleert als het niet past | [ENGINE] `Crouch`/`UnCrouch` overlap-test | Accepteren, maar geef **feedback** als het faalt — anders lijkt de knop stuk |
| CRC-05 | Hurkend sprinten | Conventie: **nee**. Sprint en hurk sluiten elkaar uit; sprint drukt uit hurk | [REDENERING] op vakconventie | Sprint forceert `UnCrouch()`. Als de uitrekking geblokkeerd is (CRC-04), faalt sprint stil — dat moet de HUD tonen |
| CRC-06 | Toggle versus hold | Beide moeten bestaan als optie. ECLIPSE gebruikt nu toggle | [REDENERING] | Toggle als default, **hold als optie**. Dit is een toegankelijkheidspunt, geen smaakpunt |
| CRC-07 | Van rand lopen tijdens hurken | `bCanWalkOffLedges = true` maar `bCanWalkOffLedgesWhenCrouching = false` — gehurkt loop je *niet* van een richel af | [ENGINE] CMC-ctor | Zo laten. Dit is precies het gedrag dat je wilt bij dekking op een dakrand |
| CRC-08 | Camera bij hurken | De boom hangt aan de capsule-root; als de capsule krimpt, zakt de camera mee — maar instant (CRC-03) | [ENGINE] afgeleid uit CRC-03 | De camerapivot moet dezelfde 0.25 s-interpolatie volgen als de mesh, niet de capsule |

---

## 7. INPUT

> Dit gebied is al eerder onderzocht in `phase0/CONTROLLER_FEEL.md` (deadzones, curves,
> gevoeligheid, aim assist) met gemeten Halo-/Destiny-/CoD-waarden. **Dat document is
> inmiddels achterhaald door de code**: het noemt deadzone 0.18/0.20, yaw 160°/s, pitch
> 110°/s, ADS 0.80, terwijl de code 0.08/0.08, 240°/s, 180°/s, 0.35 verscheept. Die
> discrepantie is zélf een audit-item (INP-13).

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| INP-01 | Deadzone-waarde | **De hardste bron is de hardwarefabrikant.** Microsofts XInput-constanten: `LEFT_THUMB_DEADZONE = 7849/32767 = ` **0.240**, `RIGHT_THUMB_DEADZONE = 8689/32767 = ` **0.265**, `TRIGGER_THRESHOLD = 30/255 = ` **0.118**. UE's legacy default is **0.25**, Enhanced Input's default **0.20**, en Warframe verscheept **20% van de stickuitslag** als default. Vier onafhankelijke bronnen die allemaal tussen 0.20 en 0.27 landen — tegenover ECLIPSE's **0.08** | [OFFICIEEL] XInput-constanten (ook in UE 5.8's eigen bron opgenomen); [ENGINE] `BaseInput.ini:26-29`, `InputModifiers.h:182`; [OFFICIEEL] DE-patchnotes | 0.08 is **veel te laag** voor een willekeurige controller uit het veld: het is afgesteld op de gemeten drift van één specifiek pad (max 0.048 op LY, `controller_kalibratie.json`). Een versleten pad drift verder. **Advies: 0.15 als default** (bewust onder de hardwarenorm omdat we scaled-radial gebruiken, wat de rand verzacht) **met een gebruikersschuif van 0.05–0.30** |
| INP-02 | Deadzone-vorm | Drie vormen, en de vakliteratuur is het eens over de winnaar: **axiaal** (per as, vierkant — snapt naar de assen bij een cirkelbeweging), **radiaal ongeschaald** (cirkelvormig maar springt van 0 naar de drempel), en **scaled radial** — `v = normalize(v) × (mag − dz)/(1 − dz)` — die vloeiend is zonder discontinuïteit. UE's Enhanced Input noemt precies die derde vorm `Radial` **met weergavenaam "Smoothed Radial"** en de comment *"For most games, this will give the smoothest feeling analog values"*; `UnscaledRadial` krijgt de waarschuwing *"may result in feeling 'jumpy'"*. Warframe stapte in hotfix 29.2.2 expliciet af van een vierkante zone wegens *"inconsistent mapping for diagonal inputs"* | [OFFICIEEL] Josh Sutphin (Warhawk/Starhawk), Ryan Juckett (Hypersect); [ENGINE] `InputModifiers.h:140-195`; [OFFICIEEL] DE-patchnotes | **Scaled radial** — precies wat `HandleLook`/`HandleMove` al doen (radiaal + herschalen boven de drempel). Groen, niets veranderen aan de vorm |
| INP-03 | Herschaling boven de deadzone, en de buitenrand | Zonder herschaling springt de output bij het verlaten van de deadzone. Juckett voegt de **buitenste** deadzone toe: `normalizedMag = min(1, (mag − lo)/(1 − hi − lo))`, nodig omdat sommige sticks de eenheidscirkel niet halen op een deel van hun rotatie | [OFFICIEEL] Juckett/Hypersect; [ENGINE] `UInputModifierDeadZone` heeft er `UpperThreshold` voor | Herschaling gebeurt al. **De buitenste drempel bestaat nog niet in ECLIPSE** — voeg `UpperThreshold ≈ 0.95` toe zodat een stick die net niet 1.0 haalt tóch volle snelheid geeft. En **niet ook nog een modifier op de IMC zetten**: dubbele deadzone is een klassiek stille bug |
| INP-04 | Responscurve | UE's legacy default is **exponent 1.0 (lineair)**, Enhanced Input's `ResponseCurveExponential` default ook **1.0**. Call of Duty publiceert de *vormen* officieel — Standard (versnellende machtscurve), Linear (1:1), Dynamic (omgekeerde S-curve) — maar **geen enkele shipped shooter publiceert een exponentwaarde**. Warframe verscheept überhaupt geen curve-optie voor gameplay-aim | [ENGINE] `BaseInput.ini`, `InputModifiers.h:313`; [OFFICIEEL] Activision-blog; [OFFICIEEL] Warframe-instellingen | ECLIPSE gebruikt 2.0. Twee onderzoeksrondes hebben nu **geen enkele bron** voor die waarde gevonden — `CONTROLLER_FEEL.md`'s "forumfolklore"-oordeel staat. **Laat 2.0 staan als speelbeslissing, gelabeld [REDENERING].** Tegenwicht: Nesky's mistake #39 is *"Using linear sensitivity"*, dus een curve hébben is wél onderbouwd; alleen het getal niet |
| INP-05 | Kijksnelheid | Als graden/seconde bij volle uitslag — volledig overdraagbaar. `DescribeLookTuning()` rapporteert het al als "seconden per 360°" | [ENGINE] geen default (Enhanced Input schaalt niet zelf) | 240°/s yaw = **1.5 s per 360°**. Dat is aan de trage kant voor een TPS met vijanden achter je. **Band 240–320°/s met een gebruikersschuif**; 180°/s pitch (0.75× yaw) is een goede verhouding |
| INP-06 | ADS-gevoeligheid | **Opgelost — dit is geen smaakkwestie meer.** UE 5.8 verscheept de juiste formule in `UInputModifierFOVScaling`, met de comment *"This is the proper way to scale based off FOV changes"*: `Scale = tan(FOV/2) / tan(80°/2)`, met `kPlayerInput_BaseFOV = 80.0f` hard in de code. De naïeve FOV-ratio is dus **fout**; het moet de tangens-halve-hoek-verhouding zijn. Dezelfde formule circuleert in de community als de "focal length"-methode | [ENGINE] `InputModifiers.cpp:405-418`, lokaal geverifieerd | ECLIPSE gaat van FOV 80 naar 64. De **correcte** factor is `tan(32°)/tan(40°) = ` **0.745** — niet de naïeve 0.80, en zeker niet de verscheepte **0.35**. **Advies: 0.745.** Ter kalibratie van hoe fout 0.35 is: Mass Effect LE1 gaat van 70° naar 39.6° (een veel agressievere inzoom) en komt dan nog op 0.514 uit |
| INP-06b | ADS-gevoeligheid per zoomtrap | Mass Effect LE1 verscheept een complete ladder in `BIOWeapon.ini`: `ZoomFOV 39.6` (AR/pistool/shotgun, `CameraSensitivityMultiplier 1.0`), `22.6` (sniper trap 1, sens 1.0), `11.4` (sniper trap 2, sens **0.5**). De gevoeligheid wordt dus pas op de *tweede* zoomtrap gehalveerd | [OFFICIEEL] verscheepte LE1-configwaarden | Als ECLIPSE ooit scoped wapens krijgt: per trap een eigen multiplier, berekend met de formule uit INP-06, niet één globale ADS-schuif |
| INP-07 | Muisbehandeling | UE's legacy muis-default: **DeadZone 0, Exponent 1.0, Sensitivity 0.07**. Muis krijgt dus nooit een deadzone en nooit een curve | [ENGINE] `BaseInput.ini:30-32` | Huidige aanpak (rauw, geen deadzone, geen curve) is **correct en engine-conform**. Niet aanraken |
| INP-08 | Kijkversnelling (ramp-up) | Sommige shooters versnellen de draaisnelheid als je de stick vasthoudt | [REDENERING]; `CONTROLLER_FEEL.md` heeft dit permanent op 0 gezet | Op 0 laten. Die beslissing is eerder genomen en mag alleen op een *speelbevinding* heropend worden |
| INP-09 | Inputbuffering | Epic's eigen combat-template buffert acties: `AttackInputCacheTimeTolerance = 1.0 s`, `ComboInputCacheTimeTolerance = 0.45 s`. De actiegame-conventie ligt veel korter: gemeten buffers en coyote-vensters in bekende platformers liggen op **4–6 frames bij 60 fps = 67–100 ms** (Celeste ~100 ms coyote / ~83 ms buffer, Hollow Knight ~67 ms). Algemene implementatiegidsen convergeren op **0.1–0.2 s** | [ENGINE] `CombatCharacter.h:98,144`; [GEMETEN] frame-analyse door derden — ⚠ Maddy Thorson publiceert Celeste's framegetallen bewust **niet**, dus dit zijn community-metingen | Een **algemene inputbuffer bouwen**, niet per actie ad hoc. Vensters: sprong **150 ms**, vuren **120 ms**, herladen **250 ms**, dodge **200 ms**. Epic's 1.0 s is een melee-waarde en te lang voor een shooter |
| INP-09b | Overbufferen is een echt defect | Klassiek falen: je drukt Schild, bedenkt je, drukt Aanval — en het spel voert alsnog het gebufferde schild uit. De remedie is dat de consumerende actie de input **als verbruikt stempelt** (tijdstempel ver in het verleden zetten) in plaats van hem te laten staan | [GEMETEN] breed gedocumenteerd (Smash Ultimate als canoniek voorbeeld) | Bij het bouwen van INP-09 meteen meenemen. Een buffer zonder verbruikstempel is erger dan geen buffer |
| INP-10 | Reactietijd van een ingedrukte knop | Enhanced Input `ActuationThreshold = 0.5`; `Hold` vuurt na **1.0 s**, `HoldAndRelease` na **0.5 s**, `Tap` vereist loslaten binnen **0.2 s** | [ENGINE] `InputTriggers.h:130,334,358,378` | Voor "hold to command" (Q/LB) is 1.0 s **te traag**. Een hold-om-modus-te-openen hoort op **0.15–0.25 s**, anders voelt de knop dood. Tap-drempel 0.2 s overnemen |
| INP-11 | End-to-end latentie | UE laat de renderthread standaard **één frame achterlopen** (`r.OneFrameThreadLag = 1`). NVIDIA's eigen framing: *"most gamers play on systems with 50-100 ms of system latency"*; een peer-reviewed studie (ACM MMSys '23, 39 deelnemers) meet de basislatentie van een snelle client op **22 ms ± 5 ms** en stelt de praktijkband op *"about 25 ms for a fast gaming system to around 100 ms for a typical computer system"* | [ENGINE] `RenderingThread.cpp:2435-2438`; [OFFICIEEL] NVIDIA Reflex-documentatie; [OFFICIEEL] Liu & Claypool, ACM MMSys 2023 | De LOCKED eis van **≤ 100 ms** is de *bovengrens van acceptabel*, niet een doel. Streef **≤ 40 ms** op de RTX-doelmachine. **Meet het met een camera**, niet met een logstatement |
| INP-11b | Hoe hard latentie werkelijk telt | NVIDIA's gecontroleerde studie (8 deelnemers × 400 trials × 2 condities): bij **12 ms** systeemlatentie was de mediane taaktijd **1.348 s**, bij **20 ms** was hij **1.530 s**. Een verschil van **8 ms latentie kost 182 ms taaktijd** — ruim twintigvoudige versterking (p = 0.001). De ACM-studie vindt lineaire degradatie van zowel prestatie als beleving over 25–250 ms, met *"a 100 ms increase in latency results in about a 2 second increase in time to select"* | [OFFICIEEL] NVIDIA; [OFFICIEEL] ACM MMSys 2023 | Dit is het argument om latentiebudget serieus te nemen, ook in een spel zonder competitieve modus. Belangrijk detail uit de ACM-studie: **lokale latentie is niet weg te compenseren** — netwerkcompensatie helpt alleen tegen netwerklatentie |
| INP-12 | Framerate-onafhankelijkheid | Kijksnelheid moet in graden/seconde × `DeltaSeconds`, niet per frame | [REDENERING] | `HandleLook` doet dit al goed voor de stick. **Let op: muisinput mag juist níét met `DeltaSeconds` vermenigvuldigd worden** — die levert al een delta |
| INP-13 | ⚠ Documentatie loopt achter op de code | `CONTROLLER_FEEL.md` verscheept andere getallen dan `EclipseCharacterTypes.h` | [OFFICIEEL] eigen repo | Eén van beide corrigeren, in dezelfde commit als de wijziging (change-management-regel uit `DOCUMENTATION_README.md`) |
| INP-14 | Aim assist: welk type | **De terminologie is officieel bevestigd.** Gears 5's eigen patchnotes onderscheiden **"adhesion"** (het dradenkruis kleeft aan / volgt het doel) en **"friction"** (draaisnelheid daalt over een doel) als twee losse systemen, en verschepen ze allebei. Daarnaast draait er **bullet magnetism** die **altijd aan staat en niet uitgezet kan worden — ook niet voor muis en toetsenbord** | [OFFICIEEL] Gears 5 TU3-patchnotes; [GEMETEN] community voor de magnetisme-uitzondering | ECLIPSE doet alleen **friction**, en dat is de verdedigbare keuze. Cone **4°** met `Floor 0.45` = kijksnelheid daalt tot 45%. Dat is aan de sterke kant; **advies floor 0.55–0.65**. Adhesion pas toevoegen als friction bewezen onvoldoende is |
| INP-14b | Aim assist versus zwakke plekken | Borderlands' eigen wiki documenteert dat hun **sticky aim naar het massamiddelpunt snapt**, en dat dat **actief vecht met het crit-systeem** op snipers — plus dat het in drukte de verkeerde vijand pakt | [GEMETEN] Borderlands-wiki, "Aim Assist" | ECLIPSE heeft koptreffers (×2.5). **Centroid-snap is dus verboden.** Friction (wat we doen) heeft dit probleem niet; mocht er ooit adhesion komen, dan moet die naar het **crit-volume** biasen, niet naar het midden |
| INP-14c | Adhesion is standaard blind voor zichtlijn | Gears 5 moest patchen omdat *"aim adhesion tracked enemies through low cover"*. Destiny 2's magnetisme heeft hetzelfde probleem, samengevat door de reverse-engineers als *"all walls shall be treated as air"* — met als bijvangst dat magnetisme **vijandposities door muren heen verraadt** | [OFFICIEEL] Gears 5 TU3-patchnotes; [GEMETEN] Destiny 2-compendium | Concrete implementatievalkuil om nu al af te vangen: de check in `ComputeAimAssistScale` moet een **zichtlijntrace** doen, niet alleen hoek + afstand. Nu doet hij dat niet |
| INP-14d | De canonieke onderdelenlijst | De vakstandaard-indeling (Insomniac, GDC 2013, gebouwd voor Resistance 3) kent drie kerntechnieken plus twee die er los van staan: **magnetism** (draaisnelheid volgt het doel mee), **centering** (draait actief naar het doelmidden), **friction** (draaisnelheid daalt over een doel), plus **ADS-snapping** (correctie bij het indrukken van richten) en **bullet magnetism** (de kogel buigt). Mass Effect verscheept precies drie schakelaars die hierop mappen: `bFrictionEnabled`, `bAdhesionEnabled`, `bZoomSnapEnabled` met `Min/MaxZoomSnapDistance` | [OFFICIEEL] Nick Weihs, GDC 2013; [OFFICIEEL] verscheepte ME-configvelden | Gebruik deze woorden in de codebase. ECLIPSE heeft alleen **friction**; benoem dat ook zo, zodat later duidelijk is wat er ontbreekt en wat bewust ontbreekt |
| INP-14e | Assist schaalt met zoom, niet met sterkte | Destiny 2: de Aim-Assist-stat vergroot de **conehoek**, niet de magnetismesterkte — en zoom **deelt** de cone: `cone / magnification`. Mass Effect doet hetzelfde met de hand: `FrictionMultiplier` 1.0 → 0.667 → 0.5 en `AdhesionMultiplier` 1.0 → 0.5 → 0.5 over drie zoomtrappen | [GEMETEN] Destiny 2-compendium; [OFFICIEEL] LE1 `BIOWeapon.ini` | ECLIPSE's `AimAssistConeDegrees = 4.0` is één vaste waarde. **Deel hem door de ADS-zoomfactor** zodat mikken echt precisiewerk wordt en niet juist méér hulp geeft |
| INP-14f | Assist gaten per intentie, niet per toestand | DE's beste ontwerpuitspraak in het hele corpus (Melee 3.0): *"the new system will have more intelligence on aim assist on a per-attack basis… Aim Assist will be disabled for almost all attacks in the Forward and Tactical Forward Combos, and enabled for most of the Neutral and Tactical Neutral Combo attacks."* Oftewel: **assist UIT wanneer de input een richting uitdrukt, AAN wanneer hij "raak dat ding" uitdrukt** | [OFFICIEEL] DE dev-workshop | Direct toepasbaar: assist uit tijdens sprint, tijdens een slide, tijdens dodge en tijdens de commando-hold (dat laatste doet ECLIPSE al goed). Aan tijdens hipfire en ADS |
| INP-14g | Sterkte als ratio, niet als absoluut getal | De enige schone sterktereeks die publiek is: Apex hanteert **PC 0.4, console 0.6, muis+toetsenbord 0**. Die getallen zijn eenheidsloos t.o.v. een niet-gepubliceerde basis — maar de **verhouding console = 1.5× PC** is wel bruikbaar | [GEMETEN]/[OFFICIEEL] Respawn-communicatie | Als ECLIPSE ooit per-invoerapparaat differentieert: **muis krijgt geen friction**, gamepad wel, en de verhouding tussen "aan" en "sterk" hoort rond 1.5× te liggen. Mass Effect LE3 verscheept adhesion en dampening trouwens allebei op **0** — assist is daar effectief uit |
| INP-15 | Aim assist mag niet stiekem zijn | Gears 5 verscheept Aim Assist als **toggle**, plus aparte Look/Target/Zoom-gevoeligheid, inner/outer deadzone en aim-acceleratie. Borderlands 3 zet Aim Assist onder Options > Accessibility | [GEMETEN] Gears 5- en BL3-instellingenlijsten | Schuif bouwen. ⚠ De concrete getallen uit die menu's ("Look Sensitivity 26", "outer deadzone 0") zijn **niet overdraagbaar** — het zijn punten op Gears' eigen interne schaal |
| INP-16 | Aim assist tijdens commando-modus | Terecht uitgezet in ECLIPSE | [OFFICIEEL] eigen code | Zo laten |
| INP-17 | Y-as-inversie | Standaard-optie | [OFFICIEEL] aanwezig als `bInvertLookY` + cvar | Groen |

---

## 8. FEEDBACK

**Auditstatus vooraf: dit hele gebied bestaat nog niet in ECLIPSE.** Een zoektocht door
`Eclipse/Source` op `CameraShake|ForceFeedback|Haptic|Recoil|MuzzleFlash|Tracer|Hitmarker|DamageNumber|Footstep`
levert **nul treffers**. Het wapen is een kale `EclipseHitscanWeaponComponent` die schade
toepast en verder niets terugkoppelt.

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| FDB-01 | Hitmarker | Kort visueel teken op het dradenkruis. **Let op: Borderlands heeft er géén** — de trefferbevestiging is volledig wereld-ruimtelijk (het schadegetal), en er is zelfs **geen hitmarker-geluid** in vanilla; de audiobevestiging is het materiaal-impactgeluid. De modcommunity voegt beide apart toe | [GEMETEN] reverse-engineering van het BL2-feedbacksysteem (`Better Damage Feedback`, `HitSounds`-mods) | Voor ECLIPSE **wél een hitmarker bouwen** — wij zijn third-person met commando-afleiding en hebben een UI-anker nodig dat Borderlands niet nodig heeft. **90 ms vol, 120 ms uitfade**, drie varianten (normaal/crit/kill). Maar volg Borderlands' les dat het *wereld*-signaal het zwaarste moet wegen |
| FDB-02 | Schadegetallen | **Architectuurbevinding:** Borderlands' schadegetallen zijn **particles in wereldruimte**, geen UI-widgets en geen screen-space tekst. De uitgestelde knoppen van het systeem zijn: getal-particle met element-kleur · aparte kleur bij crit · een los "CRITICAL"-effect · **schaalvergroting bij crit** · geheel uitschakelbaar | [GEMETEN] idem | **Als particles bouwen, niet als widgets.** Bij 40 actieve agents (GDD 8.6) zijn honderden UMG-widgets onbetaalbaar; een particle-systeem is dat niet. **0.7 s levensduur**, 60 uu opwaartse drift, crit groter + andere kleur |
| FDB-02b | Dichtheid ís het signaal | Waarom Borderlands' getallen als "juice" lezen en niet als spreadsheet: ze **stapelen en spatten uiteen** onder aanhoudend vuur. De dichtheid wordt de feedback | [GEMETEN] idem | Getallen bewust laten spreiden (willekeurige laterale offset) in plaats van netjes stapelen. Dit is een van de goedkoopste manieren om de Borderlands-toon te raken |
| FDB-03 | Trefferflits op de vijand | Korte materiaalflits op de geraakte mesh — de goedkoopste bevestiging die er is | [REDENERING] | **60 ms**, additief wit. Werkt uitstekend bij gestileerde/celshaded art |
| FDB-04 | Richtingindicator bij schade | Boog rond het dradenkruis die de richting van de schade toont | [REDENERING] | Boog van ~60°, **1.2 s** zichtbaar, laatste 0.4 s uitfaden. Meerdere bronnen stapelen, niet vervangen |
| FDB-05 | Screen shake bij vuren | Klein en kort; het is interpunctie, geen effect | [ENGINE] UE geeft geen enkele richtwaarde (alle shake-defaults zijn 0) | **±0.25° rotatie, 90 ms**, per schot. Bij volautomatisch vuur niet stapelen maar hernieuwen, anders bouwt het op tot onbruikbaar |
| FDB-06 | Screen shake bij impact/explosie | Groter, met uitdovende envelope. DE benoemt de twee knoppen die er werkelijk toe doen: *"Decreased the strength of the camera shake and increased the time between shakes"* — **sterkte** en **interval**, niet frequentie | [REDENERING] voor de waarden; [OFFICIEEL] DE-patchnotes voor de knoppen | **±1.5° rotatie + ±4 uu translatie, 350 ms**, Perlin-noise met uitdemping. Afstandsattenuatie via `UCameraShakeSourceComponent`. Bouw *sterkte* en *interval* als de twee tunebare parameters |
| FDB-06b | Shake is lokaal, altijd | DE handhaaft dit al tien jaar consequent in patchnotes: *"Zephyr's Tornado post FX and camera shake will only be visible to the caster"*, *"Melee Heavy Slam camera shake is now only seen on the local player"*, *"Volt's Speed boost won't affect other player's FOV"* | [OFFICIEEL] Warframe-patchnotes, meerdere updates | **Direct relevant voor ECLIPSE's squad-laag.** Een squadmate die een explosief gebruikt mag de spelercamera niet laten schudden, en een squad-ability mag nooit de speler-FOV raken |
| FDB-07 | Shake mag de aim niet verplaatsen | Architectuurregel: shake hoort op de camera-post, niet op de `ControlRotation`. Nesky's mistake **#43: "Excessively shaking the camera"** en **#44: "Bouncing the camera with the avatar's walk cycle"** (geen head-bob) | [ENGINE] afgeleid uit WPN-07 (Epic's template doet het fout); [OFFICIEEL] Nesky GDC 2014 | Hard afspreken. Anders is elke shake een aimfout |
| FDB-08 | Haptiek | Twee motoren: laagfrequent (zwaar, dof) en hoogfrequent (scherp, tik). Vuren = korte HF-tik; explosie = LF-dreun; treffer ontvangen = beide | [REDENERING]; UE levert `UForceFeedbackEffect` + `UHapticFeedbackEffect_Base`, zonder defaults | Vuren **50 ms HF**, treffer ontvangen **160 ms LF+HF**, landing **120 ms LF** geschaald op valsnelheid. Sterkteschuif verplicht |
| FDB-09 | Audio-cues | De laagsterkte doet het werk: transient (aanslag) / body / tail (staart+ruimte), plus een aparte afstandslaag. Wapengewicht komt voor een groot deel uit audio, niet uit animatie | [REDENERING] op vakconventie | Minimaal drie lagen per wapen. Er is al een `EclipseAudioSubsystem` met een layer-1 bed en een mission-complete sting — de **wapen- en trefferlagen ontbreken nog** |
| FDB-10 | Alles moet uitzetbaar | **Bevestigd door de referentie:** Gears 5 verscheepte een uitgebreide toegankelijkheidslijst met o.a. een **aparte toggle voor roadie-run camera shake**; Borderlands laat schadegetallen volledig uitschakelen; The Division 2 heeft een **"Motion Sickness Mode"** die óók motion blur uitzet | [OFFICIEEL] Gears 5 accessibility-lijst; [OFFICIEEL] Ubisoft-supportdocumentatie; [GEMETEN] BL-modsysteem | Vijf aparte schuiven — algemene shake, sprint-shake, FOV-kick, camera-bob, haptiek — en geen enkele verzamelknop. Genre-standaard, geen extra |
| FDB-10b | **Vervang verwijderde beweging, verwijder hem niet alleen** | Het leerzaamste toegankelijkheidsverhaal uit de hele studie. DE zette per ongeluk de recoil-schermbeweging mee uit met de shake-toggle, draaide dat terug, en kreeg toen misselijkheidsklachten over precies die recoil-jiggle. De uiteindelijke oplossing zijn **twee onafhankelijke instellingen**: *Enable Screen Shake* (binair) én *Recoil Type* met drie standen — **Camera** ("camera bounces with each shot", default) / **Reticle** ("a diamond represents the recoil, camera movement reduced") / **Reduced** ("no recoil representation, camera movement reduced") | [OFFICIEEL] DE dev-workshop + hotfix 40.0.3 | **Het principe is overdraagbaar en belangrijk: als je camerabeweging weghaalt voor toegankelijkheid, hercodeer je de informatie in een niet-vestibulair kanaal.** DE liet het richtpunt stilstaan en gaf de recoil een bewegende ruit. Neem die drie standen letterlijk over voor ECLIPSE's recoil (WPN-07) |
| FDB-10c | FOV als delta in plaats van absoluut | The Division noemt zijn FOV-optie **"Additional Field of Vision"** — een optelwaarde, geen absolute FOV | [OFFICIEEL] Ubisoft-supportdocumentatie | Slim voor een TPS: de camerarig bepaalt de compositie, dus een *delta* laat de speler comfort bijstellen zonder de gecomponeerde framing te slopen. Aanbevolen boven een absolute FOV-schuif |
| FDB-10d | Gameplay mag niet van de FOV-instelling afhangen | DE stelt het hard: *"the camera's Field of View setting is treated as if it were 90, regardless of the actual value set by the player"* voor zichtlijnberekeningen | [OFFICIEEL] Warframe-documentatie | Overnemen. Zodra ECLIPSE een FOV-schuif krijgt, moeten aim-assist-cone, zichtlijn en AI-perceptie op een **vaste** FOV rekenen. Anders is de schuif een exploit |
| FDB-11 | Hit-stop | Kort bevriezen bij een zware treffer geeft gewicht. Jan Willem Nijman (Vlambeer, *"The Art of Screenshake"*) noemt **~20 ms** ("Sleep") bij een dodelijke treffer — het enige harde getal in die hele talk | [OFFICIEEL] Nijman, Vlambeer | **20–40 ms**, korter dan mijn eerdere voorstel. Alleen bij melee, shotguns en kills. Nooit bij automatisch vuur — dan wordt het stotteren |
| FDB-14 | De juice-checklist | Diezelfde talk geeft 30 losse technieken; de camera-relevante zijn **camera lerp**, **camerapositie**, **screen shake**, **speler-recoil**, **wapenvertraging**, **wapen-kick** en **verhoogde camera-kick-frequentie** | [OFFICIEEL] Nijman, Vlambeer | Bruikbaar als aparte auditlijst voor §8 zodra er überhaupt feedback in de build zit. Nu is het antwoord op alle zeven "bestaat niet" |
| FDB-12 | Feedback-latentie | De trefferbevestiging moet binnen ~50 ms na het schot komen, anders koppelt de speler hem niet aan zijn eigen actie | [REDENERING] | Bij projectielen (WPN-15) komt de treffer later; dan moet de **muzzle**-feedback direct zijn en de **treffer**-feedback op impact |
| FDB-13 | Kill-bevestiging | Een kill moet een sterker signaal krijgen dan een treffer, over meerdere zintuigen tegelijk | [REDENERING] | Hitmarker-variant + audio-sting + korte haptiek. Bij een gestileerde toon mag dit royaal zijn — dat is precies wat Borderlands doet |

---

## 9. ANIMATIE

**Auditstatus vooraf:** `EclipseAnimInstance` kent alleen `GroundSpeed`, `IdleWeight`,
`WalkWeight`, `RunWeight`, `StrideRate`, `bIsInAir`, `bIsDowned`. Er is **geen richting,
geen yaw-offset, geen aim-offset, geen lean, geen foot IK, geen turn-in-place**. Sprint is
de rencyclus versneld afgespeeld (`StrideRate` 0.6–1.8).

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| ANI-01 | Standaard blendtijd | UE's `FAlphaBlend` default is **0.2 s**; een state-machine-transitie default **0.2 s**; de montage-helper **0.25 s**. Gears' gemeten transitieband ligt op **0.3–0.5 s** | [ENGINE] `AlphaBlend.h:86`, `AnimStateTransitionNode.cpp:84`; [OFFICIEEL] The Coalition GDC 2018 | 0.2 s als huisdefault nemen en alleen gemotiveerd afwijken. Dat maakt afwijkingen zichtbaar in review |
| ANI-01b | **Snellere bronbeweging hoort een KORTERE blend te krijgen** | Contra-intuïtief, maar het is Gears' verscheepte regel. Om overshoot te vermijden klemmen zij de transitieduur met **`t1 = min(t1, −5·x0 / v0)`** (x0 = pose-verschil, v0 = pose-snelheid bij transitiestart). Hun eigen voorbeeld: bij `v0 = −12.6` is 0.5 s prima; bij `v0 = −20.0` schiet 0.5 s door en klemt het systeem naar **0.3 s** | [OFFICIEEL] The Coalition, GDC 2018 "Inertialization" | Overnemen als regel. Het verklaart waarom een vaste blendtijd overal fout aanvoelt: bij snelle beweging is 0.2 s te lang en zie je de pose doorschieten |
| ANI-14 | **Inertialisatie in plaats van blenden — en UE 5.8 heeft het al** | Gears 4 **schrapte geblende transities volledig**: *"we eliminate blended transitions altogether… we simply cut immediately to the target animation and let the post-process clean up the results."* Er wordt een quintische curve op de doelanimatie opgeteld die pose-verschil én snelheid op t=0 matcht en naar nul convergeert. Prestatie op Xbox One: sequence ~20 µs, blend ~10 µs, klassieke transitie 30 µs, **inertialisatie 12 µs — netto ~60% goedkoper**, en de kosten worden *vast* in plaats van variabel | [OFFICIEEL] The Coalition, GDC 2018 + SIGGRAPH 2017 | **UE 5.8 levert dit kant-en-klaar**: `FAnimNode_Inertialization`, transitietype **`TLT_Inertialization`**, en `RequestInertialization(Duration, BlendProfile)` aanroepbaar vanuit gameplay-code — exact het "Request Inertialization"-haakje dat Gears beschrijft. Gebruik het voor ROT-09 (sprint-rotatiesnap), voor houdingswissels (WPN-03) en voor alle traversal-in/uitstappen |
| ANI-15 | **Het feitelijke recept voor "zwaar maar responsief"** | The Coalition, letterlijk: *"Gears controls are very responsive (twitchy). Filter inputs to locomotion blend spaces. If filtered values are too far from actual values → snap to actual values, inertialize. Fluid pose even with twitchy inputs."* De **gameplay**-input is ongefilterd; alleen de **blendspace**-inputs worden gefilterd | [OFFICIEEL] The Coalition, GDC 2018 | Dit is de belangrijkste architectuurregel van dit hele document. Concreet voor ECLIPSE: `GroundSpeed` en straks `Direction` gaan **gefilterd** de blendspace in (laagdoorlaat, ~8–10/s), terwijl `CharacterMovement` de rauwe waarden houdt. Wijkt het filter meer dan een drempel af, dan snap + `RequestInertialization`. **Gewicht kopen in de animatielaag kost nul latentie; gewicht kopen in `MaxAcceleration` kost het wel** |
| ANI-02 | Locomotie-blends | Idle↔walk en walk↔run moeten korter dan de huisdefault, anders "zwemt" de overgang | [REDENERING] | **0.12–0.15 s**. Start/stop-overgangen langer (0.20 s) omdat daar een gewichtsverschuiving in zit |
| ANI-03 | 1D versus 2D locomotie | Zodra het lichaam niet meer naar de loopsrichting draait (ROT-02, gerichte stand) heb je een **2D**-blendspace nodig. Dat is niet gratis: gemeten op Xbox One kost een 1D-blendspace **+150%** en een 2D-blendspace **+300%** t.o.v. één losse sequence | [REDENERING] voor de noodzaak; [OFFICIEEL] The Coalition, SIGGRAPH 2017 voor de kosten | Verplicht zodra ADS-rotatie aangaat, **maar** met GDD 8.6's budget van ~40 actieve agents is +300% per agent relevant. Zet de 2D-blendspace alleen op de speler en dichtbije squadmates; vijanden op afstand houden de 1D-set. **Bestaat nu niet** — dit is de blokkade onder ROT-08 |
| ANI-04 | Voetslip | De hoofdoorzaak is een mismatch tussen afgespeelde animatiesnelheid en werkelijke capsulesnelheid. Twee oplossingen: **stride warping** (animatie schalen naar snelheid) en **distance matching** (animatie-tijd afleiden uit afgelegde afstand) | [ENGINE] beide zitten in 5.8: `AnimationWarping/AnimNode_StrideWarping.h`, `AnimationLocomotionLibrary/AnimDistanceMatchingLibrary.h` | ECLIPSE gebruikt nu een `StrideRate`-playrate-schaling van 0.6–1.8×. Dat is de grove versie en glijdt aan de randen. **Stride warping is de nette upgrade en zit al in de engine** |
| ANI-05 | Aim offset | Additieve blendspace, bovenlichaam naar kijkrichting | [REDENERING] | Zie WPN-05. **Voorwaarde voor bijna alles in §2 en §4** |
| ANI-06 | Leunen in bochten | Additieve lean op basis van de laterale acceleratie; geeft gewicht aan richtingwissels | [REDENERING] | **Max ±10° roll**, gedreven door de component van `Acceleration` loodrecht op `Velocity`, met een interp van ~8/s. Meer dan 12° leest als een motorfiets |
| ANI-07 | Additieve lagen | Ademhaling, wapen-sway, treffer-reacties en recoil horen additief bovenop de basispose, niet als aparte states | [REDENERING] | Vier additieve slots reserveren in de AnimGraph vanaf het begin — achteraf inbouwen is duur |
| ANI-08 | Voetplanting bij stilstand | Zonder foot IK zweven of zinken voeten op oneffen grond | [ENGINE] `AnimationWarping/AnimNode_FootPlacement.h` — o.a. `SpeedThreshold = 60 cm/s` (daaronder planten), `UnplantRadius = 35`, `MaxExtensionRatio = 0.5`, `FloorLinearStiffness = 1000`, bekken `MaxOffset = 50`, `HeelLiftRatio = 0.5` | Het `FootPlacement`-node gebruiken in plaats van zelf twee-bots-IK bouwen. De defaults zijn bruikbaar; alleen `SpeedThreshold` afstemmen op `MinAnalogWalkSpeed` |
| ANI-09 | ⚠ Mesh-capsule-mismatch | De mesh wordt op **Z = −90** gezet, maar de capsule is nooit ingesteld en gebruikt dus `ACharacter`'s default halfhoogte **88** | [ENGINE] `Character.cpp` `InitCapsuleSize(34, 88)`; [OFFICIEEL] `EclipseCharacterTypes.h:316` `MeshZOffset = -90` | **De voeten staan 2 cm in de vloer.** Ofwel de capsule expliciet op 90 zetten, ofwel de offset op −88. Dit is precies het soort defect dat een audit moet vangen |
| ANI-10 | Capsulemaat | `ACharacter` default 34 r / 88 hh (176 cm); TP-template 42/96 (192 cm); combat-variant 35/90 | [ENGINE] drie ctors | **Expliciet kiezen** en in het tuning-asset zetten. Nu is het een ongekozen default, en alles (step-up, hurken, camerahoogte, dekkingshoogte) hangt eraan |
| ANI-11 | Root motion versus capsule-gedreven | Shooters gebruiken vrijwel altijd **capsule-gedreven** locomotie (voorspelbaar, netwerkbaar) en root motion alleen voor traversal, melee en takedowns | [REDENERING] op vakconventie; [ENGINE] `HasAnimRootMotion()` schakelt `CalcVelocity` volledig uit | Zo doen. Weet dat root motion de hele movementberekening **overslaat** — feel-tuning heeft dan geen effect |
| ANI-12 | Voetstap-notifies | Audio en VFX hangen aan anim-notifies, niet aan een timer | [REDENERING] | Notifies op elke voetplant, gekoppeld aan het oppervlaktetype. **Bestaat nu niet** (0 treffers op `Footstep`) |
| ANI-13 | Orientation warping | Bij strafen laat orientation warping het onderlichaam meedraaien zonder aparte animaties | [ENGINE] `AnimNode_OrientationWarping.h`: `RotationInterpSpeed = 10`, `DistributedBoneOrientationAlpha = 0.5`, `LocomotionAngleDeltaThreshold = 90°`, `MaxCorrectionDegrees = 180` | Sterk aan te raden zodra ANI-03 speelt: het bespaart een compleet 8-richtingen-animatieset **én het drukt de +300%-kosten uit ANI-03**, want warping evalueert één sequence in plaats van een blendspace. Gears gebruikt dezelfde redenering voor motion warping |
| ANI-16 | **Fasematching bij terugkeer naar locomotie** | Gears' oplossing voor "ijsschaatsen na een vault": bereken een **fasequotiënt** uit de positie van de linkervoet t.o.v. de rechter, geprojecteerd op de snelheidsvector (of de voorwaartsvector bij stilstand). Die oscilleert sinusvormig door de pascyclus. Bij opstart wordt per frame een tabel voorberekend, monotoon gefilterd, geïnverteerd en benaderd met een **stuksgewijs lineaire curve** fasequotiënt → framenummer. Bij terugkeer uit een niet-locomotie-toestand start de locomotie **op het passende frame** | [OFFICIEEL] The Coalition, GDC 2018 | Goedkoop, algemeen, en het is het antwoord op de vraag "waarom glijdt onze uitstap uit een vault altijd weg". Implementeer dit vóór foot IK — foot IK repareert het symptoom, fasematching de oorzaak |
| ANI-17 | Motion warping in plaats van blendspaces voor traversal | Gears laat animatoren **warp-punten** in de animatie plaatsen die met fysieke landmarks corresponderen; runtime wordt het traject van het personage aangepast zodat de punten samenvallen. Per warp-punt is de timing van translatie, rotatie en facing **onafhankelijk** instelbaar. Er is bewust **maar één actieve warp tegelijk** | [OFFICIEEL] S. Dickinson, GDC 2017; [ENGINE] `Engine/Plugins/Animation/MotionWarping` | Bevestigt TRV-07. Het argument is niet alleen kwaliteit maar ook kosten: één warpbare sequence vervangt een blendspace, dus één animatie-evaluatie in plaats van twee of drie |

---

## 10. TRAVERSAL

| ID | Item | Conventie | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| TRV-01 | Step-up hoogte | ⚠ **Herzien — mijn eerste advies was verkeerd onderbouwd.** Uitgedrukt als percentage van de personagehoogte is 45 uu geen uitschieter maar precies de norm van de hele id/Valve/Epic-lijn: Quake `STEPSIZE 18` qu op een hull van 56 = **32%**; Half-Life/Source **18 units** op 72 = **25%**; UE **45 cm** op 176 = **25.6%**. Unity is de uitzondering met een *realistische* aanbeveling van "0.1–0.4 voor een 2 m mens" = 5–20%. Een echte traptrede is ~17.8 cm ≈ **10%** | [OFFICIEEL] Quake III `bg_local.h`, Valve Dimensions-documentatie, Unity CharacterController-handleiding; [ENGINE] UE CMC-ctor | **De hoge waarde is opzet**, niet slordigheid: hij laat het personage stoepranden, puin, drempels en trapmeshes absorberen **zonder traversal-animatie**. Voor ECLIPSE is de vraag dus niet "is 45 te hoog" maar "willen we de traversal-laag zichtbaar hebben". Omdat de GDD vault/mantle als eigen verbs opvoert: **verlagen naar 35 uu (20%)** — nog steeds boven een traptrede, maar laag genoeg dat kniehoge dekking een vault wordt in plaats van een stap. Dat is een *ontwerp*keuze, geen correctie |
| TRV-02 | Loopbare helling | ⚠ **Herzien.** **45° is een industrieconstante**, geen tuningkeuze: Quake III `MIN_WALK_NORMAL 0.7f` → acos(0.7) = **45.573°**; Half-Life gebruikt letterlijk dezelfde test (`normal[2] < 0.7`) op drie plekken; Source meet 45.573°; UE komt op **44.77°** uit. Het getal recurreert omdat 0.7 een goedkope magische normaal-Z is | [OFFICIEEL] Quake III `bg_local.h:25`, Half-Life `pm_shared.c`; [ENGINE] UE `SetWalkableFloorZ(0.71f)` | **44.77° laten staan.** Mijn eerdere voorstel van 40° was een smaakoordeel tegen vier onafhankelijke bronnen in. Wel de Quake-waarschuwing overnemen: *"walking up a slope along a diagonal will feel much steeper"* — **hellingen en ramps as-uitgelijnd bouwen** in de graybox |
| TRV-03 | Trappen | **Bevestigd als officiële praktijk.** Epic publiceert er een tutorial over ("Setting up Collision Geometry for Stairs") en stelt de regel expliciet: *"Simple collision… tends to affect player movement. Complex collision usually refers to the visual model geometry and tends to affect bullet hits."* De Source-variant is een `toolsplayerclip`-ramp over de treden | [OFFICIEEL] Epic-tutorial + Epic-staf in de bijbehorende thread; [GEMETEN] Source-mappingpraktijk | Verplicht maken in de graybox-builder: **elke trap krijgt een simpele ramp-collider voor beweging en de mesh alleen voor kogeltreffers.** Zonder dat schokt de camera per trede |
| TRV-04 | Tegen een muur lopen | **Universeel: glijden, nooit dood stoppen.** Dezelfde functie in de hele id-lijn — projecteer de snelheid op het vlak en ga door. Quake III's `PM_ClipVelocity` gebruikt `OVERCLIP 1.001f`, en dát detail is het belangrijkste: de speler wordt een haartje *van* het vlak af geduwd zodat de trace van het volgende frame niet meteen opnieuw botst | [OFFICIEEL] Quake III `bg_pmove.c:145`, Half-Life `pm_shared.c:715`; [ENGINE] UE `SlideAlongSurface` | UE doet dit al correct. Wat je **wel** moet toevoegen: een animatie-reactie (hand tegen de muur, of minimaal een snelheidsafhankelijke stop-pose) — anders loopt de speler op de plaats |
| TRV-05 | Richels | `bCanWalkOffLedges = true`; `LedgeCheckThreshold = 4.0`; `PerchRadiusThreshold = 0`, `PerchAdditionalHeight = 40`. Ter vergelijking: Quake, Quake III en Half-Life hebben **geen enkele richel-test** — je loopt er gewoon af. UE's perch-systeem is de rijkere variant: het krimpt de capsule voor de vloertest, zodat je niet op een splinter kunt balanceren maar óf echte voetsteun hebt óf valt | [ENGINE] CMC-ctor; [OFFICIEEL] Quake III `bg_pmove.c`, Half-Life `pm_shared.c` (afwezigheid van de test) | `PerchRadiusThreshold` op **0** betekent dat je op een centimeter geometrie kunt blijven staan. Voor leesbaarheid: **op ~10 uu zetten** |
| TRV-06 | Mantle/vault | **De canonieke drempels bestaan, en ze zijn uitgedrukt als fractie van de personagehoogte — dus volledig overdraagbaar.** GDC 2012, Arne Olav Hallingstad (Lead Gameplay Programmer, Splash Damage, *BRINK*), "Vault, Slide, Mantle": obstakel binnen **2.5× de breedte van de bounding box**; **vault bij ledge-hoogte 0.4–0.8× personagehoogte**; **mantle bij 0.8–1.4×**; **auto wall hop** boven mantle-hoogte + spronghoogte | [OFFICIEEL] GDC 2012-deck, Splash Damage | Voor een personage van 1.76 m: **vault 70–140 cm, mantle 140–246 cm**, daarboven auto-hop. Dat sluit naadloos aan op de GDD's mantle-plafond van **2.4 m**. Let op dat de banden **aaneengesloten** zijn — geen dode zone tussen vault en mantle, en dát is wat traversal niet-pietluttig laat voelen. Onder 70 cm doet de step-up (TRV-01) het werk |
| TRV-07 | Mantle-uitlijning | Root-motion-animaties passen zelden precies op de gevonden rand | [ENGINE] `Engine/Plugins/Animation/MotionWarping` is beschikbaar | `MotionWarpingComponent` gebruiken om de root-motion naar het gevonden randpunt te warpen. Zelf uitlijnen is de moeilijke weg |
| TRV-08 | Traversal-detectie | Conventie: een capsule-sweep vooruit + een neerwaartse trace om de randhoogte te vinden, elk frame terwijl er vooruit-input is | [REDENERING] | Detectie vooraf laten lopen zodat de prompt/animatie *voordat* de speler de knop indrukt al klaarstaat; anders voelt vault altijd te laat |
| TRV-09 | Sprint-slide | `graybox_feel_targets.md` noemt dit "de signature verb" | [OFFICIEEL] eigen GDD (LOCKED) | Slide is fysiek een tijdelijke `BrakingDeceleration`-verlaging + capsulehoogte-reductie + root motion. **Nog niet gebouwd.** Duur **0.7 s**, alleen vanuit sprint, cooldown 1.0 s |
| TRV-10 | Dekking | GDD: contextuele soft-attach, geen sticky snap, hoekleun, dash tussen dekking ≤ 8 m | [OFFICIEEL] eigen GDD (LOCKED) | **Nog niet gebouwd.** Feel-punt: soft-attach mag de speler nooit *vasthouden* — losmaken moet met dezelfde stick werken die je erin bracht |
| TRV-12 | **Annuleervensters op dekkingsovergangen zijn waar de diepte zit** | Gears' *wall bounce* — de diepste bewegingstechniek in het meest gespeelde competitieve TPS van de laatste twee decennia — is **geen ontworpen move**. Het is dekking-ingaan dat wordt geannuleerd door een richtingsomkering: schuin op de muur af, dekkingsknop precies bij contact, dan **direct** de tegenovergestelde richting. Het spel geeft voorrang aan de nieuwe input boven de lopende dekkingsanimatie. Het begon als een door de community ontdekte exploit en werd later als bedoeld geaccepteerd | [GEMETEN] Gears-community-gidsen; [OFFICIEEL] Gears E-Day-regisseur Matt Searcy over het temperen ervan | **Ontwerpvraag die ECLIPSE nu moet beantwoorden, niet later:** is dekking-ingaan annuleerbaar, en hoe breed is het omkeervenster? Dat ene besluit bepaalt of er emergente diepte in het bewegingssysteem zit. Praktisch gevolg dat Gears ook leerde: **dekking en sprint moeten op aparte knoppen** — gedeeld leidt tot ongewilde rollen |
| TRV-13 | Beweegsnelheden binnen dekking moeten op elkaar aansluiten | Gears 5 TU3 verhoogde slide-to-cover met +3.5% en de cover-slip-boost met **precies dezelfde +3.5%**, expliciet *"tuned to match outgoing roadie run speed"* | [OFFICIEEL] Gears 5 TU3 | De les is de **gelijkschakeling**, niet het percentage: elke uitstap uit dekking moet dezelfde snelheid hebben als de sprint waarin hij uitkomt, anders voel je een schokje bij elke overgang |
| TRV-11 | Simulatiestap | `MaxSimulationTimeStep = 0.05 s`, `MaxSimulationIterations = 8`. Bij lage framerates wordt beweging opgedeeld; daarboven verlies je nauwkeurigheid en schiet je door dunne geometrie | [ENGINE] CMC-ctor | Laten staan, maar het verklaart waarom feel bij < 20 fps anders is. Meet nooit feel op de GTX 1050-devbox |

---

## 11. Gebieden die tijdens de studie opdoken en niet in de opdracht stonden

| ID | Item | Waarom het feel bepaalt | Bron | Aanbeveling ECLIPSE |
|---|---|---|---|---|
| EXT-01 | Commando-modus als feel-gebied | Een squad-shooter heeft een *tweede* besturingsmodus. De overgang ertussen is een feel-oppervlak op zich: tijdvertraging, camerablend, cursorgedrag, en wat er met de aim gebeurt | [OFFICIEEL] GDD 4.1.2, SPEC-P2-02 | `EnterBlendSeconds`/`ExitBlendSeconds` staan beide op **0.0** — de modus klapt er hard in. **0.15 s in, 0.10 s uit** is genoeg om de knip weg te halen zonder de reactietijd te schaden |
| EXT-02 | Order-bevestigingslus | LOCKED-eis: order → zichtbare bevestiging **≤ 1 s**, "voelt gehoorzaamd" is de poortmetriek | [OFFICIEEL] `graybox_feel_targets.md`, `FEEL_GAUNTLET_P2-02.md` | Dit is een feel-eis, geen AI-eis. De bevestiging moet **direct** komen (audio + marker), ook als de squadmate er nog 3 s over doet om te bewegen |
| EXT-03 | Squadmates als bewegingsobstakel | Squadmates die in de deuropening staan zijn de grootste onzichtbare feel-killer in squad-shooters | [ENGINE] `bEnablePhysicsInteraction = true`, `RepulsionForce = 2.5`, `bUseRVOAvoidance = false` | RVO-avoidance of push-through voor squadmates aanzetten. Een squadmate mag de speler **nooit** blokkeren |
| EXT-04 | Tijdvertraging en input | Bij 30% tijdvertraging voelt de camera 3× trager als de kijksnelheid met `DeltaSeconds` schaalt | [REDENERING] op `HandleLook` | Kijksnelheid tijdens vertraging **compenseren** met 1/dilation, anders is de commandomodus onbestuurbaar |
| EXT-05 | Wapenwissel en herladen | Beide ontbreken volledig in ECLIPSE (`BESTURING.md`) en zijn allebei zwaar feel-dragend | [OFFICIEEL] eigen repo | Herlaad-annulering (herladen afbreken door te sprinten of te mikken) is een verwachting geworden; plan het meteen mee |
| EXT-06 | Downed/revive-toestand | `bIsDowned` bestaat al in de anim-instance | [OFFICIEEL] eigen code | Downed is een aparte locomotie-toestand met eigen snelheid en camera. Nu is het alleen een anim-vlag |
| EXT-07 | Toegankelijkheid als feel-oppervlak | Deadzone-, gevoeligheids-, shake-, FOV- en aim-assist-schuiven zijn geen extra's; zonder die schuiven is elke gekozen waarde onherroepelijk | [REDENERING] | Eén settings-scherm met alle waarden uit §7 en §8. Dit is ook de goedkoopste manier om feel-discussies te beëindigen |
| EXT-08 | Meetbaarheid | Zonder meting is elke feel-uitspraak smaak | [OFFICIEEL] LOCKED-eis van ≤ 100 ms | Bouw een `Eclipse.Feel.Dump` die per frame input, `Velocity`, `Acceleration`, `ControlRotation` en `ActorRotation` logt. Dan is een audit reproduceerbaar in plaats van anekdotisch |

---

## 12. Wat ik NIET kon onderbouwen

Eerlijk gemarkeerd, want deze lijst is even bruikbaar als de rest. Alles hieronder staat
in de tabellen als **[REDENERING]**, en mag dus als eerste worden omgestoten.

**Geen enkele bron gevonden voor:**

1. **Gears' camera-boomlengte en schouderoffset in Unreal Units.** Dit is het grootste gat
   in het hele document, en pijnlijk, want Gears is dé referentie voor over-shoulder-framing.
   Niets gepubliceerd, geen gelekte config, geen mod die het blootlegt. CAM-01 en CAM-02
   leunen daarom volledig op UE's eigen defaults.
2. **Gears' loop-/jog-/roadie-run-snelheden in uu/s of m/s.** Alleen de 1.2×-ratio en de
   procentuele deltas uit 2019 zijn openbaar. De broncode is nooit vrijgegeven en Gears 4/5
   zijn niet moddeerbaar op een manier die dit blootlegt.
3. **Gears' aim-camera-FOV en de intrek-transitietijd.** Bevestigd is alleen *dát* er een
   aparte aim-FOV bestaat.
4. **Camera-lag- en spring-arm-dempingswaarden** van enige referentietitel. CAM-04 leunt
   puur op UE's default van 10.
5. **De absolute duur van Gears' roadie-run-to-shoot-vertraging** (alleen "−25%" bekend).
   WPN-02/LOC-10's 0.25 s is afgeleid.
6. **Concrete ADS-transitietijden in seconden** van enige referentietitel. Zoekopdrachten op
   Borderlands' `ZoomFOV`/`SightFOVScale` leverden niets. WPN-04 is vakconventie.
7. **BL2's sprint-multiplier als getal.** De enige zichtbare waarde is een schaal op een
   onleesbare attribuutbonus — expliciet niet-overdraagbaar.
8. **Borderlands' `AccelRate`**, en daarmee elke acceleratie-/remwaarde voor de hele serie.
   LOC-01/LOC-03 leunen volledig op de engine-wiskunde.
9. **BL2's zwaartekracht**, waardoor `JumpZ = 630` niet naar een apexhoogte om te rekenen is.
   De speedrun-kennisbank meldt alleen kwalitatief "reduced gravity".
10. **Turn-in-place-drempels** van shipped games. ROT-04/ROT-05 leunen op de 90°-drempel uit
    Epic's orientation-warping-node — dat is een *analogie*, geen bron.
11. **Hitmarker- en schadegetal-timings.** FDB-01/FDB-02 zijn ontworpen, niet gemeten.
    Borderlands' *architectuur* is bekend, zijn *timings* niet.
12. **Haptiek-duren.** FDB-08 is volledig redenering; geen enkele referentietitel publiceert
    hierover, en van Gears is zelfs geen impulse-trigger-ontwerpuitspraak te vinden.
13. **Screen-shake-amplitudes.** UE geeft nul richtwaarden (alle defaults zijn 0), en er is
    geen publieke conventie in graden.
14. **Numerieke aim-assist-waarden**: adhesion-conehoek, friction-sterkte, magnetisme-straal.
    De *terminologie* is nu officieel bevestigd (INP-14), de getallen niet. Dit bevestigt wat
    `CONTROLLER_FEEL.md` al vaststelde.
15. **Expliciete bevestiging van foot IK in Gears.** Fasematching is gedocumenteerd (ANI-16),
    voetplanting-IK niet.
16. **Ontwerpintentie achter Borderlands' schadegetallen.** De mechaniek is gereverse-engineerd,
    maar er is geen Gearbox-uitspraak over waarom of hoe het getuned is.
17. **Verscheepte aim-assist-conehoeken in graden voor enige westerse AAA-shooter.** Halopedia,
    de Destiny-wiki's en Bungie's eigen posts beschrijven allemaal de *structuur* zonder één
    getal. De enige conehoeken in graden die überhaupt gevonden zijn, zijn Warframe's
    *ability*-cones (15° / 55° / 90°) en die zijn iets anders.
18. **Verscheepte exponentwaarden voor responscurves.** Twee onderzoeksrondes, nul resultaten.
    Alleen UE's 1.0 en Call of Duty's kwalitatieve curvevormen. ECLIPSE's 2.0 blijft onbewezen.
19. **Camera-boomlengtes en schouderoffsets van enige shipped TPS.** Alleen Unity's
    Cinemachine-voorbeeldconfiguratie is publiek. Mass Effect's `SFXCameraMode_*`-offsets zitten
    in binaire `.pcc`-bestanden en zijn nooit gepubliceerd.
20. **Pitch-limieten van enige referentietitel.** Alleen Epic's eigen template geeft er twee.
21. **Nick Weihs' GDC 2013-slides over aim assist.** Alleen video op archive.org; geen transcript,
    geen deck. De taxonomie is bruikbaar, de implementatiedetails niet.
22. **The Division's bewegingscijfers, ADS-tijden, camera-afstanden en de deadzone/aim-acceleratie-
    patchgeschiedenis.** Ubisoft publiceert geen FOV-bereik, geen units en geen defaults.
    ⚠ Bovendien: de aanname dat Division 2 uitgebreide aim-opties verscheepte is **niet bevestigd** —
    gevonden is alleen een binaire Aim Assist-toggle plus een aparte granaat-toggle.
23. **Mass Effect's walk/sprint-snelheden, dodge-afstand en -duur**, en alle Andromeda-getallen.
24. **Warframe's default-FOV** (alleen het bereik 52–90 verticaal is gemeten) en de
    camera-preset-offsets in enige eenheid.

**Sinds deze ronde wél onderbouwd, en dus uit deze lijst verdwenen:** Borderlands'
bewegingswaarden (§0b), strafe- en achteruit-ratio's (LOC-11/LOC-12), recoil en spreiding in
graden (§4), het hele hit-feedback-model (FDB-01/FDB-02), blend- en transitietijden
(ANI-01b/ANI-14), de aim-assist-taxonomie (INP-14), **de ADS-gevoeligheidsformule (INP-06 —
opgelost met UE's eigen broncode)**, de deadzone-band (INP-01, met XInput als hardste bron),
de sprint-FOV-kick (CAM-10, Mass Effect LE1), de pitch-limieten (CAM-07, Epic's template), en
de latentiebudgetten (INP-11b, twee onafhankelijke studies).

**Bewust niet overgenomen omdat het niet overdraagbaar is:**

| Waarde | Waarom niet |
|---|---|
| Gears 5 "Look/Target/Zoom Sensitivity 26–28", inner/outer deadzone-standen | Punten op Gears' eigen interne schuifschaal |
| BL2 `BaseValueScaleConstant = 1` voor sprint | Een schaal op een onbekende bonus, **geen** ratio |
| Gears 5 TU3's "+50% / +15% / +5%" | Relatief aan een niet-gepubliceerde basislijn. De **afgeleide** 0.85-achteruitratio is wél overdraagbaar, de deltas niet |
| BL3's FOV 90/110 op PC vs 70 op console | FOV is beeldverhouding- en platformafhankelijk |
| Elke gevoeligheidsschaal (0–100, 0–500, "sensitivity 7") | Geen fysieke eenheid |
| Elke "aim assist strength" uit een ander spel | Betekent alleen iets binnen die eigen implementatie van friction-versus-adhesion |

**Wat een volgende onderzoeksronde het beste kan aanvallen**, in volgorde van rendement:

1. **§8 FEEDBACK** draagt de meeste [REDENERING] van alle gebieden (hitmarker-duren,
   schadegetal-timings, haptiekduren, muzzle-flash- en tracer-conventies). Kansrijke bronnen:
   GDC-audio-talks over wapengeluid, en frame-analyse van hitmarkers door derden.
2. **§9 ANIMATIE**: verscheepte locomotie-blendtijden en lean-hoeken. The Coalition's papers
   gaven de *transitie*-architectuur maar geen blendspace-parameters.
3. **§10 TRAVERSAL**: step-up-hoogtes en hellinglimieten van shipped games. Source-engine
   (18 units) en Quake-afgeleiden zijn hier de kansrijkste harde bron, plus level-designdocs.
4. **The Division** als geheel — deze ronde leverde er vrijwel niets over op, en het is
   onduidelijk of dat aan de bronnen of aan de bereikbaarheid lag.

**Wel veilig overgenomen, want in een fysieke of gedeelde eenheid:** alle `uu/s`-waarden
(1 uu = 1 cm, en élke referentietitel in dit document draait op Unreal — BL1/2/TPS en
Gears 1–3 op UE3, BL3/Wonderlands en Gears 4/5 op UE4, BL4 en Gears E-Day op UE5, ECLIPSE
op UE 5.8), alle graden uit de nauwkeurigheidstabel, alle tijden in seconden en µs uit de
Coalition-papers, de 1.2×-roadie-run-ratio, de 0.85-achteruitratio, de 0.2 s idle-regen-
vertraging, de 0.3–0.5 s transitieband en de klemformule `t1 = min(t1, −5·x0/v0)`.

**Structureel niet aanwezig in UE 5.8, dus altijd zelfbouw:**
aim assist (een hoofdletterloze zoektocht op `aimassist`/`aim_assist` door de volledige
`Engine/Source` én `Engine/Plugins` levert **nul** treffers), coyote time, sprong-inputbuffer,
hurk-overgangstijd, strafe/achteruit-penalty, turn-in-place, ADS, hitmarkers, schadegetallen,
camera-shake-richtwaarden. **[ENGINE]**, alle vijf lokaal geverifieerd.

---

## Bijlage A — UE 5.8 engine-defaults, geverifieerd

Alle waarden hieronder zijn gelezen uit de lokale engine-installatie. Eenheid is cm / cm-s
/ graden tenzij anders vermeld. **Dit is de hardste bron in dit document en meteen
implementeerbaar.**

### `UCharacterMovementComponent` — constructor
`Engine/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp`

| Veld | Default | Opmerking |
|---|---|---|
| `GravityScale` | 1.0 | wereldgravitatie −980 cm/s² (`BaseEngine.ini:3407`) |
| `GroundFriction` | 8.0 | richtingswisselsnelheid, géén rem |
| `MaxAcceleration` | 2048 | |
| `MaxWalkSpeed` | 600 | |
| `MaxWalkSpeedCrouched` | 300 | `= MaxWalkSpeed × 0.5` |
| `MinAnalogWalkSpeed` | 0 | template zet 20 |
| `BrakingDecelerationWalking` | 2048 | `= MaxAcceleration` |
| `BrakingDecelerationFalling` | 0 | template zet 1500 |
| `BrakingFrictionFactor` | 2.0 | Epic-comment: *"Historical value, 1 would be more appropriate"* |
| `bUseSeparateBrakingFriction` | false | |
| `BrakingSubStepTime` | 1/33 s | geklemd op 1/75..1/20 |
| `BRAKE_TO_STOP_VELOCITY` | 10 | constante, `.cpp:100` |
| `RotationRate` | (0, 360, 0) | template 500, shooter-variant 600 |
| `bUseControllerDesiredRotation` | false | |
| `JumpZVelocity` | 420 | apex 90 cm, airtime 857 ms |
| `JumpOffJumpZFactor` | 0.5 | |
| `AirControl` | 0.05 | template 0.35 |
| `AirControlBoostMultiplier` | 2.0 | |
| `AirControlBoostVelocityThreshold` | 25 | onder deze horizontale snelheid ×2 luchtcontrole |
| `FallingLateralFriction` | 0 | |
| `MaxStepHeight` | 45 | |
| `WalkableFloorZ` | 0.71 | ⇒ `WalkableFloorAngle` **44.77°** |
| `PerchRadiusThreshold` | 0 | |
| `PerchAdditionalHeight` | 40 | |
| `LedgeCheckThreshold` | 4.0 | |
| `bCanWalkOffLedges` | true | |
| `bCanWalkOffLedgesWhenCrouching` | false | |
| `CrouchedHalfHeight` | 40 | `SetCrouchedHalfHeight(40.0f)` |
| `Mass` | 100 | |
| `bMaintainHorizontalGroundVelocity` | true | |
| `MaxSimulationTimeStep` / `Iterations` | 0.05 s / 8 | |
| `NetworkSimulatedSmoothLocationTime` | 0.100 s | rotatie 0.050 s |
| `bEnablePhysicsInteraction` | true | `RepulsionForce` 2.5 |
| `bUseRVOAvoidance` | false | `AvoidanceConsiderationRadius` 500 |

### `ACharacter` / `APawn`
`Character.cpp`, `Pawn.cpp`

| Veld | Default |
|---|---|
| Capsule | radius 34, halfhoogte 88 |
| `bUseControllerRotationYaw` | **true** (pitch/roll false) |
| `JumpMaxHoldTime` | 0 (geen variabele spronghoogte) |
| `JumpMaxCount` | 1 |
| `BaseEyeHeight` | 64 |
| `CrouchedEyeHeight` | `CrouchedHalfHeight × EyeHeightRatio` |

### `USpringArmComponent` / `UCameraComponent` / `APlayerCameraManager`

| Veld | Default |
|---|---|
| `TargetArmLength` | 300 |
| `SocketOffset` / `TargetOffset` | (0,0,0) |
| `ProbeSize` | 12 |
| `ProbeChannel` | `ECC_Camera` |
| `bDoCollisionTest` | true |
| `bEnableCameraLag` / `bEnableCameraRotationLag` | false |
| `CameraLagSpeed` / `CameraRotationLagSpeed` | 10 / 10 |
| `CameraLagMaxTimeStep` | 1/60 s |
| `bUsePawnControlRotation` | false |
| `UCameraComponent::FieldOfView` | 90 |
| `APlayerCameraManager::DefaultFOV` | 90 |
| `ViewPitchMin` / `ViewPitchMax` | −89.9 / +89.9 |

### Enhanced Input
`Engine/Plugins/EnhancedInput/.../InputModifiers.h`, `InputTriggers.h`

| Veld | Default |
|---|---|
| `UInputModifierDeadZone::LowerThreshold` | 0.2 |
| `UInputModifierDeadZone::UpperThreshold` | 1.0 |
| `UInputModifierDeadZone::Type` | `Radial` |
| `UInputModifierResponseCurveExponential::CurveExponent` | (1,1,1) |
| `UInputModifierSmoothDelta` | Lerp, Speed 0.5, EasingExponent 2.0 |
| `ActuationThreshold` | 0.5 |
| `UInputTriggerHold::HoldTimeThreshold` | 1.0 s |
| `UInputTriggerHoldAndRelease::HoldTimeThreshold` | 0.5 s |
| `UInputTriggerTap::TapReleaseTimeThreshold` | 0.2 s |

### Legacy `BaseInput.ini` (nog steeds de duidelijkste engine-uitspraak over deadzones)

| As | DeadZone | Exponent | Sensitivity |
|---|---|---|---|
| `Gamepad_LeftX/Y`, `Gamepad_RightX/Y` | **0.25** | 1.0 | 1.0 |
| `MouseX/Y`, `Mouse2D` | **0.0** | 1.0 | 0.07 |

### Animatie

| Veld | Default | Bestand |
|---|---|---|
| `FAlphaBlend` blendtijd | **0.2 s** | `AlphaBlend.h:86` |
| State-machine `CrossfadeDuration` | **0.2 s** | `AnimStateTransitionNode.cpp:84` |
| Montage-slot-helper blend in/uit | 0.25 s | `AnimMontage.h:978` |
| `OrientationWarping::RotationInterpSpeed` | 10 | `AnimNode_OrientationWarping.h` |
| `OrientationWarping::LocomotionAngleDeltaThreshold` | 90° | idem |
| `FootPlacement::SpeedThreshold` | 60 cm/s | `AnimNode_FootPlacement.h` |
| `FootPlacement::UnplantRadius` | 35 | idem |
| `FootPlacement` bekken `MaxOffset` | 50 | idem |
| Transitietype `TLT_Inertialization` | beschikbaar als state-machine-transitielogica | `AnimStateMachineTypes.h:49` |
| `FAnimNode_Inertialization` | aanwezig; `RequestInertialization(Duration, BlendProfile)` aanroepbaar vanuit gameplay | `AnimNode_Inertialization.h:37` |
| `bAllowInertializationForSelfTransitions` | false | `AnimStateTransitionNode.h:96` |

### Templates en shooter-variant

| Bron | Waarden |
|---|---|
| `TP_ThirdPersonCharacter` | capsule 42/96 · `bOrientRotationToMovement` true · `RotationRate` (0,500,0) · `JumpZVelocity` 500 · `AirControl` 0.35 · `MaxWalkSpeed` 500 · `MinAnalogWalkSpeed` 20 · `BrakingDecelerationWalking` 2000 · `BrakingDecelerationFalling` 1500 · boom 400, `bUsePawnControlRotation` true |
| `CombatCharacter` (TP-variant) | capsule 35/90 · `MaxWalkSpeed` 400 · boom 100 met positie- én rotatielag aan · `AttackInputCacheTimeTolerance` 1.0 s · `ComboInputCacheTimeTolerance` 0.45 s |
| `ShooterCharacter` (FP-variant) | `RotationRate` (0,600,0) · recoil = `AddControllerPitchInput(Recoil)` · **geen ADS** |
| `ShooterWeapon` | `RefireRate` 0.5 s · `bFullAuto` false · `AimVariance` = cone-halfhoek in graden (0) · `MuzzleOffset` 10 cm · `MagazineSize` 10 |
| `ShooterProjectile` | `InitialSpeed`/`MaxSpeed` 3000 cm/s (30 m/s) |

### Rendering / latentie

| Veld | Default |
|---|---|
| `r.OneFrameThreadLag` | **1** (renderthread loopt één frame achter) |
| `bSmoothFrameRate` | false |

---

## Bijlage B — Afgeleide rekentabel

Berekend met een exacte nabootsing van `ApplyVelocityBraking` (substep 1/33 s,
stopdrempel 10 cm/s) en van de lineaire acceleratie uit `CalcVelocity`.
Script: `scratchpad/brake.py`. **Deze getallen zijn engine-waar, niet geschat.**

### Tijd en afstand tot topsnelheid

| Configuratie | Tijd | Afstand |
|---|---|---|
| UE-default 600 / 2048 | 293 ms | 88 cm |
| TP-template 500 / 2048 | 244 ms | 61 cm |
| ECLIPSE ren 420 / 1400 (huidig) | 300 ms | 63 cm |
| ECLIPSE sprint 650 / 1400 (huidig) | **464 ms** | 151 cm |
| Voorstel ren 420 / 1800 | 233 ms | 49 cm |
| Voorstel sprint 650 / 1800 | 361 ms | 117 cm |

### Stoptijd en glijafstand

| `BrakingFriction` × `Factor`, `Decel` | sprint (650) | ren (420) |
|---|---|---|
| 8 × 2, 2000 — **huidig ECLIPSE** | 100 ms / 23 cm | 83 ms / 13 cm |
| 4 × 1, 2000 — **aanbevolen** | 200 ms / 57 cm | 150 ms / 28 cm |
| 2 × 1, 2000 | 250 ms / 73 cm | 183 ms / 34 cm |
| 2 × 1, 1500 | 317 ms / 90 cm | 217 ms / 43 cm |
| 0 × 1, 1200 | 533 ms / 176 cm | 350 ms / 74 cm |

### Richtingwissel van 90° op sprint, per `GroundFriction`

| `GroundFriction` | Tijd |
|---|---|
| 4 | 400 ms |
| **8 (default, aanbevolen)** | **233 ms** |
| 12 | 167 ms |
| 16 | 117 ms |

### Sprongbanen

| `JumpZVelocity` | Apex | Stijgtijd | Airtime |
|---|---|---|---|
| 420 (engine-default) | 90 cm | 429 ms | 857 ms |
| 450 (aanbevolen) | 103 cm | 459 ms | 918 ms |
| 500 (huidig ECLIPSE + template) | 128 cm | 510 ms | 1020 ms |
| 600 | 184 cm | 612 ms | 1224 ms |

---

## Bijlage C — Auditstartpunt: ECLIPSE, stand 2026-07-25

Voor elk gebied de kortste samenvatting van wat er is. Geen oordeel, alleen de stand,
zodat een audit weet waar hij moet beginnen.

| Gebied | Stand |
|---|---|
| Locomotie | Drie snelheden bestaan (180/420/650), sprint werkt via `MaxWalkSpeed`-overschrijving. Geen strafe-/achteruit-penalty, geen loop-toets op toetsenbord, `GroundFriction` ongekozen |
| Rotatie | Alleen de vrije stand (`bOrientRotationToMovement`). Geen turn-in-place, geen ADS-rotatiestand, geen yaw-offset in de anim-instance |
| Camera | Boom 300, offset (0,55,65), lag 12, probe 12, FOV 80, pitch ±70, ADS ×0.55 arm / ×0.80 FOV. Geen schouderwissel, geen sprint-camera, geen shake, ⚠ commando-pullback wijkt af van de spec |
| Wapen | Kale hitscan-component met schade. Geen houdingen, geen ADS-animatie, geen recoil, geen spreiding, geen muzzle/tracer/hulzen |
| Sprong | `JumpZ` 500, `AirControl` 0.35. Geen coyote time, geen buffer, geen landingstrappen |
| Hurken | Toggle, snelheid 150 (0.36×). Geen overgangstijd, hoogte is de engine-default 40 |
| Input | Radiale deadzone 0.08, exponent 2.0, 240/180°/s, ADS 0.35, aim-assist-slowdown 4°/floor 0.45. Geen schuiven, geen inputbuffer, ⚠ doc wijkt af van code |
| Feedback | **Niets.** Geen shake, haptiek, hitmarker, schadegetallen, voetstappen |
| Animatie | Snelheid → idle/walk/run-gewichten + `StrideRate`. Geen richting, aim-offset, lean, foot IK of turn-in-place. ⚠ mesh 2 cm in de vloer |
| Traversal | Alles engine-default. Geen vault, mantle, slide of dekking |

---

## Bijlage C2 — Camera-auditlijst naar John Nesky

John Nesky's *"50 Game Camera Mistakes"* (GDC 2014) is de canonieke camera-talk. **Let op:
de talk bevat vrijwel geen getallen** — dat is geen zoekfout maar opzet; Nesky geeft
principes omdat *"what works for one game might not work for another"*. Gebruik hem dus als
**checklist**, niet als parameterbron. Hieronder de items die op een TPS met een
squad-commandolaag van toepassing zijn, met de ECLIPSE-stand. **[OFFICIEEL]**

| # | Fout | ECLIPSE |
|---|---|---|
| 4 | Een default-cameraafstand die de zichtlijn waarschijnlijk breekt | boom 300 in een dichte graybox — controleren |
| 5 | Obstakels de zichtlijn van opzij laten breken | niet afgevangen |
| 6 | De camera van een obstakel wegduwen terwijl de speler hem er juist heen draait | niet afgevangen |
| 7 | De speler de camera in een obstakel laten duwen | probe 12 is te klein — zie CAM-06 |
| 8 | Onafhankelijke krachten om de camera laten vechten | nu nog één systeem; bewaken zodra sprint-, dekking- en commando-camera's erbij komen |
| 10 | De camera door smalle pilaren laten snijden | niet afgevangen |
| 11 | Een heuvel als te vermijden muur interpreteren | niet afgevangen |
| 12 | De camera zijwaarts zwenken bij occluders van achteren | niet afgevangen |
| 13 | De near-clip-plane door de avatar laten snijden | risico bij ADS (arm ×0.55 → 165 uu) |
| 14 | Dezelfde cameraafstand voor alle hoeken gebruiken | ja, één afstand |
| 15 | Dezelfde FOV voor kikkerperspectief en normaal | ja, één FOV |
| **16** | **Pitch, afstand en FOV onafhankelijk verschuiven** | ADS koppelt ze correct; sprint-/commando-camera moet dat ook doen |
| 22 | Erop rekenen dat de speler de camera continu bestuurt | relevant voor de commando-modus |
| 23 | De camera-yaw met rust laten terwijl de speler rent | bewuste keuze; geen auto-follow |
| 27 | De regel van derden verkeerd toepassen | schouderoffset (0,55,65) — beoordelen |
| 29 | Volledig op procedurele camera's leunen | n.v.t. in graybox |
| 33 | Het eigen lichaam doelen laten occluderen | reëel risico met zware armor — zie CAM-06b |
| 34 | De speler camerabesturing geven en die dan afpakken | commando-modus doet dit; controleren |
| 35 | Direct een camera-hint toepassen nadat de speler zelf gedraaid heeft | n.v.t. |
| 37 | Geen omgekeerde besturing aanbieden | ✅ aanwezig (`bInvertLookY`) |
| **38** | **Op onbedoelde controllerinput reageren** | deadzone 0.08 is te laag — zie INP-01 |
| **39** | **Lineaire gevoeligheid gebruiken** | ✅ machtscurve aanwezig (exponent onbewezen) |
| 40 | De camerapivot te ver laten afdrijven | n.v.t. |
| 41 | Een te kleine FOV gebruiken | 80 TP / 64 ADS — beoordelen |
| **42** | **De FOV snel verschuiven** | nog geen FOV-kick; bij bouwen smoothen |
| **43** | **De camera overmatig schudden** | nog geen shake |
| **44** | **De camera met de loopcyclus laten stuiteren** | geen head-bob — goed zo |
| 45 | Transleren of roteren bij een sprong van de avatar | geen landings-dip; bij bouwen klein houden (JMP-10) |
| 46 | Snel naar een nieuwe camerapositie springen | commando-modus blendt in **0.0 s** — zie EXT-01 |
| **47** | **Pitchsnelheid vasthouden tot aan de limiet** | niet gedempt — zie CAM-07b |
| 50 | Een algemene "constraint solver" schrijven die de camera optimaliseert | niet doen |

---

## Bijlage D — Concrete voorstellenlijst

Alles uit dit document dat neerkomt op één waarde, op één rij. Dit is wat een audit
afvinkt. **Geen van deze wijzigingen is doorgevoerd** — dit document beschrijft, het
verandert niets.

| ID | Veld | Nu | Voorstel | Waarom |
|---|---|---|---|---|
| LOC-01 | `MaxAcceleration` | 1400 | **1800** | sprint komt nu pas na 464 ms op snelheid; Gears' responsiviteitsfix was óók acceleratie |
| LOC-03 | `bUseSeparateBrakingFriction` | false | **true** | anders is `BrakingFriction` niet los te tunen |
| LOC-03 | `BrakingFriction` | n.v.t. | **4.0** | sprint stopt in 200 ms / 57 cm i.p.v. 100 ms / 23 cm |
| LOC-04 | `BrakingFrictionFactor` | 2.0 | **1.0** | Epic's eigen comment noemt 2.0 een historisch artefact |
| LOC-14 | `GroundFriction` | ongezet (8.0) | **expliciet 8.0** | 90°-richtingwissel in 233 ms; nu een ongekozen default |
| LOC-11 | strafe-ratio | bestaat niet | **1.00** | Gears 5 groepeert lateraal met voorwaarts |
| LOC-12 | achteruit-ratio | bestaat niet | **0.85** | enige gepubliceerde waarde in het genre |
| CRC-01 | `CrouchSpeed` | 150 (0.36×) | **210 (0.50×)** | onder engine-default én beide Borderlands-generaties |
| CRC-02 | `CrouchedHalfHeight` | ongezet (40) | **60** | 40 uu is korter dan een hurkende mens |
| CRC-03 | hurk-overgangstijd | bestaat niet (instant) | **0.25 s** visueel | UE verandert de capsule in één frame |
| JMP-01 | `JumpZVelocity` | 500 | **450** | 128 cm apex is royaal voor een tactische shooter |
| JMP-03 | `AirControl` | 0.35 | **0.20** | tussen Borderlands' 0.11 en de template-0.35 |
| JMP-07 | coyote time | bestaat niet | **110 ms** | UE levert het niet; goedkoopste feel-winst die er is |
| JMP-08 | sprong-inputbuffer | bestaat niet | **150 ms** | idem |
| ROT-02 | `BodyRotationRateYaw` | 500 | **500 vrij / 720 gericht** | Epic's eigen shooter-variant gaat naar 600 |
| ROT-03 | turn-in-place | bestaat niet | **bouwen**, drempel 70° | stilstaand draaien laat nu een bevroren rug zien |
| CAM-06 | `CameraProbeSize` | 12 | **20–25** | 12 uu is te klein voor een 34–42 uu capsule |
| CAM-11 | sprint-camerastack | bestaat niet | **bouwen** | Gears haalt met 1.2× meer dan wij met 1.55× |
| ANI-09 | `MeshZOffset` vs capsule | −90 vs 88 | **gelijktrekken** | voeten staan 2 cm in de vloer |
| ANI-10 | capsulemaat | ongezet (34/88) | **expliciet kiezen** | alles hangt eraan: step-up, hurken, camerahoogte, dekking |
| TRV-01 | `MaxStepHeight` | ongezet (45) | **30** | 45 cm stapt geluidloos over dingen die een mantle horen te zijn |
| TRV-02 | `WalkableFloorAngle` | ongezet (44.77°) | **40°** | leesbaarder wat wel en niet beloopbaar is |
| TRV-05 | `PerchRadiusThreshold` | ongezet (0) | **10** | nu kun je op een centimeter geometrie balanceren |
| INP-01 | `StickDeadzone` / `MoveDeadzone` | 0.08 | **0.15 + schuif 0.05–0.30** | XInput zelf zegt 0.24/0.265, UE 0.20–0.25, Warframe 0.20; 0.08 is op één gemeten pad afgesteld |
| INP-03 | buitenste drempel | bestaat niet | **`UpperThreshold` 0.95** | sommige sticks halen de eenheidscirkel niet |
| INP-06 | `AdsLookMultiplier` | 0.35 | **0.745** | UE's eigen `UInputModifierFOVScaling`: `tan(32°)/tan(40°)`. Dit is geen smaak maar meetkunde |
| INP-14 | `AimAssistFloor` | 0.45 | **0.55–0.65** | 45% kijksnelheid is aan de sterke kant |
| INP-14c | aim-assist-zichtlijn | alleen hoek + afstand | **+ zichtlijntrace** | Gears én Destiny moesten hiervoor patchen |
| INP-14e | `AimAssistConeDegrees` | 4.0 vast | **4.0 / zoomfactor** | Destiny en Mass Effect verkleinen de cone allebei met de zoom |
| CAM-07 | `ViewPitchMax` | +70 | **+75 (asymmetrisch)** | Epic's eigen template gebruikt −70 / +80 |
| CAM-10 | FOV-kick bij sprint | bestaat niet | **+10°, gesmoothd, uitzetbaar** | Mass Effect LE1: 70 → 80 → 90 |
| FDB-10b | recoil-weergavekeuze | bestaat niet | **Camera / Reticle / Reduced** | DE's oplossing voor recoil-misselijkheid |
| EXT-01 | `EnterBlendSeconds` / `ExitBlendSeconds` | 0.0 / 0.0 | **0.15 / 0.10** | de commando-modus klapt er nu hard in |
| CAM-08 | ⚠ FOV | code 80, GDD 90 | **80, GDD corrigeren** | Gears 5 verscheept ook 80; change management vereist |
| CAM-13 | ⚠ commando-pullback | spec 15%, code +73% | **één waarheid kiezen** | de character leest `CameraPullbackPercent` nooit |
| INP-13 | ⚠ `CONTROLLER_FEEL.md` | wijkt af van de code | **doc bijwerken** | vijf waarden lopen uiteen |

---

## Bijlage E — Bronnen

**[ENGINE]** — alle engine-claims zijn gelezen uit de lokale installatie
`C:\Program Files\Epic Games\UE_5.8\Engine\`. Bestand en regelnummer staan per claim in
Bijlage A. Reproduceerbaar zonder internet.

**[OFFICIEEL] — Gears of War / The Coalition**

- Cliff Bleszinski, GDC 2007, roadie run = 1.2× — engadget.com/2007-03-09-gdc-07-cliffyb-on-designing-gears-of-war.html
- David Bollo, GDC 2018, *Inertialization: High Performance Animation Transitions in Gears of War* — cdn.gearsofwar.com/thecoalition/publications/ (PDF)
- David Bollo, SIGGRAPH 2017, *High Performance Animation in Gears of War 4* — cdn.gearsofwar.com/thecoalition/publications/ (PDF, DOI 10.1145/3084363.3085069)
- S. Dickinson, GDC 2017, *Motion Warping in Gears of War 4* — gdcvault.com/play/1024219
- Gears 5 Title Update 3 (dec 2019) patchnotes, bewegingscijfers en aim-assist-terminologie — e-arena.gr, xboxdynasty.de, allpatchnotes.com. ⚠ De originele gearsofwar.com-pagina is dood en archive.org was onbereikbaar; deze claims steunen op secundaire hosts en zijn daarmee *medium* betrouwbaar.
- Gears 5 toegankelijkheidsopties — gamedeveloper.com
- Matt Searcy over Gears E-Day wall bouncing — kotaku.com

**[GEMETEN] — Borderlands**

- BL2 bewegingswaarden: `BL2 Movement Speed Cheats.blcm` (stock-profiel) — github.com/BLCM/BLCMods
- BL3 / Wonderlands bewegingswaarden: `movement_speed_cheats_normal.bl3hotfix` en `gen_movement_speed_cheats.py` — github.com/BLCM/bl3mods, github.com/apocalyptech/wlmods
- BL2 sprint-attribuut: `SprintAdjuster` — github.com/plu5/p-borderlands
- Feedback-/schadegetalsysteem: `Better Damage Feedback` (ZetaDæmon) — bl-sdk.github.io/willow2-mod-db
- Nauwkeurigheidsmodel en -tabel, crit-formule, aim assist — borderlands.fandom.com (Accuracy, Critical hit, Aim Assist)
- BL2 "reduced gravity" — kb.speeddemosarchive.com/Borderlands_2

**[OFFICIEEL] — Mass Effect, Warframe, The Division**

- Mass Effect LE1 verscheepte FOV-ladder (`SFXCameraMode_*`) en `BIOWeapon.ini` AimModes — getranscribeerd door modders op nexusmods.com (mods 497 en 2963)
- BioWare, *Gameplay Calibrations* (ME:LE) — ea.com/ea-play/news/gameplay-calibrations
- Christina Norman, GDC, *Refining the Real-Time Combat in Mass Effect 2* — gamedeveloper.com
- Digital Extremes: dev-workshop over screen shake en recoil-types (Devstream 189, hotfix 40.0.3), gamepad-documentatie (warframe.com/gamepad), zichtlijn-FOV-regel, Melee 3.0-workshop — forums.warframe.com, wiki.warframe.com
- Ubisoft-support: aim assist, "Additional Field of Vision", Motion Sickness Mode — ubisoft.com/help

**[OFFICIEEL] — vaktheorie**

- John Nesky, GDC 2014, *50 Game Camera Mistakes* — zie Bijlage C2. **Bevat bewust geen getallen.**
- Nick Weihs (Insomniac), GDC 2013, *Techniques for Building Aim Assist in Console Shooters* — gdcvault.com/play/1017942. ⚠ Alleen video; geen transcript of slides publiek, dus alleen de taxonomie is bruikbaar.
- Jan Willem Nijman (Vlambeer), *The Art of Screenshake* — archive.org
- Josh Sutphin, *Doing Thumbstick Dead Zones Right* — joshsutphin.com; Ryan Juckett (Hypersect), *Interpreting Analog Sticks* — blog.hypersect.com
- Daniel Holden, *Spring-It-On* (halveringstijd-parametrisatie van dempers) — theorangeduck.com
- Microsoft XInput `XINPUT_GAMEPAD_*_THUMB_DEADZONE` — learn.microsoft.com
- NVIDIA Reflex-latentieonderzoek — developer.nvidia.com; Liu & Claypool, ACM MMSys 2023 — dl.acm.org/doi/10.1145/3587819.3590977
- Unity Cinemachine 3.1 `CinemachineThirdPersonFollow` — docs.unity3d.com
- *Game AI Pro* hfst. 47, *Tips and Tricks for a Robust Third-Person Camera System* — gameaipro.com

**[GEMETEN] — Gears, community**

- Roadie-run-camerastack — gearsofwar.fandom.com/wiki/Roadie_Run, giantbomb.com, gamedeveloper.com ("Camera Evolution in Third-Person Games")
- Wall bounce-invoer, schouderwissel-afwezigheid, FOV-standen, bullet magnetism — steamcommunity.com-discussies (Gears 5, Gears Reloaded)

**Eerder werk in deze repo dat hier op aansluit:** `phase0/CONTROLLER_FEEL.md` (deadzones,
curves, gevoeligheid, aim assist — met gemeten Halo-/Destiny-/CoD-waarden),
`phase0/controller_kalibratie.json` (stickdrift-meting),
`phase0/graybox_feel_targets.md` (LOCKED tuning-contract),
`phase0/FEEL_GAUNTLET_P2-02.md` (commando-modus-verdictscript).

---

*Onderhoud: dit document is een referentie, geen contract. `graybox_feel_targets.md`
blijft de LOCKED tuning-autoriteit en de GDD blijft daarboven staan. Wijkt een aanbeveling
hier af van een van die twee, dan is dat een change-management-voorstel — met een ⚠
gemarkeerd — en nooit een stille correctie.*
