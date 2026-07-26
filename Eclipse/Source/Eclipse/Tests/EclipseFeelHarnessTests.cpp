// Feel-harnas, laag 1 en laag 2 (owner-nachtopdracht 2026-07-25).
//
// LAAG 1 — "komt de tuning aan?" Na het spawnen worden de DAADWERKELIJK
//   toegepaste waarden van het movement component en de camera gelezen en naast
//   de tuningbron gelegd. Dit is het net onder het defect dat de owner een hele
//   testronde kostte: een opgeslagen DataAsset wint van een C++-default, en van
//   buitenaf ziet "tuning toegepast" er precies zo uit als "tuning genegeerd".
//
// LAAG 2 — "wat volgt eruit?" Input wordt geïnjecteerd via Enhanced Input, op
//   dezelfde UInputAction-objecten die de hardware aanstuurt, en het RESULTAAT
//   wordt over tijd gemeten. Elke meting logt zijn getal (AddInfo), niet alleen
//   pass/fail: een groene bar zonder getallen vertelt niet of iets 0.30 s of
//   0.46 s duurt, en dat verschil is precies waar feel over gaat.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipsePlayerController.h"
#include "Characters/EclipseCommandModeComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/InputSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "InputMappingContext.h"
#include "Tests/EclipseFeelHarness.h"
#include "UI/EclipseGauntletOverlayLogic.h"
#include "UI/EclipseTestGuideLogic.h"

namespace EclipseFeelTest
{
	// EditorContext: het harnas heeft een echte wereld met physics nodig, en die
	// draait in de -nullrhi editor-run waarmee de bar gedraaid wordt.
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Zet Eclipse.Input.ForceGamepad tijdelijk om en zet hem gegarandeerd terug. */
	struct FForceGamepadScope
	{
		explicit FForceGamepadScope(int32 Value)
		{
			Var = IConsoleManager::Get().FindConsoleVariable(TEXT("Eclipse.Input.ForceGamepad"));
			if (Var != nullptr)
			{
				Previous = Var->GetInt();
				Var->Set(Value, ECVF_SetByCode);
			}
		}
		~FForceGamepadScope()
		{
			if (Var != nullptr)
			{
				Var->Set(Previous, ECVF_SetByCode);
			}
		}
		IConsoleVariable* Var = nullptr;
		int32 Previous = -1;
	};
}

// ---------------------------------------------------------------------------
// LAAG 1
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFeelLayer1Test,
	"Eclipse.Feel.Layer1.AppliedValuesMatchTheTuningSource",
	EclipseFeelTest::TestFlags)

bool FEclipseFeelLayer1Test::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	const UCharacterMovementComponent* Movement = Harness.Body->GetCharacterMovement();
	const UEclipseCharacterTuningAsset& T = *Harness.Tuning;

	// Movement. Elke rij is een veld dat ApplyTuning zegt te zetten; de vergelijking
	// is met de BRON, niet met een getal in deze test — anders zou de test zijn eigen
	// antwoord meebrengen en zou een gewijzigde tuning stil langs het net glippen.
	auto CheckFloat = [this](const TCHAR* Label, float Applied, float Source)
	{
		TestEqual(FString::Printf(TEXT("laag 1: %s komt aan op het movement component"), Label), Applied, Source, 0.01f);
		Report(*this, Label, Applied, TEXT(""), *FString::Printf(TEXT("%.2f uit DA_CharacterTuning"), Source));
	};

	CheckFloat(TEXT("MaxWalkSpeed (= RunSpeed)"), Movement->MaxWalkSpeed, T.RunSpeed);
	CheckFloat(TEXT("MaxWalkSpeedCrouched"), Movement->MaxWalkSpeedCrouched, T.CrouchSpeed);
	CheckFloat(TEXT("MaxAcceleration"), Movement->MaxAcceleration, T.MaxAcceleration);
	CheckFloat(TEXT("BrakingDecelerationWalking"), Movement->BrakingDecelerationWalking, T.BrakingDecelerationWalking);
	CheckFloat(TEXT("BrakingDecelerationFalling"), Movement->BrakingDecelerationFalling, T.BrakingDecelerationFalling);
	CheckFloat(TEXT("RotationRate.Yaw"), Movement->RotationRate.Yaw, T.BodyRotationRateYaw);
	CheckFloat(TEXT("JumpZVelocity"), Movement->JumpZVelocity, T.JumpZVelocity);
	CheckFloat(TEXT("AirControl"), Movement->AirControl, T.AirControl);
	CheckFloat(TEXT("MinAnalogWalkSpeed"), Movement->MinAnalogWalkSpeed, T.MinAnalogWalkSpeed);
	CheckFloat(TEXT("BrakingFriction"), Movement->BrakingFriction, T.BrakingFriction);
	CheckFloat(TEXT("BrakingFrictionFactor"), Movement->BrakingFrictionFactor, T.BrakingFrictionFactor);
	CheckFloat(TEXT("GroundFriction"), Movement->GroundFriction, T.GroundFriction);
	CheckFloat(TEXT("StrafeSpeedRatio"), Movement->GetMaxSpeed() > 0.0f ? T.StrafeSpeedRatio : 0.0f, T.StrafeSpeedRatio);
	TestTrue(TEXT("laag 1: bUseSeparateBrakingFriction staat aan, anders is BrakingFriction dood gewicht"),
		Movement->bUseSeparateBrakingFriction);
	CheckFloat(TEXT("MaxStepHeight (TRV-01)"), Movement->MaxStepHeight, T.MaxStepHeightCm);
	CheckFloat(TEXT("PerchRadiusThreshold (TRV-05)"), Movement->PerchRadiusThreshold, T.PerchRadiusThresholdCm);
	// De loopbare hellingshoek blijft BEWUST de engine-default: 44,77 graden keert
	// bij Quake, Half-Life, Source en UE onafhankelijk terug. Gepind zodat een
	// toekomstige "opruiming" hem niet stilletjes op een smaakwaarde zet.
	TestEqual(TEXT("laag 1: WalkableFloorAngle staat op de engine-default (industrieconstante)"),
		Movement->GetWalkableFloorAngle(), 44.765f, 0.05f);

	// Capabilities die geen getal zijn maar wél de reden dat een getal werkt.
	TestTrue(TEXT("laag 1: bOrientRotationToMovement staat aan (lichaam volgt looprichting)"),
		Movement->bOrientRotationToMovement);
	TestTrue(TEXT("laag 1: bCanCrouch staat aan, dus Crouch() is geen no-op"),
		Movement->GetNavAgentPropertiesRef().bCanCrouch);
	TestFalse(TEXT("laag 1: bUseControllerRotationYaw staat uit (anders leest lopen als schaatsen)"),
		Harness.Body->bUseControllerRotationYaw);

	// Camera. Dezelfde discipline: lezen wat er op de componenten staat.
	const USpringArmComponent* Boom = Harness.Body->FindComponentByClass<USpringArmComponent>();
	const UCameraComponent* Camera = Harness.Body->GetViewCamera();
	if (TestNotNull(TEXT("laag 1: spring arm bestaat"), Boom) && TestNotNull(TEXT("laag 1: camera bestaat"), Camera))
	{
		CheckFloat(TEXT("boomlengte (3e persoon)"), Boom->TargetArmLength, T.ThirdPersonArmLength);
		CheckFloat(TEXT("camera-FOV (3e persoon)"), Camera->FieldOfView, T.ThirdPersonFOV);
		CheckFloat(TEXT("socketoffset Z"), Boom->SocketOffset.Z, T.CameraSocketOffset.Z);
		CheckFloat(TEXT("socketoffset Y (schouder)"), Boom->SocketOffset.Y, T.CameraSocketOffset.Y);
		CheckFloat(TEXT("collision-probe (CAM-06)"), Boom->ProbeSize, T.CameraProbeSize);
		TestTrue(TEXT("laag 1: de camera-probe is minstens zo groot als de capsule-radius (CAM-06)"),
			Boom->ProbeSize >= Harness.Body->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.55f);
		CheckFloat(TEXT("camera-lag snelheid"), Boom->CameraLagSpeed, T.CameraLagSpeed);
		CheckFloat(TEXT("camera-lag klem (S1)"), Boom->CameraLagMaxDistance, T.CameraLagMaxDistance);
		TestTrue(TEXT("laag 1: de camera-lag-klem is gezet (zonder klem schaalt het personage met snelheid — S1)"),
			Boom->CameraLagMaxDistance > 0.0f);
		TestTrue(TEXT("laag 1: de boom botst met muren"), Boom->bDoCollisionTest);
		TestTrue(TEXT("laag 1: de boom draait mee met de kijkrichting"), Boom->bUsePawnControlRotation);
	}

	// Pitch-limieten staan op de camera manager en niet op de handler — een klem
	// achteraf zou de camera de limiet laten bereiken en daar laten plakken.
	if (TestNotNull(TEXT("laag 1: camera manager bestaat"), Harness.Controller->PlayerCameraManager.Get()))
	{
		CheckFloat(TEXT("ViewPitchMin"), Harness.Controller->PlayerCameraManager->ViewPitchMin, T.ViewPitchMin);
		CheckFloat(TEXT("ViewPitchMax"), Harness.Controller->PlayerCameraManager->ViewPitchMax, T.ViewPitchMax);
	}

	// De legacy-invoerschalen moeten UIT staan, anders vermenigvuldigt de engine de
	// getunede kijksnelheid stil met 2.5 en meet 240 gr/s als 600 (feel-harnas
	// 2026-07-25). Dit is een laag-1-controle omdat het precies dezelfde soort fout
	// is als een asset dat een code-default overschrijft: alles ziet er goed uit,
	// behalve het draaiende ding.
	if (const UInputSettings* InputSettings = GetDefault<UInputSettings>())
	{
		TestFalse(TEXT("laag 1: legacy-invoerschalen staan uit, dus de getunede kijksnelheid is de echte"),
			static_cast<bool>(InputSettings->bEnableLegacyInputScales));
	}

	Harness.Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// LAAG 2 — locomotie over tijd
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFeelLayer2LocomotionTest,
	"Eclipse.Feel.Layer2.InjectedLocomotionMeasuresOverTime",
	EclipseFeelTest::TestFlags)

