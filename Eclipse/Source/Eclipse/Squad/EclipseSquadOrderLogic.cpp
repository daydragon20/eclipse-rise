#include "Squad/EclipseSquadOrderLogic.h"

namespace EclipseSquadOrderLogic
{

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

	default:
		Decision.Reason = EEclipseOrderRefusalReason::InvalidTarget;
		return Decision;
	}

	Decision.bAccepted = true;
	return Decision;
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

} // namespace EclipseSquadOrderLogic
