#include "Core/EclipseGameMode.h"

#include "AI/EclipseEnemyController.h"
#include "AI/EclipseSquadmateController.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipsePlayerController.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Core/EclipseGameplayTags.h"
#include "Core/EclipseGrayboxBuilder.h"
#include "Eclipse.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Scalability.h"
#include "TimerManager.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"

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
#endif
}

#if !UE_BUILD_SHIPPING
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

	// Body showcase (step-2 character pipeline QC): one body per DT_BodyDefs row
	// lined up in review-camera 1's view, dressed through the REAL ApplyBodyDef
	// path — the review stills prove the data, not a bespoke preview path.
	const UEclipseCampaignSubsystem* Campaign = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	const UDataTable* BodyDefs = Setup != nullptr ? Setup->BodyDefs.LoadSynchronous() : nullptr;
	if (BodyDefs != nullptr && BodyDefs->GetRowStruct() == FEclipseBodyDefRow::StaticStruct())
	{
		int32 BodyIndex = 0;
		for (const TPair<FName, uint8*>& Row : BodyDefs->GetRowMap())
		{
			AEclipseCharacter* Body = SpawnBodyNear(FVector(3600.0f + BodyIndex * 240.0f, -2350.0f, 0.0f), Row.Key.ToString());
			if (Body != nullptr)
			{
				Body->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
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
		FString Error;
		Mission->ResolveDebrief(false, Error);
	}
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
	const FVector PlayerLocation = PlayerPawn != nullptr ? PlayerPawn->GetActorLocation() : FVector::ZeroVector;

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
		EnsureWeapon(*PlayerBody).ApplyWeaponRow(PlayerWeaponRow != nullptr ? *PlayerWeaponRow : DefaultWeaponRow);

		// Player body down ends the run (bind once; RemoveAll guards re-entry dupes).
		PlayerBody->OnDowned.RemoveAll(this);
		PlayerBody->OnDowned.AddUObject(this, &AEclipseGameMode::HandlePlayerDowned);
	}

	// Squad of 2 (SPEC-P1-06): the picked roster soldiers, spawned beside the
	// player, registered so orders and the downed pipeline reach them. They carry
	// the player-side platform (GDD 8.3 fairness: same guns, same rules).
	int32 SquadIndex = 0;
	for (const FGuid& SoldierId : Mission->GetDeployedSoldierIds())
	{
		const FEclipseSoldierRecord* Record = Campaign->GetState().FindSoldier(SoldierId);
		AEclipseCharacter* Body = SpawnBodyNear(PlayerLocation + FVector(150.0f, 150.0f, 0.0f),
			Record != nullptr ? Record->Name : TEXT("Squadmate"));
		if (Body == nullptr)
		{
			continue;
		}
		Body->ApplyTuning(CharacterTuning);
		if (!RebelBodies.IsEmpty())
		{
			Body->ApplyBodyDef(*RebelBodies[SquadIndex % RebelBodies.Num()]);
		}
		++SquadIndex;
		EnsureWeapon(*Body).ApplyWeaponRow(PlayerWeaponRow != nullptr ? *PlayerWeaponRow : DefaultWeaponRow);

		AEclipseSquadmateController* Controller = GetWorld()->SpawnActor<AEclipseSquadmateController>();
		if (Controller != nullptr)
		{
			Controller->Possess(Body);
			Squad->RegisterSquadmate(Controller, SoldierId);
			SpawnedMissionActors.Add(Controller);
		}
		SpawnedMissionActors.Add(Body);
	}

	// PLACEHOLDER(SPEC-P1-05): enemy placement reads the mission asset's spawn
	// sets in the content pass; presence below cycles through ALL archetype rows
	// (step-2: per-archetype stats + bodies) so the mix is visible today.
	int32 EnemyIndex = 0;
	const FVector PrimarySite = FindSiteLocation(TEXT("Site_ControlPost"), PlayerLocation + FVector(3000.0f, 0.0f, 0.0f));

	TArray<TPair<FName, const FEclipseEnemyArchetypeRow*>> ArchetypeRows;
	if (ArchetypeTable != nullptr && ArchetypeTable->GetRowStruct() == FEclipseEnemyArchetypeRow::StaticStruct())
	{
		for (const TPair<FName, uint8*>& Row : ArchetypeTable->GetRowMap())
		{
			ArchetypeRows.Emplace(Row.Key, reinterpret_cast<const FEclipseEnemyArchetypeRow*>(Row.Value));
		}
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FEclipseEnemyArchetypeRow* RowForEnemy = !ArchetypeRows.IsEmpty()
			? ArchetypeRows[Index % ArchetypeRows.Num()].Value : ArchetypeRow;
		const FName RowName = !ArchetypeRows.IsEmpty()
			? ArchetypeRows[Index % ArchetypeRows.Num()].Key : FName(TEXT("Enforcer"));

		AEclipseCharacter* Enemy = SpawnBodyNear(PrimarySite + FVector(300.0f * Index, 200.0f * (Index % 2), 0.0f), FString::Printf(TEXT("%s_%d"), *RowName.ToString(), Index));
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
		SpawnedMissionActors.Add(Enemy);
	}

	UE_LOG(LogEclipse, Display, TEXT("GameMode: mission actors spawned (%d squadmates, %d enemies)."),
		Mission->GetDeployedSoldierIds().Num(), EnemyIndex);
}
