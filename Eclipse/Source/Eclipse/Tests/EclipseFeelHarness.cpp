#include "Tests/EclipseFeelHarness.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipsePlayerController.h"
#include "Core/EclipseGameMode.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Eclipse.h"
#include "Engine/World.h"
#include "AI/NavigationSystemBase.h"
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

	bool FHarness::Start(FAutomationTestBase& Test, const FOptions& Options)
	{
		StepSeconds = Options.StepSeconds;
		// Hier stond een AddExpectedError voor "Using CommonUI without a
		// CommonGameViewportClient". Die verwachting is 26-07 laat weggehaald,
		// omdat de OORZAAK is weggenomen: DefaultEngine.ini wijst nu
		// CommonGameViewportClient aan, zoals de plugin zelf eist.
		//
		// De oude opmerking hier zei dat de error "correct gedrag" was omdat er
		// headless geen viewport is. Dat klopte niet: de engine meldde dat
		// INVOERROUTERING niet goed werkt, en dat gold ook in het echte spel —
		// waar de owner precies dat symptoom zag (de View-knop kwam niet aan).
		// Een verwachte fout is een geaccepteerde fout, en die had hier niet
		// geaccepteerd mogen worden.
		//
		// 47 tests werden rood toen de error verdween. Dat is de bedoeling van
		// AddExpectedError en het bewijst dat de verwachting echt hing aan wat
		// hij beweerde.
		OwningTest = &Test;

		GameInstance = NewObject<UGameInstance>(GEngine);
		GameInstance->InitializeStandalone();
		World = GameInstance->GetWorld();
		if (!Test.TestNotNull(TEXT("harnas: standalone wereld"), World))
		{
			return false;
		}

		if (Options.bRealGameMode)
		{
			// De ECHTE game mode: AEclipseGameMode::InitGame bouwt het district (en
			// levert dus de vloer, de sites, de objective-triggers en Entry_Main),
			// en zijn Event.Mission.Started-abonnement spawnt squad en vijanden.
			// SetGameMode moet vóór InitializeActorsForPlay, want die roept InitGame
			// aan op de game mode die er dan staat.
			World->GetWorldSettings()->DefaultGameMode = AEclipseGameMode::StaticClass();
			if (!Test.TestTrue(TEXT("harnas: game mode aangemaakt"), World->SetGameMode(FURL())))
			{
				return false;
			}

			// EXPLICIET een navigatiesysteem, want UWorld::CreateWorld maakt er voor
			// een Game-wereld géén (de InitializationValues zetten CreateNavigation
			// alleen aan voor Editor-werelden). Zonder dit bestaat er geen navmesh,
			// faalt elke MoveToLocation, en weigert de squad ELKE order met NoRoute
			// — wat er van buitenaf uitziet als een gameplay-defect maar een gat in
			// het harnas is. Precies het soort verschil dat je alleen ziet door te
			// meten: de eerste ronde rapporteerde "navigatiesysteem ONTBREEKT".
			FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::GameMode);
		}
		else if (!Test.TestTrue(TEXT("harnas: vloer gespawnd"), SpawnFloor(*World)))
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
		if (DeltaSeconds <= 0.0f)
		{
			DeltaSeconds = StepSeconds;
		}
		const double StepStart = FPlatformTime::Seconds();
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

		const double StepMs = (FPlatformTime::Seconds() - StepStart) * 1000.0;
		WorstStepMs = FMath::Max(WorstStepMs, StepMs);
		TotalStepMs += StepMs;
		++StepCount;
	}

	void FHarness::Idle(float Seconds)
	{
		const int32 Steps = FMath::Max(1, FMath::RoundToInt(Seconds / StepSeconds));
		for (int32 I = 0; I < Steps; ++I)
		{
			Step();
		}
	}

	void FHarness::ReportUnknownAction(FName ActionName)
	{
		// Een onbekende actienaam injecteerde NIETS en zei er niets over, dus de
		// test tikte netjes door met een speler die geen knop indrukte. Zo kostte
		// een sprint-meting een hele ronde: de acties heten SprintHold en
		// SprintToggle, "Sprint" bestaat niet, en het resultaat las als "sprint
		// werkt niet" in plaats van "die knop bestaat niet".
		//
		// Eén keer per naam: een tikfout in een lus van 120 ticks moet geen 120
		// regels opleveren.
		if (OwningTest != nullptr && !UnknownActionsReported.Contains(ActionName))
		{
			UnknownActionsReported.Add(ActionName);
			OwningTest->AddError(FString::Printf(
				TEXT("harnas: actie '%s' bestaat niet — die injectie deed NIETS. ")
				TEXT("Controleer de naam tegen AEclipsePlayerController::FindInputAction."),
				*ActionName.ToString()));
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
		else
		{
			ReportUnknownAction(ActionName);
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
		else
		{
			ReportUnknownAction(ActionName);
		}
	}

	double FHarness::HoldFor(FName ActionName, const FVector2D& Value, double Seconds, TFunctionRef<bool()> StopWhen)
	{
		const double Start = ElapsedSeconds;
		const int32 MaxSteps = FMath::Max(1, FMath::RoundToInt(Seconds / StepSeconds));
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

	void FHarness::AimAt(const FVector& WorldPoint)
	{
		if (Controller == nullptr || Body == nullptr || Tuning == nullptr)
		{
			return;
		}
		// Richten vanaf het VIEW POINT, niet vanaf de pawn. Het wapen traceert vanaf
		// de camera (AEclipsePlayerController::HandleFire) en die hangt 300 uu
		// achter en 55 uu naast het lichaam. Op 42 meter is die zijwaartse offset
		// 0,75 graden, terwijl een capsule van 34 cm daar 0,46 graden beslaat —
		// dus richten vanaf de pawn mist STRUCTUREEL. Eerste ronde: 5771 ticks
		// gevuurd, 0 van de 4 vijanden geraakt. Een speler heeft dit probleem niet,
		// want hij richt op wat er in het schermmidden staat, en dat IS de
		// camerastraal.
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const FRotator Desired = (WorldPoint - ViewLocation).Rotation();
		const FRotator Current = Controller->GetControlRotation();
		const float YawError = FMath::FindDeltaAngleDegrees(Current.Yaw, Desired.Yaw);
		const float PitchError = FMath::FindDeltaAngleDegrees(Current.Pitch, Desired.Pitch);
		// De muis-tak is lineair: AddYawInput(Axis.X * MouseLookScale) graden per
		// gebeurtenis, dus dit is exact de invoer die de fout in één tick wegdraait.
		const float Scale = FMath::Max(Tuning->MouseLookScale, KINDA_SMALL_NUMBER);
		Inject(TEXT("Look"), FVector2D(YawError / Scale, PitchError / Scale));
	}

	bool FHarness::DriveTo(const FVector& Target, double MaxSeconds, float ArriveRadiusCm, bool bSprint)
	{
		const int32 MaxSteps = FMath::Max(1, FMath::RoundToInt(MaxSeconds / StepSeconds));
		// Vastloop-herstel. Het district heeft muren en compounds, dus een rechte
		// lijn naar een routepunt loopt vroeg of laat ergens tegenaan — de eerste
		// ronde bleef 897 cm voor het controlepost hangen tegen de compoundmuur.
		// Een mens zet dan een paar stappen opzij en probeert opnieuw; dat doet dit
		// ook. Geen pathfinding, wél een route die aankomt.
		const int32 ProgressWindow = FMath::Max(1, FMath::RoundToInt(0.5f / StepSeconds));
		float DistanceAtWindowStart = FVector::Dist2D(Body->GetActorLocation(), Target);
		int32 StepsInWindow = 0;
		int32 SidestepStepsLeft = 0;
		float SidestepDirection = 1.0f;

		for (int32 I = 0; I < MaxSteps; ++I)
		{
			const float Distance = FVector::Dist2D(Body->GetActorLocation(), Target);
			if (Distance <= ArriveRadiusCm)
			{
				return true;
			}

			if (++StepsInWindow >= ProgressWindow)
			{
				if (SidestepStepsLeft <= 0 && DistanceAtWindowStart - Distance < 20.0f)
				{
					// Geen voortgang in een halve seconde: opzij, en de volgende keer
					// de andere kant op zodat we niet in dezelfde hoek blijven duwen.
					SidestepStepsLeft = FMath::Max(1, FMath::RoundToInt(0.8f / StepSeconds));
					SidestepDirection = -SidestepDirection;
				}
				DistanceAtWindowStart = Distance;
				StepsInWindow = 0;
			}

			AimAt(FVector(Target.X, Target.Y, Body->GetActorLocation().Z + 60.0f));
			if (SidestepStepsLeft > 0)
			{
				--SidestepStepsLeft;
				Inject(TEXT("Move"), FVector2D(SidestepDirection, 0.35f));
			}
			else
			{
				Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
			}
			if (bSprint && !Controller->IsSprintLatched())
			{
				Inject(TEXT("SprintToggle"), true);
			}
			Step();
		}
		return FVector::Dist2D(Body->GetActorLocation(), Target) <= ArriveRadiusCm;
	}

	bool FHarness::EngageHostile(AEclipseCharacter& Hostile, double MaxSeconds)
	{
		const int32 MaxSteps = FMath::Max(1, FMath::RoundToInt(MaxSeconds / StepSeconds));
		for (int32 I = 0; I < MaxSteps && !Hostile.IsDowned(); ++I)
		{
			// Mikken op borsthoogte, niet op de actor-oorsprong: die ligt bij een
			// character op vloerniveau en een schot daarheen gaat onder hem door.
			AimAt(Hostile.GetActorLocation() + FVector(0.0f, 0.0f, 40.0f));
			Inject(TEXT("Fire"), true);
			Step();
		}
		return Hostile.IsDowned();
	}

	float FHarness::MeasureStepUp(float HeightCm, double MaxSeconds)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (Cube == nullptr || World == nullptr || Body == nullptr)
		{
			return 0.0f;
		}

		// Tot stilstand komen en de kijkrichting nullen, zodat "vooruit" +X is en
		// het blok gegarandeerd op de looplijn staat.
		HoldFor(TEXT("Move"), FVector2D::ZeroVector, 1.5, [this]() { return SpeedCm() < 0.5f; });
		Controller->SetControlRotation(FRotator::ZeroRotator);
		Idle(0.1f);

		const FVector Start = Body->GetActorLocation();
		// Blok met zijn BOVENKANT op HeightCm boven de vloer (vloer = Z 0). De
		// engine-kubus is 100 uu, dus schaal Z = hoogte / 100 en het middelpunt
		// ligt op de halve hoogte.
		// 250 vooruit met een halve diepte van 100: de voorkant staat op 150 cm van
		// de startpositie, dus ruim buiten de capsule (radius 34) én met genoeg
		// aanloop. De eerste versie was 600 uu diep en stond daarmee bovenop het
		// personage — dat mat 0 cm hoogtewinst op elke hoogte, wat op een defect
		// leek maar de meetopstelling was.
		AStaticMeshActor* StepActor = World->SpawnActor<AStaticMeshActor>(
			FVector(Start.X + 250.0f, Start.Y, HeightCm * 0.5f), FRotator::ZeroRotator);
		if (StepActor == nullptr)
		{
			return 0.0f;
		}
		UStaticMeshComponent* Mesh = StepActor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		StepActor->SetActorScale3D(FVector(2.0f, 8.0f, FMath::Max(HeightCm, 1.0f) / 100.0f));

		// De HOOGSTE stand tijdens de wandeling, niet de eindstand. Een blok van
		// 20 cm wordt beklommen én er wordt aan de andere kant weer afgestapt, dus
		// de eindhoogte is gewoon de vloer — die eerste versie mat 1198 cm afgelegd
		// met 0 cm winst en zag er daardoor uit als "geblokkeerd" terwijl hij er
		// juist overheen liep.
		float Peak = static_cast<float>(Start.Z);
		{
			const int32 MaxSteps = FMath::Max(1, FMath::RoundToInt(MaxSeconds / StepSeconds));
			for (int32 I = 0; I < MaxSteps; ++I)
			{
				Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
				Step();
				Peak = FMath::Max(Peak, static_cast<float>(Body->GetActorLocation().Z));
			}
		}
		const float Gained = Peak - static_cast<float>(Start.Z);
		UE_LOG(LogEclipse, Display, TEXT("harnas stap-meting: blok %.0f cm · horizontaal afgelegd %.1f cm · hoogste stand +%.2f cm"),
			HeightCm, FVector::Dist2D(Body->GetActorLocation(), Start), Gained);
		StepActor->Destroy();
		// Terug naar de vertrekhoogte, anders erft de volgende meting deze stap.
		Body->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
		Idle(0.3f);
		return Gained;
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
