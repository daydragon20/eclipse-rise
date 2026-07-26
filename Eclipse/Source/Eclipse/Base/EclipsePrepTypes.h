#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipsePrepTypes.generated.h"

/**
 * Preparation data (SPEC-P1-08). Preparation is a first-class phase (GDD 11.1):
 * it converts strategic wealth into tactical fairness, so its numbers are
 * strategy data, never UI constants (GDD 14.2).
 */

/** One pickable loadout (DT_LoadoutOptions row; row name = option id). */
USTRUCT(BlueprintType)
struct FEclipseLoadoutOptionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FText DisplayName;

	/** Loadout identity handed to the mission runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FGameplayTag LoadoutTag;

	/** Empty = base option; otherwise requires this tag in CampaignState.UnlockedLoadoutTags (SPEC-P1-03 production gate). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FGameplayTag RequiredUnlockTag;

	/**
	 * WELKE WAPENS deze loadout je geeft (owner-opdracht 26-07 avond, punt 5).
	 *
	 * Rijnamen in DT_Weapons. Tot vandaag droeg een loadout alleen een TAG: je
	 * koos netjes, de keuze werd netjes onthouden, en het wapen in je handen was
	 * altijd de eerste rij van de tabel. Drie van de vier wapens waren daardoor
	 * voor niemand bereikbaar — en dat gat werd elke as die ik vandaag toevoegde
	 * alleen maar groter.
	 *
	 * RESEARCH — hoe het genre een loadout aan wapens bindt:
	 *   Destiny 2      drie vaste slots op munitiesoort; wisselen op één knop
	 *   The Division   twee primaries plus een sidearm, wisselen op Y/Driehoek
	 *   Ghost Recon    primair, secundair, sidearm
	 *   Borderlands    vier slots die je gaandeweg vrijspeelt, cyclen op één knop
	 *   Fortnite       vijf slots, cijfertoetsen of cyclen
	 *
	 * Wat ze ALLEMAAL doen: een klein VAST aantal slots, waarvan er één een
	 * sidearm is, en wisselen op één knop met een opheftijd die per wapen
	 * verschilt. Dat laatste is precies wat `ReadySeconds` in DT_Weapons al zegt
	 * en wat nog nergens gelezen werd.
	 *
	 * Twee slots en niet vier: vier vraagt een inventaris die opgeraapte wapens
	 * bijhoudt, en die bestaat hier niet. Twee is het minimum waarbij "wisselen"
	 * iets betekent, en het is wat Division en Ghost Recon ook geven.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FName PrimaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	FName SidearmWeapon;
};

UCLASS(BlueprintType)
class ECLIPSE_API UEclipsePrepTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Intel price of the briefing reveal (GDD 11.1 fairness loop, stub form). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep", meta = (ClampMin = 0))
	int32 IntelRevealCost = 5;

	/** Fallback squadmate count when no squad tuning resolves; DA_SquadTuning.MaxDeployed - 1 is the real source (SPEC-P2-01). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep", meta = (ClampMin = 1))
	int32 SquadSize = 2;

	/** Rows: FEclipseLoadoutOptionRow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Prep")
	TSoftObjectPtr<UDataTable> LoadoutOptions;
};
