#include "Combat/EclipseHitscanWeaponComponent.h"

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipsePlayerController.h"
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

bool UEclipseHitscanWeaponComponent::ShooterBodyIsAiming() const
{
	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetOwner());
	return Body != nullptr && Body->IsAiming();
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

	// De reeks breekt als je de trekker loslaat. Drie vuurintervallen stilte is
	// ruim genoeg om een bewuste pauze te zijn en te kort om per ongeluk te halen
	// tijdens aanhoudend vuur — dat is wat "eerste schot zuiver" bruikbaar maakt
	// in plaats van een eenmalige gratis kogel per missie.
	if (LastFireTimeSeconds < 0.0 || Now - LastFireTimeSeconds > Weapon.FireInterval * 3.0)
	{
		ConsecutiveShots = 0;
	}
	LastFireTimeSeconds = Now;
	++ShotsFired;

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
			Shot.WeaponSoundFamily = Weapon.SoundFamily;
			Shot.bSuppressed = Weapon.bSuppressed;
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

	// SPREIDING (owner-opdracht 26-07 avond, punt 4, laag B).
	//
	// EERSTE SCHOT IS ALTIJD ZUIVER. Dat is geen concessie aan testbaarheid maar
	// de conventie: Fortnite noemt het first-shot accuracy, CS bouwt zijn hele
	// spraypatroon eromheen, en Borderlands laat de accuracy pas onder aanhoudend
	// vuur zakken. Het maakt een enkel gericht schot een precisiehandeling en
	// aanhoudend vuur een gok — precies het onderscheid dat een DMR van een SMG
	// scheidt.
	//
	// Het geeft er ook een meetbaar systeem van: één schot is deterministisch, dus
	// het harnas kan de kogel volgen. Zonder dat zou elke gevechtsmeting ruis
	// bevatten en zou "mist hij of is hij kapot?" niet meer te beantwoorden zijn.
	const bool bAiming = ShooterBodyIsAiming();
	float SpreadDegrees = 0.0f;
	if (ConsecutiveShots > 0)
	{
		SpreadDegrees = bAiming ? Weapon.AimSpreadDegrees : Weapon.HipSpreadDegrees;

		// Bewegen straft, en per wapen anders: de SMG heeft 0,8 graden en de DMR
		// 4,0. Dat is wat "waardeloos in beweging" in data betekent.
		if (const APawn* ShooterPawn = Cast<APawn>(GetOwner()))
		{
			const float Speed = ShooterPawn->GetVelocity().Size2D();
			// Op de loopsnelheid geschaald, niet op de sprint: wie wandelt hoort
			// nauwelijks straf te voelen, wie rent de volle.
			const float MoveFraction = FMath::Clamp(Speed / 420.0f, 0.0f, 1.0f);
			SpreadDegrees += Weapon.MovingSpreadDegrees * MoveFraction;
		}
	}
	++ConsecutiveShots;

	// Terugslag naar de BESTUURDER, niet naar het wapen: het kruis moet omhoog, en
	// dat is een eigenschap van kijken. Alleen voor de speler — een vijand die
	// zijn eigen mikpunt omhoog duwt zou alleen zichzelf in de weg zitten.
	if (const AEclipseCharacter* ShooterBody = Cast<AEclipseCharacter>(GetOwner()))
	{
		if (AEclipsePlayerController* PC = Cast<AEclipsePlayerController>(ShooterBody->GetController()))
		{
			PC->AddRecoil(Weapon.RecoilPitchDegrees, Weapon.RecoilYawDegrees,
				Weapon.RecoilRecoveryDegreesPerSecond);
		}
	}

	FVector ShotDirection = ViewDirection.GetSafeNormal();
	if (SpreadDegrees > 0.0f)
	{
		ShotDirection = FMath::VRandCone(ShotDirection, FMath::DegreesToRadians(SpreadDegrees));
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseHitscan), /*bTraceComplex*/ false, GetOwner());
	const FVector End = ViewLocation + ShotDirection * Weapon.RangeCm;
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

	// De TREFFER als eigen feit (26-07). ShotFired vuurt ook bij een misser — dat
	// is precies de bedoeling daar — dus er was geen enkel feit dat "je hebt iets
	// geraakt" betekent. Daardoor lag Cue_SFX_Impact_BulletMetal_01 sinds de
	// audio-import ongebruikt in de repo: er was niets om hem aan te hangen.
	//
	// Draagt of het een kopschot was en hoeveel schade er landde, want dat is wat
	// feedback nodig heeft om onderscheid te maken. De hitmarker die de owner
	// overweegt kan hier zonder verdere aanpassing aan.
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			const AEclipseCharacter* ShooterBody = Cast<AEclipseCharacter>(GetOwner());
			FEclipseCombatEventPayload Landed;
			Landed.Shooter = GetOwner();
			Landed.Origin = Hit.ImpactPoint;
			Landed.bPlayerSide = ShooterBody != nullptr && ShooterBody->IsPlayerSide();
			Landed.bHeadshot = bHeadshot;
			Landed.Damage = Damage;
			Landed.Victim = HitCharacter;
			Bus->Broadcast(EclipseTags::Event_Combat_HitLanded, FInstancedStruct::Make(Landed));
		}
	}
	return true;
}
