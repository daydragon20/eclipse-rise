#include "Combat/EclipseHitscanWeaponComponent.h"

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipsePlayerController.h"
#include "Combat/EclipseImpactMark.h"
#include "Eclipse.h"
#include "Components/SphereComponent.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StructUtils/InstancedStruct.h"

#include "Characters/EclipseCharacter.h"
#include "Engine/World.h"

UEclipseHitscanWeaponComponent::UEclipseHitscanWeaponComponent()
{
	// EVENT-GEDREVEN (GDD 14.2), MET ÉÉN VENSTER WAARIN HIJ TIKT.
	//
	// `bCanEverTick` stond hier op false en dat klopte zolang alles aan dit wapen
	// een gebeurtenis was. Herlaad-VOORTGANG is dat niet: tussen het begin en het
	// eind van een beurt gebeurt er niets, er verstrijkt alleen tijd, en een balk
	// die daaraan hangt heeft iets nodig dat kijkt. `bStartWithTickEnabled = false`
	// houdt de kosten waar ze horen — StartReload zet hem aan, TickComponent zet
	// hem zelf weer uit, en voor een wapen zonder lokale speler gaat hij nooit aan.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UEclipseHitscanWeaponComponent::HasLocalPlayerConsumer() const
{
	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetOwner());
	if (Body == nullptr)
	{
		return false;
	}
	const AController* Driver = Body->GetController();
	return Driver != nullptr && Driver->IsPlayerController() && Driver->IsLocalController();
}

EclipseWeaponStatusFeed::FEclipseWeaponSnapshot UEclipseHitscanWeaponComponent::MakeStatusSnapshot() const
{
	EclipseWeaponStatusFeed::FEclipseWeaponSnapshot Snapshot;
	// DE GETTERS EN NIET DE VELDEN, en dat is geen netheid. `AmmoInMagazine` en
	// `bReloading` zijn opgeslagen waarden die tot een handeling langskomt achter de
	// klok aan kunnen lopen; GetAmmoInMagazine() en IsReloading() REKENEN de
	// waarheid uit (zie de toelichting bij IsReloading — dat was defect 2). Het feit
	// op de bus moet zeggen wat het wapen werkelijk is, niet wat er het laatst is
	// opgeschreven.
	Snapshot.AmmoInMagazine = GetAmmoInMagazine();
	Snapshot.MagazineSize = Weapon.MagazineSize;
	// -1: de voorraad is met opzet oneindig; zie de toelichting bij StartReload in
	// deze header. Als er ooit een echte voorraad komt, is dit het enige veld dat
	// verandert — schema, catalogus en HUD blijven zoals ze zijn.
	Snapshot.SpareMagazines = -1;
	Snapshot.bReloading = IsReloading();
	Snapshot.ReloadProgress = GetReloadProgress();
	Snapshot.ReloadSecondsTotal = Weapon.ReloadSeconds;

	const UWorld* World = GetWorld();
	Snapshot.ReloadSecondsRemaining = Snapshot.bReloading && World != nullptr && ReloadEndSeconds >= 0.0
		? static_cast<float>(FMath::Max(0.0, ReloadEndSeconds - World->GetTimeSeconds()))
		: 0.0f;

	Snapshot.WeaponRowName = GetActiveWeaponName();
	Snapshot.WeaponDisplayName = Weapon.DisplayName;
	Snapshot.ActiveSlot = ActiveSlot;
	Snapshot.SlotCount = SlotRows.Num();
	Snapshot.FireMode = Weapon.FireMode;
	return Snapshot;
}

void UEclipseHitscanWeaponComponent::PublishWeaponStatus()
{
	if (!HasLocalPlayerConsumer())
	{
		return;
	}

	const EclipseWeaponStatusFeed::FEclipseWeaponStatusDecision Decision =
		StatusTracker.Submit(MakeStatusSnapshot());
	if (!Decision.bShouldBroadcast)
	{
		return;
	}

	// Geen bus (een test zonder GameInstance, of een component dat buiten een wereld
	// leeft): het feit is dan al door de tracker geteld en er gebeurt verder niets.
	// Stil degraderen, nooit crashen (GDD 14.3.5).
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	if (UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Broadcast(EclipseTags::Event_Player_WeaponStatusChanged, FInstancedStruct::Make(Decision.Payload));
	}
}

