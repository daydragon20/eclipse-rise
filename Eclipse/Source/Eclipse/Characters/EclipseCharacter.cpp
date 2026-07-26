#include "Characters/EclipseCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/EclipseHealthAttributeSet.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Camera/CameraComponent.h"
#include "Characters/EclipseAnimInstance.h"
#include "Characters/EclipseCharacterMovementComponent.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Eclipse.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationInvokerComponent.h"

namespace
{
	/** Best-guess base-color texture of a pack material (restyle detail source). */
	UTexture* FindBaseColorTexture(UMaterialInterface* Material)
	{
		if (Material == nullptr)
		{
			return nullptr;
		}
		TArray<UTexture*> Textures;
		Material->GetUsedTextures(Textures, EMaterialQualityLevel::High, true, ERHIFeatureLevel::SM6, true);
		UTexture* Fallback = nullptr;
		for (UTexture* Texture : Textures)
		{
			const FString TexName = Texture->GetName();
			if (TexName.Contains(TEXT("Diff")) || TexName.Contains(TEXT("Base")) || TexName.Contains(TEXT("Alb"))
				|| TexName.Contains(TEXT("_BC")) || TexName.Contains(TEXT("Color")) || TexName.EndsWith(TEXT("_D")))
			{
				return Texture;
			}
			if (Fallback == nullptr)
			{
				Fallback = Texture;
			}
		}
		return Fallback;
	}
}

AEclipseCharacter::AEclipseCharacter(const FObjectInitializer& ObjectInitializer)
	// Het movement component vervangen moet HIER: ACharacter maakt het in zijn
	// eigen constructor aan, dus later omzetten kan niet meer. Zie
	// UEclipseCharacterMovementComponent — het draagt de richtingsstraf, waar UE
	// zelf niets voor heeft.
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UEclipseCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Event-driven by default (GDD 14.2): nothing here needs per-frame work — but
	// the camera blend DOES need a tick for the fraction of a second it runs, and
	// bCanEverTick=false made that impossible.
	//
	// SetActorTickEnabled(true) is a no-op on an actor whose tick function was
	// never registered, so RefreshCameraTargets() asked for a tick it could never
	// get: TargetArmLength moved to 0 and the boom stayed at 300 forever. Dat trof
	// drie dingen die de speler ziet — 1e/3e persoon, mikken, en de Command
	// Mode-uitzoom — en alle drie zetten hun DOEL netjes, dus in de code leek het
	// te kloppen.
	//
	// bCanEverTick=true met bStartWithTickEnabled=false houdt de bedoeling van
	// 12.4 exact overeind: de tickfunctie bestaat, maar staat uit tot er een blend
	// loopt, en UpdateCameraBlend zet hem zelf weer uit als hij klaar is.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	HealthAttributes = CreateDefaultSubobject<UEclipseHealthAttributeSet>(TEXT("HealthAttributes"));

	// Navmesh generates around characters (invoker model): the graybox level
	// needs no authored bounds volume, and big districts never pay for navmesh
	// where nobody walks (GDD 12.4 budget thinking).
	CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));

	// Third-person camera (owner playtest 2026-07-25). There was no camera layer at
	// all before this: no spring arm, no camera component, so the engine fell back
	// to the pawn's own view point and the player looked out of the body. For a
	// third-person action game (GDD 01 / 15.1) that is a hole, and it blocked the
	// R3 verdict outright — targeting readability and comfort cannot be judged
	// through a camera that does not exist.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = ThirdPersonArmLength;
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 65.0f);
	BaseCameraSocketZ = 65.0f;
	// The boom follows where the player LOOKS; the camera then just rides its end.
	// Both true would double-apply control rotation and the view would swing twice
	// as fast as the stick.
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	// Zie DA_CharacterTuning::CameraLagMaxDistance: zonder klem loopt de camera
	// evenredig met de snelheid achter, en dan schaalt het personage met snelheid.
	CameraBoom->CameraLagMaxDistance = 6.0f;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 20.0f;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ViewCamera->bUsePawnControlRotation = false;
	ViewCamera->SetFieldOfView(ThirdPersonFOV);

	// CAMERA-RELATIEF (owner-opdracht 26-07, punt 8).
	//
	// Hiervoor stond hier bOrientRotationToMovement=true met de redenering "het
	// lichaam kijkt waar het loopt". Die redenering klopt voor een third-person
	// ADVENTURE (Uncharted, Tomb Raider), maar niet voor een third-person SHOOTER,
	// en dat verschil is precies wat de owner meldde: alleen achteruit duwen liet
	// het personage volledig omdraaien.
	//
	// Wat de referentie doet — Gears, The Division en Borderlands doen dit alle
	// drie hetzelfde: het personage kijkt waar de CAMERA kijkt, en beweegt daar
	// omheen met vier gangcycli (vooruit, achteruit, links, rechts). Achteruit en
	// zijwaarts zijn trager dan vooruit. Dat is geen stijlkeuze maar een gevolg
	// van het genre: je moet kunnen schieten waar je kijkt terwijl je wegloopt,
	// en dat kan niet als je lichaam meedraait met je looprichting.
	//
	// Wat we NIET overnemen: die drie draaien het lichaam ook mee als je alleen de
	// camera beweegt en stilstaat. Zonder draai-animatie (die zit niet in de
	// packs) levert dat voetslip op — je ruilt een zichtbaar defect voor een
	// ander. Daarom volgt het lichaam de camera alleen terwijl er bewegingsinvoer
	// is; stilstaand kijken blijft aan de bovenlichaam-aimoffset over. Zie
	// SetOrientationFollowsCameraNow().
	//
	// De constructor zelf houdt het AI-model: een vijand of squadmate heeft geen camera,
	// dus "kijk waar je loopt" is voor hen precies goed. Het camera-relatieve model
	// wordt aangezet door NotifyControllerChanged zodra een SPELER dit lichaam
	// bestuurt — anders zou een squadmate zijn AI-focus als looprichting nemen en
	// zijwaarts naar zijn doel schuifelen.
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Crouch is a designed control, not an engine extra (GDD 04_core_gameplay:
	// "Crouch = stealth default"), and ApplyTuning already feeds it a speed from
	// DA_CharacterTuning. But FNavAgentProperties::bCanCrouch defaults to FALSE,
	// so ACharacter::Crouch() silently did nothing and Ctrl / pad-B was a dead key
	// — the tuning knob was being applied to a capability that was switched off.
	// Found by the in-game test guide while writing the crouch step's expectation,
	// which is exactly what that guide exists for.
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	// Kopschot-volume (owner-opdracht 26-07, punt 2). 14 cm straal: een mensenhoofd
	// is ongeveer 22 cm breed, dus dit is iets ruimer dan levensecht — bewust, want
	// een hitbox die exact het model volgt leest als "ik raakte hem wél" terwijl de
	// game nee zegt. Shooters maken hoofdhitboxen standaard iets genereuzer dan het
	// silhouet; wie 'm strakker wil zet 'm hier smaller.
	// Aan de CAPSULE en niet aan de mesh, want een graybox-lichaam heeft vaak geen
	// mesh — en juist dat lichaam is waar de owner tegenaan schoot. Zonder een
	// meetkundige terugval zou "kopschoten werken nu" alleen gelden voor de drie
	// lichamen die toevallig een aangeklede mesh hebben, en dat is precies het
	// soort halve reparatie dat deze sessie steeds aan het licht brengt.
	// ApplyBodyDef verhangt hem naar de echte hoofdbot zodra die er is.
	HeadHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("HeadHitbox"));
	HeadHitbox->SetupAttachment(GetCapsuleComponent());
	HeadHitbox->InitSphereRadius(14.0f);
	// Puur een meetvolume: het mag niets blokkeren, niets duwen en geen overlaps
	// genereren. De schotlijn wordt er in code tegenaan gerekend.
	HeadHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadHitbox->SetGenerateOverlapEvents(false);
}

void AEclipseCharacter::PlaceHeadHitboxOnCapsule()
{
	if (HeadHitbox == nullptr)
	{
		return;
	}
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (Capsule == nullptr)
	{
		return;
	}

	// Bovenin de capsule, één straal onder de kruin: dat is waar een hoofd zit bij
	// een staande capsule van 176 cm. Hij schaalt mee met hurken, want de
	// capsulehoogte doet dat ook — gehurkt zit je hoofd lager, en daar hoort de
	// hitbox te volgen.
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	HeadHitbox->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	HeadHitbox->SetRelativeLocation(FVector(0.0f, 0.0f, HalfHeight - HeadHitbox->GetScaledSphereRadius()));
	bHeadHitboxAttached = true;
}

