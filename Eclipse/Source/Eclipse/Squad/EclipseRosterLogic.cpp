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

FString FirstNameOf(const FString& FullName)
{
	FString First;
	FString Rest;
	// Op de EERSTE spatie en niet op de laatste: "Vara Chen" en een eventuele
	// tussenvoegsel-achternaam ("Vara van Doorn") geven allebei "Vara".
	return FullName.Split(TEXT(" "), &First, &Rest) ? First : FullName;
}

int32 CountFirstNameCollisions(const TArray<FEclipseSoldierRecord>& Roster)
{
	TMap<FString, int32> Counts;
	for (const FEclipseSoldierRecord& Soldier : Roster)
	{
		++Counts.FindOrAdd(FirstNameOf(Soldier.Name));
	}

	int32 Colliding = 0;
	for (const TPair<FString, int32>& Entry : Counts)
	{
		if (Entry.Value > 1)
		{
			// Elk van de botsende mensen telt mee, niet de botsing als geheel: drie
			// keer "Sef" is erger dan twee keer, en dat hoort het getal te laten zien.
			Colliding += Entry.Value;
		}
	}
	return Colliding;
}

TArray<FEclipseSoldierRecord> GenerateRoster(
	FName OriginId, const FEclipseNameGenerationParams& Params, int32 Count,
	int32 FirstSeed, bool* OutFirstNamesExhausted)
{
	TArray<FEclipseSoldierRecord> Roster;
	if (Count <= 0)
	{
		return Roster;
	}
	Roster.Reserve(Count);

	// De pool KAN te klein zijn, en dan is uniek onmogelijk in plaats van vergeten.
	// Dat onderscheid hoort de aanroeper te krijgen: één is een datatekort, de ander
	// een bug.
	const bool bPoolTooSmall = Params.FirstNames.Num() > 0 && Params.FirstNames.Num() < Count;
	if (OutFirstNamesExhausted != nullptr)
	{
		*OutFirstNamesExhausted = bPoolTooSmall;
	}

	TSet<FString> UsedFirstNames;
	TSet<FString> UsedFullNames;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FEclipseSoldierRecord Soldier = GenerateSoldier(OriginId, Params, FirstSeed + Index);

		// Zonder pools levert GenerateSoldier "Recruit 07" en dergelijke: die zijn per
		// constructie al uniek (het zaad staat erin), dus er valt hier niets te
		// herstellen.
		if (Params.FirstNames.IsEmpty() || Params.LastNames.IsEmpty())
		{
			UsedFullNames.Add(Soldier.Name);
			Roster.Add(MoveTemp(Soldier));
			continue;
		}

		const FString DrawnFirst = FirstNameOf(Soldier.Name);
		FString First = DrawnFirst;
		FString Last;
		Soldier.Name.Split(TEXT(" "), nullptr, &Last);

		// DOORLOPEN VANAF DE GETROKKEN PLEK, niet opnieuw trekken.
		//
		// Opnieuw trekken tot het klopt is de voor de hand liggende oplossing en de
		// verkeerde: dat is een lus zonder bovengrens die bij een volle pool niet
		// eindigt, en hij zou de determinisme-eigenschap laten afhangen van hoeveel
		// keer er toevallig misgetrokken werd. Vooruitlopen door de lijst is
		// begrensd (hoogstens N stappen), altijd hetzelfde bij hetzelfde zaad, en
		// laat de oorspronkelijke trekking staan wanneer die al goed was.
		const int32 PoolSize = Params.FirstNames.Num();
		int32 StartIndex = Params.FirstNames.IndexOfByKey(DrawnFirst);
		if (StartIndex == INDEX_NONE)
		{
			StartIndex = 0;
		}
		for (int32 Step = 0; Step < PoolSize && UsedFirstNames.Contains(First); ++Step)
		{
			First = Params.FirstNames[(StartIndex + 1 + Step) % PoolSize];
		}

		// Is de voornamenpool op, dan mag hij terugkomen — maar dan móet de
		// achternaam verschillen, anders staan er twee identieke mensen in de ploeg
		// en is er helemaal niets meer te onderscheiden.
		FString FullName = FString::Printf(TEXT("%s %s"), *First, *Last);
		const int32 LastPoolSize = Params.LastNames.Num();
		int32 LastStart = Params.LastNames.IndexOfByKey(Last);
		if (LastStart == INDEX_NONE)
		{
			LastStart = 0;
		}
		for (int32 Step = 0; Step < LastPoolSize && UsedFullNames.Contains(FullName); ++Step)
		{
			Last = Params.LastNames[(LastStart + 1 + Step) % LastPoolSize];
			FullName = FString::Printf(TEXT("%s %s"), *First, *Last);
		}

		UsedFirstNames.Add(First);
		UsedFullNames.Add(FullName);
		Soldier.Name = FullName;
		Roster.Add(MoveTemp(Soldier));
	}

	return Roster;
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