void UEclipseHitscanWeaponComponent::RequestStatusResend()
{
	// Vergeten WAT er het laatst uit ging, zodat het volgende monster gegarandeerd
	// een feit oplevert. ForgetLastBroadcast en niet Reset: de uitzendteller is een
	// meetinstrument en mag door een montage niet worden uitgegumd (zie
	// EclipseWeaponStatusFeed.h).
	StatusTracker.ForgetLastBroadcast();
	PublishWeaponStatus();
}

void UEclipseHitscanWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// De beurt kan op de klok voorbij zijn; SettleReloadIfElapsed sluit hem dan af
	// én zendt dat eind uit. Daarna is dit een gewone voortgangsstap — en die levert
	// alleen iets op als hij de drempel uit EclipseWeaponStatusFeed haalt.
	SettleReloadIfElapsed();
	PublishWeaponStatus();

	if (!bReloading)
	{
		// Het venster sluit zichzelf. Een tick die aan blijft staan omdat niemand
		// hem uitzet, is exact de vorm van defect 2 (een toestand die wacht op een
		// handeling die misschien nooit komt) — alleen dan met framekosten erbij.
		SetComponentTickEnabled(false);
	}
}

void UEclipseHitscanWeaponComponent::SpawnImpactMark(UWorld& World, const FHitResult& Hit)
{
	// HET ZICHTBARE SPOOR — stap 3 van owner-punt 4, en de reden dat de owner dacht
	// dat er niets gebeurde als hij schoot: elke MIS was onzichtbaar, en missen doe
	// je het vaakst.
	//
	// HET RECEPT STAAT NIET MEER HIER. Alles wat dit spoor VORM geeft — maat, ring,
	// vulling, opstaande kern, materialen — woont sinds 31-07 in
	// Combat/EclipseImpactMark.{h,cpp}. Dit component beslist alleen nog DAT er een
	// spoor komt en WAAR.
	//
	// Twee redenen, en de tweede is de belangrijkste:
	//
	//   1. ER WAREN TWEE BOUWERS. `AEclipseGameMode::OnWorldImpact` zette op elke
	//      wereldtreffer een tweede, exact samenvallend spoor neer met een ander
	//      materiaal en zonder rotatie: 22 objecten bij 11 treffers. Die is weg; de
	//      toelichting staat bij de subscriptie in de game mode.
	//   2. HET MEETHARNAS MOET HETZELFDE DING METEN als het spel toont. Zolang het
	//      recept in dit bestand stond, kon `EclipseRenderProof` het alleen NABOUWEN
	//      — en een meting aan een nagebouwd object zegt niets over het verscheepte
	//      (`meten-voor-je-concludeert`: authored is niet verscheept).
	//
	// De uitgebreide vaststellingen van 27-07 en 31-07 (twaalf uitgesloten oorzaken,
	// de transform-meting, de differentiële pixeltelling) staan bewust NIET meer in
	// dit bestand maar in phase0/DEBUG_DISCIPLINE.md §4.3. Een tweede exemplaar van
	// een meting veroudert los van het origineel, en dit bestand had daar een blok
	// van tachtig regels vol van.
	AStaticMeshActor* Mark = EclipseImpactMark::Spawn(
		World, Hit.ImpactPoint, Hit.ImpactNormal, EclipseImpactMark::Verscheept());
	if (Mark == nullptr)
	{
		// Spawn heeft zelf al gelogd WAAROM. Hier alleen de gevolgtrekking, want een
		// stille return is precies de vorm waar dit dossier maanden op is blijven
		// hangen: aannemen dat de bron werkt en alleen aan de uitvoer meten.
		UE_LOG(LogEclipse, Warning,
			TEXT("Inslagspoor: er ontstond GEEN spoor voor de treffer op %s."),
			*Hit.ImpactPoint.ToCompactString());
		return;
	}
	++ImpactMarksSpawned;

	// DE DRIE GETALLEN DIE HIER NOOIT STONDEN, en zonder welke elke conclusie in
	// §4.3 een gok blijft.
	//
	// De regel hieronder logde tot 31-07 alleen de bedoelde plek — de plek waar het
	// spoor NAARTOE ging. Waar het daarna werkelijk staat was in dit dossier nooit
	// gemeten, en juist dat was de vraag: de controleproef die "transform-bug, geen
	// rendering-bug" moest dragen, was een blok dat PER CONSTRUCTIE aan het
	// personage vastzat en daar dus per definitie stond. Zo'n proef kan over
	// gespawnde sporen niets zeggen.
	//
	//   ECHT      = Mark->GetActorLocation() ná het spawnen. Wijkt hij van `bedoeld`
	//               af, dan zit de fout in de spawn/transform-keten.
	//   schutter  = waar de eigenaar van dit wapen staat. Landt het spoor daar,
	//               dan is het een lokale-ruimte- of attach-fout.
	//   geraakt   = Hit.GetActor(), zodat een spoor herleidbaar is naar het
	//               oppervlak dat de trace vond.
	//
	// De twee AFSTANDEN staan er los bij, want dit project heeft eerder een
	// wereldcoördinaat voor een afstand aangezien (zie de wereldtrefferregel in
	// Fire) en twee coördinaten naast elkaar leggen is precies waar dat misgaat.
	const AActor* ShooterActor = GetOwner();
	const FVector ShooterSpot = ShooterActor != nullptr ? ShooterActor->GetActorLocation() : FVector::ZeroVector;
	const FVector Landed = Mark->GetActorLocation();
	const FVector Bedoeld = Hit.ImpactPoint + Hit.ImpactNormal.GetSafeNormal() * 1.0f;
	UE_LOG(LogEclipse, Display,
		TEXT("Inslagspoor %d PLEK: bedoeld %s, ECHT %s (verschil %.2f cm), inslagpunt %s (%.2f cm), schutter %s (%.1f cm, %s), geraakt %s."),
		ImpactMarksSpawned, *Bedoeld.ToCompactString(), *Landed.ToCompactString(),
		FVector::Dist(Landed, Bedoeld), *Hit.ImpactPoint.ToCompactString(),
		FVector::Dist(Landed, Hit.ImpactPoint), *ShooterSpot.ToCompactString(),
		FVector::Dist(Landed, ShooterSpot), ShooterActor != nullptr ? TEXT("eigenaar bekend") : TEXT("GEEN eigenaar"),
		*GetNameSafe(Hit.GetActor()));

	// DE MAAT, en niet meer de mesh-omvang van één plaat.
	//
	// Hier stond de bolstraal van de enige component, en dat getal is met het
	// samengestelde spoor misleidend geworden: het zou de grootste van de drie
	// delen noemen en de andere twee verzwijgen. De bounding box van de hele actor
	// is wat de camera ziet, en dat is precies het getal dat naast de pixeltelling
	// van EclipseRenderProof hoort te liggen.
	const FBox Omvang = Mark->GetComponentsBoundingBox(true);
	const FVector Maat = Omvang.GetSize();
	UE_LOG(LogEclipse, Display,
		TEXT("Inslagspoor %d GESPAWND op %s (omvang %.1f x %.1f x %.1f cm, %d delen, zichtbaar-in-spel %d)"),
		ImpactMarksSpawned, *Landed.ToCompactString(), Maat.X, Maat.Y, Maat.Z,
		Mark->GetComponents().Num(), Mark->IsHidden() ? 0 : 1);
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
	NotifyActiveWeaponChanged();
	// De EERSTE foto van dit wapen, zodat de teller niet op nul staat tot het eerste
	// schot. Via ApplyLoadout komt hier een tweede feit achteraan (dat pad vult de
	// rijnamen en het tweede slot pas ná deze regel); dat is één frame en het tweede
	// feit is degene die de HUD echt tekent.
	PublishWeaponStatus();
}