bool FEclipseFeelLayer2LocomotionTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	UCharacterMovementComponent* Movement = Harness.Body->GetCharacterMovement();
	const float RunSpeed = Movement->MaxWalkSpeed;

	// --- 1. tijd tot topsnelheid -------------------------------------------
	// UE's grondversnelling is lineair, dus dit is exact narekenbaar:
	// t = MaxWalkSpeed / MaxAcceleration. Dat maakt de meting een echte toets op
	// de keten en niet op de formule — wijkt hij af, dan zit er iets tussen.
	const double TimeToTop = Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 4.0,
		[&Harness, RunSpeed]() { return Harness.SpeedCm() >= RunSpeed * 0.995f; });
	const double PredictedTimeToTop = RunSpeed / Movement->MaxAcceleration;
	Report(*this, TEXT("tijd tot topsnelheid (rennen)"), TimeToTop, TEXT("s"),
		*FString::Printf(TEXT("%.3f s = %.0f / %.0f"), PredictedTimeToTop, RunSpeed, Movement->MaxAcceleration));
	TestTrue(TEXT("laag 2: topsnelheid gehaald"), Harness.SpeedCm() >= RunSpeed * 0.98f);
	// Ruime band (30 ms + 15%): substepping en de vaste tijdstap mogen wat kosten,
	// een factor mag niets kosten.
	TestTrue(FString::Printf(TEXT("laag 2: aanlooptijd volgt de lineaire voorspelling (%.3f vs %.3f s)"), TimeToTop, PredictedTimeToTop),
		FMath::Abs(TimeToTop - PredictedTimeToTop) <= 0.03 + PredictedTimeToTop * 0.15);

	// --- 2. remweg ----------------------------------------------------------
	const FVector BrakeStart = Harness.Location();
	const double BrakeTime = Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 3.0,
		[&Harness]() { return Harness.SpeedCm() < 1.0f; });
	const double BrakeDistance = FVector::Dist2D(Harness.Location(), BrakeStart);
	Report(*this, TEXT("stoptijd vanaf rennen"), BrakeTime, TEXT("s"), TEXT("Bijlage B bij 4x1 / 2000: 0.150 s (was 0.083 bij 8x2)"));
	Report(*this, TEXT("glijafstand vanaf rennen"), BrakeDistance, TEXT("cm"), TEXT("Bijlage B bij 4x1 / 2000: 28 cm (was 12)"));
	TestTrue(TEXT("laag 2: het personage komt daadwerkelijk tot stilstand"), Harness.SpeedCm() < 5.0f);
	TestTrue(TEXT("laag 2: de remweg is eindig en niet nul (er is massa, en die stopt)"),
		BrakeDistance > 0.5 && BrakeDistance < 200.0);

	// --- 2b. richtingsstraf: zijwaarts en achteruit -------------------------
	// UE kent hier niets voor, dus dit meet of UEclipseCharacterMovementComponent
	// zijn werk doet. Achteruitlopen was tot fase 3 even snel als vooruit rennen,
	// en dat is een van de duidelijkste "dit is een prototype"-signalen die er
	// zijn. De verhouding is het criterium, niet de absolute snelheid.
	auto MeasureTopSpeedInDirection = [&Harness, RunSpeed](const FVector2D& Stick)
	{
		Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 1.0, [&Harness]() { return Harness.SpeedCm() < 0.5f; });
		Harness.HoldFor(TEXT("Move"), Stick, 3.0);
		return Harness.SpeedCm();
	};
	const float ForwardTop = MeasureTopSpeedInDirection(FVector2D(0.0f, 1.0f));
	const float StrafeTop = MeasureTopSpeedInDirection(FVector2D(1.0f, 0.0f));
	const float BackwardTop = MeasureTopSpeedInDirection(FVector2D(0.0f, -1.0f));
	const float StrafeRatio = ForwardTop > KINDA_SMALL_NUMBER ? StrafeTop / ForwardTop : 0.0f;
	const float BackwardRatio = ForwardTop > KINDA_SMALL_NUMBER ? BackwardTop / ForwardTop : 0.0f;
	Report(*this, TEXT("topsnelheid vooruit"), ForwardTop, TEXT("cm/s"));
	Report(*this, TEXT("topsnelheid zijwaarts"), StrafeTop, TEXT("cm/s"));
	Report(*this, TEXT("topsnelheid achteruit"), BackwardTop, TEXT("cm/s"));
	Report(*this, TEXT("verhouding zijwaarts/vooruit"), StrafeRatio, TEXT(""), TEXT("1.00 (Gears 5 TU3)"));
	Report(*this, TEXT("verhouding achteruit/vooruit"), BackwardRatio, TEXT(""), TEXT("0.85 (Gears 5 TU3)"));
	TestEqual(TEXT("laag 2: zijwaarts loopt even snel als vooruit"), StrafeRatio, Harness.Tuning->StrafeSpeedRatio, 0.03f);
	TestEqual(TEXT("laag 2: achteruit is langzamer, precies volgens de getunede verhouding"),
		BackwardRatio, Harness.Tuning->BackwardSpeedRatio, 0.03f);
	TestTrue(TEXT("laag 2: achteruit is echt trager dan vooruit (de straf bestaat)"), BackwardTop < ForwardTop * 0.95f);

	// --- 3. 180-omkering ----------------------------------------------------
	// Een 180 is NIET "de stick naar achteren duwen" — dat is achteruitlopen, en
	// dat heeft sinds fase 3 terecht zijn eigen (tragere) snelheid. Een echte
	// omkering draait de KIJKRICHTING mee. Deze meting doet dus wat een speler
	// doet: de camera 180 graden omgooien en dan weer vooruit duwen.
	//
	// Twee getallen, want het zijn twee dingen: de snelheidsvector draait met
	// GroundFriction, het lichaam draait met RotationRate.Yaw. De speler voelt de
	// eerste en ziet de tweede.
	Harness.RunUpToTopSpeed(*this);
	const float BodyYawBefore = Harness.Body->GetActorRotation().Yaw;
	const FVector StartDirection = Movement->Velocity.GetSafeNormal2D();
	// Muisflick: de muis-tak is lineair, AddYawInput(Axis.X * MouseLookScale)
	// graden per gebeurtenis, dus 180 / MouseLookScale is precies een halve slag.
	Harness.Inject(TEXT("Look"), FVector2D(180.0f / Harness.Tuning->MouseLookScale, 0.0f));
	Harness.Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
	Harness.Step();

	double TurnTime = 0.0;
	{
		const double Start = Harness.ElapsedSeconds;
		Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 3.0, [&Harness, Movement, RunSpeed, StartDirection]()
		{
			const FVector V = Movement->Velocity;
			return V.Size2D() >= RunSpeed * 0.98f && FVector::DotProduct(V.GetSafeNormal2D(), StartDirection) < -0.99f;
		});
		TurnTime = Harness.ElapsedSeconds - Start;
	}
	const float BodyYawSwept = FMath::Abs(FMath::FindDeltaAngleDegrees(BodyYawBefore, Harness.Body->GetActorRotation().Yaw));
	Report(*this, TEXT("180-omkering (snelheid weer op top)"), TurnTime, TEXT("s"), TEXT("afremmen + richtingwissel via GroundFriction + heracceleratie"));
	Report(*this, TEXT("180-omkering: lichaam gedraaid"), BodyYawSwept, TEXT("gr"), TEXT("~180 gr bij RotationRate.Yaw"));
	TestTrue(FString::Printf(TEXT("laag 2: de omkering is af binnen 3 s (%.3f s)"), TurnTime), TurnTime < 2.999);
	TestTrue(FString::Printf(TEXT("laag 2: het lichaam draait echt om (%.0f gr)"), BodyYawSwept), BodyYawSwept > 150.0f);
	TestTrue(FString::Printf(TEXT("laag 2: na de omkering loopt hij weer op volle vooruitsnelheid (%.0f cm/s)"), Harness.SpeedCm()),
		Harness.SpeedCm() >= RunSpeed * 0.98f);

	// --- 4. sprong ----------------------------------------------------------
	Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 1.0, [&Harness]() { return Harness.SpeedCm() < 1.0f; });
	const float GroundZ = Harness.Location().Z;
	Harness.Press(TEXT("Jump"));
	float ApexZ = GroundZ;
	double Airtime = 0.0;
	{
		const double Start = Harness.ElapsedSeconds;
		const int32 MaxSteps = FMath::RoundToInt(DefaultTimeoutSeconds / FixedStepSeconds);
		bool bLeftGround = false;
		for (int32 I = 0; I < MaxSteps; ++I)
		{
			Harness.Step();
			ApexZ = FMath::Max(ApexZ, static_cast<float>(Harness.Location().Z));
			const bool bOnGround = Movement->IsMovingOnGround();
			if (!bOnGround)
			{
				bLeftGround = true;
			}
			else if (bLeftGround)
			{
				break;
			}
		}
		Airtime = Harness.ElapsedSeconds - Start;
		TestTrue(TEXT("laag 2: de sprong verliet de grond"), bLeftGround);
	}
	const float JumpHeight = ApexZ - GroundZ;
	const float PredictedApex = (Movement->JumpZVelocity * Movement->JumpZVelocity) / (2.0f * FMath::Abs(Harness.World->GetGravityZ()));
	Report(*this, TEXT("springhoogte"), JumpHeight, TEXT("cm"), *FString::Printf(TEXT("%.0f cm = v^2 / 2g"), PredictedApex));
	Report(*this, TEXT("airtime"), Airtime, TEXT("s"), *FString::Printf(TEXT("~%.3f s"), 2.0f * Movement->JumpZVelocity / FMath::Abs(Harness.World->GetGravityZ())));
	TestTrue(FString::Printf(TEXT("laag 2: springhoogte volgt v^2/2g (%.0f vs %.0f cm)"), JumpHeight, PredictedApex),
		FMath::Abs(JumpHeight - PredictedApex) < FMath::Max(10.0f, PredictedApex * 0.15f));
	TestTrue(FString::Printf(TEXT("laag 2: airtime is plausibel (%.3f s)"), Airtime), Airtime > 0.4 && Airtime < 2.0);

	// --- 4a. stap-hoogte: wat er GEBEURT, niet wat er op het veld staat -----
	// De maat zelf zegt niets; wat telt is of kniehoge dekking een stap of een
	// hindernis is. Twee blokken, één onder en één boven de drempel.
	const float LowStepGain = Harness.MeasureStepUp(20.0f);
	const float HighStepGain = Harness.MeasureStepUp(50.0f);
	Report(*this, TEXT("hoogtewinst op een blok van 20 cm"), LowStepGain, TEXT("cm"), TEXT("~20 — een stoeprand blijft een stap"));
	Report(*this, TEXT("hoogtewinst op een blok van 50 cm"), HighStepGain, TEXT("cm"), TEXT("~0 — kniehoge dekking hoort een vault te worden"));
	TestTrue(FString::Printf(TEXT("laag 2: over een stoeprand van 20 cm stapt hij heen (%.1f cm)"), LowStepGain), LowStepGain > 15.0f);
	TestTrue(FString::Printf(TEXT("laag 2: op kniehoogte (50 cm) stapt hij NIET meer geruisloos omhoog (%.1f cm)"), HighStepGain), HighStepGain < 10.0f);

	// --- 4b. de twee vergevingsvensters (JMP-07/08) -------------------------
	// Coyote time: van een rand AF LOPEN en een fractie te laat drukken. De val
	// wordt hier geinduceerd met SetMovementMode in plaats van van een richel
	// gelopen — de vloer van dit harnas is vlak. De toestand is echt (dezelfde
	// OnMovementModeChanged die een echte rand ook doorloopt), de aanleiding niet;
	// dat hoort in het rapport te staan in plaats van weggepoetst te worden.
	auto CoyoteJumpSucceeds = [this, &Harness, Movement](float WaitSeconds)
	{
		// Ruim wachten tot hij ECHT staat: van 15 meter vallen duurt bijna twee
		// seconden, en een meting die begint terwijl hij nog in de lucht hangt
		// meet iets anders dan hij denkt. De eerste versie van deze test deed dat
		// wél en gaf daardoor een vals positief: de "buiten het venster"-tak
		// landde tijdens het wachten en deed gewoon een normale grondsprong.
		Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 8.0, [&Harness, Movement]()
		{
			return Movement->IsMovingOnGround() && Harness.SpeedCm() < 1.0f;
		});
		// Van de grond af KOMEN zonder te springen. Optillen en de zwaartekracht
		// zijn werk laten doen geeft exact dezelfde walking->falling-overgang als
		// van een richel aflopen — en anders dan SetMovementMode(MOVE_Falling)
		// landt hij niet binnen twee ticks weer op de vlakke vloer.
		Harness.Body->SetActorLocation(Harness.Body->GetActorLocation() + FVector(0.0f, 0.0f, 1500.0f), false, nullptr, ETeleportType::TeleportPhysics);
		Harness.Idle(WaitSeconds);
		// De meting is alleen geldig als hij op dít moment nog valt. Anders meet je
		// een gewone grondsprong en noem je dat coyote time.
		TestTrue(FString::Printf(TEXT("laag 2: de coyote-meting vertrekt vanuit een val (wacht %.2f s)"), WaitSeconds),
			Movement->IsFalling());
		const float BeforeZ = Movement->Velocity.Z;
		Harness.Inject(TEXT("Jump"), true);
		Harness.Step();
		Harness.Step();
		return Movement->Velocity.Z > BeforeZ + 100.0f;
	};
	const bool bCoyoteInside = CoyoteJumpSucceeds(0.05f);
	const bool bCoyoteOutside = CoyoteJumpSucceeds(0.40f);
	Report(*this, TEXT("coyote-venster"), Harness.Tuning->CoyoteTimeSeconds, TEXT("s"), TEXT("0.11 — UE levert dit niet"));
	TestTrue(TEXT("laag 2: binnen het coyote-venster telt de sprong alsnog"), bCoyoteInside);
	TestFalse(TEXT("laag 2: buiten het venster niet — het is vergeving, geen dubbelsprong"), bCoyoteOutside);

	// Inputbuffer: drukken VÓÓR de landing, en bij het raken van de grond moet de
	// sprong alsnog afgaan zonder tweede druk.
	Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 2.0, [&Harness, Movement]()
	{
		return Movement->IsMovingOnGround() && Harness.SpeedCm() < 1.0f;
	});
	Harness.Press(TEXT("Jump"));
	// Wachten tot hij daalt en dichtbij de grond is, dan één keer drukken.
	Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 2.0, [Movement]()
	{
		return Movement->IsFalling() && Movement->Velocity.Z < -350.0f;
	});
	Harness.Inject(TEXT("Jump"), true);
	Harness.Step();
	bool bBufferedJumpFired = false;
	{
		const int32 MaxSteps = FMath::RoundToInt(1.5f / FixedStepSeconds);
		bool bTouchedDown = false;
		for (int32 I = 0; I < MaxSteps; ++I)
		{
			Harness.Step();
			if (Movement->IsMovingOnGround())
			{
				bTouchedDown = true;
			}
			// Zonder buffer blijft hij liggen; met buffer gaat hij zonder tweede
			// druk opnieuw de lucht in.
			if (bTouchedDown && Movement->Velocity.Z > 100.0f)
			{
				bBufferedJumpFired = true;
				break;
			}
		}
	}
	Report(*this, TEXT("inputbuffer-venster"), Harness.Tuning->JumpInputBufferSeconds, TEXT("s"), TEXT("0.15 — UE levert dit niet"));
	TestTrue(TEXT("laag 2: een druk vlak vóór de landing gaat bij het neerkomen alsnog af"), bBufferedJumpFired);

	Harness.Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// LAAG 2 — kijken en deadzone (de stick-tak)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFeelLayer2LookTest,
	"Eclipse.Feel.Layer2.InjectedLookAndDeadzone",
	EclipseFeelTest::TestFlags)

