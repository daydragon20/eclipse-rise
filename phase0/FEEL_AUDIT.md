# Feel-audit van de huidige build

*Stap 2 van de autonome feel-opdracht (owner, 2026-07-25). De referentie-checklist
staat in `FEEL_REFERENTIE.md`; dit document zet de HUIDIGE build daar naast.*

Status per item: **OK** (gedraagt zich als de conventie) · **AFWIJKEND** (bestaat,
verkeerd afgesteld — met de gemeten waarde) · **ONTBREEKT** (bestaat niet in het
project) · **N.V.T.** (bewust anders, met reden).

Alles hieronder is uit de code gelezen of gemeten, niet uit het geheugen.

## EINDSTAND per gebied (nacht 25→26 juli 2026)

Wie hier morgen begint, hoeft alleen deze tabel te lezen. Elk gebied is óf gemeten
en geland, óf gemeten en bewust doorgeschoven, óf niet gebouwd — en de laatste
kolom zegt waar het bewijs staat.

| Gebied | Stand | Bewijs |
|---|---|---|
| Locomotie (snelheid, aanloop, remmen, richting) | **gemeten en geland** | `Eclipse.Feel.Layer2.InjectedLocomotion*` — aanloop 0,300 s · stoppen 0,150 s / 26,6 cm · achteruit 0,85× |
| Sprint in/uit | **gemeten, asymmetrie is owner-keuze** | 0,150 s erin, 0,042 s eruit — `SprintRampsInsteadOfSnapping` |
| Hurken | **gemeten en geland** | 150 cm/s en 124 cm hoog (was de engine-default 80) — `CrouchCostsSpeedAndBuysHeight` |
| Sprong (hoogte, airtime, vergevingsvensters) | **gemeten en geland** | 127,5 cm · 1,008 s · coyote 0,110 s · buffer 0,150 s |
| Luchtsturing | **gemeten, hoeveelheid is owner-keuze** | 213 cm met aanloop (287 vanuit stilstand — daar verdubbelt UE hem) |
| Traversal (stap, perch) | **gemeten en geland** | stoeprand 19,98 cm · kniehoogte 0,00 cm |
| Kijken (snelheid, deadzone, demping) | **gemeten en geland** | 1,500 s per 360° · stick 0,05 geeft niets · demping 180 → 52,8 gr/s |
| Kijken tijdens mikken | **gemeten, factor is owner-keuze** | 4,29 s per 360° = 2,86× trager |
| Camera-afstand bij snelheid (S1) | **gemeten en geland** | 8,4% → 0,00% tussen rennen en sprinten |
| Camera-overgangen (1e persoon, ADS, Command Mode) | **was DOOD, nu geland** | de blend draaide nooit; nu 300→0, 300→165, 300→520 |
| Pitch-limieten | **gemeten — de audit had het mis** | omlaag gebeurt niets; omhoog 300 → 84 cm vanaf ~+25° |
| Rotatie lichaam | **gemeten en geland** | 500 gr/s, 180° in 0,400 s |
| Turn-in-place | **niet gebouwd** | vraagt een draai-animatie (owner, §4 punt 7) |
| Wapen (tempo, bereik, kopschot) | **gemeten; twee getallen kloppen niet** | 14 schoten/2 s ✓ · bereik 4700 i.p.v. 5000 · kopschot doet niets |
| §8 FEEDBACK (shake, recoil, hitmarkers) | **bestaat niet** | grootste openstaande post (owner, §4 punt 8) |

## Wat §8 FEEDBACK moet weten vóór het gebouwd wordt

Vier dingen die deze nacht boven kwamen en die de bouwer van de feedback-laag gaan
raken. Geen ontwerpkeuzes — feiten die je liever vooraf kent dan halverwege.

1. **`bInheritRoll` staat aan op de spring arm.** Vandaag onschadelijk, want de
   control rotation heeft geen roll. Zodra camera-shake roll gebruikt, roteert de
   **hele boom** mee in plaats van alleen het beeld. Zet die vlag uit vóór de
   eerste shake, niet erna.
2. **De camera-blend werkt sinds 26-07 pas echt** (`bCanEverTick` stond uit, zie
   §1 van HANDOFF). Hij loopt op constante SNELHEID, niet op constante tijd, en
   hij erft de tijddilatatie: in Command Mode duurt dezelfde beweging ruim drie
   keer zo lang. Een shake die daar bovenop komt, komt dus in een systeem dat al
   aan de camera trekt.
