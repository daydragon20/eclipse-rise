#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipseCharacterTypes.generated.h"

class UAnimSequence;
class USkeletalMesh;
class UStaticMesh;

/**
 * Character data (SPEC-P1-05). The movement numbers are the LOCKED graybox feel
 * targets (phase0/graybox_feel_targets.md §2, GDD 4.1.1) — changing them goes
 * through change management, not through this file's defaults.
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseCharacterTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** cm/s. Feel target: 1.8 m/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float WalkSpeed = 180.0f;

	/** cm/s. Feel target: 4.2 m/s (default gait). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float RunSpeed = 420.0f;

	/** cm/s. Feel target: 6.5 m/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float SprintSpeed = 650.0f;

	/** Feel target: crouch is the stealth default. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float CrouchSpeed = 150.0f;

	/**
	 * Snelheid terwijl je MIKT (owner-opdracht 26-07 avond, punt 3). Tot vandaag
	 * kostte mikken niets: je sprintte even hard met je vizier op, en dan is
	 * mikken geen keuze maar een gratis verbetering.
	 *
	 * RESEARCH. De multipliers die dit genre gebruikt, uitgedrukt op de
	 * basisloopsnelheid: Call of Duty ~0,45x (zwaardere wapens lager), Rainbow Six
	 * Siege ~0,5x, Battlefield ~0,6x, Destiny 2 ~0,4x, The Division ongeveer
	 * halveert. De band is dus 0,4-0,6, met de zware kant bij tactische shooters.
	 *
	 * Onze 420 is snel voor het genre, dus de onderkant van die band is hier de
	 * juiste: 145 is 0,345x. Dat is bewust ONDER wandelen (180) én onder hurken
	 * (150), zodat mikken de traagste houding is die er is — de owner-opdracht
	 * zegt letterlijk "mikken is een keuze die je vastzet", en dan mag het niet
	 * ook nog de snelste manier zijn om ergens te komen.
	 * [CONVENTIE met bron, onderkant van de band gekozen om de snelle basis.]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float AimSpeed = 145.0f;

	/**
	 * Halve capsulehoogte gehurkt, in cm. De ENGINE-default is 40 (dus 80 cm
	 * totaal) en die stond hier nooit tegenover een keuze — precies zoals de rest
	 * van het bewegingscomponent bij de feel-audit.
	 *
	 * 80 cm is geen hurkhoogte maar kruiphoogte: tegen een staande capsule van
	 * 176 cm zak je 96 cm, oftewel 55%. Een mens die door de knieën gaat verliest
	 * ongeveer 30%. Op 62 (124 cm) is dat 30% en past de houding bij de animatie
	 * die er straks op komt — en het scheelt of je ergens onderdoor past die als
	 * dekking bedoeld is.
	 * [CONVENTIE + meting; zie Eclipse.Feel.Layer2.CrouchCostsSpeedAndBuysHeight.]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Locomotie", meta = (ClampMin = 30, ClampMax = 88))
	float CrouchedHalfHeightCm = 62.0f;

	// ---- Feel-audit 2026-07-25, gebied LOCOMOTIE + SPRONG -------------------
	// Bevinding: van het CharacterMovementComponent werden alleen MaxWalkSpeed en
	// MaxWalkSpeedCrouched gezet. Al het andere stond op de ENGINE-default, en die
	// defaults zijn niet ontworpen voor een third-person shooter — Epic's eigen
	// TP_ThirdPerson-template overschrijft ze allemaal. Dat is de reden dat lopen
	// als schaatsen leest: geen aanloop, een trage draai en vrijwel geen
	// luchtcontrole. Elke waarde hieronder noemt de engine-default en waar de
	// gekozen waarde vandaan komt.

	/**
	 * cm/s². Engine-default 2048 = vrijwel onmiddellijk op topsnelheid, wat als
	 * gewichtloos leest. 1400 geeft ~0,30 s aanloop naar de loopsnelheid van 420:
	 * merkbaar dat er massa vertrekt, kort genoeg om niet traag te voelen.
	 * [REDENERING binnen een [ENGINE]-band — Epic tuned dit niet in de template.]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float MaxAcceleration = 1400.0f;

	/** cm/s². Engine-default 2048, TP-template 2000. [ENGINE] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float BrakingDecelerationWalking = 2000.0f;

	// ---- Feel-audit fase 3, gebied LOCOMOTIE: remmen en richting -----------
	// Het harnas mat wat er stond: een sprint stopte in 0,083 s over 12,0 cm. Dat
	// is niet "responsief", dat is gewichtloos — de reference noemt het "dichter
	// bij een arena-shooter dan bij een TPS met gewicht" (LOC-03). De drie velden
	// hieronder zijn de enige knoppen die daar iets aan doen, en ze zitten in
	// elkaars weg als je er maar één aanraakt.

	/**
	 * Zonder deze vlag gebruikt UE de GEWONE GroundFriction ook om te remmen, en
	 * is BrakingFriction dood gewicht. Aanzetten is de voorwaarde voor alles
	 * hieronder. [ENGINE] ApplyVelocityBraking.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement")
	bool bUseSeparateBrakingFriction = true;

	/**
	 * De rem zelf. dv/dt = -(BrakingFriction x Factor)*v - BrakingDeceleration,
	 * dus dit is de snelheidsAFHANKELIJKE helft: hij bijt hard bij hoge snelheid
	 * en laat de staart uitlopen. 4.0 geeft sprint 200 ms / 57 cm en rennen
	 * 150 ms / 28 cm — zwaar maar responsief, zonder in Division-traagheid te
	 * vallen. [ENGINE] + Bijlage B van FEEL_REFERENTIE.md (exact nagerekend).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float BrakingFriction = 4.0f;

	/**
	 * 1.0, niet de engine-default 2.0. Epic's eigen commentaar in de broncode
	 * noemt die 2.0 letterlijk "Historical value, 1 would be more appropriate" —
	 * en hij vermenigvuldigt OOK BrakingFriction, dus met 2.0 rem je twee keer zo
	 * hard als het veld hierboven suggereert en is elke tuning ervan misleidend.
	 * [ENGINE] CMC-constructor, letterlijk commentaar.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float BrakingFrictionFactor = 1.0f;

	/**
	 * GEEN rem maar de RICHTINGSWISSELSNELHEID: in CalcVelocity draait
	 * GroundFriction de snelheidsvector naar de inputrichting, met tijdconstante
	 * 1/Friction. Op 8.0 kost een 90-graden wissel op sprint 233 ms — merkbaar
	 * momentum, geen ijs (4 = 400 ms glijden, 16 = 117 ms klittenband). Stond
	 * nooit gezet; 8.0 was toevallig goed maar niet GEKOZEN. [ENGINE] CalcVelocity.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float GroundFriction = 8.0f;

	/**
	 * Zijwaarts en achteruit, als factor op de topsnelheid. UE kent hier niets
	 * voor — zie UEclipseCharacterMovementComponent, dat is de enige plek waar
	 * het kan. 1.00 / 0.85 komen uit Gears 5 TU3, het enige spel in dit genre dat
	 * er cijfers over publiceert, en 0.85 kwam daar NA een gemeten 0.739 die te
	 * sloom bleek. [OFFICIEEL]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0.1, ClampMax = 1.0))
	float StrafeSpeedRatio = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0.1, ClampMax = 1.0))
	float BackwardSpeedRatio = 0.85f;

	// ---- Feel-audit fase 3, gebied SPRONG: de twee vergevingsvensters --------
	// Allebei bestaan ze in UE NIET, en allebei repareren ze een moment waarop de
	// speler het goede deed en de game nee zei. Dat is de goedkoopste feel-winst
	// die er is: niemand merkt ze op als ze er zijn, iedereen voelt ze als ze
	// ontbreken — en dan heet het "de besturing reageert niet".

	/**
	 * Coyote time: hoe lang na het van een rand AF LOPEN een sprongdruk alsnog
	 * telt. UE staat dit expliciet niet toe (ACharacter::JumpIsAllowedInternal
	 * eist JumpCurrentCount + 1 < JumpMaxCount zodra je valt, en met JumpMaxCount
	 * = 1 is dat altijd onwaar), dus een speler die een fractie te laat drukt
	 * krijgt geen sprong maar een val — terwijl hij op het scherm nog op de rand
	 * leek te staan. 110 ms is net genoeg om de menselijke reactiemarge te dekken
	 * en te kort om als zweven te lezen.
	 *
	 * RESEARCH (owner-opdracht 26-07, punt 6 — de waarde stond er al, de bron niet).
	 * Wat andere spellen gebruiken, voor zover gepubliceerd of gemeten door de
	 * community:
	 *   Celeste            ~0,10 s (Maddy Thorson heeft dit publiek bevestigd)
	 *   Hollow Knight      ~0,08 s
	 *   Super Mario Bros.  ~6 frames op 60 Hz = 0,10 s
	 *   platformers breed  0,08-0,15 s is de band waarbinnen niemand het merkt
	 * Onze 0,11 s zit midden in die band. Boven ~0,20 s gaat het opvallen als
	 * zweven; onder ~0,06 s dekt het de menselijke reactiemarge niet meer.
	 * [CONVENTIE, met bron. UE levert dit niet; zie FEEL_REFERENTIE.md JMP-07.]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0, ClampMax = 0.5))
	float CoyoteTimeSeconds = 0.11f;

	/**
	 * Sprong-inputbuffer: hoe lang VÓÓR de landing een sprongdruk bewaard blijft
	 * en bij het raken van de grond alsnog uitgevoerd wordt. Zonder buffer moet
	 * de speler op de frame van de landing drukken, en elke druk daarvoor is
	 * gewoon weg. 150 ms.
	 *
	 * RESEARCH (owner-opdracht 26-07, punt 6). Gepubliceerde en gemeten waarden:
	 *   Celeste            ~0,10 s
	 *   Hollow Knight      ~0,10 s
	 *   Meat Boy-achtigen  0,10-0,20 s
	 *   actie-shooters     eerder 0,15-0,25 s, omdat een sprong daar zeldzamer is
	 *                      en een gemiste sprong duurder voelt dan een te gretige
	 * Onze 0,15 s zit op de grens tussen platformer en shooter, en dat is precies
	 * waar dit spel staat. Ruimer dan de coyote-tijd met opzet: te vroeg drukken
	 * is een veel vaker gemaakte fout dan te laat.
	 * [CONVENTIE, met bron; JMP-08.]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0, ClampMax = 0.5))
	float JumpInputBufferSeconds = 0.15f;

	// ---- Feel-audit fase 3, gebied TRAVERSAL --------------------------------
	// LET OP bij het naslaan: Bijlage D van FEEL_REFERENTIE.md is op deze twee
	// rijen VEROUDERD. Zij noemt 30 uu en 40 graden; de herziene §10 in hetzelfde
	// document komt op 35 uu uit en laat de hellingshoek juist STAAN, met bronnen.
	// De body is de latere en beter onderbouwde tekst en wint hier.

	/**
	 * Hoe hoog het personage geruisloos op stapt. Engine-default 45.
	 *
	 * 45 is géén slordigheid: als percentage van de personagehoogte (25,6% van
	 * 176 cm) is het precies de norm van de hele id/Valve/Epic-lijn — Quake 32%,
	 * Half-Life 25%, UE 25,6%. Het bestaat zodat een personage stoepranden, puin
	 * en trapmeshes kan absorberen zónder traversal-animatie.
	 *
	 * Voor ECLIPSE is de vraag daarom niet "is 45 te hoog" maar "willen we de
	 * traversal-laag zien". De GDD voert vault en mantle op als eigen verbs, dus
	 * ja: op 35 uu (20%) blijft een echte traptrede (~17,8 cm) een stap, maar
	 * wordt kniehoge dekking een vault in plaats van een geruisloze stap omhoog.
	 * Dat is een ontwerpkeuze die uit de GDD volgt, geen correctie van de engine.
	 * [OFFICIEEL] Quake III bg_local.h, Valve Dimensions; [ENGINE] CMC-ctor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 1))
	float MaxStepHeightCm = 35.0f;

	/**
	 * Hoe smal een richel mag zijn om er nog op te blijven staan. Engine-default
	 * 0 betekent: je kunt op een centimeter geometrie balanceren. UE krimpt bij
	 * een perch-test de capsule voor de vloercontrole, zodat je óf echte voetsteun
	 * hebt óf valt; met 0 is die test uitgeschakeld. 10 uu maakt leesbaar waar je
	 * kunt staan. [ENGINE] CMC-ctor.
	 *
	 * De loopbare hellingshoek blijft BEWUST op de engine-default (44,77°). Een
	 * eerder voorstel in dit project was 40, maar dat was een smaakoordeel tegen
	 * vier onafhankelijke bronnen in: Quake III komt op 45,573° uit, Half-Life
	 * gebruikt letterlijk dezelfde test, Source meet 45,573° en UE 44,77°. Het
	 * getal keert terug omdat 0.7 een goedkope magische normaal-Z is. Niet
	 * aankomen; wél de Quake-waarschuwing overnemen dat een helling diagonaal
	 * belopen veel steiler voelt — dus ramps as-uitgelijnd bouwen.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float PerchRadiusThresholdCm = 10.0f;

	/**
	 * Graden/s waarmee het lichaam naar zijn looprichting draait. Engine-default
	 * 360, TP-template 500. Op 360 kost een omkering een halve seconde draaien
	 * terwijl je al schuift — precies het schaats-effect. [ENGINE, template]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float BodyRotationRateYaw = 500.0f;

	/** Engine-default 420, TP-template 500. [ENGINE, template] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float JumpZVelocity = 500.0f;

	/**
	 * Engine-default 0.05 — dat is bijna geen sturing in de lucht en leest als een
	 * sprong op rails. TP-template 0.35. [ENGINE, template]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0, ClampMax = 1))
	float AirControl = 0.35f;

	/** cm/s². Engine-default 0 (val remt nooit), TP-template 1500. [ENGINE, template] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float BrakingDecelerationFalling = 1500.0f;

	/**
	 * cm/s. Engine-default 0: een stick die 5% uitslaat geeft dan 5% snelheid, en
	 * dat is onder de snelheid waarop een loopcyclus nog leest — het personage
	 * glijdt dan in een idle-pose vooruit. TP-template 20. [ENGINE, template]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0))
	float MinAnalogWalkSpeed = 20.0f;

	/**
	 * First-person FOV. This field predates the camera itself: it sat here unused
	 * because AEclipseCharacter had no camera at all and the engine fell back to
	 * the pawn's own view point (owner playtest 2026-07-25 — "ik kijk vanuit het
	 * lichaam of vanaf de voeten"). It now drives the first-person half of the
	 * C toggle, where 90 is the right number; third person wants less.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 30, ClampMax = 140))
	float CameraFOV = 90.0f;

	/** Over-the-shoulder FOV. Narrower than first person so the shoulder framing
	 *  does not fisheye and distant targets keep their pixels (owner spec). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 30, ClampMax = 140))
	float ThirdPersonFOV = 80.0f;

	/** Boom length in third person: hip-to-shoulder distance that still reads at
	 *  command range. 0 is first person — the toggle lerps between the two. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float ThirdPersonArmLength = 300.0f;

	/** Over-the-shoulder offset in BOOM space (X forward, Y right, Z up), so it
	 *  stays over the same shoulder as the camera swings around. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera")
	FVector CameraSocketOffset = FVector(0.0f, 55.0f, 65.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float CameraLagSpeed = 12.0f;

	/**
	 * Hoe ver de camera maximaal mag achterblijven, in uu. Dit is de fix voor het
	 * defect dat de owner meldde als "mijn personage schaalt met snelheid"
	 * (feel-audit S1, 2026-07-25) — en het is GEMETEN, niet beredeneerd.
	 *
	 * De spring arm laat zijn OORSPRONG achterlopen op de pawn met een
	 * exponentiële demper. In stationaire toestand is die achterstand exact
	 * `snelheid / CameraLagSpeed`, dus 15 cm bij lopen (180), 35 cm bij rennen
	 * (420) en 54 cm bij sprinten (650). Dat is de ENIGE term in de hele
	 * camerarig die met de snelheid meebeweegt: mesh-schaal, boomlengte en FOV
	 * staan alle drie stil (het harnas rekent ze per meting af). Gemeten aan de
	 * hoek die de capsule bij de camera opspant: 31,50 graden stil tegen 28,84
	 * graden rennend, oftewel het personage werd 8,4% kleiner van gaan rennen.
	 *
	 * De klem lost dat op zonder de lag weg te gooien, en dat is de bedoeling:
	 * camera-lag bestaat om KLEINE, schokkerige correcties glad te strijken, niet
	 * om een halve meter afstand te kopen. Boven ~72 cm/s (= 6 x 12) zit de lag
	 * op zijn klem en is hij dus constant — precies wat "mag niet met de snelheid
	 * meebewegen" betekent. Wat overblijft is 6 cm tussen stilstand en lopen,
	 * ofwel 1,8% schijnbare grootte, en dat is onder de meetbare drempel.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float CameraLagMaxDistance = 6.0f;

	/**
	 * The boom's collision probe: the camera may never pass through a wall.
	 *
	 * 20 en niet 12 (feel-audit fase 3, CAM-06): een probe van 12 uu is kleiner
	 * dan de capsule van 34 uu die hij moet beschermen, dus de camera schaaft
	 * langs hoeken en schiet er soms half doorheen voordat de trace iets merkt.
	 * De referentieband is 20-25; 20 is de onderkant daarvan, want elke uu extra
	 * trekt de camera ook eerder naar binnen in krappe stegen.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 1))
	float CameraProbeSize = 20.0f;

	/** Seconds for the first/third-person swap. Hard-cutting is nauseating; this
	 *  is short enough to feel instant and long enough to keep the horizon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float ViewToggleBlendTime = 0.2f;

	/**
	 * Command Mode framing (GDD 14.5 debug-grade): during the hold the boom pulls
	 * back and rises so the player reads the field instead of a shoulder. No new
	 * mode — SPEC-P2-07 owns the input contexts; this only lerps two numbers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float CommandModeArmLength = 520.0f;

	/**
	 * How far the camera RISES during the hold, in units, on top of the boom
	 * pulling back. Deliberately height and not pitch: the boom runs on
	 * bUsePawnControlRotation, so pitch belongs to the player's stick, and a
	 * camera that tilts itself while the player is aiming fights the person
	 * holding the controller. Raising the eye line reads as "overseeing the
	 * field" without taking the aim away.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Camera", meta = (ClampMin = 0))
	float CommandModeCameraRise = 120.0f;

	/** Degrees per second at full stick deflection, yaw and pitch separately —
	 *  they are not the same task and never want the same speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0))
	float StickYawSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0))
	float StickPitchSpeed = 180.0f;

	/** Stick drift must never move the camera on its own. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0, ClampMax = 0.9))
	float StickDeadzone = 0.08f;

	/**
	 * Radial deadzone on the MOVEMENT stick. Its own number, and slightly wider
	 * than the look deadzone, because the two sticks fail differently: a drifting
	 * look stick wobbles the camera and you notice at once, while a drifting move
	 * stick walks the character away AND — with bOrientRotationToMovement — turns
	 * the body with it, so the whole world appears to rotate on its own. The
	 * owner's left stick measures LY = -0.048 at rest.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0, ClampMax = 0.9))
	float MoveDeadzone = 0.08f;

	/**
	 * How much slower looking gets while aiming. This was MISSING, and it is the
	 * likely answer to the owner's "kijken voelt op sommige momenten te scherp":
	 * ADS narrows the FOV to 0.80 but left the look speed alone, so the reticle
	 * swept the screen FASTER while aiming than while hip-firing — the opposite
	 * of what aiming is for.
	 * The physically neutral value is tan(32)/tan(40) = 0.745 (a target then keeps
	 * the same on-screen speed). Every shipped shooter goes deliberately below it:
	 * CoD ships 0.346 for yaw, Apex pros run 0.30-0.40.
	 *
	 * LET OP — hier stond dat 0.60 gekozen was, terwijl het veld op 0.35 staat.
	 * Een tuningwaarde waarvan de eigen onderbouwing een ANDER getal noemt is
	 * dezelfde soort leugen als een paneel dat "1,50 s per 360" toont terwijl de
	 * game 0,60 s draait — en die kostte deze week al een sessie. Het commentaar
	 * beschrijft nu de waarde die er echt staat.
	 *
	 * 0.35 ligt binnen de band die spellen daadwerkelijk verschepen (CoD 0.346,
	 * Apex-pro's 0.30-0.40) en is dus verdedigbaar; 0.60 (tussen die band en de
	 * meetkundig neutrale 0.745) is dat óók. Welke van de twee bij ONZE milde zoom
	 * hoort, is smaak en niet af te leiden — dat staat als keuze op de ownerlijst.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0.1, ClampMax = 1.5))
	float AdsLookMultiplier = 0.35f;

	/**
	 * Response curve exponent for stick look, sign-preserving. A stick is a RATE
	 * controller with about a centimetre of travel that has to cover both a micro
	 * correction and a 180-degree turn; linear spends that centimetre evenly, so
	 * the slow band where aiming happens gets only 20% of the throw. At exponent 2
	 * it gets about 45%.
	 * The SHAPE is documented (Activision names its curves Standard = power curve,
	 * Linear, Dynamic = reverse-S). The NUMBER is not: no publisher prints its
	 * exponent, and an earlier comment here claimed "2.0 is the console-shooter
	 * default" — that is folklore and it has been removed. What is measured:
	 * Halo's older, livelier curve reads as roughly quadratic and Infinite's as
	 * roughly cubic, and Infinite is the one players call sluggish. Hence 2.0,
	 * chosen on that evidence rather than on a number someone repeated.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 1, ClampMax = 4))
	float StickResponseExponent = 2.0f;

	/**
	 * Mouse look stays raw — a mouse has no deadzone and no drift, and curving it
	 * breaks muscle memory. Only the scale is tunable.
	 *
	 * 2.5 en niet 1.0 sinds 2026-07-25, en dat is een gedrag-NEUTRALE wijziging:
	 * APlayerController vermenigvuldigde tot dan met de legacy-schaal 2.5 (zie
	 * AEclipsePlayerController's constructor). Die schaal staat nu uit omdat hij
	 * de stick-kijksnelheid stil 2,5x te hoog maakte; bij de muis is dit getal
	 * een kale schaal zonder eenheid, dus daar is de eerlijke keuze het gedrag
	 * gelijk houden in plaats van het getal.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0))
	float MouseLookScale = 2.5f;

	/**
	 * Aim assist, target-slowdown form: the camera gets heavier while the reticle
	 * is over a hostile. 0 = off, 1 = maximum. Deliberately NOT magnetism (which
	 * pulls the reticle toward a target): at command distance you sweep the
	 * reticle past your own squad to give orders, and a pull would fight that
	 * every time. Slowdown only ever resists, so it can never move your aim
	 * somewhere you did not point it — and it is the honest form when hits are
	 * locational (GDD 8.2).
	 * At full strength the look keeps AimAssistFloor of its speed; CoD ships 0.4
	 * hip / 0.5 ADS for the same mechanism, so this band is sourced even though
	 * this exact default is a starting point to tune against.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0, ClampMax = 1))
	float AimAssistStrength = 0.6f;

	/** Speed retained at full strength. 0.45 sits between CoD's 0.4 and 0.5. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0.1, ClampMax = 1))
	float AimAssistFloor = 0.45f;

	/** Half-angle of the cone around the reticle that counts as "over a target".
	 *  Halo's friction cone measures about 5 degrees, Lyra's box is tighter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0, ClampMax = 20))
	float AimAssistConeDegrees = 4.0f;

	/** Beyond this the assist fades out. 5000 uu = 50 m, which is where the GDD
	 *  puts the hitscan/projectile boundary — help stops where ballistics change. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|AimAssist", meta = (ClampMin = 0))
	float AimAssistRange = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look")
	bool bInvertLookY = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = -89, ClampMax = 0))
	float ViewPitchMin = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0, ClampMax = 89))
	float ViewPitchMax = 70.0f;

	/**
	 * Over hoeveel graden vóór de pitch-limiet het kijken afremt (CAM-07b).
	 *
	 * Nesky's camerafout #47 is "maintaining pitch speed until hitting the pitch
	 * limit": de camera moet tégen de klem aan afremmen, niet er met volle
	 * snelheid tegenaan slaan. Het verschil tussen "de camera stopt" en "de camera
	 * knalt tegen een muur" is direct voelbaar en kost bijna niets.
	 *
	 * De demping loopt naar een BODEM (PitchDampFloor) en niet naar nul: bij nul
	 * wordt de limiet asymptotisch benaderd en dus nooit gehaald, en een camera
	 * die net niet omhoog wil kijken is een erger defect dan de klap.
	 * [OFFICIEEL] John Nesky, "50 Game Camera Mistakes", GDC 2014.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0, ClampMax = 45))
	float PitchDampBandDegrees = 10.0f;

	/** Hoeveel kijksnelheid er ín de dempband minimaal overblijft, zodat de
	 *  limiet bereikbaar blijft. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Look", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float PitchDampFloor = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Attributes", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;
};

/**
 * Visual body definition (DT_BodyDefs row — step-2 character pipeline). Soft
 * references only: a missing asset degrades to the capsule body with a logged
 * warning, never a crash (GDD 14.3.5).
 *
 * The anim columns feed UEclipseAnimInstance's minimal locomotion (GDD 14.5
 * debug-grade). They are resolved PER BODY and checked against THAT body's
 * skeleton, because DT_BodyDefs points at nine packs that share a skeleton
 * family but not a skeleton asset — see FEclipseLocomotionSet. Missing takes
 * cost the body a rung on the ladder, not the frame.
 */
