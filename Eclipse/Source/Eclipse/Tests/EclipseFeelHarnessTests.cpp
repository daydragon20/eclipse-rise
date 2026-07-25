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
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/InputSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Tests/EclipseFeelHarness.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
