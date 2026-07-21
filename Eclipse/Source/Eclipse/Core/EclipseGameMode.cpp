#include "Core/EclipseGameMode.h"

#include "AI/EclipseEnemyController.h"
#include "AI/EclipseSquadmateController.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipsePlayerController.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
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
	SpawnMissionActors();
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

	if (Mission == nullptr || Squad == nullptr || Campaign == nullptr
		|| Mission->GetPhase() != EEclipseMissionPhase::Objectives)
	{
		// No active mission = free-roam graybox (feel-target tuning sessions).
		return;
	}

	const FVector PlayerLocation = GetWorld()->GetFirstPlayerController() != nullptr && GetWorld()->GetFirstPlayerController()->GetPawn() != nullptr
		? GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation()
		: FVector::ZeroVector;

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
		}
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
			++EnemyIndex;
		}
	}

	UE_LOG(LogEclipse, Display, TEXT("GameMode: mission actors spawned (%d squadmates, %d enemies)."),
		Mission->GetDeployedSoldierIds().Num(), EnemyIndex);
}