3. **Richten en wapenbereik hangen aan de camera.** De hitscan vertrekt uit
   `GetPlayerViewPoint()`, dus élke camerabeweging verplaatst je kogel. Gemeten
   gevolg dat er nu al is: mikken trekt de boom in van 300 naar 165 cm en verlengt
   daarmee je effectieve bereik met ~135 cm. Een shake die de camera beweegt,
   beweegt je schot mee — dat is een ontwerpkeuze, maar wel eentje die je bewust
   moet maken.
4. **Er is geen enkel event om een hitmarker aan op te hangen.** `Fire()` geeft een
   bool terug die de aanroeper weggooit, en er is geen `Event.Combat.*`. Een
   hitmarker vraagt dus eerst een feit op de bus, en dat is meteen het haakje voor
   geluid en haptiek (de audio-subsystem is al een pure bus-consument).

## De vondst die het meeste verklaart

Van het `CharacterMovementComponent` werden **vier** dingen gezet: `MaxWalkSpeed`,
`MaxWalkSpeedCrouched`, `bOrientRotationToMovement` en `bCanCrouch`. Al het overige
stond op de **engine-default**. Die defaults zijn niet voor een third-person shooter
bedoeld, en het beste bewijs daarvoor is dat Epic's eigen `TP_ThirdPerson`-template
ze allemaal overschrijft. Samen zijn ze de reden dat lopen als schaatsen leest.

## LOCOMOTIE

| Item | Status | Was | Wordt | Bron |
|---|---|---|---|---|
| Acceleratie | AFWIJKEND | 2048 (engine-default) = direct op topsnelheid | 1400 (~0,30 s aanloop) | [REDENERING] binnen engine-band |
| Afremmen (lopen) | AFWIJKEND | 2048 (default) | 2000 | [ENGINE] template |
| Min. analoge loopsnelheid | AFWIJKEND | 0 (default) — lichte stickuitslag glijdt vooruit in idle-pose | 20 | [ENGINE] template |
| Snelheidstrappen | OK | walk 180 / run 420 / sprint 650 / crouch 150, uit data | — | project-eigen feel targets |
| Sprint in/uitstap | **GEMETEN — half gefixt** | in: 0,150 s vloeiende oploop (was inderdaad hard toen acceleratie nog 2048 was). uit: 0,042 s, dus 3,5x sneller terug dan erin | asymmetrie is een owner-keuze | meting |
| Strafen / achteruit | ONTBREEKT | geen aparte snelheden; achteruit is even snel als vooruit | eigen stap | conventie: 60-80% |

## ROTATIE

| Item | Status | Was | Wordt | Bron |
|---|---|---|---|---|
| Lichaam volgt beweging | OK | `bOrientRotationToMovement = true` | — | [ENGINE] template |
| Draaisnelheid lichaam | AFWIJKEND | 360 gr/s (default) — een omkering kost een halve seconde draaien terwijl je al schuift | 500 | [ENGINE] template |
| `bUseControllerRotationYaw` | OK | false | — | [ENGINE] elke Epic-template |
| Turn-in-place bij stilstand | ONTBREEKT | het lichaam draait niet als je alleen de camera beweegt | eigen stap (vraagt animatie) | — |
| Strafe-modus bij mikken | ONTBREEKT | lichaam blijft op looprichting tijdens ADS | eigen stap; vraagt aim-offset, anders klapt de pose | [ENGINE] `bUseControllerDesiredRotation` |

## CAMERA

| Item | Status | Waarde | Bron |
|---|---|---|---|
| Boomlengte | OK | 300 (3e), 0 (1e), 520 (Command Mode) | conventie 300-400 |
| Schouderoffset | OK | (0, 55, 65) | owner-spec |
| Lag | OK | 12 | owner-spec |
| Botsing met muren | OK | probe 12, `bDoCollisionTest` aan | [ENGINE] |
| FOV | OK | 80 (3e) / 90 (1e) / ×0,80 bij ADS | owner-spec |
| Pitch-limieten | **GEMETEN — deze rij was FOUT** | −70/+70. De oorspronkelijke redenering hieronder klopte van richting niet; zie de meting eronder | geen wijziging op −, wel een keuze op + (owner) | meting |
| Shake | ONTBREEKT | geen enkele camera-shake in het project | eigen stap | — |
| Gedrag bij sprint | ONTBREEKT | camera reageert niet op sprint (geen FOV-punch, geen lag-verandering) | eigen stap | conventie: lichte FOV-toename |