bool AEclipseCharacter::ShotLineHitsHead(const FVector& Start, const FVector& End) const
{
	if (HeadHitbox == nullptr || !bHeadHitboxAttached)
	{
		return false;
	}

	// Dichtste punt van het SEGMENT tot het middelpunt van de bol. Segment en niet
	// oneindige lijn: een schot dat vóór het hoofd stopt (bereik op, muur ertussen)
	// is geen kopschot.
	const FVector Centre = HeadHitbox->GetComponentLocation();
	const FVector Closest = FMath::ClosestPointOnSegment(Centre, Start, End);
	return FVector::DistSquared(Closest, Centre) <= FMath::Square(HeadHitbox->GetScaledSphereRadius());
}

void AEclipseCharacter::PlayOneShotPose(UAnimSequence* Clip, float Duration, float PeakWeight, const TCHAR* PoseName)
{
	if (Clip == nullptr)
	{
		// LUID DEGRADEREN (14.3.5), en dat was hier stil. Een ontbrekende eenmalige
		// pose is geen crash maar wel een gat dat je ZIET: de speler herlaadt en zijn
		// armen bewegen niet. Belica (het spelerslichaam) heeft bijvoorbeeld geen
		// enkele reload-take — nul in het hele pack — en dat merkte niets.
		//
		// Eén regel per lichaam en niet per aanroep: een schietpose valt 6,67 keer
		// per seconde, en dan is een waarschuwing een logbom in plaats van een
		// bevinding.
		// ALLEEN VOOR AANGEKLEDE LICHAMEN. Een capsule die nooit een body def kreeg
		// (testdummies, doelwitten) heeft per definitie geen enkele pose, en dat is
		// geen bevinding maar de opstelling. Zonder deze poort vuurde de
		// waarschuwing 42 keer per suite-run op iets wat niemand hoeft te
		// repareren — en dan leest niemand hem meer.
		if (!bBodyDefApplied)
		{
			return;
		}

		// De NAAM erbij, anders moet je alsnog gaan zoeken welke van de vier het is.
		if (!WarnedMissingPoses.Contains(PoseName))
		{
			WarnedMissingPoses.Add(PoseName);
			EclipseDegradation::Note(TEXT("pose ontbreekt"),
				FString::Printf(TEXT("%s: %s"), *GetName(), PoseName));
			UE_LOG(LogEclipse, Warning,
				TEXT("Animatie: %s mist de pose '%s' — die handeling is hoorbaar en leesbaar, maar niet ZICHTBAAR."),
				*GetName(), PoseName);
		}
		return;
	}
	if (UEclipseAnimInstance* Anim = Cast<UEclipseAnimInstance>(GetMesh() != nullptr ? GetMesh()->GetAnimInstance() : nullptr))
	{
		Anim->PlayOneShotPose(Clip, Duration, PeakWeight);
	}
}

void AEclipseCharacter::PlayShootPose()
{
	// 0,12 s: korter dan het vuurinterval (0,15) zodat automatisch vuur een
	// doorlopende beweging geeft in plaats van een pose die zichzelf steeds
	// herstart en daardoor bevriest.
	PlayOneShotPose(LocomotionSet.Shoot, 0.12f, 0.85f, TEXT("schieten"));
}

void AEclipseCharacter::PlayReloadPose(float Seconds)
{
	// Half gewicht, net als de schietpose: herladen is een handeling van de armen
	// en mag de looppose niet overrulen. Wie herlaadt terwijl hij rent, moet nog
	// steeds rennen.
	PlayOneShotPose(LocomotionSet.Reload, FMath::Max(Seconds, 0.1f), 0.85f, TEXT("herladen"));
}

void AEclipseCharacter::PlayTurnPose(bool bTurningRight)
{
	UAnimSequence* Clip = bTurningRight ? LocomotionSet.TurnRight : LocomotionSet.TurnLeft;
	if (Clip == nullptr)
	{
		return; // geen take voor dit lichaam: draaien zonder animatie, zoals voorheen
	}
	// Eén keer hardop WELKE take er draait en of hij additief is. De
	// locomotie-audit vroeg of de draaianimatie echt speelt, en "de pose
	// resolvet" is daar geen antwoord op — een additieve take gedraagt zich
	// anders dan een volledige, en juist dat verschil liet vandaag een
	// personage verdwijnen. Eén regel, niet per draai: dit is een feit over het
	// lichaam en geen gebeurtenis.
	if (!bLoggedTurnTake)
	{
		bLoggedTurnTake = true;
		UE_LOG(LogEclipse, Display, TEXT("Draaitake: %s (%s, %.2f s)"),
			*Clip->GetName(),
			Clip->IsValidAdditive() ? TEXT("ADDITIEF") : TEXT("volledige pose"),
			Clip->GetPlayLength());
	}
	// NIET de lengte van de take, en dat is 26-07 laat gemeten.
	//
	// Idle_Turn_90_Right duurt 4,00 s. De gewichtscurve is sin(alpha*PI), dus het
	// piekgewicht van 0,85 valt op t=2,00 s — precies waar de take al is
	// uitgezakt (heuptwist 3,4 gr, tegen een piek van 34,8 gr rond t=0,80). En
	// omdat een volledige pose de andere samples schaalt met 1-gewicht, zakt de
	// loopcyclus daar terug naar 15%. Gevolg: na elke draai loop je seconden lang
	// nauwelijks, terwijl de draaibeweging zelf op half gewicht voorbijkwam.
	//
	// Nagemeten over alle tien de Turn-takes in ParagonLtBelica: GEEN ENKELE
	// draagt netto rotatie (0,0-3,5 gr). Het zijn voetenschuifels die bij een
	// extern aangedreven draai horen — precies wat wij doen — maar ze zijn
	// geauthord op 2,7 tot 5,0 s.
	//
	// 0,8 s is de bovenkant van waar de referentie een kwartslag-draai in
	// stilstand legt (Gears, The Division: 0,4-0,8 s). De blend loopt op nul af,
	// dus afkappen geeft geen sprong: sin(PI) is nul, ongeacht waar de take dan
	// staat.
	const float TurnPoseSeconds = FMath::Min(Clip->GetPlayLength(), 0.8f);
	PlayOneShotPose(Clip, FMath::Max(TurnPoseSeconds, 0.15f), 0.85f, TEXT("draaien"));
}

void AEclipseCharacter::PlayHitReactPose()
{
	// 0,25 s en vol gewicht. De referentie (Gears, Division, Destiny) zit op
	// 0,2-0,35 s voor een flinch; korter leest als ruis, langer onderbreekt het
	// vuurgevecht. Vol gewicht omdat dit een VIJAND is die je neerschiet: dat is
	// waar de klap gezien wordt, en daar mag hij duidelijk zijn.
	PlayOneShotPose(LocomotionSet.HitReact, 0.25f, 1.0f, TEXT("klap"));
}

void AEclipseCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Alleen het bestuurde lichaam krijgt het camera-relatieve model. Hier en niet
	// in de constructor, omdat een lichaam pas bij bezetting weet of er een speler
	// of een AI achter zit — en de game mode spawnt beide uit dezelfde klasse.
	SetCameraRelativeOrientation(IsPlayerControlled());
}

void AEclipseCharacter::SetCameraRelativeOrientation(bool bEnabled)
{
	bCameraRelativeOrientation = bEnabled;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr)
	{
		return;
	}

	// Uit = terug naar het AI-model (kijk waar je loopt). Aan = het lichaam draait
	// naar de camera, maar pas zodra er bewegingsinvoer is; zie
	// SetOrientationFollowsCameraNow.
	Movement->bOrientRotationToMovement = !bEnabled;
	Movement->bUseControllerDesiredRotation = false;
}

