#include "AI/EclipseSquadmateController.h"

#include "Characters/EclipseCharacter.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "TimerManager.h"
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
	// Vanaf hier is er ECHT iets gezegd; meelopen wijkt daarvoor (zie UpdateFollow).
	bHasStandingOrder = true;
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
		// Class modulation in data (SPEC-P2-01, GDD 9.5): Assault kits push the
		// ordered point further along the approach; the order surface is unchanged.
		const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
		const FVector PushedTarget = Order == EEclipseSquadOrder::MoveTo && GetPawn() != nullptr
			? EclipseSquadOrderLogic::ComputePushedOrderPoint(GetPawn()->GetActorLocation(), TargetLocation, ClassKit.OrderPushDistanceCm)
			: TargetLocation;
		const FVector Destination = Order == EEclipseSquadOrder::MoveTo ? SelectCoverPointNear(PushedTarget) : TargetLocation;
		const float AcceptanceRadius = Order == EEclipseSquadOrder::MoveTo
			? (Tuning != nullptr ? Tuning->MoveAcceptanceRadius : 50.0f)
			: (Tuning != nullptr ? Tuning->RegroupAcceptanceRadius : 150.0f);
		// bProjectDestinationToNavigation is hier BEWUST niet aangezet, en dat is de
		// uitkomst van een proef: ik heb het geprobeerd omdat de padzoeker een
		// GEDEELTELIJK pad teruggaf, wat op een doel net naast de mesh wees. Het
		// veranderde niets aan de weigeringen, dus het bewijs ontbreekt en de
		// wijziging is teruggedraaid. Een aanpassing die niets aantoonbaar oplost
		// hoort niet in de boom, hoe redelijk hij ook klinkt.
		const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(Destination, AcceptanceRadius);
		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			// De UITKOMSTCODE erbij, want de engine houdt hem voor zich:
			// AAIController::MoveTo schrijft zijn faalreden naar de VISUAL log
			// (UE_VLOG) en niet naar het tekstlog. Daardoor was elke weigering van
			// buitenaf een zwarte doos — vijf hypotheses zijn erop stukgelopen.
			// Eén regel maakt "geweigerd" onderscheidbaar van "geweigerd, en wel
			// hierom", en dat is het verschil tussen zoeken en meten.
			UE_LOG(LogEclipse, Warning,
				TEXT("Squad: MoveToLocation geweigerd (%s) — doel %s, acceptatiestraal %.0f, pawn %s."),
				*UEnum::GetValueAsString(MoveResult), *Destination.ToCompactString(), AcceptanceRadius,
				GetPawn() != nullptr ? *GetPawn()->GetActorLocation().ToCompactString() : TEXT("geen"));
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
		// EEN ORDER IS EEN BELOFTE (GDD 8.4), en tot 26-07 werd deze gebroken:
		// "richt op dat doelwit" vuurde precies EEN kogel, op het moment van de
		// order, en daarna niets meer. De order bleef staan in CurrentOrder, maar
		// er was niets dat hem opnieuw uitvoerde — squadmates hebben, anders dan
		// vijanden, geen denkbeurt.
		//
		// Nu houdt een timer hem vast zolang de order staat en het doelwit leeft.
		// Bewust NIET meer dan dat: autonoom vuren op alles wat ze zien is een
		// ontwerpbeslissing (drie extra schutters verandert elk gevecht) en die is
		// niet aan mij. Dit herstelt alleen de belofte die je al gaf.
		FocusTargetActor = TargetActor;
		FocusFireShots = 0;
		ContinueFocusFire();
		break;
	}
	default:
		break;
	}

	return Decision;
}