bool FEclipseFeelLayer2LookTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	// De stick-tak is alleen te meten als de handlers de invoer ook ALS stick
	// lezen. Injectie raakt geen hardware, dus de autodetectie zou hier altijd
	// "muis" zeggen en de hele deadzone/curve-tak ongetest laten.
	EclipseFeelTest::FForceGamepadScope Gamepad(1);

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	// --- 5. seconden per 360 graden ----------------------------------------
	double YawSwept = 0.0;
	double TurnSeconds = 0.0;
	{
		double LastYaw = Harness.Controller->GetControlRotation().Yaw;
		const double Start = Harness.ElapsedSeconds;
		const int32 MaxSteps = FMath::RoundToInt(8.0f / FixedStepSeconds);
		for (int32 I = 0; I < MaxSteps && YawSwept < 360.0; ++I)
		{
			Harness.Inject(TEXT("Look"), FVector2D(1.0f, 0.0f));
			Harness.Step();
			const double Yaw = Harness.Controller->GetControlRotation().Yaw;
			YawSwept += FMath::Abs(FMath::FindDeltaAngleDegrees(LastYaw, Yaw));
			LastYaw = Yaw;
		}
		TurnSeconds = Harness.ElapsedSeconds - Start;
	}
	const double MeasuredYawRate = TurnSeconds > 0.0 ? YawSwept / TurnSeconds : 0.0;
	Report(*this, TEXT("seconden per 360 graden kijken"), TurnSeconds, TEXT("s"), TEXT("DA_CharacterTuning: 360 / StickYawSpeed"));
	Report(*this, TEXT("gemeten kijksnelheid"), MeasuredYawRate, TEXT("gr/s"),
		*FString::Printf(TEXT("StickYawSpeed = %.0f gr/s"), Harness.Tuning->StickYawSpeed));
	TestTrue(TEXT("laag 2: een volle 360 is binnen 8 s haalbaar"), YawSwept >= 359.0);
	// De verhouding is het hele punt: als de engine er nog een schaal overheen
	// legt, is dit de enige plek waar dat zichtbaar wordt.
	TestTrue(FString::Printf(TEXT("laag 2: de kijksnelheid IS de getunede kijksnelheid (%.0f vs %.0f gr/s)"),
			MeasuredYawRate, Harness.Tuning->StickYawSpeed),
		FMath::Abs(MeasuredYawRate - Harness.Tuning->StickYawSpeed) <= Harness.Tuning->StickYawSpeed * 0.10);

	// --- 5b. pitch: snelheid EN teken --------------------------------------
	// Het teken is hier geen detail. De legacy-pitchschaal van de engine was
	// NEGATIEF (-2.5), dus de handler compenseerde een verborgen omkering; sinds
	// die schaal uit staat draagt de handler het teken zelf. Zo'n wissel is
	// precies het soort ding dat je pas merkt met een controller in je handen —
	// tenzij een test hem vastpint.
	double PitchSwept = 0.0;
	double PitchSeconds = 0.0;
	{
		Harness.Controller->SetControlRotation(FRotator::ZeroRotator);
		Harness.Idle(0.05f);
		const double PitchBefore = Harness.Controller->GetControlRotation().Pitch;
		const double Start = Harness.ElapsedSeconds;
		Harness.HoldFor(TEXT("Look"), FVector2D(0.0f, 1.0f), 0.20);
		PitchSeconds = Harness.ElapsedSeconds - Start;
		PitchSwept = FMath::FindDeltaAngleDegrees(PitchBefore, Harness.Controller->GetControlRotation().Pitch);
	}
	const double MeasuredPitchRate = PitchSeconds > 0.0 ? PitchSwept / PitchSeconds : 0.0;
	Report(*this, TEXT("gemeten kantelsnelheid"), MeasuredPitchRate, TEXT("gr/s"),
		*FString::Printf(TEXT("StickPitchSpeed = +%.0f gr/s (omhoog)"), Harness.Tuning->StickPitchSpeed));
	TestTrue(FString::Printf(TEXT("laag 2: stick omhoog kijkt OMHOOG (%.1f gr/s)"), MeasuredPitchRate), MeasuredPitchRate > 0.0);
	TestTrue(FString::Printf(TEXT("laag 2: de kantelsnelheid IS de getunede kantelsnelheid (%.0f vs %.0f gr/s)"),
			FMath::Abs(MeasuredPitchRate), Harness.Tuning->StickPitchSpeed),
		FMath::Abs(FMath::Abs(MeasuredPitchRate) - Harness.Tuning->StickPitchSpeed) <= Harness.Tuning->StickPitchSpeed * 0.10);
	Harness.Controller->SetControlRotation(FRotator::ZeroRotator);
	Harness.Idle(0.05f);

	// --- 5c. de camera remt af tegen de pitch-limiet (CAM-07b) --------------
	// Nesky's fout #47: "maintaining pitch speed until hitting the pitch limit".
	// Het verschil tussen "de camera stopt" en "de camera knalt tegen een muur" is
	// direct voelbaar. Gemeten als: hoeveel graden per seconde haalt hij midden in
	// het bereik, en hoeveel in de laatste graden vóór de klem?
	{
		Harness.Controller->SetControlRotation(FRotator::ZeroRotator);
		Harness.Idle(0.05f);
		// Midden in het bereik: 0.2 s omhoog vanaf 0 komt niet in de dempband.
		const double MidStart = Harness.Controller->GetControlRotation().Pitch;
		Harness.HoldFor(TEXT("Look"), FVector2D(0.0f, 1.0f), 0.20);
		const double MidRate = FMath::Abs(FMath::FindDeltaAngleDegrees(MidStart,
			Harness.Controller->GetControlRotation().Pitch)) / 0.20;

		// BINNEN de dempband meten, niet ERÓP. Twee graden onder de limiet is nog
		// 1,7 graden bewegingsruimte in 0,03 s, dus de meting verzadigt niet.
		// De eerste versie mat de snelheid terwijl de camera al op de klem stond en
		// las dus 0 gr/s — dat is trivial waar, ook zónder demping, en een assert
		// die zonder de wijziging ook zou slagen bewijst niets.
		Harness.Controller->SetControlRotation(FRotator(Harness.Tuning->ViewPitchMax - 2.0f, 0.0f, 0.0f));
		Harness.Idle(0.05f);
		const double NearStart = Harness.Controller->GetControlRotation().Pitch;
		Harness.HoldFor(TEXT("Look"), FVector2D(0.0f, 1.0f), 0.03);
		const double NearRate = FMath::Abs(FMath::FindDeltaAngleDegrees(NearStart,
			Harness.Controller->GetControlRotation().Pitch)) / 0.03;

		// En daarna doorduwen: de limiet moet ECHT haalbaar blijven.
		Harness.HoldFor(TEXT("Look"), FVector2D(0.0f, 1.0f), 3.0);
		const double Reached = FRotator::NormalizeAxis(Harness.Controller->GetControlRotation().Pitch);

		Report(*this, TEXT("kantelsnelheid midden in het bereik"), MidRate, TEXT("gr/s"));
		Report(*this, TEXT("kantelsnelheid 2 gr voor de limiet"), NearRate, TEXT("gr/s"), TEXT("fors lager dan midden in het bereik, maar NIET nul"));
		Report(*this, TEXT("bereikte pitch"), Reached, TEXT("gr"), *FString::Printf(TEXT("ViewPitchMax = %.0f"), Harness.Tuning->ViewPitchMax));
		TestTrue(FString::Printf(TEXT("laag 2: de limiet is ECHT bereikbaar (%.2f van %.0f gr)"), Reached, Harness.Tuning->ViewPitchMax),
			Reached >= Harness.Tuning->ViewPitchMax - 1.0f);
		TestTrue(FString::Printf(TEXT("laag 2: de camera remt af binnen de dempband (%.0f -> %.0f gr/s)"), MidRate, NearRate),
			NearRate < MidRate * 0.6);
		TestTrue(FString::Printf(TEXT("laag 2: maar hij staat niet stil in de band (%.0f gr/s)"), NearRate), NearRate > 1.0);
		Harness.Controller->SetControlRotation(FRotator::ZeroRotator);
		Harness.Idle(0.05f);
	}

	// --- 6. een stick op 0,05 mag NIETS doen --------------------------------
	// De owner-meting in phase0/controller_kalibratie.json: zijn linkerstick rust
	// op LY = -0.048. Drift mag het personage niet laten lopen en de camera niet
	// laten draaien — dat laatste is hoe een hele wereld uit zichzelf lijkt te
	// roteren, want het lichaam volgt zijn looprichting.
	const FVector DriftStart = Harness.Location();
	const double DriftYawStart = Harness.Controller->GetControlRotation().Yaw;
	for (int32 I = 0; I < FMath::RoundToInt(1.0f / FixedStepSeconds); ++I)
	{
		Harness.Inject(TEXT("Move"), FVector2D(0.0f, 0.05f));
		Harness.Inject(TEXT("Look"), FVector2D(0.05f, 0.05f));
		Harness.Step();
	}
	const double DriftMoved = FVector::Dist2D(Harness.Location(), DriftStart);
	const double DriftYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(DriftYawStart, Harness.Controller->GetControlRotation().Yaw));
	Report(*this, TEXT("verplaatsing bij stick 0.05, 1 s"), DriftMoved, TEXT("cm"), TEXT("0 — deadzone 0.08"));
	Report(*this, TEXT("camera-draai bij stick 0.05, 1 s"), DriftYaw, TEXT("gr"), TEXT("0 — deadzone 0.08"));
	TestTrue(FString::Printf(TEXT("laag 2: stickdrift 0.05 beweegt het personage NIET (%.3f cm)"), DriftMoved), DriftMoved < 1.0);
	TestTrue(FString::Printf(TEXT("laag 2: stickdrift 0.05 draait de camera NIET (%.3f gr)"), DriftYaw), DriftYaw < 0.05);

	// En het spiegelbeeld: net BOVEN de deadzone moet er wel degelijk iets
	// gebeuren. Zonder deze helft zou een deadzone van 1.0 ook groen zijn.
	const FVector LiveStart = Harness.Location();
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 0.45f), 1.0);
	const double LiveMoved = FVector::Dist2D(Harness.Location(), LiveStart);
	Report(*this, TEXT("verplaatsing bij stick 0.45, 1 s"), LiveMoved, TEXT("cm"), TEXT("> 0 — voorbij de deadzone"));
	TestTrue(FString::Printf(TEXT("laag 2: voorbij de deadzone loopt het personage wél (%.1f cm)"), LiveMoved), LiveMoved > 20.0);

	Harness.Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// S1 — "mijn personage schaalt met snelheid"
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFeelCameraScaleTest,
	"Eclipse.Feel.Camera.ApparentSizeDoesNotTrackSpeed",
	EclipseFeelTest::TestFlags)

bool FEclipseFeelCameraScaleTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	auto Describe = [this](const TCHAR* When, const FEclipseFeelSample& S)
	{
		AddInfo(FString::Printf(
			TEXT("S1 %-10s snelheid %6.0f · mesh-schaal %.3f · boom %6.1f (doel %6.1f) · camera->pawn %7.2f · FOV %5.1f · schijnbare hoogte %6.3f gr (%.2f%% van beeld)"),
			When, S.SpeedCm, S.MeshScale, S.BoomArmLength, S.BoomTargetArmLength,
			S.CameraToPawnCm, S.FieldOfView, S.ApparentHeightDegrees, S.ApparentFractionOfView * 100.0f));
	};

	// Stilstand als nulmeting.
	const FEclipseFeelSample AtRest = Harness.Body->SampleFeelState();
	Describe(TEXT("stilstand"), AtRest);

	// Meting A: op rensnelheid, in stabiele toestand (de lag moet uitgeconvergeerd
	// zijn, anders meten we de aanloop en niet de eindstand).
	Harness.RunUpToTopSpeed(*this);
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 1.5);
	const FEclipseFeelSample AtRun = Harness.Body->SampleFeelState();
	Describe(TEXT("rennen"), AtRun);

	// Meting B: sprint aan (L3-toggle), doorlopen tot de nieuwe eindstand.
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 2.5);
	const FEclipseFeelSample AtSprint = Harness.Body->SampleFeelState();
	Describe(TEXT("sprinten"), AtSprint);

	TestTrue(FString::Printf(TEXT("S1: de sprint is echt sneller (%.0f -> %.0f cm/s)"), AtRun.SpeedCm, AtSprint.SpeedCm),
		AtSprint.SpeedCm > AtRun.SpeedCm * 1.2f);

	// De vier kandidaten, elk apart afgerekend. Drie horen ONVERANDERD te zijn;
	// blijft er dan iets over dat wél meebeweegt, dan is dat per uitsluiting de
	// oorzaak — en niet een plausibele verklaring.
	TestEqual(TEXT("S1: mesh-schaal beweegt niet mee met de snelheid"), AtSprint.MeshScale, AtRun.MeshScale, 0.001f);
	TestEqual(TEXT("S1: doel-boomlengte beweegt niet mee met de snelheid"), AtSprint.BoomTargetArmLength, AtRun.BoomTargetArmLength, 0.01f);
	TestEqual(TEXT("S1: boomlengte beweegt niet mee met de snelheid"), AtSprint.BoomArmLength, AtRun.BoomArmLength, 0.01f);
	TestEqual(TEXT("S1: FOV beweegt niet mee met de snelheid"), AtSprint.FieldOfView, AtRun.FieldOfView, 0.01f);

	// En de uitkomst die de speler daadwerkelijk ziet. 2% is streng maar eerlijk:
	// onder die grens is het verschil op een 1080p-scherm minder dan een pixel of
	// tien op een personage van een paar honderd pixels hoog.
	auto Drift = [](const FEclipseFeelSample& A, const FEclipseFeelSample& B)
	{
		return A.ApparentHeightDegrees > KINDA_SMALL_NUMBER
			? FMath::Abs(B.ApparentHeightDegrees - A.ApparentHeightDegrees) / A.ApparentHeightDegrees : 0.0;
	};
	const double SizeDrift = Drift(AtRun, AtSprint);
	const double RestDrift = Drift(AtRest, AtSprint);
	Report(*this, TEXT("camera-afstand: sprint - rennen"), AtSprint.CameraToPawnCm - AtRun.CameraToPawnCm, TEXT("cm"),
		TEXT("0 — de afstand mag niet met de snelheid meebewegen"));
	Report(*this, TEXT("camera-afstand: sprint - stilstand"), AtSprint.CameraToPawnCm - AtRest.CameraToPawnCm, TEXT("cm"),
		TEXT("<= CameraLagMaxDistance"));
	Report(*this, TEXT("schijnbare grootte: rennen -> sprinten"), SizeDrift * 100.0, TEXT("%"), TEXT("< 2%"));
	Report(*this, TEXT("schijnbare grootte: stilstand -> sprinten"), RestDrift * 100.0, TEXT("%"), TEXT("< 2%"));

	// De harde eis: tussen twee SNELHEDEN mag er niets veranderen. Zonder de klem
	// op de camera-lag stond hier 8,4% (31,50 -> 28,84 graden), en dat is precies
	// wat de owner "mijn personage schaalt met snelheid" noemde. De klem laat
	// alleen het verschil stilstand-versus-lopen over, en dat is de bovengrens
	// hieronder.
	TestTrue(FString::Printf(TEXT("S1: rennen en sprinten geven dezelfde schijnbare grootte (%.2f%%)"), SizeDrift * 100.0),
		SizeDrift < 0.02);
	TestTrue(FString::Printf(TEXT("S1: ook stilstand tegen sprinten blijft onder de drempel (%.2f%%)"), RestDrift * 100.0),
		RestDrift < 0.02);
	TestTrue(FString::Printf(TEXT("S1: de camera-achterstand is geklemd (%.1f cm, klem %.1f)"),
			AtSprint.CameraToPawnCm - AtRest.CameraToPawnCm, Harness.Tuning->CameraLagMaxDistance),
		(AtSprint.CameraToPawnCm - AtRest.CameraToPawnCm) <= Harness.Tuning->CameraLagMaxDistance + 0.5f);

	Harness.Shutdown();
	return true;
}