void AEclipseCharacter::SetOrientationFollowsCameraNow(bool bFollowing)
{
	// Lui aanzetten in plaats van vertrouwen op NotifyControllerChanged.
	//
	// Die haak bleek niet betrouwbaar het moment te markeren waarop een SPELER dit
	// lichaam overneemt: in het testharnas bezit een echte player controller de
	// pawn, maar de eerste achteruit-meting kwam er toch uit op 180 graden gedraaid
	// — het AI-model stond nog aan. Deze functie wordt alleen aangeroepen vanuit de
	// invoerlaag van de speler, dus het feit DAT hij wordt aangeroepen bewijst al
	// wat de haak moest vaststellen.
	if (!bCameraRelativeOrientation)
	{
		SetCameraRelativeOrientation(true);
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr)
	{
		return;
	}

	// De hele reden dat dit per frame schakelt in plaats van één keer: zonder
	// draai-animatie levert stilstaand meedraaien met de camera voetslip op — de
	// voeten staan stil terwijl het bekken roteert. De owner was daar expliciet
	// over bij het turn-in-place-punt: een zichtbaar defect inruilen voor een
	// ander is geen winst. Dus draait het lichaam alleen mee terwijl je LOOPT, en
	// neemt de bovenlichaam-aimoffset het stilstaande kijken over.
	Movement->bUseControllerDesiredRotation = bFollowing;
}

void AEclipseCharacter::PostInitializeComponents()
{
	// Elke capsule krijgt meteen een hoofd (26-07, punt 2); ApplyBodyDef verhangt
	// hem later naar de echte hoofdbot als het lichaam er een heeft.
	PlaceHeadHitboxOnCapsule();
	Super::PostInitializeComponents();

	AbilitySystem->InitAbilityActorInfo(this, this);
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UEclipseHealthAttributeSet::GetHealthAttribute())
		.AddUObject(this, &AEclipseCharacter::HandleHealthChanged);
}

void AEclipseCharacter::ApplyTuning(const UEclipseCharacterTuningAsset* Tuning)
{
	if (Tuning == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("%s: no character tuning asset — engine defaults stay (GDD 14.3.5)."), *GetName());
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = Tuning->RunSpeed;
	Movement->MaxWalkSpeedCrouched = Tuning->CrouchSpeed;
	// Zonder deze regel bleef de gehurkte capsule op de engine-default van 40
	// (80 cm) staan — kruiphoogte, niet hurkhoogte. Zie het veld voor de afweging.
	Movement->SetCrouchedHalfHeight(Tuning->CrouchedHalfHeightCm);
	// Feel-audit: alles hieronder stond op de engine-default omdat het nooit was
	// gezet. Die defaults zijn niet voor een third-person shooter bedoeld — Epic's
	// eigen template overschrijft ze allemaal — en samen zijn ze de reden dat lopen
	// als schaatsen leest. Waarden en herkomst staan in DA_CharacterTuning.
	Movement->MaxAcceleration = Tuning->MaxAcceleration;
	Movement->BrakingDecelerationWalking = Tuning->BrakingDecelerationWalking;
	Movement->BrakingDecelerationFalling = Tuning->BrakingDecelerationFalling;
	Movement->RotationRate = FRotator(0.0f, Tuning->BodyRotationRateYaw, 0.0f);
	Movement->JumpZVelocity = Tuning->JumpZVelocity;
	Movement->AirControl = Tuning->AirControl;
	Movement->MinAnalogWalkSpeed = Tuning->MinAnalogWalkSpeed;
	// Fase 3: remmen en richtingswisselen. Zonder bUseSeparateBrakingFriction is
	// BrakingFriction dood gewicht — de drie horen bij elkaar.
	Movement->bUseSeparateBrakingFriction = Tuning->bUseSeparateBrakingFriction;
	Movement->BrakingFriction = Tuning->BrakingFriction;
	Movement->BrakingFrictionFactor = Tuning->BrakingFrictionFactor;
	Movement->GroundFriction = Tuning->GroundFriction;
	if (UEclipseCharacterMovementComponent* Directional = Cast<UEclipseCharacterMovementComponent>(Movement))
	{
		Directional->StrafeSpeedRatio = Tuning->StrafeSpeedRatio;
		Directional->AimSpeed = Tuning->AimSpeed;
		Directional->BackwardSpeedRatio = Tuning->BackwardSpeedRatio;
	}
	CoyoteTimeSeconds = Tuning->CoyoteTimeSeconds;
	JumpInputBufferSeconds = Tuning->JumpInputBufferSeconds;
	// Traversal (TRV-01/05). WalkableFloorAngle blijft bewust op de engine-default:
	// 44,77 graden is een industrieconstante die bij Quake, Half-Life, Source en UE
	// onafhankelijk terugkeert — zie de toelichting in DA_CharacterTuning.
	Movement->MaxStepHeight = Tuning->MaxStepHeightCm;
	Movement->PerchRadiusThreshold = Tuning->PerchRadiusThresholdCm;

	// VERIFICATIE, en die is er gekomen na een terechte owner-melding: "niets is
	// veranderd in de game". Een opgeslagen DataAsset wint van een C++-default voor
	// velden die het asset al KENDE; nieuw toegevoegde velden bestaan niet in een
	// oud asset en vallen dan terug op de CDO. Welke van die twee hier speelt is
	// niet te zien aan de code en niet aan de asset-datum - alleen aan wat er
	// werkelijk op het component staat NA het toepassen. Dus staat dat er nu.
	// Eén regel per pawn bij spawn; geen tick, geen kosten tijdens spelen.
	UE_LOG(LogEclipse, Display,
		TEXT("Tuning TOEGEPAST op %s uit '%s': accel %.0f · draai %.0f gr/s · luchtcontrole %.2f · sprong %.0f · valrem %.0f · min-analoog %.0f · loop %.0f"),
		*GetName(), *Tuning->GetName(),
		Movement->MaxAcceleration, Movement->RotationRate.Yaw, Movement->AirControl,
		Movement->JumpZVelocity, Movement->BrakingDecelerationFalling,
		Movement->MinAnalogWalkSpeed, Movement->MaxWalkSpeed);

	// En luid als het asset iets anders zegt dan de code bedoelde (14.3.5). Dit is
	// precies het defect dat de owner een hele testronde kostte: tunen dat de game
	// niet bereikt. Vergelijken met de CDO-default, want dát is wat de code zegt.
	if (const UEclipseCharacterTuningAsset* Defaults = GetDefault<UEclipseCharacterTuningAsset>())
	{
		auto WarnIfStale = [this, Tuning](const TCHAR* Label, float FromAsset, float FromCode)
		{
			if (!FMath::IsNearlyEqual(FromAsset, FromCode, 0.01f))
			{
				UE_LOG(LogEclipse, Warning,
					TEXT("Tuning VEROUDERD: %s staat op %.2f in %s maar de code bedoelt %.2f. Het asset wint — draai het setup-script of werk het asset bij."),
					Label, FromAsset, *Tuning->GetName(), FromCode);
			}
		};
		WarnIfStale(TEXT("MaxAcceleration"), Tuning->MaxAcceleration, Defaults->MaxAcceleration);
		WarnIfStale(TEXT("BodyRotationRateYaw"), Tuning->BodyRotationRateYaw, Defaults->BodyRotationRateYaw);
		WarnIfStale(TEXT("AirControl"), Tuning->AirControl, Defaults->AirControl);
		WarnIfStale(TEXT("JumpZVelocity"), Tuning->JumpZVelocity, Defaults->JumpZVelocity);
		WarnIfStale(TEXT("MinAnalogWalkSpeed"), Tuning->MinAnalogWalkSpeed, Defaults->MinAnalogWalkSpeed);
		WarnIfStale(TEXT("BrakingFriction"), Tuning->BrakingFriction, Defaults->BrakingFriction);
		WarnIfStale(TEXT("BrakingFrictionFactor"), Tuning->BrakingFrictionFactor, Defaults->BrakingFrictionFactor);
		WarnIfStale(TEXT("GroundFriction"), Tuning->GroundFriction, Defaults->GroundFriction);
		WarnIfStale(TEXT("BackwardSpeedRatio"), Tuning->BackwardSpeedRatio, Defaults->BackwardSpeedRatio);
		WarnIfStale(TEXT("CameraProbeSize"), Tuning->CameraProbeSize, Defaults->CameraProbeSize);
		WarnIfStale(TEXT("CoyoteTimeSeconds"), Tuning->CoyoteTimeSeconds, Defaults->CoyoteTimeSeconds);
		WarnIfStale(TEXT("JumpInputBufferSeconds"), Tuning->JumpInputBufferSeconds, Defaults->JumpInputBufferSeconds);
		WarnIfStale(TEXT("MaxStepHeightCm"), Tuning->MaxStepHeightCm, Defaults->MaxStepHeightCm);
		WarnIfStale(TEXT("ViewPitchMin"), Tuning->ViewPitchMin, Defaults->ViewPitchMin);
		WarnIfStale(TEXT("StickDeadzone"), Tuning->StickDeadzone, Defaults->StickDeadzone);
		WarnIfStale(TEXT("StickYawSpeed"), Tuning->StickYawSpeed, Defaults->StickYawSpeed);
		WarnIfStale(TEXT("MouseLookScale"), Tuning->MouseLookScale, Defaults->MouseLookScale);
		WarnIfStale(TEXT("AimAssistStrength"), Tuning->AimAssistStrength, Defaults->AimAssistStrength);
		WarnIfStale(TEXT("ThirdPersonArmLength"), Tuning->ThirdPersonArmLength, Defaults->ThirdPersonArmLength);
		WarnIfStale(TEXT("CameraLagMaxDistance"), Tuning->CameraLagMaxDistance, Defaults->CameraLagMaxDistance);
	}
	InitializeHealth(Tuning->MaxHealth);

	// Camera framing is data, like every other feel number (GDD 14.2). The
	// constructor's values are the same defaults, so an untuned pawn still frames
	// correctly rather than collapsing to a zero-length boom.
	ThirdPersonArmLength = Tuning->ThirdPersonArmLength;
	CommandModeArmLength = Tuning->CommandModeArmLength;
	CommandModeCameraRise = Tuning->CommandModeCameraRise;
	BaseSocketOffsetZ = Tuning->CameraSocketOffset.Z;
	FirstPersonFOV = Tuning->CameraFOV;
	ThirdPersonFOV = Tuning->ThirdPersonFOV;
	ViewToggleBlendTime = Tuning->ViewToggleBlendTime;
	TunedCameraLagSpeed = Tuning->CameraLagSpeed;
	if (CameraBoom != nullptr)
	{
		CameraBoom->SocketOffset = Tuning->CameraSocketOffset;
		CameraBoom->CameraLagSpeed = TunedCameraLagSpeed;
		CameraBoom->CameraLagMaxDistance = Tuning->CameraLagMaxDistance;
		CameraBoom->bEnableCameraLag = !bCameraLagSuspended && TunedCameraLagSpeed > 0.0f;
		CameraBoom->ProbeSize = Tuning->CameraProbeSize;
	}
	RefreshCameraTargets();
	// Land on the target immediately here: tuning arrives at spawn, and blending
	// in from the constructor defaults would show as a lurch on the first frame.
	if (CameraBoom != nullptr)
	{
		CameraBoom->TargetArmLength = TargetArmLength;
	}
	if (ViewCamera != nullptr)
	{
		ViewCamera->SetFieldOfView(TargetFOV);
	}
}

