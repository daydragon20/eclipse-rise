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
	// Vol beginnen. Een wapen dat je halfleeg krijgt zou een verhaal vertellen dat
	// niemand geschreven heeft.
	AmmoInMagazine = Weapon.MagazineSize;
	bReloading = false;
	ReloadEndSeconds = -1.0;

	// Eén slot. Vijanden en squadmates krijgen hun wapen via dit pad en hebben
	// geen wissel — die is van de speler, want alleen hij kiest een loadout.
	SlotRows = { Row };
	SlotAmmo = { AmmoInMagazine };
	SlotNames = { NAME_None };
	ActiveSlot = 0;
	ReadyAtSeconds = -1.0;
}

void UEclipseHitscanWeaponComponent::ApplyLoadout(const FEclipseWeaponRow& Primary, FName PrimaryName,
	const FEclipseWeaponRow& Sidearm, FName SidearmName)
{
	ApplyWeaponRow(Primary);
	SlotNames[0] = PrimaryName;
	SlotRows.Add(Sidearm);
	SlotAmmo.Add(Sidearm.MagazineSize);
	SlotNames.Add(SidearmName);
}

bool UEclipseHitscanWeaponComponent::IsReady() const
{
	const UWorld* World = GetWorld();
	return World == nullptr || ReadyAtSeconds < 0.0 || World->GetTimeSeconds() >= ReadyAtSeconds;
}

bool UEclipseHitscanWeaponComponent::SwapWeapon()
{
	UWorld* World = GetWorld();
	if (World == nullptr || SlotRows.Num() < 2)
	{
		return false;
	}
	if (!IsReady())
	{
		return false; // je bent het vorige nog aan het optillen
	}

	// Het huidige magazijn bewaren. Dit is wat wisselen tactisch maakt in plaats
	// van cosmetisch: een halfleeg wapen komt halfleeg terug.
	SlotAmmo[ActiveSlot] = AmmoInMagazine;

	ActiveSlot = (ActiveSlot + 1) % SlotRows.Num();
	Weapon = SlotRows[ActiveSlot];
	AmmoInMagazine = SlotAmmo[ActiveSlot];

	// Een herlaadbeurt overleeft de wissel NIET. Dat is ook de klassieke truc uit
	// het genre: wisselen is sneller dan herladen, dus een tweede wapen is een
	// antwoord op een leeg magazijn. Zonder dit zou je kunnen wisselen en het
	// wapen alsnog vol terugkrijgen.
	bReloading = false;
	ReloadEndSeconds = -1.0;

	// Handling: ReadySeconds stond sinds vanmiddag in de data en werd door niets
	// gelezen. Dit is waar het getal betekenis krijgt — de sidearm is er in 0,25 s,
	// de DMR pas na 0,8 s, en dat is het verschil tussen een noodwapen en een keuze.
	ReadyAtSeconds = World->GetTimeSeconds() + Weapon.ReadySeconds;
	ConsecutiveShots = 0; // vers wapen, verse reeks: het eerste schot is weer zuiver

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			FEclipseCombatEventPayload Swap;
			Swap.Shooter = GetOwner();
			Swap.Origin = GetOwner() != nullptr ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
			Swap.WeaponSoundFamily = Weapon.SoundFamily;
			Swap.DurationSeconds = Weapon.ReadySeconds;
			const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetOwner());
			Swap.bPlayerSide = Body != nullptr && Body->IsPlayerSide();
			Bus->Broadcast(EclipseTags::Event_Combat_WeaponSwapped, FInstancedStruct::Make(Swap));
		}
	}
	return true;
}

bool UEclipseHitscanWeaponComponent::StartReload(FName Cause)
{
	// Cause is diagnostiek: "PlayerReload" of "MagazineEmpty". Het zegt WAAROM er
	// herladen wordt, en dat is het verschil tussen een speler die vooruitdenkt en
	// een die droogloopt — precies wat je wilt zien in een log van een speelronde.
	UWorld* World = GetWorld();
	if (World == nullptr || bReloading || Weapon.MagazineSize <= 0)
	{
		return false;
	}
	if (AmmoInMagazine >= Weapon.MagazineSize)
	{
		return false; // vol is vol; herladen om niets is een animatie zonder reden
	}

	bReloading = true;
	++ReloadCount;
	ReloadEndSeconds = World->GetTimeSeconds() + Weapon.ReloadSeconds;

	// HET FEIT, met zijn duur erbij. De foley-keten hangt aan de FASEN hiervan en
	// niet aan één geluid bij de start: het pack levert vier takes (magazijn
	// pakken, laten vallen, insteken, grendel) en die horen over de herlaadbeurt
	// verdeeld te worden. Zonder de duur in het feit zou de audiolaag de
	// wapentabel moeten lezen om te weten wanneer de grendel valt.
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			FEclipseCombatEventPayload Reload;
			Reload.Shooter = GetOwner();
			Reload.Origin = GetOwner() != nullptr ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
			Reload.WeaponSoundFamily = Weapon.SoundFamily;
			Reload.bSuppressed = Weapon.bSuppressed;
			Reload.DurationSeconds = Weapon.ReloadSeconds;
			const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetOwner());
			Reload.bPlayerSide = Body != nullptr && Body->IsPlayerSide();
			Bus->Broadcast(EclipseTags::Event_Combat_ReloadStarted, FInstancedStruct::Make(Reload));
		}
	}

	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetOwner()))
	{
		Body->PlayReloadPose(Weapon.ReloadSeconds);
	}
	return true;
}