USTRUCT(BlueprintType)
struct FEclipseBodyDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/**
	 * Looping standing pose. Mandatory: it is the floor of the locomotion ladder
	 * (EEclipseLocomotionTier) — without it a body has nothing to cross-fade out
	 * of and falls all the way back to the mesh's ref pose.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> IdleAnim;

	/** Looping walk cycle, blended in at DA_CharacterTuning's WalkSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> WalkAnim;

	/**
	 * Looping run/jog cycle, blended in at RunSpeed and driven faster (never
	 * swapped) for sprint. Optional: a body with only a walk still gets a gait,
	 * it just carries the whole ramp (GDD 14.3.5).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> RunAnim;

	/**
	 * Zijwaarts en achteruit (owner-opdracht 26-07, punt 8).
	 *
	 * Vanaf vandaag is het personage CAMERA-relatief: duw je achteruit, dan blijft
	 * hij naar de camera kijken en loopt hij achteruit — zoals Gears, The Division
	 * en Borderlands het doen. Daarvoor draaide hij zich om, en dan volstond één
	 * vooruit-cyclus voor elke richting.
	 *
	 * Ontbreekt een richting, dan valt hij terug op de vooruit-cyclus: dat leest
	 * als moonwalken, maar het is nog altijd beter dan de ref-pose (14.3.5). De
	 * SciFiCharacter-pack heeft alle vier (Jog_Fwd/Bwd/Lt/Rt_Rifle).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> WalkBackAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> WalkLeftAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> WalkRightAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> RunBackAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> RunLeftAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> RunRightAnim;

	/**
	 * Wordt afgespeeld bij elk schot (owner-opdracht 26-07, punt 3). Stond hier
	 * tot vandaag als NIET GELEZEN: de take was geauthord en werd nooit gespeeld,
	 * dus vuren veranderde de pose niet.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> ShootAnim;

	/**
	 * Korte klap bij het oplopen van schade (26-07, punt 2, tweede helft).
	 * SciFiCharacter levert Hit_React_1..4; het setup-script pakt de kortste.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> HitReactAnim;

	/**
	 * Herladen (26-07 avond, punt 4). SciFiCharacter levert Reload_Rifle_Hip,
	 * Reload_Rifle_Ironsights en Reload_Pistol; ze lagen er sinds de import en
	 * werden door niets afgespeeld — er wás geen herladen.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> ReloadAnim;

	/**
	 * Draaien op de plek (26-07 avond, punt 7). Een kwartslag links en rechts.
	 *
	 * De speler is Belica en die levert `Idle_Turn_90_Left/Right`; de SciFi-packs
	 * hebben ze niet. Ontbreken ze, dan draait het lichaam nog steeds mee en
	 * schuiven de voeten een fractie — hetzelfde gedrag als vóór vanavond, en
	 * dat is de juiste terugval (14.3.5: luid degraderen, niet stilvallen).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> TurnLeftAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> TurnRightAnim;

	/**
	 * Hurken en opstaan (locomotie-audit 26-07, punt 13). Zonder deze take
	 * verspringt het lichaam tussen staand en gehurkt zonder tussenbeweging — de
	 * capsule krimpt, de pose knipt. SciFiCharacter levert Crouch_to_Stand; die
	 * wordt beide kanten op gebruikt, want een omgekeerd afgespeelde opstaanpose
	 * bestaat niet in de pack en de beweging is kort genoeg om te lezen als
	 * "houding wisselt".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> CrouchTransitionAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	TSoftObjectPtr<UAnimSequence> DeathAnim;

	/**
	 * Toon restyle (15.5 asset policy: downloaded, then restyled): re-dress every
	 * material slot with the cel master — the slot's own base texture becomes
	 * luminance detail, the faction palette below supplies the hue. Keeps every
	 * body on the same exposure tier as the unlit district (lit PBR bodies
	 * underexpose to silhouettes against the emissive world).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	bool bToonRestyle = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body", meta = (EditCondition = "bToonRestyle"))
	FLinearColor TintLit = FLinearColor(0.20f, 0.20f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body", meta = (EditCondition = "bToonRestyle"))
	FLinearColor TintShade = FLinearColor(0.07f, 0.07f, 0.09f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body", meta = (ClampMin = 0.1))
	float MeshScale = 1.0f;

	/** Mesh-root drop below the capsule center; -90 fits a standing humanoid in the default capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	float MeshZOffset = -90.0f;

	/** UE humanoid meshes face +Y; -90 turns them to the capsule's forward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body")
	float MeshYaw = -90.0f;

	// ---- HET WAPEN ALS LOS OBJECT (owner-besluit O-5 "volledig", 31-07) --------
	//
	// De maatstaf (phase0/REFERENTIE_TPS.md, hoofdstuk 4) stelde vast dat er WEL
	// een wapen in beeld hangt maar dat het GEEN los object is: het zit in de
	// karaktermesh. Belica draagt het als materiaalslot 4, 'M_Belica_Guns'
	// [GEMETEN uit het log van 31-07, niet afgeleid]. Dat verklaart drie
	// waarnemingen met één oorzaak — geen attachment, wél een wapen, en een
	// wapenwissel die visueel niets doet.
	//
	// Deze vier velden zijn de RIG-kant van de reparatie. Ze staan per lichaam en
	// niet per wapen, want ze beschrijven de HAND en niet het geweer: de
	// wapen-assets presenteren allemaal hun greep op hun eigen oorsprong
	// (Tools/blender/gen_weapons.py), dus wat er per rig verschilt is uitsluitend
	// hoe die hand staat.

	/**
	 * Materiaalslot-namen waarvan de geometrie het INGEBOUWDE wapen is. Bevat-match,
	 * niet gelijk-aan: de packs noemen het 'M_Belica_Guns', 'MI_Rifle_A' enzovoort.
	 *
	 * OP NAAM EN NIET OP INDEX, om dezelfde reden als de gunmetal-tint hierboven:
	 * slotvolgorde is een eigenschap van het asset, en een hardgecodeerde 4 breekt
	 * stil zodra er een lichaam met een andere indeling bijkomt.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body|Wapen")
	FName BuiltInWeaponSlotMatch = TEXT("Gun");

	/**
	 * De ingebouwde wapensectie verbergen zodra er een LOS wapen aan de hand hangt.
	 *
	 * Niet onvoorwaardelijk: een lichaam zonder los wapen-asset zou dan met lege
	 * handen staan, en dat is zichtbaar slechter dan wat er nu is. De code verbergt
	 * pas als het losse mesh geladen IS — verbergen en aanhechten zijn hetzelfde
	 * feit, niet twee schakelaars (owner-regel "basis en kader, geen schakelaars").
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body|Wapen")
	bool bHideBuiltInWeapon = true;

	/**
	 * Het bot waar het wapen aan hangt. Op het Paragon-rig van de speler heet dat
	 * `hand_r`; de SciFi-packs draaien op het UE4-mannequin en gebruiken dezelfde
	 * naam. Bestaat hij niet, dan zegt de code dat luid en hangt er geen wapen —
	 * stil aan de wortel hechten zou een geweer midden in de borst zetten.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body|Wapen")
	FName WeaponGripBone = TEXT("hand_r");

	/**
	 * De greepcorrectie: waar de greep van het wapen ligt ten opzichte van het
	 * handbot, in BOT-ruimte.
	 *
	 * De standaard is niet nul en dat is met opzet — het botcentrum van een hand
	 * ligt in de pols, niet in de palm. Deze getallen zijn de eerste stand en
	 * horen op een frame bijgesteld te worden, niet op een redenering.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body|Wapen")
	FVector WeaponGripOffset = FVector(-2.0f, 4.0f, 0.0f);

	/**
	 * DE 180 GRADEN ZIJN GEMETEN, EN DE MEETUITSLAG SLOOT ÉÉN VAN TWEE OORZAKEN UIT.
	 *
	 * Owner-waarneming (SHOT_FINDINGS.md): *"het wapen wordt omgekeerd
	 * vastgehouden"*. Daar zijn twee kandidaten voor, en ze vragen tegengestelde
	 * reparaties — die verwarren kost een avond:
	 *
	 *   1. de GREEPROTATIE hier klopt niet  -> repareren in deze data
	 *   2. de forward-as van de MESH klopt niet -> repareren in gen_weapons.py,
	 *      en alle vier de wapens opnieuw exporteren
	 *
	 * Kandidaat 2 is UITGESLOTEN met een meting die er al lag voordat de
	 * waarneming binnenkwam. De import drukt de bounding box af per as
	 * (Tools/import_blender_weapons.py) en die zei: langste as = **X**, 94,9 cm
	 * tegen 7,2 (Y) en 35,4 (Z), met de doos lopend van X=-24,3 (kolf) tot
	 * X=+70,6 (loop). Unreal verwacht +X als voorwaarts, en dat is precies wat er
	 * staat. De mesh-as is dus goed, en hem "omdraaien" in Blender zou de
	 * UE-conventie juist breken voor elk volgend wapen.
	 *
	 * Wat er dan overblijft is kandidaat 1, en die is ook rechtstreeks gemeten:
	 * `[WAPEN * STAP3 GREEP]` drukt per opname de correctie af die de loop langs
	 * de kijkrichting van het lichaam legt, en die kwam uit op een YAW van -158
	 * tot -180 graden. Een halve slag, en dat is exact hoe "omgekeerd
	 * vastgehouden" er in getallen uitziet.
	 *
	 * Vandaar 180 en niet een gefit getal: de pitch en roll in die meting
	 * VARIËREN met de armpose (P=-6 tot -28, R=+5 tot +18) omdat de hand meedraait
	 * met de animatie, en dat hoort ook — het wapen moet stijf in de hand zitten en
	 * de arm draagt hem. Alleen de halve slag is de constante fout.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body|Wapen")
	FRotator WeaponGripRotation = FRotator(0.0f, 180.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Body|Wapen", meta = (ClampMin = 0.1))
	float WeaponGripScale = 1.0f;
};

/**
 * Named story-character slot (DT_NamedCharacters — step-3 MetaHuman pipeline).
 * The owner authors MH_<Name> in MetaHuman Creator (phase0/metahuman_recipes.md).
 *
 * NOG NIET AANGESLOTEN — deze comment beschreef tot 26-07 in tegenwoordige tijd
 * een terugval ("dresses itself from FallbackBodyDef") die nergens gebeurt: geen
 * enkele regel C++ laadt DT_NamedCharacters, ook al bestaat het asset en staat
 * het veld op FEclipseCampaignSetupAsset. Wie een rij invult ziet dus niets.
 * De vorm ligt klaar; de laadstap is stap 3 en die is niet gezet.
 */