void UEclipseHitscanWeaponComponent::NotifyActiveWeaponChanged()
{
	// AAN HET FEIT, NIET AAN DE KNOP (owner-regel "hang gedrag aan het feit").
	//
	// Dit is de reparatie van owner-punt 5, en de vorm ervan telt: het zichtbare
	// wapen hangt aan "het actieve wapen is veranderd" en niet aan de RB-binding.
	// Daardoor verandert ELK pad dat het wapen wisselt ook wat je ziet — de
	// speler, ApplyLoadout bij de missiestart, een debugcommando, en straks de AI.
	// Een pad-tabel is incompleet zodra er een pad bij komt; dit feit niet.
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetOwner()))
	{
		Body->RefreshWeaponVisual();
	}
}

void UEclipseHitscanWeaponComponent::ApplyLoadout(const FEclipseWeaponRow& Primary, FName PrimaryName,
	const FEclipseWeaponRow& Sidearm, FName SidearmName)
{
	ApplyWeaponRow(Primary);
	SlotNames[0] = PrimaryName;
	SlotRows.Add(Sidearm);
	SlotAmmo.Add(Sidearm.MagazineSize);
	SlotNames.Add(SidearmName);
	// ApplyWeaponRow hierboven riep dit al aan, maar toen heette slot 0 nog
	// NAME_None — en zonder rijnaam is er geen mesh te vinden. Opnieuw, nu de
	// namen er zijn.
	NotifyActiveWeaponChanged();
	PublishWeaponStatus();
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

	// EN NU VERANDERT ER OOK IETS AAN WAT JE ZIET (O-5 "volledig", 31-07).
	//
	// Dit is de stap waar het hele dossier op uitkomt. De wapenlaag was als DATA
	// al compleet — twee slots, elk met een eigen magazijn — maar de wissel deed
	// visueel NIETS, want er was niets om te wisselen zolang het wapen deel van de
	// karaktermesh was. Deze regel is de proef op de som.
	NotifyActiveWeaponChanged();

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

	// EN DE STAND ERACHTERAAN, want WeaponSwapped hierboven zegt WELKE FAMILIE er
	// nu in de handen ligt en hoe lang het optillen duurt — niet hoeveel er in zit.
	// Dat is precies het gat waar dit werk over gaat: er waren drie combat-feiten en
	// geen daarvan droeg de stand. Eén feit, want de tracker ziet één verandering:
	// ander slot, andere rijnaam, ander magazijn.
	PublishWeaponStatus();
	return true;
}

