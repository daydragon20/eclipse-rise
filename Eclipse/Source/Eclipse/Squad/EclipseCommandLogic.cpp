#include "Squad/EclipseCommandLogic.h"

namespace EclipseCommandLogic
{

FEclipseCommandModeState ApplySignal(const FEclipseCommandModeState& State, EEclipseCommandSignal Signal, double NowWallSeconds)
{
	FEclipseCommandModeState Next = State;
	if (Signal == EEclipseCommandSignal::HoldPressed)
	{
		if (!State.bHeld)
		{
			Next.bHeld = true;
			Next.EnteredWallSeconds = NowWallSeconds;
			Next.OrdersIssuedWhileHeld = 0;
		}
		return Next;
	}

	// Release, pawn death, mission end, level travel: all of them end the hold
	// (SPEC-P2-02 fail-safe). A signal on an idle state is a no-op, not an error.
	Next.bHeld = false;
	return Next;
}

float DesiredDilation(const FEclipseCommandModeState& State, float DilationFactor)
{
	return State.bHeld ? DilationFactor : 1.0f;
}

void FEclipseEncounterBeatTally::NoteModeEntered(double NowWallSeconds, double IdleGapSeconds)
{
	// Only an entry can trigger the idle rule, so the beat it closes always had
	// at least one entry — the automatic path never invents an empty beat.
	if (EntriesThisBeat > 0 && IdleGapSeconds > 0.0 && (NowWallSeconds - LastEntryWallSeconds) > IdleGapSeconds)
	{
		BeginNextBeat(/*bCountEmptyBeat*/ false);
	}
	LastEntryWallSeconds = NowWallSeconds;
	++EntriesThisBeat;
}

void FEclipseEncounterBeatTally::BeginNextBeat(bool bCountEmptyBeat)
{
	if (EntriesThisBeat <= 0 && !bCountEmptyBeat)
	{
		return; // nothing to close; a phase boundary is not evidence a fight happened
	}
	ClosedBeatEntries += EntriesThisBeat;
	++ClosedBeatCount;
	EntriesThisBeat = 0;
}

void FEclipseEncounterBeatTally::Reset()
{
	*this = FEclipseEncounterBeatTally();
}

float FEclipseEncounterBeatTally::GetAverageEntriesPerBeat() const
{
	return ClosedBeatCount > 0 ? static_cast<float>(ClosedBeatEntries) / static_cast<float>(ClosedBeatCount) : 0.0f;
}

FGuid CycleSelection(const TArray<FGuid>& AliveSoldiers, const FGuid& Current, int32 Direction)
{
	if (AliveSoldiers.IsEmpty())
	{
		return FGuid();
	}

	const int32 CurrentIndex = AliveSoldiers.IndexOfByKey(Current);
	if (CurrentIndex == INDEX_NONE)
	{
		// No selection yet, or the selected soldier left the list mid-hold:
		// restart from the edge the direction naturally enters.
		return Direction >= 0 ? AliveSoldiers[0] : AliveSoldiers.Last();
	}

	const int32 Count = AliveSoldiers.Num();
	const int32 Step = Direction >= 0 ? 1 : Count - 1; // modular decrement without negative wrap
	return AliveSoldiers[(CurrentIndex + Step) % Count];
}

} // namespace EclipseCommandLogic
