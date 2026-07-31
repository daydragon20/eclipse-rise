#include "Squad/EclipseSquadOrderLogic.h"

#include "HAL/PlatformTime.h"

namespace EclipseSquadOrderLogic
{

double NowWallSeconds()
{
	return FPlatformTime::Seconds();
}

void FEclipseOrderRoundTripStats::NoteRoundTrip(double Seconds)
{
	const double Clamped = FMath::Max(0.0, Seconds);
	++SampleCount;
	WithinBarCount += Clamped <= BarSeconds ? 1 : 0;
	WorstSeconds = FMath::Max(WorstSeconds, Clamped);
	TotalSeconds += Clamped;
}

void FEclipseOrderRoundTripStats::Reset()
{
	*this = FEclipseOrderRoundTripStats();
}

double FEclipseOrderRoundTripStats::GetAverageSeconds() const
{
	return SampleCount > 0 ? TotalSeconds / SampleCount : 0.0;
}

FEclipseOrderDecision DecideOrder(EEclipseSquadOrder Order, const FEclipseOrderWorldFacts& Facts)
{
	FEclipseOrderDecision Decision;

	// A downed soldier answers nothing but the medic — every order refuses with
	// the same reason (the player must never wonder why nobody moved).
	if (!Facts.bSoldierConscious)
	{
		Decision.Reason = EEclipseOrderRefusalReason::Downed;
		return Decision;
	}

	switch (Order)
	{
	case EEclipseSquadOrder::MoveTo:
	case EEclipseSquadOrder::Regroup:
		if (!Facts.bHasPathToTarget)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NoRoute;
			return Decision;
		}
		break;

	case EEclipseSquadOrder::FocusTarget:
		if (!Facts.bTargetValid)
		{
			Decision.Reason = EEclipseOrderRefusalReason::InvalidTarget;
			return Decision;
		}
		if (!Facts.bTargetVisible)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NoLineOfSight;
			return Decision;
		}
		break;

	case EEclipseSquadOrder::Hold:
		// Holding position needs nothing but consciousness.
		break;

	// ---- SPEC-P2-02 Stage B ------------------------------------------------
	// Elke nieuwe verb toetst ALLEEN wat hij zelf nodig heeft. De verleiding is
	// om er "en een geldig doel" bij te zetten omdat dat nooit kwaad kan; dat is
	// precies hoe je een reden krijgt die alles afdekt en dus niets uitlegt.

	case EEclipseSquadOrder::Suppress:
		// Een gebied onder vuur leggen vraagt ZICHT op dat gebied, geen route
		// erheen: onderdrukken doe je vanaf waar je staat.
		if (!Facts.bHasLineToArea)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NoLineOfSight;
			return Decision;
		}
		break;

	case EEclipseSquadOrder::Flank:
		// Eerst of er iets te flankeren VALT, dan of er omheen te komen is. De
		// volgorde is de uitleg: "flankeer wat?" is een ander probleem dan
		// "daar kom ik niet omheen", en de speler hoort het juiste te horen.
		if (!Facts.bTargetValid)
		{
			Decision.Reason = EEclipseOrderRefusalReason::InvalidTarget;
			return Decision;
		}
		if (!Facts.bHasPathToTarget)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NoRoute;
			return Decision;
		}
		break;

	case EEclipseSquadOrder::Breach:
		// Geen breekpunt gaat VOOR geen route: een missie zonder geauthorde
		// deurframes hoort "daar is niets om open te breken" te zeggen en niet
		// "ik kom er niet" — dat laatste zou de speler naar een routeprobleem
		// sturen dat niet bestaat (14.3.5, dezelfde val als de Downed-bug).
		if (!Facts.bHasBreachPointInRange)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NoBreachPoint;
			return Decision;
		}
		if (!Facts.bHasPathToTarget)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NoRoute;
			return Decision;
		}
		break;

	case EEclipseSquadOrder::UseAbility:
		// "Stabilize terwijl er niemand ligt" is geen kapotte order maar een
		// verkeerd moment, en dat is exact wat InvalidTarget betekent.
		if (!Facts.bAbilityContextValid)
		{
			Decision.Reason = EEclipseOrderRefusalReason::InvalidTarget;
			return Decision;
		}
		break;

	case EEclipseSquadOrder::SyncStrike:
		// Niets gemarkeerd gaat VOOR gezien worden: zonder doelen is de vraag of
		// ze je zien niet eens aan de orde, en "ze zien me" zou dan een reden zijn
		// die je op het verkeerde been zet terwijl je gewoon vergat te markeren.
		if (Facts.MarkedTargetCount <= 0)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NoTargetsMarked;
			return Decision;
		}
		if (!Facts.bAllAssignedConcealed)
		{
			Decision.Reason = EEclipseOrderRefusalReason::NotConcealed;
			return Decision;
		}
		break;

	default:
		// Onbereikbaar zolang elke enum-waarde hierboven staat, en dat is precies
		// waarom hij blijft: een verb die iemand toevoegt zonder tak eindigt hier
		// met een REDEN in plaats van met stilte (8.4).
		Decision.Reason = EEclipseOrderRefusalReason::InvalidTarget;
		return Decision;
	}

	Decision.bAccepted = true;
	return Decision;
}

