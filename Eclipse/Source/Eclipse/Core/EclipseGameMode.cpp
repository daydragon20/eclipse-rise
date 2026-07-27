#include "Core/EclipseGameMode.h"

#include "AI/EclipseEnemyController.h"
#include "AI/EclipseSquadmateController.h"
#include "Base/EclipsePrepSubsystem.h"
#include "Base/EclipsePrepTypes.h"
#include "Animation/SkeletalMeshActor.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipseClassLogic.h"
#include "Characters/EclipsePlayerController.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Core/EclipseGrayboxBuilder.h"
#include "Eclipse.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Scalability.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "Squad/EclipseSquadTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseStrategySubsystem.h"

namespace
{
	/** The body's hitscan weapon, created on demand (the engine-spawned player pawn has none). */
	UEclipseHitscanWeaponComponent& EnsureWeapon(AEclipseCharacter& Body)
	{
		UEclipseHitscanWeaponComponent* Weapon = Body.FindComponentByClass<UEclipseHitscanWeaponComponent>();
		if (Weapon == nullptr)
		{
			Weapon = NewObject<UEclipseHitscanWeaponComponent>(&Body);
			Body.AddOwnedComponent(Weapon);
			Weapon->RegisterComponent();
		}
		return *Weapon;
	}

	/**
	 * First row of a typed table. PLACEHOLDER(SPEC-P1-05/08): loadout choice maps
	 * to a specific row when the content pass lands; Phase 1 carries one platform
	 * and one archetype, so "first" is the whole catalog.
	 */
	template <typename TRow>
	const TRow* FirstRowOf(const UDataTable* Table)
	{
		if (Table == nullptr || Table->GetRowStruct() != TRow::StaticStruct())
		{
			return nullptr;
		}
		for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
		{
			return reinterpret_cast<const TRow*>(Row.Value);
		}
		return nullptr;
	}
}

AEclipseGameMode::AEclipseGameMode()
{
	DefaultPawnClass = AEclipseCharacter::StaticClass();
	PlayerControllerClass = AEclipsePlayerController::StaticClass();
}

void AEclipseGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// The district builds itself when the map doesn't carry it (SPEC-P1-05:
	// reproducible-from-code graybox until the art pass authors a real map).
	if (GetWorld() != nullptr && !EclipseGraybox::IsDistrictPresent(*GetWorld()))
	{
		EclipseGraybox::BuildDistrict(*GetWorld());
	}
}

void AEclipseGameMode::OnShotFired(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseCombatEventPayload* Shot = Payload.GetPtr<FEclipseCombatEventPayload>();
	UWorld* World = GetWorld();
	if (Shot == nullptr || World == nullptr || Shot->AlertRadiusCm <= 0.0f)
	{
		return;
	}

	// Alleen schoten van de SPELERSKANT alarmeren. Een vijand die vuurt heeft je al
	// gezien, en zijn eigen schoten door de hele groep laten cascaderen zou van één
	// waarneming een district-brede opstand maken — dat is een moeilijkheidskeuze
	// die de owner niet gevraagd heeft. De knop staat klaar als hij hem wil.
	if (!Shot->bPlayerSide)
	{
		return;
	}

	const float RadiusSquared = FMath::Square(Shot->AlertRadiusCm);
	int32 Alerted = 0;
	for (TActorIterator<AEclipseEnemyController> It(World); It; ++It)
	{
		AEclipseEnemyController* Enemy = *It;
		const APawn* Body = Enemy != nullptr ? Enemy->GetPawn() : nullptr;
		if (Body == nullptr)
		{
			continue;
		}
		if (FVector::DistSquared(Body->GetActorLocation(), Shot->Origin) > RadiusSquared)
		{
			continue;
		}
		Enemy->NotifyGunshotHeard(Shot->Origin);
		++Alerted;
	}

	// Het schot zet OOK het alarm aan: gehoord worden is verraden worden. De latch
	// is idempotent, dus dit botst niet met het alarm-op-eerste-waarneming — wie
	// eerst is, is eerst, en een tweede hoorn is geen nieuw feit.
	EnemiesAlertedByShots += Alerted;
	if (Alerted > 0)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>())
			{
				Mission->NotifyAlarmRaised();
			}
		}
	}

	if (!bLoggedFirstShotAlert && Alerted > 0)
	{
		bLoggedFirstShotAlert = true;
		UE_LOG(LogEclipse, Display,
			TEXT("Schot gehoord door %d vijand(en) binnen %.0f cm — ze lopen naar de plek waar geschoten werd, niet naar de speler."),
			Alerted, Shot->AlertRadiusCm);
	}
}

void AEclipseGameMode::StartPlay()
{
	Super::StartPlay();

	// The mission lifecycle drives ground actors (SPEC-P1-05): spawn when a run
	// starts, tear down at debrief — so a launch after boot works, not only a
	// mission that happened to be active at StartPlay.
	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		MissionEventsHandle = Bus->Subscribe(
			FGameplayTag::RequestGameplayTag(TEXT("Event.Mission")),
			FEclipseEventNativeDelegate::CreateUObject(this, &AEclipseGameMode::OnMissionLifecycle));

		// Schoten vertalen naar wie ze hoort (26-07, punt 1). Hier en niet in het
		// wapen: het wapen hoort niet te weten dat er vijanden bestaan, en de game
		// mode kent de gespawnde actoren al.
		ShotFiredHandle = Bus->Subscribe(
			EclipseTags::Event_Combat_ShotFired,
			FEclipseEventNativeDelegate::CreateUObject(this, &AEclipseGameMode::OnShotFired),
			FEclipseCombatEventPayload::StaticStruct());
	}

	// A mission already running at boot (e.g. after a load) still populates.
	if (const UEclipseMissionSubsystem* Mission = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseMissionSubsystem>() : nullptr)
	{
		if (Mission->GetPhase() == EEclipseMissionPhase::Objectives)
		{
			SpawnMissionActors();
		}
	}

#if !UE_BUILD_SHIPPING
	SetupShotRig();
	SetupPlayShotRound();
	StartMissionFromCommandLine();
#endif
}

#if !UE_BUILD_SHIPPING
void AEclipseGameMode::StartMissionFromCommandLine()
{
	// Playtest finding 13.2 (owner, 2026-07-25): the feel-gauntlet has to run dozens
	// of times, and clicking through the base hub every time costs minutes and
	// pollutes the measurement. -EclipseStartMission=<RegionId> lands straight in a
	// mission. Same family as -EclipseShot: a debug entry point, not a new system —
	// it drives the SAME seam the hub uses (SelectMission -> AutoLaunch), so a bug in
	// the real path cannot hide behind the shortcut.
	FString RegionId;
	if (!FParse::Value(FCommandLine::Get(), TEXT("EclipseStartMission="), RegionId) || RegionId.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UEclipseStrategySubsystem* Strategy = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseStrategySubsystem>() : nullptr;
	UEclipsePrepSubsystem* Prep = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipsePrepSubsystem>() : nullptr;
	if (Strategy == nullptr || Prep == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("-EclipseStartMission: strategy/prep subsystem unavailable — starting in the hub instead (GDD 14.3.5)."));
		return;
	}

	FString Error;
	if (!Strategy->SelectMission(FName(*RegionId), Error))
	{
		UE_LOG(LogEclipse, Warning, TEXT("-EclipseStartMission=%s: %s — starting in the hub instead. Use a region id from the campaign graph (e.g. TransitCheckpoint)."), *RegionId, *Error);
		return;
	}
	if (!Prep->AutoLaunch(Error))
	{
		UE_LOG(LogEclipse, Warning, TEXT("-EclipseStartMission=%s: selected, but launch failed: %s — starting in the hub instead."), *RegionId, *Error);
		return;
	}
	UE_LOG(LogEclipse, Display, TEXT("-EclipseStartMission=%s: mission running, hub skipped."), *RegionId);
}
#endif

#if !UE_BUILD_SHIPPING
void AEclipseGameMode::SetupPlayShotRound()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("EclipseShotPlay")))
	{
		return;
	}

	// Volle kwaliteit, zelfde reden als bij de review-ronde: een beoordeling mag
	// niet afhangen van wat deze laptop autodetecteert.
	Scalability::FQualityLevels Quality;
	Quality.SetFromSingleQualityLevel(3);
	Scalability::SetQualityLevels(Quality);

	// 5 s voordat de eerste opname valt: streaming en belichting moeten settelen,
	// anders beoordeel je een half geladen frame.
	GetWorldTimerManager().SetTimer(PlayShotTimer, this, &AEclipseGameMode::AdvancePlayShotRound,
		2.0f, /*bLoop*/ true, /*FirstDelay*/ 5.0f);
	// De invoerduw loopt sneller: bewegingsinvoer moet elke tick binnenkomen,
	// anders staat hij op het moment van de opname alweer stil.
	GetWorldTimerManager().SetTimer(PlayShotDriveTimer, this, &AEclipseGameMode::DrivePlayShotInput,
		0.02f, /*bLoop*/ true);
	UE_LOG(LogEclipse, Display, TEXT("PlayShot: armed — opnames vanuit de speler tijdens het spelen."));
}

