#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "EclipseCampaignSetupAsset.generated.h"

class UDataTable;
class UEclipseCharacterTuningAsset;
class UEclipseEconomyDataAsset;
class UEclipsePrepTuningAsset;
class UEclipseRegionGraphAsset;
class UEclipseRosterTuningAsset;
class UEclipseSquadTuningAsset;

/**
 * Campaign starting conditions (SPEC-P1-02 data schema). Every starting number
 * lives here — a hardcoded starting value in code is a defect (GDD 14.2).
 * Missing asset at campaign start = logged warning + graceful empty campaign,
 * never a crash (GDD 14.3.5).
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseCampaignSetupAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Campaign clock start (day index; display formatting is UI's problem). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign", meta = (ClampMin = 0))
	int32 StartingDay = 0;

	/** Starting balances keyed by Resource.* tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TMap<FGameplayTag, int32> StartingResources;

	/**
	 * Starting roster size (~8 per SPEC-P1-07). Names/traits come from the name
	 * pool tables when SPEC-P1-07 lands; until then StartNewCampaign generates
	 * numbered placeholders. PLACEHOLDER(GDD 4.2.1).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign", meta = (ClampMin = 0))
	int32 StartingRosterSize = 8;

	/** The district's region definitions (SPEC-P1-04 asset). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UEclipseRegionGraphAsset> RegionGraph;

	/** Economy tunables for this campaign (SPEC-P1-03). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UEclipseEconomyDataAsset> EconomyData;

	/** Roster tunables + name/trait pools (SPEC-P1-07). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UEclipseRosterTuningAsset> RosterTuning;

	/** Preparation tunables + loadout options (SPEC-P1-08). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UEclipsePrepTuningAsset> PrepTuning;

	/** Squad tunables + order bark pools (SPEC-P1-06). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UEclipseSquadTuningAsset> SquadTuning;

	/** Movement/health feel targets applied to every spawned body (SPEC-P1-05; locked graybox feel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UEclipseCharacterTuningAsset> CharacterTuning;

	/** Rows: FEclipseWeaponRow — the player-side weapon platforms (SPEC-P1-05). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UDataTable> Weapons;

	/** Rows: FEclipseEnemyArchetypeRow — enemy stats resolved at mission spawn (SPEC-P1-05). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UDataTable> EnemyArchetypes;

	/** Rows: FEclipseBodyDefRow — visual bodies for player/squad/enemies (step-2 character pipeline); "Player" and "Rebel_*" rows are player-side. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TSoftObjectPtr<UDataTable> BodyDefs;
};