void AEclipseSquadmateController::ContinueFocusFire()
{
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	AActor* Target = FocusTargetActor.Get();
	const AEclipseCharacter* TargetBody = Cast<AEclipseCharacter>(Target);

	// Stoppen zodra de belofte niet meer geldt: andere order, doelwit weg of
	// neer, of deze soldaat zelf neer. Alle vier zijn redenen om te zwijgen, en
	// geen ervan is een fout — daarom geen waarschuwing.
	if (Body == nullptr || Body->IsDowned() || Target == nullptr
		|| CurrentOrder != EEclipseSquadOrder::FocusTarget
		|| (TargetBody != nullptr && TargetBody->IsDowned()))
	{
		GetWorldTimerManager().ClearTimer(FocusFireTimer);
		FocusTargetActor = nullptr;
		return;
	}

	UEclipseHitscanWeaponComponent* Weapon = Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
	if (Weapon != nullptr)
	{
		const FVector Origin = Body->GetPawnViewLocation();
		if (Weapon->Fire(Origin, Target->GetActorLocation() - Origin, TEXT("SquadFocusFire")))
		{
			++FocusFireShots;
		}
	}

	// Op het vuurinterval van het wapen zelf, niet op een eigen getal: de poort in
	// Fire() bepaalt toch al het tempo, en een tweede klok ernaast zou daar tegenin
	// werken.
	const float Interval = Weapon != nullptr ? FMath::Max(Weapon->GetFireInterval(), 0.05f) : 0.15f;
	GetWorldTimerManager().SetTimer(FocusFireTimer, this,
		&AEclipseSquadmateController::ContinueFocusFire, Interval, /*bLoop*/ false);
}

APawn* AEclipseSquadmateController::GetLeaderPawn() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Player = World != nullptr ? World->GetFirstPlayerController() : nullptr;
	return Player != nullptr ? Player->GetPawn() : nullptr;
}

void AEclipseSquadmateController::BeginFollowing(float FollowDistanceCm)
{
	FollowDistance = FMath::Max(FollowDistanceCm, 100.0f);
	// Elke halve seconde. Een soldaat die op 420 cm/s loopt legt daarin 210 cm af,
	// ruim binnen de speling van een volgafstand van 400 — vaker kijken zou alleen
	// meer padberekeningen kosten en niets veranderen aan wat je ziet.
	GetWorldTimerManager().SetTimer(FollowTimer, this,
		&AEclipseSquadmateController::UpdateFollow, 0.5f, /*bLoop*/ true);
}

void AEclipseSquadmateController::UpdateFollow()
{
	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (Body == nullptr || Body->IsDowned())
	{
		return;
	}

	// EEN STAANDE ORDER WINT. Hold en MoveTo zetten deze soldaat ergens neer; die
	// belofte breken omdat de speler wegloopt zou orders waardeloos maken (8.4).
	// FocusTarget houdt hem ook: hij staat te vuren op wat je aanwees.
	if (bHasStandingOrder
		&& (CurrentOrder == EEclipseSquadOrder::Hold || CurrentOrder == EEclipseSquadOrder::MoveTo
			|| CurrentOrder == EEclipseSquadOrder::FocusTarget))
	{
		return;
	}

	// Triage wint ook: iemand overeind helpen is belangrijker dan bij de speler
	// blijven, en het loopt al op zijn eigen beweging.
	if (TriageTarget.IsValid())
	{
		return;
	}

	const APawn* Leader = GetLeaderPawn();
	if (Leader == nullptr)
	{
		return;
	}

	const float Apart = FVector::Dist2D(Body->GetActorLocation(), Leader->GetActorLocation());
	if (Apart <= FollowDistance)
	{
		return;
	}

	// Naar een punt VÓÓR de speler uit, niet naar zijn voeten: vier soldaten die
	// allemaal exact op de speler af lopen duwen elkaar weg en blijven om hem heen
	// draaien. Op volgafstand achter hem is waar een squad in Ghost Recon en
	// Division ook loopt.
	const FVector Behind = Leader->GetActorLocation() - Leader->GetActorForwardVector() * (FollowDistance * 0.5f);
	if (MoveToLocation(Behind, /*AcceptanceRadius*/ FollowDistance * 0.5f) == EPathFollowingRequestResult::RequestSuccessful)
	{
		++FollowMoves;
	}
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
		if (Candidate == Body || Candidate->IsDowned() || Candidate->IsPlayerSide())
		{
			continue; // one friend/foe source of truth: the player counts as friendly
			          // even without a soldier id (GDD 9.3; the id-only check made the
			          // squad take cover *from the player*)
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

		// Pure scorer (SPEC-P2-01): the class kit's lane bias makes Snipers
		// prefer the covered sample with the longer clear lane; bias 0 keeps
		// the SPEC-P1-06 nearest-cover behavior bit-for-bit.
		const float Score = EclipseSquadOrderLogic::ScoreCoverSample(
			bBlocked,
			FVector::Dist(Sample, OrderedLocation),
			FVector::Dist(Sample, NearestEnemy->GetActorLocation()),
			ClassKit.CoverLaneBias,
			Tuning != nullptr ? Tuning->CoverBlockBonus : 10.0f,
			Tuning != nullptr ? Tuning->CoverDistanceWeightPerCm : 0.001f);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestPoint = Sample;
		}
	}

	// Het gekozen punt op de NAVMESH projecteren, en anders terugvallen op het
	// bevolen punt. Dekking ligt per definitie ACHTER geometrie, dus een
	// ring-sample die goed scoort landt geregeld in of achter een muur — buiten de
	// navmesh. MoveToLocation faalt daar, en het order eindigt als een NoRoute-
	// weigering: de soldaat zegt "that path's blocked" terwijl het bevolen punt
	// prima bereikbaar was en alleen zijn eigen dekkingskeuze dat niet was.
	//
	// Gemeten met de speelronde (2026-07-26): navmesh aanwezig, het orderdoel op de
	// mesh, alle drie de soldaten op de mesh — en toch drie weigeringen. Daarmee
	// bleef dit als enige kandidaat over.
	//
	// De terugval is precies wat het commentaar hierboven zelf als principe noemt:
	// het order naar de LETTER uitvoeren wint van het optimaliseren van de geest.
	if (const UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation Projected;
		if (Nav->ProjectPointToNavigation(BestPoint, Projected, FVector(RingRadius, RingRadius, 200.0f)))
		{
			return Projected.Location;
		}
		return OrderedLocation;
	}
	return BestPoint;
}