void AEclipseCharacter::SetFirstPerson(bool bNewFirstPerson)
{
	if (bFirstPerson == bNewFirstPerson)
	{
		return;
	}
	bFirstPerson = bNewFirstPerson;
	// Hide only the HEAD, not the whole body (owner playtest 2026-07-25: "in 1e
	// persoon zie ik letterlijk niets van mezelf"). SetOwnerNoSee on the mesh was
	// the cheap first pass and it is wrong for a game with no first-person arms:
	// it trades looking-inside-your-skull for looking-at-nothing. Hiding the head
	// bone keeps arms, weapon and legs on screen and costs no new asset.
	// It is honestly a stopgap: a third-person rig seen from its own eye socket
	// has no arm animation authored for that view, so it will read as odd from
	// some angles. Real FP arms are their own asset job; this at least gives you
	// a body while that does not exist.
	if (USkeletalMeshComponent* Body = GetMesh())
	{
		Body->SetOwnerNoSee(false);
		static const FName HeadBone(TEXT("head"));
		if (Body->GetBoneIndex(HeadBone) != INDEX_NONE)
		{
			bFirstPerson
				? Body->HideBoneByName(HeadBone, EPhysBodyOp::PBO_None)
				: Body->UnHideBoneByName(HeadBone);
		}
		else if (bFirstPerson)
		{
			// Loud rather than silently headless-or-not (14.3.5): a rig without a
			// bone called "head" would otherwise quietly keep its head on screen.
			UE_LOG(LogEclipse, Warning, TEXT("%s: geen bone 'head' op dit skelet — 1e persoon toont het hoofd."), *GetName());
		}
	}
	RefreshCameraTargets();
}

void AEclipseCharacter::SetCommandModeCamera(bool bActive)
{
	if (bCommandModeCamera == bActive)
	{
		return;
	}
	bCommandModeCamera = bActive;
	RefreshCameraTargets();
}

FEclipseFeelSample AEclipseCharacter::SampleFeelState() const
{
	// Feel-audit-instrument. De owner meldt "personage schaalt met snelheid:
	// piepklein bij langzaam lopen, groter bij sprinten". De mesh-schaal wordt
	// maar EEN keer gezet (ApplyBodyDef), dus die kan dat niet doen — het moet de
	// camera zijn. Deze meting zet alle kandidaten naast elkaar op hetzelfde
	// moment, zodat zichtbaar is welke meebeweegt in plaats van dat iemand het
	// moet raden.
	//
	// De beslissende regel is CameraToPawnCm, en die wordt uit de twee WERELD-
	// transforms gehaald in plaats van uit TargetArmLength. Camera-lag en de
	// collision-probe zitten allebei ná de boomlengte: een afgeleid getal zou
	// precies de term wegpoetsen die met de snelheid meebeweegt.
	FEclipseFeelSample Sample;
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const USkeletalMeshComponent* Body = GetMesh();

	Sample.SpeedCm = Movement != nullptr ? Movement->Velocity.Size2D() : 0.0f;
	Sample.MeshScale = Body != nullptr ? Body->GetRelativeScale3D().X : 0.0f;
	Sample.BoomArmLength = CameraBoom != nullptr ? CameraBoom->TargetArmLength : 0.0f;
	Sample.BoomTargetArmLength = TargetArmLength;
	Sample.FieldOfView = ViewCamera != nullptr ? ViewCamera->FieldOfView : 0.0f;
	Sample.TargetFieldOfView = TargetFOV;
	Sample.SocketOffsetZ = CameraBoom != nullptr ? CameraBoom->SocketOffset.Z : 0.0f;
	Sample.CapsuleHalfHeightCm = GetCapsuleComponent() != nullptr ? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.0f;

	if (ViewCamera != nullptr)
	{
		Sample.CameraToPawnCm = FVector::Dist(ViewCamera->GetComponentLocation(), GetActorLocation());
	}
	if (Sample.CameraToPawnCm > KINDA_SMALL_NUMBER && Sample.CapsuleHalfHeightCm > 0.0f)
	{
		Sample.ApparentHeightDegrees = FMath::RadiansToDegrees(
			2.0f * FMath::Atan(Sample.CapsuleHalfHeightCm / Sample.CameraToPawnCm));
		if (Sample.FieldOfView > KINDA_SMALL_NUMBER)
		{
			Sample.ApparentFractionOfView = Sample.ApparentHeightDegrees / Sample.FieldOfView;
		}
	}
	return Sample;
}

FString AEclipseCharacter::DescribeFeelState() const
{
	const FEclipseFeelSample S = SampleFeelState();
	return FString::Printf(
		TEXT("snelheid %.0f cm/s · mesh-schaal %.3f · boom %.0f (doel %.0f) · camera->pawn %.1f cm · FOV %.1f (doel %.1f) · schijnbare hoogte %.2f gr (%.1f%% van beeld) · socketZ %.0f · lag %s · modus %s"),
		S.SpeedCm, S.MeshScale, S.BoomArmLength, S.BoomTargetArmLength,
		S.CameraToPawnCm, S.FieldOfView, S.TargetFieldOfView,
		S.ApparentHeightDegrees, S.ApparentFractionOfView * 100.0f, S.SocketOffsetZ,
		(CameraBoom != nullptr && CameraBoom->bEnableCameraLag)
			? *FString::Printf(TEXT("%.0f"), CameraBoom->CameraLagSpeed) : TEXT("uit"),
		bFirstPerson ? TEXT("1e") : (bCommandModeCamera ? TEXT("command") : TEXT("3e")));
}