void AEclipseGameMode::DrivePlayShotInput()
{
	APlayerController* Controller = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	AEclipseCharacter* Body = Controller != nullptr ? Cast<AEclipseCharacter>(Controller->GetPawn()) : nullptr;
	if (Body == nullptr)
	{
		return;
	}
	if (bPlayShotWalking)
	{
		// Rechtdoor, camera-relatief — precies wat de speler doet.
		const FRotator YawOnly(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		Body->AddMovementInput(FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X), 1.0f);
	}
	if (bPlayShotTurning)
	{
		// Via AddYawInput en niet met SetControlRotation: dit is dezelfde weg die
		// jouw muis neemt, dus een fout in die weg kan zich hier niet achter
		// verstoppen.
		Controller->AddYawInput(1.2f);
	}
	if (bPlayShotFiring)
	{
		if (UEclipseHitscanWeaponComponent* Weapon = Body->FindComponentByClass<UEclipseHitscanWeaponComponent>())
		{
			const FVector Origin = Body->GetPawnViewLocation();
			Weapon->Fire(Origin, Controller->GetControlRotation().Vector(), TEXT("PlayShot"));
		}
	}
}

void AEclipseGameMode::MeasurePlayShot(int32 ShotIndex)
{
	APlayerController* Controller = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (Controller == nullptr)
	{
		return;
	}

	// Een beeld alleen is niet genoeg: op een screenshot zie je DAT er een figuur
	// groeit, niet WELKE actor het is of met hoeveel. Deze regels hangen een naam
	// en een maat aan elke vorm in het frame, zodat de beoordeling van het beeld
	// een bevinding wordt in plaats van een vermoeden.
	//
	// Schermpositie staat erbij omdat dat de enige manier is om "de gele links"
	// aan een actor te koppelen zonder te gokken.
	FVector CameraLocation;
	FRotator CameraRotation;
	Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// Zonder de vensterafmeting zijn de schermcoordinaten hieronder onvertaalbaar
	// naar de PNG, en dan wijs je alsnog met je vinger naar het verkeerde figuur.
	int32 ViewportX = 0;
	int32 ViewportY = 0;
	Controller->GetViewportSize(ViewportX, ViewportY);
	UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT %d MEET] venster=%dx%d"), ShotIndex, ViewportX, ViewportY);

	// HOE SNEL DRAAIT HET ECHT. De speelronde in de suite meet de game-thread
	// headless op 0,80 ms, en dat getal zegt niets over wat de owner ziet: daar
	// wordt niet gerenderd. Dit is de enige plek in het project waar een ECHTE
	// frame draait, dus dit is de enige plek waar dit te meten valt.
	//
	// GAverageMS is het lopende gemiddelde van de engine zelf; geen eigen teller,
	// want een tweede manier van meten geeft een tweede getal om te vertrouwen.
	extern ENGINE_API float GAverageMS;
	extern ENGINE_API float GAverageFPS;
	UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT %d TEMPO] %.1f ms per frame (%.0f fps) op %dx%d"),
		ShotIndex, GAverageMS, GAverageFPS, ViewportX, ViewportY);

	// GDD 12.4 legt het budget op 16,7 ms (60 fps). Ruim daarboven is geen
	// smaakkwestie meer: dan voelt mikken traag, en dat is precies het soort
	// oordeel waar de owner voor moet spelen.
	if (GAverageMS > 33.3f && GAverageMS > 0.0f)
	{
		UE_LOG(LogEclipse, Error,
			TEXT("[PLAYSHOT %d FOUT] %.1f ms per frame — onder de 30 fps, dat voelt de speler"),
			ShotIndex, GAverageMS);
	}

	// KOMT BEWEGING IN HET BEELD AAN? Moment 2 en 3 zijn de loopmomenten, en
	// tussen die twee hoort het uitzicht verschoven te zijn. Zonder deze controle
	// zou een speler die wel invoer krijgt maar niet beweegt (of een camera die
	// losgekoppeld is) negen keurige opnames opleveren van hetzelfde plaatje.
	//
	// Op de camera en niet op de pawn: het gaat er hier om of de beweging het
	// SCHERM haalt, niet of er een getal in het bewegingscomponent verandert.
	if (ShotIndex == 3)
	{
		const float Moved = FVector::Dist(CameraLocation, PlayShotLastCamera);
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 3 BEWEGING] uitzicht %.0f cm verschoven sinds moment 2"), Moved);
		if (Moved < 50.0f)
		{
			UE_LOG(LogEclipse, Error,
				TEXT("[PLAYSHOT 3 FOUT] het uitzicht schoof maar %.0f cm op tussen twee loopmomenten — beweging bereikt het beeld niet"),
				Moved);
		}
	}
	PlayShotLastCamera = CameraLocation;

	const APawn* PlayerPawn = Controller->GetPawn();
	int32 SquadDrawnNearby = 0;
	for (TActorIterator<AEclipseCharacter> It(GetWorld()); It; ++It)
	{
		const AEclipseCharacter* Body = *It;
		const USkeletalMeshComponent* Mesh = Body != nullptr ? Body->GetMesh() : nullptr;
		if (Mesh == nullptr)
		{
			continue;
		}

		FVector2D Screen = FVector2D::ZeroVector;
		const bool bOnScreen = Controller->ProjectWorldLocationToScreen(Body->GetActorLocation(), Screen);
		const FBoxSphereBounds Bounds = Mesh->Bounds;
		const FVector MeshScale = Mesh->GetComponentScale();

		// GETEKEND is het enige woord dat telt. Bounds, schaal en schermpositie
		// kunnen alle drie kloppen terwijl de renderer het lichaam overslaat —
		// dat is precies het gat waar de owner op wees: "je tests bewijzen dat de
		// code doet wat de code zegt, niet dat het resultaat zichtbaar wordt".
		// WasRecentlyRendered() vraagt het aan de renderer zelf.
		// MEEGEKOMEN, EN TE ZIEN. De suite bewijst dat de squad volgt: de verste
		// soldaat gaat van 2326 naar 779 cm. Dat is wereldruimte, en precies het
		// soort bewijs waarvan vandaag bleek dat het niets zegt over het scherm.
		// Een squad die volgt maar nooit in beeld komt, is voor de speler geen
		// squad. Alleen lichamen die de renderer ECHT getekend heeft tellen mee.
		if (Body != PlayerPawn && Mesh->WasRecentlyRendered(0.2f)
			&& FVector::Dist(CameraLocation, Body->GetActorLocation()) < 1500.0f)
		{
			++SquadDrawnNearby;
		}

		if (Body == PlayerPawn)
		{
			// Wapenstatus erbij. Op het eerste beeld met UI stond er geen
			// munitieteller en hield niemand een wapen vast, en er vielen nul
			// schoten — drie waarnemingen die één oorzaak kunnen hebben. Dit
			// zegt welke.
			const UEclipseHitscanWeaponComponent* Weapon = Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
			UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT %d WAPEN] component=%d magazijn=%d munitie=%d"),
				ShotIndex,
				Weapon != nullptr ? 1 : 0,
				Weapon != nullptr ? Weapon->GetMagazineSize() : -1,
				Weapon != nullptr ? Weapon->GetAmmoInMagazine() : -1);

			// HARDE UITSPRAKEN OVER WAT ER IN HET FRAME STAAT.
			//
			// De opnameronde leverde tot nu alleen beelden, en een beeld beoordeelt
			// zichzelf niet. Deze drie controles zijn precies de dingen die vandaag
			// misgingen en die geen enkele logica-test kon zien — ze meten in
			// SCHERMRUIMTE en niet in wereldruimte, want dat is het verschil tussen
			// "de code klopt" en "je ziet iets".
			//
			// De ronde stopt er niet voor: hij logt FOUT-regels en draait door, want
			// een halve ronde levert minder bewijs op dan een volledige met een fout
			// erin. verify.ps1 valt op die regels.
			FVector2D FeetScreen = FVector2D::ZeroVector;
			FVector2D HeadScreen = FVector2D::ZeroVector;
			const FVector Feet = Body->GetActorLocation() - FVector(0, 0, Bounds.BoxExtent.Z);
			const FVector Head = Body->GetActorLocation() + FVector(0, 0, Bounds.BoxExtent.Z);
			const bool bFeet = Controller->ProjectWorldLocationToScreen(Feet, FeetScreen);
			const bool bHead = Controller->ProjectWorldLocationToScreen(Head, HeadScreen);
			const float SilhouettePixels = (bFeet && bHead) ? FMath::Abs(HeadScreen.Y - FeetScreen.Y) : 0.0f;

			UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT %d SILHOUET] %.0f px hoog in een frame van %d px"),
				ShotIndex, SilhouettePixels, ViewportY);

			// 1. Wordt hij getekend? Dit is de vraag waar het gisteren op stukliep:
			//    bounds konden kloppen terwijl de renderer hem oversloeg.
			if (!Mesh->WasRecentlyRendered(0.2f))
			{
				UE_LOG(LogEclipse, Error, TEXT("[PLAYSHOT %d FOUT] de speler wordt NIET getekend"), ShotIndex);
			}

			// 2. Neemt hij een echt stuk beeld in? Een ingeklapt personage haalt
			//    dit niet: bij de additieve idle-take was de omvang 1,0 cm, en dat
			//    is in schermruimte een handvol pixels. Een tiende van de
			//    beeldhoogte is ruim onder wat een lichaam op 312 cm afstand hoort
			//    te vullen, dus dit slaat geen normale situatie af.
			const float MinimumPixels = static_cast<float>(ViewportY) * 0.1f;
			if (SilhouettePixels < MinimumPixels)
			{
				UE_LOG(LogEclipse, Error,
					TEXT("[PLAYSHOT %d FOUT] de speler is maar %.0f px hoog, minder dan %.0f — ingeklapt of buiten beeld?"),
					ShotIndex, SilhouettePixels, MinimumPixels);
			}

			// 3. Staat hij in beeld? Een lichaam dat correct getekend wordt maar
			//    buiten het frame valt, bewijst niets over wat de speler ziet.
			if (!bOnScreen || Screen.X < 0.0f || Screen.X > ViewportX || Screen.Y < 0.0f || Screen.Y > ViewportY)
			{
				UE_LOG(LogEclipse, Error,
					TEXT("[PLAYSHOT %d FOUT] de speler staat op (%.0f,%.0f) en dat valt buiten het frame"),
					ShotIndex, Screen.X, Screen.Y);
			}

			const float BodyYaw = static_cast<float>(Body->GetActorRotation().Yaw);
			const float ViewYaw = static_cast<float>(Controller->GetControlRotation().Yaw);
			UE_LOG(LogEclipse, Display,
				TEXT("[PLAYSHOT %d DRAAI] lichaam=%.1f gr  camera=%.1f gr  verschil=%.1f gr"),
				ShotIndex, BodyYaw, ViewYaw,
				FMath::Abs(FMath::FindDeltaAngleDegrees(BodyYaw, ViewYaw)));
		}

		UE_LOG(LogEclipse, Display,
			TEXT("[PLAYSHOT %d MEET] %s%s  schaal=%.3f  hoogte=%.1fcm  afstand=%.0fcm  scherm=%s(%.0f,%.0f)  zichtbaar=%d verborgen=%d eigenaarzietniet=%d GETEKEND=%d"),
			ShotIndex,
			*Body->GetName(),
			Body == PlayerPawn ? TEXT(" <-SPELER") : TEXT(""),
			MeshScale.Z,
			Bounds.BoxExtent.Z * 2.0f,
			FVector::Dist(CameraLocation, Body->GetActorLocation()),
			bOnScreen ? TEXT("") : TEXT("BUITEN "),
			Screen.X, Screen.Y,
			Mesh->IsVisible() ? 1 : 0,
			Mesh->bHiddenInGame ? 1 : 0,
			Mesh->bOwnerNoSee ? 1 : 0,
			Mesh->WasRecentlyRendered(0.2f) ? 1 : 0);
	}

	ReportSquadInFrame(ShotIndex, SquadDrawnNearby);
}