bool UEclipseHitscanWeaponComponent::StartReload(FName Cause)
{
	// Cause is diagnostiek: "PlayerReload" of "MagazineEmpty". Het zegt WAAROM er
	// herladen wordt, en dat is het verschil tussen een speler die vooruitdenkt en
	// een die droogloopt — precies wat je wilt zien in een log van een speelronde.
	UWorld* World = GetWorld();
	if (World == nullptr || Weapon.MagazineSize <= 0)
	{
		return false;
	}
	// EERST DE VORIGE BEURT AFSLUITEN als de klok er al voorbij is. Zonder deze
	// regel zou een speler die één beurt lang niet schoot, daarna NOOIT meer kunnen
	// herladen: `bReloading` stond nog aan en de test hieronder wees hem af.
	SettleReloadIfElapsed();
	if (bReloading)
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

	// DE START VAN DE BEURT ALS STAND-FEIT, en het venster waarin de voortgang
	// gemeten wordt. De tick gaat alleen aan als er ook echt iemand naar kijkt: voor
	// een vijand die herlaadt is er geen consument en dus geen reden om te tikken.
	PublishWeaponStatus();
	if (HasLocalPlayerConsumer())
	{
		SetComponentTickEnabled(true);
	}
	return true;
}

void UEclipseHitscanWeaponComponent::FinishReload()
{
	bReloading = false;
	ReloadEndSeconds = -1.0;
	AmmoInMagazine = Weapon.MagazineSize;
}