## SPRONG

| Item | Status | Was | Wordt | Bron |
|---|---|---|---|---|
| Sprongkracht | AFWIJKEND | 420 (default) | 500 | [ENGINE] template |
| Luchtcontrole | **GEMETEN — geland** | 0,05 (default) was een sprong op rails | 0,35 = **287 cm sturing per sprong** (controle-sprong: 0,00 cm drift) | [ENGINE] template + meting |
| Valremming | AFWIJKEND | **0** (default) — een val remt nooit | 1500 | [ENGINE] template |
| Landingsanimatie | ONTBREEKT | geen landings-take, geen knikje | eigen stap | — |

### CAM-05 pitch-limieten: de redenering klopte niet, de meting wel

Deze rij stond als **AFWIJKEND** met als reden: *"bij −70 duikt de boom in de
grond en trekt de collision hem naar binnen"*, met −55/+70 als doel. Dat was een
redenering uit code lezen, en de **richting klopt niet**. Een spring arm steekt
tégen de kijkrichting in: omlaag kijken tilt de camera juist **omhoog**, weg van
de vloer.

Gemeten over het hele bereik (`Eclipse.Feel.Camera.WhereTheBoomCollapsesAcrossThePitchRange`),
met de daadwerkelijke camera-tot-pawn-afstand waarin arm, lag én collision zitten:

| pitch | afstand | van de boom |
|---|---|---|
| −70 t/m +20 | 311,85 cm | 104% — vlak, geen enkele inpull |
| +30 | 233,45 cm | 78% |
| +40 | 152,93 cm | 51% |
| +50 | 116,34 cm | 39% |
| +60 | 96,24 cm | 32% |
| **+70** | **84,24 cm** | **28%** |

**Omlaag kijken doet niets. Omhoog kijken vanaf ongeveer +25 duwt de camera in
je rug**, tot 216 cm inpull op de limiet.

Dat het de **vloer** is en niet het personage zelf, is apart gemeten: dezelfde
sweep met de pawn los van de grond komt nergens onder 300,00 cm — nul inpull over
het hele bereik. (Die controle deugde in eerste opzet niet: de pawn 1500 cm hoog
zetten en dan 19 stappen van 0,3 s aflopen betekent dat hij na 1,75 s gewoon weer
op de vloer staat, en dan meet de "controle" hetzelfde als het origineel. Met
vliegen aan klopt hij.)

**Gevolg: `ViewPitchMin` op −55 zetten lost niets op** — er valt daar niets te
repareren. De wijziging die de audit voorstelde is dus **niet** doorgevoerd; dat
zou een verandering op een weerlegde aanname zijn geweest.

**Wat er wél ligt is een keuze, en die is van de owner** (staat in zijn lijstje):
omhoog kijken kost je het derde-persoons-kader. Drie richtingen, geen ervan
duidelijk beter zonder zijn oordeel: `ViewPitchMax` omlaag naar ~+45 (behoudt het
kader, kost je omhoog richten), de socket hoger of de boom korter bij positieve
pitch (behoudt beide, vraagt bouwwerk), of laten staan (elke derde-persoons-game
met een boom van 3 m heeft dit in enige mate).


### LOC-04 sprint in/uitstap: erin is gerepareerd, eruit is 3,5x sneller

Deze rij zei "harde omschakeling van MaxWalkSpeed, geen overgang". Dat klopte
toen hij geschreven werd — met `MaxAcceleration` op de engine-default 2048 zat er
niets tussen. De acceleratiefix van dezelfde nacht heeft de instap onderweg
opgelost, en dat was tot nu toe nergens nagemeten.

Gemeten (`Eclipse.Feel.Layer2.SprintRampsInsteadOfSnapping`), vooruit blijven
duwen in beide richtingen:

| richting | duur | verloop |
|---|---|---|
| rennen → sprint | **0,150 s** | 420 → 455 → 490 → 525 → 560 → 583 → 650, gelijkmatig over 18 ticks |
| sprint → rennen | **0,042 s** | 650 → 539 → 504 → 471 → 438 → 420, in 5 ticks |

**Erin is een echte oploop. Eruit is 3,5x sneller** — en dat verschil is geen
instelling die iemand gekozen heeft: versnellen loopt via `MaxAcceleration`,
terugzakken via de grondwrijving, en die twee zijn nooit op elkaar afgestemd.

