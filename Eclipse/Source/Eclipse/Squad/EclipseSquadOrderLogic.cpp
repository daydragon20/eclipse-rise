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

float ScoreCoverSample(bool bBlocksThreatLine, float DistanceToOrderCm, float DistanceToThreatCm, float LaneBias)
{
	// The lane bonus applies only to covered samples: no bias value can talk a
	// soldier out of cover — the class changes taste, never competence (GDD 9.5
	// bug bar). Among covered samples, Sniper bias prefers the longer lane.
	const float LaneBonus = bBlocksThreatLine ? FMath::Max(0.0f, LaneBias) * DistanceToThreatCm * 0.001f : 0.0f;
	return (bBlocksThreatLine ? 10.0f : 0.0f) + LaneBonus - DistanceToOrderCm * 0.001f;
}

} // namespace EclipseSquadOrderLogic