USTRUCT(BlueprintType)
struct FEclipseNamedCharacterRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	FText DisplayName;

	/**
	 * Imported MetaHuman body (MH_<Name>); unset = fallback body below.
	 *
	 * **NIET GELEZEN** — deze drie velden horen bij elkaar en wachten op hetzelfde:
	 * er is nog geen code die een NAMED CHARACTER aankleedt. De game mode kleedt
	 * lichamen aan uit DT_BodyDefs op rol (Player, Rebel_A, ...), niet op naam.
	 *
	 * Dat is geen vergeetachtigheid maar een volgorde: de MetaHuman-gezichten zijn
	 * machine-lokaal (908 MB, per machine opnieuw ophalen) en het aankleden op
	 * naam heeft pas zin als die gezichten er staan. Zolang dat niet zo is, zou
	 * lezen van deze velden betekenen dat de code een pad kiest dat op elke andere
	 * machine leeg is.
	 *
	 * Wat er dan moet gebeuren: MetaHumanMesh als mesh, FallbackBodyDef als
	 * terugval zodra hij ontbreekt, en Faction voor de tint en de barkkeuze.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	TSoftObjectPtr<USkeletalMesh> MetaHumanMesh;

	/** DT_BodyDefs-rij die instaat zolang de MetaHuman ontbreekt. **NIET GELEZEN**, zie MetaHumanMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	FName FallbackBodyDef;

	/** Rebel / Dominion — tint en barkkeuze (GDD 08/16). **NIET GELEZEN**, zie MetaHumanMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Named")
	FName Faction;
};

/**
 * One soldier class (DT_ClassDefs row; row name = class id — SPEC-P2-01,
 * GDD 4.2.3). Classes are data, not subclasses (GDD 12.3): the one
 * AEclipseCharacter body stays; a class changes kit, one signature verb and
 * bark flavor — never the shared movement/health/order contract. A soldier
 * whose ClassId has no row here degrades to the classless kit (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseClassDefRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FText DisplayName;

	/** DT_Weapons row this class carries; NAME_None = the platform default (first row). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FName WeaponRow;

	/** Optional DT_BodyDefs row that dresses this class (visible kit); NAME_None = the shared squad body pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FName BodyDefOverride;

	/** Signature verb identity (Class.Verb.* family — Momentum/Stabilize/Killzone). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FGameplayTag SignatureVerb;

	/** Seconds after a down in which this class can stabilize (GDD 4.2.5 window; 30 for Medic, 0 = cannot). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float StabilizeWindowSeconds = 0.0f;

	/** Killzone lane range in cm (6000 for Sniper, 0 = none; Command Mode wiring = SPEC-P2-02). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float KillzoneRangeCm = 0.0f;

	/** Bark-pool set id for class-flavored lines (content tier fills the pools). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	FName BarkSet;

	/**
	 * Per-class order modulation as data (GDD 9.5: traits/classes modulate
	 * parameters, orders stay unchanged in surface). Assault pushes past the
	 * ordered point by this many cm toward the order direction; 0 = obey exactly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float OrderPushDistanceCm = 0.0f;

	/**
	 * Cover-scorer lane bias (existing ring scorer, SPEC-P1-06): 0 = nearest
	 * cover wins; higher values prefer covered samples with a longer clear lane
	 * to the threat (Sniper overwatch preference).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class", meta = (ClampMin = 0))
	float CoverLaneBias = 0.0f;

	/** Auto-triage: move to downed squadmates and stabilize without an order (Medic true). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Class")
	bool bAutoTriage = false;
};

/** One enemy archetype (DT_EnemyArchetypes row, SPEC-P1-05 data). */
USTRUCT(BlueprintType)
struct FEclipseEnemyArchetypeRow : public FTableRowBase
{
	GENERATED_BODY()