Niets aan gewijzigd. Snel uit de sprint vallen is verdedigbaar (je wilt je
controle terug), maar 3,5x is een gevolg van twee losse mechanismen en geen
keuze, dus het hoort langs de owner. Staat in zijn lijstje.


### JMP-05 luchtcontrole: 287 cm per sprong, en dat is veel

Laag 1 controleerde al dat 0,35 op het component aankomt, maar een waarde die
aankomt is nog geen gedrag. Gemeten
(`Eclipse.Feel.Layer2.AirControlSteersTheJump`), vanuit stilstand zodat er geen
horizontale beginsnelheid meespeelt:

| sprong | verplaatsing |
|---|---|
| zonder input (controle) | **0,00 cm** |
| met zijwaartse sturing | **287,01 cm** |

De controle-sprong op exact nul is wat de meting geldig maakt: zonder die tweede
sprong weet je niet of je luchtcontrole meet of drift.

**Ter maat:** de airtime is 1,008 s en de rensnelheid 420 cm/s, dus je kunt
tijdens één sprong bijna **70% van een volle renseconde** opzij komen. Dat is
royaal voor een aardse shooter — Gears geeft je vrijwel niets, Halo juist veel.
De waarde komt uit Epic's eigen third-person-template en is dus verdedigbaar,
maar hij is nooit tégen dit spel afgewogen.

Niets aan gewijzigd: de assert is een ondergrens ("niet op rails"), geen
streefwaarde. Hoeveel sturing prettig is, is smaak — dat staat in het lijstje.


## WAPEN, FEEDBACK, ANIMATIE, TRAVERSAL — de ONTBREEKT-lijst

Een gerichte zoektocht door de hele module vond **geen enkele** referentie naar:

`Recoil` · `MuzzleFlash` · `Tracer` · `HitMarker` · `DamageIndicator` ·
`CameraShake` · `ForceFeedback`/haptiek · `FootIK` · `AimOffset` · `MaxStepHeight`
(engine-default 45 blijft staan) · `WalkableFloorAngle` (default 44 blijft staan)

Dat is geen defectenlijst maar een **scope-lijst**: dit zijn systemen die nooit
gebouwd zijn, niet instellingen die verkeerd staan. Ze horen in de rapportage thuis
omdat de owner vroeg om ook te horen wat hij zelf nooit zou opmerken — en het
verschil tussen "verkeerd afgesteld" en "bestaat niet" bepaalt of iets een tuning-
ronde is of een bouwopdracht.

Wapenhoudingen bestaan wél (hip/ready), en de owner meldt dat de overgang te hard
klapt: de houding klopt, de blendtijd niet. Dat is AFWIJKEND, geen ONTBREEKT.

## Wat hiervan al geschreven is

De hele LOCOMOTIE- en SPRONG-kolom plus de pitch-limiet staan in de geparkeerde
patch, met per waarde de engine-default en de herkomst in de code. Ze wachten op een
vrij build-slot.

## WAT HET HARNAS GEMETEN HEEFT (nacht 2026-07-25/26)

De testharnas (laag 1 + laag 2, `Eclipse/Source/Eclipse/Tests/EclipseFeelHarness*`)
draait nu mee in de suite. Laag 1 leest de daadwerkelijk toegepaste waarden van
het movement component en de camera; laag 2 injecteert input via Enhanced Input
en meet over tijd. Dit zijn de getallen, niet de verwachtingen:

| Meting | Gemeten | Referentie | Status |
|---|---|---|---|
| Tijd tot topsnelheid (rennen) | **0,300 s** | 420 / 1400 = 0,300 s | OK |
| Stoptijd vanaf rennen | **0,083 s** | Bijlage B: 0,083 s | arcade-kort — **in fase 3 hieronder gewijzigd naar 0,150 s** |
| Glijafstand vanaf rennen | **12,0 cm** | Bijlage B: 13 cm | idem — **fase 3: 26,6 cm** |
| 180-omkering (snelheid weer op top) | **0,400 s** | — | OK |
| 180-omkering: lichaam gedraaid | **180 gr** | RotationRate 500 gr/s | OK |
| Springhoogte | **127,5 cm** | v²/2g = 128 cm | OK |
| Airtime | **1,008 s** | 2v/g = 1,020 s | OK |
| Seconden per 360 kijken | **1,500 s** (was **0,600 s**) | 360 / 240 | **WAS AFWIJKEND — gefixt** |
| Kantelsnelheid | **+180 gr/s** (omhoog) | StickPitchSpeed 180 | OK, teken vastgepind |
| Stick 0,05 → verplaatsing | **0,00 cm** | 0 | OK |
| Stick 0,05 → camera-draai | **0,00 gr** | 0 | OK |
| Stick 0,45 → verplaatsing | **144,3 cm/s** | > 0 | OK |