FName RefusalPoolRowName(EEclipseOrderRefusalReason Reason)
{
	switch (Reason)
	{
	case EEclipseOrderRefusalReason::Downed:          return FName(TEXT("Downed"));
	case EEclipseOrderRefusalReason::NoBreachPoint:   return FName(TEXT("NoBreachPoint"));
	case EEclipseOrderRefusalReason::NoTargetsMarked: return FName(TEXT("NoTargetsMarked"));
	case EEclipseOrderRefusalReason::NotConcealed:    return FName(TEXT("NotConcealed"));

	// NoRoute / NoLineOfSight / InvalidTarget delen de pool van het ordertype, en
	// dat klopt: daar ZEGT de orderzin al wat er mis is ("No route, boss" onder
	// een MoveTo). De vier hierboven betekenen iets wat de orderzin niet dekt.
	default: return NAME_None;
	}
}

FString PickBarkLine(const TArray<FString>& Pool, const FGuid& SoldierId, uint32 Salt)
{
	if (Pool.IsEmpty())
	{
		// The pool being empty is a content gap, but the *answer* still exists:
		// silence is forbidden by design (GDD 9.5), so a stock line stands in.
		return TEXT("Copy.");
	}
	const uint32 Hash = HashCombine(GetTypeHash(SoldierId), Salt);
	return Pool[Hash % static_cast<uint32>(Pool.Num())];
}

FString ComposeOrderStateLine(const FString& SoldierName, const FGuid& SoldierId, const FString& OrderLabel)
{
	const FString Order = OrderLabel.IsEmpty() ? TEXT("(onbekend)") : OrderLabel;
	if (!SoldierName.IsEmpty())
	{
		return FString::Printf(TEXT("%s  ->  %s"), *SoldierName, *Order);
	}

	// GEEN NAAM = EEN GAT, en dat hoort er als gat uit te zien.
	//
	// De verleiding is om dan maar de id te tonen. Dat is precies hoe deze regel
	// drie soldaten lang `45434C53` liet zien: een getal dat op identiteit lijkt
	// leest als identiteit, ook als het niets onderscheidt. Als er een stuk id bij
	// staat, dan het achterste deel — daar zitten OriginId en Seed, de enige twee
	// woorden die per soldaat verschillen (zie EclipseRosterLogic.cpp).
	return FString::Printf(TEXT("(niet in de roster: %s)  ->  %s"),
		*SoldierId.ToString(EGuidFormats::Digits).Right(16), *Order);
}

FVector ComputePushedOrderPoint(const FVector& SoldierLocation, const FVector& OrderedLocation, float PushDistanceCm)
{
	if (PushDistanceCm <= 0.0f)
	{
		return OrderedLocation;
	}
	const FVector ToOrder = OrderedLocation - SoldierLocation;
	if (ToOrder.SizeSquared2D() < 1.0f)
	{
		return OrderedLocation; // ordered onto our own feet — nothing to push along
	}
	// Push stays planar: classes modulate ground positioning, not altitude.
	return OrderedLocation + ToOrder.GetSafeNormal2D() * PushDistanceCm;
}

float ScoreCoverSample(bool bBlocksThreatLine, float DistanceToOrderCm, float DistanceToThreatCm, float LaneBias,
	float CoverBlockBonus, float DistanceWeightPerCm)
{
	// The lane bonus applies only to covered samples: no bias value can talk a
	// soldier out of cover — the class changes taste, never competence (GDD 9.5
	// bug bar). Among covered samples, Sniper bias prefers the longer lane.
	// Bonus/weight come from DA_SquadTuning (P2-01 review m6); the defaults
	// mirror the shipped P1-06 numbers so missing tuning is behavior-neutral.
	const float LaneBonus = bBlocksThreatLine ? FMath::Max(0.0f, LaneBias) * DistanceToThreatCm * DistanceWeightPerCm : 0.0f;
	return (bBlocksThreatLine ? CoverBlockBonus : 0.0f) + LaneBonus - DistanceToOrderCm * DistanceWeightPerCm;
}

// ==========================================================================
// SPEC-P2-02 Stage B
// ==========================================================================

