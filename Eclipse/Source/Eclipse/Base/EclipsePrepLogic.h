#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Strategy/EclipseCampaignTypes.h"

/**
 * Preparation validation as pure logic (GDD 14.3.2, SPEC-P1-08). The launch
 * payload is a contract: dead/wounded soldiers are unpickable, gated loadouts
 * require their produced unlock — validated here, not trusted from UI.
 */
namespace EclipsePrepLogic
{
	/** One resolved loadout option (subsystem loads the table; the core sees plain data). */
	struct FEclipseLoadoutOption
	{
		FName OptionId;
		FGameplayTag LoadoutTag;
		FGameplayTag RequiredUnlockTag;
	};

	/** Squad pick is legal iff exactly SquadSize distinct soldiers, all available on the current day and none holding a base post (SPEC-P2-03: assigned staff are undeployable). */
	ECLIPSE_API bool ValidateSquadPick(const FEclipseCampaignState& State, const TArray<FGuid>& SquadSoldierIds, int32 SquadSize, FString& OutError);

	/** Loadout is legal iff the option exists and its unlock (if any) has been produced (SPEC-P1-03 gate). */
	ECLIPSE_API bool ValidateLoadout(const FEclipseCampaignState& State, const TArray<FEclipseLoadoutOption>& Options, FGameplayTag LoadoutTag, FString& OutError);

	/** Options currently pickable (base + produced unlocks), table order. */
	ECLIPSE_API TArray<FEclipseLoadoutOption> GetAvailableLoadouts(const FEclipseCampaignState& State, const TArray<FEclipseLoadoutOption>& Options);
}