	/** DT_BodyDefs row that dresses this archetype (step-2 character pipeline); NAME_None = capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy")
	FName BodyDef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 1))
	float Health = 60.0f;

	/** Damage per hit. Feel target: exposed player dies in ~2.5 s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float Damage = 10.0f;

	/** Sight radius (cm) for the perception stub. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float PerceptionRadius = 2500.0f;

	/** Seconds between shots. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0.05))
	float FireInterval = 0.8f;

	/** Pursuit stop distance (cm) — how close this archetype closes before holding to shoot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float EngageRange = 600.0f;

	/** Weaponless fallback strike range (cm) — a data mistake degrades to weak melee, never invincible (GDD 14.3.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Enemy", meta = (ClampMin = 0))
	float MeleeRange = 200.0f;
};

/**
 * Hoe dit wapen op één trekkerbeweging reageert — het woord dat rechtsonder naast
 * het magazijn hoort te staan (SINGLE / BURST / AUTO).
 *
 * NIET GEAUTHORD OP 01-08, en dat staat hier expliciet in plaats van dat het veld
 * er stil bij komt te staan. GEMETEN: `FireMode`, `SemiAuto`, `bAutomatic` en
 * `BurstCount` komen samen NUL keer voor in `Source/` en `Tools/`; DT_Weapons
 * draagt de kolom dus niet en de vier rijen uit `create_phase1_content.py` vullen
 * hem niet. Elke rij leest daarom vandaag `Unspecified`.
 *
 * Waarom hij er tóch staat, en waarom `Unspecified` een echte waarde is en geen
 * gaatje: de schermlaag hoort het verschil te kunnen zien tussen "dit wapen is
 * enkelschot" en "niemand heeft dit ooit ingevuld". Een enum die bij afwezigheid
 * stilletjes `Single` zou zeggen, laat het scherm iets beweren wat niemand heeft
 * besloten — precies de klasse fout waar `Sidearm_Scrap` uit voortkwam. Bij
 * `Unspecified` toont de HUD niets en degradeert luid.
 *
 * WELK wapen welke modus krijgt is een balansbeslissing over DT_Weapons en dus
 * van de owner; de bedrading eronder (rij -> feed -> bus -> payload) is compleet,
 * zodat het invullen van die kolom geen regel code meer kost.
 */
UENUM(BlueprintType)
enum class EEclipseWeaponFireMode : uint8
{
	/** DT_Weapons zegt er niets over. De schermlaag hoort dan te ZWIJGEN, niet te gokken. */
	Unspecified,
	Single,
	Burst,
	Automatic
};

/** One weapon platform (DT_Weapons row; Phase 1: one AR, one sidearm per feel targets). */
USTRUCT(BlueprintType)
struct FEclipseWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Zie EEclipseWeaponFireMode: vandaag voor elke rij `Unspecified`, bewust. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel")
	EEclipseWeaponFireMode FireMode = EEclipseWeaponFireMode::Unspecified;