bool FEclipseSyncStrikeMarkSet::AddMark(const FGuid& TargetId, int32 MaxMarks)
{
	if (!TargetId.IsValid() || Marks.Contains(TargetId))
	{
		return false;
	}
	// Een cap van nul of minder is een datafout, geen bedoeling: één markering
	// toestaan degradeert naar een bruikbaar verb in plaats van naar een knop die
	// stilletjes nooit iets doet (14.3.5).
	if (Marks.Num() >= FMath::Max(1, MaxMarks))
	{
		return false;
	}
	Marks.Add(TargetId);
	return true;
}

bool FEclipseSyncStrikeMarkSet::RemoveMark(const FGuid& TargetId)
{
	return Marks.Remove(TargetId) > 0;
}

bool FEclipseSyncStrikeMarkSet::ToggleMark(const FGuid& TargetId, int32 MaxMarks, bool& bOutMarked)
{
	if (IsMarked(TargetId))
	{
		RemoveMark(TargetId);
		bOutMarked = false;
		return true;
	}
	const bool bAdded = AddMark(TargetId, MaxMarks);
	bOutMarked = bAdded;
	return bAdded;
}

int32 FEclipseSyncStrikeMarkSet::PruneMarks(const TArray<FGuid>& StillValidTargets)
{
	return Marks.RemoveAll([&StillValidTargets](const FGuid& Mark)
	{
		return !StillValidTargets.Contains(Mark);
	});
}

bool IsFlankWindowOpen(const FEclipseFlankApproval& State, double NowWallSeconds, double TimeoutSeconds)
{
	if (State.State != EEclipseFlankState::Proposed)
	{
		return false;
	}
	if (TimeoutSeconds <= 0.0)
	{
		return true; // geen venster geconfigureerd = geen verval (zie header)
	}
	return (NowWallSeconds - State.ProposedWallSeconds) <= TimeoutSeconds;
}

FEclipseFlankApproval ApplyFlankSignal(const FEclipseFlankApproval& State, EEclipseFlankSignal Signal,
	double NowWallSeconds, double TimeoutSeconds)
{
	FEclipseFlankApproval Next = State;

	switch (Signal)
	{
	case EEclipseFlankSignal::Propose:
		// Een nieuw voorstel wint altijd van een oud, ook van een goedgekeurd:
		// nog eens flankeren commanderen betekent "vergeet die vorige route".
		Next.State = EEclipseFlankState::Proposed;
		Next.ProposedWallSeconds = NowWallSeconds;
		return Next;

	case EEclipseFlankSignal::Tick:
		if (State.State == EEclipseFlankState::Proposed && !IsFlankWindowOpen(State, NowWallSeconds, TimeoutSeconds))
		{
			Next.State = EEclipseFlankState::Expired;
		}
		return Next;

	case EEclipseFlankSignal::Approve:
		if (State.State != EEclipseFlankState::Proposed)
		{
			return Next; // niets voorgesteld = niets goed te keuren (geen fout, een no-op)
		}
		// DE DRAGENDE REGEL: te laat keurt niet goed, te laat VERLOOPT.
		Next.State = IsFlankWindowOpen(State, NowWallSeconds, TimeoutSeconds)
			? EEclipseFlankState::Approved
			: EEclipseFlankState::Expired;
		return Next;

	case EEclipseFlankSignal::Cancel:
		if (State.State == EEclipseFlankState::Proposed)
		{
			Next.State = EEclipseFlankState::Cancelled;
		}
		return Next;
	}

	return Next;
}

bool StanceAllowsAutonomousFire(EEclipseSquadStance Stance, const FEclipseFireDisciplineFacts& Facts)
{
	switch (Stance)
	{
	case EEclipseSquadStance::Recon:
		// De bestaande Recon-regel, ongewijzigd: vuur pas als er op je geschoten
		// wordt. Hij staat hier nu naast Stealth zodat het verschil te lezen is in
		// plaats van verspreid over twee ifs in een controller.
		return Facts.bTakenFire;

	case EEclipseSquadStance::Stealth:
		// Twee poorten, en de tweede is het hele verschil met Recon: zodra de
		// vijand ons dóór heeft, koopt zwijgen niets meer.
		return Facts.bOrderedToFire || Facts.bTakenFire || Facts.bEnemyAware;

	default:
		// Ready, Overwatch, Aggressive: autonoom vuren IS de basis (26-07 laag 2).
		return true;
	}
}

TArray<int32> AssignSyncStrikeMarkIndices(int32 SoldierIndex, int32 SoldierCount, int32 MarkCount)
{
	TArray<int32> Assigned;
	if (SoldierIndex < 0 || SoldierCount <= 0 || MarkCount <= 0 || SoldierIndex >= SoldierCount)
	{
		return Assigned;
	}
	for (int32 Mark = SoldierIndex; Mark < MarkCount; Mark += SoldierCount)
	{
		Assigned.Add(Mark);
	}
	return Assigned;
}

} // namespace EclipseSquadOrderLogic
