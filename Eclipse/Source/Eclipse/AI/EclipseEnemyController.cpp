#include "AI/EclipseEnemyController.h"
#include "Eclipse.h"
#include "Navigation/PathFollowingComponent.h"

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
	// De uitkomstcode meelezen, want AAIController::MoveTo houdt zijn faalreden
	// voor zich (die gaat naar de VISUAL log). De speelronde meet dat de vijanden
	// 0,00 cm bewegen terwijl deze regel wél draait; zonder de code erbij is niet
	// te zien of hij faalt of denkt dat hij er al is. Eén regel, één keer per
	// vijand per denkbeurt, alleen als er iets misgaat of niets gebeurt.
	const EPathFollowingRequestResult::Type MoveResult = MoveToActor(Target, Archetype.EngageRange);
	// Eén keer melden dát hij iemand ziet. Zonder deze regel is "de vijand beweegt
	// niet" niet te onderscheiden van "de vijand heeft nooit iemand gezien" — en
	// dat verschil bepaalt of het een defect is of een testopstelling die hem van
	// buiten zijn waarnemingsbereik uitschakelt.
	if (!bLoggedFirstContact)
	{
		bLoggedFirstContact = true;
		UE_LOG(LogEclipse, Display, TEXT("Vijand %s: eerste contact op %.0f cm (waarnemingsbereik %.0f, engage-bereik %.0f)."),
			*GetNameSafe(GetPawn()), GetPawn() != nullptr ? FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation()) : -1.0f,
			Archetype.PerceptionRadius, Archetype.EngageRange);
	}

	// EEN KEER per vijand, op Display. Verbose leek netter maar vraagt een extra
	// console-commando en maakt het log onbruikbaar groot; een uitkomst die per
	// denkbeurt herhaalt voegt na de eerste keer niets toe. Zelfde patroon als de
	// gamepad-actuatieregels in de player controller.
	if (MoveResult != EPathFollowingRequestResult::RequestSuccessful && !bLoggedMoveOutcome)
	{
		bLoggedMoveOutcome = true;
		UE_LOG(LogEclipse, Display, TEXT("Vijand %s: MoveToActor gaf %s (afstand %.0f, engage-bereik %.0f)."),
			*GetNameSafe(GetPawn()), *UEnum::GetValueAsString(MoveResult),
			GetPawn() != nullptr ? FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation()) : -1.0f,
			Archetype.EngageRange);
	}

	if (UEclipseHitscanWeaponComponent* Weapon = Body->FindComponentByClass<UEclipseHitscanWeaponComponent>())
	{
		const FVector Origin = Body->GetPawnViewLocation();
		Weapon->Fire(Origin, Target->GetActorLocation() - Origin, TEXT("EnemyFire"));
	}
	else
	{
		// Archetype damage without a weapon component: melee-range fallback so a
		// data mistake degrades to weak enemies, not invincible ones (GDD 14.3.5).
		if (FVector::DistSquared(Target->GetActorLocation(), Body->GetActorLocation()) < FMath::Square(Archetype.MeleeRange))
		{
			Target->ApplyDamage(Archetype.Damage, Body, TEXT("EnemyMelee"));
		}
	}
}