void UEclipseHitscanWeaponComponent::FinishReload()
{
	bReloading = false;
	ReloadEndSeconds = -1.0;
	AmmoInMagazine = Weapon.MagazineSize;
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

	// Herladen loopt af op de klok en niet op een timer: een timer die tijdens een
	// missiewissel blijft staan zou een wapen voorgoed blokkeren, en dit component
	// tikt niet uit zichzelf.
	if (bReloading)
	{
		if (Now < ReloadEndSeconds)
		{
			return false; // je handen zitten aan het magazijn
		}
		FinishReload();
	}

	if (!IsReady())
	{
		return false; // het wapen is nog omhoog aan het komen (handling)
	}

	if (LastFireTimeSeconds >= 0.0 && Now - LastFireTimeSeconds < Weapon.FireInterval)
	{
		return false;
	}

	// LEEG: automatisch herladen in plaats van een dode trekker. Call of Duty,
	// Borderlands en Destiny doen het alle drie zo, en om dezelfde reden — een
	// trekker die niets doet leest als een defect, ook als je zelf vergat te
	// herladen.
	if (Weapon.MagazineSize > 0 && AmmoInMagazine <= 0)
	{
		StartReload(TEXT("MagazineEmpty"));
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
	if (Weapon.MagazineSize > 0)
	{
		--AmmoInMagazine;
	}

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
		// EEN SCHOT DAT DE WERELD RAAKT KRIJGT EEN UITKOMST. Owner-punt 4, 27-07:
		// "ik zie de kogelinslagen nauwelijks; ik weet niet of ik mis of dat de
		// vijand veel leven heeft."
		//
		// Hier stond alleen `return false`. Een schot in een muur, een krat, een
		// dekkingsblok of de grond raakte dus wél iets en werd vervolgens STIL
		// verworpen — geen feit, geen geluid, geen decal, geen spoor. Alleen
		// treffers op een personage bestonden. Daarmee is elke MIS onzichtbaar, en
		// missen is precies waaruit de speler probeert af te leiden of hij raakt.
		//
		// DIT RAAKT HET SCHADEPAD NIET. De tak hierboven (een personage geraakt)
		// blijft ongewijzigd; alleen de tak die niets deed doet nu iets. Dat is
		// bewust de kleinste stap: de owner vroeg om apart landen zodat een
		// regressie herleidbaar is, en een branch die eerder niets deed kan niets
		// breken.
		//
		// EERST HET FEIT, DAN PAS HET ZICHTBARE. Wat hier ontbrak was geen effect
		// maar een UITKOMST — er was niets om een decal of een geluid aan te
		// hangen. Oppervlak erbij, want dat systeem bestaat al voor voetstappen
		// (beton/metaal via physical materials) en is straks aan te sluiten zonder
		// een tweede bron voor hetzelfde gegeven.
		++WorldHits;
		const UPhysicalMaterial* Surface = Hit.PhysMaterial.Get();
		UE_LOG(LogEclipse, Verbose,
			TEXT("Wapen: wereldtreffer op %s (oppervlak %s) op %s — %d deze missie."),
			*GetNameSafe(Hit.GetActor()),
			Surface != nullptr ? *Surface->GetName() : TEXT("onbekend"),
			*Hit.ImpactPoint.ToCompactString(), WorldHits);

		// EN HET FEIT OP DE BUS, want een teller die alleen in dit component leeft
		// is precies de vorm waar dit project telkens op valt: staat in de data,
		// wordt nergens gelezen. De audiolaag luistert hier al op HitLanded voor
		// het inslaggeluid; die abonneert zich nu ook hierop, zodat een MISSER
		// hoorbaar wordt. Zelfde payloadtype — schutter en oorsprong zijn precies
		// wat een inslag nodig heeft — maar een EIGEN tag, omdat HitLanded
		// 'schade aan een personage' betekent en de hitmarker eraan hangt.
		if (UWorld* BusWorld = GetWorld())
		{
			if (UGameInstance* GameInstance = BusWorld->GetGameInstance())
			{
				if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
				{
					FEclipseCombatEventPayload Impact;
					Impact.Shooter = GetOwner();
					Impact.Origin = Hit.ImpactPoint;
					Bus->Broadcast(EclipseTags::Event_Combat_WorldImpact, FInstancedStruct::Make(Impact));
				}
			}
		}
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
	// SCHADE-AFVAL (26-07 avond, punt 4 — en een gat dat de dode-veldensweep
	// vond). FalloffStartCm en FalloffMinFraction stonden sinds vanmiddag in
	// DT_Weapons, met een uitleg erboven die zei wat ze deden, en er was geen
	// regel code die ze las. Ik had het als kenmerk opgeschreven en niet gebouwd.
	//
	// Lineair tussen de twee grenzen: op FalloffStartCm nog volle schade, op
	// RangeCm nog FalloffMinFraction daarvan. Lineair en niet met een curve, want
	// een curve is een tweede stel getallen die niemand heeft afgesteld — en dit
	// is precies het verschil dat een DMR van een SMG maakt zonder aan één
	// schadegetal te komen.
	if (Weapon.FalloffStartCm > 0.0f && Weapon.RangeCm > Weapon.FalloffStartCm)
	{
		const float Distance = static_cast<float>(FVector::Dist(ViewLocation, Hit.ImpactPoint));
		if (Distance > Weapon.FalloffStartCm)
		{
			const float Span = Weapon.RangeCm - Weapon.FalloffStartCm;
			const float Past = FMath::Clamp((Distance - Weapon.FalloffStartCm) / Span, 0.0f, 1.0f);
			Damage *= FMath::Lerp(1.0f, Weapon.FalloffMinFraction, Past);
		}
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
