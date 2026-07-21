#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EclipseGameMode.generated.h"

class AEclipseCharacter;

/**
 * Graybox game mode (SPEC-P1-05): possesses the player body, and — when the
 * mission runtime is active — spawns the squad of 2 (SPEC-P1-06) and the
 * mission's enemy sets at their labeled sites. All spawn parameters come from
 * mission/tuning data (GDD 14.2).
 */
UCLASS()
class ECLIPSE_API AEclipseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEclipseGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;

private:
	void SpawnMissionActors();
	AEclipseCharacter* SpawnBodyNear(const FVector& Location, const FString& Label);
	FVector FindSiteLocation(FName SiteId, const FVector& Fallback) const;
};