FString AEclipseCharacter::DescribeMovementState() const
{
	// Uitgelezen van het COMPONENT, niet uit het asset: het verschil tussen die
	// twee is precies het defect dat de owner een testronde kostte.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr)
	{
		return TEXT("(geen bewegingscomponent)");
	}
	const UEclipseCharacterMovementComponent* Directional = Cast<UEclipseCharacterMovementComponent>(Movement);
	return FString::Printf(
		TEXT("loop %.0f · accel %.0f (%.2f s aanloop) · rem %.1fx%.1f/%.0f · draai %.0f gr/s · grondwrijving %.0f · zijwaarts %.2f / achteruit %.2f · stap %.0f · sprong %.0f (lucht %.2f)"),
		Movement->MaxWalkSpeed, Movement->MaxAcceleration,
		Movement->MaxAcceleration > KINDA_SMALL_NUMBER ? Movement->MaxWalkSpeed / Movement->MaxAcceleration : 0.0f,
		Movement->BrakingFriction, Movement->BrakingFrictionFactor, Movement->BrakingDecelerationWalking,
		Movement->RotationRate.Yaw, Movement->GroundFriction,
		Directional != nullptr ? Directional->StrafeSpeedRatio : 1.0f,
		Directional != nullptr ? Directional->BackwardSpeedRatio : 1.0f,
		Movement->MaxStepHeight, Movement->JumpZVelocity, Movement->AirControl);
}

void AEclipseCharacter::SetCameraLagSuspended(bool bSuspended)
{
	if (bCameraLagSuspended == bSuspended || CameraBoom == nullptr)
	{
		return;
	}
	bCameraLagSuspended = bSuspended;
	CameraBoom->CameraLagSpeed = TunedCameraLagSpeed;
	CameraBoom->bEnableCameraLag = !bSuspended && TunedCameraLagSpeed > 0.0f;
}

void AEclipseCharacter::SetAiming(bool bNewAiming)
{
	if (bAiming == bNewAiming)
	{
		return;
	}
	bAiming = bNewAiming;
	RefreshCameraTargets();
}

void AEclipseCharacter::RefreshCameraTargets()
{
	// Command Mode wins over the view toggle: pulling back to read the field is
	// the whole point of the hold, and doing that from inside the head is not a
	// reading position.
	if (bCommandModeCamera)
	{
		TargetArmLength = CommandModeArmLength;
		TargetFOV = ThirdPersonFOV;
	}
	else
	{
		TargetArmLength = bFirstPerson ? 0.0f : ThirdPersonArmLength;
		TargetFOV = bFirstPerson ? FirstPersonFOV : ThirdPersonFOV;
		if (bAiming)
		{
			// Aiming pulls in and narrows: 0.55 of the boom and 0.8 of the FOV.
			// Relative to whichever view you are in, so ADS works the same in
			// first and third person instead of needing two more constants.
			TargetArmLength *= 0.55f;
			TargetFOV *= 0.80f;
		}
	}

	const bool bNeedsBlend =
		(CameraBoom != nullptr && !FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetArmLength, 0.5f))
		|| (ViewCamera != nullptr && !FMath::IsNearlyEqual(ViewCamera->FieldOfView, TargetFOV, 0.1f));
	// Tick exists only for the duration of a blend (GDD 12.4). A camera that
	// costs a tick every frame forever, to move twice a minute, is not free at
	// four squadmates plus enemies sharing this class.
	SetActorTickEnabled(bNeedsBlend);
}

void AEclipseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateLandingDip(DeltaSeconds);
	UpdateCameraBlend(DeltaSeconds);
}

void AEclipseCharacter::UpdateLandingDip(float DeltaSeconds)
{
	if (LandingDipSecondsLeft <= 0.0f || CameraBoom == nullptr)
	{
		return;
	}

	LandingDipSecondsLeft = FMath::Max(LandingDipSecondsLeft - DeltaSeconds, 0.0f);
	// Sin-envelope, net als de eenmalige poses: meteen omlaag, dan terug. Een
	// lineaire terugloop leest als een lift, een sin als een veer.
	const float Alpha = 1.0f - (LandingDipSecondsLeft / 0.22f);
	const float Dip = LandingDipSecondsLeft > 0.0f
		? LandingDipCm * FMath::Sin(FMath::Clamp(Alpha, 0.0f, 1.0f) * PI)
		: 0.0f;

	// NIET zelf de socket schrijven. Eerste versie deed dat, en de dip mat 0,0 cm:
	// UpdateCameraBlend interpoleert diezelfde Z elke tick terug naar zijn eigen
	// doel en zette daarna de tick uit. Twee schrijvers op één waarde is altijd
	// een verliezer, en dat was ik. De dip is nu een OPTELLING die de blend zelf
	// toepast — zie het eind van UpdateCameraBlend.
	CurrentDipCm = Dip;

	if (LandingDipSecondsLeft <= 0.0f)
	{
		LandingDipCm = 0.0f;
		CurrentDipCm = 0.0f;
	}
}

void AEclipseCharacter::UpdateCameraBlend(float DeltaSeconds)
{
	if (CameraBoom == nullptr || ViewCamera == nullptr)
	{
		SetActorTickEnabled(false);
		return;
	}

	// Constant-rate blend over ViewToggleBlendTime rather than an exponential ease.
	//
	// LET OP: dit commentaar beweerde dat de overgang "altijd even lang duurt zodat
	// de speler hem leert". Dat is niet wat een constante SNELHEID doet — de tijd
	// schaalt met de afstand. Gemeten: de 1e-persoons-sprong (300 cm) kost 0,100 s
	// en de Command Mode-sprong (220 cm) 0,258 s. Die tweede is bovendien trager
	// dan zijn afstand suggereert omdat Command Mode de wereld op 0,30 zet en deze
	// blend met een gedilateerde delta rekent — hij erft dus de slow-motion die de
	// modus zelf aanzet. Zie Eclipse.Feel.Camera.ViewToggleAndAdsActuallyMoveTheCamera.
	//
	// Niets aan gewijzigd: welke van de twee (constante tijd of constante snelheid)
	// beter voelt, is een ontwerpkeuze en staat op de owner-lijst.
	const float ArmRate = ViewToggleBlendTime > KINDA_SMALL_NUMBER
		? FMath::Max(ThirdPersonArmLength, CommandModeArmLength) / ViewToggleBlendTime
		: BIG_NUMBER;
	const float FovRate = ViewToggleBlendTime > KINDA_SMALL_NUMBER
		? FMath::Abs(FirstPersonFOV - ThirdPersonFOV) / ViewToggleBlendTime
		: BIG_NUMBER;

	CameraBoom->TargetArmLength = FMath::FInterpConstantTo(
		CameraBoom->TargetArmLength, TargetArmLength, DeltaSeconds, ArmRate);
	ViewCamera->SetFieldOfView(FMath::FInterpConstantTo(
		ViewCamera->FieldOfView, TargetFOV, DeltaSeconds, FMath::Max(FovRate, 1.0f)));

	// Command Mode also raises the eye line. Height, not pitch: the boom runs on
	// pawn control rotation, so pitch is the player's stick — a camera that tilts
	// itself mid-aim fights the person holding the controller.
	const float TargetRise = bCommandModeCamera ? CommandModeCameraRise : 0.0f;
	const float RiseRate = ViewToggleBlendTime > KINDA_SMALL_NUMBER
		? FMath::Max(CommandModeCameraRise, 1.0f) / ViewToggleBlendTime : BIG_NUMBER;
	FVector Offset = CameraBoom->SocketOffset;
	// Vanaf de ONGEDIPTE hoogte interpoleren, anders trekt de dip het doel mee en
	// komt de camera na een sprong lager te hangen dan hij begon.
	Offset.Z = FMath::FInterpConstantTo(Offset.Z + CurrentDipCm, BaseSocketOffsetZ + TargetRise, DeltaSeconds, RiseRate);
	const float SettledZ = Offset.Z;
	Offset.Z -= CurrentDipCm;
	CameraBoom->SocketOffset = Offset;

	// De landingsdip houdt de tick in leven: zonder deze voorwaarde zet de blend
	// zichzelf uit zodra hij klaar is en bevriest de camera halverwege de dip.
	if (LandingDipSecondsLeft <= 0.0f
		&& FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetArmLength, 0.5f)
		&& FMath::IsNearlyEqual(ViewCamera->FieldOfView, TargetFOV, 0.1f)
		&& FMath::IsNearlyEqual(SettledZ, BaseSocketOffsetZ + TargetRise, 0.5f))
	{
		SetActorTickEnabled(false);
	}
}