### S1 — "personage schaalt met snelheid": OORZAAK GEVONDEN EN GEFIXT

Van de vier kandidaten bewegen er drie **niet** mee met de snelheid: mesh-schaal
(1,000), boomlengte (300,0) en FOV (80,0) zijn identiek bij stilstand, rennen en
sprinten. Wat wél meebewoog was de **gemeten camera-tot-pawn-afstand**:

| | camera→pawn | schijnbare hoogte |
|---|---|---|
| stilstand | 312,07 cm | 31,50 gr |
| rennen (420) | **342,26 cm** | **28,84 gr** |

Dat is **8,4% kleiner van gaan rennen**. De oorzaak is `bEnableCameraLag`: de
spring arm laat zijn oorsprong achterlopen, en die achterstand is in stationaire
toestand exact `snelheid / CameraLagSpeed` — 35 cm bij rennen, 54 cm bij
sprinten. Het is de enige term in de hele camerarig die met de snelheid
meebeweegt.

**Let op de richting.** Gemeten wordt het personage KLEINER naarmate je sneller
loopt; de owner meldde het omgekeerde. Eén defect, twee lezingen — wat vaststaat
is de koppeling en de grootte. Het is goed mogelijk dat wat als "nagenoeg
onzichtbaar bij langzaam lopen" gelezen werd iets anders is (kadrering, of het
lichaam dat bij lage snelheid dichtbij en deels buiten beeld staat); dat blijft
open tot de owner de gefixte build speelt.

**Fix:** `CameraLagMaxDistance = 6` uu. De lag blijft bestaan — die is er om
kleine schokkerige correcties glad te strijken — maar zit boven ~72 cm/s op zijn
klem en is daarmee constant. Na de fix: rennen → sprinten **0,00%**, stilstand →
sprinten **1,67%** (5,6 cm). Regressietest:
`Eclipse.Feel.Camera.ApparentSizeDoesNotTrackSpeed`, drempel 2%.

### Een tweede defect dat het harnas onderweg vond: kijken liep 2,5x te snel

`DA_CharacterTuning` zegt 240 gr/s en het testgids-paneel toont "1,50 s per 360".
Gemeten: **600 gr/s, 0,60 s per 360**. Oorzaak: `bEnableLegacyInputScales` stond
aan (engine-default), waardoor `AddYawInput`/`AddPitchInput` nog vermenigvuldigen
met `InputYawScale = 2.5` en `InputPitchScale = -2.5` uit
`Engine/Config/BaseGame.ini`. Erger dan de factor is het teken: de pitch-schaal
is negatief, dus de handler compenseerde een verborgen omkering en "invert Y" was
niet te beredeneren.

Uitgezet in `Config/DefaultInput.ini`; het pitch-teken zit nu in de handler.
Netto: de kijkrichting is ongewijzigd, de kijksnelheid is 2,5x lager en gelijk
aan wat de tuning zegt, en de muis blijft even snel (`MouseLookScale` 1,0 → 2,5,
een kale schaal zonder eenheid). **Dit is een merkbare gedragswijziging** — als
240 gr/s te traag blijkt, is dat één getal in `DA_CharacterTuning`.

## FASE 3 — afgewerkt op impact-volgorde, elk item gemeten

Volgorde gekozen op "wat maakt het spelen het meest onnatuurlijk", en elk item
heeft een meetbaar criterium in het harnas. Vier geland, de rest staat onderaan
met een reden.

