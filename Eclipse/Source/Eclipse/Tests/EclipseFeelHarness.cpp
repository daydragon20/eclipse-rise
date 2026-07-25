#include "Tests/EclipseFeelHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipsePlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "Misc/AutomationTest.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"

namespace EclipseFeelHarness
{
	namespace
	{
		/** Vlakke, ruime vloer op Z = 0. Movable, niet Static: een Static actor die
		 *  tijdens het spelen gespawnd wordt is in UE een waarschuwing waard, en
		 *  bewegen doet hij toch niet. */
		bool SpawnFloor(UWorld& World)
		{
			UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			if (Cube == nullptr)
			{
				return false;
			}
			AStaticMeshActor* Floor = World.SpawnActor<AStaticMeshActor>(FVector(0.0f, 0.0f, -50.0f), FRotator::ZeroRotator);
			if (Floor == nullptr)
			{
				return false;
			}
			UStaticMeshComponent* Mesh = Floor->GetStaticMeshComponent();
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetStaticMesh(Cube);
			Mesh->SetCollisionProfileName(TEXT("BlockAll"));
			// De engine-kubus is 100 uu; 400x400 geeft 40 x 40 m vloer, ruim genoeg
			// voor een sprint van vier seconden plus een 180-omkering.
			Floor->SetActorScale3D(FVector(400.0f, 400.0f, 1.0f));
			return true;
		}
	}