void AEclipseGameMode::ReportSquadInFrame(int32 ShotIndex, int32 DrawnNearby)
{
	UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT %d SQUAD] %d lichamen getekend binnen 15 m"),
		ShotIndex, DrawnNearby);

	// Alleen op moment 4 een harde eis: dat is NA het lopen, dus daar moet blijken
	// dat ze zijn meegekomen. Op moment 1 staan ze er sowieso (net gespawnd) en
	// vanaf moment 5 zijn ze bewust verborgen voor de isolatie-opname — een eis
	// daar zou rood worden op iets wat ik zelf doe.
	if (ShotIndex == 4 && DrawnNearby == 0)
	{
		UE_LOG(LogEclipse, Error,
			TEXT("[PLAYSHOT 4 FOUT] na het lopen staat er geen enkel ander lichaam in beeld — de squad is niet meegekomen"));
	}
}

void AEclipseGameMode::MeasureDressingFigures(int32 ShotIndex)
{
	APlayerController* Controller = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (Controller == nullptr)
	{
		return;
	}

	// De aankleedfiguren zijn GEEN AEclipseCharacter, dus de meting hierboven ziet
	// ze niet. Op het eerste echte spelbeeld bleken ze de speler te overtreffen —
	// een fout die geen enkele meting op de speler ooit had kunnen vinden, omdat
	// hij pas bestaat als je twee lichamen NAAST elkaar ziet.
	FVector CameraLocation;
	FRotator CameraRotation;
	Controller->GetPlayerViewPoint(CameraLocation, CameraRotation);

	for (TActorIterator<ASkeletalMeshActor> It(GetWorld()); It; ++It)
	{
		const ASkeletalMeshActor* Figure = *It;
		const USkeletalMeshComponent* Mesh = Figure != nullptr ? Figure->GetSkeletalMeshComponent() : nullptr;
		if (Mesh == nullptr || Mesh->GetSkeletalMeshAsset() == nullptr)
		{
			continue;
		}
		const float Distance = FVector::Dist(CameraLocation, Figure->GetActorLocation());
		if (Distance > 3000.0f)
		{
			continue;
		}
		UE_LOG(LogEclipse, Display,
			TEXT("[PLAYSHOT %d AANKLEDING] %s  hoogte=%.1fcm  schaal=%.2f  afstand=%.0fcm  GETEKEND=%d"),
			ShotIndex,
			*Mesh->GetSkeletalMeshAsset()->GetName(),
			Mesh->Bounds.BoxExtent.Z * 2.0f,
			Mesh->GetComponentScale().Z,
			Distance,
			Mesh->WasRecentlyRendered(0.2f) ? 1 : 0);
	}
}