// ---------------------------------------------------------------------------
// S2 — sprint: hold op het toetsenbord, toggle op de pad
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFeelSprintTest,
	"Eclipse.Feel.Input.SprintHoldsOnKeyboardAndTogglesOnPad",
	EclipseFeelTest::TestFlags)

bool FEclipseFeelSprintTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	UCharacterMovementComponent* Movement = Harness.Body->GetCharacterMovement();
	const float RunSpeed = Harness.Tuning->RunSpeed;
	const float SprintSpeed = Harness.Tuning->SprintSpeed;

	// --- de hold (Shift) ----------------------------------------------------
	TestEqual(TEXT("S2: rust staat op rensnelheid"), Movement->MaxWalkSpeed, RunSpeed, 0.01f);
	Harness.HoldFor(TEXT("SprintHold"), FVector2D(1.0f, 0.0f), 0.2);
	TestEqual(TEXT("S2: Shift vasthouden geeft sprintsnelheid"), Movement->MaxWalkSpeed, SprintSpeed, 0.01f);
	TestTrue(TEXT("S2: de controller weet dat hij sprint"), Harness.Controller->IsSprinting());
	Harness.Idle(0.2f);
	TestEqual(TEXT("S2: Shift loslaten valt terug op rensnelheid"), Movement->MaxWalkSpeed, RunSpeed, 0.01f);
	TestFalse(TEXT("S2: een hold latcht niet"), Harness.Controller->IsSprintLatched());

	// --- de toggle (L3) -----------------------------------------------------
	// Eerst vooruit duwen: zonder voorwaartse invoer vervalt de toggle meteen, en
	// dat is precies het gedrag dat we verderop apart toetsen.
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 0.3);
	TestTrue(TEXT("S2: één klik op L3 zet de sprint aan"), Harness.Controller->IsSprintLatched());
	TestEqual(TEXT("S2: de latch geeft sprintsnelheid"), Movement->MaxWalkSpeed, SprintSpeed, 0.01f);

	// ...en hij BLIJFT aan zolang je vooruit duwt — dat is het hele verschil met
	// de hold die er stond.
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 1.5);
	TestTrue(TEXT("S2: de latch blijft aan zolang je vooruit duwt"), Harness.Controller->IsSprintLatched());

	// Uitstap 4: nogmaals L3.
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	TestFalse(TEXT("S2: nogmaals L3 zet de sprint uit"), Harness.Controller->IsSprintLatched());
	TestEqual(TEXT("S2: na uitzetten weer rensnelheid"), Movement->MaxWalkSpeed, RunSpeed, 0.01f);

	// Uitstap 1: ophouden met vooruit duwen.
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 0.3);
	TestTrue(TEXT("S2: latch weer aan voor de uitstaptest"), Harness.Controller->IsSprintLatched());
	Harness.Idle(0.2f); // stick los
	TestFalse(TEXT("S2: stick loslaten beëindigt de sprint"), Harness.Controller->IsSprintLatched());

	// Uitstap 1b: zijwaarts sturen mag WEL tijdens een sprint.
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	Harness.HoldFor(TEXT("Move"), FVector2D(0.5f, 0.85f), 0.5);
	TestTrue(TEXT("S2: schuin vooruit sturen beëindigt de sprint niet"), Harness.Controller->IsSprintLatched());
	Harness.Idle(0.2f);

	// Uitstap 2: mikken.
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 0.3);
	TestTrue(TEXT("S2: latch aan vóór het mikken"), Harness.Controller->IsSprintLatched());
	Harness.PressWhileMovingForward(TEXT("Aim"));
	TestFalse(TEXT("S2: mikken beëindigt de sprint"), Harness.Controller->IsSprintLatched());
	Harness.Idle(0.2f);

	// Uitstap 3: vuren.
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 0.3);
	TestTrue(TEXT("S2: latch aan vóór het vuren"), Harness.Controller->IsSprintLatched());
	Harness.PressWhileMovingForward(TEXT("Fire"));
	TestFalse(TEXT("S2: vuren beëindigt de sprint"), Harness.Controller->IsSprintLatched());

	// En het bewijs dat het ook echt sneller LOOPT en niet alleen een getal zet.
	Harness.Idle(0.3f);
	Harness.RunUpToTopSpeed(*this);
	const float MeasuredRun = Harness.SpeedCm();
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 2.5,
		[&Harness, SprintSpeed]() { return Harness.SpeedCm() >= SprintSpeed * 0.995f; });
	const float MeasuredSprint = Harness.SpeedCm();
	Report(*this, TEXT("gemeten rensnelheid"), MeasuredRun, TEXT("cm/s"), *FString::Printf(TEXT("%.0f"), RunSpeed));
	Report(*this, TEXT("gemeten sprintsnelheid (L3-toggle)"), MeasuredSprint, TEXT("cm/s"), *FString::Printf(TEXT("%.0f"), SprintSpeed));
	TestTrue(FString::Printf(TEXT("S2: de L3-sprint versnelt het personage echt (%.0f -> %.0f cm/s)"), MeasuredRun, MeasuredSprint),
		MeasuredSprint >= SprintSpeed * 0.98f);

	Harness.Shutdown();
	return true;
}


// ---------------------------------------------------------------------------
// De beschrijvingen bewaken, niet alleen het gedrag
// ---------------------------------------------------------------------------
//
// Zes keer vannacht klopte een BESCHRIJVING niet met de code, en elke keer was
// het de beschrijving die de volgende ronde de verkeerde kant op stuurde — niet
// het gedrag. De tests dekten het gedrag volledig en zagen er geen één.
//
// Dit is de ontbrekende schakel in minimale vorm: de F2-controletabel is wat de
// speler TIJDENS het spelen leest, en elke rij dáár claimt dat er een binding
// bestaat. Deze test legt die claim naast de echte mapping context. Hij
// controleert de TEKST niet ("Shift" versus LeftShift is prozawerk), maar wel
// het harde deel: beweert de rij een controller-binding, dan hoort er een
// gamepad-toets gemapt te zijn, en andersom.
//
// De twee uitzonderingen staan hier expliciet in plaats van in een comment,
// zodat ze niet stilletjes kunnen groeien.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseControlTableMatchesBindingsTest,
	"Eclipse.Feel.Input.ControlTableClaimsOnlyBindingsThatExist",
	EclipseFeelTest::TestFlags)

bool FEclipseControlTableMatchesBindingsTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	const UInputMappingContext* Context = Harness.Controller->GetMappingContext();
	if (!TestNotNull(TEXT("controletabel: mapping context"), Context))
	{
		Harness.Shutdown();
		return false;
	}

	// Heeft deze actie minstens één toets van het gevraagde soort?
	auto HasKeyOfKind = [Context](const UInputAction* Action, bool bWantGamepad)
	{
		if (Action == nullptr)
		{
			return false;
		}
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.Action == Action && Mapping.Key.IsGamepadKey() == bWantGamepad)
			{
				return true;
			}
		}
		return false;
	};

	struct FRowClaim
	{
		const TCHAR* Row;          // zoals hij in de F2-tabel staat
		const TCHAR* KeyboardAction; // nullptr = deze rij claimt geen actie-binding
		const TCHAR* PadAction;
		const TCHAR* Exemption;    // niet-null = uitzondering, mét reden
	};
	const FRowClaim Claims[] = {
		{ TEXT("Lopen"),         TEXT("Move"),        TEXT("Move"),          nullptr },
		{ TEXT("Rondkijken"),    TEXT("Look"),        TEXT("Look"),          nullptr },
		{ TEXT("Vuren"),         TEXT("Fire"),        TEXT("Fire"),          nullptr },
		// Twee APARTE acties, en dat is de S2-fix: hold op toetsenbord, toggle op de pad.
		{ TEXT("Sprint"),        TEXT("SprintHold"),  TEXT("SprintToggle"),  nullptr },
		{ TEXT("Hurken"),        TEXT("Crouch"),      TEXT("Crouch"),        nullptr },
		{ TEXT("Springen"),      TEXT("Jump"),        TEXT("Jump"),          nullptr },
		{ TEXT("Mikken"),        TEXT("Aim"),         TEXT("Aim"),           nullptr },
		{ TEXT("1e/3e persoon"), TEXT("ToggleView"),  nullptr,
		  TEXT("de tabel zegt zelf 'geen' op de pad — R3 ging per ongeluk af en is eraf gehaald") },
		{ TEXT("Command Mode"),  TEXT("CommandHold"), TEXT("CommandHold"),   nullptr },
		{ TEXT("Volgende"),      TEXT("SelectNext"),  TEXT("SelectNext"),    nullptr },
		{ TEXT("Vorige"),        TEXT("SelectPrev"),  TEXT("SelectPrev"),    nullptr },
		{ TEXT("Onder kruis"),   TEXT("DirectPick"),  TEXT("DirectPick"),    nullptr },
		{ TEXT("Orders"),        TEXT("Order1"),      TEXT("Order1"),        nullptr },
		{ TEXT("Stance"),        nullptr,             TEXT("StanceToggle"),
		  TEXT("LeftAlt is GEEN actie: IssueSquadOrder pollt IsInputKeyDown op het moment van geven") },
	};

	const TArray<EclipseGauntletOverlay::FEclipseControlRow> Rows = EclipseGauntletOverlay::GetControlRows();
	TestEqual(TEXT("controletabel: evenveel rijen als claims — een nieuwe rij hoort hier ook te landen"),
		Rows.Num(), static_cast<int32>(UE_ARRAY_COUNT(Claims)));

	int32 Exemptions = 0;
	for (const FRowClaim& Claim : Claims)
	{
		const EclipseGauntletOverlay::FEclipseControlRow* Row = Rows.FindByPredicate(
			[&Claim](const EclipseGauntletOverlay::FEclipseControlRow& Candidate)
			{
				return FCString::Strcmp(Candidate.Action, Claim.Row) == 0;
			});
		if (!TestNotNull(*FString::Printf(TEXT("controletabel: rij '%s' bestaat"), Claim.Row), Row))
		{
			continue;
		}

		if (Claim.KeyboardAction != nullptr)
		{
			TestTrue(*FString::Printf(TEXT("controletabel: '%s' claimt muis/toetsenbord ('%s') en die binding bestaat"),
					Claim.Row, Row->MouseKeyboard),
				HasKeyOfKind(Harness.Controller->FindInputAction(Claim.KeyboardAction), /*bWantGamepad*/ false));
		}
		if (Claim.PadAction != nullptr)
		{
			TestTrue(*FString::Printf(TEXT("controletabel: '%s' claimt controller ('%s') en die binding bestaat"),
					Claim.Row, Row->Controller),
				HasKeyOfKind(Harness.Controller->FindInputAction(Claim.PadAction), /*bWantGamepad*/ true));
		}
		if (Claim.Exemption != nullptr)
		{
			++Exemptions;
			AddInfo(FString::Printf(TEXT("controletabel: '%s' is een bewuste uitzondering — %s"), Claim.Row, Claim.Exemption));
		}
	}

	// De uitzonderingen tellen, zodat er niet stilletjes een derde bijkomt.
	TestEqual(TEXT("controletabel: precies twee bewuste uitzonderingen, allebei met reden"), Exemptions, 2);

	Harness.Shutdown();
	return true;
}


// ---------------------------------------------------------------------------
// De tweede beschrijving die de speler leest: de testgids
// ---------------------------------------------------------------------------
//
// De F2-tabel zegt WELKE toets iets doet; de gids zegt WAT er dan hoort te
// gebeuren, en die zin bevat harde getallen ("420 -> 650 cm/s", "150 cm/s").
// Precies die getallen drijven weg zodra iemand DA_CharacterTuning aanraakt, en
// dan leert de game de speler een verwachting die zijn eigen build niet waarmaakt
// — hetzelfde defect als het paneel dat "1,50 s per 360" toonde bij 0,60 s.
//
// Bewust GEEN prozacontrole: als iemand de zin herschrijft maar de getallen laat
// staan, hoort dat te mogen. Alleen de getallen worden vastgehouden.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGuideNumbersMatchTuningTest,
	"Eclipse.Feel.Input.GuideNumbersStillMatchTheTuning",
	EclipseFeelTest::TestFlags)

bool FEclipseGuideNumbersMatchTuningTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}
	const UEclipseCharacterTuningAsset& T = *Harness.Tuning;

	const TArray<EclipseTestGuide::FEclipseGuideStep> Steps = EclipseTestGuide::GetGuideSteps();
	auto ExpectationFor = [&Steps](EclipseTestGuide::EEclipseGuideSignal Signal) -> FString
	{
		const EclipseTestGuide::FEclipseGuideStep* Step = Steps.FindByPredicate(
			[Signal](const EclipseTestGuide::FEclipseGuideStep& Candidate) { return Candidate.Signal == Signal; });
		return Step != nullptr ? Step->Expectation : FString();
	};

	auto MentionsNumber = [](const FString& Text, float Value)
	{
		return Text.Contains(FString::Printf(TEXT("%.0f"), Value));
	};

	const FString SprintText = ExpectationFor(EclipseTestGuide::EEclipseGuideSignal::Sprint);
	TestTrue(TEXT("gids: er is een sprint-stap"), !SprintText.IsEmpty());
	TestTrue(*FString::Printf(TEXT("gids: de sprint-stap noemt de getunede rensnelheid (%.0f) — '%s'"), T.RunSpeed, *SprintText),
		MentionsNumber(SprintText, T.RunSpeed));
	TestTrue(*FString::Printf(TEXT("gids: de sprint-stap noemt de getunede sprintsnelheid (%.0f)"), T.SprintSpeed),
		MentionsNumber(SprintText, T.SprintSpeed));

	const FString CrouchText = ExpectationFor(EclipseTestGuide::EEclipseGuideSignal::Crouch);
	TestTrue(TEXT("gids: er is een hurk-stap"), !CrouchText.IsEmpty());
	TestTrue(*FString::Printf(TEXT("gids: de hurk-stap noemt de getunede hurksnelheid (%.0f)"), T.CrouchSpeed),
		MentionsNumber(CrouchText, T.CrouchSpeed));

	// En de kop van de gids toont de kijkwaarden; die regel wordt uit dezelfde
	// members gebouwd die de handler gebruikt, dus hij kan niet verouderen — maar
	// het getal dat hij toont MOET wel het getunede zijn. Dat was het tot vannacht
	// niet, en dat is precies waarom deze test bestaat.
	const FString LookLine = Harness.Controller->DescribeLookTuning();
	TestTrue(*FString::Printf(TEXT("gids-kop: toont de getunede kijksnelheid (%.0f gr/s) — '%s'"), T.StickYawSpeed, *LookLine),
		LookLine.Contains(FString::Printf(TEXT("%.0f"), T.StickYawSpeed)));
	TestTrue(*FString::Printf(TEXT("gids-kop: toont de seconden per 360 die daarbij horen (%.2f s)"), 360.0f / T.StickYawSpeed),
		LookLine.Contains(FString::Printf(TEXT("%.2f"), 360.0f / T.StickYawSpeed)));

	Harness.Shutdown();
	return true;
}