	bool FHarness::Start(FAutomationTestBase& Test)
	{
		// CommonUI klaagt luid (op Error-niveau) zodra er een lokale speler bijkomt
		// zonder CommonGameViewportClient. Dat is hier correct gedrag — er IS geen
		// viewport, dat is het punt van headless — maar een Error laat de test
		// vallen. Verwacht hem expliciet in plaats van hem te dempen, zodat een
		// ANDERE UI-fout nog steeds gewoon rood wordt.
		Test.AddExpectedError(TEXT("Using CommonUI without a CommonGameViewportClient"),
			EAutomationExpectedErrorFlags::Contains, /*Occurrences*/ 0);

		GameInstance = NewObject<UGameInstance>(GEngine);
		GameInstance->InitializeStandalone();
		World = GameInstance->GetWorld();
		if (!Test.TestNotNull(TEXT("harnas: standalone wereld"), World))
		{
			return false;
		}

		if (!Test.TestTrue(TEXT("harnas: vloer gespawnd"), SpawnFloor(*World)))
		{
			return false;
		}

		World->InitializeActorsForPlay(FURL());

		// UWorld::BeginPlay() is NIET wat de wereld op "begonnen" zet — dat doet de
		// GameMode, via GameState -> AWorldSettings::NotifyBeginPlay. Zonder game
		// mode blijft bBegunPlay dus false en krijgt GEEN ENKELE actor ooit
		// BeginPlay: de controller startte zijn campagne niet en er was geen
		// tuningbron. Eén regel, en het is de echte engine-route, geen omweg.
		World->BeginPlay();
		if (AWorldSettings* Settings = World->GetWorldSettings())
		{
			Settings->NotifyBeginPlay();
		}

		// Een ECHTE ULocalPlayer, want daar hangt de Enhanced-Input-subsystem aan.
		// Handmatig in plaats van CreateLocalPlayer: die eist een game viewport en
		// laat er anders een ensure op los, en een harnas dat elke run een ensure
		// stookt maakt het log onleesbaar voor de echte fouten.
		LocalPlayer = NewObject<ULocalPlayer>(GEngine, GEngine->LocalPlayerClass);
		if (!Test.TestNotNull(TEXT("harnas: lokale speler"), LocalPlayer))
		{
			return false;
		}
		GameInstance->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));

		Body = World->SpawnActor<AEclipseCharacter>(FVector(0.0f, 0.0f, 120.0f), FRotator::ZeroRotator);
		Controller = World->SpawnActor<AEclipsePlayerController>();
		if (!Test.TestNotNull(TEXT("harnas: pawn"), Body) || !Test.TestNotNull(TEXT("harnas: controller"), Controller))
		{
			return false;
		}

		// SetPlayer draait InitInputSystem en dus SetupInputComponent: pas hierna
		// bestaan de UInputAction-objecten waarop we injecteren.
		Controller->SetPlayer(LocalPlayer);
		Controller->Possess(Body);

		Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
		if (!Test.TestNotNull(TEXT("harnas: Enhanced Input subsystem"), Input))
		{
			return false;
		}
		// De mapping context is voor injectie niet nodig, maar wél voor realisme:
		// zo draait de meting op exact de stack die een speler ook heeft.
		if (UInputMappingContext* Context = Controller->GetMappingContext())
		{
			Input->AddMappingContext(Context, /*Priority*/ 0);
		}

		// Tuning uit de ECHTE bron: de campagne-setup die de controller bij
		// BeginPlay al heeft gestart. Laag 1 vergelijkt hiertegen, dus hij mag niet
		// uit een testfixture komen — dan zou de test zijn eigen antwoord meebrengen.
		const UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
		const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
		Tuning = Setup != nullptr ? Setup->CharacterTuning.LoadSynchronous() : nullptr;
		if (Tuning == nullptr)
		{
			Test.AddError(TEXT("harnas: geen DA_CharacterTuning via de campagne-setup — laag 1 heeft geen bron om tegen te vergelijken."));
			return false;
		}
		Body->ApplyTuning(Tuning);

		// Laten landen: neerkomen op de vloer en de spawn-impuls kwijtraken, zodat
		// elke meting vanaf dezelfde rusttoestand vertrekt.
		Idle(0.75f);
		ElapsedSeconds = 0.0;

		// Eén read-only zelfcontrole. Zegt een van deze regels ooit "nee", dan meet
		// het harnas niets meer, en dat moet luid zijn in plaats van als een reeks
		// nullen die op een gameplay-defect lijken.
		Test.AddInfo(FString::Printf(
			TEXT("harnas: lokaal=%d · %s · enhanced-component=%d · staat=%s · op-de-grond=%d · wereld-begonnen=%d · tuning='%s'"),
			Controller->IsLocalPlayerController() ? 1 : 0,
			*GetNameSafe(Controller->PlayerInput),
			Cast<UEnhancedInputComponent>(Controller->InputComponent) != nullptr ? 1 : 0,
			*Controller->GetStateName().ToString(),
			Body->GetCharacterMovement()->IsMovingOnGround() ? 1 : 0,
			World->GetBegunPlay() ? 1 : 0,
			*GetNameSafe(Tuning)));

		// En het bewijs dat injectie ook echt AANKOMT, vóór er iets gemeten wordt.
		// Zonder deze regel is "de injectie komt niet aan" niet te onderscheiden van
		// "hij komt aan maar het personage beweegt niet" — en dat onderscheid kostte
		// deze sessie al een iteratie (het bleek GFrameCounter, zie Step()).
		Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
		Step();
		Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
		Step();
		Test.TestTrue(FString::Printf(TEXT("harnas: geïnjecteerde beweging komt aan (%.2f cm/s na 2 ticks)"), SpeedCm()),
			SpeedCm() > 1.0f);

		// Terug naar rust, zodat de eerste echte meting vanaf stilstand vertrekt.
		HoldFor(TEXT("Move"), FVector2D::ZeroVector, 1.0, [this]() { return SpeedCm() < 0.5f; });
		ElapsedSeconds = 0.0;
		return true;
	}

	void FHarness::Shutdown()
	{
		if (GameInstance != nullptr)
		{
			GameInstance->Shutdown();
		}
		GameInstance = nullptr;
		World = nullptr;
		Controller = nullptr;
		Body = nullptr;
		Input = nullptr;
		Tuning = nullptr;
	}

	void FHarness::Step(float DeltaSeconds)
	{
		if (World == nullptr)
		{
			return;
		}
		// GFrameCounter OPHOGEN, en dat is geen detail: FTickFunction::QueueTickFunction
		// onthoudt per tick-functie in welk frame hij al bezocht is
		// (TickVisitedGFrameCounter) en slaat hem daarna over. Een test draait al zijn
		// wereldticks binnen ÉÉN engine-frame, dus zonder deze regel wordt er na de
		// eerste ronde niets meer gepland: geen actor tikt, geen component tikt, en de
		// speler-invoer wordt nooit verwerkt. Precies de toestand die dit harnas eerst
		// mat — 480 ticks, nul beweging.
		++GFrameCounter;
		World->Tick(LEVELTICK_All, DeltaSeconds);
		ElapsedSeconds += DeltaSeconds;
	}

	void FHarness::Idle(float Seconds)
	{
		const int32 Steps = FMath::Max(1, FMath::RoundToInt(Seconds / FixedStepSeconds));
		for (int32 I = 0; I < Steps; ++I)
		{
			Step();
		}
	}

	void FHarness::Inject(FName ActionName, const FVector2D& Value)
	{
		if (Input == nullptr || Controller == nullptr)
		{
			return;
		}
		if (const UInputAction* Action = Controller->FindInputAction(ActionName))
		{
			Input->InjectInputForAction(Action, FInputActionValue(Value), {}, {});
		}
	}

	void FHarness::Inject(FName ActionName, bool bPressed)
	{
		if (Input == nullptr || Controller == nullptr || !bPressed)
		{
			return; // "niet injecteren" IS het loslaten (Enhanced Input strip-pad)
		}
		if (const UInputAction* Action = Controller->FindInputAction(ActionName))
		{
			Input->InjectInputForAction(Action, FInputActionValue(true), {}, {});
		}
	}

	double FHarness::HoldFor(FName ActionName, const FVector2D& Value, double Seconds, TFunctionRef<bool()> StopWhen)
	{
		const double Start = ElapsedSeconds;
		const int32 MaxSteps = FMath::Max(1, FMath::RoundToInt(Seconds / FixedStepSeconds));
		for (int32 I = 0; I < MaxSteps; ++I)
		{
			Inject(ActionName, Value);
			Step();
			if (StopWhen())
			{
				break;
			}
		}
		return ElapsedSeconds - Start;
	}

	double FHarness::HoldFor(FName ActionName, const FVector2D& Value, double Seconds)
	{
		return HoldFor(ActionName, Value, Seconds, []() { return false; });
	}

	void FHarness::Press(FName ActionName)
	{
		Inject(ActionName, true);
		Step();
		// Eén tick zonder injectie: dát is wat Enhanced Input als loslaten leest en
		// wat een Completed laat vuren.
		Step();
	}

	void FHarness::PressWhileMovingForward(FName ActionName)
	{
		Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
		Inject(ActionName, true);
		Step();
		Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
		Step();
	}

	float FHarness::SpeedCm() const
	{
		const UCharacterMovementComponent* Movement = Body != nullptr ? Body->GetCharacterMovement() : nullptr;
		return Movement != nullptr ? Movement->Velocity.Size2D() : 0.0f;
	}

	FVector FHarness::Location() const
	{
		return Body != nullptr ? Body->GetActorLocation() : FVector::ZeroVector;
	}

	bool FHarness::RunUpToTopSpeed(FAutomationTestBase& Test, double MaxSeconds)
	{
		const float Target = Body != nullptr ? Body->GetCharacterMovement()->MaxWalkSpeed : 0.0f;
		HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), MaxSeconds,
			[this, Target]() { return SpeedCm() >= Target * 0.995f; });
		const bool bReached = SpeedCm() >= Target * 0.98f;
		Test.TestTrue(FString::Printf(TEXT("harnas: topsnelheid gehaald (%.0f van %.0f cm/s)"), SpeedCm(), Target), bReached);
		return bReached;
	}

	void Report(FAutomationTestBase& Test, const TCHAR* Label, double Value, const TCHAR* Unit, const TCHAR* Expectation)
	{
		// AddInfo en niet UE_LOG: dan staat het getal in het automation-rapport dat
		// de bar oplevert, en niet alleen in een logbestand dat niemand opent.
		Test.AddInfo(Expectation != nullptr
			? FString::Printf(TEXT("GEMETEN  %-38s %10.3f %s   (verwacht: %s)"), Label, Value, Unit, Expectation)
			: FString::Printf(TEXT("GEMETEN  %-38s %10.3f %s"), Label, Value, Unit));
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