void AEclipseGameMode::AdvancePlayShotRound()
{
	APlayerController* Controller = GetWorld() != nullptr ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (Controller == nullptr)
	{
		return;
	}
	// Tot 8: stap 9 valt op een eigen timer midden in de herlaadbeurt en meet
	// daar zelf, anders staat elke regel er twee keer.
	if (PlayShotStep >= 1 && PlayShotStep <= 8)
	{
		MeasurePlayShot(PlayShotStep);
		MeasureDressingFigures(PlayShotStep);
	}

	// Elke stap: eerst de TOESTAND zetten, dan één stap later de opname. Zo staat
	// het personage al in de houding die beoordeeld moet worden.
	switch (PlayShotStep)
	{
	case 0:
		bPlayShotWalking = false;
		bPlayShotFiring = false;
		break;
	case 1:
		Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 1] stilstaand, net gespawnd"));
		bPlayShotWalking = true;
		break;
	case 2:
		Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 2] tijdens lopen"));
		bPlayShotFiring = true;
		break;
	case 3:
		Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 3] lopend en vurend"));
		bPlayShotWalking = false;
		bPlayShotFiring = false;
		break;
	case 4:
		Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 4] weer stilstaand"));
		// Vijfde opname met ALLE ANDERE lichamen verborgen. Zonder dit blijft
		// "mijn personage staat er" een vermoeden: op een frame met vier figuren
		// kun je niet aanwijzen welke van jou is, en een meting die de actor
		// noemt zegt niets over de vorm die je ziet.
		//
		// Eerst geprobeerd als het omgekeerde — de speler weghalen en het
		// verschil zoeken. Dat werkte niet: de squad loopt door tussen twee
		// opnames, dus twee frames verschillen sowieso en het verschil bewijst
		// niets. Overhouden is ondubbelzinnig waar weglaten dat niet is.
		for (TActorIterator<AEclipseCharacter> It(GetWorld()); It; ++It)
		{
			AEclipseCharacter* Body = *It;
			if (Body != nullptr && Body != Controller->GetPawn() && Body->GetMesh() != nullptr)
			{
				Body->GetMesh()->SetVisibility(false, true);
			}
		}
		// De aankleedfiguren erbij: de camera zwaait er bij de draaiing vlak
		// langs, en dan vult een figuur van 40 cm afstand het frame terwijl je
		// juist naar het silhouet van de speler moet kijken.
		for (TActorIterator<ASkeletalMeshActor> It(GetWorld()); It; ++It)
		{
			ASkeletalMeshActor* Figure = *It;
			if (Figure != nullptr && Figure->GetSkeletalMeshComponent() != nullptr)
			{
				Figure->GetSkeletalMeshComponent()->SetVisibility(false, true);
			}
		}
		break;
	case 5:
		Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 5] alleen de SPELER zichtbaar — wat hier staat, is jouw personage"));
		// TURN-IN-PLACE OP BEELD. Gebouwd, drempel op 60 graden, gemeten — maar
		// nooit gezien. De andere lichamen blijven verborgen: bij een draaiing
		// gaat het om ÉÉN silhouet, en drie extra figuren maken het beeld alleen
		// moeilijker te lezen.
		//
		// 100 graden en niet 61: net over de drempel meet de drempel, en die is
		// al gedekt door een test. Hier moet je ZIEN dat het lichaam de camera
		// nadraait, en daarvoor moet de draai groot genoeg zijn om op twee
		// frames uit elkaar te houden.
		bPlayShotTurning = true;
		break;
	case 6:
		Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 6] tijdens het wegkijken — draait het lichaam mee?"));
		break;
	case 7:
		bPlayShotTurning = false;
		Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 7] uitgedraaid — lichaam hoort weer met de camera mee te staan"));
		break;
	case 8:
		// DE HERLAADPOSE OP BEELD. Belica heeft er zelf geen; deze is geleend uit
		// SciFiCharacter via compatibele skeletten. Dat de take RESOLVET is
		// gemeten, maar dat is niet hetzelfde als dat hij er goed uitziet: dit is
		// een take van een ander rig, en een verkeerd landende bot valt alleen op
		// een beeld op.
		//
		// Vandaar de opname MIDDEN in de herlaadbeurt (0,8 s van de 2,2 s) via
		// een eenmalige timer, en niet op de volgende stap van deze ronde — die
		// valt 2,0 s later en dan is de beurt zo goed als voorbij.
		if (AEclipseCharacter* Reloader = Cast<AEclipseCharacter>(Controller->GetPawn()))
		{
			if (UEclipseHitscanWeaponComponent* Weapon = Reloader->FindComponentByClass<UEclipseHitscanWeaponComponent>())
			{
				if (Weapon->StartReload(TEXT("PlayShot")))
				{
					FTimerHandle MidReload;
					GetWorldTimerManager().SetTimer(MidReload, FTimerDelegate::CreateWeakLambda(this,
						[this, Controller]()
						{
							Controller->ConsoleCommand(TEXT("HighResShot 1280x720"));
							UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 9] MIDDEN in het herladen — geleende take uit SciFiCharacter"));
							MeasurePlayShot(9);
						}), 0.8f, /*bLoop*/ false);
				}
				else
				{
					UE_LOG(LogEclipse, Warning, TEXT("[PLAYSHOT 9] herladen START NIET — geen opname van de herlaadpose."));
				}
			}
		}
		// MET DE UI EROP. HighResShot tekent alleen de 3D-scene; de HUD is een
		// UMG-widget en valt er buiten. Op de eerste zes beelden stond dus geen
		// munitieteller, en ik had dat bijna als ontbrekende HUD gerapporteerd —
		// een bevinding die niet over de game ging maar over mijn meetmethode.
		//
		// FScreenshotRequest met bShowUI neemt de widgets wel mee, dus dit beeld
		// beantwoordt de vraag in plaats van hem open te laten.
		FScreenshotRequest::RequestScreenshot(TEXT("PlayShot_MetUI"), /*bShowUI*/ true, /*bAddFilenameSuffix*/ false);
		UE_LOG(LogEclipse, Display, TEXT("[PLAYSHOT 8] zelfde beeld MET de HUD erop"));
		// De liberation-dump meedraaien in de ronde. Niet omdat hij bij een
		// SCREENSHOT hoort, maar omdat dit de enige plek is waar een echte
		// campagne draait met een console eronder — en een debug-commando dat
		// nooit wordt aangeroepen, is precies zo betrouwbaar als een test die
		// nooit rood kan worden.
		// DE GIDS AAN, want zo start de owner het spel (SPEEL_ECLIPSE.bat zet
		// Eclipse.Guide.Overlay op 1). Mijn ronde draaide hem uit, en dan blijven
		// juist de controles op het gids-paneel ongeoefend — ik zou een toestand
		// bewaken waar hij nooit in zit. Dezelfde les als het draaien van zijn
		// eigen startregel: test de configuratie die de speler gebruikt.
		Controller->ConsoleCommand(TEXT("Eclipse.Guide.Overlay 1"));
		Controller->ConsoleCommand(TEXT("Eclipse.Liberation.Report"));
		// En de UI-dump, om dezelfde reden: dit is de enige plek waar een echte
		// HUD draait met een console eronder, en de UI-laag is niet te
		// fotograferen. Zonder deze regel zou het commando bestaan zonder ooit
		// gedraaid te hebben.
		Controller->ConsoleCommand(TEXT("Eclipse.UI.Report"));
		break;
	default:
		UE_LOG(LogEclipse, Display, TEXT("PlayShot: ronde klaar."));
		Controller->ConsoleCommand(TEXT("quit"));
		return;
	}
	++PlayShotStep;
}

void AEclipseGameMode::SetupShotRig()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("EclipseShot")))
	{
		return;
	}

	// Review stills judge the art, not this laptop's autodetected settings —
	// force full scalability so the skylight capture/volumetrics actually run.
	Scalability::FQualityLevels Quality;
	Quality.SetFromSingleQualityLevel(3);
	Scalability::SetQualityLevels(Quality);

	// First fire waits for exposure/streaming to settle; then a 2 s cadence
	// alternates teleport and capture so every shot gets a stabilized frame.
	GetWorldTimerManager().SetTimer(ShotRigTimer, this, &AEclipseGameMode::AdvanceShotRig, 2.0f, /*bLoop*/ true, /*FirstDelay*/ 8.0f);
	UE_LOG(LogEclipse, Display, TEXT("ShotRig: armed (Part 15.9 fixed review cameras)."));

	// ALLEEN DE ARMATUREN (-EclipseShotFixtures, 26-07 laat).
	//
	// De lichtreview liep vast op een vraag die geen ontwerpvraag is: WELKE van de
	// twintig ambervlakken op het plein zijn de dertien armaturen? Zolang dat niet
	// vaststaat, kan een oordeel als "ze zijn te fel" over de verkeerde objecten
	// gaan — dezelfde valkuil als het meetvak dat voor 95% een ander oppervlak
	// bleek.
	//
	// Dezelfde truc als de isolatie-opname in de speelronde: overhouden in plaats
	// van weglaten. Wat hier in beeld staat, ZIJN de armaturen; er valt niets meer
	// te interpreteren. Dit plaatst er geen enkele bij, dus het respecteert de
	// pauze die de owner zelf heeft gevraagd.
	if (FParse::Param(FCommandLine::Get(), TEXT("EclipseShotFixtures")))
	{
		int32 Hidden = 0;
		int32 Kept = 0;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor == nullptr || Actor->ActorHasTag(TEXT("EclipseFixture")))
			{
				Kept += Actor != nullptr ? 1 : 0;
				continue;
			}
			// De hemel en de mist blijven: zonder die twee beoordeel je een
			// armatuur tegen zwart, en dat is een andere vraag dan of hij het in
			// dit district doet.
			if (Actor->IsA<AExponentialHeightFog>() || Actor->IsA<ASkyAtmosphere>() || Actor->IsA<ADirectionalLight>())
			{
				continue;
			}
			if (Actor->GetRootComponent() != nullptr && !Actor->IsHidden())
			{
				Actor->SetActorHiddenInGame(true);
				++Hidden;
			}
		}
		// Het AANTAL is hier het bewijs, niet het beeld. Toen deze regel 226
		// armaturen meldde tegen 13 geplaatste, bleek de gedeelde tag in de
		// verkeerde spawner te staan — terwijl het frame er plausibel uitzag.
		UE_LOG(LogEclipse, Display,
			TEXT("ShotRig: ALLEEN ARMATUREN — %d armaturen zichtbaar, %d andere actoren verborgen."),
			Kept, Hidden);
		if (Kept != 13)
		{
			UE_LOG(LogEclipse, Error,
				TEXT("ShotRig: %d armaturen gevonden, maar de graybox plaatst er 13 — de tag zit op de verkeerde actoren."),
				Kept);
		}
	}

	// Body showcase (step-2 character pipeline QC): one body per DT_BodyDefs row
	// lined up in review-camera 1's view, dressed through the REAL ApplyBodyDef
	// path — the review stills prove the data, not a bespoke preview path.
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	const UDataTable* BodyDefs = Setup != nullptr ? Setup->BodyDefs.LoadSynchronous() : nullptr;
	if (BodyDefs != nullptr && BodyDefs->GetRowStruct() == FEclipseBodyDefRow::StaticStruct())
	{
		// Open plaza south of the compound: fully inside camera 1's frustum and
		// clear of the compound walls (first showcase round hid half the line).
		int32 BodyIndex = 0;
		for (const TPair<FName, uint8*>& Row : BodyDefs->GetRowMap())
		{
			AEclipseCharacter* Body = SpawnBodyNear(FVector(3300.0f + (BodyIndex % 5) * 260.0f, -3300.0f - (BodyIndex / 5) * 300.0f, 0.0f), Row.Key.ToString());
			if (Body != nullptr)
			{
				Body->SetActorRotation(FRotator(0.0f, 200.0f, 0.0f));
				Body->ApplyBodyDef(*reinterpret_cast<const FEclipseBodyDefRow*>(Row.Value));
				++BodyIndex;
			}
		}
		UE_LOG(LogEclipse, Display, TEXT("ShotRig: body showcase spawned (%d bodies)."), BodyIndex);
	}
}

