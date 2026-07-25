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

## Wat een beslissing vraagt

Niets van het bovenstaande — het zijn allemaal conventie-waarden met een bron. De
beslissingen zitten verderop: strafe-modus bij mikken vraagt eerst een aim-offset in
de animatielaag, en camera-shake/recoil/hitmarkers zijn nieuwe systemen die buiten de
huidige milestone kunnen vallen. Die worden apart voorgelegd met wat ze kosten.
