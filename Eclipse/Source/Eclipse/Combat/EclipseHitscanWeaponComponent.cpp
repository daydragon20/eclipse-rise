#include "Combat/EclipseHitscanWeaponComponent.h"

#include "Characters/EclipseCharacter.h"
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
	float Damage = Weapon.Damage;
	if (Hit.BoneName == TEXT("head"))
	{
		Damage *= Weapon.HeadshotMultiplier;
	}

	HitCharacter->ApplyDamage(Damage, Cast<AEclipseCharacter>(GetOwner()), Cause);
	return true;
}
