#include "AI/EclipseSquadmateController.h"

#include "Characters/EclipseCharacter.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Eclipse.h"
#include "EngineUtils.h"
#include "Navigation/PathFollowingComponent.h" // EPathFollowingRequestResult (AIController.h only forward-declares it)
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "Squad/EclipseSquadTypes.h"

EclipseSquadOrderLogic::FEclipseOrderWorldFacts AEclipseSquadmateController::GatherFacts(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor) const
{
	EclipseSquadOrderLogic::FEclipseOrderWorldFacts Facts;

	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	Facts.bSoldierConscious = Body != nullptr && !Body->IsDowned();

	if (Order == EEclipseSquadOrder::MoveTo || Order == EEclipseSquadOrder::Regroup)
	{
		Facts.bHasPathToTarget = false;
		if (const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()); NavSystem != nullptr && Body != nullptr)
		{
			const UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
				GetWorld(), Body->GetActorLocation(), TargetLocation, const_cast<AEclipseCharacter*>(Body));
			Facts.bHasPathToTarget = Path != nullptr && Path->IsValid() && !Path->IsPartial();
		}
	}

	if (Order == EEclipseSquadOrder::FocusTarget)
	{
		// A valid focus target is a live hostile — never a friendly or the player
		// (focusing our own side must refuse, not open fire on them).
		const AEclipseCharacter* Target = Cast<AEclipseCharacter>(TargetActor);
		Facts.bTargetValid = Target != nullptr && !Target->IsDowned() && !Target->IsPlayerSide();
		Facts.bTargetVisible = Facts.bTargetValid && LineOfSightTo(Target);
	}

	return Facts;
}

const UEclipseSquadTuningAsset* AEclipseSquadmateController::ResolveTuning() const
{
	const UEclipseSquadSubsystem* Squad = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UEclipseSquadSubsystem>() : nullptr;
	return Squad != nullptr ? Squad->ResolveTuning() : nullptr;
}

EclipseSquadOrderLogic::FEclipseOrderDecision AEclipseSquadmateController::ExecuteOrder(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor, EEclipseSquadStance Stance)
{
	const EclipseSquadOrderLogic::FEclipseOrderDecision Decision =
		EclipseSquadOrderLogic::DecideOrder(Order, GatherFacts(Order, TargetLocation, TargetActor));
	if (!Decision.bAccepted)
	{
		return Decision;
	}

	CurrentOrder = Order;
	CurrentStance = Stance; // stored for the feel pass; no behavior split in Phase 1
	switch (Order)
	{
	case EEclipseSquadOrder::MoveTo:
	case EEclipseSquadOrder::Regroup:
	{
		// GatherFacts validated a path to the ordered point, but MoveTo actually
		// drives to a cover point near it. If that pick is unreachable, downgrade
		// the accept to a reasoned NoRoute refusal so an accepted order never
		// stalls silently (GDD 8.4 never-silent contract — the pure decision
		// table cannot see the chosen destination).
		const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
		const FVector Destination = Order == EEclipseSquadOrder::MoveTo ? SelectCoverPointNear(TargetLocation) : TargetLocation;
		const float AcceptanceRadius = Order == EEclipseSquadOrder::MoveTo
			? (Tuning != nullptr ? Tuning->MoveAcceptanceRadius : 50.0f)
			: (Tuning != nullptr ? Tuning->RegroupAcceptanceRadius : 150.0f);
		if (MoveToLocation(Destination, AcceptanceRadius) == EPathFollowingRequestResult::Failed)
		{
			StopMovement();
			CurrentOrder = EEclipseSquadOrder::Hold;
			EclipseSquadOrderLogic::FEclipseOrderDecision Refusal;
			Refusal.Reason = EEclipseOrderRefusalReason::NoRoute;
			return Refusal;
		}
		break;
	}
	case EEclipseSquadOrder::Hold:
		StopMovement();
		break;
	case EEclipseSquadOrder::FocusTarget:
	{
		SetFocus(TargetActor);
		AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
		UEclipseHitscanWeaponComponent* Weapon = Body != nullptr ? Body->FindComponentByClass<UEclipseHitscanWeaponComponent>() : nullptr;
		if (Weapon != nullptr && TargetActor != nullptr)
		{
			const FVector Origin = Body->GetPawnViewLocation();
			Weapon->Fire(Origin, TargetActor->GetActorLocation() - Origin, TEXT("SquadFocusFire"));
		}
		break;
	}
	default:
		break;
	}

	return Decision;
}

void AEclipseSquadmateController::HandlePawnDowned()
{
	StopMovement();
	SetFocus(nullptr);
	CurrentOrder = EEclipseSquadOrder::Hold;
}

FVector AEclipseSquadmateController::SelectCoverPointNear(const FVector& OrderedLocation) const
{
	const APawn* Body = GetPawn();
	UWorld* World = GetWorld();
	if (Body == nullptr || World == nullptr)
	{
		return OrderedLocation;
	}

	// Nearest conscious hostile = anything the mission spawned against us; the
	// scorer only needs one threat direction at prototype scale.
	const AEclipseCharacter* NearestEnemy = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<AEclipseCharacter> It(World); It; ++It)
	{
		const AEclipseCharacter* Candidate = *It;
		if (Candidate == Body || Candidate->IsDowned() || Candidate->GetSoldierId().IsValid())
		{
			continue; // roster soldiers are friendlies; enemies carry no soldier id
		}
		const float Distance = FVector::DistSquared(Candidate->GetActorLocation(), OrderedLocation);
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			NearestEnemy = Candidate;
		}
	}
	if (NearestEnemy == nullptr)
	{
		return OrderedLocation;
	}

	// Ring samples around the ordered point; a sample "is cover" when geometry
	// blocks the enemy's line to it. Ties go to the sample closest to the order —
	// obeying the letter of the order beats optimizing the spirit. Radius + sample
	// count are data (SPEC-P1-06 Data); missing tuning degrades to code defaults.
	const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
	const int32 SampleCount = Tuning != nullptr ? FMath::Max(3, Tuning->CoverRingSamples) : 8;
	const float RingRadius = Tuning != nullptr ? Tuning->CoverRingRadius : 200.0f;
	FVector BestPoint = OrderedLocation;
	float BestScore = -1.0f;
	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Angle = (2.0f * PI * Index) / SampleCount;
		const FVector Sample = OrderedLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * RingRadius;

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseCoverScore), false);
		Params.AddIgnoredActor(Body);
		const bool bBlocked = World->LineTraceSingleByChannel(
			Hit, Sample + FVector(0, 0, 50.0f), NearestEnemy->GetActorLocation() + FVector(0, 0, 50.0f), ECC_Visibility, Params)
			&& Hit.GetActor() != NearestEnemy;

		const float Score = (bBlocked ? 10.0f : 0.0f) - FVector::Dist(Sample, OrderedLocation) * 0.001f;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestPoint = Sample;
		}
	}
	return BestPoint;
}
