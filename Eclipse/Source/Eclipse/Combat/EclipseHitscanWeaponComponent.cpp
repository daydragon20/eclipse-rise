#include "Combat/EclipseHitscanWeaponComponent.h"

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipsePlayerController.h"
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
	PrimaryComponentTick.bCanEverTick = false; // event-driven (GDD 14.2)
}

void UEclipseHitscanWeaponComponent::SpawnImpactMark(UWorld& World, const FHitResult& Hit)
{
	// HET ZICHTBARE SPOOR — stap 3 van owner-punt 4, en de reden dat hij dacht dat
	// er niets gebeurde als hij schoot: elke MIS was onzichtbaar, en missen doe je
	// het vaakst.
	//
	// WAAROM EEN QUAD EN GEEN DECAL. De wijk staat unlit, en een deferred decal
	// heeft een G-buffer nodig die er dan niet is: hij rendert simpelweg niet. Dat
	// is eerder in dit project vastgesteld en het is de reden dat ook de
	// grondcues van de grayboxbouwer platte kubussen zijn en geen decals. Zelfde
	// recept dus, alleen op de inslagplek in plaats van op de vloer.
	//
	// WAAROM HIER EN NIET IN EEN VFX-SUBSYSTEEM. De owner-opdracht is expliciet
	// "geen nieuwe systemen". Een eigen verbruiker van de bus zou een nieuw
	// subsysteem betekenen voor precies een gebruiker; dat mag terugkomen zodra er
	// een TWEEDE visuele verbruiker is (muzzle flash staat al op de rol) — dan is
	// het extraheren en niet vooruit bouwen.
	// EERST MELDEN OF HET DING ER KOMT, en dat had hier vanaf het begin moeten staan.
	//
	// Ik heb dit spoor drie keer proberen zichtbaar te maken en telkens iets anders
	// uitgesloten: de maat (een proef op 90 cm), de plaats (7-9 m recht vooruit),
	// de levensduur (2,5 s) en het materiaal. Wat ik NOOIT heb gecontroleerd is of
	// de actor uberhaupt ontstaat — precies de fout waar ik vandaag vijf keer op
	// heb gewezen bij anderen: aannemen dat de bron werkt en alleen aan de uitvoer
	// meten. Een stille `return` bij een mislukte laadpoging is dezelfde vorm als de
	// twee stille afwijzingen in dit bestand.
	UStaticMesh* Quad = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Quad == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("Inslagspoor: /Engine/BasicShapes/Cube.Cube laadde NIET — geen spoor mogelijk."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;   // een inslag hoort nooit in een save

	// Een centimeter langs de normaal, anders vecht het vlak met het oppervlak
	// waar het op ligt en flikkert het per frame.
	const FVector Spot = Hit.ImpactPoint + Hit.ImpactNormal * 1.0f;
	AStaticMeshActor* Mark = World.SpawnActor<AStaticMeshActor>(
		Spot, FRotationMatrix::MakeFromZ(Hit.ImpactNormal).Rotator(), Params);
	if (Mark == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Inslagspoor: SpawnActor gaf niets terug op %s."),
			*Spot.ToCompactString());
		return;
	}

	Mark->SetMobility(EComponentMobility::Movable);
	UStaticMeshComponent* Plate = Mark->GetStaticMeshComponent();
	Plate->SetStaticMesh(Quad);
	// 9 cm in het vierkant, 0,4 mm dik: groot genoeg om op 20 m te lezen, plat
	// genoeg om als spoor te lezen en niet als blokje.
	Mark->SetActorScale3D(FVector(0.09f, 0.09f, 0.004f));
	Plate->SetCastShadow(false);
	Plate->SetAffectDistanceFieldLighting(false);
	Mark->SetActorEnableCollision(false);

	// HET RECEPT VAN DE GRONDVLAKKEN DIE WEL RENDEREN — de laatste ongeteste
	// verdachte, en de enige die ik over het hoofd had gezien.
	//
	// Ik heb dit spoor met M_EclipseToon geprobeerd, met het engine-standaard, en
	// met EmissiveMeshMaterial: alle drie onzichtbaar. Maar de lichtplekken en
	// contactschaduwen van de grayboxbouwer, die aantoonbaar WEL op deze vloer
	// staan, gebruiken geen van die drie: zij nemen M_EclipseToonDecal MET EEN
	// MASKER. Dat masker is daar niet optioneel — de bouwer weigert zelfs een vlak
	// te maken als het ontbreekt, en dat is precies het soort hint waar ik
	// overheen las omdat ik naar het materiaal keek en niet naar de VOORWAARDE.
	//
	// De blob is hier de juiste vorm: een zachte ronde vlek is wat een inslag op
	// beton achterlaat, en hij bestaat al.
	UTexture* Masker = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Decals/T_blob_mask.T_blob_mask"));
	UMaterialInterface* DecalMaster = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/Art/M_EclipseToonDecal.M_EclipseToonDecal"));
	if (Masker == nullptr || DecalMaster == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("Inslagspoor: decal-master of masker ontbreekt (master %s, masker %s) — spoor blijft onzichtbaar."),
			DecalMaster != nullptr ? TEXT("ok") : TEXT("weg"),
			Masker != nullptr ? TEXT("ok") : TEXT("weg"));
	}
	if (UMaterialInterface* Toon = DecalMaster)
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Toon, Mark))
		{
			// Warm en helder, want in een unlit wijk is een inslag die zijn
			// helderheid van het licht moet halen geen inslag.
			const FLinearColor Spark(1.00f, 0.62f, 0.22f, 1.0f);
			Mid->SetVectorParameterValue(TEXT("LitColor"), Spark);
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), Spark * 0.45f);
			// LIGHTDIR — de enige parameter die ik nooit had gezet, en elke andere
			// gebruiker van dit master zet hem wel (de lichamen op regel 1035 van
			// EclipseCharacter.cpp, met dezelfde zonrichting). Een toon-master mengt
			// zijn licht- en schaduwkleur op N·L; met een LightDir van nul is die
			// term nul en valt er niets te mengen. Dat past op alle bewijs tot nu toe:
			// de spawn lukt, zichtbaar=1, hij staat aantoonbaar in het kader
			// (scherm 589,499 — inbeeld=1, 8,6 m) en er komt niets uit de shader.
			const FVector SunTravel = FRotator(-25.0f, 55.0f, 0.0f).Vector();
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunTravel, 0.0f)));
			Mid->SetScalarParameterValue(TEXT("EmissiveScale"), 10.0f);
			// UVMode EN een albedo, want zonder die twee bleef het spoor onzichtbaar.
			// GEMETEN: met een proefspoor van 90 cm - tien keer de echte maat, op 7 tot
			// 9 m voor de speler en dus vol in beeld - was er nog steeds niets te zien.
			// Dat sluit maat en plaats uit en laat het materiaal over: elke andere
			// gebruiker van dit master (de lichamen, de grayboxblokken) zet een textuur
			// en een UV-modus, mijn spoor als enige niet. Een master die zijn kleur
			// door een ontbrekende albedo vermenigvuldigt, levert zwart op zwart asfalt.
			Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
			// Het masker rijdt op MeshUV: een vlak van de engine-kubus spant 0..1.
			Mid->SetTextureParameterValue(TEXT("MaskTex"), Masker);
			Mid->SetScalarParameterValue(TEXT("OpacityScale"), 1.0f);
			if (UTexture* White = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")))
			{
				Mid->SetTextureParameterValue(TEXT("AlbedoTex"), White);
			}
			Plate->SetMaterial(0, Mid);
		}
	}

	// Kort: dit is een treffer-bevestiging en geen kogelgat dat blijft liggen. Met
	// SetLifeSpan ruimt de engine hem zelf op, dus er is geen pool en geen teller
	// die kan blijven hangen — bij tien schoten per seconde leven er hooguit een
	// stuk of acht tegelijk.
	// 2,5 s en niet 0,8. De eerste keus was 0,8 en dat bleek om twee redenen te
	// kort: op de opname van 12:56 stonden elf gemeten wereldtreffers en was er GEEN
	// spoor te zien, want de opname valt na het vuurinterval en bij frames van 0,4 s
	// is 0,8 s twee frames. En los daarvan: een kogelspoor dat binnen een seconde
	// weg is, is precies de klacht die dit moet oplossen — je kijkt na een salvo naar
	// de muur om te zien waar je zat.
	// ==================================================================
	// STAND 27-07: DIT SPOOR IS ONZICHTBAAR EN IK WEET NIET WAAROM.
	//
	// Alles hieronder is GEMETEN, niet beredeneerd, en sluit telkens een oorzaak
	// uit. De regel [PLAYSHOT n SPOREN] in de opnameronde projecteert de levende
	// sporen naar schermcoordinaten; die maakt van "ik zie het niet" een getal.
	//
	//   ontstaat de actor        JA  - 11 stuks, zichtbaar=1, niet verborgen
	//   heeft hij een mesh       JA  - MESH Cube, bolstraal 6,4, geregistreerd=1
	//   staat hij in het kader   JA  - scherm (589,502), inbeeld=1, op 8,4 m
	//   ligt het aan de maat     NEE - 90 cm (~100 px breed) is even onzichtbaar
	//   aan de levensduur        NEE - 2,5 s, en er leven er 11 op het opnamemoment
	//   aan de kleur             NEE - knalwit met honderdvoudige emissie: niets
	//   aan MIJN materiaal       NEE - ook met de engine-standaard: niets
	//   aan de spawnwijze        NEE - identiek aan de districtblokken die WEL renderen
	//   aan het MOMENT           NEE - een kanarie bij BeginPlay, op open weg, 90 cm,
	//                                  geprojecteerd in beeld: even onzichtbaar
	//   aan de dikte             NEE - ook op de Z-schaal 0,04 van de grondvlakken
	//                                  van de bouwer (10x dikker dan mijn 0,004)
	//   aan MIJN materiaalkeuze  NEE - ook met EmissiveMeshMaterial, dat niet van
	//                                  licht afhangt en geen masker heeft
	//   aan het decal-recept     NEE - ook met M_EclipseToonDecal + T_blob_mask +
	//                                  OpacityScale, exact het recept van de
	//                                  grondvlakken van de grayboxbouwer
	//
	// EN TOEN DE CONTROLEPROEF DIE ALLES OMDRAAIDE. Een magenta blok van 50 cm,
	// VASTGEMAAKT AAN HET PERSONAGE en neergezet bij BeginPlay, staat groot en
	// helder in elk frame. Spawnen, materiaal, toon-master en renderen werken dus
	// gewoon — en dat had ik na twaalf uitsluitingen nog niet vastgesteld.
	//
	// Daarna hetzelfde blok als INSLAGSPOOR: zelfde mesh, zelfde materiaal, zelfde
	// maat, geen levensduur, geen RF_Transient. Onzichtbaar. Ook zonder RF_Transient.
	// De sporen staan aantoonbaar VOOR de camera (dot-product) op 8,5 m, met
	// straal 63,7, geregistreerd en zichtbaar.
	//
	// HET ENIGE VERSCHIL DAT OVERBLIJFT IS WIE ZE NEERZET: dit component tijdens
	// het vuren, tegen de game mode bij het starten. Dat is waar de volgende sessie
	// hoort te beginnen — bijvoorbeeld door hetzelfde spoor vanuit de game mode op
	// een timer neer te zetten. Niet meer gokken aan materiaal, maat of vorm: die
	// zijn alle drie uitgesloten door een blok dat WEL verschijnt.
	//
	// TWEE VAN DIE UITSLUITINGEN ZIJN ZWAKKER DAN ZE LIJKEN, en dat hoort erbij:
	// de kanarie-proeven leunden op "inbeeld=1", en dat zegt NIETS over wat er voor
	// het vlak staat. De eerste kanarie bleek achter een pilaar te vallen; een
	// poging om de grondvlakken van de bouwer te controleren viel achter een muur.
	//
	// EN DE AANNAME ERONDER IS NOOIT GETOETST: ik ging ervan uit dat de
	// lichtplekken en contactschaduwen van de bouwer WEL renderen. Dat heb ik nooit
	// gezien, alleen aangenomen omdat de wijk aangekleed oogt. Er staan er 38 in de
	// wereld. Is die laag ook onzichtbaar, dan is dit geen defect van mijn spoor
	// maar van alles wat plat op de grond ligt — en dat is een veel groter verhaal.
	//
	// DAT IS DE VRAAG WAARMEE DE VOLGENDE SESSIE HOORT TE BEGINNEN, en hij is in de
	// editor in tien seconden te beantwoorden: zet de wijk stil en kijk of er onder
	// een lantaarnpaal een lichtplek ligt.
	//
	// Wat dus overblijft ligt buiten wat dit harnas kan zien. Volgende stap is een
	// ander gereedschap dan de opnameronde: in de editor kijken, of de renderlagen
	// uitzetten tot hij verschijnt. NIET nog een parameter gokken - dat is vandaag
	// zeven keer gedaan en heeft zeven keer niets opgeleverd.
	//
	// STAND 31-07 - DE PLEK IS UITGESLOTEN, MET EEN METING.
	//
	// Alles hierboven blijft staan als geschiedenis, maar twee regels erin zijn
	// achterhaald. "HET ENIGE VERSCHIL DAT OVERBLIJFT IS WIE ZE NEERZET" is langs
	// twee kanten weerlegd: de game mode zet er via OnWorldImpact al een TWEEDE neer
	// met een ander materiaal en een andere rotatie, en die is even onzichtbaar. En
	// de plek zelf is geen verdachte meer:
	// Eclipse.Combat.ImpactMarkLandsOnTheHitAndNotOnTheShooter loopt twintig
	// gevarieerde schoten headless na en vindt het spoor elke keer OP de inslagplek
	// en nooit bij de schutter - met een controleproef die aantoont dat die meting
	// ook rood kan worden.
	//
	// De getallen staan hier niet, en dat is opzet: ze staan in
	// phase0/DEBUG_DISCIPLINE.md §4.3. Een tweede exemplaar van een meting veroudert
	// los van het origineel, en dit bestand heeft daar al een blok vol van.
	// ==================================================================
	Mark->SetLifeSpan(2.5f);
	Mark->Tags.Add(TEXT("Eclipse_ImpactMark"));
	++ImpactMarksSpawned;

	// DE DRIE GETALLEN DIE HIER NOOIT STONDEN, en zonder welke elke conclusie in
	// §4.3 een gok blijft.
	//
	// De regel hieronder logde tot 31-07 alleen `Spot` — de plek waar ik het spoor
	// NAARTOE stuurde. Waar het daarna werkelijk staat is in dit dossier nog nooit
	// gemeten, en juist dat is de vraag: de controleproef die "transform-bug, geen
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
	UE_LOG(LogEclipse, Display,
		TEXT("Inslagspoor %d PLEK: bedoeld %s, ECHT %s (verschil %.2f cm), inslagpunt %s (%.2f cm), schutter %s (%.1f cm, %s), geraakt %s."),
		ImpactMarksSpawned, *Spot.ToCompactString(), *Landed.ToCompactString(),
		FVector::Dist(Landed, Spot), *Hit.ImpactPoint.ToCompactString(),
		FVector::Dist(Landed, Hit.ImpactPoint), *ShooterSpot.ToCompactString(),
		FVector::Dist(Landed, ShooterSpot), ShooterActor != nullptr ? TEXT("eigenaar bekend") : TEXT("GEEN eigenaar"),
		*GetNameSafe(Hit.GetActor()));

	UE_LOG(LogEclipse, Display,
		TEXT("Inslagspoor %d GESPAWND op %s (schaal %s, zichtbaar %d, materiaal %s, MESH %s, straal %.1f, geregistreerd %d, zichtbaar-in-spel %d)"),
		ImpactMarksSpawned, *Spot.ToCompactString(), *Mark->GetActorScale3D().ToCompactString(),
		Plate->IsVisible() ? 1 : 0, *GetNameSafe(Plate->GetMaterial(0)),
		// DE MESH ZELF, en die had hier vanaf het begin moeten staan. Materiaal,
		// maat, plaats, levensduur en kleur zijn allemaal uitgesloten met een
		// meting; het enige dat ik nooit heb gecontroleerd is of er echt een mesh
		// aan hangt en of hij een omvang heeft. Een bolstraal van nul betekent dat
		// er niets te tekenen valt, hoe zichtbaar de component ook zegt te zijn.
		*GetNameSafe(Plate->GetStaticMesh()), Plate->Bounds.SphereRadius,
		Plate->IsRegistered() ? 1 : 0, Mark->IsHidden() ? 0 : 1);
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