	/** Damage per shot. Feel target: well-aimed player TTK vs. basic enemy ~0.6 s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 0))
	float Damage = 22.0f;

	/** Hitscan under 50 m (feel targets); projectile tier is Phase 2+. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 0))
	float RangeCm = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 0.05))
	float FireInterval = 0.15f;

	/**
	 * Hoe ver dit wapen te HOREN is (owner-opdracht 26-07, punt 1: het eerste
	 * schot verraadt je).
	 *
	 * Waarom 5000 en niet het zichtbereik: geluid hoort verder te dragen dan zicht,
	 * anders verandert schieten niets aan wat de vijand al wist. De waarneming van
	 * een vijand is 2500 cm (DT_EnemyArchetypes), dus dit is precies het dubbele —
	 * sluipen op 30 m blijft mogelijk, maar precies één schot lang.
	 *
	 * Per wapen bedoeld: een sniper hoort luider te zijn dan een AR en een demper
	 * stiller. Dat is de knop waarmee je dat afstelt.
	 *
	 * Referentie: The Division en Borderlands doen allebei radius-alarm vanaf de
	 * loop. Gears heeft geen stealth en is hier geen bruikbare referentie.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 0))
	float GunshotAlertRadiusCm = 5000.0f;

	// ---- WAPENPROFIEL (owner-opdracht 26-07 avond, punt 4) ------------------
	//
	// RESEARCH. Wat een wapen in Borderlands, Destiny 2 en Fortnite van een ander
	// onderscheidt, en welke daarvan alle drie gebruiken:
	//
	//   schade per kogel        alle drie
	//   vuursnelheid            alle drie (RPM of interval)
	//   magazijn + herladen     alle drie
	//   bereik MET schade-afval Destiny (Range = waar de afval begint), Fortnite;
	//                           Borderlands doet het per wapentype impliciet
	//   spreiding heup vs mik   Borderlands (Accuracy), Fortnite (first-shot)
	//   terugslag + stabiliteit Destiny (Stability + Recoil direction), Fortnite
	//   handling                Borderlands en Destiny allebei een eigen stat
	//   kritieke schade         alle drie
	//   kogels per schot        Borderlands en Fortnite (hagel)
	//
	// Overgenomen: alle negen. Wat NIET: elementaire schade (vuur/zuur/schok).
	// Dat is een ontwerpkeuze van formaat en staat als voorstel in HANDOFF.
	//
	// De velden hieronder zijn DATA; het gedrag landt in lagen. Laag A (dit) is
	// het profiel plus validatie, zodat een wapen zonder profiel niet stil
	// doorglipt.

	/** Magazijn. 0 = oneindig (het gedrag van vóór 26-07). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	int32 MagazineSize = 30;

	/** Herlaadtijd in seconden. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float ReloadSeconds = 2.2f;

	/**
	 * Schade-afval. Tot FalloffStartCm doet het wapen zijn volle schade; op
	 * RangeCm is het FalloffMinFraction daarvan. Dit is wat een DMR anders maakt
	 * dan een SMG zonder aan één schadegetal te komen.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float FalloffStartCm = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0.05, ClampMax = 1.0))
	float FalloffMinFraction = 0.5f;

	/** Spreiding in graden vanaf de heup en tijdens mikken. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float HipSpreadDegrees = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float AimSpreadDegrees = 0.6f;

	/** Extra spreiding terwijl je beweegt; straft rennen-en-schieten. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float MovingSpreadDegrees = 1.5f;

	/** Terugslag per schot: omhoog, en de horizontale uitslag eromheen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float RecoilPitchDegrees = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float RecoilYawDegrees = 0.15f;

	/** Stabiliteit: hoe snel het kruis terugzakt, in graden per seconde. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float RecoilRecoveryDegreesPerSecond = 6.0f;

	/** Handling: hoe lang het duurt voor dit wapen klaar is na een wissel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 0))
	float ReadySeconds = 0.5f;

	/**
	 * Kogels per trekkerbeweging (hagel > 1). **NIET GELEZEN** — bewust, en met
	 * een datum erbij zodat het niet stilletjes blijft liggen (26-07 avond).
	 *
	 * Geen van de vier verscheepte wapens heeft er meer dan één, dus aansluiten
	 * zou betekenen dat ik de vuurweg — spreiding, terugslag, magazijn, afval —
	 * openbreek voor gedrag dat vandaag niemand gebruikt. Het veld staat er omdat
	 * het bij het profiel hoort dat Borderlands en Fortnite allebei hebben, en het
	 * wordt aangesloten op de dag dat er een hagelwapen komt.
	 *
	 * De terugslag hoort dan per TREKKERBEWEGING te blijven en niet per kogel, en
	 * het magazijn telt één ronde af — anders kost één schot met acht korrels acht
	 * patronen.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel", meta = (ClampMin = 1))
	int32 PelletsPerShot = 1;

	/**
	 * Welke geluidsfamilie dit wapen gebruikt (FreeWeaponSounds, 26-07 avond).
	 * AssaultRifle / Handgun / Shotgun / GrenadeLauncher — de mappen van het pack.
	 *
	 * Als NAAM en niet als pad: het pad is een detail van waar het pack toevallig
	 * staat, de familie is wat het wapen IS. De validator controleert dat de naam
	 * bestaat, want een tikfout hier maakt een wapen stil.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel")
	FName SoundFamily = TEXT("AssaultRifle");

	/**
	 * Demper (owner-vraag 26-07 avond bij de drie sil_-varianten).
	 *
	 * RESEARCH — hoe het genre een demper laat meetellen:
	 *   Metal Gear Solid V  demper maakt schoten vrijwel niet-alarmerend; slijt op
	 *   Splinter Cell       gedempt pistool alarmeert alleen op enkele meters
	 *   Rainbow Six Siege   haalt de richtingsindicator wég en verkleint het bereik
	 *   Escape from Tarkov  hoorbare afstand van ~500 m naar ~100-150 m
	 *   Ghost Recon         gedempt vuur alarmeert alleen dichtbij
	 *
	 * Wat ze ALLEMAAL doen: een demper maakt je niet stil, hij verkleint de
	 * radius fors — grofweg naar een kwart tot een derde. Wat ik daaruit overneem
	 * is niet dat getal maar de VERHOUDING waar het om draait: gedempt hoort ONDER
	 * de waarnemingsafstand van een vijand (2500 cm) te liggen. Anders verraadt een
	 * gedempt schot je alsnog aan iemand die je niet eens kon zien, en dan is de
	 * demper alleen een ander geluid.
	 *
	 * De validator bewaakt precies dat, en niet een getal dat ik verzin.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel")
	bool bSuppressed = false;

	/** Eén zin die zegt waar dit wapen voor is; de validator eist hem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel")
	FText RoleSummary;

	/**
	 * DE NAAM DIE DE SPELER LEEST. "Foundry AR", niet `AR_Foundry`.
	 *
	 * Gemeld 31-07 door de screenshot-inspecteur: op `HUD_wapen_E_na_wissel.png`
	 * staat letterlijk **`Sidearm_Scrap`** op het scherm — mét underscore. Dat is
	 * de ruwe RIJNAAM uit deze tabel, en een rijnaam is een sleutel: hij is
	 * hoofdlettergevoelig, hij mag geen spaties dragen, en hij verandert nooit
	 * meer zodra er data aan hangt. Precies de eigenschappen die hem ongeschikt
	 * maken als leestekst.
	 *
	 * Zelfde klasse fout als de `45434C53` die dezelfde avond op het scherm stond:
	 * een interne sleutel die als mensentaal wordt gebruikt. De reparatie is niet
	 * "vervang de underscore" maar "geef het ding een naam" — een sleutel opsmukken
	 * blijft een sleutel tonen.
	 *
	 * HIER EN NIET IN DE HUD, en dat is de laagscheiding: welk wapen dit IS, is
	 * data; hoe het op het scherm past, is de schermlaag. De HUD leest dit veld en
	 * valt terug op de rijnaam als het leeg is — luid degraderen (14.3.5), want een
	 * lege naam mag geen leeg vakje opleveren.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon|Profiel")
	FText DisplayName;

	/** Locational damage stub (GDD 8.2): headshot multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon", meta = (ClampMin = 1))
	float HeadshotMultiplier = 2.5f;

	/**
	 * HET ZICHTBARE WAPEN (owner-besluit O-5 "volledig", 31-07).
	 *
	 * Zelf geauthord in Blender (Tools/blender/gen_weapons.py), één per rij in deze
	 * tabel, Borderlands-verhoudingen en door de toon-master (15.5). De conventie
	 * die het asset draagt en die de code dus NIET hoeft te raden:
	 *   oorsprong = de greep, +X = de loop, +Z = boven.
	 *
	 * TERUGVAL OP NAAM als dit veld leeg is: /Game/Art/Weapons/SM_Weapon_<rijnaam>.
	 * Dat is er niet voor de sier — DT_Weapons wordt door een commandlet gevuld en
	 * die rijen bestaan al zonder dit veld. Zonder terugval zou een wapen dat
	 * gewoon op schijf staat stil onzichtbaar blijven tot iemand de tabel opnieuw
	 * genereert, en dat is precies de klasse fout die dit hele dossier is
	 * (14.3.5: luid degraderen). De code LOGT welke van de twee wegen hij nam.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Weapon")
	TSoftObjectPtr<UStaticMesh> Mesh;
};
