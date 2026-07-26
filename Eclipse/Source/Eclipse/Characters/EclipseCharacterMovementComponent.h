#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EclipseCharacterMovementComponent.generated.h"

/**
 * Het movement component met richtingsbesef (feel-audit LOC-11/LOC-12).
 *
 * UE kent GEEN ingebouwde richtingsstraf: MaxWalkSpeed geldt voor alle richtingen
 * gelijk, dus achteruitlopen is standaard even snel als vooruit rennen. Dat is de
 * enige plek waar dat te repareren is — GetMaxSpeed is virtueel en wordt door
 * CalcVelocity per frame geraadpleegd.
 *
 * De waarden komen uit het enige spel in dit genre dat er cijfers over publiceert.
 * Gears 5 Title Update 3 (dec 2019) zette lateraal op 1.00 (strafen groepeert met
 * voorwaarts) en achteruit op 0.85, en kwam daar op uit NADAT 0.739 in de praktijk
 * te sloom bleek. Dat is een uitgespeelde, competitief getunede waarde uit precies
 * ons genre. [OFFICIEEL] Gears 5 TU3-patchnotes; zie phase0/FEEL_REFERENTIE.md
 * LOC-11/LOC-12.
 *
 * Dezelfde regel geldt voor IEDEREEN — speler, squad en vijand delen dit
 * component. Dat is geen gemakzucht maar GDD 8.3: dezelfde wapens, dezelfde
 * regels. Een vijand die zijwaarts om je heen loopt terwijl hij je aankijkt,
 * betaalt dus dezelfde prijs als jij.
 */
UCLASS()
class ECLIPSE_API UEclipseCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	virtual float GetMaxSpeed() const override;

	/**
	 * Snelheidsfactor bij zijwaarts. 1.00 = geen straf, en dat is bewust: Gears 5
	 * groepeert lateraal mét voorwaarts en investeerde in TU3 juist in BETER
	 * strafen ("increase the opportunities for skilled strafing"). Staat hier
	 * als knop, niet omdat er een straf hoort maar omdat de vorige aanname in dit
	 * project 0.75 was en die aanname nergens op steunde.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0.1, ClampMax = 1.0))
	float StrafeSpeedRatio = 1.0f;

	/** Snelheidsplafond tijdens mikken (cm/s); 0 = geen straf. Gezet door ApplyTuning. */
	UPROPERTY()
	float AimSpeed = 145.0f;

	/** Snelheidsfactor bij achteruit. 0.85 = de enige gepubliceerde waarde in het genre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Movement", meta = (ClampMin = 0.1, ClampMax = 1.0))
	float BackwardSpeedRatio = 0.85f;
};