| Item | Was | Wordt | Gemeten na de wijziging | Bron |
|---|---|---|---|---|
| LOC-03 remmen | stop in **0,083 s / 12,0 cm** vanaf rennen | `bUseSeparateBrakingFriction` aan, `BrakingFriction` 4.0 | **0,150 s / 26,6 cm** | [ENGINE] + Bijlage B (exact nagerekend) |
| LOC-04 `BrakingFrictionFactor` | 2.0 (engine-default) | **1.0** | zie boven | [ENGINE] Epic's eigen commentaar: *"Historical value, 1 would be more appropriate"* |
| LOC-14 `GroundFriction` | ongezet (toevallig 8.0) | **expliciet 8.0** | 180-omkering in **0,400 s** | [ENGINE] `CalcVelocity` |
| LOC-11 strafe-ratio | bestond niet (1.00 impliciet) | **1.00 expliciet** | zijwaarts **420 cm/s** = 1,000× | [OFFICIEEL] Gears 5 TU3 |
| LOC-12 achteruit-ratio | bestond niet — **achteruit was even snel als vooruit rennen** | **0.85** | achteruit **357 cm/s** = 0,850× | [OFFICIEEL] Gears 5 TU3 (na een gemeten 0.739 die te sloom bleek) |
| CAM-06 camera-probe | 12 uu | **20 uu** | probe ≥ capsule-radius | referentieband 20–25 |

**Waarom remmen bovenaan staat.** Het harnas mat een stop in 83 ms over 12 cm.
Dat is geen "responsief", dat is gewichtloos — dezelfde familie als het schaatsen
dat de vorige ronde opleverde, alleen aan de andere kant van de beweging. De drie
remvelden zitten in elkaars weg als je er maar één aanraakt: zonder
`bUseSeparateBrakingFriction` is `BrakingFriction` dood gewicht, en met
`BrakingFrictionFactor` op 2.0 rem je twee keer zo hard als het veld suggereert.

**Achteruitlopen was even snel als vooruit rennen.** UE kent geen ingebouwde
richtingsstraf; `GetMaxSpeed()` is de enige plek waar dat kan, dus daar staat nu
`UEclipseCharacterMovementComponent`. De ratio schaalt vloeiend tussen de drie
ankers in plaats van in drie vakjes — een klif op precies 90 graden voelt als een
hapering midden in een bocht. Dezelfde regel geldt voor speler, squad én vijand
(GDD 8.3: dezelfde wapens, dezelfde regels); de speelronde bewijst dat de AI er
niet door breekt.

**De twee sprong-vergevingsvensters (JMP-07/08) zijn er ook.** Ze bestaan in UE
geen van beide, en allebei repareren ze een moment waarop de speler het goede
deed en de game nee zei — dat heet dan "de besturing reageert niet".

| Venster | Waarde | Wat het repareert |
|---|---|---|
| Coyote time | **110 ms** | Van een rand aflopen en een fractie te laat drukken. `ACharacter::JumpIsAllowedInternal` eist `JumpCurrentCount + 1 < JumpMaxCount` zodra je valt, en met `JumpMaxCount = 1` is dat altijd onwaar — je krijgt geen sprong maar een val. |
| Sprong-inputbuffer | **150 ms** | Drukken vlak vóór de landing. Zonder buffer moet je op precies de landingsframe drukken; elke druk daarvoor is weg. |

Twee valkuilen die de meting blootlegde en die in de code staan omdat ze
niet-vanzelfsprekend zijn: (1) `CheckJumpInput` hoogt `JumpCurrentCount` op
**vóórdat** het `CanJump()` vraagt, dus een coyote-check op die teller ziet altijd
1 waar 0 staat — vandaar `JumpCurrentCountPreJump`; (2) `Landed()` draait binnen
`ProcessLanded`, en `OnMovementModeChanged` zet daarna nog `ResetJumpState()`, die
`bPressedJump` weer wist — een gebufferde sprong die je in `Landed()` afvuurt,
verdwijnt dus stil.

### TRAVERSAL (TRV-01/02/05) — en een tegenspraak in de referentie zelf

**Let op bij het naslaan: Bijlage D van `FEEL_REFERENTIE.md` is op twee van deze
rijen VEROUDERD.** Bijlage D noemt `MaxStepHeight` 30 en `WalkableFloorAngle` 40;
de herziene §10 van hetzélfde document komt op 35 uit en laat de hellingshoek
juist staan, allebei mét bronnen en allebei expliciet gemarkeerd als correctie op
een eerder advies. De body is de latere en beter onderbouwde tekst en wint.

