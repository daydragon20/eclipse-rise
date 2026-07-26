#include "Characters/EclipseCharacterMovementComponent.h"

#include "Characters/EclipseCharacter.h"

#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

float UEclipseCharacterMovementComponent::GetMaxSpeed() const
{
	const float BaseSpeed = Super::GetMaxSpeed();

	// MIKKEN ZET JE VAST (owner-opdracht 26-07 avond, punt 3). Vóór de
	// grond-controle, want mikken hoort ook in de lucht te remmen — al is dat een
	// randgeval, het is geen reden om het anders te laten werken dan de speler
	// verwacht.
	//
	// Een CAP en geen factor: de straf moet gelden ongeacht of je loopt, rent of
	// hurkt, en een vermenigvuldiging zou hurkend mikken (150 x 0,345) op 52 cm/s
	// zetten. De traagste van de twee wint, dus hurkend mikken blijft 145.
	if (const AEclipseCharacter* Body = Cast<AEclipseCharacter>(CharacterOwner))
	{
		if (Body->IsAiming() && AimSpeed > 0.0f)
		{
			return FMath::Min(BaseSpeed, AimSpeed);
		}
	}

	// Alleen op de grond. In de lucht stuurt AirControl al, en een richtingsstraf
	// daar bovenop zou een sprong onvoorspelbaar maken.
	if (MovementMode != MOVE_Walking && MovementMode != MOVE_NavWalking)
	{
		return BaseSpeed;
	}

	// De referentierichting is waar de speler KIJKT, niet waar het lichaam staat.
	// Met bOrientRotationToMovement draait het lichaam naar zijn looprichting, dus
	// lichaams-relatief zou elke richting "vooruit" heten en zou de straf nooit
	// vuren. Dat onderscheid is de hele reden dat deze functie bestaat.
	const AController* OwningController = CharacterOwner != nullptr ? CharacterOwner->GetController() : nullptr;
	if (OwningController == nullptr)
	{
		return BaseSpeed;
	}

	// Acceleration is de bewegingsINTENTIE van dit frame; CalcVelocity heeft hem
	// al gezet voordat het GetMaxSpeed vraagt.
	const FVector Direction = Acceleration.GetSafeNormal2D();
	if (Direction.IsNearlyZero())
	{
		return BaseSpeed;
	}

	const FVector Forward = FRotator(0.0f, OwningController->GetControlRotation().Yaw, 0.0f).Vector();
	const float ForwardDot = FVector::DotProduct(Direction, Forward); // +1 vooruit, 0 zijwaarts, -1 achteruit

	// Vloeiend tussen de drie ankers in plaats van drie vakjes: een klif op precies
	// 90 graden voelt als een hapering terwijl je een bocht loopt.
	const float Ratio = ForwardDot >= 0.0f
		? FMath::Lerp(StrafeSpeedRatio, 1.0f, ForwardDot)
		: FMath::Lerp(StrafeSpeedRatio, BackwardSpeedRatio, -ForwardDot);

	return BaseSpeed * Ratio;
}
