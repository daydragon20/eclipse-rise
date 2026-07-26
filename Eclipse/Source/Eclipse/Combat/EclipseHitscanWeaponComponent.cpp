#include "Combat/EclipseHitscanWeaponComponent.h"

#include "Characters/EclipseCharacter.h"
#include "Eclipse.h"
#include "Components/SphereComponent.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/GameInstance.h"
#include "StructUtils/InstancedStruct.h"

#include "Characters/EclipseCharacter.h"
#include "Engine/World.h"

UEclipseHitscanWeaponComponent::UEclipseHitscanWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // event-driven (GDD 14.2)
}

void UEclipseHitscanWeaponComponent::ApplyWeaponRow(const FEclipseWeaponRow& Row)
{
	Weapon = Row;
}

bool UEclipseHitscanWeaponComponent::Fire(const FVector& ViewLocation, const FVector& ViewDirection, FName Cause)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const double Now = World->GetTimeSeconds();
	if (LastFireTimeSeconds >= 0.0 && Now - LastFireTimeSeconds < Weapon.FireInterval)
	{
		return false;
	}
	LastFireTimeSeconds = Now;

	// HET SCHOT VERRAADT JE (owner-opdracht 26-07, punt 1).
	//
	// Hier en niet na de trace, en dat is de hele pointe: een GEMIST schot maakt
	// evenveel lawaai als een rake. Dat is ook wat de referentie doet — in
	// Borderlands en The Division komt het geluid van de loop, niet van de inslag.
	// Stond dit onder de trace, dan zou missen gratis zijn en zou de hele mechaniek
	// omgekeerd werken: hoe slechter je schiet, hoe stiller je bent.
	//
	// De bus en niet rechtstreeks de AI aanroepen: het wapen hoort niet te weten
	// dat er vijanden bestaan (12.2 rule 2). De game mode luistert en vertaalt het
	// feit naar wie het hoort.
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			const AEclipseCharacter* Shooter = Cast<AEclipseCharacter>(GetOwner());
			FEclipseCombatEventPayload Shot;
			Shot.Shooter = GetOwner();
			Shot.Origin = ViewLocation;
			Shot.AlertRadiusCm = Weapon.GunshotAlertRadiusCm;
			Shot.bPlayerSide = Shooter != nullptr && Shooter->IsPlayerSide();
			Bus->Broadcast(EclipseTags::Event_Combat_ShotFired, FInstancedStruct::Make(Shot));
		}
	}

	// De schutter beweegt zichtbaar (26-07, punt 3). Vóór de trace, net als het
	// geluid: of je raakt verandert niets aan de beweging die je maakt.
	if (AEclipseCharacter* ShooterBody = Cast<AEclipseCharacter>(GetOwner()))
	{
		ShooterBody->PlayShootPose();
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseHitscan), /*bTraceComplex*/ false, GetOwner());
	const FVector End = ViewLocation + ViewDirection.GetSafeNormal() * Weapon.RangeCm;
	if (!World->LineTraceSingleByChannel(Hit, ViewLocation, End, ECC_Pawn, Params))
	{
		return false;
	}

	AEclipseCharacter* HitCharacter = Cast<AEclipseCharacter>(Hit.GetActor());
	if (HitCharacter == nullptr)
	{
		return false;
	}

	// Locational damage stub (GDD 8.2): head bone multiplier when a skeletal
	// hit is available; graybox capsules simply take base damage.
	// KOPSCHOT (owner-opdracht 26-07, punt 2, optie 1).
	//
	// Hier stond `Hit.BoneName == "head"`, en die naam komt bij een capsule-trace
	// nooit terug: de graybox-lichamen worden geraakt op hun capsule, die geen
	// botten heeft. Kopschoten deden dus letterlijk niets — 22 hp op borst én
	// hoofd, terwijl DT_Weapons 2,5x zegt. Gemeten in de nacht van 25→26 juli en
	// vandaag gerepareerd.
	//
	// De hitbox is een echte component op de hoofd-socket; de beslissing is een
	// straal-tegen-bol. Zie AEclipseCharacter::ShotLineHitsHead voor waarom niet
	// op trace-volgorde: de bol zit binnen de capsule, dus die wint altijd.
	float Damage = Weapon.Damage;
	const bool bHeadshot = HitCharacter->ShotLineHitsHead(ViewLocation, End);
	// Verbose: de probe was onmisbaar bij het bouwen (hij liet zien dat de
	// schotlijn 18,9 cm langs een hitbox van 14 cm ging) en is dat weer zodra
	// iemand aan de hitbox of het richten komt. Op Display zou hij het log
	// vullen met een regel per schot.
	if (HeadshotProbesLogged < 40)
	{
		++HeadshotProbesLogged;
		const USphereComponent* Head = HitCharacter->GetHeadHitbox();
		const FVector Centre = Head != nullptr ? Head->GetComponentLocation() : FVector::ZeroVector;
		UE_LOG(LogEclipse, Verbose,
			TEXT("Kopschot-probe: hitbox=%s midden=%s straal=%.1f, dichtste nadering=%.1f cm, raak=%d"),
			Head != nullptr ? TEXT("ja") : TEXT("NEE"), *Centre.ToCompactString(),
			Head != nullptr ? Head->GetScaledSphereRadius() : -1.0f,
			Head != nullptr ? FVector::Dist(FMath::ClosestPointOnSegment(Centre, ViewLocation, End), Centre) : -1.0f,
			bHeadshot ? 1 : 0);
	}
	if (bHeadshot)
	{
		Damage *= Weapon.HeadshotMultiplier;
	}

	HitCharacter->ApplyDamage(Damage, Cast<AEclipseCharacter>(GetOwner()),
		bHeadshot ? FName(*(Cause.ToString() + TEXT("_Head"))) : Cause);
	return true;
}