void AEclipseCharacter::ApplyBodyDef(const FEclipseBodyDefRow& BodyDef)
{
	USkeletalMesh* BodyMesh = BodyDef.Mesh.LoadSynchronous();
	if (BodyMesh == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("%s: body mesh %s missing — capsule body stands in (GDD 14.3.5)."),
			*GetName(), *BodyDef.Mesh.ToString());
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetSkeletalMesh(BodyMesh);
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, BodyDef.MeshZOffset));
	MeshComponent->SetRelativeRotation(FRotator(0.0f, BodyDef.MeshYaw, 0.0f));
	MeshComponent->SetRelativeScale3D(FVector(BodyDef.MeshScale));

	// DE POSE MOET ALTIJD TIKKEN, ook als de mesh niet gerenderd wordt.
	//
	// Gevonden 26-07 21:30 met de nieuwe zichtbaarheidscontrole: nul van de 123
	// botten bewoog in 0,35 s en de bounding box was 1 cm hoog. De standaard van
	// USkeletalMeshComponent is "tik de pose alleen als je gerenderd wordt", en
	// dan bevriest een personage dat de renderer even niet meetelt — en met een
	// bevroren pose klapt ook zijn omvang in, waardoor hij helemaal wegvalt.
	//
	// Voor ECLIPSE kan dat sowieso niet: de hoofd-hitbox hangt aan een BOT
	// (26-07, kopschoten). Een lichaam waarvan de botten niet bijwerken, heeft
	// zijn hoofd op de plek van vorig frame — dus dit is niet alleen een
	// zichtbaarheidskwestie maar ook een raakbaarheidskwestie.
	MeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// EN GEEN UPDATE-RATE-OPTIMALISATIE. URO knijpt de animatie-updates af voor wat
	// de renderer weinig ziet, en interpoleert ertussen. Voor een spel waarin een
	// HITBOX aan een bot hangt is dat geen prestatiewinst maar een
	// correctheidsprobleem: een kopschot zou dan op een geïnterpoleerde schedel
	// landen. En het is de tweede verdachte voor het personage dat bij stilstand
	// verdwijnt — gemeten omvang 182,9 cm rennend tegen 1,0 cm stilstaand.
	MeshComponent->bEnableUpdateRateOptimizations = false;

	// De kopschot-hitbox op de hoofdbot hangen (26-07, punt 2). Hier en niet in de
	// constructor: pas met een mesh is er een skelet om aan te hechten.
	//
	// Vier kandidaatnamen omdat de lichamen uit drie verschillende packs komen en
	// niemand het over botnamen eens is. Vindt hij er geen, dan telt dit lichaam
	// geen kopschoten — en dat wordt gezegd, want stil geen kopschot is precies de
	// toestand die vandaag gerepareerd wordt (14.3.5).
	bHeadHitboxAttached = false;
	if (HeadHitbox != nullptr)
	{
		static const FName HeadBoneCandidates[] = { TEXT("head"), TEXT("Head"), TEXT("neck_01"), TEXT("Bip001Head") };
		for (const FName& Candidate : HeadBoneCandidates)
		{
			if (MeshComponent->DoesSocketExist(Candidate))
			{
				HeadHitbox->AttachToComponent(MeshComponent,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale, Candidate);
				bHeadHitboxAttached = true;
				break;
			}
		}
		if (!bHeadHitboxAttached)
		{
			UE_LOG(LogEclipse, Warning,
				TEXT("%s: geen hoofdbot gevonden op %s — kopschoten tellen op dit lichaam als rompschoten (14.3.5)."),
				*GetName(), *BodyDef.Mesh.ToString());
		}
	}

	ApplyBodyDefAnimation(BodyDef, BodyMesh, *MeshComponent);

	// Toon restyle (15.5 asset policy): every slot re-dressed with the cel
	// master; the pack's own base texture stays as luminance detail, the
	// faction palette supplies the hue. This also puts bodies on the same
	// exposure tier as the unlit district — lit PBR underexposes to a
	// silhouette against the emissive world (step-2 QC forensics).
	if (BodyDef.bToonRestyle)
	{
		UMaterialInterface* ToonMaster = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToon.M_EclipseToon"));
		if (ToonMaster == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("%s: M_EclipseToon missing — pack materials stay (GDD 14.3.5)."), *GetName());
			return;
		}
		// Keep in sync with EclipseGrayboxBuilder's SunRotation (-25, 55): the
		// cel bands must tell the same sun story as the district around them.
		const FVector SunTravel = FRotator(-25.0f, 55.0f, 0.0f).Vector();
		for (int32 SlotIndex = 0; SlotIndex < MeshComponent->GetNumMaterials(); ++SlotIndex)
		{
			UTexture* BaseTexture = FindBaseColorTexture(MeshComponent->GetMaterial(SlotIndex));
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(ToonMaster, this);
			Mid->SetVectorParameterValue(TEXT("LitColor"), BodyDef.TintLit);
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), BodyDef.TintShade);
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunTravel, 0.0f)));
			Mid->SetScalarParameterValue(TEXT("EmissiveScale"), 10.0f);
			Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
			if (BaseTexture != nullptr)
			{
				Mid->SetTextureParameterValue(TEXT("AlbedoTex"), BaseTexture);
				// Generic gain: character textures hover ~0.3 linear; bodies are
				// small on screen, so the unmeasured normalization is acceptable
				// at this tier (the measured discipline applies to big surfaces).
				Mid->SetScalarParameterValue(TEXT("AlbedoGain"), 3.2f);
				Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.9f);
			}
			MeshComponent->SetMaterial(SlotIndex, Mid);
		}
	}
}

