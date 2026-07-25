# Feel-audit van de huidige build

*Stap 2 van de autonome feel-opdracht (owner, 2026-07-25). De referentie-checklist
staat in `FEEL_REFERENTIE.md`; dit document zet de HUIDIGE build daar naast.*

Status per item: **OK** (gedraagt zich als de conventie) · **AFWIJKEND** (bestaat,
verkeerd afgesteld — met de gemeten waarde) · **ONTBREEKT** (bestaat niet in het
project) · **N.V.T.** (bewust anders, met reden).

Alles hieronder is uit de code gelezen of gemeten, niet uit het geheugen.

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
| Sprint in/uitstap | AFWIJKEND | harde omschakeling van MaxWalkSpeed, geen overgang | eigen stap, zie openstaand | — |
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
| Pitch-limieten | **AFWIJKEND** | −70/+70 — owner meldt "camera kijkt onder mijn eigen schoenen"; bij −70 duikt de boom in de grond en trekt de collision hem naar binnen | −55/+70 | conventie |
| Shake | ONTBREEKT | geen enkele camera-shake in het project | eigen stap | — |
| Gedrag bij sprint | ONTBREEKT | camera reageert niet op sprint (geen FOV-punch, geen lag-verandering) | eigen stap | conventie: lichte FOV-toename |

## SPRONG

| Item | Status | Was | Wordt | Bron |
|---|---|---|---|---|
| Sprongkracht | AFWIJKEND | 420 (default) | 500 | [ENGINE] template |
| Luchtcontrole | AFWIJKEND | **0,05** (default) — een sprong op rails | 0,35 | [ENGINE] template |
| Valremming | AFWIJKEND | **0** (default) — een val remt nooit | 1500 | [ENGINE] template |
| Landingsanimatie | ONTBREEKT | geen landings-take, geen knikje | eigen stap | — |

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
| Stoptijd vanaf rennen | **0,083 s** | Bijlage B: 0,083 s | OK (maar zie LOC-03: dit is arcade-kort) |
| Glijafstand vanaf rennen | **12,0 cm** | Bijlage B: 13 cm | OK |
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

**Nog niet gedaan, met reden** — dit zijn bouwopdrachten, geen tuningrondes:
turn-in-place (ROT-03, vraagt een draai-animatie of je krijgt voetslip),
sprint-camerastack (CAM-11), camera-shake en recoil/hitmarkers (§8 FEEDBACK
bestaat volledig niet), coyote time + sprong-inputbuffer (JMP-07/08 — klein, maar
het raakt de sprongtiming die de owner net getest heeft). Ze staan met een
aanbeveling in HANDOFF.md.

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