bool UEclipseHitscanWeaponComponent::IsReloading() const
{
	if (!bReloading)
	{
		return false;
	}
	const UWorld* World = GetWorld();
	if (World == nullptr || ReloadEndSeconds < 0.0)
	{
		// Geen wereld = geen klok. Dan is de vlag het enige wat er is, en die
		// zeggen we eerlijk terug in plaats van te gokken dat het wel klaar zal zijn.
		return true;
	}
	return World->GetTimeSeconds() < ReloadEndSeconds;
}

int32 UEclipseHitscanWeaponComponent::GetAmmoInMagazine() const
{
	// Een beurt die op de klok voorbij is, HEEFT het magazijn gevuld — ook als er nog
	// geen enkel pad langs is gekomen om dat op te schrijven. Zie IsReloading() voor
	// waarom de waarheid hier uitgerekend wordt in plaats van onthouden.
	if (bReloading && !IsReloading())
	{
		return Weapon.MagazineSize;
	}
	return AmmoInMagazine;
}

float UEclipseHitscanWeaponComponent::GetReloadProgress() const
{
	if (!IsReloading() || Weapon.ReloadSeconds <= 0.0f)
	{
		return 0.0f;
	}
	const UWorld* World = GetWorld();
	if (World == nullptr || ReloadEndSeconds < 0.0)
	{
		return 0.0f;
	}
	const double Remaining = ReloadEndSeconds - World->GetTimeSeconds();
	return static_cast<float>(FMath::Clamp(1.0 - Remaining / Weapon.ReloadSeconds, 0.0, 1.0));
}

float UEclipseHitscanWeaponComponent::GetCurrentSpreadDegrees() const
{
	// EERSTE SCHOT IS ALTIJD ZUIVER, en de reeks BREEKT op de klok.
	//
	// Beide regels stonden in Fire() en zijn hierheen verplaatst zodat het kruis
	// dezelfde waarheid leest als de kogel. De klokregel is de belangrijkste van de
	// twee voor de schermlaag: `ConsecutiveShots` wordt pas op de VOLGENDE
	// trekkerbeweging teruggezet, dus wie alleen die teller leest, ziet een wapen
	// dat eeuwig "midden in een reeks" staat nadat de speler is gestopt. Dat is
	// exact dezelfde vorm als het herladen dat nooit eindigde — een toestand die
	// wacht op een handeling die misschien nooit komt.
	const UWorld* World = GetWorld();
	const double Now = World != nullptr ? World->GetTimeSeconds() : 0.0;
	const bool bSeriesBroken = LastFireTimeSeconds < 0.0
		|| (World != nullptr && Now - LastFireTimeSeconds > Weapon.FireInterval * 3.0);
	if (ConsecutiveShots <= 0 || bSeriesBroken)
	{
		return 0.0f;
	}

	float SpreadDegrees = ShooterBodyIsAiming() ? Weapon.AimSpreadDegrees : Weapon.HipSpreadDegrees;

	// Bewegen straft, en per wapen anders: de SMG heeft 0,8 graden en de DMR 4,0.
	// Dat is wat "waardeloos in beweging" in data betekent.
	if (const APawn* ShooterPawn = Cast<APawn>(GetOwner()))
	{
		const float Speed = ShooterPawn->GetVelocity().Size2D();
		// Op de loopsnelheid geschaald, niet op de sprint: wie wandelt hoort
		// nauwelijks straf te voelen, wie rent de volle.
		const float MoveFraction = FMath::Clamp(Speed / 420.0f, 0.0f, 1.0f);
		SpreadDegrees += Weapon.MovingSpreadDegrees * MoveFraction;
	}
	return SpreadDegrees;
}