| Item | Was | Wordt | Waarom |
|---|---|---|---|
| `MaxStepHeight` | 45 (engine-default) | **35** | 45 is geen slordigheid — als percentage van de personagehoogte (25,6%) is het de norm van de hele id/Valve/Epic-lijn. Maar de GDD voert vault en mantle op als eigen verbs, dus kniehoge dekking hoort een vault te zijn en geen geruisloze stap. Ontwerpkeuze die uit de GDD volgt. |
| `WalkableFloorAngle` | 44,77 | **44,77 — ONGEWIJZIGD** | Vier onafhankelijke bronnen komen op ~45° uit (Quake III 45,573°, Half-Life dezelfde test, Source 45,573°, UE 44,77°). Het eerdere voorstel van 40 was een smaakoordeel daartegen in. Nu gepind in laag 1 zodat een latere "opruiming" hem niet stil verzet. |
| `PerchRadiusThreshold` | 0 (engine-default) | **10** | Met 0 kun je op een centimeter geometrie balanceren. |

**Gemeten gedrag, want de maat zelf zegt niets:** een blok van 20 cm levert
**+19,98 cm** hoogtewinst (de stoeprand blijft een stap), een blok van 50 cm
levert **0,00 cm** en het personage stopt er na 116 cm tegenaan (kniehoogte is nu
een hindernis). Dat is de bedoeling van de wijziging, en het staat als assert.

De meting kostte onderweg twee eigen fouten die het vermelden waard zijn omdat ze
allebei op een gameplay-defect leken: het testblok was eerst 600 uu diep en stond
dus bovenop het personage (0 cm op élke hoogte), en daarna mat ik de EINDhoogte
in plaats van de hoogste stand — het personage stapte over het blokje van 20 cm
heen en er aan de andere kant weer af, dus de eindhoogte was gewoon de vloer.

### CAM-07b — de camera remt af tegen de pitch-limiet

Nesky's camerafout **#47**: *"maintaining pitch speed until hitting the pitch
limit"*. De camera hoort tégen de klem aan af te remmen, niet er met volle
snelheid tegenaan te slaan. Kost bijna niets, en het verschil tussen "de camera
stopt" en "de camera knalt tegen een muur" is direct voelbaar.

De demping loopt naar een **bodem** (15%) en niet naar nul: bij nul wordt de
limiet asymptotisch benaderd en dus nooit gehaald, en een camera die net niet
omhoog wil kijken is een erger defect dan de klap.

**Gemeten:** 180 gr/s midden in het bereik tegen **52,8 gr/s** op twee graden van
de limiet, en de limiet wordt nog steeds exact bereikt (70,00°).

*Voetnoot bij de meting, want de eerste versie was waardeloos:* die mat de
kijksnelheid terwijl de camera al ÓP de klem stond en las 0 gr/s. Dat is trivial
waar — ook zonder demping — dus die assert zou ook geslaagd zijn als de wijziging
er niet was. Nu wordt er binnen de dempband gemeten. Een zwakke assert is erger
dan geen assert: hij geeft dekking die er niet is.

### Drie plekken waar de documentatie niet klopte met de code

De audit vindt niet alleen verkeerde waarden maar ook verkeerde *beschrijvingen*,
en die zijn gevaarlijker: ze sturen de volgende ronde de verkeerde kant op.

