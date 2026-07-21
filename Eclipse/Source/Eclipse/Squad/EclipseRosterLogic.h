#pragma once

#include "CoreMinimal.h"
#include "Quests/EclipseMissionLogic.h"
#include "Strategy/EclipseCampaignTypes.h"

/**
 * Roster generation and casualty policy as pure logic (GDD 14.3.2).
 * Deterministic by seed so campaign starts are reproducible in tests.
 */
namespace EclipseRosterLogic
{
	/** Resolved name/trait pools (the subsystem loads tables; the core sees plain data). */
	struct FEclipseNameGenerationParams
	{
		TArray<FString> FirstNames;
		TArray<FString> LastNames;
		TArray<FName> TraitIds;
	};

	/**
	 * Generate one soldier deterministically. Falls back to a numbered recruit
	 * when pools are empty (missing content = graceful default, GDD 14.3.5) —
	 * but a name is always produced: nameless soldiers violate Pillar 3.
	 */
	ECLIPSE_API FEclipseSoldierRecord GenerateSoldier(FName OriginId, const FEclipseNameGenerationParams& Params, int32 Seed);

	/**
	 * The Phase 1 death-resolution policy (SPEC-P1-07):
	 *  - mission failed  -> every downed soldier is dead (nobody carried them out)
	 *  - mission won     -> downed soldiers come home Wounded (out WoundedDaysOut)
	 * The "extraction without body" stub is the failure branch.
	 */
	ECLIPSE_API TArray<FEclipseResolvedCasualty> ResolveCasualties(
		const TMap<FGuid, FName>& DownedSoldiers,
		const FEclipseCampaignState& State,
		bool bMissionSuccess,
		int32 WoundedDaysOut);

	/** Availability read (SPEC-P1-08 squad picking): wounded soldiers return when the clock passes their recovery day. */
	ECLIPSE_API bool IsSoldierAvailableOnDay(const FEclipseSoldierRecord& Soldier, int32 Day);
}
