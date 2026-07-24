#include "Squad/EclipseRosterLogic.h"

#include "Math/RandomStream.h"

namespace EclipseRosterLogic
{

FEclipseSoldierRecord GenerateSoldier(FName OriginId, const FEclipseNameGenerationParams& Params, int32 Seed)
{
	FRandomStream Random(Seed);

	FEclipseSoldierRecord Soldier;
	// Deterministic id from the seed: the same campaign start produces the same
	// people, which keeps save fixtures and soak tests reproducible.
	Soldier.SoldierId = FGuid(0x45434C53, 0x50454F50, static_cast<uint32>(OriginId.GetNumber()), static_cast<uint32>(Seed));
	Soldier.OriginId = OriginId;
	Soldier.Status = EEclipseSoldierStatus::Available;

	if (!Params.FirstNames.IsEmpty() && !Params.LastNames.IsEmpty())
	{
		const FString& First = Params.FirstNames[Random.RandRange(0, Params.FirstNames.Num() - 1)];
		const FString& Last = Params.LastNames[Random.RandRange(0, Params.LastNames.Num() - 1)];
		Soldier.Name = FString::Printf(TEXT("%s %s"), *First, *Last);
	}
	else
	{
		Soldier.Name = FString::Printf(TEXT("Recruit %02d"), Seed);
	}

	if (!Params.TraitIds.IsEmpty())
	{
		Soldier.TraitId = Params.TraitIds[Random.RandRange(0, Params.TraitIds.Num() - 1)];
	}

	return Soldier;
}

TArray<FEclipseResolvedCasualty> ResolveCasualties(
	const TMap<FGuid, FName>& DownedSoldiers,
	const FEclipseCampaignState& State,
	bool bMissionSuccess,
	int32 WoundedDaysOut,
	const TSet<FGuid>& StabilizedSoldiers)
{
	TArray<FEclipseResolvedCasualty> Casualties;
	for (const TPair<FGuid, FName>& Downed : DownedSoldiers)
	{
		const FEclipseSoldierRecord* Record = State.FindSoldier(Downed.Key);

		// A stabilize inside the window (SPEC-P2-01) survives even a failed
		// mission — but only when the wound can be expressed (days > 0); a
		// missing-tuning zero keeps the conservative all-dead reading.
		const bool bStabilized = StabilizedSoldiers.Contains(Downed.Key) && WoundedDaysOut > 0;

		FEclipseResolvedCasualty& Casualty = Casualties.AddDefaulted_GetRef();
		Casualty.SoldierId = Downed.Key;
		Casualty.SoldierName = Record != nullptr ? Record->Name : FString();
		Casualty.Cause = Downed.Value;
		Casualty.bDead = !bMissionSuccess && !bStabilized;
		Casualty.DaysOut = Casualty.bDead ? 0 : WoundedDaysOut;
	}
	return Casualties;
}

bool IsSoldierAvailableOnDay(const FEclipseSoldierRecord& Soldier, int32 Day)
{
	switch (Soldier.Status)
	{
	case EEclipseSoldierStatus::Available: return true;
	case EEclipseSoldierStatus::Deployed:  return false;
	case EEclipseSoldierStatus::Wounded:   return Day >= Soldier.WoundedUntilDay;
	case EEclipseSoldierStatus::Dead:      return false;
	default:                               return false;
	}
}

} // namespace EclipseRosterLogic
