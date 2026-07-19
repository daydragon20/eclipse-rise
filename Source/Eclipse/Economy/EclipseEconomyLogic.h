#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Strategy/EclipseCampaignTransaction.h"

/**
 * The economy ledger core (SPEC-P1-03): pure, deterministic, headless
 * (GDD 14.3.2 — no engine asset/actor types in here). The subsystem resolves
 * DataAssets/DataTables into FEclipseEconomyTickParams; this core turns state +
 * params into one campaign transaction. Same inputs, same transaction, always —
 * replayability is the test contract (GDD 12.3).
 */

/** One region's resolved static economy data (from the region graph asset). */
struct FEclipseRegionYieldParams
{
	FName RegionId;
	TMap<FGameplayTag, int32> BaseYieldPerDay;
};

/** One production item's resolved definition (from DT_ProductionItems). */
struct FEclipseProductionItemParams
{
	FName ItemId;
	int32 CostMaterials = 0;
	int32 CostCredits = 0;
	int32 TimeDays = 0;
	FGameplayTag ResultLoadoutTag;
};

/** Everything the day tick needs, resolved from data by the subsystem (GDD 14.2: all numbers in data). */
struct FEclipseEconomyTickParams
{
	int32 WagePerSoldierPerDay = 0;

	/** Fraction of Intel lost per decay application (GDD 6.2: 5%/week). */
	float IntelDecayPerWeek = 0.0f;

	/** Decay cadence in days (7 = weekly per GDD 6.2). */
	int32 IntelDecayIntervalDays = 7;

	/** Contested regions produce at this fraction (GDD 6.3.2: 40%). */
	float ContestedYieldFactor = 0.0f;

	TArray<FEclipseRegionYieldParams> Regions;
	TArray<FEclipseProductionItemParams> ProductionItems;

	FGameplayTag CreditsTag;
	FGameplayTag IntelTag;
};

namespace EclipseEconomyLogic
{
	/**
	 * Compose the post-DayAdvanced tick: region yields, wages, intel decay,
	 * production completions. Wages clamp to the available balance (a broke
	 * rebellion mounts debt drama in dialogue, not a rejected tick — GDD 6.5
	 * anti-frustration); the clamp is visible in the mutation's reason.
	 * Returns false if the tick has nothing to do.
	 */
	ECLIPSE_API bool BuildDayTick(const FEclipseCampaignState& State, const FEclipseEconomyTickParams& Params, FEclipseCampaignTransaction& OutTransaction);

	/**
	 * Compose the purchase transaction for one production order: spend + queue
	 * atomically (the "one real choice" of 13.2 — affordability is validated by
	 * the transaction, not trusted from UI).
	 */
	ECLIPSE_API bool BuildProductionOrder(const FEclipseCampaignState& State, const FEclipseEconomyTickParams& Params, FName ItemId, FGameplayTag MaterialsTag, FEclipseCampaignTransaction& OutTransaction, FString& OutError);
}