void UEclipseHitscanWeaponComponent::SettleReloadIfElapsed()
{
	if (bReloading && !IsReloading())
	{
		FinishReload();
		// HET EIND VAN DE BEURT HANGT HIER EN NIET AAN DE TICK, om dezelfde reden
		// dat het zichtbare wapen aan NotifyActiveWeaponChanged hangt en niet aan de
		// RB-binding ("hang gedrag aan het feit"): élk pad dat een beurt afsluit
		// meldt hem nu. De tick is er één van, Fire() en StartReload() zijn de
		// andere twee, en er komt er vanzelf een bij zonder dat iemand deze regel
		// hoeft te onthouden.
		PublishWeaponStatus();
	}
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
	//
	// DIT WAS TOT 31-07 DE ENIGE PLEK waar een beurt kon eindigen, en dat was defect
	// 2: wie na het herladen niet meer schoot, bleef eeuwig herladen. De klokregel
	// staat nu in SettleReloadIfElapsed en in de getters, zodat elke lezer hem krijgt
	// en niet alleen de trekker.
	SettleReloadIfElapsed();
	if (bReloading)
	{
		return false; // je handen zitten aan het magazijn
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

	// EEN SCHOT IS EEN KOGEL MINDER, ONGEACHT WAT HIJ RAAKT.
	//
	// Hier en niet onderaan Fire(), om precies dezelfde reden als het ShotFired-feit
	// hieronder: onder de trace staan drie `return false`-takken (niets geraakt, de
	// wereld geraakt, en de personage-tak eronder), en in twee daarvan zou de teller
	// dus nooit vertrekken. Dan zou de HUD alleen aftellen bij RAKE schoten — en
	// missen doe je het vaakst.
	PublishWeaponStatus();

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
	// DE FORMULE STAAT IN GetCurrentSpreadDegrees en nergens anders — zie de header
	// voor waarom een tweede plek hier een bekende, dure fout van dit project is.
	// Het richtkruis leest exact deze functie, dus wat je ziet is wat de kogel doet.
	const float SpreadDegrees = GetCurrentSpreadDegrees();
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
		// DE TWEEDE STILLE AFWIJZING, en die had ik vanochtend gemist.
		//
		// Toen repareerde ik de tak "raakt wel iets, maar geen personage" en noemde
		// dat 'de misser bestaat nu'. Onvolledig: HIER keert een schot terug dat
		// HELEMAAL NIETS raakte - de lucht in, of over alles heen - en dat gebeurde
		// net zo stil. Gevonden doordat de opnameronde WERELDTREFFERS=0 bleef melden
		// terwijl er elf schoten vielen; zonder dat getal had ik het niet gezien en
		// had "de misser is hoorbaar" ongetoetst blijven staan.
		//
		// Alleen tellen, geen feit op de bus: een schot in het niets IS een uitkomst
		// voor de speler (hij hoort zijn wapen, hij ziet geen inslag) maar er is geen
		// plek om iets aan te hangen. Het getal maakt het verschil zichtbaar tussen
		// "er gebeurde niets" en "er gebeurde iets dat ik niet toon".
		++CleanMisses;
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
		UE_LOG(LogEclipse, Display,
			TEXT("Wapen: wereldtreffer op %s (oppervlak %s) op %s, %.0f cm ver — %d deze missie."),
			*GetNameSafe(Hit.GetActor()),
			Surface != nullptr ? *Surface->GetName() : TEXT("onbekend"),
			*Hit.ImpactPoint.ToCompactString(),
			// DE AFSTAND EN NIET ALLEEN DE PLEK. Ik las hierboven een WERELDCOORDINAAT
			// (X=-7900) als een afstand en schreef 'de inslagen landen op 79 meter'.
			// Dat is precies de fout die dit project telkens maakt: een getal aflezen
			// zonder te weten waarin het staat. Nu staat de afstand er los bij.
			FVector::Dist(Hit.ImpactPoint, ViewLocation), WorldHits);

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
			SpawnImpactMark(*BusWorld, Hit);
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