void AEclipseCharacter::ApplyBodyDefAnimation(const FEclipseBodyDefRow& BodyDef, const USkeletalMesh* BodyMesh, USkeletalMeshComponent& MeshComponent)
{
	// Until 2026-07-25 this was four lines: single-node mode plus a looping idle.
	// Every body in the game therefore slid across the district in a standing
	// pose — player, squad and every enemy — which is the defect this block
	// closes (GDD 14.5 debug-grade locomotion).
	const USkeleton* MeshSkeleton = BodyMesh != nullptr ? BodyMesh->GetSkeleton() : nullptr;

	// Resolution happens HERE and not in the anim instance because this is the
	// one place that knows which mesh the body just put on. The packs behind
	// DT_BodyDefs share the UE4-Mannequin skeleton FAMILY but each ships its own
	// USkeleton asset, and a clip evaluated against a foreign skeleton does not
	// throw — it produces a mangled pose. So a mismatch is rejected loudly and
	// the body drops one rung instead of being quietly broken (GDD 14.3.5).
	// bAllowAdditive: eenmalige poses (klap, schot, herladen, draai) MOGEN additief
	// zijn — een klap is per definitie een verschil bovenop je gangpose, en zo zijn
	// ze in de packs ook geauthord. Locomotie mag het NOOIT: die is de basispose
	// zelf, en additief sampelen laat het personage inklappen (26-07: de owner zag
	// zijn personage verdwijnen bij stilstand).
	auto ResolveClip = [this, MeshSkeleton](const TSoftObjectPtr<UAnimSequence>& SoftClip, const TCHAR* SlotName,
		bool bAllowAdditive = false) -> UAnimSequence*
	{
		if (SoftClip.IsNull())
		{
			return nullptr;
		}
		UAnimSequence* Clip = SoftClip.LoadSynchronous();
		if (Clip == nullptr)
		{
			EclipseDegradation::Note(TEXT("animatie niet geladen"),
				FString::Printf(TEXT("%s: %s (%s)"), *GetName(), SlotName, *SoftClip.ToString()));
			UE_LOG(LogEclipse, Warning, TEXT("%s: %s take %s did not load — locomotion drops a rung (GDD 14.3.5)."),
				*GetName(), SlotName, *SoftClip.ToString());
			return nullptr;
		}
		// ADDITIEVE TAKES WEIGEREN. Een additieve animatie is een VERSCHIL ten
		// opzichte van een basispose, geen pose op zich. Sample je hem als volledige
		// pose, dan vallen alle botten naar de oorsprong en klapt het personage in.
		//
		// Gevonden 26-07 21:30: de owner meldde "bij stilstand is het personage
		// helemaal weg". Gemeten omvang 182,9 cm rennend tegen 1,0 cm stilstaand, en
		// de idle van ParagonLtBelica heet gewoon "Idle" maar is additief. Bij
		// stilstand heeft die clip gewicht 1,0 — vandaar dat het alleen daar te zien
		// was.
		if (Clip->IsValidAdditive() && !bAllowAdditive)
		{
			EclipseDegradation::Note(TEXT("animatie is additief"),
				FString::Printf(TEXT("%s: %s (%s)"), *GetName(), SlotName, *SoftClip.ToString()));
			UE_LOG(LogEclipse, Warning,
				TEXT("%s: %s take %s is ADDITIEF en kan geen volledige pose zijn — geweigerd (GDD 14.3.5)."),
				*GetName(), SlotName, *SoftClip.ToString());
			return nullptr;
		}

		if (!EclipseLocomotion::IsUsableOn(Clip->GetSkeleton(), MeshSkeleton))
		{
			EclipseDegradation::Note(TEXT("animatie afgewezen op skelet"),
				FString::Printf(TEXT("%s: %s (%s)"), *GetName(), SlotName, *SoftClip.ToString()));
			UE_LOG(LogEclipse, Warning,
				TEXT("%s: %s take %s is authored for skeleton %s but this body wears %s — REJECTED, locomotion drops a rung (GDD 14.3.5)."),
				*GetName(), SlotName, *Clip->GetName(),
				*GetPathNameSafe(Clip->GetSkeleton()), *GetPathNameSafe(MeshSkeleton));
			return nullptr;
		}
		return Clip;
	};

	LocomotionSet = FEclipseLocomotionSet();
	LocomotionSet.Idle = ResolveClip(BodyDef.IdleAnim, TEXT("idle"));
	LocomotionSet.Walk = ResolveClip(BodyDef.WalkAnim, TEXT("walk"));
	LocomotionSet.Run = ResolveClip(BodyDef.RunAnim, TEXT("run"));
	// Richtingsvarianten (26-07, punt 8). Ontbreken mag: de proxy valt dan terug op
	// de vooruit-cyclus. Geen aparte waarschuwing per richting — dat zou bij een
	// pack zonder zijcycli vier regels per lichaam opleveren, en de tier-melding
	// hieronder zegt al wat er te weinig is.
	LocomotionSet.WalkBack = ResolveClip(BodyDef.WalkBackAnim, TEXT("walk-back"));
	LocomotionSet.WalkLeft = ResolveClip(BodyDef.WalkLeftAnim, TEXT("walk-left"));
	LocomotionSet.WalkRight = ResolveClip(BodyDef.WalkRightAnim, TEXT("walk-right"));
	LocomotionSet.RunBack = ResolveClip(BodyDef.RunBackAnim, TEXT("run-back"));
	LocomotionSet.RunLeft = ResolveClip(BodyDef.RunLeftAnim, TEXT("run-left"));
	LocomotionSet.RunRight = ResolveClip(BodyDef.RunRightAnim, TEXT("run-right"));

	// Kruislingse terugval: mist een gang zijn zijcyclus maar heeft de ANDERE gang
	// hem wel, gebruik die. Gemeten op de verscheepte lichamen (26-07): de speler
	// (ParagonLtBelica) heeft er 3 van de 6, en zonder deze regel moonwalkt hij in
	// precies die drie gevallen. Een looppas op renttempo ziet er hooguit gehaast
	// uit — de gangklok schaalt hem toch al — en dat is onvergelijkbaar veel beter
	// dan achteruit lopen met een vooruit-cyclus.
	auto FillFrom = [](TObjectPtr<UAnimSequence>& Target, const TObjectPtr<UAnimSequence>& Source)
	{
		if (Target == nullptr)
		{
			Target = Source;
		}
	};
	FillFrom(LocomotionSet.WalkBack, LocomotionSet.RunBack);
	FillFrom(LocomotionSet.WalkLeft, LocomotionSet.RunLeft);
	FillFrom(LocomotionSet.WalkRight, LocomotionSet.RunRight);
	FillFrom(LocomotionSet.RunBack, LocomotionSet.WalkBack);
	FillFrom(LocomotionSet.RunLeft, LocomotionSet.WalkLeft);
	FillFrom(LocomotionSet.RunRight, LocomotionSet.WalkRight);
	LocomotionSet.Death = ResolveClip(BodyDef.DeathAnim, TEXT("death"));
	LocomotionSet.Shoot = ResolveClip(BodyDef.ShootAnim, TEXT("shoot"), /*bAllowAdditive=*/true);
	LocomotionSet.HitReact = ResolveClip(BodyDef.HitReactAnim, TEXT("hit-react"), /*bAllowAdditive=*/true);
	LocomotionSet.Reload = ResolveClip(BodyDef.ReloadAnim, TEXT("herladen"), /*bAllowAdditive=*/true);
	bBodyDefApplied = true;
	LocomotionSet.TurnLeft = ResolveClip(BodyDef.TurnLeftAnim, TEXT("draai links"), /*bAllowAdditive=*/true);
	LocomotionSet.TurnRight = ResolveClip(BodyDef.TurnRightAnim, TEXT("draai rechts"), /*bAllowAdditive=*/true);
	LocomotionSet.CrouchTransition = ResolveClip(BodyDef.CrouchTransitionAnim, TEXT("crouch-transition"));

	const EEclipseLocomotionTier Tier = LocomotionSet.GetTier();
	if (Tier >= EEclipseLocomotionTier::OneGait)
	{
		MeshComponent.SetAnimInstanceClass(UEclipseAnimInstance::StaticClass());
		if (UEclipseAnimInstance* Locomotion = Cast<UEclipseAnimInstance>(MeshComponent.GetAnimInstance()))
		{
			// SetAnimInstanceClass is a no-op when the class is already set, and
			// the SetSkeletalMesh above re-created the instance anyway — so push
			// the set explicitly rather than trusting either path.
			Locomotion->SetLocomotionSet(LocomotionSet);
			return;
		}
		UE_LOG(LogEclipse, Warning,
			TEXT("%s: the locomotion anim instance did not spawn — single-node idle stands in (GDD 14.3.5)."), *GetName());
	}

	// The fallback deliberately uses the RAW idle, not the skeleton-checked one.
	// The strict gate above exists because THIS layer evaluates clips itself and
	// a foreign skeleton would give it a mangled pose; single-node playback is
	// the engine's own path and has been running these same rows since the
	// step-2 pipeline landed. Gating it too would take working bodies away
	// (DT_BodyDefs still has rows whose mesh and takes come from different
	// packs) — a locomotion pass may not cost anyone a pose they already had.
	if (UAnimSequence* RawIdle = BodyDef.IdleAnim.LoadSynchronous())
	{
		if (Tier <= EEclipseLocomotionTier::IdleOnly)
		{
			UE_LOG(LogEclipse, Warning,
				TEXT("%s: no gait take usable on this body's skeleton — single-node idle stands in and this body WILL slide (GDD 14.3.5)."),
				*GetName());
		}
		MeshComponent.SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComponent.PlayAnimation(RawIdle, /*bLooping*/ true);
		return;
	}

	UE_LOG(LogEclipse, Warning, TEXT("%s: no idle take at all for %s — the mesh draws its ref pose (GDD 14.3.5)."),
		*GetName(), *BodyDef.Mesh.ToString());
}

void AEclipseCharacter::RequestJump()
{
	Jump();
	// Al in de lucht? Dan is dit een druk VÓÓR de landing. Onthouden, en bij het
	// raken van de grond alsnog uitvoeren (JMP-08). Zonder dit moet de speler op
	// precies de landingsframe drukken en is elke druk daarvoor gewoon weg.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement != nullptr && Movement->IsFalling() && GetWorld() != nullptr)
	{
		JumpPressedWhileFallingAtSeconds = GetWorld()->GetTimeSeconds();
	}
}

void AEclipseCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PrevCustomMode);

	// Het moment waarop we de grond verlieten ZONDER te springen is het enige dat
	// coyote time nodig heeft. bWasJumping onderscheidt de twee: wie sprong heeft
	// zijn sprong al gehad en verdient geen tweede.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement != nullptr && Movement->IsFalling() && PrevMovementMode == MOVE_Walking && GetWorld() != nullptr)
	{
		LeftGroundAtSeconds = bWasJumping ? -1.0 : GetWorld()->GetTimeSeconds();
	}

	// Landing helemaal rond (Super heeft ResetJumpState al gedaan): nu mag de
	// gebufferde sprong alsnog af.
	if (bBufferedJumpPending && Movement != nullptr && !Movement->IsFalling())
	{
		bBufferedJumpPending = false;
		Jump();
	}
}

void AEclipseCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	// 0,3 s: de capsule krimpt onmiddellijk, dus de pose moet het verschil
	// overbruggen zonder de speler te laten wachten. Halve piek — dit is een
	// houdingswissel, geen klap.
	PlayOneShotPose(LocomotionSet.CrouchTransition, 0.3f, 0.6f, TEXT("hurken"));
	PlaceHeadHitboxOnCapsule(); // gehurkt zit het hoofd lager
}

void AEclipseCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	PlayOneShotPose(LocomotionSet.CrouchTransition, 0.3f, 0.6f, TEXT("hurken"));
	PlaceHeadHitboxOnCapsule();
}

void AEclipseCharacter::Landed(const FHitResult& Hit)
{
	// LANDINGSDIP (locomotie-audit 26-07, punt 12). Er is geen landingstake in de
	// packs, dus het lichaam kan niet door de knieën. Wat wél kan zonder assets is
	// de CAMERA laten zakken — dat is precies wat Borderlands doet, en het leest
	// als gewicht dat aankomt.
	//
	// Geschaald met de valsnelheid: van een stoeprand stappen hoort niet hetzelfde
	// te voelen als van een dak springen. Boven de 1200 cm/s (ruim een verdieping)
	// loopt hij niet verder op, anders duikt de camera bij een lange val door de
	// vloer.
	const float FallSpeed = FMath::Abs(static_cast<float>(GetVelocity().Z));
	if (FallSpeed > 300.0f)
	{
		LandingDipCm = FMath::GetMappedRangeValueClamped(
			FVector2f(300.0f, 1200.0f), FVector2f(2.0f, 12.0f), FallSpeed);
		LandingDipSecondsLeft = 0.22f;
		SetActorTickEnabled(true);
	}

	Super::Landed(Hit);
	LeftGroundAtSeconds = -1.0;

	if (JumpPressedWhileFallingAtSeconds < 0.0 || GetWorld() == nullptr)
	{
		return;
	}
	const double Age = GetWorld()->GetTimeSeconds() - JumpPressedWhileFallingAtSeconds;
	JumpPressedWhileFallingAtSeconds = -1.0;
	// NIET hier Jump() aanroepen, en dat is geen stijlkeuze: Landed() draait
	// binnen ProcessLanded, en daarna zet ACharacter::OnMovementModeChanged nog
	// ResetJumpState() — die wist bPressedJump weer. De gebufferde sprong zou dus
	// stil verdwijnen. Onthouden en pas afvuren als de landing helemaal rond is.
	bBufferedJumpPending = Age <= JumpInputBufferSeconds;
}

bool AEclipseCharacter::CanJumpInternal_Implementation() const
{
	if (Super::CanJumpInternal_Implementation())
	{
		return true;
	}

	// Coyote time (JMP-07). UE zegt hier expliciet nee: JumpIsAllowedInternal eist
	// JumpCurrentCount + 1 < JumpMaxCount zodra je valt, en met JumpMaxCount = 1
	// is dat altijd onwaar. Wie een fractie te laat drukt na van een rand te
	// lopen, krijgt dus geen sprong maar een val — terwijl hij op het scherm nog
	// op de rand leek te staan.
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr || !Movement->IsFalling() || bIsCrouched
		|| LeftGroundAtSeconds < 0.0 || GetWorld() == nullptr)
	{
		return false;
	}

	// JumpCurrentCountPreJump en niet JumpCurrentCount, en dat is precies het
	// verschil tussen werken en niet werken: ACharacter::CheckJumpInput hoogt de
	// teller op VOORDAT het CanJump() vraagt ("if this is the first jump and we're
	// already falling, then increment the JumpCount to compensate"). Deze
	// override zag dus altijd 1 waar 0 stond, en weigerde elke coyote-sprong.
	// Gevonden door de meting: binnen het venster faalde hij, buiten het venster
	// gedroeg hij zich correct — dat patroon past bij geen enkele tijdsfout.
	const int32 CountBeforeThisPress = FMath::Min(JumpCurrentCount, JumpCurrentCountPreJump);
	if (CountBeforeThisPress != 0)
	{
		return false;
	}
	return (GetWorld()->GetTimeSeconds() - LeftGroundAtSeconds) <= CoyoteTimeSeconds;
}

void AEclipseCharacter::InitializeHealth(float MaxHealth)
{
	HealthAttributes->SetMaxHealth(MaxHealth);
	HealthAttributes->SetHealth(MaxHealth);
	bDowned = false;
}

void AEclipseCharacter::ReviveForMission()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	InitializeHealth(HealthAttributes->GetMaxHealth());
}

void AEclipseCharacter::ApplyDamage(float Amount, AEclipseCharacter* DamageInstigator, FName Cause)
{
	if (Amount <= 0.0f || bDowned)
	{
		return;
	}

	LastDamageCause = Cause;

	// De klap (26-07, punt 2, tweede helft). Hier en niet in het wapen, want
	// schade komt ook van andere kanten — melee, en straks vallen of vuur. Wie
	// geraakt wordt hoort te reageren, ongeacht waardoor.
	PlayHitReactPose();

	// "Er wordt op me geschoten" is een FEIT dat dit lichaam overkomt, en het is
	// wat een getrainde soldaat in beweging zet (26-07 avond, doctrine laag 3).
	// Hier en niet in het wapen, om dezelfde reden als de klap hierboven: schade
	// komt straks ook van melee, vallen of vuur, en dekking zoeken hoort daar
	// net zo goed op te volgen.
	OnHitTaken.Broadcast(DamageInstigator, Amount);

	// PLACEHOLDER(GDD 12.1): direct attribute write until damage GameplayEffects
	// land with the ability pass — the IncomingDamage meta attribute is already
	// in place as their landing zone (PostGameplayEffectExecute).
	const float NewHealth = FMath::Max(0.0f, HealthAttributes->GetHealth() - Amount);
	AbilitySystem->SetNumericAttributeBase(UEclipseHealthAttributeSet::GetHealthAttribute(), NewHealth);
}

float AEclipseCharacter::GetHealth() const
{
	return HealthAttributes->GetHealth();
}

void AEclipseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bDowned || Data.NewValue > 0.0f)
	{
		return;
	}

	bDowned = true;
	GetCharacterMovement()->DisableMovement();

	// Going down was entirely silent — including for the PLAYER. A self-playing
	// round walked into the checkpoint, lost 100 hp in 2.5 s and went down, and
	// the only trace in the log was the mission failing 0.5 s later. Anyone
	// reading Saved/Logs after a session had no way to see they died, let alone
	// to what. One line per body, once (bDowned latches above), so it stays a
	// fact and not a stream (GDD 14.3.5).
	UE_LOG(LogEclipse, Display, TEXT("%s DOWN: %s at %s (cause: %s)"),
		IsPlayerSide() ? TEXT("Player-side") : TEXT("Hostile"), *GetName(),
		*GetActorLocation().ToCompactString(),
		LastDamageCause.IsNone() ? TEXT("Unknown") : *LastDamageCause.ToString());

	// One fact, broadcast once: squad/mission listeners resolve dead-vs-wounded
	// at debrief (SPEC-P1-07); the body itself only reports.
	OnDowned.Broadcast(this, LastDamageCause.IsNone() ? FName(TEXT("Unknown")) : LastDamageCause);
}
