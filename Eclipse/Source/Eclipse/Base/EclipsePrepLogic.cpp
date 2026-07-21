#include "Base/EclipsePrepLogic.h"

#include "Squad/EclipseRosterLogic.h"

namespace EclipsePrepLogic
{

bool ValidateSquadPick(const FEclipseCampaignState& State, const TArray<FGuid>& SquadSoldierIds, int32 SquadSize, FString& OutError)
{
	if (SquadSoldierIds.Num() != SquadSize)
	{
		OutError = FString::Printf(TEXT("Pick exactly %d squadmates (%d picked)"), SquadSize, SquadSoldierIds.Num());
		return false;
	}

	TSet<FGuid> Seen;
	for (const FGuid& SoldierId : SquadSoldierIds)
	{
		bool bAlreadyPicked = false;
		Seen.Add(SoldierId, &bAlreadyPicked);
		if (bAlreadyPicked)
		{
			OutError = TEXT("The same soldier cannot deploy twice");
			return false;
		}

		const FEclipseSoldierRecord* Soldier = State.FindSoldier(SoldierId);
		if (Soldier == nullptr)
		{
			OutError = FString::Printf(TEXT("Unknown soldier %s"), *SoldierId.ToString());
			return false;
		}
		if (!EclipseRosterLogic::IsSoldierAvailableOnDay(*Soldier, State.Day))
		{
			// The reason names the person — the tester routes around a wounded
			// soldier because the UI can say *why* (SPEC-P1-08 DoD).
			OutError = FString::Printf(TEXT("%s is not available (status %d)"), *Soldier->Name, static_cast<int32>(Soldier->Status));
			return false;
		}
	}
	return true;
}

bool ValidateLoadout(const FEclipseCampaignState& State, const TArray<FEclipseLoadoutOption>& Options, FGameplayTag LoadoutTag, FString& OutError)
{
	const FEclipseLoadoutOption* Option = Options.FindByPredicate(
		[&LoadoutTag](const FEclipseLoadoutOption& Candidate) { return Candidate.LoadoutTag == LoadoutTag; });
	if (Option == nullptr)
	{
		OutError = FString::Printf(TEXT("Unknown loadout '%s'"), *LoadoutTag.ToString());
		return false;
	}
	if (Option->RequiredUnlockTag.IsValid() && !State.UnlockedLoadoutTags.Contains(Option->RequiredUnlockTag))
	{
		OutError = FString::Printf(TEXT("Loadout '%s' requires produced item '%s' (Workshop)"),
			*LoadoutTag.ToString(), *Option->RequiredUnlockTag.ToString());
		return false;
	}
	return true;
}

TArray<FEclipseLoadoutOption> GetAvailableLoadouts(const FEclipseCampaignState& State, const TArray<FEclipseLoadoutOption>& Options)
{
	TArray<FEclipseLoadoutOption> Available;
	for (const FEclipseLoadoutOption& Option : Options)
	{
		if (!Option.RequiredUnlockTag.IsValid() || State.UnlockedLoadoutTags.Contains(Option.RequiredUnlockTag))
		{
			Available.Add(Option);
		}
	}
	return Available;
}

} // namespace EclipsePrepLogic
