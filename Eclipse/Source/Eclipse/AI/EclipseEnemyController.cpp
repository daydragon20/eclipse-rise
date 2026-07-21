#include "AI/EclipseEnemyController.h"

#include "Characters/EclipseCharacter.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void AEclipseEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(InPawn))
	{
		Body->InitializeHealth(Archetype.Health);
	}
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &AEclipseEnemyController::SenseAndAct, Archetype.FireInterval, /*bLoop*/ true);
}

void AEclipseEnemyController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	Super::OnUnPossess();
}

void AEclipseEnemyController::ApplyArchetype(const FEclipseEnemyArchetypeRow& Row)
{
	Archetype = Row;
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
	{
		Body->InitializeHealth(Archetype.Health);
	}
}

AEclipseCharacter* AEclipseEnemyController::FindNearestVisibleHostile() const
{
	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (Body == nullptr || GetWorld() == nullptr)
	{
		return nullptr;
	}

	AEclipseCharacter* Nearest = nullptr;
	float NearestDistanceSquared = FMath::Square(Archetype.PerceptionRadius);
	for (TActorIterator<AEclipseCharacter> It(GetWorld()); It; ++It)
	{
		AEclipseCharacter* Candidate = *It;
		// Hostiles = the player's side (shared friend/foe rule on AEclipseCharacter).
		const bool bPlayerSide = Candidate->IsPlayerSide();
		if (Candidate == Body || Candidate->IsDowned() || !bPlayerSide)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), Body->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared && LineOfSightTo(Candidate))
		{
			NearestDistanceSquared = DistanceSquared;
			Nearest = Candidate;
		}
	}
	return Nearest;
}

void AEclipseEnemyController::SenseAndAct()
{
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (Body == nullptr || Body->IsDowned())
	{
		GetWorldTimerManager().ClearTimer(ThinkTimer);
		return;
	}

	AEclipseCharacter* Target = FindNearestVisibleHostile();
	if (Target == nullptr)
	{
		StopMovement(); // idle; patrol routes are level content (SPEC-P1-05 graybox pass)
		return;
	}

	SetFocus(Target);
	MoveToActor(Target, /*AcceptanceRadius*/ 600.0f);

	if (UEclipseHitscanWeaponComponent* Weapon = Body->FindComponentByClass<UEclipseHitscanWeaponComponent>())
	{
		const FVector Origin = Body->GetPawnViewLocation();
		Weapon->Fire(Origin, Target->GetActorLocation() - Origin, TEXT("EnemyFire"));
	}
	else
	{
		// Archetype damage without a weapon component: melee-range fallback so a
		// data mistake degrades to weak enemies, not invincible ones (GDD 14.3.5).
		if (FVector::DistSquared(Target->GetActorLocation(), Body->GetActorLocation()) < FMath::Square(200.0f))
		{
			Target->ApplyDamage(Archetype.Damage, Body, TEXT("EnemyMelee"));
		}
	}
}