void AEclipseGameMode::AdvanceShotRig()
{
	// Fixed review cameras (15.9): both compounds, cover field, high overview.
	// Index 0 repeats the first camera as a sacrificial warm-up — the first
	// capture of a session carries streaming/history artifacts; discard it.
	struct FShotDef { FVector Location; FRotator Rotation; };
	static const FShotDef Shots[] = {
		{ FVector(2600.0f, -2000.0f, 260.0f), FRotator(-4.0f, 0.0f, 0.0f) },
		{ FVector(2600.0f, -2000.0f, 260.0f), FRotator(-4.0f, 0.0f, 0.0f) },
		{ FVector(-1800.0f, 3000.0f, 260.0f), FRotator(-4.0f, 180.0f, 0.0f) },
		{ FVector(700.0f, -5200.0f, 220.0f), FRotator(-6.0f, 115.0f, 0.0f) },
		{ FVector(-8200.0f, -8200.0f, 1500.0f), FRotator(-24.0f, 45.0f, 0.0f) },
		// 15.8 frames: the gate portal + STOP/TOXIC signs looking back west
		// toward Entry_Main, and the crossing's cable-arc lamp pair +
		// radiation placard — the new dressing must be judged close-up, not
		// as overview specks. Crossing cam sits south of the artery so the
		// pole placard is face-on and the propaganda board's back stays out
		// of the sightline (first 15.8 round blocked the patrol figure).
		{ FVector(-8200.0f, -250.0f, 300.0f), FRotator(-4.0f, 185.0f, 0.0f) },
		{ FVector(-4700.0f, -1500.0f, 280.0f), FRotator(-4.0f, 75.0f, 0.0f) },
	};

	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	APawn* Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
	if (Pawn == nullptr)
	{
		return;
	}

	const int32 ShotIndex = ShotRigStep / 2;
	if (ShotIndex >= static_cast<int32>(UE_ARRAY_COUNT(Shots)))
	{
		Controller->ConsoleCommand(TEXT("quit"));
		return;
	}

	if ((ShotRigStep % 2) == 0)
	{
		Pawn->SetActorLocation(Shots[ShotIndex].Location);
		Controller->SetControlRotation(Shots[ShotIndex].Rotation);
		// The overview camera sits 15 m up; a walking character falls the whole
		// 2 s stabilization window and the capture frames bare floor instead of
		// the district (first strong-PC round, camera 4). Flying + zeroed
		// velocity pins the pawn to the review position for every shot.
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			Character->GetCharacterMovement()->StopMovementImmediately();
		}
	}
	else
	{
		// ConsoleCommand routes through the local player -> game viewport exec
		// chain; a bare GEngine->Exec never reaches the screenshot handler.
		Controller->ConsoleCommand(TEXT("HighResShot 1920x1080"));
	}
	++ShotRigStep;
}
#endif

void AEclipseGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Unsubscribe(MissionEventsHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AEclipseGameMode::OnMissionLifecycle(FGameplayTag EventTag, const FInstancedStruct& /*Payload*/)
{
	if (EventTag == EclipseTags::Event_Mission_Started)
	{
		SpawnMissionActors();
	}
	else if (EventTag == EclipseTags::Event_Mission_Completed || EventTag == EclipseTags::Event_Mission_Failed)
	{
		DespawnMissionActors();
	}
}

void AEclipseGameMode::DespawnMissionActors()
{
	ObjectiveHostiles.Reset();
	ObjectiveHostileSiteId = NAME_None;

	if (UEclipseSquadSubsystem* Squad = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UEclipseSquadSubsystem>() : nullptr)
	{
		Squad->UnregisterAll();
	}
	for (AActor* Actor : SpawnedMissionActors)
	{
		if (Actor != nullptr)
		{
			Actor->Destroy();
		}
	}
	SpawnedMissionActors.Reset();
}

void AEclipseGameMode::HandlePlayerDowned(AEclipseCharacter* /*Player*/, FName /*Cause*/)
{
	UEclipseMissionSubsystem* Mission = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseMissionSubsystem>() : nullptr;
	if (Mission == nullptr)
	{
		return;
	}
	const EEclipseMissionPhase P = Mission->GetPhase();
	if (P == EEclipseMissionPhase::Objectives || P == EEclipseMissionPhase::Extraction)
	{
		// Player down ends the run as a failure — fail-forward commits at debrief (GDD 11.4).
		//
		// De UITKOMST wordt gelezen en niet weggegooid (26-07). Dit is het pad dat
		// draait op het moment dat de speler doodgaat, en juist daar mag niets stil
		// mislukken: faalt de debrief, dan committeert er niets — geen gewonden,
		// geen dagklok, geen regiostand — en blijft de missie in een halve toestand
		// hangen terwijl het scherm zegt dat je verloren hebt.
		//
		// Gevonden door te zoeken naar aanroepen die succes teruggeven waar de
		// aanroeper niets mee doet. De twee andere treffers (Weapon->Fire) negeren
		// hun uitkomst terecht: missen is geen fout.
		FString Error;
		if (!Mission->ResolveDebrief(false, Error))
		{
			UE_LOG(LogEclipse, Error,
				TEXT("GameMode: de speler ging neer maar de debrief mislukte (%s) — er is NIETS gecommitteerd (14.3.5)."),
				*Error);
		}
	}
}

void AEclipseGameMode::HandleHostileDowned(AEclipseCharacter* /*Hostile*/, FName /*Cause*/)
{
	if (ObjectiveHostileSiteId.IsNone())
	{
		return;
	}
	// Alle vijanden van de set moeten neer zijn. Een vernietigde (invalid) actor
	// telt als neer: hij is er niet meer, en de speler kan hem niet alsnog raken.
	for (const TWeakObjectPtr<AEclipseCharacter>& Hostile : ObjectiveHostiles)
	{
		const AEclipseCharacter* Live = Hostile.Get();
		if (Live != nullptr && !Live->IsDowned())
		{
			return;
		}
	}

	UEclipseMissionSubsystem* Mission = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseMissionSubsystem>() : nullptr;
	if (Mission == nullptr)
	{
		return;
	}
	UE_LOG(LogEclipse, Display, TEXT("GameMode: het doelwit bij '%s' ligt neer — DestroyTarget vervuld."),
		*ObjectiveHostileSiteId.ToString());
	Mission->CompleteObjectiveByTarget(ObjectiveHostileSiteId);
	// Eén keer: verdere downs mogen niet opnieuw voltooien (CompleteObjective is
	// idempotent, maar een tweede logregel zou liegen over wat er gebeurde).
	ObjectiveHostileSiteId = NAME_None;
}

FVector AEclipseGameMode::FindSiteLocation(FName SiteId, const FVector& Fallback) const
{
	// Sites carry actor tags (runtime-safe; labels are editor-only data).
	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(SiteId))
		{
			return It->GetActorLocation();
		}
	}
	UE_LOG(LogEclipse, Warning, TEXT("GameMode: site '%s' not found in level — using fallback location (GDD 14.3.5)."), *SiteId.ToString());
	return Fallback;
}

AEclipseCharacter* AEclipseGameMode::SpawnBodyNear(const FVector& Location, const FString& Label)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AEclipseCharacter* Body = GetWorld()->SpawnActor<AEclipseCharacter>(
		AEclipseCharacter::StaticClass(), Location + FVector(0, 0, 100.0f), FRotator::ZeroRotator, Params);
	if (Body != nullptr)
	{
#if WITH_EDITOR
		Body->SetActorLabel(Label);
#endif
		EnsureWeapon(*Body);
	}
	return Body;
}


