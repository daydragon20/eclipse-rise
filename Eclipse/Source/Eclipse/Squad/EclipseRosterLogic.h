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
	 * The death-resolution policy (SPEC-P1-07, extended by SPEC-P2-01):
	 *  - mission failed  -> every downed soldier is dead (nobody carried them out)
	 *  - mission won     -> downed soldiers come home Wounded (out WoundedDaysOut)
	 *  - stabilized      -> Wounded even on failure (the GDD 4.2.5 save: a
	 *    stabilize inside the window converts permadeath into an injury)
	 * The stabilized branch needs WoundedDaysOut > 0 to express a wound; without
	 * roster tuning the conservative all-dead reading stands (GDD 14.3.5).
	 */
	ECLIPSE_API TArray<FEclipseResolvedCasualty> ResolveCasualties(
		const TMap<FGuid, FName>& DownedSoldiers,
		const FEclipseCampaignState& State,
		bool bMissionSuccess,
		int32 WoundedDaysOut,
		const TSet<FGuid>& StabilizedSoldiers = TSet<FGuid>());

	/** Availability read (SPEC-P1-08 squad picking): wounded soldiers return when the clock passes their recovery day. */
	ECLIPSE_API bool IsSoldierAvailableOnDay(const FEclipseSoldierRecord& Soldier, int32 Day);
}