1. **`AdsLookMultiplier`** — het commentaar redeneerde zich naar **0.60** en het
   veld staat op **0.35**. Een tuningwaarde waarvan de eigen onderbouwing een
   ánder getal noemt is dezelfde soort leugen als het paneel dat "1,50 s per 360"
   toonde terwijl de game 0,60 s draaide. **Het commentaar is gecorrigeerd, de
   waarde niet aangeraakt:** 0.35 ligt binnen de band die spellen daadwerkelijk
   verschepen (CoD 0.346, Apex-pro's 0.30–0.40) en is verdedigbaar; 0.60 is dat
   óók. Welke bij ónze milde zoom hoort is smaak → ownerlijst.

2. **ANI-09 in Bijlage D is FOUT, en dit is nu gemeten.** Bijlage D zegt dat
   `MeshZOffset` −90 tegen een capsule van 88 "de voeten 2 cm in de vloer" zet en
   dat de twee gelijkgetrokken moeten worden. UE laat een personage echter niet
   ópde vloer rusten maar er ~2,15 cm boven (MIN/MAX_FLOOR_DIST), dus −90
   compenseert precies dát. **Gemeten in de speelronde: de meshwortel staat
   0,15 cm boven de grond.** Op −88 zouden de voeten juist 2 cm in de lucht
   hangen. Vastgepind, zodat niemand dit "repareert".

3. **TRV-01/TRV-02 in Bijlage D zijn verouderd** ten opzichte van de herziene §10
   van hetzelfde document — zie de traversal-tabel hierboven.

**Nog niet gedaan, met reden** — dit zijn bouwopdrachten, geen tuningrondes:
turn-in-place (ROT-03, vraagt een draai-animatie of je krijgt voetslip),
sprint-camerastack (CAM-11), camera-shake en recoil/hitmarkers (§8 FEEDBACK
bestaat volledig niet). Ze staan met een aanbeveling in HANDOFF.md.

## INPUT — hold versus toggle (owner-eis 2026-07-25)

| Item | Status | Nu | Wordt | Reden |
|---|---|---|---|---|
| Sprint op L3 | **AFWIJKEND** | hold: `SprintAction` op `Gamepad_LeftThumbstick`, gebonden op Triggered + Completed | **toggle** | Een stick ingedrukt houden terwijl je er tegelijk mee stuurt is onhandig, en het is niet de conventie |
| Sprint op Shift | OK | hold | hold | Op toetsenbord is hold juist wél de conventie |

**Gewenst gedrag van de toggle**, zoals Borderlands / Gears / The Division: één klik op
L3 start sprint, en die blijft aan tot de speler (a) ophoudt met vooruit duwen, (b)
mikt, (c) vuurt, of (d) nogmaals L3 drukt.

**Twee devices mogen verschillen en dat is hier het punt.** De neiging om beide
invoerapparaten hetzelfde te laten doen is precies wat dit item fout maakt: een
sprintknop die je moet vasthouden botst op een pad met de stick waarmee je stuurt, en
op een toetsenbord botst een toggle met de gewoonte van elke shooter. Gelijktrekken zou
één van beide slechter maken.

**Bredere opdracht die hierbij hoort:** loop alle bindings na op acties waar hold en
toggle door elkaar lopen, en breng ze in lijn met wat per device logisch is. Kandidaten
om te controleren: mikken (nu hold op beide), Command Mode (hold — dat moet zo blijven,
de tijddilatatie hangt eraan), stance (nu toggle op de pad, hold-bij-het-geven op
toetsenbord — die asymmetrie is mogelijk al correct maar is nooit expliciet besloten).

## Twee overdraagbare lessen uit The Division (onderzoek 2026-07-25)

**1. Aim-assist: los snap-misbruik op met een COOLDOWN, niet met minder sterkte.**
Massive's antwoord op jarenlange klachten over "snappen naar het hoofd" was nooit het
magnetisme verzwakken maar een **re-acquisitie-cooldown per doel**: snappen mag, maar
niet herhaald op hetzelfde doelwit binnen korte tijd (D1 Update 1.8.2 `[OFFICIEEL]`,
en pre-emptief meegenomen naar D2 vóór launch). Dat is scherper dan sterkte verlagen,
want het raakt het misbruik zonder de hulp weg te nemen.
**Toe te passen op onze target-slowdown**: die heeft nu geen enige tijdcomponent. Een
cooldown per doelwit is de volgende stap zodra de owner meldt dat de assist te sterk
of te plakkerig voelt — en het is een betere eerste knop dan `AimAssistStrength`.

**2. Waarschuwing: per-toestand camera-profielen lezen als onvoorspelbaarheid.**
Division past verschillende deadzones, curves en maximumsnelheden toe per toestand
(idle / sprint / in dekking / ADS). Spelers ervaren de OVERGANGEN daartussen niet als
polish maar als input-lag en willekeur; er lopen jarenlange klachten over, tot en met
"het voelt alsof ik permanente input-lag heb" `[GEMETEN]`.
**Direct relevant voor ons**, want we hebben al drie camera-toestanden (3e persoon,
1e persoon, Command Mode) plus ADS, en de ADS-tak verandert nu al de kijksnelheid
(×0.35). De regel die we hieruit overnemen: **verschillen tussen profielen klein
houden, of de overgang ramp-en** — nooit een harde sprong in deadzone of curve op het
moment dat de speler van toestand wisselt.

## Wat een beslissing vraagt

Niets van het bovenstaande — het zijn allemaal conventie-waarden met een bron. De
beslissingen zitten verderop: strafe-modus bij mikken vraagt eerst een aim-offset in
de animatielaag, en camera-shake/recoil/hitmarkers zijn nieuwe systemen die buiten de
huidige milestone kunnen vallen. Die worden apart voorgelegd met wat ze kosten.