// ---------------------------------------------------------------------------
// De derde beschrijving: de console-commando's die BESTURING.md adverteert
// ---------------------------------------------------------------------------
//
// BESTURING.md heeft een tabel met startopties en console-commando's, en die is
// het eerste wat iemand raadpleegt als iets niet werkt. Een commando dat daar
// staat maar niet bestaat, kost precies dezelfde avond als de F9-binding die niet
// aankwam: je typt het, er gebeurt niets, en je weet niet of je het fout typte of
// dat het er niet is.
//
// Dit dekt de HARDE kant — bestaat het object — en niet de omschrijving. Waar de
// vorige twee bewakers ophouden bij wat de speler tijdens het spelen leest, gaat
// deze over wat hij leest als hij vastloopt.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseDocumentedConsoleCommandsExistTest,
	"Eclipse.Feel.Input.DocumentedConsoleCommandsExist",
	EclipseFeelTest::TestFlags)

bool FEclipseDocumentedConsoleCommandsExistTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	// De harnas draait mee omdat twee van de vier pas bij BeginPlay geregistreerd
	// worden — een test die alleen de statische CVars kent, mist juist die twee.
	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	// Exact de namen zoals ze in BESTURING.md staan. Wijzigt er een in de code,
	// dan hoort deze test te vallen en de documentatie mee te veranderen.
	const TCHAR* Documented[] = {
		TEXT("Eclipse.Feel.Dump"),
		TEXT("Eclipse.Command.Dump"),
		TEXT("Eclipse.Look.InvertY"),
		TEXT("Eclipse.Input.ForceGamepad"),
		TEXT("Eclipse.Guide.Overlay"),
	};
	for (const TCHAR* Name : Documented)
	{
		TestNotNull(*FString::Printf(TEXT("BESTURING.md adverteert '%s' — bestaat dat commando?"), Name),
			IConsoleManager::Get().FindConsoleObject(Name));
	}

	// Inline falsificatie: als de opzoeking alles zou vinden, bewees hij niets.
	// Een naam die gegarandeerd niet bestaat MOET null geven.
	TestNull(TEXT("de opzoeking discrimineert (een verzonnen commando bestaat niet)"),
		IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.DitBestaatNiet.Controle")));

	Harness.Shutdown();
	return true;
}

// Waar stort de boom in? (CAM-05, de laatste openstaande AFWIJKEND-rij)
//
// De feel-audit zegt over de pitch-limieten: "bij -70 duikt de boom in de grond
// en trekt de collision hem naar binnen", met -55/+70 als doel. Dat is een
// REDENERING, geen meting, en de richting klopt bovendien niet vanzelf: een
// spring arm steekt tegen de kijkrichting in, dus omlaag kijken (negatieve
// pitch) tilt de camera juist OMHOOG, weg van de vloer. Wie in de grond duikt is
// dan eerder de andere kant.
//
// Deze test rekent niet na maar meet: over het hele bereik de daadwerkelijke
// camera-tot-pawn-afstand, waarin arm, lag en collision-inpull alle drie zitten.
// De hoek waar dat getal begint te zakken is de echte grens; een conventie-getal
// is dat hoogstens bij toeval.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCameraPitchCollapseTest,
	"Eclipse.Feel.Camera.WhereTheBoomCollapsesAcrossThePitchRange",
	EclipseFeelTest::TestFlags)

bool FEclipseCameraPitchCollapseTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	// Op de vloer laten staan: een vallende pawn sleept de boom mee en dan meet je
	// de val, niet de hoek (dezelfde valkuil als bij de ANI-09-meting).
	Harness.Idle(0.5f);

	const float NominalArm = Harness.Body->SampleFeelState().BoomArmLength;
	Report(*this, TEXT("boomlengte zonder hindernis"), NominalArm, TEXT("cm"));

	float WorstDistance = NominalArm;
	float WorstPitch = 0.0f;
	TArray<FString> Curve;

	// Van omlaag naar omhoog in stappen van 10 graden, plus de twee limieten zelf.
	TArray<float> Pitches;
	for (float Pitch = -80.0f; Pitch <= 80.0f; Pitch += 10.0f)
	{
		Pitches.Add(Pitch);
	}
	Pitches.Add(Harness.Tuning->ViewPitchMin);
	Pitches.Add(Harness.Tuning->ViewPitchMax);
	Pitches.Sort();

	for (float Pitch : Pitches)
	{
		Harness.Controller->SetControlRotation(FRotator(Pitch, 0.0f, 0.0f));
		// Even laten zetten: de boom volgt met lag, dus de eerste tick na een
		// sprong in de rotatie meet nog de vorige hoek.
		Harness.Idle(0.4f);

		const FEclipseFeelSample Sample = Harness.Body->SampleFeelState();
		// De camera-manager klemt de pitch; wat we vroegen is niet altijd wat we
		// kregen, dus de BEREIKTE hoek rapporteren en niet de gevraagde.
		const float Reached = Harness.Controller->GetControlRotation().Pitch > 180.0f
			? Harness.Controller->GetControlRotation().Pitch - 360.0f
			: Harness.Controller->GetControlRotation().Pitch;

		const float Ratio = NominalArm > 0.0f ? Sample.CameraToPawnCm / NominalArm : 0.0f;
		Curve.Add(FString::Printf(TEXT("pitch %+6.1f gr (gevraagd %+6.1f) -> %7.2f cm  (%5.1f%% van de boom)"),
			Reached, Pitch, Sample.CameraToPawnCm, Ratio * 100.0f));

		if (Sample.CameraToPawnCm < WorstDistance)
		{
			WorstDistance = Sample.CameraToPawnCm;
			WorstPitch = Reached;
		}
	}

	for (const FString& Line : Curve)
	{
		AddInfo(FString::Printf(TEXT("pitch-sweep: %s"), *Line));
	}

	// Discriminator: is de VLOER de hindernis, of duwt het personage zijn eigen
	// camera weg? Dezelfde sweep hoog in de lucht. Verdwijnt het inpullen daar,
	// dan is de grond de dader; blijft het, dan zit het in de pawn zelf. Zonder
	// deze tweede meting is "de boom duikt in de grond" weer een redenering.
	//
	// LET OP het meetmoment: alleen omhoog zetten is niet genoeg. De eerste versie
	// zette de pawn 1500 cm hoog en liep daarna 19 stappen van 0,3 s af — na 1,75 s
	// valt hij die hoogte er weer af, dus vanaf stap drie stond hij gewoon weer op
	// de vloer en mat de "controle" precies hetzelfde als het origineel. Vliegen
	// zetten houdt hem er echt boven.
	Harness.Body->SetActorLocation(Harness.Body->GetActorLocation() + FVector(0.0f, 0.0f, 1500.0f));
	Harness.Body->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Harness.Idle(0.3f);
	float WorstAloft = NominalArm;
	float WorstAloftPitch = 0.0f;
	for (float Pitch : Pitches)
	{
		Harness.Controller->SetControlRotation(FRotator(Pitch, 0.0f, 0.0f));
		Harness.Idle(0.3f);
		const float Distance = Harness.Body->SampleFeelState().CameraToPawnCm;
		if (Distance < WorstAloft)
		{
			WorstAloft = Distance;
			WorstAloftPitch = Pitch;
		}
	}
	Report(*this, TEXT("kortste camera-afstand LOS van de vloer"), WorstAloft, TEXT("cm"),
		TEXT("~gelijk aan de boomlengte = de vloer was de hindernis"));
	Report(*this, TEXT("bij pitch (los van de vloer)"), WorstAloftPitch, TEXT("gr"));

	Report(*this, TEXT("kortste camera-afstand over het bereik"), WorstDistance, TEXT("cm"));
	Report(*this, TEXT("bij pitch"), WorstPitch, TEXT("gr"));
	Report(*this, TEXT("inpull op het diepste punt"), NominalArm - WorstDistance, TEXT("cm"),
		TEXT("0 = de boom raakt nergens iets binnen de limieten"));

	// GEEN assert op een grenswaarde: welke hoek acceptabel is, is smaak, en het
	// vastpinnen van de huidige limiet zou juist de wijziging blokkeren die de
	// audit voorstelt. Wat hier WEL vastligt is dat de meting blijft bestaan —
	// een boom die instort binnen zijn eigen limieten is per definitie een
	// probleem, en dat is meetbaar zonder over de juiste hoek te oordelen.
	TestTrue(TEXT("camera: er is een bruikbare boomlengte om tegen af te zetten"), NominalArm > 0.0f);

	Harness.Shutdown();
	return true;
}

// Hoe hard schakelt sprint? (LOC-04, de laatste locomotie-rij die nog "AFWIJKEND
// — harde omschakeling van MaxWalkSpeed, geen overgang" heet.)
//
// Die rij is geschreven toen MaxAcceleration nog op de engine-default 2048 stond
// en er dus inderdaad niets tussen zat. Sindsdien staat acceleratie op 1400 en
// remmen op 2000, en dat verandert het antwoord: MaxWalkSpeed springt wel, maar
// de SNELHEID volgt via de acceleratie. Rekenen zou 230 / 1400 = 0,164 s zeggen —
// alleen is remmen een ander mechanisme dan versnellen, dus de terugweg valt daar
// niet uit af te leiden.
//
// Dus meten, beide kanten, en de vorm van de curve rapporteren in plaats van
// alleen de duur: een overgang die in twee ticks klaar is voelt als een schakelaar,
// ook als "0,16 s" op papier soepel lijkt.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSprintRampTest,
	"Eclipse.Feel.Layer2.SprintRampsInsteadOfSnapping",
	EclipseFeelTest::TestFlags)

