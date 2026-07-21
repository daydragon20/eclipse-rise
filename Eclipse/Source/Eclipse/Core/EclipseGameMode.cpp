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
#include "Engine/GameInstance.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "Strategy/EclipseCampaignSubsystem.h"

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
}

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
		Body->AddOwnedComponent(NewObject<UEclipseHitscanWeaponComponent>(Body));
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

	// Player body down ends the run (bind once; RemoveAll guards re-entry dupes).
	if (AEclipseCharacter* PlayerBody = Cast<AEclipseCharacter>(PlayerPawn))
	{
		PlayerBody->OnDowned.RemoveAll(this);
		PlayerBody->OnDowned.AddUObject(this, &AEclipseGameMode::HandlePlayerDowned);
	}

	// Squad of 2 (SPEC-P1-06): the picked roster soldiers, spawned beside the
	// player, registered so orders and the downed pipeline reach them.
	for (const FGuid& SoldierId : Mission->GetDeployedSoldierIds())
	{
		const FEclipseSoldierRecord* Record = Campaign->GetState().FindSoldier(SoldierId);
		AEclipseCharacter* Body = SpawnBodyNear(PlayerLocation + FVector(150.0f, 150.0f, 0.0f),
			Record != nullptr ? Record->Name : TEXT("Squadmate"));
		if (Body == nullptr)
		{
			continue;
		}

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
	// sets + DT_EnemyArchetypes in the content pass; the minimal presence below
	// makes squad orders meaningful today (4-8 dummies per spec).
	int32 EnemyIndex = 0;
	const FVector PrimarySite = FindSiteLocation(TEXT("Site_ControlPost"), PlayerLocation + FVector(3000.0f, 0.0f, 0.0f));
	for (int32 Index = 0; Index < 4; ++Index)
	{
		AEclipseCharacter* Enemy = SpawnBodyNear(PrimarySite + FVector(300.0f * Index, 200.0f * (Index % 2), 0.0f), FString::Printf(TEXT("Enforcer_%d"), Index));
		if (Enemy == nullptr)
		{
			continue;
		}
		AEclipseEnemyController* Controller = GetWorld()->SpawnActor<AEclipseEnemyController>();
		if (Controller != nullptr)
		{
			Controller->Possess(Enemy);
			SpawnedMissionActors.Add(Controller);
			++EnemyIndex;
		}
		SpawnedMissionActors.Add(Enemy);
	}

	UE_LOG(LogEclipse, Display, TEXT("GameMode: mission actors spawned (%d squadmates, %d enemies)."),
		Mission->GetDeployedSoldierIds().Num(), EnemyIndex);
}