bool AEclipseSquadmateController::BeginTriage(AEclipseCharacter* DownedBody)
{
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (DownedBody == nullptr || Body == nullptr || Body->IsDowned() || Body == DownedBody)
	{
		return false;
	}
	if (!ClassKit.bAutoTriage || ClassKit.StabilizeWindowSeconds <= 0.0f)
	{
		return false; // data says this kit does not triage — no class names in code
	}
	if (TriageTarget.IsValid())
	{
		return false; // one patient at a time; FinishTriage re-dispatches for casualties that went down mid-run
	}

	// Close to touch range; the squad tuning's move acceptance keeps the number
	// in data. The move is fire-and-forget: OnMoveCompleted picks it back up —
	// zero per-frame work (GDD 12.4).
	const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
	const float AcceptanceRadius = Tuning != nullptr ? Tuning->MoveAcceptanceRadius : 50.0f;

	FAIMoveRequest Request(DownedBody);
	Request.SetAcceptanceRadius(AcceptanceRadius);
	Request.SetUsePathfinding(true);
	const FPathFollowingRequestResult Result = MoveTo(Request);
	if (Result.Code == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogEclipse, Display, TEXT("Triage: no route to the casualty — the window keeps ticking (SPEC-P2-01)."));
		return false;
	}

	TriageTarget = DownedBody;
	TriageMoveId = Result.MoveId;
	CurrentOrder = EEclipseSquadOrder::MoveTo;

	// AlreadyAtGoal never fires OnMoveCompleted for a new id — finish inline.
	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		FinishTriage();
	}
	return true;
}

void AEclipseSquadmateController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	if (!TriageTarget.IsValid() || RequestID != TriageMoveId)
	{
		return; // a regular order's move, or the patient despawned mid-run
	}
	if (!Result.IsSuccess())
	{
		TriageTarget.Reset(); // route died — report nothing false; the mission clock decides the outcome
		return;
	}
	FinishTriage();
}

void AEclipseSquadmateController::FinishTriage()
{
	AEclipseCharacter* Patient = TriageTarget.Get();
	TriageTarget.Reset();
	UEclipseSquadSubsystem* Squad = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UEclipseSquadSubsystem>() : nullptr;
	if (Squad == nullptr)
	{
		return;
	}
	if (Patient != nullptr)
	{
		Squad->NotifyTriageArrived(this, Patient);
	}
	// This responder is free again: a casualty that went down mid-run gets its
	// dispatch now, not never (SPEC-P2-01; recursion terminates because every
	// arrival either saves or expires its patient, and the pending scan skips
	// both).
	Squad->DispatchPendingTriage();
}