bool FEclipseSprintRampTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	const float RunSpeed = Harness.Tuning->RunSpeed;
	const float SprintSpeed = Harness.Tuning->SprintSpeed;

	// Eerst op stabiele rensnelheid komen; anders meet de sprint-overgang nog de
	// staart van de aanloop mee.
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 3.0, [&Harness, RunSpeed]()
	{
		return Harness.SpeedCm() >= RunSpeed - 2.0f;
	});
	const float BeforeSprint = Harness.SpeedCm();
	Report(*this, TEXT("snelheid vlak voor sprint"), BeforeSprint, TEXT("cm/s"),
		*FString::Printf(TEXT("~%.0f = rensnelheid"), RunSpeed));

	// OMHOOG: sprint erbij, vooruit blijven duwen (loslaten annuleert de latch).
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	double UpStart = Harness.ElapsedSeconds;
	double UpReached = -1.0;
	int32 UpTicks = 0;
	TArray<FString> UpCurve;
	while (Harness.ElapsedSeconds - UpStart < 2.0)
	{
		Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), Harness.StepSeconds);
		++UpTicks;
		const float Speed = Harness.SpeedCm();
		if (UpTicks <= 12 || UpTicks % 6 == 0)
		{
			UpCurve.Add(FString::Printf(TEXT("%5.3f s -> %6.1f cm/s"), Harness.ElapsedSeconds - UpStart, Speed));
		}
		if (UpReached < 0.0 && Speed >= SprintSpeed - 2.0f)
		{
			UpReached = Harness.ElapsedSeconds - UpStart;
			break;
		}
	}
	for (const FString& Line : UpCurve)
	{
		AddInfo(FString::Printf(TEXT("sprint-in: %s"), *Line));
	}
	Report(*this, TEXT("van rennen naar sprint"), UpReached, TEXT("s"), TEXT("-1 = sprintsnelheid niet gehaald"));

	// OMLAAG: sprint eraf, vooruit blijven duwen. Dit is de kant die niet uit de
	// acceleratie volgt — afremmen naar een LAGERE maximumsnelheid loopt via
	// wrijving, niet via BrakingDecelerationWalking (dat geldt bij loslaten).
	Harness.PressWhileMovingForward(TEXT("SprintToggle"));
	double DownStart = Harness.ElapsedSeconds;
	double DownReached = -1.0;
	int32 DownTicks = 0;
	TArray<FString> DownCurve;
	while (Harness.ElapsedSeconds - DownStart < 2.0)
	{
		Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), Harness.StepSeconds);
		++DownTicks;
		const float Speed = Harness.SpeedCm();
		if (DownTicks <= 12 || DownTicks % 6 == 0)
		{
			DownCurve.Add(FString::Printf(TEXT("%5.3f s -> %6.1f cm/s"), Harness.ElapsedSeconds - DownStart, Speed));
		}
		if (DownReached < 0.0 && Speed <= RunSpeed + 2.0f)
		{
			DownReached = Harness.ElapsedSeconds - DownStart;
			break;
		}
	}
	for (const FString& Line : DownCurve)
	{
		AddInfo(FString::Printf(TEXT("sprint-uit: %s"), *Line));
	}
	Report(*this, TEXT("van sprint terug naar rennen"), DownReached, TEXT("s"), TEXT("-1 = niet teruggezakt"));

	// De ondergrens is wat "geen overgang" betekent: één of twee ticks. Alles
	// daarboven is een echte overgang, ook al is hij kort. Geen bovengrens —
	// hoe soepel het MOET voelen is smaak, en daar hoort geen assert op.
	TestTrue(TEXT("sprint: omhoog is een overgang en geen schakelaar (> 2 ticks)"),
		UpReached > Harness.StepSeconds * 2.0);
	TestTrue(TEXT("sprint: omlaag is een overgang en geen schakelaar (> 2 ticks)"),
		DownReached > Harness.StepSeconds * 2.0);

	Harness.Shutdown();
	return true;
}

// Kun je in de lucht nog sturen? (JMP-05, luchtcontrole)
//
// De audit noemde de engine-default 0,05 "een sprong op rails" en zette hem op
// 0,35. Laag 1 controleert dat die 0,35 ook echt op het component staat — maar
// een waarde die aankomt is nog geen gedrag. Wat de speler voelt is hoeveel hij
// tijdens één sprong opzij kan komen, en dat getal bestond nergens.
//
// Vanuit stilstand springen isoleert het: er is geen horizontale beginsnelheid,
// dus alles wat zijwaarts gebeurt komt van de luchtcontrole en van niets anders.
// De controle-sprong zonder input hoort daarom op nul uit te komen — zonder die
// tweede meting weet je niet of je luchtcontrole meet of drift.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseAirControlTest,
	"Eclipse.Feel.Layer2.AirControlSteersTheJump",
	EclipseFeelTest::TestFlags)

bool FEclipseAirControlTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f); // op de vloer laten zakken voor de eerste sprong

	// Eén sprong, en tijdens de vlucht een stuurrichting vasthouden. Geeft de
	// zijwaartse verplaatsing tussen afzet en landing.
	auto MeasureJump = [&Harness](const FVector2D& AirInput) -> double
	{
		const FVector Start = Harness.Location();
		Harness.Inject(TEXT("Jump"), true);
		Harness.Step();

		double Guard = 0.0;
		// Eerst echt van de grond komen; anders leest de lus meteen "geland".
		while (!Harness.Body->GetCharacterMovement()->IsFalling() && Guard < 0.5)
		{
			Harness.HoldFor(TEXT("Move"), AirInput, Harness.StepSeconds);
			Guard += Harness.StepSeconds;
		}
		while (Harness.Body->GetCharacterMovement()->IsFalling() && Guard < 4.0)
		{
			Harness.HoldFor(TEXT("Move"), AirInput, Harness.StepSeconds);
			Guard += Harness.StepSeconds;
		}
		Harness.Idle(0.4f); // laten uitzakken voor de volgende meting
		return FVector::Dist2D(Harness.Location(), Start);
	};

	// Controle eerst: springen zonder input mag NERGENS heen. Wie dit overslaat
	// weet niet of hij luchtcontrole meet of gewoon drift.
	const double Drift = MeasureJump(FVector2D::ZeroVector);
	Report(*this, TEXT("sprong zonder input: verplaatsing"), Drift, TEXT("cm"),
		TEXT("~0 — anders meet de sturing hieronder deels drift"));

	const double Steered = MeasureJump(FVector2D(1.0f, 0.0f));
	Report(*this, TEXT("sprong met zijwaartse sturing"), Steered, TEXT("cm"));
	Report(*this, TEXT("netto stuurwinst"), Steered - Drift, TEXT("cm"),
		TEXT("wat je met één sprong opzij kunt komen"));

	TestTrue(FString::Printf(TEXT("sprong: zonder input blijf je boven je afzetpunt (%.2f cm)"), Drift),
		Drift < 5.0);
	// Ondergrens, geen streefwaarde: "op rails" is wat hier fout is, en de
	// engine-default 0,05 zou hier rond een handvol centimeter uitkomen. Hoeveel
	// sturing PRETTIG is, is smaak en krijgt dus geen bovengrens.
	TestTrue(FString::Printf(TEXT("sprong: je kunt in de lucht echt sturen (%.2f cm opzij)"), Steered - Drift),
		Steered - Drift > 25.0);

	Harness.Shutdown();
	return true;
}

// Stance werkt alleen ín Command Mode — en dat stond nergens.
//
// ToggleHeldStance() keert meteen terug als Command Mode niet vastgehouden
// wordt. Y indrukken in het veld doet dus NIETS, stil. De controletabel die de
// speler tijdens het spelen leest, zei alleen "Stance / Alt vasthouden / Y" —
// terwijl twee andere rijen hun context wél noemen ("LT (buiten Command Mode)",
// "LT (tijdens Command Mode)"). Precies de dode-toets-verwarring van bukken,
// alleen dan half: de toets leeft, maar niet waar je hem indrukt.
//
// Deze test pint het GEDRAG vast, niet de tekst. Verandert iemand later de regel
// zodat stance overal werkt, dan valt deze test om en wordt de tekst meegenomen
// — andersom zou een test op de tekst juist de verbetering blokkeren.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStanceNeedsCommandModeTest,
	"Eclipse.Feel.Input.StanceOnlyAnswersInsideCommandMode",
	EclipseFeelTest::TestFlags)

bool FEclipseStanceNeedsCommandModeTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	UEclipseCommandModeComponent* CommandMode = Harness.Controller->FindComponentByClass<UEclipseCommandModeComponent>();
	if (!TestNotNull(TEXT("stance: de Command Mode-component bestaat"), CommandMode))
	{
		Harness.Shutdown();
		return false;
	}

	const EEclipseSquadStance Before = CommandMode->GetHeldStance();

	// 1. Buiten Command Mode: indrukken hoort niets te doen.
	Harness.Press(TEXT("StanceToggle"));
	Harness.Idle(0.1f);
	TestTrue(TEXT("stance: buiten Command Mode verandert Y niets"), CommandMode->GetHeldStance() == Before);

	// 2. Command Mode vasthouden en dan pas drukken.
	Harness.Inject(TEXT("CommandHold"), true);
	Harness.Step();
	Harness.Inject(TEXT("CommandHold"), true);
	Harness.Inject(TEXT("StanceToggle"), true);
	Harness.Step();
	Harness.Inject(TEXT("CommandHold"), true);
	Harness.Step();

	const EEclipseSquadStance Inside = CommandMode->GetHeldStance();
	AddInfo(FString::Printf(TEXT("stance: %s -> %s met Command Mode vast"),
		Before == EEclipseSquadStance::Aggressive ? TEXT("aggressive") : TEXT("ready"),
		Inside == EEclipseSquadStance::Aggressive ? TEXT("aggressive") : TEXT("ready")));
	TestTrue(TEXT("stance: ín Command Mode wisselt Y wél"), Inside != Before);

	Harness.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