void AEclipseGameMode::SpawnMissionActors()
{
	UGameInstance* GameInstance = GetGameInstance();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseSquadSubsystem* Squad = GetWorld()->GetSubsystem<UEclipseSquadSubsystem>();
	const UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();

	const EEclipseMissionPhase Phase = Mission != nullptr ? Mission->GetPhase() : EEclipseMissionPhase::None;
	if (Mission == nullptr || Squad == nullptr || Campaign == nullptr
		|| (Phase != EEclipseMissionPhase::Insertion && Phase != EEclipseMissionPhase::Objectives))
	{
		// No active mission = free-roam graybox (feel-target tuning sessions).
		return;
	}

	// Balanced with DespawnMissionActors; a re-entry rebuilds cleanly.
	if (!SpawnedMissionActors.IsEmpty())
	{
		DespawnMissionActors();
	}

	APawn* PlayerPawn = GetWorld()->GetFirstPlayerController() != nullptr ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;

	// De squad spawnt rond het INSERTIEPUNT, niet rond waar de pawn toevallig
	// staat. Dat verschil is 93 meter, en het was de oorzaak van "mijn squad doet
	// niets": zowel deze game mode als de player controller luistert naar
	// Event.Mission.Started, en de controller is degene die de speler naar
	// Entry_Main verplaatst. Wie het eerst aan de beurt is, bepaalt dus waar de
	// squad landt — en in de gemeten praktijk stond hij nog op zijn oude plek,
	// dus spawnde de squad daar en niet bij de speler.
	//
	// Gemeten met de speelronde (2026-07-26): verste squadmate 9282 cm van de
	// speler. Elke MoveTo-order leverde daardoor een gedeeltelijk pad op, en een
	// gedeeltelijk pad telt in de beslistabel als GEEN pad — dus weigerde de squad
	// alles met "no route". Drie symptomen, één oorzaak.
	//
	// Het insertiepunt zelf uitlezen haalt de volgorde-afhankelijkheid weg: dan
	// maakt het niet meer uit wie van de twee luisteraars het eerst is.
	FVector PlayerLocation = PlayerPawn != nullptr ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Entry_Main")))
		{
			PlayerLocation = It->GetActorLocation();
			break;
		}
	}

	// Ground data resolved from the campaign setup (GDD 14.2: the numbers live in
	// DA_CharacterTuning / DT_Weapons / DT_EnemyArchetypes, not in code defaults).
	const UEclipseCampaignSetupAsset* Setup = Campaign->GetActiveSetup();
	const UEclipseCharacterTuningAsset* CharacterTuning = Setup != nullptr ? Setup->CharacterTuning.LoadSynchronous() : nullptr;
	const FEclipseWeaponRow* PlayerWeaponRow = Setup != nullptr ? FirstRowOf<FEclipseWeaponRow>(Setup->Weapons.LoadSynchronous()) : nullptr;
	const UDataTable* ArchetypeTable = Setup != nullptr ? Setup->EnemyArchetypes.LoadSynchronous() : nullptr;
	const FEclipseEnemyArchetypeRow* ArchetypeRow = FirstRowOf<FEclipseEnemyArchetypeRow>(ArchetypeTable);
	if (CharacterTuning == nullptr || PlayerWeaponRow == nullptr || ArchetypeRow == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("GameMode: ground data incomplete (tuning %s, weapons %s, archetypes %s) — struct defaults stand in (GDD 14.3.5)."),
			CharacterTuning != nullptr ? TEXT("ok") : TEXT("missing"),
			PlayerWeaponRow != nullptr ? TEXT("ok") : TEXT("missing"),
			ArchetypeRow != nullptr ? TEXT("ok") : TEXT("missing"));
	}
	const FEclipseWeaponRow DefaultWeaponRow;

	// Visual bodies (step-2 character pipeline): DT_BodyDefs rows dress the
	// spawns; a missing table or row = capsule bodies, never a crash (14.3.5).
	const UDataTable* BodyDefs = Setup != nullptr ? Setup->BodyDefs.LoadSynchronous() : nullptr;
	auto FindBodyDef = [BodyDefs](FName RowName) -> const FEclipseBodyDefRow*
	{
		return (BodyDefs != nullptr && !RowName.IsNone() && BodyDefs->GetRowStruct() == FEclipseBodyDefRow::StaticStruct())
			? reinterpret_cast<const FEclipseBodyDefRow*>(BodyDefs->FindRowUnchecked(RowName))
			: nullptr;
	};
	// Rebel body pool: rows whose name starts with "Rebel" dress squadmates,
	// assigned stably by deploy order.
	TArray<const FEclipseBodyDefRow*> RebelBodies;
	if (BodyDefs != nullptr && BodyDefs->GetRowStruct() == FEclipseBodyDefRow::StaticStruct())
	{
		for (const TPair<FName, uint8*>& Row : BodyDefs->GetRowMap())
		{
			if (Row.Key.ToString().StartsWith(TEXT("Rebel")))
			{
				RebelBodies.Add(reinterpret_cast<const FEclipseBodyDefRow*>(Row.Value));
			}
		}
	}

	// The player pawn persists across runs: re-arm, re-tune and revive it so a
	// lost mission never launches the next one with a downed, unarmed body.
	if (AEclipseCharacter* PlayerBody = Cast<AEclipseCharacter>(PlayerPawn))
	{
		PlayerBody->ApplyTuning(CharacterTuning);
		PlayerBody->ReviveForMission();
		if (const FEclipseBodyDefRow* PlayerBodyDef = FindBodyDef(TEXT("Player")))
		{
			PlayerBody->ApplyBodyDef(*PlayerBodyDef);
		}
		// DE GEKOZEN LOADOUT BEPAALT JE WAPENS (26-07 avond, punt 5). Tot vandaag
		// kreeg iedereen de eerste rij van DT_Weapons en waren drie van de vier
		// wapens voor niemand bereikbaar — terwijl de loadout-keuze wél bestond,
		// gevalideerd werd en verzonden. Dit is de schakel die ontbrak.
		//
		// Terugval op de eerste rij als de loadout geen wapens noemt: een missie
		// zonder wapen is erger dan een missie met het verkeerde.
		UEclipseHitscanWeaponComponent& PlayerWeapon = EnsureWeapon(*PlayerBody);
		const FEclipseWeaponRow* Primary = nullptr;
		const FEclipseWeaponRow* Sidearm = nullptr;
		FName PrimaryName;
		FName SidearmName;
		if (Mission != nullptr)
		{
			// Mission staat hierboven al opgezocht (regel ~461); nog een keer
			// vragen zou een tweede naam voor hetzelfde object opleveren.
			const FGameplayTag Chosen = Mission->GetActiveLoadoutTag();
			// De tabel hangt aan DA_PrepTuning en niet aan de setup: de prep-laag
			// is eigenaar van wat je kunt kiezen, en die eigenaar mag hier niet
			// omzeild worden.
			const UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
			const UDataTable* LoadoutTable = Prep != nullptr ? Prep->GetLoadoutTable() : nullptr;
			const UDataTable* WeaponTable = Setup != nullptr ? Setup->Weapons.LoadSynchronous() : nullptr;
			if (Chosen.IsValid() && LoadoutTable != nullptr && WeaponTable != nullptr)
			{
				LoadoutTable->ForeachRow<FEclipseLoadoutOptionRow>(TEXT("EquipLoadout"),
					[&Primary, &Sidearm, &PrimaryName, &SidearmName, &Chosen, WeaponTable](const FName&, const FEclipseLoadoutOptionRow& Option)
					{
						if (Option.LoadoutTag != Chosen)
						{
							return;
						}
						if (!Option.PrimaryWeapon.IsNone())
						{
							Primary = WeaponTable->FindRow<FEclipseWeaponRow>(Option.PrimaryWeapon, TEXT("Primary"));
							PrimaryName = Option.PrimaryWeapon;
						}
						if (!Option.SidearmWeapon.IsNone())
						{
							Sidearm = WeaponTable->FindRow<FEclipseWeaponRow>(Option.SidearmWeapon, TEXT("Sidearm"));
							SidearmName = Option.SidearmWeapon;
						}
					});
			}
		}

		if (Primary == nullptr)
		{
			Primary = PlayerWeaponRow;
		}
		if (Primary != nullptr && Sidearm != nullptr)
		{
			PlayerWeapon.ApplyLoadout(*Primary, PrimaryName, *Sidearm, SidearmName);
		}
		else
		{
			PlayerWeapon.ApplyWeaponRow(Primary != nullptr ? *Primary : DefaultWeaponRow);
		}

		// Player body down ends the run (bind once; RemoveAll guards re-entry dupes).
		PlayerBody->OnDowned.RemoveAll(this);
		PlayerBody->OnDowned.AddUObject(this, &AEclipseGameMode::HandlePlayerDowned);
	}

	// Squad of 4 = player + 3 (SPEC-P2-01): the picked roster soldiers, spawned
	// in a fan beside the player, registered so orders and the downed pipeline
	// reach them. Class is data over the shared body (GDD 12.3): the kit picks
	// the weapon row and an optional body override; the classless fallback
	// carries the player-side platform (GDD 8.3 fairness: same guns, same rules).
	const UDataTable* WeaponsTable = Setup != nullptr ? Setup->Weapons.LoadSynchronous() : nullptr;
	const UDataTable* ClassDefs = Setup != nullptr ? Setup->ClassDefs.LoadSynchronous() : nullptr;
	if (ClassDefs != nullptr && ClassDefs->GetRowStruct() != FEclipseClassDefRow::StaticStruct())
	{
		UE_LOG(LogEclipse, Warning, TEXT("GameMode: ClassDefs table has the wrong row struct — squad deploys classless (GDD 14.3.5)."));
		ClassDefs = nullptr;
	}

	int32 SquadIndex = 0;
	for (const FGuid& SoldierId : Mission->GetDeployedSoldierIds())
	{
		const FEclipseSoldierRecord* Record = Campaign->GetState().FindSoldier(SoldierId);

		// Kit resolution is the pure core (SPEC-P2-01): a missing row or table
		// degrades to the classless kit, never a crash.
		const FEclipseClassDefRow* ClassRow = (ClassDefs != nullptr && Record != nullptr && !Record->ClassId.IsNone())
			? reinterpret_cast<const FEclipseClassDefRow*>(ClassDefs->FindRowUnchecked(Record->ClassId))
			: nullptr;
		const EclipseClassLogic::FEclipseResolvedClassKit Kit =
			EclipseClassLogic::ResolveClassKit(Record != nullptr ? *Record : FEclipseSoldierRecord(), ClassRow);
		if (Record != nullptr && !Record->ClassId.IsNone() && ClassRow == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("GameMode: class '%s' of %s has no DT_ClassDefs row — classless kit stands in (GDD 14.3.5)."),
				*Record->ClassId.ToString(), *Record->Name);
		}

		// Fan the squad out so four bodies never stack in one capsule scrum;
		// base/step live in DA_SquadTuning (P2-01 review m4).
		const UEclipseSquadTuningAsset* SquadTuning = Setup != nullptr ? Setup->SquadTuning.LoadSynchronous() : nullptr;
		const float FanBaseCm = SquadTuning != nullptr ? SquadTuning->SpawnFanBaseCm : 150.0f;
		const float FanStepCm = SquadTuning != nullptr ? SquadTuning->SpawnFanStepCm : 130.0f;
		const FVector SpawnOffset(FanBaseCm + FanStepCm * (SquadIndex % 2), FanBaseCm + FanStepCm * (SquadIndex / 2), 0.0f);
		AEclipseCharacter* Body = SpawnBodyNear(PlayerLocation + SpawnOffset,
			Record != nullptr ? Record->Name : TEXT("Squadmate"));
		if (Body == nullptr)
		{
			continue;
		}
		Body->ApplyTuning(CharacterTuning);

		// Visible kit: the class body override wins; otherwise the shared Rebel
		// pool dresses the soldier (step-2 pipeline), assigned stably by order.
		const FEclipseBodyDefRow* ClassBody = FindBodyDef(Kit.BodyDefOverride);
		if (ClassBody != nullptr)
		{
			Body->ApplyBodyDef(*ClassBody);
		}
		else if (!RebelBodies.IsEmpty())
		{
			Body->ApplyBodyDef(*RebelBodies[SquadIndex % RebelBodies.Num()]);
		}
		++SquadIndex;

		// Class weapon row from data; the platform default (first row) is the
		// classless fallback — fairness stays intact either way (GDD 8.3).
		const FEclipseWeaponRow* KitWeaponRow = nullptr;
		if (WeaponsTable != nullptr && !Kit.WeaponRow.IsNone() && WeaponsTable->GetRowStruct() == FEclipseWeaponRow::StaticStruct())
		{
			KitWeaponRow = reinterpret_cast<const FEclipseWeaponRow*>(WeaponsTable->FindRowUnchecked(Kit.WeaponRow));
			if (KitWeaponRow == nullptr)
			{
				UE_LOG(LogEclipse, Warning, TEXT("GameMode: weapon row '%s' (class '%s') missing — platform default stands in (GDD 14.3.5)."),
					*Kit.WeaponRow.ToString(), *Kit.ClassId.ToString());
			}
		}
		const FEclipseWeaponRow& SquadWeaponRow = KitWeaponRow != nullptr ? *KitWeaponRow
			: (PlayerWeaponRow != nullptr ? *PlayerWeaponRow : DefaultWeaponRow);
		EnsureWeapon(*Body).ApplyWeaponRow(SquadWeaponRow);

		AEclipseSquadmateController* Controller = GetWorld()->SpawnActor<AEclipseSquadmateController>();
		if (Controller != nullptr)
		{
			Controller->ApplyClassKit(Kit); // before registration: the downed wiring reads the kit
			Controller->Possess(Body);
			Squad->RegisterSquadmate(Controller, SoldierId);

			// MEELOPEN AANZETTEN (26-07 avond, punt 1 — laag 1). FollowDistance
			// stond in DA_SquadTuning met een comment die zei dat hij niet gelezen
			// werd; dit is de regel die dat waar maakt. Niet als optie: de owner
			// noemde meelopen expliciet basisgedrag, geen feature.
			Controller->BeginFollowing(
				SquadTuning != nullptr ? SquadTuning->FollowDistance : 400.0f,
				SquadTuning != nullptr ? SquadTuning->CoverSearchRadius : 800.0f);
			SpawnedMissionActors.Add(Controller);
		}
		SpawnedMissionActors.Add(Body);
	}

	// Vijandplaatsing leest sinds 26-07 de spawnsets van het missiesjabloon
	// (owner-beslissing). Daarvoor stond hier een vaste lus van vier die de
	// archetype-rijen afwisselde, en die negeerde stil wat drie van de vier
	// missies in EnemySpawns hadden staan: welk archetype, hoeveel, op welk site.
	// Wie "6 Enforcers op SiteB" authordde kreeg vier gemengde vijanden bij het
	// hoofddoel en geen enkel signaal dat zijn data was weggegooid.
	int32 EnemyIndex = 0;
	const FVector PrimarySite = FindSiteLocation(TEXT("Site_ControlPost"), PlayerLocation + FVector(3000.0f, 0.0f, 0.0f));

	// Welk site vraagt om een DESTROYED doelwit? De vijanden die daar spawnen zijn
	// dat doelwit, en zodra de laatste neerligt is het objective vervuld. Zonder
	// deze koppeling had DestroyTarget geen enkel voltooiingspad en vinkte de
	// overlap-trigger hem af op aanwezigheid (gevonden door de speelronde).
	ObjectiveHostiles.Reset();
	ObjectiveHostileSiteId = NAME_None;
	for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
	{
		if (Objective.Type == EEclipseObjectiveType::DestroyTarget)
		{
			ObjectiveHostileSiteId = Objective.TargetId;
			break;
		}
	}

	TArray<TPair<FName, const FEclipseEnemyArchetypeRow*>> ArchetypeRows;
	if (ArchetypeTable != nullptr && ArchetypeTable->GetRowStruct() == FEclipseEnemyArchetypeRow::StaticStruct())
	{
		for (const TPair<FName, uint8*>& Row : ArchetypeTable->GetRowMap())
		{
			ArchetypeRows.Emplace(Row.Key, reinterpret_cast<const FEclipseEnemyArchetypeRow*>(Row.Value));
		}
	}

	// De plaatsingslijst: één ingang per vijand, opgebouwd uit de geauthorde
	// batches. Eerst de lijst bouwen en dan pas spawnen, zodat de spawnlus zelf
	// niets hoeft te weten van waar zijn rij vandaan komt.
	struct FPlacement
	{
		const FEclipseEnemyArchetypeRow* Row = nullptr;
		FName RowName;
		FVector Location = FVector::ZeroVector;
		/** Het site waar deze batch om vroeg; None = de graybox-terugval. */
		FName SiteId;
	};
	TArray<FPlacement> Placements;

	auto ArchetypeByName = [&ArchetypeRows](FName Wanted) -> const FEclipseEnemyArchetypeRow*
	{
		for (const TPair<FName, const FEclipseEnemyArchetypeRow*>& Pair : ArchetypeRows)
		{
			if (Pair.Key == Wanted)
			{
				return Pair.Value;
			}
		}
		return nullptr;
	};

	const TArray<FEclipseEnemySpawnSet>& AuthoredSpawns = Mission->GetActiveEnemySpawns();
	for (const FEclipseEnemySpawnSet& Set : AuthoredSpawns)
	{
		// Onbekende archetype-id: luid melden en de batch overslaan, niet stil een
		// andere vijand neerzetten (14.3.5). Een missie die vraagt om Enforcers en
		// er Snipers krijgt is erger dan een missie die er geen krijgt, want het
		// eerste ziet er goed uit.
		const FEclipseEnemyArchetypeRow* Row = ArchetypeByName(Set.ArchetypeId);
		if (Row == nullptr)
		{
			UE_LOG(LogEclipse, Warning,
				TEXT("EnemySpawns: archetype '%s' staat niet in DT_EnemyArchetypes — batch van %d overgeslagen."),
				*Set.ArchetypeId.ToString(), Set.Count);
			continue;
		}

		// Site onbekend? FindSiteLocation valt terug op de meegegeven positie, dus
		// dat is al fail-forward — maar het hoort wel gezegd te worden.
		const bool bHasSite = !Set.SpawnSiteId.IsNone();
		const FVector Anchor = bHasSite
			? FindSiteLocation(Set.SpawnSiteId, PrimarySite)
			: PrimarySite;
		if (bHasSite && Anchor.Equals(PrimarySite))
		{
			UE_LOG(LogEclipse, Warning,
				TEXT("EnemySpawns: site '%s' niet gevonden — de %d %s staan bij het primaire doel."),
				*Set.SpawnSiteId.ToString(), Set.Count, *Set.ArchetypeId.ToString());
		}

		for (int32 InSet = 0; InSet < Set.Count; ++InSet)
		{
			Placements.Add({ Row, Set.ArchetypeId,
				Anchor + FVector(300.0f * InSet, 200.0f * (InSet % 2), 0.0f), Set.SpawnSiteId });
		}
	}

	if (Placements.IsEmpty())
	{
		// Geen geauthorde batches (M1.1 heeft ze niet) = de graybox-standaard van
		// vier bij het hoofddoel. Bewust behouden: een missie zonder spawnset moet
		// speelbaar blijven, en dit is de opstelling waarop alle speelrondes van
		// vannacht gemeten zijn.
		if (!AuthoredSpawns.IsEmpty())
		{
			UE_LOG(LogEclipse, Warning,
				TEXT("EnemySpawns: %d batch(es) geauthord maar geen enkele bruikbaar — terug op de standaard van vier."),
				AuthoredSpawns.Num());
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FEclipseEnemyArchetypeRow* RowForEnemy = !ArchetypeRows.IsEmpty()
				? ArchetypeRows[Index % ArchetypeRows.Num()].Value : ArchetypeRow;
			const FName RowName = !ArchetypeRows.IsEmpty()
				? ArchetypeRows[Index % ArchetypeRows.Num()].Key : FName(TEXT("Enforcer"));
			Placements.Add({ RowForEnemy, RowName,
				PrimarySite + FVector(300.0f * Index, 200.0f * (Index % 2), 0.0f), NAME_None });
		}
	}
	else
	{
		UE_LOG(LogEclipse, Display, TEXT("EnemySpawns: %d vijanden uit %d geauthorde batch(es)."),
			Placements.Num(), AuthoredSpawns.Num());
	}

	// Welke plaatsing wordt het DestroyTarget-doelwit? De dichtstbijzijnde bij het
	// site dat het objective noemt. Vooraf bepaald zodat de spawnlus hieronder
	// alleen nog hoeft te vergelijken, en zodat "geen enkele vijand" zichtbaar is
	// vóór er iets gespawnd is.
	int32 ObjectiveHostileIndex = INDEX_NONE;
	if (!ObjectiveHostileSiteId.IsNone() && !Placements.IsEmpty())
	{
		const FVector ObjectiveLocation = FindSiteLocation(ObjectiveHostileSiteId, PrimarySite);
		double Best = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < Placements.Num(); ++Index)
		{
			const double Distance = FVector::Dist(Placements[Index].Location, ObjectiveLocation);
			if (Distance < Best)
			{
				Best = Distance;
				ObjectiveHostileIndex = Index;
			}
		}
		UE_LOG(LogEclipse, Display,
			TEXT("GameMode: doelwit van '%s' wordt de vijand op %.0f cm van dat site (van %d geplaatst)."),
			*ObjectiveHostileSiteId.ToString(), Best, Placements.Num());
	}

	for (int32 Index = 0; Index < Placements.Num(); ++Index)
	{
		const FEclipseEnemyArchetypeRow* RowForEnemy = Placements[Index].Row;
		const FName RowName = Placements[Index].RowName;

		AEclipseCharacter* Enemy = SpawnBodyNear(Placements[Index].Location, FString::Printf(TEXT("%s_%d"), *RowName.ToString(), Index));
		if (Enemy == nullptr)
		{
			continue;
		}

		// Enemy ballistics derive from the archetype row (GDD 8.3 fairness: same
		// hitscan seam, data-tuned): damage/cadence from the row, reach = sight.
		FEclipseWeaponRow EnemyWeaponRow;
		if (RowForEnemy != nullptr)
		{
			EnemyWeaponRow.Damage = RowForEnemy->Damage;
			EnemyWeaponRow.FireInterval = RowForEnemy->FireInterval;
			EnemyWeaponRow.RangeCm = RowForEnemy->PerceptionRadius;
			EnemyWeaponRow.HeadshotMultiplier = 1.0f; // placeholder bodies: no headshot lottery until hit zones land

			Enemy->InitializeHealth(RowForEnemy->Health);
			if (const FEclipseBodyDefRow* EnemyBodyDef = FindBodyDef(RowForEnemy->BodyDef))
			{
				Enemy->ApplyBodyDef(*EnemyBodyDef);
			}
		}
		EnsureWeapon(*Enemy).ApplyWeaponRow(EnemyWeaponRow);
		AEclipseEnemyController* Controller = GetWorld()->SpawnActor<AEclipseEnemyController>();
		if (Controller != nullptr)
		{
			if (RowForEnemy != nullptr)
			{
				Controller->ApplyArchetype(*RowForEnemy);
			}
			Controller->Possess(Enemy);
			SpawnedMissionActors.Add(Controller);
			++EnemyIndex;
		}
		// Het doelwit is de vijand die het DICHTST bij het objective-site staat.
		//
		// Regressie die ik vanochtend zelf introduceerde en binnen het uur vond.
		// Tot de spawn-koppeling landde stonden alle vier de vijanden bij het
		// primaire doel, dus "de eerste" was per definitie ook "de juiste". Met
		// geauthorde batches klopt dat niet meer: de eerste plaatsing komt uit de
		// eerste batch, en die kan aan de andere kant van het district staan. Dat
		// raakt Assault (DestroyTarget op Site_ControlPost) en Sabotage (Site_Crane),
		// en het zou stil misgaan — een doelwit dat leeft, alleen niet waar de
		// marker staat.
		//
		// Op AFSTAND en niet op sitenaam. De eerste versie van deze reparatie eiste
		// een batch mét dezelfde sitenaam, en die aanname was fout: spawnpunten
		// heten Spawn_* en objectives Site_*, met opzet — de vijanden bewaken het
		// doel, ze staan er niet bovenop. Afstand is de vraag die er echt toe doet
		// en die overleeft elke hernoeming.
		if (!ObjectiveHostileSiteId.IsNone() && Index == ObjectiveHostileIndex)
		{
			// De EERSTE vijand van de set is het doelwit. M1.1 zegt "take out the
			// patrol leader", enkelvoud: de andere drie zijn de patrouille, en die
			// hoef je niet allemaal om te leggen om de hinderlaag te laten slagen.
			// Eerst stond hier "alle vier", en de speelronde liet meteen zien
			// waarom dat de verkeerde lezing is: één schutter achter dekking hield
			// het objective 94 seconden lang open terwijl de missie al klaar was.
			Enemy->OnDowned.AddUObject(this, &AEclipseGameMode::HandleHostileDowned);
			ObjectiveHostiles.Add(Enemy);
			UE_LOG(LogEclipse, Display, TEXT("GameMode: '%s' is het doelwit van '%s' (DestroyTarget)."),
				*Enemy->GetName(), *ObjectiveHostileSiteId.ToString());
		}
		SpawnedMissionActors.Add(Enemy);
	}

	UE_LOG(LogEclipse, Display, TEXT("GameMode: mission actors spawned (%d squadmates, %d enemies)."),
		Mission->GetDeployedSoldierIds().Num(), EnemyIndex);
}
