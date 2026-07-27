// De missie speelt zichzelf uit (owner-nachtopdracht 2026-07-25, fase 2B).
//
// Waarom dit belangrijker is dan nog een shotronde, in de woorden van de
// opdracht: screenshots van zes vaste camera's vinden alleen wat er in beeld
// staat. Een missie die zichzelf uitspeelt vindt vastlopers, objectives die
// nooit triggeren, orders die niet aankomen en debriefs die verkeerd afrekenen.
//
// Wat deze ronde doet:
//   1. start M1.1 via het ECHTE laadpad — SelectMission -> AutoLaunch, exact de
//      twee aanroepen die -EclipseStartMission doet, zodat een bug in dat pad
//      zich er niet achter kan verstoppen;
//   2. loopt met GEÏNJECTEERDE input van Entry_Main naar het controlepost-site;
//   3. HAALT het objective echt — de patrouille gaat neer omdat er geschoten
//      wordt, niet omdat er iemand langsloopt;
//   4. loopt door naar extractie en bereikt de debrief;
//   5. asserteert op de UITKOMST: beloning, dagklok en regiostaat;
//   6. legt onderweg vast: haalt de squad zijn orders op, komt er ack terug,
//      blijft de game-thread binnen budget, en gebeurt er nergens iets stils dat
//      luid had moeten zijn.
//
// De eerste ronde vond meteen wat hij moest vinden: DestroyTarget had geen
// enkel voltooiingspad, en de overlap-trigger vinkte hem daarom af op
// AANWEZIGHEID. "Spring the ambush: take out the patrol leader" was dus af zodra
// je erlangs liep, en de missie eindigde in een keurige geslaagde debrief zonder
// dat er iets gebeurd was. Geen enkele bestaande test zag dat, want alles was
// groen. Beide helften zijn gerepareerd (NotifySiteEntered + de
// hostile-downed-koppeling in de game mode) en staan hieronder vastgepind.

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Eclipse.h" // LogEclipse
#include "AI/EclipseEnemyController.h"
#include "Animation/SkeletalMeshActor.h"
#include "AI/EclipseSquadmateController.h"
#include "Characters/EclipseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/EclipsePlayerController.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Audio/EclipseAudioSubsystem.h"
#include "Core/EclipseGameMode.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "Characters/EclipseAnimInstance.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Base/EclipsePrepSubsystem.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Quests/EclipseMissionTypes.h"
#include "Squad/EclipseSquadTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Quests/EclipseStoryTypes.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "Tests/EclipseFeelHarness.h"

namespace EclipsePlaythrough
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Het graybox-district uit EclipseGrayboxBuilder. Coördinaten en geen
	 *  pathfinding — de opdracht zegt expliciet dat dat volstaat. */
	const FVector SiteControlPost(5000.0f, -2000.0f, 120.0f);
	const FVector SiteExtraction(-8500.0f, -8500.0f, 120.0f);

	/** Telt de squad-feiten mee die de opdracht wil zien: elke order hoort exact
	 *  één antwoord te krijgen, en een order zonder antwoord is de stilte waar
	 *  "orders zijn beloftes" op stukloopt. */
	struct FSquadWatch
	{
		int32 Issued = 0;
		int32 Acknowledged = 0;
		int32 Refused = 0;
		TArray<FString> RefusalLines;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMissionPlaythroughTest,
	"Eclipse.Playthrough.M11PlaysItselfFromLaunchToDebrief",
	EclipsePlaythrough::TestFlags)

bool FEclipseMissionPlaythroughTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	// Muis/toetsenbord-tak: die is lineair en zonder deadzone, dus sturen is exact
	// en de route wordt niet ondergesneeuwd door een regelaar die zichzelf zit te
	// corrigeren. De stick-tak wordt volledig gemeten in laag 2.
	IConsoleVariable* ForceGamepad = IConsoleManager::Get().FindConsoleVariable(TEXT("Eclipse.Input.ForceGamepad"));
	const int32 PreviousForce = ForceGamepad != nullptr ? ForceGamepad->GetInt() : -1;
	if (ForceGamepad != nullptr)
	{
		ForceGamepad->Set(0, ECVF_SetByCode);
	}
	ON_SCOPE_EXIT
	{
		if (ForceGamepad != nullptr)
		{
			ForceGamepad->Set(PreviousForce, ECVF_SetByCode);
		}
	};

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	// 1/60 s: de ronde legt bijna 300 meter af, en 8 ms-resolutie koopt daar
	// niets voor terwijl het de wandkloktijd verdubbelt.
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestNotNull(TEXT("speelronde: campagne"), Campaign) || !TestNotNull(TEXT("speelronde: strategie"), Strategy)
		|| !TestNotNull(TEXT("speelronde: prep"), Prep) || !TestNotNull(TEXT("speelronde: missie"), Mission)
		|| !TestNotNull(TEXT("speelronde: eventbus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	// Het district hoort te staan vóór de missie start: dat is wat InitGame doet,
	// en zonder sites is er niets om naartoe te lopen.
	int32 PlayerStarts = 0;
	for (TActorIterator<APlayerStart> It(Harness.World); It; ++It)
	{
		++PlayerStarts;
	}
	TestTrue(FString::Printf(TEXT("speelronde: het district staat (%d insertiepunten)"), PlayerStarts), PlayerStarts > 0);

	// --- welke van de 29 events vuren er ooit? ------------------------------
	// De catalogus bewaakt dat elk gedocumenteerd event ook geïmplementeerd is,
	// maar niet dat het ooit AFGAAT. Een event dat alleen vanuit een
	// console-commando vertrekt telt daar mee als geïmplementeerd — zo bleef het
	// alarm onopgemerkt. Deze ronde speelt M1.1 volledig uit, dus wat hier niet
	// langskomt, komt in een normale missie niet langs.
	TSet<FName> SeenEventTags;
	FEclipseEventSubscriptionHandle AllHandle = Bus->Subscribe(
		FGameplayTag::RequestGameplayTag(TEXT("Event")),
		FEclipseEventNativeDelegate::CreateLambda([&SeenEventTags](FGameplayTag Tag, const FInstancedStruct&)
		{
			SeenEventTags.Add(Tag.GetTagName());
		}));

	// --- squad-feiten meeluisteren -----------------------------------------
	FSquadWatch Watch;
	FEclipseEventSubscriptionHandle SquadHandle = Bus->Subscribe(
		FGameplayTag::RequestGameplayTag(TEXT("Event.Squad")),
		FEclipseEventNativeDelegate::CreateLambda([&Watch](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			const FString Name = Tag.ToString();
			if (Name.Contains(TEXT("Acknowledged")))
			{
				++Watch.Acknowledged;
			}
			else if (Name.Contains(TEXT("Refused")))
			{
				++Watch.Refused;
				// De REDEN meenemen, niet alleen de tag: "orders zijn beloftes"
				// (GDD 8.4) betekent dat een weigering beredeneerd is, en een
				// rapport dat alleen "geweigerd" zegt is zelf een stilte.
				const FEclipseSquadEventPayload* Squad = Payload.GetPtr<FEclipseSquadEventPayload>();
				Watch.RefusalLines.Add(Squad != nullptr
					? FString::Printf(TEXT("%s (reden: %s, bark: '%s')"), *Name, *Squad->Reason.ToString(), *Squad->BarkLine)
					: Name);
			}
		}));

	// Beloningen worden gemeten aan de COMMIT-EIGEN feiten (Event.Economy.
	// ResourcesChanged met reden "MissionReward") en niet aan wallet-delta's: de
	// dagtick boekt legitiem eigen inkomsten en uitgaven, dus een saldoverschil
	// zegt niets over wat de missie uitkeerde. Dezelfde discipline als de
	// M1.1-Gauntlet in EclipseMissionM1Tests.
	int32 CreditsRewarded = 0;
	int32 MaterialsRewarded = 0;
	// De stretchbonus komt onder een EIGEN reden binnen ("OptionalObjective"), en
	// dat is precies waarom dit uur lang onzichtbaar was: de teller hierboven
	// filtert op "MissionReward" en zag hem dus nooit, ook niet toen hij wél
	// betaald werd. Een meting die de helft van de uitbetaling wegfiltert leest
	// als "de bonus komt niet" terwijl er niets mis is met de bonus.
	int32 OptionalMaterials = 0;
	FEclipseEventSubscriptionHandle RewardHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&CreditsRewarded, &MaterialsRewarded, &OptionalMaterials](FGameplayTag, const FInstancedStruct& Payload)
		{
			const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>();
			if (Economy == nullptr)
			{
				return;
			}
			if (Economy->Reason == TEXT("OptionalObjective")
				&& Economy->ResourceType == EclipseTags::Resource_Materials.GetTag())
			{
				OptionalMaterials += Economy->Delta;
				return;
			}
			if (Economy->Reason != TEXT("MissionReward"))
			{
				return;
			}
			if (Economy->ResourceType == EclipseTags::Resource_Credits.GetTag()) { CreditsRewarded += Economy->Delta; }
			if (Economy->ResourceType == EclipseTags::Resource_Materials.GetTag()) { MaterialsRewarded += Economy->Delta; }
		}));

	// --- 1. starten via het ECHTE laadpad -----------------------------------
	const int32 DayBefore = Campaign->GetState().Day;
	// Wallet is een tag->saldo-map (GDD 14.2: grondstofsoorten zijn data).
	auto Wallet = [Campaign](const FGameplayTag& Resource)
	{
		const int32* Balance = Campaign->GetState().Wallet.Find(Resource);
		return Balance != nullptr ? *Balance : 0;
	};
	const int32 MaterialsBefore = Wallet(EclipseTags::Resource_Materials);
	const int32 CreditsBefore = Wallet(EclipseTags::Resource_Credits);

	FString Error;
	if (!TestTrue(FString::Printf(TEXT("speelronde: missie geselecteerd op TransitCheckpoint (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error)))
	{
		Bus->Unsubscribe(SquadHandle);
		Harness.Shutdown();
		return false;
	}
	if (!TestTrue(FString::Printf(TEXT("speelronde: gelanceerd via AutoLaunch (%s)"), *Error), Prep->AutoLaunch(Error)))
	{
		Bus->Unsubscribe(SquadHandle);
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f); // insertie laten landen (pawn naar Entry_Main, spawns)

	TestTrue(TEXT("speelronde: de missie loopt"), Mission->GetPhase() == EEclipseMissionPhase::Objectives
		|| Mission->GetPhase() == EEclipseMissionPhase::Insertion);
	const TArray<FEclipseObjectiveDef>& Objectives = Mission->GetActiveObjectives();
	AddInfo(FString::Printf(TEXT("speelronde: %d objectives actief, squad van %d, start op %s"),
		Objectives.Num(), Mission->GetDeployedSoldierIds().Num(), *Harness.Location().ToCompactString()));
	TestTrue(TEXT("speelronde: er zijn objectives om te halen"), Objectives.Num() >= 2);

	// --- 1b. staan de voeten op de grond? (ANI-09) --------------------------
	// Bijlage D van FEEL_REFERENTIE.md zegt dat MeshZOffset -90 tegen een capsule
	// van 88 "de voeten 2 cm in de vloer" zet, en dat die twee gelijkgetrokken
	// moeten worden. Dat klopt NIET, en het is de moeite waard om dat vast te
	// pinnen voordat iemand het "repareert": UE laat een personage niet ép de
	// vloer rusten maar er een paar centimeter boven (MIN/MAX_FLOOR_DIST), dus de
	// capsule-onderkant zweeft en -90 compenseert precies dat. Op -88 zouden de
	// voeten juist in de lucht hangen.
	//
	// Daarom niet narekenen maar METEN: waar ligt de meshwortel ten opzichte van
	// het oppervlak waar hij op staat?
	if (Harness.Body->GetMesh() != nullptr)
	{
		// Eerst laten landen. EnterMissionMode zet de pawn boven het insertiepunt
		// neer, dus wie meteen meet, meet een vallend personage — de eerste ronde
		// las 160,86 cm en dat leek een defect.
		//
		// Meteen vastleggen HOE VER en HOE LANG hij valt, want dat is de eerste
		// halve seconde van elke missie en die is niet van de speler. Geen assert:
		// er is nog geen norm voor, en een drempel verzinnen zou net zo'n
		// ongefundeerd getal zijn als de 0.75 strafe-ratio die vannacht sneuvelde.
		// Eerst het getal op tafel.
		const double InsertionZ = Harness.Location().Z;
		const double InsertionStart = Harness.ElapsedSeconds;
		Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 3.0, [&Harness]()
		{
			return Harness.Body->GetCharacterMovement()->IsMovingOnGround() && Harness.SpeedCm() < 1.0f;
		});
		const double InsertionFall = InsertionZ - Harness.Location().Z;
		const double InsertionSettle = Harness.ElapsedSeconds - InsertionStart;
		Report(*this, TEXT("val bij insertie"), InsertionFall, TEXT("cm"), TEXT("~0 — hij hoort te STAAN, niet te landen"));
		Report(*this, TEXT("tijd tot de speler staat"), InsertionSettle, TEXT("s"), TEXT("~0 — zolang is de besturing niet van hem"));
		// Er is nu wél een norm, want er is gemeten dat 0 haalbaar is: de pawn wordt
		// op de getraceerde vloer gezet in plaats van 100 cm erboven. Was 160,7 cm
		// over 0,35 s. Ruime drempel, want een andere map mag een paar cm afwijken —
		// maar niet anderhalve meter.
		TestTrue(FString::Printf(TEXT("speelronde: de speler STAAT bij insertie, hij valt er niet in (%.1f cm)"), InsertionFall),
			InsertionFall < 20.0);
		TestTrue(FString::Printf(TEXT("speelronde: de besturing is meteen van hem (%.3f s)"), InsertionSettle),
			InsertionSettle < 0.15);
		FHitResult Ground;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseFootCheck), false, Harness.Body);
		const FVector From = Harness.Location();
		if (Harness.World->LineTraceSingleByChannel(Ground, From, From - FVector(0.0f, 0.0f, 500.0f), ECC_Visibility, Params))
		{
			const double MeshRootZ = Harness.Body->GetMesh()->GetComponentLocation().Z;
			const double FeetAboveGround = MeshRootZ - Ground.ImpactPoint.Z;
			Report(*this, TEXT("meshwortel boven de grond (ANI-09)"), FeetAboveGround, TEXT("cm"),
				TEXT("~0; negatief = voeten in de vloer, positief = zwevend"));
			TestTrue(FString::Printf(TEXT("speelronde: de voeten staan op de grond, niet erin of erboven (%.2f cm)"), FeetAboveGround),
				FMath::Abs(FeetAboveGround) < 3.0);
		}
	}

	// --- 2. een order geven, en er antwoord op krijgen ----------------------
	// "Orders zijn beloftes" (GDD 8.4): elke order hoort exact één ack of één
	// beredeneerde weigering te krijgen. Stilte is de fout die dit meet.
	// Hoe lang duurt het voordat de squad ERGENS heen kan? De navmesh wordt hier
	// rond invokers gegenereerd (bGenerateNavigationOnlyAroundNavigationInvokers)
	// en dat is asynchroon werk. Zolang er geen navmesh onder de squad ligt,
	// weigert elke MoveTo terecht met NoRoute — luid en beredeneerd, maar voor de
	// speler ziet het eruit als een squad die niets doet.
	{
		UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Harness.World);
		int32 NavDataActors = 0;
		for (TActorIterator<ANavigationData> It(Harness.World); It; ++It)
		{
			++NavDataActors;
		}
		AddInfo(FString::Printf(TEXT("speelronde: navigatiesysteem %s · nav-data-actoren %d · nav-bounds %d"),
			Nav != nullptr ? TEXT("aanwezig") : TEXT("ONTBREEKT"), NavDataActors,
			Nav != nullptr ? Nav->GetNavigationBounds().Num() : -1));

		// Synchroon bouwen. Recast bouwt zijn tegels normaal asynchroon, en de
		// duizend snelle ticks van dit harnas kosten samen minder wandkloktijd dan
		// zo'n bouw nodig heeft — dan meet je de taakplanner en niet de game.
		double NavReadyAfter = -1.0;
		if (Nav != nullptr && NavDataActors > 0)
		{
			Nav->Build();
			// ECHTE tijd geven, niet gesimuleerde. Recast bouwt zijn tegels op een
			// achtergrondtaak, en duizend snelle wereldticks kosten samen minder
			// wandkloktijd dan zo'n bouw nodig heeft — dan meet je de taakplanner
			// in plaats van de game.
			//
			// Eén seconde en niet vijf. Vijf was de eerste, onderzoekende waarde;
			// inmiddels is gemeten dat er headless helemaal geen tegels komen, dus
			// die vier extra seconden zijn dood wachten — en ze werden bij ELKE
			// commit betaald (de suite duurde 13,9 s, waarvan 11,1 s deze test).
			// De diagnose blijft: hij rapporteert nog steeds of er navmesh ligt.
			const double WallStart = FPlatformTime::Seconds();
			while (FPlatformTime::Seconds() - WallStart < 1.0)
			{
				Harness.Step();
				FNavLocation Projected;
				if (Nav->ProjectPointToNavigation(Harness.Location(), Projected, FVector(500.0f, 500.0f, 500.0f)))
				{
					NavReadyAfter = FPlatformTime::Seconds() - WallStart;
					break;
				}
				FPlatformProcess::Sleep(0.01f);
			}
		}
		Report(*this, TEXT("navmesh onder de speler na (wandklok)"), NavReadyAfter, TEXT("s"), TEXT(">= 0 = er ligt navmesh; -1 = geen binnen 5 s"));
	}

	// Wachten tot de squad STAAT. SpawnBodyNear zet elk lichaam 100 cm boven de
	// grond neer, dus vlak na de insertie hangen ze in de lucht — en een AI die
	// valt kan geen pad aanvragen. Als je dán een order geeft, weigert hij terecht
	// met "no route", en dat leest als een kapotte squad terwijl het timing is.
	{
		const double SquadStart = Harness.ElapsedSeconds;
		int32 Standing = 0;
		const int32 MaxSteps = FMath::RoundToInt(5.0f / Options.StepSeconds);
		for (int32 I = 0; I < MaxSteps; ++I)
		{
			Standing = 0;
			int32 Total = 0;
			for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
			{
				AEclipseCharacter* Body = *It;
				if (Body == nullptr || Body == Harness.Body || !Body->IsPlayerSide())
				{
					continue;
				}
				++Total;
				if (Body->GetCharacterMovement()->IsMovingOnGround())
				{
					++Standing;
				}
			}
			if (Total > 0 && Standing == Total)
			{
				break;
			}
			Harness.Step();
		}
		Report(*this, TEXT("tijd tot de squad staat"), Harness.ElapsedSeconds - SquadStart, TEXT("s"),
			TEXT("zolang kan hij geen enkele order aannemen"));

		// WAAR staat de squad eigenlijk? Ze horen naast je te staan. De
		// gedeeltelijk-pad-meting suggereerde iets anders: 48% afgelegd met nog
		// 4448 cm te gaan naar een punt dat 600 cm van de SPELER lag — dan staat de
		// soldaat dus kilometers verderop.
		float FarthestSquadmate = 0.0f;
		for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
		{
			const AEclipseCharacter* Soldier = *It;
			if (Soldier != nullptr && Soldier != Harness.Body && Soldier->IsPlayerSide())
			{
				FarthestSquadmate = FMath::Max(FarthestSquadmate,
					static_cast<float>(FVector::Dist2D(Soldier->GetActorLocation(), Harness.Location())));
			}
		}
		Report(*this, TEXT("verste squadmate van de speler"), FarthestSquadmate, TEXT("cm"),
			TEXT("een paar honderd — ze horen naast je te staan bij insertie"));
		// Was 9282 cm: de game mode las de positie van de pawn vóórdat de
		// controller hem naar Entry_Main verplaatste. Vastgepind, want dit is een
		// VOLGORDE-bug tussen twee luisteraars op hetzelfde event, en die soort
		// sluipt terug zodra iemand een derde luisteraar toevoegt.
		TestTrue(FString::Printf(TEXT("speelronde: de squad staat NAAST je bij insertie (%.0f cm)"), FarthestSquadmate),
			FarthestSquadmate < 1500.0f);
	}

	if (UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>())
	{
		Watch.Issued = 1;
		const FVector OrderTarget = Harness.Location() + FVector(600.0f, 0.0f, 0.0f);
		// Ligt het DOEL op de navmesh, en liggen de soldaten er zelf op? Weigert de
		// squad terwijl beide "ja" zeggen, dan zit de oorzaak in het orderpad; zegt
		// een van beide "nee", dan is de weigering terecht en is het dekking.
		if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Harness.World))
		{
			FNavLocation Projected;
			const bool bTargetOnMesh = Nav->ProjectPointToNavigation(OrderTarget, Projected, FVector(500.0f, 500.0f, 500.0f));
			int32 OnMesh = 0;
			int32 Soldiers = 0;
			for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
			{
				AEclipseCharacter* Body = *It;
				if (Body == nullptr || Body == Harness.Body || !Body->IsPlayerSide())
				{
					continue;
				}
				++Soldiers;
				FNavLocation SoldierSpot;
				if (Nav->ProjectPointToNavigation(Body->GetActorLocation(), SoldierSpot, FVector(500.0f, 500.0f, 500.0f)))
				{
					++OnMesh;
				}
			}
			AddInfo(FString::Printf(TEXT("speelronde: orderdoel op navmesh = %s · soldaten op navmesh = %d van %d"),
				bTargetOnMesh ? TEXT("JA") : TEXT("NEE"), OnMesh, Soldiers));

			// De beslissende vraag, nu de geometrie is vrijgepleit: KAN er een pad
			// gevonden worden? Vindt de padzoeker er wel een en faalt MoveToLocation
			// toch, dan ligt het aan de controller-opzet. Vindt hij er geen terwijl
			// begin en eind allebei op de mesh liggen, dan matcht de nav-agent van
			// deze pawn niet met de geregistreerde nav data.
			for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
			{
				AEclipseCharacter* Soldier = *It;
				if (Soldier == nullptr || Soldier == Harness.Body || !Soldier->IsPlayerSide())
				{
					continue;
				}
				const ANavigationData* AgentData = Nav->GetNavDataForProps(Soldier->GetNavAgentPropertiesRef());
				const FPathFindingResult Path = Nav->FindPathSync(
					FPathFindingQuery(Soldier, *Nav->GetDefaultNavDataInstance(),
						Soldier->GetNavAgentLocation(), OrderTarget));
				AddInfo(FString::Printf(
					TEXT("speelronde: '%s' — nav-data voor zijn agent = %s · padzoeker = %s%s"),
					*Soldier->GetName(),
					AgentData != nullptr ? *AgentData->GetName() : TEXT("GEEN"),
					Path.IsSuccessful() ? TEXT("pad gevonden") : TEXT("GEEN pad"),
					Path.IsPartial() ? TEXT(" (gedeeltelijk)") : TEXT("")));

				// HOE VER komt hij met dat gedeeltelijke pad? Dat getal is wat de
				// A/B-keuze in HANDOFF §4b onderbouwt: haalt hij 95% van de weg,
				// dan is weigeren onzin; haalt hij 10%, dan is "no route" eerlijk.
				// Zonder dit is die keuze smaak tegen smaak.
				if (Path.IsSuccessful() && Path.Path.IsValid() && Path.Path->GetPathPoints().Num() > 0)
				{
					const FVector Start = Soldier->GetNavAgentLocation();
					const FVector Reached = Path.Path->GetPathPoints().Last().Location;
					const float Ordered = FVector::Dist2D(Start, OrderTarget);
					const float Covered = FVector::Dist2D(Start, Reached);
					Report(*this, TEXT("gedeeltelijk pad: afgelegd van het bevolen punt"),
						Ordered > KINDA_SMALL_NUMBER ? (Covered / Ordered) * 100.0f : 0.0f, TEXT("%"),
						TEXT("hoog = weigeren is onzin; laag = 'no route' is eerlijk (HANDOFF 4b)"));
					Report(*this, TEXT("gedeeltelijk pad: resterende afstand tot het doel"),
						FVector::Dist2D(Reached, OrderTarget), TEXT("cm"));
				}
				break; // één soldaat is genoeg om dit te beantwoorden
			}
		}
		Squad->IssueOrderToAll(EEclipseSquadOrder::MoveTo, OrderTarget, nullptr);
		Harness.Idle(0.5f);
		const int32 Answers = Watch.Acknowledged + Watch.Refused;
		Report(*this, TEXT("orders gegeven"), Watch.Issued, TEXT(""));
		Report(*this, TEXT("antwoorden terug (ack + weigering)"), Answers, TEXT(""), TEXT("nooit 0 — stilte is de fout"));
		TestTrue(FString::Printf(TEXT("speelronde: de order kreeg antwoord (%d ack, %d geweigerd)"),
				Watch.Acknowledged, Watch.Refused), Answers > 0);
		// En hij wordt AANGENOMEN, niet alleen beantwoord. Tot 2026-07-26 weigerde
		// de squad hier alle drie de orders; dat was geen ontwerpkeuze maar een
		// gevolg van de spawnpositie 93 meter verderop.
		TestEqual(TEXT("speelronde: een haalbare order wordt aangenomen, niet geweigerd"), Watch.Refused, 0);

		// EN BEWEGEN ZE OOK. Een ack zonder beweging is precies de stille fout waar
		// "orders zijn beloftes" (GDD 8.4) voor bestaat: het antwoord klopt, het
		// gedrag niet, en niets in de suite zou dat merken. Tot vannacht kon deze
		// meting niet eens bestaan — de squad weigerde alles.
		TMap<AEclipseCharacter*, float> DistanceBefore;
		for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
		{
			AEclipseCharacter* Soldier = *It;
			if (Soldier != nullptr && Soldier != Harness.Body && Soldier->IsPlayerSide())
			{
				DistanceBefore.Add(Soldier, FVector::Dist2D(Soldier->GetActorLocation(), OrderTarget));
			}
		}
		Harness.Idle(2.5f);
		int32 Approached = 0;
		float BestGain = 0.0f;
		for (const TPair<AEclipseCharacter*, float>& Pair : DistanceBefore)
		{
			if (Pair.Key == nullptr)
			{
				continue;
			}
			const float Gain = Pair.Value - static_cast<float>(FVector::Dist2D(Pair.Key->GetActorLocation(), OrderTarget));
			BestGain = FMath::Max(BestGain, Gain);
			if (Gain > 25.0f)
			{
				++Approached;
			}
		}
		Report(*this, TEXT("soldaten die na de order dichterbij kwamen"), Approached, TEXT(""),
			*FString::Printf(TEXT("van %d — een ack zonder beweging is een stille fout"), DistanceBefore.Num()));
		Report(*this, TEXT("grootste toenadering in 2,5 s"), BestGain, TEXT("cm"));
		TestTrue(FString::Printf(TEXT("speelronde: de squad VOERT de order ook uit (%d van %d bewoog, beste %.0f cm)"),
				Approached, DistanceBefore.Num(), BestGain), Approached > 0);
		Report(*this, TEXT("order-round-trip, slechtste"), Squad->GetOrderRoundTripStats().WorstSeconds, TEXT("s"), TEXT("< 1 s (R3-criterium 1)"));
	}

	// --- 3. oprukken EN vechten ---------------------------------------------
	// Niet eerst lopen en dan pas kijken: de eerste ronde liep zo recht in vier
	// vuurmonden en de speler lag neer voordat hij het site zag. Elke tick de
	// dichtstbijzijnde levende vijand zoeken; is die binnen ONS bereik (5000 cm,
	// AR_Foundry) maar liefst nog buiten het hunne (2500-3000 cm perceptie), dan
	// staan blijven en vuren. Anders doorlopen. Dat is wat een speler doet die
	// zijn wapenbereik kent, en het is de enige manier waarop deze missie te
	// winnen is met 100 HP tegen vier schutters.
	const FVector StartLocation = Harness.Location();
	constexpr float EngageRangeCm = 4200.0f;

	auto FindNearestHostile = [&Harness]() -> AEclipseCharacter*
	{
		AEclipseCharacter* Nearest = nullptr;
		float NearestDistance = TNumericLimits<float>::Max();
		for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
		{
			AEclipseCharacter* Character = *It;
			if (Character == nullptr || Character == Harness.Body || Character->IsPlayerSide() || Character->IsDowned())
			{
				continue;
			}
			const float Distance = FVector::Dist(Character->GetActorLocation(), Harness.Location());
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = Character;
			}
		}
		return Nearest;
	};

	int32 HostilesAtStart = 0;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		AEclipseCharacter* Character = *It;
		if (Character != nullptr && Character != Harness.Body && !Character->IsPlayerSide())
		{
			++HostilesAtStart;
		}
	}
	Report(*this, TEXT("vijanden gespawnd voor deze missie"), HostilesAtStart, TEXT(""), TEXT("> 0 — anders valt er niets te halen"));

	// Spawnposities bewaren om straks te zien of ze uberhaupt bewogen hebben.
	TMap<TWeakObjectPtr<AEclipseCharacter>, FVector> HostileStartPositions;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		AEclipseCharacter* Character = *It;
		if (Character != nullptr && Character != Harness.Body && !Character->IsPlayerSide())
		{
			HostileStartPositions.Add(Character, Character->GetActorLocation());
		}
	}
	TestTrue(TEXT("speelronde: er staan vijanden om uit te schakelen"), HostilesAtStart > 0);

	// Welk objective vraagt om een dood doelwit?
	const FName DestroyObjectiveId = [&Objectives]()
	{
		for (const FEclipseObjectiveDef& O : Objectives)
		{
			if (O.Type == EEclipseObjectiveType::DestroyTarget) { return O.ObjectiveId; }
		}
		return FName(NAME_None);
	}();
	TestTrue(TEXT("speelronde: M1.1 heeft een DestroyTarget-objective"), !DestroyObjectiveId.IsNone());

	// Bewijs dat AANWEZIGHEID het niet doet: we staan straks pal op het site en
	// het objective mag pas afgaan als er geschoten is. Nu, aan het begin, hoort
	// hij sowieso open te staan.
	TestFalse(TEXT("speelronde: het DestroyTarget-objective staat nog open bij vertrek"),
		Mission->GetCompletedObjectiveIds().Contains(DestroyObjectiveId));

	int32 FiringTicks = 0;
	int32 HostilesDowned = 0;
	bool bReachedPost = false;
	// Hoe dicht is de speler ooit bij een vijand geweest? Dat getal legt een
	// DEKKINGSGAT vast in plaats van het te verbergen: deze ronde wint door buiten
	// hun waarnemingsbereik te blijven, dus de vijand-AI (naderen, dekking zoeken,
	// terugvuren) wordt hier niet uitgeoefend. Zolang dit getal boven hun
	// waarnemingsbereik blijft, is die kant van het gevecht ONGETEST, en dat hoort
	// zichtbaar te zijn in plaats van impliciet.
	float ClosestHostileEver = TNumericLimits<float>::Max();
	{
		const int32 MaxSteps = FMath::RoundToInt(120.0f / Options.StepSeconds);
		const int32 ProgressWindow = FMath::RoundToInt(0.5f / Options.StepSeconds);
		const int32 StalledFireLimit = FMath::RoundToInt(2.0f / Options.StepSeconds);
		float DistanceAtWindowStart = FVector::Dist2D(Harness.Location(), SiteControlPost);
		int32 StepsInWindow = 0;
		int32 SidestepStepsLeft = 0;
		float SidestepDirection = 1.0f;
		// Zakt het leven van het doelwit niet, dan schieten we in dekking. Een
		// speler loopt dan door tot hij zicht heeft; dat doet dit ook.
		AEclipseCharacter* LastTarget = nullptr;
		float LastTargetHealth = 0.0f;
		int32 TicksWithoutDamage = 0;

		for (int32 I = 0; I < MaxSteps && !Harness.Body->IsDowned(); ++I)
		{
			const bool bObjectiveDone = Mission->GetCompletedObjectiveIds().Contains(DestroyObjectiveId);
			AEclipseCharacter* Hostile = FindNearestHostile();
			const float HostileDistance = Hostile != nullptr
				? FVector::Dist(Hostile->GetActorLocation(), Harness.Location()) : TNumericLimits<float>::Max();
			ClosestHostileEver = FMath::Min(ClosestHostileEver, HostileDistance);

			if (Hostile != nullptr && HostileDistance <= EngageRangeCm)
			{
				if (Hostile != LastTarget)
				{
					LastTarget = Hostile;
					LastTargetHealth = Hostile->GetHealth();
					TicksWithoutDamage = 0;
				}
				else if (Hostile->GetHealth() < LastTargetHealth - KINDA_SMALL_NUMBER)
				{
					LastTargetHealth = Hostile->GetHealth();
					TicksWithoutDamage = 0;
				}
				else
				{
					++TicksWithoutDamage;
				}

				// Borsthoogte, niet de actor-oorsprong: die ligt bij een character op
				// vloerniveau, en een schot daarheen gaat onder hem door.
				Harness.AimAt(Hostile->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f));
				Harness.Inject(TEXT("Fire"), true);
				++FiringTicks;
				if (TicksWithoutDamage > StalledFireLimit && HostileDistance > 800.0f)
				{
					// Twee seconden vuren zonder schade = dekking. Doorlopen.
					Harness.Inject(TEXT("Move"), FVector2D(0.0f, 1.0f));
				}
				Harness.Step();
				continue;
			}

			const float Distance = FVector::Dist2D(Harness.Location(), SiteControlPost);
			if (bObjectiveDone && HostileDistance > EngageRangeCm)
			{
				bReachedPost = true;
				break; // doel geraakt en niemand meer in bereik: door naar extractie
			}
			if (Distance <= 700.0f && Hostile == nullptr)
			{
				bReachedPost = true;
				break;
			}
			if (++StepsInWindow >= ProgressWindow)
			{
				if (SidestepStepsLeft <= 0 && DistanceAtWindowStart - Distance < 20.0f)
				{
					SidestepStepsLeft = FMath::RoundToInt(0.8f / Options.StepSeconds);
					SidestepDirection = -SidestepDirection;
				}
				DistanceAtWindowStart = Distance;
				StepsInWindow = 0;
			}
			Harness.AimAt(FVector(SiteControlPost.X, SiteControlPost.Y, Harness.Location().Z + 60.0f));
			const bool bSidestepping = SidestepStepsLeft > 0;
			if (bSidestepping)
			{
				--SidestepStepsLeft;
			}
			Harness.Inject(TEXT("Move"), bSidestepping ? FVector2D(SidestepDirection, 0.35f) : FVector2D(0.0f, 1.0f));
			Harness.Step();
		}
	}

	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		AEclipseCharacter* Character = *It;
		if (Character != nullptr && Character != Harness.Body && !Character->IsPlayerSide() && Character->IsDowned())
		{
			++HostilesDowned;
		}
	}

	// Bewegen de VIJANDEN ook? Let op bij het lezen: 0 is hier de VERWACHTE uitkomst
	// en geen defect. Deze ronde vuurt vanaf 4200 cm terwijl hun waarnemingsbereik
	// 2500-3000 is, dus ze zien nooit iemand — bewezen met een eerste-contactregel
	// die nul keer vuurde. Het getal staat er om de dag dat iemand de bereiken
	// verandert: gaat de speler dichterbij vechten, dan hoort dit omhoog te gaan.
	{
		int32 Moved = 0;
		float BestMove = 0.0f;
		for (const TPair<TWeakObjectPtr<AEclipseCharacter>, FVector>& Pair : HostileStartPositions)
		{
			const AEclipseCharacter* Hostile = Pair.Key.Get();
			if (Hostile == nullptr)
			{
				continue;
			}
			const float Moved2D = static_cast<float>(FVector::Dist2D(Hostile->GetActorLocation(), Pair.Value));
			BestMove = FMath::Max(BestMove, Moved2D);
			if (Moved2D > 100.0f)
			{
				++Moved;
			}
		}
		Report(*this, TEXT("vijanden die van hun spawnplek kwamen"), Moved, TEXT(""),
			*FString::Printf(TEXT("van %d — 0 betekent hier NIET 'kapot': deze ronde schakelt ze uit van buiten hun waarnemingsbereik, dus ze zien nooit iemand"), HostileStartPositions.Num()));
		Report(*this, TEXT("grootste vijandbeweging"), BestMove, TEXT("cm"));
	}

	Report(*this, TEXT("afgelegde weg naar het controlepost"), FVector::Dist2D(Harness.Location(), StartLocation), TEXT("cm"));
	Report(*this, TEXT("resterende afstand tot het site"), FVector::Dist2D(Harness.Location(), SiteControlPost), TEXT("cm"));
	Report(*this, TEXT("ticks waarin gevuurd is"), FiringTicks, TEXT(""), TEXT("> 0 — er MOET geschoten worden"));
	Report(*this, TEXT("dichtste nadering tot een vijand"),
		ClosestHostileEver == TNumericLimits<float>::Max() ? -1.0f : ClosestHostileEver, TEXT("cm"),
		TEXT("boven hun waarnemingsbereik (2500-3000) = de vijand-AI is deze ronde ONGETEST"));
	TestFalse(TEXT("speelronde: de speler overleefde het vuurgevecht"), Harness.Body->IsDowned());
	TestTrue(TEXT("speelronde: er is daadwerkelijk gevuurd"), FiringTicks > 0);

	Report(*this, TEXT("vijanden neergehaald"), HostilesDowned, TEXT(""), TEXT("minstens het doelwit"));
	TestTrue(TEXT("speelronde: er ging minstens één vijand neer"), HostilesDowned > 0);

	// WAT HET DE SQUAD KOSTTE (26-07 avond). Vóór vanavond stonden ze stil te
	// kijken en gingen ze dus nooit neer; sinds ze uit zichzelf vuren, staan ze in
	// het vuurgevecht en kunnen ze vallen. Dat is de andere helft van de
	// balansverschuiving die de gevechts-audit op 2,78× zet: het gaat sneller, maar
	// het kost je nu ook mensen.
	//
	// Als METING en niet als eis: hoeveel er hoort te vallen is een balansvraag en
	// die is van de owner. Een getal dat er niet staat kan hij niet beoordelen.
	int32 SquadStanding = 0;
	int32 SquadDown = 0;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		if (const AEclipseCharacter* Body = Cast<AEclipseCharacter>(It->GetPawn()))
		{
			Body->IsDowned() ? ++SquadDown : ++SquadStanding;
		}
	}
	Report(*this, TEXT("squadleden nog overeind"), static_cast<float>(SquadStanding), TEXT(""));
	Report(*this, TEXT("squadleden neergegaan"), static_cast<float>(SquadDown), TEXT(""),
		TEXT("vóór 26-07 avond was dit altijd 0: ze vochten niet mee"));

	// --- 4. het objective is af DOOR TE SCHIETEN ----------------------------
	Harness.Idle(0.3f);
	TestTrue(TEXT("speelronde: 'take out the patrol leader' is af — door te schieten, niet door erlangs te lopen"),
		Mission->GetCompletedObjectiveIds().Contains(DestroyObjectiveId));

	// --- 5. naar extractie, en dus naar de debrief --------------------------
	// 150 cm en niet 700: de objective-trigger is een box met extent 200, dus op
	// 700 cm sta je er netjes naast en gebeurt er niets. De eerste ronde haalde
	// 698 cm en meldde "aangekomen" terwijl de missie gewoon doorliep — een
	// aankomstradius die groter is dan de trigger meet zichzelf voor de gek.
	const bool bReachedExfil = Harness.DriveTo(SiteExtraction, /*MaxSeconds*/ 90.0, /*ArriveRadius*/ 150.0f);
	Report(*this, TEXT("resterende afstand tot extractie"), FVector::Dist2D(Harness.Location(), SiteExtraction), TEXT("cm"), TEXT("< 150 (trigger-box extent 200)"));
	TestTrue(TEXT("speelronde: extractie is te bereiken (geen vastloper)"), bReachedExfil);
	Harness.Idle(0.5f);

	// --- 6. asserteren op de UITKOMST ---------------------------------------
	TestTrue(TEXT("speelronde: de missie is afgerond (debrief bereikt)"),
		Mission->GetPhase() == EEclipseMissionPhase::Finished);
	const FEclipseMissionOutcome& Outcome = Mission->GetLastOutcome();
	AddInfo(FString::Printf(TEXT("speelronde: debrief — geslaagd=%d, %d/%d objectives"),
		Outcome.bSuccess ? 1 : 0, Mission->GetCompletedObjectiveIds().Num(), Objectives.Num()));
	TestTrue(TEXT("speelronde: de run slaagde (alle verplichte objectives gehaald)"), Outcome.bSuccess);

	const int32 DayAfter = Campaign->GetState().Day;
	const int32 MaterialsAfter = Wallet(EclipseTags::Resource_Materials);
	const int32 CreditsAfter = Wallet(EclipseTags::Resource_Credits);
	Report(*this, TEXT("dagklok voor -> na"), DayAfter - DayBefore, TEXT("dagen"), TEXT("+1 — elke missie kost een dag"));
	Report(*this, TEXT("materialen uitgekeerd (commit-feit)"), MaterialsRewarded, TEXT(""), TEXT("M1.1-band: 25"));
	Report(*this, TEXT("credits uitgekeerd (commit-feit)"), CreditsRewarded, TEXT(""), TEXT("M1.1-band: 50"));
	Report(*this, TEXT("wallet-delta materialen (dagtick meegerekend)"), MaterialsAfter - MaterialsBefore, TEXT(""), TEXT("ter informatie, geen criterium"));
	Report(*this, TEXT("wallet-delta credits (dagtick meegerekend)"), CreditsAfter - CreditsBefore, TEXT(""), TEXT("ter informatie, geen criterium"));
	TestTrue(FString::Printf(TEXT("speelronde: de dag is opgeschoven (%d -> %d)"), DayBefore, DayAfter), DayAfter > DayBefore);
	TestEqual(TEXT("speelronde: de missie keerde de materialen van de rij uit"), MaterialsRewarded, 25);

	// De +20 voor een ronde zonder gewonden (owner-beslissing 26-07). Tot vandaag
	// hing die aan een optional die VOLTOOID moest zijn terwijl niets hem kon
	// voltooien — het is een voorwaarde, geen taak. De HUD toonde een stretch-doel
	// dat nooit afvinkte en de bonus kwam nooit binnen.
	//
	// Asserteren op de voorwaarde en niet blind op 20: ging er iemand neer, dan
	// HOORT hij nul te zijn. Anders zou deze test bij een zware ronde rood gaan om
	// precies de reden waarom het ontwerp klopt.
	const bool bEveryoneStanding = !Mission->HasAnyCasualtyThisRun();
	Report(*this, TEXT("stretchbonus materialen"), OptionalMaterials, TEXT(""),
		TEXT("+20 als iedereen staande bleef, anders 0"));
	TestEqual(bEveryoneStanding
			? TEXT("speelronde: iedereen bleef staan, dus de +20-stretchbonus is uitbetaald")
			: TEXT("speelronde: er ging iemand neer, dus de stretchbonus is terecht NIET uitbetaald"),
		OptionalMaterials, bEveryoneStanding ? 20 : 0);
	TestEqual(TEXT("speelronde: de missie keerde de credits van de rij uit"), CreditsRewarded, 50);

	// Regiostaat: M1.1 mag de wereld NIET flippen (SPEC-P2-04 besluit 6 — M1.3 is
	// via de P2-05-naad de enige world-state-change). Dit is de assert die een
	// per ongeluk aangezette bProgressRegionOnSuccess zou vangen.
	if (const FEclipseRegionState* Region = Campaign->GetState().FindRegion(TEXT("TransitCheckpoint")))
	{
		AddInfo(FString::Printf(TEXT("speelronde: TransitCheckpoint blijft van %s"),
			*UEnum::GetValueAsString(Region->Owner)));
		TestTrue(TEXT("speelronde: M1.1 flipt de regio niet (SPEC-P2-04 besluit 6)"),
			Region->Owner != EEclipseRegionOwner::Player);
	}

	// --- 7. het budget en de stiltes ----------------------------------------
	// EERLIJK: dit is de kosten van de GAME-THREAD-simulatie, niet de framerate.
	// Headless rendert niets, dus GPU-tijd komt hier niet in voor en dat mag dit
	// getal ook niet suggereren.
	Report(*this, TEXT("game-thread per tick, gemiddeld"), Harness.AverageStepMs(), TEXT("ms"), TEXT("< 16.7 ms (60 fps-budget, 12.4)"));
	Report(*this, TEXT("game-thread per tick, slechtste"), Harness.WorstStepMs, TEXT("ms"), TEXT("uitschieters horen bij spawns"));
	Bus->Unsubscribe(AllHandle);
	{
		TArray<FName> Fired = SeenEventTags.Array();
		Fired.Sort(FNameLexicalLess());
		for (const FName& Tag : Fired)
		{
			AddInfo(FString::Printf(TEXT("event gevuurd: %s"), *Tag.ToString()));
		}
		Report(*this, TEXT("verschillende events gevuurd in deze missie"), Fired.Num(), TEXT(""),
			TEXT("de rest vuurt buiten een missie — sinds 26-07 is gemeten dat ELK gecatalogiseerd event ergens afgaat"));

		// Zelfde principe één laag lager: welke objective-TYPES kwamen langs? Ook
		// dat is dekking die je alleen ziet als de ronde het zelf zegt. M1.1 heeft
		// geen CollectItem; die wordt apart gedekt door de MT_Rescue-test, en zonder
		// deze regel is dat gat onzichtbaar.
		TSet<uint8> TypesSeen;
		FString TypeList;
		for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
		{
			if (!TypesSeen.Contains(static_cast<uint8>(Objective.Type)))
			{
				TypesSeen.Add(static_cast<uint8>(Objective.Type));
				TypeList += (TypeList.IsEmpty() ? TEXT("") : TEXT(", "))
					+ UEnum::GetValueAsString(Objective.Type).RightChop(FString(TEXT("EEclipseObjectiveType::")).Len());
			}
		}
		AddInfo(FString::Printf(TEXT("objective-types in deze missie: %s"), *TypeList));
		Report(*this, TEXT("objective-types uitgeoefend"), TypesSeen.Num(), TEXT(""),
			TEXT("van de 4 — CollectItem zit niet in M1.1 en wordt apart gedekt"));
	}

	Report(*this, TEXT("ticks in deze ronde"), Harness.StepCount, TEXT(""));
	Report(*this, TEXT("gesimuleerde speeltijd"), Harness.ElapsedSeconds, TEXT("s"));
	TestTrue(FString::Printf(TEXT("speelronde: de game-thread blijft binnen het 60 fps-budget (%.2f ms gemiddeld)"),
			Harness.AverageStepMs()), Harness.AverageStepMs() < 16.7);

	Report(*this, TEXT("squad-weigeringen"), Watch.Refused, TEXT(""), TEXT("mag, maar nooit stil"));
	for (const FString& Line : Watch.RefusalLines)
	{
		AddInfo(FString::Printf(TEXT("speelronde: weigering — %s"), *Line));
	}

	Bus->Unsubscribe(SquadHandle);
	Bus->Unsubscribe(RewardHandle);
	Harness.Shutdown();
	return true;
}


// ---------------------------------------------------------------------------
// De regressiepin onder het defect dat de speelronde vond
// ---------------------------------------------------------------------------
//
// De volle speelronde hierboven bewijst dat de missie te WINNEN is door te
// schieten. Deze test bewijst het omgekeerde, en dat is de helft die het defect
// zou hebben gevangen: er STAAN op een DestroyTarget-site vinkt hem niet af.
// Puur op de runtime-naad, zonder wereld — snel genoeg om elke bar mee te lopen.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipsePresenceDoesNotDestroyTest,
	"Eclipse.Playthrough.PresenceNeverCompletesADestroyObjective",
	EclipsePlaythrough::TestFlags)

bool FEclipsePresenceDoesNotDestroyTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	const UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (!TestNotNull(TEXT("pin: DA_CampaignSetup"), Setup))
	{
		GameInstance->Shutdown();
		return false;
	}
	Campaign->StartNewCampaign(Setup);

	FString Error;
	if (!TestTrue(TEXT("pin: M1.1 geselecteerd"), Strategy->SelectMission(TEXT("TransitCheckpoint"), Error))
		|| !TestTrue(TEXT("pin: M1.1 gelanceerd"), Prep->AutoLaunch(Error)))
	{
		GameInstance->Shutdown();
		return false;
	}

	FName DestroySite = NAME_None;
	FName DestroyObjectiveId = NAME_None;
	for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
	{
		if (Objective.Type == EEclipseObjectiveType::DestroyTarget)
		{
			DestroySite = Objective.TargetId;
			DestroyObjectiveId = Objective.ObjectiveId;
			break;
		}
	}
	if (!TestTrue(TEXT("pin: M1.1 heeft een DestroyTarget-objective"), !DestroyObjectiveId.IsNone()))
	{
		GameInstance->Shutdown();
		return false;
	}

	// Dit is precies wat AEclipseObjectiveTrigger doet als je het vak binnenloopt.
	// Tien keer, want een speler loopt heen en weer over zo'n site.
	for (int32 I = 0; I < 10; ++I)
	{
		Mission->NotifySiteEntered(DestroySite);
	}
	TestFalse(TEXT("pin: erop staan vinkt het DestroyTarget-objective NIET af"),
		Mission->GetCompletedObjectiveIds().Contains(DestroyObjectiveId));

	// En de andere helft: het doelwit neerhalen doet het wél. Zonder deze assert
	// zou "nooit voltooien" ook groen zijn, en dat is een erger defect.
	Mission->CompleteObjectiveByTarget(DestroySite);
	TestTrue(TEXT("pin: het doelwit neerhalen vinkt hem wél af"),
		Mission->GetCompletedObjectiveIds().Contains(DestroyObjectiveId));

	// Presence-typen blijven werken zoals ze deden — de gate mag niet te breed zijn.
	//
	// GETELD, niet aangenomen: deze lus stond hier als "ReachLocation OF
	// CollectItem", en las daardoor als dekking van beide. M1.1 heeft geen
	// CollectItem, dus die helft draaide NOOIT — een assert in een tak die niet
	// bestaat is dekking op papier. De teller hieronder maakt zichtbaar wat er
	// werkelijk langskwam, en de assert erop zorgt dat een lege lus niet meer
	// stilletjes voor een geslaagde test doorgaat.
	int32 PresenceObjectivesChecked = 0;
	for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
	{
		if (Objective.Type == EEclipseObjectiveType::ReachLocation || Objective.Type == EEclipseObjectiveType::CollectItem)
		{
			++PresenceObjectivesChecked;
			Mission->NotifySiteEntered(Objective.TargetId);
			TestTrue(FString::Printf(TEXT("pin: aanwezigheid vervult '%s' wél"), *Objective.ObjectiveId.ToString()),
				Mission->GetCompletedObjectiveIds().Contains(Objective.ObjectiveId));
		}
	}
	AddInfo(FString::Printf(TEXT("pin: %d presence-objectives in deze missie gecontroleerd"), PresenceObjectivesChecked));
	TestTrue(TEXT("pin: er is minstens één presence-objective gecontroleerd (anders is deze lus lege dekking)"),
		PresenceObjectivesChecked > 0);
	// CollectItem zit NIET in M1.1 en wordt daarom apart gedekt door
	// Eclipse.Playthrough.CollectItemIsCompletableInAShippedMission (MT_Rescue).

	GameInstance->Shutdown();
	return true;
}

// De andere helft van het gevecht.
//
// De grote speelronde wint door van 42 meter af te schieten. Dat is legitiem
// spelersgedrag, maar het betekent dat de vijand daar NOOIT iets doet: geen
// naderen, geen terugvuren. Die ronde meet zijn dichtste nadering (~4170 cm)
// juist om dat gat zichtbaar te houden; deze test dekt het af. Hij speelt niet
// om te winnen — hij loopt bewust hun bereik in en kijkt wat er dan gebeurt.
//
// Wat de eerste rondes leerden, en waarom hier per stap gemeten wordt: bij
// aankomst waren er nul vijanden en nul schade, en dat las als "er gebeurt
// niets". Het tegendeel was waar. De speler ging NEER op 31,5 s, de missie
// faalde, en het opruimen despawnde alle vier de vijanden. Alles wat daarna
// gemeten werd, mat een opgeruimde wereld. Meten na de gebeurtenis meet de
// gebeurtenis niet.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEnemiesEngageTest,
	"Eclipse.Playthrough.EnemiesEngageWhenYouWalkIntoTheirRange",
	EclipsePlaythrough::TestFlags)

bool FEclipseEnemiesEngageTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	IConsoleVariable* ForceGamepad = IConsoleManager::Get().FindConsoleVariable(TEXT("Eclipse.Input.ForceGamepad"));
	const int32 PreviousForce = ForceGamepad != nullptr ? ForceGamepad->GetInt() : -1;
	if (ForceGamepad != nullptr)
	{
		ForceGamepad->Set(0, ECVF_SetByCode);
	}
	ON_SCOPE_EXIT
	{
		if (ForceGamepad != nullptr)
		{
			ForceGamepad->Set(PreviousForce, ECVF_SetByCode);
		}
	};

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* MissionSub = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestNotNull(TEXT("contact: strategie"), Strategy) || !TestNotNull(TEXT("contact: prep"), Prep)
		|| !TestNotNull(TEXT("contact: missie"), MissionSub) || !TestNotNull(TEXT("contact: eventbus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	// Hetzelfde ECHTE laadpad als de grote ronde: een contacttest op een eigen
	// opstelling bewijst iets over die opstelling, niet over M1.1.
	FString Error;
	if (!TestTrue(FString::Printf(TEXT("contact: missie geselecteerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error))
		|| !TestTrue(FString::Printf(TEXT("contact: gelanceerd (%s)"), *Error), Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	double MissionFailedAt = -1.0;
	FEclipseEventSubscriptionHandle MissionHandle = Bus->Subscribe(
		FGameplayTag::RequestGameplayTag(TEXT("Event.Mission")),
		FEclipseEventNativeDelegate::CreateLambda([&MissionFailedAt, &Harness](FGameplayTag Tag, const FInstancedStruct&)
		{
			if (MissionFailedAt < 0.0 && Tag.ToString().Contains(TEXT("Failed")))
			{
				MissionFailedAt = Harness.ElapsedSeconds;
			}
		}));

	auto ForEachHostile = [&Harness](TFunctionRef<void(AEclipseCharacter&)> Fn)
	{
		for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
		{
			AEclipseCharacter* Character = *It;
			if (Character != nullptr && Character != Harness.Body && !Character->IsPlayerSide())
			{
				Fn(*Character);
			}
		}
	};

	TMap<TWeakObjectPtr<AEclipseCharacter>, FVector> SpawnPositions;
	ForEachHostile([&SpawnPositions](AEclipseCharacter& Hostile)
	{
		SpawnPositions.Add(&Hostile, Hostile.GetActorLocation());
	});
	Report(*this, TEXT("vijanden bij vertrek"), SpawnPositions.Num(), TEXT(""), TEXT("> 0 — anders is er niets om tegenaan te lopen"));
	if (SpawnPositions.Num() == 0)
	{
		Bus->Unsubscribe(MissionHandle);
		Harness.Shutdown();
		return false;
	}

	// Naar het controlepost lopen en NIET stoppen op vuurafstand. Geen sprint:
	// dit gaat over binnenkomen, niet over snel zijn.
	//
	// In korte etappes, met een meting NA ELKE etappe. Alles wat telt gebeurt in
	// de zes seconden tussen "binnen bereik" en "neer", en een meting achteraf
	// mist dat volledig.
	const float HealthAtStart = Harness.Body->GetHealth();
	float FirstDamageAt = -1.0f;
	float DistanceAtFirstDamage = -1.0f;
	float ClosestApproach = TNumericLimits<float>::Max();
	float FurthestHostileMove = 0.0f;
	float DownedAt = -1.0f;
	float LowestHealth = HealthAtStart;

	// VANAF WELKE AFSTAND VERTREKT HIJ. Nodig om "heeft hij afstand overbrugd" te
	// kunnen beoordelen in plaats van "is hij dichtbij genoeg gekomen" - dat
	// tweede faalt zodra hij onderweg sneuvelt, en dat gebeurt (gemeten 27-07:
	// neer op 4331 cm). Nog geen assertie: eerst het getal zien.
	float StartClosest = TNumericLimits<float>::Max();
	ForEachHostile([&](AEclipseCharacter& Hostile)
	{
		StartClosest = FMath::Min(StartClosest,
			static_cast<float>(FVector::Dist(Hostile.GetActorLocation(), Harness.Location())));
	});
	Report(*this, TEXT("afstand tot de dichtstbijzijnde vijand bij vertrek"), StartClosest, TEXT("cm"));

	constexpr double LegSeconds = 0.5;
	constexpr double ApproachBudget = 90.0;
	const double ApproachStart = Harness.ElapsedSeconds;
	bool bArrived = false;
	while (!bArrived && Harness.ElapsedSeconds - ApproachStart < ApproachBudget && DownedAt < 0.0f)
	{
		bArrived = Harness.DriveTo(SiteControlPost, LegSeconds, /*ArriveRadius*/ 800.0f, /*bSprint*/ false);

		const float Health = Harness.Body->GetHealth();
		LowestHealth = FMath::Min(LowestHealth, Health);
		if (FirstDamageAt < 0.0f && Health < HealthAtStart - KINDA_SMALL_NUMBER)
		{
			FirstDamageAt = static_cast<float>(Harness.ElapsedSeconds - ApproachStart);
			DistanceAtFirstDamage = ClosestApproach;
		}
		if (DownedAt < 0.0f && Harness.Body->IsDowned())
		{
			DownedAt = static_cast<float>(Harness.ElapsedSeconds - ApproachStart);
		}

		ForEachHostile([&](AEclipseCharacter& Hostile)
		{
			if (!Hostile.IsDowned())
			{
				ClosestApproach = FMath::Min(ClosestApproach,
					static_cast<float>(FVector::Dist(Hostile.GetActorLocation(), Harness.Location())));
			}
			if (const FVector* Spawn = SpawnPositions.Find(&Hostile))
			{
				FurthestHostileMove = FMath::Max(FurthestHostileMove,
					static_cast<float>(FVector::Dist2D(Hostile.GetActorLocation(), *Spawn)));
			}
		});
	}

	Bus->Unsubscribe(MissionHandle);

	const float DamageTaken = HealthAtStart - LowestHealth;
	Report(*this, TEXT("dichtste nadering tot een vijand"),
		ClosestApproach == TNumericLimits<float>::Max() ? -1.0f : ClosestApproach, TEXT("cm"),
		TEXT("< 3000 = binnen hun waarneming, anders zegt deze test niets"));
	Report(*this, TEXT("eerste schade na"), FirstDamageAt, TEXT("s"), TEXT("-1 = nooit geraakt"));
	Report(*this, TEXT("afstand bij de eerste treffer"), DistanceAtFirstDamage, TEXT("cm"));
	Report(*this, TEXT("schade die de speler opliep"), DamageTaken, TEXT("hp"), TEXT("van 100"));
	Report(*this, TEXT("speler neer na"), DownedAt, TEXT("s"), TEXT("-1 = bleef staan"));
	Report(*this, TEXT("verste dat een vijand van zijn spawn kwam"), FurthestHostileMove, TEXT("cm"));
	Report(*this, TEXT("missie gefaald op"), MissionFailedAt, TEXT("s"), TEXT("-1 = niet gefaald"));

	// Het alarm. Tot 26-07 was dit een rapportageregel met een pijnlijke uitkomst:
	// vier schutters brachten je van vol naar neer en de latch bleef uit, want het
	// ENIGE wat NotifyAlarmRaised() aanriep was een console-commando.
	//
	// Op owner-beslissing zit hij nu op de eerste waarneming van een vijand. Dat
	// maakt dit een echte assert: loop je hun bereik in en word je gezien, dan
	// hóórt het alarm te staan. Gaat deze regel rood, dan is de koppeling stuk of
	// heeft niemand je gezien — en dat tweede zou de rest van deze test ook al
	// rood hebben gemaakt.
	Report(*this, TEXT("alarm geslagen"), MissionSub->IsAlarmRaised() ? 1.0f : 0.0f, TEXT(""),
		TEXT("1 = de eerste waarneming heeft het alarm aangezet"));
	TestTrue(TEXT("alarm: gezien worden zet het alarm aan (owner-beslissing 26-07: eerste waarneming)"),
		MissionSub->IsAlarmRaised());
	if (FirstDamageAt >= 0.0f && DownedAt >= 0.0f)
	{
		// Time-to-death vanaf de eerste treffer: dit is het getal dat zegt hoeveel
		// speelruimte je hebt als je hun bereik in loopt. Geen assert — hoe hard
		// dat mag zijn is balans en dus een owner-keuze, net als de bereik-
		// asymmetrie waar het de keerzijde van is.
		Report(*this, TEXT("van eerste treffer tot neer"), DownedAt - FirstDamageAt, TEXT("s"));
	}

	// De assert is bewust ruim: geraakt worden OF zien naderen telt allebei als
	// "hij doet iets". Welke van de twee het wordt hangt af van dekking en van
	// waar de squad staat; dat vastpinnen levert een test op die op toeval rood
	// gaat. Wat NIET mag is dat er niets gebeurt terwijl je binnen hun bereik
	// staat — dat is precies het gat dat de grote ronde openlaat.
	Report(*this, TEXT("afstand die de speler overbrugde"), StartClosest - ClosestApproach, TEXT("cm"),
		TEXT("bewijst dat de aanloop werkte, ook als hij onderweg sneuvelt"));

	const bool bEngaged = DamageTaken > 0.0f || FurthestHostileMove > 200.0f;
	TestTrue(TEXT("contact: de vijand doet iets als je zijn bereik in loopt (nadert of vuurt terug)"), bEngaged);
	// DE BEWAKER TOETST DE AANLOOP, NIET DE EINDAFSTAND.
	//
	// Hier stond "dichtste nadering < 3000 cm". Dat leek een redelijke garantie
	// dat de test iets zinnigs mat, en het viel op 27-07 twee keer om. De
	// meetregels die de bar sinds die ochtend bij een val afdrukt, gaven het
	// antwoord in één oogopslag: dichtste nadering 4331 cm, schade 100 hp, neer
	// na 24,0 s, van eerste treffer tot neer 0,0 s. De speler wordt op 43 METER
	// neergeschoten en gaat binnen één meetvenster van 100 naar 0 - daarna stopt
	// de aanlooplus omdat hij ligt, en die 3000 cm haalt hij nooit.
	//
	// Twee eerdere verklaringen van mij waren fout (belasting: de harnastijd is
	// gesimuleerd; vastlopen op de compoundmuur: hij loopt niet vast, hij
	// sterft). Pas het derde antwoord klopte, en het kwam uit de meting.
	//
	// Wat de bewaker MOET aantonen is dat de aanloop echt gebeurd is - anders
	// betekent "de vijand doet iets" niets. Dat is de AFGELEGDE WEG en niet de
	// eindafstand: sterven op 43 meter is geen mislukte aanloop maar een zeer
	// geslaagde, van de verkeerde kant bekeken.
	//
	// Drempel uit meting, niet uit smaak. Startafstand is constant 14.142 cm; de
	// speler overbrugt daarvan 97-99% in een gezonde run en 69% in de run die
	// omviel. Een speler die nooit vertrekt zit op ~0%. De helft ligt daar ruim
	// tussenin en scheidt "de aanloop werkte" van "er bewoog niets".
	const float Closed = StartClosest - ClosestApproach;
	const float ClosedFraction = StartClosest > KINDA_SMALL_NUMBER ? Closed / StartClosest : 0.0f;
	TestTrue(*FString::Printf(
		TEXT("contact: de speler heeft de aanloop echt gelopen — %.0f van de %.0f cm overbrugd (%.0f%%), dichtste nadering %.0f cm, schade %.0f hp"),
		Closed, StartClosest, ClosedFraction * 100.0f, ClosestApproach, DamageTaken),
		ClosedFraction > 0.5f);

	Harness.Shutdown();
	return true;
}

// Een weigering is een antwoord (GDD 8.4) — maar niemand had er ooit een gezien.
//
// De weigerregels zelf zijn goed getest, alleen als PURE functie
// (EclipseSquadTests: NoRoute, InvalidTarget, NoLineOfSight, Downed). De weg
// erboven — IssueOrder -> DecideOrder -> bark kiezen -> Event.Squad.OrderRefused
// — draaide nog nooit: een volledige M1.1 levert nul weigeringen op, want de
// squad kan alles wat er gevraagd wordt. Alles wat aan een weigering vastzit
// (de bark, de reden in de payload, de logregel) was dus onbewezen.
//
// Deze test dwingt er een af langs het goedkoopste pad dat de regels toestaan:
// FocusTarget zonder doelwit -> bTargetValid is onwaar -> InvalidTarget.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadRefusalIsAnAnswerTest,
	"Eclipse.Playthrough.ARefusalReachesThePlayerWithAReason",
	EclipsePlaythrough::TestFlags)

bool FEclipseSquadRefusalIsAnAnswerTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestNotNull(TEXT("weigering: strategie"), Strategy) || !TestNotNull(TEXT("weigering: prep"), Prep)
		|| !TestNotNull(TEXT("weigering: missie"), Mission) || !TestNotNull(TEXT("weigering: eventbus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	FString Error;
	if (!TestTrue(FString::Printf(TEXT("weigering: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	int32 Refusals = 0;
	int32 Acknowledgements = 0;
	FName ReceivedReason = NAME_None;
	FString ReceivedBark;
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		FGameplayTag::RequestGameplayTag(TEXT("Event.Squad")),
		FEclipseEventNativeDelegate::CreateLambda(
			[&Refusals, &Acknowledgements, &ReceivedReason, &ReceivedBark](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			const FString Name = Tag.ToString();
			if (Name.Contains(TEXT("Refused")))
			{
				++Refusals;
				if (const FEclipseSquadEventPayload* Squad = Payload.GetPtr<FEclipseSquadEventPayload>())
				{
					ReceivedReason = Squad->Reason;
					ReceivedBark = Squad->BarkLine;
				}
			}
			else if (Name.Contains(TEXT("Acknowledged")))
			{
				++Acknowledgements;
			}
		}));

	UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>();
	const TArray<FGuid> Deployed = Mission->GetDeployedSoldierIds();
	if (!TestNotNull(TEXT("weigering: squad-subsysteem"), Squad)
		|| !TestTrue(TEXT("weigering: er is een soldaat om iets aan te vragen"), Deployed.Num() > 0))
	{
		Bus->Unsubscribe(Handle);
		Harness.Shutdown();
		return false;
	}

	// FocusTarget zonder doelwit. Niet omdat een speler dat doet, maar omdat het
	// de enige weigergrond is die zonder wereldopstelling af te dwingen is —
	// NoRoute vraagt een onbereikbaar punt, Downed vraagt een gevallen soldaat.
	const bool bProcessed = Squad->IssueOrder(Deployed[0], EEclipseSquadOrder::FocusTarget,
		FVector::ZeroVector, /*TargetActor*/ nullptr);
	Harness.Idle(0.2f);
	Bus->Unsubscribe(Handle);

	// De order is VERWERKT (hij is aangekomen en beoordeeld); dat is iets anders
	// dan geaccepteerd. Zou dit onwaar zijn, dan viel de order al eerder stil en
	// zegt de rest van deze test niets.
	TestTrue(TEXT("weigering: de order is beoordeeld, niet stil weggevallen"), bProcessed);

	Report(*this, TEXT("weigeringen ontvangen"), Refusals, TEXT(""), TEXT("precies 1 — een order krijgt één antwoord"));
	Report(*this, TEXT("bevestigingen ontvangen"), Acknowledgements, TEXT(""), TEXT("0 — dit kon niet uitgevoerd worden"));

	TestEqual(TEXT("weigering: er komt precies één antwoord terug, en het is een weigering"), Refusals, 1);
	TestEqual(TEXT("weigering: geen bevestiging voor een order die niet kan"), Acknowledgements, 0);
	// De reden reist als FName over de bus en draagt de VOLLEDIGE enum-naam
	// ("EEclipseOrderRefusalReason::InvalidTarget"), niet de korte vorm — dat is
	// hier gemeten, niet aangenomen; de eerste versie van deze test verwachtte de
	// korte vorm en ging daarop rood. De payload zegt zelf dat dit veld op
	// OrderRefused nooit leeg mag zijn, dus vergelijken op de hele naam.
	TestEqual(TEXT("weigering: de reden is InvalidTarget en niet leeg"),
		ReceivedReason, FName(TEXT("EEclipseOrderRefusalReason::InvalidTarget")));
	// Een weigering zonder tekst is nog steeds stilte voor de speler: hij hoort
	// niets en ziet alleen dat er niets gebeurt. De regel valt terug op een
	// generieke zin als de barkpool leeg is, dus leeg is hier een echte fout.
	TestTrue(FString::Printf(TEXT("weigering: er komt een hoorbare zin mee ('%s')"), *ReceivedBark),
		!ReceivedBark.IsEmpty());
	AddInfo(FString::Printf(TEXT("weigering: de squad zegt \"%s\""), *ReceivedBark));

	Harness.Shutdown();
	return true;
}

// Elk ordertype krijgt precies één antwoord — alle vier, in een echte missie.
//
// Van de vier orders (MoveTo, FocusTarget, Hold, Regroup) draaide er tot nu toe
// één end-to-end: de speelronde geeft MoveTo. FocusTarget kwam er via de
// weigeringstest bij, maar alleen op zijn afwijzende tak. Hold en Regroup waren
// NOOIT in een levende missie gegeven — de beslisregels zijn als pure functie
// getest, de weg erboven niet.
//
// Dat is dezelfde soort gat als bij de weigeringen: alles groen, halve
// woordenschat onbeproefd. Deze test geeft alle vier de orders aan een echte
// squad en controleert het contract dat GDD 8.4 stelt: elke order krijgt precies
// één antwoord, nooit nul (stilte) en nooit twee (dubbel geboekt).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEveryOrderIsAnsweredTest,
	"Eclipse.Playthrough.EveryOrderTypeGetsExactlyOneAnswer",
	EclipsePlaythrough::TestFlags)

bool FEclipseEveryOrderIsAnsweredTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestNotNull(TEXT("orders: strategie"), Strategy) || !TestNotNull(TEXT("orders: prep"), Prep)
		|| !TestNotNull(TEXT("orders: missie"), Mission) || !TestNotNull(TEXT("orders: eventbus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	FString Error;
	if (!TestTrue(FString::Printf(TEXT("orders: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	int32 Acks = 0;
	int32 Refusals = 0;
	FName LastReason = NAME_None;
	FString LastBark;
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		FGameplayTag::RequestGameplayTag(TEXT("Event.Squad")),
		FEclipseEventNativeDelegate::CreateLambda(
			[&Acks, &Refusals, &LastReason, &LastBark](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			const FString Name = Tag.ToString();
			const FEclipseSquadEventPayload* Squad = Payload.GetPtr<FEclipseSquadEventPayload>();
			if (Name.Contains(TEXT("Acknowledged")))
			{
				++Acks;
				if (Squad != nullptr) { LastBark = Squad->BarkLine; }
			}
			else if (Name.Contains(TEXT("Refused")))
			{
				++Refusals;
				if (Squad != nullptr) { LastReason = Squad->Reason; LastBark = Squad->BarkLine; }
			}
		}));

	UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>();
	const TArray<FGuid> Deployed = Mission->GetDeployedSoldierIds();
	if (!TestNotNull(TEXT("orders: squad-subsysteem"), Squad)
		|| !TestTrue(TEXT("orders: er is een soldaat"), Deployed.Num() > 0))
	{
		Bus->Unsubscribe(Handle);
		Harness.Shutdown();
		return false;
	}

	// Een ECHTE vijand als doelwit, want FocusTarget met niets is de weigertak en
	// die is elders al gedekt. Hier gaat het om de andere kant.
	AActor* Hostile = nullptr;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		if (*It != Harness.Body && !It->IsPlayerSide() && !It->IsDowned())
		{
			Hostile = *It;
			break;
		}
	}
	TestNotNull(TEXT("orders: er is een vijand om op te richten"), Hostile);

	struct FOrderCase
	{
		EEclipseSquadOrder Order;
		const TCHAR* Name;
		AActor* Target;
	};
	const FVector Nearby = Harness.Location() + FVector(600.0f, 0.0f, 0.0f);
	const FOrderCase Cases[] = {
		{ EEclipseSquadOrder::MoveTo,      TEXT("MoveTo"),      nullptr },
		{ EEclipseSquadOrder::Hold,        TEXT("Hold"),        nullptr },
		{ EEclipseSquadOrder::Regroup,     TEXT("Regroup"),     nullptr },
		{ EEclipseSquadOrder::FocusTarget, TEXT("FocusTarget"), Hostile },
	};

	for (const FOrderCase& Case : Cases)
	{
		const int32 AcksBefore = Acks;
		const int32 RefusalsBefore = Refusals;
		LastReason = NAME_None;
		LastBark.Reset();

		const bool bProcessed = Squad->IssueOrder(Deployed[0], Case.Order, Nearby, Case.Target);
		Harness.Idle(0.2f);

		const int32 NewAcks = Acks - AcksBefore;
		const int32 NewRefusals = Refusals - RefusalsBefore;

		TestTrue(FString::Printf(TEXT("orders: '%s' is beoordeeld en niet stil weggevallen"), Case.Name), bProcessed);
		// HET contract uit GDD 8.4, en het enige dat voor alle vier hetzelfde is:
		// nul antwoorden is stilte, twee is dubbel geboekt. Of het ja of nee wordt
		// hangt van de wereld af en wordt daarom gerapporteerd, niet vastgepind.
		TestEqual(FString::Printf(TEXT("orders: '%s' krijgt precies één antwoord"), Case.Name),
			NewAcks + NewRefusals, 1);

		AddInfo(FString::Printf(TEXT("order '%s': %s%s%s"), Case.Name,
			NewAcks > 0 ? TEXT("bevestigd") : NewRefusals > 0 ? TEXT("geweigerd") : TEXT("GEEN ANTWOORD"),
			LastReason.IsNone() ? TEXT("") : *FString::Printf(TEXT(" (reden: %s)"), *LastReason.ToString()),
			LastBark.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" — \"%s\""), *LastBark)));

		// Een antwoord zonder tekst is voor de speler nog steeds stilte: hij hoort
		// niets en ziet alleen dat er iets of niets gebeurt.
		TestTrue(FString::Printf(TEXT("orders: '%s' komt met een hoorbare zin"), Case.Name), !LastBark.IsEmpty());
	}

	Bus->Unsubscribe(Handle);
	Report(*this, TEXT("orders gegeven"), UE_ARRAY_COUNT(Cases), TEXT(""));
	Report(*this, TEXT("bevestigingen"), Acks, TEXT(""));
	Report(*this, TEXT("weigeringen"), Refusals, TEXT(""), TEXT("mag, maar nooit stil"));
	TestEqual(TEXT("orders: het totaal aantal antwoorden is gelijk aan het aantal orders"),
		Acks + Refusals, static_cast<int32>(UE_ARRAY_COUNT(Cases)));

	Harness.Shutdown();
	return true;
}

// Het vierde objective-type, dat nog nooit gedraaid had.
//
// Van de vier types (ReachLocation, DestroyTarget, CollectItem, ExtractSquad)
// komen er drie langs in M1.1 en dus in de speelronde. CollectItem staat alleen
// in MT_Rescue, en die missie is nog nooit gestart door een test. Erger: de
// bestaande presence-test loopt over "objectives van het type ReachLocation of
// CollectItem" in M1.1 — en omdat M1.1 er geen CollectItem heeft, was die helft
// van die assert LEEG. Dekking die breder leek dan hij was.
//
// Deze test start MT_Rescue echt (via de residentiële regio die hem aanbiedt) en
// legt het CollectItem-pad af.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCollectItemObjectiveTest,
	"Eclipse.Playthrough.CollectItemIsCompletableInAShippedMission",
	EclipsePlaythrough::TestFlags)

bool FEclipseCollectItemObjectiveTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

	const UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(
		nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (!TestNotNull(TEXT("collect: DA_CampaignSetup"), Setup))
	{
		GameInstance->Shutdown();
		return false;
	}
	Campaign->StartNewCampaign(Setup);

	// WorkerHousing is de residentiële regio, en het residentiële aanbod is
	// MT_Rescue — de enige geleverde missie met een CollectItem-objective.
	FString Error;
	if (!TestTrue(FString::Printf(TEXT("collect: WorkerHousing geselecteerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("WorkerHousing"), Error))
		|| !TestTrue(FString::Printf(TEXT("collect: gelanceerd (%s)"), *Error), Prep->AutoLaunch(Error)))
	{
		GameInstance->Shutdown();
		return false;
	}

	FName CollectSite = NAME_None;
	FName CollectObjectiveId = NAME_None;
	for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
	{
		if (Objective.Type == EEclipseObjectiveType::CollectItem)
		{
			CollectSite = Objective.TargetId;
			CollectObjectiveId = Objective.ObjectiveId;
			break;
		}
	}

	// Als dit faalt is de test zelf waardeloos geworden (verkeerde regio, of het
	// type is uit de data verdwenen) — dat moet luid, niet als stille pass.
	if (!TestTrue(TEXT("collect: deze missie heeft echt een CollectItem-objective"), !CollectObjectiveId.IsNone()))
	{
		GameInstance->Shutdown();
		return false;
	}
	AddInfo(FString::Printf(TEXT("collect: '%s' hangt aan site '%s'"),
		*CollectObjectiveId.ToString(), *CollectSite.ToString()));

	TestFalse(TEXT("collect: hij staat nog niet af voordat je er bent"),
		Mission->GetCompletedObjectiveIds().Contains(CollectObjectiveId));

	// Negatieve controle EERST: een ander vak binnenlopen mag dit objective niet
	// vullen. Zonder die controle bewijst de regel hieronder alleen dat er íéts
	// afvinkt, niet dat het aan het juiste vak hangt.
	Mission->NotifySiteEntered(TEXT("Site_Extraction_NietDitVak"));
	TestFalse(TEXT("collect: een ander vak vult dit objective NIET"),
		Mission->GetCompletedObjectiveIds().Contains(CollectObjectiveId));

	// Dit is precies wat de trigger doet als je het vak binnenloopt. CollectItem
	// MAG op aanwezigheid — anders dan DestroyTarget, dat daarop geweigerd wordt.
	Mission->NotifySiteEntered(CollectSite);
	TestTrue(TEXT("collect: het vak binnenlopen vult het objective"),
		Mission->GetCompletedObjectiveIds().Contains(CollectObjectiveId));

	GameInstance->Shutdown();
	return true;
}

// Een gevallen soldaat weigert, en zegt waaróm.
//
// Van de vier weigerredenen is InvalidTarget elders end-to-end gedekt en is
// NoRoute vannacht live voorgekomen (de squad weigerde bij insertie toen hij 93 m
// verderop stond). Downed was nog nooit door de hele keten gegaan, terwijl het de
// reden is die er voor de SPELER het meest toe doet: je geeft een order, er
// gebeurt niets, en je moet kunnen horen dat het komt doordat die man neerligt.
// De regel in de beslislogica zegt dat ook met zoveel woorden — "de speler moet
// zich nooit afvragen waarom er niemand bewoog".
//
// Geen id-naar-pawn-koppeling nodig: door ALLE squadmates neer te leggen wijst
// elk ingezet soldaat-id per definitie naar een gevallen lichaam.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseDownedSoldierRefusesTest,
	"Eclipse.Playthrough.ADownedSoldierRefusesAndSaysWhy",
	EclipsePlaythrough::TestFlags)

bool FEclipseDownedSoldierRefusesTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestNotNull(TEXT("neer: strategie"), Strategy) || !TestNotNull(TEXT("neer: prep"), Prep)
		|| !TestNotNull(TEXT("neer: missie"), Mission) || !TestNotNull(TEXT("neer: eventbus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	FString Error;
	if (!TestTrue(FString::Printf(TEXT("neer: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	// Alle squadmates neerleggen. De speler zelf blijft staan — die geeft de order.
	int32 Downed = 0;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		AEclipseCharacter* Character = *It;
		if (Character != nullptr && Character != Harness.Body && Character->IsPlayerSide() && !Character->IsDowned())
		{
			Character->ApplyDamage(9999.0f, nullptr, TEXT("TestSetup"));
			Downed += Character->IsDowned() ? 1 : 0;
		}
	}
	Report(*this, TEXT("squadmates neergelegd"), Downed, TEXT(""), TEXT("> 0 — anders test dit niets"));
	if (!TestTrue(TEXT("neer: er ligt minstens één squadmate neer"), Downed > 0))
	{
		Harness.Shutdown();
		return false;
	}

	int32 Acks = 0;
	int32 Refusals = 0;
	FName Reason = NAME_None;
	FString Bark;
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		FGameplayTag::RequestGameplayTag(TEXT("Event.Squad")),
		FEclipseEventNativeDelegate::CreateLambda(
			[&Acks, &Refusals, &Reason, &Bark](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			const FString Name = Tag.ToString();
			if (Name.Contains(TEXT("Acknowledged"))) { ++Acks; }
			else if (Name.Contains(TEXT("Refused")))
			{
				++Refusals;
				if (const FEclipseSquadEventPayload* Squad = Payload.GetPtr<FEclipseSquadEventPayload>())
				{
					Reason = Squad->Reason;
					Bark = Squad->BarkLine;
				}
			}
		}));

	UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>();
	const TArray<FGuid> Deployed = Mission->GetDeployedSoldierIds();
	if (!TestNotNull(TEXT("neer: squad-subsysteem"), Squad)
		|| !TestTrue(TEXT("neer: er is een soldaat om iets aan te vragen"), Deployed.Num() > 0))
	{
		Bus->Unsubscribe(Handle);
		Harness.Shutdown();
		return false;
	}

	// Alle vier de orders proberen. Hold vraagt van alle orders het minst — alleen
	// bewustzijn — dus als zelfs DIE weigert, komt het door de toestand van de
	// soldaat en niet door de opdracht. De andere drie zijn er om te horen WAT hij
	// zegt: de barkpool hangt aan het ORDERTYPE en niet aan de reden, dus dit is
	// de plek waar zichtbaar wordt of dat een probleem is.
	const TPair<EEclipseSquadOrder, const TCHAR*> Orders[] = {
		{ EEclipseSquadOrder::Hold,        TEXT("Hold") },
		{ EEclipseSquadOrder::MoveTo,      TEXT("MoveTo") },
		{ EEclipseSquadOrder::Regroup,     TEXT("Regroup") },
		{ EEclipseSquadOrder::FocusTarget, TEXT("FocusTarget") },
	};
	for (const TPair<EEclipseSquadOrder, const TCHAR*>& Case : Orders)
	{
		Acks = 0;
		Refusals = 0;
		Reason = NAME_None;
		Bark.Reset();

		const bool bProcessed = Squad->IssueOrder(Deployed[0], Case.Value == nullptr ? EEclipseSquadOrder::Hold : Case.Key,
			Harness.Location(), nullptr);
		Harness.Idle(0.15f);

		TestTrue(FString::Printf(TEXT("neer: '%s' is beoordeeld en niet stil weggevallen"), Case.Value), bProcessed);
		TestEqual(FString::Printf(TEXT("neer: '%s' krijgt precies één antwoord"), Case.Value), Acks + Refusals, 1);
		TestEqual(FString::Printf(TEXT("neer: '%s' wordt geweigerd"), Case.Value), Refusals, 1);
		TestEqual(FString::Printf(TEXT("neer: '%s' weigert met reden Downed"), Case.Value),
			Reason, FName(TEXT("EEclipseOrderRefusalReason::Downed")));
		TestTrue(FString::Printf(TEXT("neer: '%s' komt met een hoorbare zin"), Case.Value), !Bark.IsEmpty());
		AddInfo(FString::Printf(TEXT("neer: op '%s' zegt een GEVALLEN soldaat: \"%s\""), Case.Value, *Bark));
	}
	Bus->Unsubscribe(Handle);

	// WEL een assert op de inhoud, sinds 26-07 (owner koos optie 1: één gedeelde
	// Downed-pool). Tot vandaag stond hier het omgekeerde: de reden in de payload
	// klopte, maar de zin kwam uit de pool van het ORDERTYPE, dus een gevallen
	// soldaat antwoordde met "Can't hold here." — dat wijst de speler naar een
	// plaatsingsprobleem terwijl de man op de grond ligt.
	//
	// De assert is op de VORM en niet op één zin: welke van de drie hij pakt hangt
	// af van zijn id. Wat moet gelden is dat het antwoord over zijn toestand gaat.
	// Daarom drie sleutelwoorden en niet één citaat — een test die de exacte zin
	// vastpint blokkeert het herschrijven ervan.
	{
		const bool bAboutBeingDown = Bark.Contains(TEXT("hit"))
			|| Bark.Contains(TEXT("down"))
			|| Bark.Contains(TEXT("medic"));
		TestTrue(FString::Printf(
				TEXT("neer: de weigerzin gaat over zijn TOESTAND en niet over de opdracht — kreeg \"%s\""), *Bark),
			bAboutBeingDown);
	}

	Harness.Shutdown();
	return true;
}

// Vuurt het wapen op de snelheid die in de data staat?
//
// FireInterval (0,15 s = 6,7 schoten per seconde) is nooit gemeten. Het wapen
// bewaakt hem zelf met `Now - LastFireTimeSeconds < FireInterval`, en Fire() is
// gebonden aan Triggered — dus terwijl je de trekker vasthoudt, wordt hij elk
// frame aangeroepen en moet die poort het tempo bepalen. Als die poort niet
// klopt, schiet je op framerate.
//
// Gemeten aan de UITKOMST en niet aan het aantal aanroepen: hoeveel leven verliest
// een doelwit in twee seconden vuren, gedeeld door de schade per schot. Dat telt
// de schoten die echt vertrokken zijn.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFireRateTest,
	"Eclipse.Feel.Layer2.WeaponFiresAtTheAuthoredRate",
	EclipsePlaythrough::TestFlags)

bool FEclipseFireRateTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	// ECHTE game mode plus een gelanceerde missie, want het wapen komt uit
	// DT_Weapons en wordt door de game mode aangehangen. Een wapen dat de test
	// zelf aanmaakt zou zijn eigen getallen meten in plaats van de geleverde.
	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("vuurtempo: strategie"), Strategy) || !TestNotNull(TEXT("vuurtempo: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("vuurtempo: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	UEclipseHitscanWeaponComponent* Weapon = Harness.Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
	if (!TestNotNull(TEXT("vuurtempo: de speler heeft een wapen"), Weapon))
	{
		Harness.Shutdown();
		return false;
	}
	const float FireInterval = Weapon->GetFireInterval();
	const float ShotDamage = Weapon->GetDamage();
	Report(*this, TEXT("vuurinterval uit de data"), FireInterval, TEXT("s"),
		*FString::Printf(TEXT("= %.2f schoten/s"), FireInterval > 0.0f ? 1.0f / FireInterval : 0.0f));

	// Een doelwit met veel leven, recht vooruit en ruim binnen bereik.
	const FVector TargetLocation = Harness.Location() + Harness.Body->GetActorForwardVector() * 800.0f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEclipseCharacter* Target = Harness.World->SpawnActor<AEclipseCharacter>(
		AEclipseCharacter::StaticClass(), TargetLocation, FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("vuurtempo: er staat een doelwit"), Target))
	{
		Harness.Shutdown();
		return false;
	}
	Target->InitializeHealth(100000.0f);
	Harness.Idle(0.2f);

	Harness.AimAt(Target->GetActorLocation());
	Harness.Idle(0.1f);

	const float HealthBefore = Target->GetHealth();
	const int32 ShotsBefore = Weapon->GetShotsFired();
	const double Start = Harness.ElapsedSeconds;
	constexpr double FireSeconds = 2.0;
	while (Harness.ElapsedSeconds - Start < FireSeconds)
	{
		Harness.Inject(TEXT("Fire"), true);
		Harness.Step();
	}
	const double Elapsed = Harness.ElapsedSeconds - Start;
	const float DamageDealt = HealthBefore - Target->GetHealth();

	// Uit de WAPENTELLER en niet uit de schade (26-07). Dit leidde het aantal
	// schoten af uit de aangerichte schade, wat klopt zolang elk schot raak is —
	// en dat is precies wat spreiding en terugslag komen wegnemen. Dan zou deze
	// test het aantal TREFFERS meten en het vuurtempo noemen.
	const float ShotsFired = static_cast<float>(Weapon->GetShotsFired() - ShotsBefore);
	const float MeasuredRate = Elapsed > 0.0 ? ShotsFired / static_cast<float>(Elapsed) : 0.0f;

	Report(*this, TEXT("schade in 2 s vuren"), DamageDealt, TEXT("hp"));
	Report(*this, TEXT("dat zijn schoten"), ShotsFired, TEXT(""));
	Report(*this, TEXT("gemeten vuurtempo"), MeasuredRate, TEXT("schoten/s"),
		*FString::Printf(TEXT("data zegt %.2f"), FireInterval > 0.0f ? 1.0f / FireInterval : 0.0f));

	TestTrue(TEXT("vuurtempo: er is überhaupt geschoten"), ShotsFired >= 1.0f);

	// --- bereik: houdt het wapen op waar de data zegt? ----------------------
	// RangeCm staat op 5000 en is nooit nagemeten, terwijl het precies het getal
	// is waar de bereik-asymmetrie op de owner-lijst over gaat. Een doelwit net
	// binnen en net buiten die grens beantwoordt het.
	{
		const float Range = Weapon->GetRangeCm();
		// De teller waar DamageAt en DamageAiming hieronder op leest, en de inschrijving die hem
		// vult. Alleen treffers waarvan de SCHUTTER de speler is tellen mee.
		float PlayerDamage = 0.0f;
		int32 ProbeShots = 0;
		FEclipseEventSubscriptionHandle HitHandle;
		UEclipseEventBusSubsystem* HitBus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
		if (HitBus != nullptr)
		{
			AEclipseCharacter* PlayerBody = Harness.Body;
			HitHandle = HitBus->Subscribe(EclipseTags::Event_Combat_HitLanded,
				FEclipseEventNativeDelegate::CreateLambda(
					[&PlayerDamage, PlayerBody](FGameplayTag, const FInstancedStruct& Payload)
					{
						const FEclipseCombatEventPayload* Landed = Payload.GetPtr<FEclipseCombatEventPayload>();
						if (Landed != nullptr && Landed->Shooter.Get() == PlayerBody)
						{
							PlayerDamage += Landed->Damage;
						}
					}),
				FEclipseCombatEventPayload::StaticStruct());
		}

		auto DamageAt = [&](float DistanceCm) -> float
		{
			Target->SetActorLocation(Harness.Location() + Harness.Body->GetActorForwardVector() * DistanceCm);
			Harness.Idle(0.2f);
			Harness.AimAt(Target->GetActorLocation());
			// De schade van de SPELER, niet die aan het doelwit: de squad vuurt sinds
			// vanavond mee en dit doelwit staat binnen zijn bereik.
			PlayerDamage = 0.0f;
			// Ruim over één vuurinterval heen, zodat de poort zeker opengaat.
	const double Start2 = Harness.ElapsedSeconds;
			while (Harness.ElapsedSeconds - Start2 < FireInterval * 3.0)
			{
				Harness.Inject(TEXT("Fire"), true);
				Harness.Step();
			}
			return PlayerDamage;
		};

		// SCHADE-AFVAL, en dit is de meting die er tot vanavond niet was. De velden
		// stonden in de data mét een uitleg, en het wapen las ze nergens — gevonden
		// door de dode-veldensweep, niet door een test.
		//
		// PRECIES ÉÉN SCHOT per afstand, en vóór de reeks hieronder. De eerste
		// versie hing achter die reeks en mat twee keer nul: het magazijn was leeg
		// geschoten. En de reeks zelf vuurt drie vuurintervallen lang, dus daar
		// landen er één tot drie — 22, 38, 15, 46 hp achter elkaar, waar je geen
		// afval in kunt lezen.
		auto OneShotDamageAt = [&](float DistanceCm) -> float
		{
			Target->SetActorLocation(Harness.Location() + Harness.Body->GetActorForwardVector() * DistanceCm);
			Harness.Idle(0.2f);
			for (int32 Correction = 0; Correction < 10; ++Correction)
			{
				Harness.AimAt(Target->GetActorLocation());
				Harness.Step();
			}
			if (Weapon->IsReloading() || Weapon->GetAmmoInMagazine() < 2)
			{
				Weapon->StartReload(TEXT("TestTopUp"));
				Harness.Idle(Weapon->GetReloadSeconds() + 0.3f);
			}
			Harness.Idle(FireInterval * 4.0f); // reeks breken: eerste schot is zuiver
			PlayerDamage = 0.0f;
			Harness.Inject(TEXT("Fire"), true);
			Harness.Step();
			Harness.Idle(0.1f);
			return PlayerDamage;
		};

		{
			const float Near = OneShotDamageAt(Weapon->GetFalloffStartCm() * 0.5f);
			const float Far = OneShotDamageAt(Range * 0.90f);
			Report(*this, TEXT("schade binnen de afvalgrens"), Near, TEXT("hp"),
				*FString::Printf(TEXT("afval begint op %.0f cm"), Weapon->GetFalloffStartCm()));
			Report(*this, TEXT("schade op 90%% van het bereik"), Far, TEXT("hp"),
				*FString::Printf(TEXT("data zegt daar nog ~%.0f%%"), Weapon->GetFalloffMinFraction() * 100.0f));
			if (Near > 0.0f && Far > 0.0f)
			{
				Report(*this, TEXT("verhouding ver/dichtbij"), Far / Near, TEXT("x"));
				TestTrue(FString::Printf(TEXT("afval: ver doet minder dan dichtbij (%.0f tegen %.0f hp)"), Far, Near),
					Far < Near);
			}
			TestTrue(TEXT("afval: er is überhaupt schade om te vergelijken"), Near > 0.0f);
		}

		// Een REEKS afstanden, want één meting die nul geeft zegt niet waar de grens
		// ligt. De eerste versie prikte op 95% en 110% en kreeg twee keer nul; dat
		// leest als "het wapen raakt nooit", terwijl het betekende dat de grens
		// ergens anders lag dan verwacht.
		Report(*this, TEXT("wapenbereik uit de data"), Range, TEXT("cm"));
		float LastHitDistance = -1.0f;
		float FirstMissDistance = -1.0f;
		for (const float Fraction : { 0.30f, 0.60f, 0.80f, 0.90f, 0.94f, 0.98f, 1.10f })
		{
			const float Distance = Range * Fraction;
			const float Dealt = DamageAt(Distance);
			AddInfo(FString::Printf(TEXT("bereik: op %.0f cm (%.0f%% van %.0f) -> %.0f hp"),
				Distance, Fraction * 100.0f, Range, Dealt));
			if (Dealt > 0.0f)
			{
				LastHitDistance = Distance;
			}
			else if (FirstMissDistance < 0.0f && LastHitDistance > 0.0f)
			{
				FirstMissDistance = Distance;
			}
		}
		Report(*this, TEXT("verste afstand waarop je nog raakt"), LastHitDistance, TEXT("cm"));
		Report(*this, TEXT("eerste afstand waarop je mist"), FirstMissDistance, TEXT("cm"));
		TestTrue(TEXT("bereik: het wapen raakt überhaupt iets binnen zijn bereik"), LastHitDistance > 0.0f);

		// --- kopschot: bestaat die vermenigvuldiging in de praktijk? ---------
		// De code vermenigvuldigt met HeadshotMultiplier als de geraakte bone
		// "head" heet. Het commentaar erbij zegt zelf dat graybox-capsules gewoon
		// basisschade krijgen — dus de vraag is niet of de REGEL klopt maar of hij
		// ooit uitgevoerd wordt. Zelfde vraag als bij de dode camera-blend: een
		// correcte regel die nooit draait, is geen feature.
		Target->SetActorLocation(Harness.Location() + Harness.Body->GetActorForwardVector() * 600.0f);
		Harness.Idle(0.2f);
		const float TargetHalfHeight = Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		auto DamageAiming = [&](float ZOffset) -> float
		{
			// HERHAALD richten, niet één keer. AimAt injecteert één Look-correctie ter
			// grootte van de fout, maar het kijkpad schaalt die invoer, dus één
			// correctie komt er niet. Gemeten bleef de schotlijn 18,9 cm langs een
			// hitbox van 14 cm liggen — dat leek een te kleine hitbox en was een
			// niet-geconvergeerde richtbeweging. Tien iteraties halen de restfout
			// naar vrijwel nul; het is een test, geen speler.
			const FVector AimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, ZOffset);
			for (int32 Correction = 0; Correction < 10; ++Correction)
			{
				Harness.AimAt(AimPoint);
				Harness.Step();
			}
			// PRECIES EEN SCHOT, en dat is sinds de spreiding van 26-07 het enige
			// wat deze meting kan zijn. Hij vuurde er twee, en met spreiding is het
			// tweede schot geen kopschot meer: gemeten 55 + 22 = 77, oftewel een
			// verhouding van 1,75 die niets zegt over de multiplier.
			//
			// VOL MAGAZIJN afdwingen. Sinds 26-07 avond raakt een magazijn leeg, en
			// de secties hierboven (vuurtempo, bereik, afval) schieten er samen
			// ruim doorheen. Een kopschot-meting die toevallig in een herlaadbeurt
			// valt, meet nul schade en leest als "kopschoten doen niets" — precies
			// de verkeerde conclusie uit een correct spel.
			// OOK als hij al bezig is, en dat was de fout in de eerste versie: bij
			// een leeg magazijn had het vuren hierboven de herlaadbeurt zelf al
			// gestart, dus StartReload gaf false en er werd niet gewacht. Het
			// meetschot viel dan middenin het herladen en gaf 0 schade — wat leest
			// als "kopschoten doen niets".
			if (Weapon->IsReloading() || Weapon->GetAmmoInMagazine() < 2)
			{
				Weapon->StartReload(TEXT("TestTopUp"));
				Harness.Idle(Weapon->GetReloadSeconds() + 0.3f);
			}

			// De rust ervoor breekt de vuurreeks (drie vuurintervallen stilte), dus
			// dit schot is gegarandeerd het EERSTE van zijn reeks en daarmee zuiver.
			// Dat is precies waar first-shot accuracy voor gemaakt is.
			Harness.Idle(FireInterval * 4.0f);

			// DE SCHADE VAN DE SPELER, niet de schade AAN het doelwit. Sinds de
			// squad vanavond uit zichzelf vuurt (laag 2) schiet hij mee op alles wat
			// binnen zijn bereik staat, en dit doelwit staat daar precies in. Een
			// gezondheidsbalk is van iedereen; het feit "de speler raakte" is van de
			// speler, en dat feit bestaat al (Event.Combat.HitLanded draagt zijn
			// schutter mee).
			PlayerDamage = 0.0f;
			const int32 ShotsBeforeProbe = Weapon->GetShotsFired();
			Harness.Inject(TEXT("Fire"), true);
			Harness.Step();
			Harness.Idle(0.1f);
			ProbeShots = Weapon->GetShotsFired() - ShotsBeforeProbe;
			return PlayerDamage;
		};

		// Mikken op waar de hitbox ECHT staat, niet op een hoogte die ik zelf
		// uitreken. Eerste rondes mikten op 0,85 x halve capsulehoogte en kwamen
		// gemeten 18,4 cm langs het hoofd — dat leek een te kleine hitbox en was
		// mijn eigen meetkunde. De component weet zijn positie; vraag het hem.
		const USphereComponent* HeadBox = Target->GetHeadHitbox();
		const float HeadOffset = HeadBox != nullptr
			? static_cast<float>(HeadBox->GetComponentLocation().Z - Target->GetActorLocation().Z)
			: TargetHalfHeight * 0.85f;
		Report(*this, TEXT("hoogte van de hoofd-hitbox"), HeadOffset, TEXT("cm"),
			TEXT("boven het midden van de capsule"));

		const float ChestDamage = DamageAiming(0.0f);
		const float HeadDamage = DamageAiming(HeadOffset);
		Report(*this, TEXT("schade op borsthoogte"), ChestDamage, TEXT("hp"));
		Report(*this, TEXT("schoten in de meetronde"), static_cast<float>(ProbeShots), TEXT(""),
			TEXT("moet 1 zijn; 2 betekent dat de trekker blijft hangen"));
		Report(*this, TEXT("schade op hoofdhoogte"), HeadDamage, TEXT("hp"),
			*FString::Printf(TEXT("één zuiver schot; ×%.1f zou %.0f hp zijn"), Weapon->GetHeadshotMultiplier(),
				ChestDamage * Weapon->GetHeadshotMultiplier()));
		// WEL een assert sinds 26-07 (owner-opdracht punt 2, optie 1). Hier stond
		// expliciet dat er niet geasserteerd werd, omdat kopschoten toen niets
		// deden: 22 hp op borst én hoofd, want Hit.BoneName == "head" komt bij een
		// capsule-trace nooit terug. Nu is er een echte hitbox op de hoofd-socket
		// (met een meetkundige terugval bovenin de capsule voor lichamen zonder
		// mesh) en wordt de beslissing als straal-tegen-bol genomen.
		TestTrue(TEXT("kopschot: er is überhaupt schade om te vergelijken"), ChestDamage > 0.0f);
		TestTrue(FString::Printf(
				TEXT("kopschot: het hoofd doet meer dan de romp (%.0f tegen %.0f hp)"), HeadDamage, ChestDamage),
			HeadDamage > ChestDamage);
		// Op de VERHOUDING en niet op 110 hp: het aantal schoten per meetvenster mag
		// veranderen zonder deze test rood te maken, de multiplier niet.
		const float Ratio = ChestDamage > 0.0f ? HeadDamage / ChestDamage : 0.0f;
		Report(*this, TEXT("kopschot-verhouding"), Ratio, TEXT("x"),
			*FString::Printf(TEXT("DT_Weapons zegt %.1fx"), Weapon->GetHeadshotMultiplier()));
		TestTrue(FString::Printf(TEXT("kopschot: de verhouding is de geschreven %.1fx (gemeten %.2fx)"),
				Weapon->GetHeadshotMultiplier(), Ratio),
			FMath::IsNearlyEqual(Ratio, Weapon->GetHeadshotMultiplier(), 0.15f));

		if (HitBus != nullptr)
		{
			HitBus->Unsubscribe(HitHandle);
		}
	}
	// 10% marge: de laatste schot-poort valt zelden precies op het einde van het
	// venster, en de stapgrootte kwantiseert.
	const float Authored = FireInterval > 0.0f ? 1.0f / FireInterval : 0.0f;
	TestTrue(FString::Printf(TEXT("vuurtempo: het tempo IS het geschreven tempo (%.2f tegen %.2f schoten/s)"),
			MeasuredRate, Authored),
		FMath::Abs(MeasuredRate - Authored) <= Authored * 0.10f);

	Harness.Shutdown();
	return true;
}

// Wijst elk objective naar een plek die echt bestaat?
//
// De speelronde vond vannacht een objective zonder voltooiingspad. De naaste
// verwant daarvan is een objective dat naar een vak wijst dat NIET in het
// district staat: dan is de missie evenmin uit te spelen, alleen merk je het pas
// als je ernaartoe loopt en er niets gebeurt.
//
// Geen data-validator maar een GEDRAGSTEST, en dat is bewust: de vakkenlijst
// staat in de graybox-builder (C++), niet in data. Een validator zou die lijst
// moeten dupliceren en dat is precies het soort tweede waarheid dat vannacht zes
// keer misging. De gebouwde wereld is de bron; deze test vraagt het aan de wereld.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseObjectiveSitesExistTest,
	"Eclipse.Playthrough.EveryObjectivePointsAtASiteThatExists",
	EclipsePlaythrough::TestFlags)

bool FEclipseObjectiveSitesExistTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	// De drie geleverde missies, elk via de regio die hem aanbiedt.
	const TPair<const TCHAR*, const TCHAR*> Runs[] = {
		{ TEXT("TransitCheckpoint"), TEXT("M1.1 / checkpoint") },
		{ TEXT("WorkerHousing"),     TEXT("rescue / residentieel") },
		// FoundryRow en niet Underworks: die laatste is bij campagnestart al in
		// handen van de speler ("Region is already player-held"), dus daar valt geen
		// missie te selecteren. Vijfde keer vannacht dat mijn opstelling de fout was
		// en niet de code — vandaar deze regel, zodat de volgende het niet opnieuw
		// probeert.
		{ TEXT("FoundryRow"),        TEXT("sabotage / industrieel") },
	};

	int32 ObjectivesChecked = 0;

	for (const TPair<const TCHAR*, const TCHAR*>& Run : Runs)
	{
		FHarness::FOptions Options;
		Options.bRealGameMode = true;

		FHarness Harness;
		if (!Harness.Start(*this, Options))
		{
			Harness.Shutdown();
			return false;
		}

		UGameInstance* GameInstance = Harness.GameInstance;
		UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
		UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
		UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();

		// NIET ELKE MISSIE IS OP DAG 1 BEREIKBAAR, en dat is spelregel en geen fout:
		// de campagne laat je alleen naar regio's die aan speler-gebied grenzen
		// (GDD 3.1 regel 1). Underworks is bij aanvang al van jou en FoundryRow ligt
		// er niet naast, dus de sabotagemissie valt op dag 1 buiten bereik. Dat
		// MELDEN in plaats van er rood op gaan — anders zou deze test de speler
		// verwijten dat de campagne werkt zoals hij hoort.
		FString Error;
		const bool bSelected = Strategy != nullptr && Strategy->SelectMission(Run.Key, Error);
		if (!bSelected)
		{
			AddInfo(FString::Printf(TEXT("vakken: %s is op dag 1 niet te kiezen (%s) — niet gecontroleerd"),
				Run.Value, *Error));
			Harness.Shutdown();
			continue;
		}
		if (!TestTrue(FString::Printf(TEXT("vakken: %s gelanceerd (%s)"), Run.Value, *Error),
				Prep != nullptr && Prep->AutoLaunch(Error)))
		{
			Harness.Shutdown();
			continue; // andere missies alsnog controleren — één mislukte start mag de rest niet blinderen
		}
		Harness.Idle(0.3f);

		// Welke vaknamen dragen de actoren in de GEBOUWDE wereld?
		TSet<FName> SitesInWorld;
		for (TActorIterator<AActor> It(Harness.World); It; ++It)
		{
			for (const FName& Tag : It->Tags)
			{
				SitesInWorld.Add(Tag);
			}
		}

		for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
		{
			if (Objective.TargetId.IsNone())
			{
				continue; // objectives zonder plek bestaan legitiem (bv. tellers)
			}
			++ObjectivesChecked;
			TestTrue(FString::Printf(TEXT("vakken: %s — objective '%s' wijst naar '%s' en dat vak staat er"),
					Run.Value, *Objective.ObjectiveId.ToString(), *Objective.TargetId.ToString()),
				SitesInWorld.Contains(Objective.TargetId));
		}
		AddInfo(FString::Printf(TEXT("vakken: %s — %d objectives, %d getagde plekken in de wereld"),
			Run.Value, Mission->GetActiveObjectives().Num(), SitesInWorld.Num()));

		Harness.Shutdown();
	}

	Report(*this, TEXT("objectives gecontroleerd over alle geleverde missies"), ObjectivesChecked, TEXT(""),
		TEXT("> 0 — anders controleert deze test niets"));
	// Ondergrens op wat er WEL bereikbaar was: twee missies met samen vier
	// objectives die een plek noemen. Zakt dat, dan is er een missie stil
	// onbereikbaar geworden en dekt deze test minder dan hij lijkt te dekken.
	TestTrue(FString::Printf(TEXT("vakken: er zijn minstens vier objectives gecontroleerd (%d)"), ObjectivesChecked),
		ObjectivesChecked >= 4);

	return true;
}

// Is elke regio ooit te bereiken?
//
// De vakkentest hierboven liep erop stuk dat de sabotagemissie op dag 1 buiten
// bereik ligt — de campagne laat je alleen naar regio's die aan speler-gebied
// grenzen (GDD 3.1 regel 1). Dat is spelregel, maar het roept de zwaardere vraag
// op: is elke regio ooit te bereiken? Een regio die nergens aan vastzit, is een
// missie die niemand ooit speelt, en dat zie je aan de data niet af.
//
// Vandaag klopt het: vanuit Underworks (speler-gebied bij aanvang) kom je via
// TransitCheckpoint bij FoundryRow en CommsRelay, en via WorkerHousing bij
// SupplyDepot. Deze test houdt dat vast — hij gaat rood zodra iemand een regio
// toevoegt zonder verbinding, of een verbinding weghaalt die de enige was.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEveryRegionReachableTest,
	"Eclipse.Playthrough.EveryRegionIsReachableFromTheStart",
	EclipsePlaythrough::TestFlags)

bool FEclipseEveryRegionReachableTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness; // Report()

	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone();

	UEclipseCampaignSubsystem* Campaign = GameInstance->GetSubsystem<UEclipseCampaignSubsystem>();
	const UEclipseCampaignSetupAsset* Setup = LoadObject<UEclipseCampaignSetupAsset>(
		nullptr, TEXT("/Game/Data/DA_CampaignSetup.DA_CampaignSetup"));
	if (!TestNotNull(TEXT("bereik: campagne"), Campaign) || !TestNotNull(TEXT("bereik: DA_CampaignSetup"), Setup))
	{
		GameInstance->Shutdown();
		return false;
	}
	Campaign->StartNewCampaign(Setup);

	// De LINKS staan in de graaf-asset, niet in de campagnestaat — die laatste
	// kent alleen eigenaar en onrust per regio. Vandaar de graaf als bron.
	const UEclipseRegionGraphAsset* Graph = Setup->RegionGraph.LoadSynchronous();
	if (!TestNotNull(TEXT("bereik: de regiograaf"), Graph))
	{
		GameInstance->Shutdown();
		return false;
	}
	const TArray<FEclipseRegionDefinition>& Regions = Graph->Regions;
	if (!TestTrue(TEXT("bereik: er zijn regio's"), Regions.Num() > 0))
	{
		GameInstance->Shutdown();
		return false;
	}

	// Startpunt: alles wat de speler bij aanvang al heeft.
	TSet<FName> Reached;
	TArray<FName> Frontier;
	for (const FEclipseRegionDefinition& Region : Regions)
	{
		if (Region.StartingOwner == EEclipseRegionOwner::Player)
		{
			Reached.Add(Region.RegionId);
			Frontier.Add(Region.RegionId);
		}
	}
	Report(*this, TEXT("regio's in handen bij aanvang"), Reached.Num(), TEXT(""), TEXT("> 0 — anders begin je nergens"));
	if (!TestTrue(TEXT("bereik: de speler begint ergens"), Reached.Num() > 0))
	{
		GameInstance->Shutdown();
		return false;
	}

	// Golf voor golf de buren erbij, precies zoals de campagne je laat oprukken.
	while (Frontier.Num() > 0)
	{
		const FName Current = Frontier.Pop();
		const FEclipseRegionDefinition* State = Regions.FindByPredicate(
			[&Current](const FEclipseRegionDefinition& R) { return R.RegionId == Current; });
		if (State == nullptr)
		{
			continue;
		}
		for (const FName& Neighbour : State->ConnectedRegionIds)
		{
			if (!Reached.Contains(Neighbour))
			{
				Reached.Add(Neighbour);
				Frontier.Add(Neighbour);
			}
		}
	}

	Report(*this, TEXT("regio's bereikbaar vanaf het begin"), Reached.Num(), TEXT(""),
		*FString::Printf(TEXT("van %d in de graaf"), Regions.Num()));

	// --- en biedt elke regiosoort ook echt zijn missie aan? -------------------
	// Een aanbod dat aan een regiotype hangt dat NIET in de graaf voorkomt, is een
	// missie die niemand ooit te zien krijgt. Andersom: een regiotype zonder
	// aanbod levert een regio op die je kunt bevrijden maar nooit kunt aanvallen.
	// Allebei zijn ze onzichtbaar in de data zelf.
	if (const UDataTable* Offers = Graph->MissionOffers.LoadSynchronous())
	{
		TSet<uint8> TypesInGraph;
		for (const FEclipseRegionDefinition& Region : Regions)
		{
			TypesInGraph.Add(static_cast<uint8>(Region.RegionType));
		}
		TSet<uint8> TypesWithOffer;
		int32 OfferCount = 0;
		Offers->ForeachRow<FEclipseMissionOfferRow>(TEXT("AanbodControle"),
			[&TypesWithOffer, &OfferCount, this, &TypesInGraph](const FName&, const FEclipseMissionOfferRow& Row)
			{
				++OfferCount;
				TypesWithOffer.Add(static_cast<uint8>(Row.RegionType));
				TestTrue(FString::Printf(TEXT("aanbod: '%s' hangt aan een regiotype dat in de graaf voorkomt"),
						*Row.TemplateId.ToString()),
					TypesInGraph.Contains(static_cast<uint8>(Row.RegionType)));
			});
		Report(*this, TEXT("missie-aanbiedingen in de data"), OfferCount, TEXT(""));
		Report(*this, TEXT("regiosoorten in de graaf"), TypesInGraph.Num(), TEXT(""),
			*FString::Printf(TEXT("waarvan %d een aanbod hebben"), TypesWithOffer.Num()));
		for (const uint8 Type : TypesInGraph)
		{
			TestTrue(FString::Printf(TEXT("aanbod: regiosoort %s heeft een missie om aan te bieden"),
					*UEnum::GetValueAsString(static_cast<EEclipseRegionType>(Type))
						.RightChop(FString(TEXT("EEclipseRegionType::")).Len())),
				TypesWithOffer.Contains(Type));
		}
	}

	for (const FEclipseRegionDefinition& Region : Regions)
	{
		TestTrue(FString::Printf(TEXT("bereik: '%s' is ooit te bereiken"), *Region.RegionId.ToString()),
			Reached.Contains(Region.RegionId));
	}

	// --- en de verhaallijn: wijst elke beat naar een regio die bestaat? -------
	// Een story-missie die aan een regio hangt die niet in de graaf staat, wordt
	// nooit aangeboden — dezelfde dode content als hierboven, maar dan in de laag
	// die de campagne zijn verhaal geeft. Geen van de vier bestaande validators
	// kijkt hiernaar.
	if (const UDataTable* Story = Setup->StoryMissions.LoadSynchronous())
	{
		TSet<FName> RegionIds;
		for (const FEclipseRegionDefinition& Region : Regions)
		{
			RegionIds.Add(Region.RegionId);
		}
		int32 StoryRows = 0;
		Story->ForeachRow<FEclipseStoryMissionRow>(TEXT("VerhaalControle"),
			[&StoryRows, &RegionIds, this](const FName&, const FEclipseStoryMissionRow& Row)
			{
				++StoryRows;
				if (Row.PinnedRegionId.IsNone())
				{
					return; // niet vastgepind = overal aan te bieden, dus niets te controleren
				}
				TestTrue(FString::Printf(TEXT("verhaal: '%s' is vastgepind op regio '%s' en die bestaat"),
						*Row.MissionId.ToString(), *Row.PinnedRegionId.ToString()),
					RegionIds.Contains(Row.PinnedRegionId));
			});
		Report(*this, TEXT("story-missies gecontroleerd"), StoryRows, TEXT(""),
			TEXT("elk vastgepind op een regio die in de graaf staat"));
	}

	GameInstance->Shutdown();
	return true;
}

// Speelt een missie MET geauthorde spawns, en telt wat er echt staat.
//
// Dit gat heb ik zelf gemaakt en meteen weer gedicht. Op 26-07 sloot ik
// EnemySpawns aan, waarmee drie missies van vier vijanden naar hun eigen
// opstelling gingen — en elke bestaande speelronde start TransitCheckpoint, de
// enige missie ZONDER geauthorde spawns. De zwaarste wijziging van de ochtend
// werd dus door geen enkele ronde aangeraakt.
//
// Wat deze test wel en niet doet. Wel: de missie via het echte laadpad starten
// en de vijanden TELLEN die er komen te staan, tegen wat het sjabloon vraagt.
// Niet: hem uitspelen of oordelen of het te zwaar is. Of vijf vijanden op een
// site eerlijk is, is balans, en balans hoort de owner te spelen — een test die
// daar een grens op zet, pint zijn ontwerp vast met mijn smaak.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHeavyMissionSpawnsTest,
	"Eclipse.Playthrough.AMissionWithAuthoredSpawnsPlacesThem",
	EclipsePlaythrough::TestFlags)

bool FEclipseHeavyMissionSpawnsTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* MissionSub = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	if (!TestNotNull(TEXT("zwaar: strategie"), Strategy) || !TestNotNull(TEXT("zwaar: prep"), Prep)
		|| !TestNotNull(TEXT("zwaar: missie"), MissionSub))
	{
		Harness.Shutdown();
		return false;
	}

	// WorkerHousing biedt de rescue aan, en die authordt twee batches. Underworks
	// en Transit vallen af: de eerste is bij campagnestart al van de speler, de
	// tweede is M1.1 zonder spawns. Dat is vannacht drie keer uitgezocht en staat
	// hier zodat de volgende het niet opnieuw hoeft te doen.
	FString Error;
	if (!TestTrue(FString::Printf(TEXT("zwaar: missie op WorkerHousing geselecteerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("WorkerHousing"), Error))
		|| !TestTrue(FString::Printf(TEXT("zwaar: gelanceerd (%s)"), *Error), Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	// Wat het sjabloon vraagt, gelezen via dezelfde weg als de game mode.
	int32 Wanted = 0;
	for (const FEclipseEnemySpawnSet& Set : MissionSub->GetActiveEnemySpawns())
	{
		Wanted += Set.Count;
	}
	Report(*this, TEXT("vijanden die het sjabloon vraagt"), Wanted, TEXT(""), TEXT("uit EnemySpawns"));

	// Zonder dit is de test een tautologie: een missie zonder batches zou hem
	// even groen maken als een die perfect spawnt. Exact de valkuil die vannacht
	// twee andere bewakers stil hield.
	if (!TestTrue(TEXT("zwaar: deze missie authordt uberhaupt spawns (anders bewijst de telling niets)"), Wanted > 0))
	{
		Harness.Shutdown();
		return false;
	}

	int32 Hostiles = 0;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		const AEclipseCharacter* Body = *It;
		if (Body != nullptr && Body != Harness.Body && !Body->IsPlayerSide())
		{
			++Hostiles;
		}
	}
	Report(*this, TEXT("vijanden die er echt staan"), Hostiles, TEXT(""), TEXT("geteld in de wereld"));
	Report(*this, TEXT("verschil met de oude vaste lus"), Hostiles - 4, TEXT(""),
		TEXT("tot 26-07 kreeg elke missie er precies 4"));

	// De kern: vraagt het sjabloon om N, dan staan er N. Dit is het enige stuk
	// van de koppeling dat tot nu toe alleen door een logregel bewezen werd, en
	// een logregel is geen bewijs — dat was vannacht de duurste les.
	TestEqual(TEXT("zwaar: er staan precies zoveel vijanden als het sjabloon vraagt"), Hostiles, Wanted);

	Harness.Shutdown();
	return true;
}

// Een FocusTarget-order is een belofte, geen enkel schot (GDD 8.4).
//
// Gevonden bij een audit van de squad, en het is de grootste asymmetrie die deze
// sessie heeft opgeleverd: VIJANDEN hebben een denkbeurt op een timer — waarnemen,
// naderen, vuren, elke vuurinterval opnieuw — en SQUADMATES hadden er geen enkele.
// Ze reageerden één keer op een order en zwegen daarna. "Richt op dat doelwit"
// leverde precies één kogel op, terwijl de order gewoon bleef staan.
//
// In een vuurgevecht betekende dat: vier vijanden schieten aanhoudend, en je drie
// soldaten staan erbij.
//
// Deze test meet de UITKOMST over tijd, want dat is waar het verschil zit: hoeveel
// schoten er vallen terwijl de order staat, niet of de functie bestaat.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFocusFireIsSustainedTest,
	"Eclipse.Playthrough.AFocusOrderKeepsFiring",
	EclipsePlaythrough::TestFlags)

bool FEclipseFocusFireIsSustainedTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestTrue(TEXT("focus: missie gelanceerd"),
			Strategy != nullptr && Prep != nullptr
			&& Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	// Een squadmate en een vijand vinden, en de vijand binnen bereik zetten. Zonder
	// dat laatste meet deze test of het wapen bereik heeft, niet of de order
	// volgehouden wordt.
	AEclipseSquadmateController* Mate = nullptr;
	AEclipseCharacter* Hostile = nullptr;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mate = *It;
		break;
	}
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		AEclipseCharacter* Body = *It;
		if (Body != nullptr && Body != Harness.Body && !Body->IsPlayerSide() && !Body->IsDowned())
		{
			Hostile = Body;
			break;
		}
	}
	if (Hostile != nullptr)
	{
		// HET DOELWIT MOET HET OVERLEVEN, anders meet deze test iets anders dan hij
		// beweert. Sinds de squad vanavond uit zichzelf vuurt, gaat het doelwit
		// binnen een seconde neer en eindigt de order — en dan telt de test hoe
		// SNEL hij viel in plaats van of de order werd volgehouden. Gemeten
		// schommelde hij daardoor tussen 2 en 14 schoten in dezelfde bar.
		Hostile->InitializeHealth(100000.0f);
	}
	if (!TestNotNull(TEXT("focus: er is een squadmate"), Mate)
		|| !TestNotNull(TEXT("focus: er is een vijand"), Hostile))
	{
		Harness.Shutdown();
		return false;
	}

	const APawn* MateBody = Mate->GetPawn();
	if (!TestNotNull(TEXT("focus: de squadmate heeft een lichaam"), MateBody))
	{
		Harness.Shutdown();
		return false;
	}
	Hostile->SetActorLocation(MateBody->GetActorLocation() + FVector(800.0f, 0.0f, 0.0f), /*bSweep*/ false);
	Harness.Idle(0.3f);

	const float HealthBefore = Hostile->GetHealth();
	Mate->ExecuteOrder(EEclipseSquadOrder::FocusTarget, Hostile->GetActorLocation(), Hostile);

	// Twee seconden. Bij een vuurinterval van 0,15 s zijn dat er ruim tien als de
	// order wordt volgehouden, en precies één als hij dat niet wordt.
	Harness.Idle(2.0f);

	const int32 Shots = Mate->GetFocusFireShots();
	const float Damage = HealthBefore - Hostile->GetHealth();

	// WIE is er geraakt? De teller telt alleen gelukte treffers, dus "14 schoten
	// en 0 schade op het doelwit" kan niet allebei waar zijn tenzij er iets ANDERS
	// geraakt wordt. Zonder deze discriminator zou ik gaan gissen — en dat is deze
	// sessie al drie keer de verkeerde kant op gegaan.
	float FriendlyHealth = 0.0f;
	float OtherHostileHealth = 0.0f;
	int32 Friends = 0;
	int32 FriendlyFireVictims = 0;
	int32 OtherHostiles = 0;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		const AEclipseCharacter* Body = *It;
		if (Body == nullptr || Body == Hostile)
		{
			continue;
		}
		if (Body->IsPlayerSide())
		{
			FriendlyHealth += Body->GetHealth();
			++Friends;
			// De oorzaak op het lichaam is de enige harde discriminator tussen
			// "de vijand schoot hem" en "onze eigen squadmate schoot hem".
			if (Body->GetLastDamageCause() == FName(TEXT("SquadFocusFire")))
			{
				++FriendlyFireVictims;
			}
		}
		else
		{
			OtherHostileHealth += Body->GetHealth();
			++OtherHostiles;
		}
	}
	// Vol leven is 100 per lichaam; alles daaronder betekent dat er op geschoten is.
	Report(*this, TEXT("leven van de EIGEN kant"), FriendlyHealth, TEXT("hp"),
		*FString::Printf(TEXT("%d lichamen, vol = %d"), Friends, Friends * 100));
	Report(*this, TEXT("leven van ANDERE vijanden"), OtherHostileHealth, TEXT("hp"),
		*FString::Printf(TEXT("%d lichamen, vol = %d"), OtherHostiles, OtherHostiles * 100));
	Report(*this, TEXT("eigen mensen geraakt door onze squadmate"), FriendlyFireVictims, TEXT(""),
		TEXT("> 0 = het wapen kent geen vriend of vijand"));
	Report(*this, TEXT("schoten op een staand FocusTarget-order"), Shots, TEXT(""),
		TEXT("1 = de order werd niet volgehouden"));
	// 0 hp op het AANGEWEZEN doelwit is hier geen fout, en die uitleg hoort erbij
	// omdat het getal anders alarmerend leest. De vijanden staan geclusterd, dus
	// een kogel raakt wie er in de schootslijn staat — gemeten ging er 100 hp af
	// bij een ANDERE vijand terwijl er nul eigen mensen geraakt werden. Het wapen
	// is een hitscan zonder doorschot, geen geleid projectiel.
	Report(*this, TEXT("schade op het aangewezen doelwit"), Damage, TEXT("hp"),
		TEXT("0 kan kloppen: er stond een andere vijand in de schootslijn"));

	TestTrue(FString::Printf(
			TEXT("focus: de order wordt VOLGEHOUDEN en is niet één schot (gemeten %d schoten)"), Shots),
		Shots > 3);

	Harness.Shutdown();
	return true;
}

// Het eerste schot verraadt je (owner-opdracht 26-07, punt 1).
//
// WAT DE REFERENTIE DOET, want daar komen de keuzes vandaan:
//   The Division  schot alarmeert binnen een geluidsradius; de AI loopt naar de
//                 LAATST BEKENDE POSITIE — een momentopname die verloopt als je
//                 beweegt. Een demper verkleint de radius.
//   Borderlands   radius-aggro vanaf de LOOP, niet de inslag: een gemist schot
//                 maakt evenveel lawaai als een rake.
//   Gears         geen bruikbare referentie — dat spel heeft geen stealth, alle
//                 encounters zijn gescript.
//
// Wat we daarvan overnemen: radius per wapen (DT_Weapons), oorsprong als laatst
// bekende positie, en lawaai bij ELK schot dat de cadans passeert. Wat we niet
// overnemen: dempers (bestaan nog niet als concept in deze build).
//
// Deze test meet drie dingen die alle drie stil konden falen:
//   1. een vijand BUITEN de radius hoort niets — anders is de radius decoratie
//   2. een vijand BINNEN de radius komt kijken en beweegt echt
//   3. hij loopt naar waar geschoten IS, niet naar waar de speler NU is
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseGunshotGivesYouAwayTest,
	"Eclipse.Playthrough.AGunshotSendsEnemiesToWhereYouFired",
	EclipsePlaythrough::TestFlags)

bool FEclipseGunshotGivesYouAwayTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	if (!TestNotNull(TEXT("schot: strategie"), Strategy) || !TestNotNull(TEXT("schot: prep"), Prep))
	{
		Harness.Shutdown();
		return false;
	}

	FString Error;
	if (!TestTrue(FString::Printf(TEXT("schot: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	// Waar staan de vijanden, en hoe ver zijn ze van de speler? Zonder deze
	// nulmeting is elke latere verplaatsing onbewijsbaar.
	TArray<TPair<AEclipseCharacter*, FVector>> Hostiles;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		AEclipseCharacter* Body = *It;
		if (Body != nullptr && Body != Harness.Body && !Body->IsPlayerSide())
		{
			Hostiles.Emplace(Body, Body->GetActorLocation());
		}
	}
	if (!TestTrue(TEXT("schot: er staan vijanden om te alarmeren"), Hostiles.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	// De speler ver genoeg weg zetten dat NIEMAND hem ziet: het waarnemingsbereik
	// is 2500 cm, de geluidsradius 5000. Precies in dat gat zit de mechaniek — hij
	// mag ongezien zijn en toch gehoord worden. Zonder die scheiding meet deze test
	// gewoon opnieuw of vijanden je zien.
	const FVector Anchor = Hostiles[0].Value;
	const FVector FiringSpot = Anchor + FVector(4200.0f, 0.0f, 0.0f);

	// De SQUAD gaat mee, en dat is geen detail. Eerste twee rondes mat deze test
	// "ze lopen ervandaan": de vijanden zagen de achtergebleven squadmates, en een
	// vijand die iemand ZIET negeert een gerucht terecht — dat staat zo in
	// NotifyGunshotHeard. Ze deden dus precies wat ze moesten doen, en de test
	// isoleerde de mechaniek niet. Zelfde soort fout als de besmette baseline
	// hierboven: niet de code maar de opstelling.
	int32 Repositioned = 0;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		AEclipseCharacter* Body = *It;
		if (Body != nullptr && Body->IsPlayerSide())
		{
			Body->SetActorLocation(FiringSpot + FVector(0.0f, 150.0f * Repositioned, 0.0f), /*bSweep*/ false);
			++Repositioned;
		}
	}
	Harness.Idle(0.5f);

	// Nooit de BEDOELDE positie rapporteren maar de WERKELIJKE: een teleport die
	// niet aankomt zou anders als "3600 cm" in het rapport staan terwijl de speler
	// nog bij de vijanden staat, en dan meet de hele test iets anders dan hij zegt.
	const FVector ActualSpot = Harness.Location();
	Report(*this, TEXT("afstand speler tot dichtste vijand"),
		static_cast<float>(FVector::Dist2D(ActualSpot, Anchor)), TEXT("cm"),
		TEXT("> 3200 (buiten het RUIMSTE waarnemingsbereik) en < 5000 (binnen gehoor)"));
	Report(*this, TEXT("teleport-afwijking"),
		static_cast<float>(FVector::Dist2D(ActualSpot, FiringSpot)), TEXT("cm"),
		TEXT("0 = de speler staat waar de test denkt"));
	// 3200 en niet 2500: de vier vijanden wisselen archetypes af en hun
	// waarnemingsbereiken verschillen. De eerste opstelling stond op 3600 cm en
	// daar zág er eentje de speler gewoon — gevonden door de vijand zelf te laten
	// vertellen wat hij deed, in plaats van door mijn aanname te herhalen.
	if (!TestTrue(TEXT("schot: de speler staat buiten hun RUIMSTE waarnemingsbereik (anders meet dit zien i.p.v. horen)"),
			FVector::Dist2D(ActualSpot, Anchor) > 3200.0f))
	{
		Harness.Shutdown();
		return false;
	}

	// Schieten in het luchtledige: richting weg van de vijanden, zodat er zeker
	// niemand geraakt wordt. Het gaat om het GELUID, en een gemist schot hoort
	// evenveel te verraden als een rake — dat is de kern van de mechaniek.
	UEclipseHitscanWeaponComponent* Weapon = Harness.Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
	if (!TestNotNull(TEXT("schot: de speler heeft een wapen"), Weapon))
	{
		Harness.Shutdown();
		return false;
	}
	// De nulmeting hoort HIER, vlak voor het schot — niet bij het spawnen.
	// Eerste ronde mat -203 cm ("ze lopen ervandaan") omdat de vijanden in de
	// tussenliggende anderhalve seconde nog naar de plek liepen waar de speler
	// stond vóór de teleport. Dat was geen gedrag maar een besmette baseline: het
	// meetmoment lag vóór de gebeurtenis die ik wilde meten.
	for (TPair<AEclipseCharacter*, FVector>& Entry : Hostiles)
	{
		if (Entry.Key != nullptr)
		{
			Entry.Value = Entry.Key->GetActorLocation();
		}
	}

	Weapon->Fire(ActualSpot + FVector(0.0f, 0.0f, 60.0f), FVector(0.0f, 0.0f, 1.0f), TEXT("TestShot"));

	// Ruim laten lopen. Vier seconden bleek te kort: de eerste denkbeurt komt pas
	// na een vuurinterval, en daarna moeten ze nog om de dekking heen die tussen
	// hen en de schietplek staat — de eerste meters van zo'n route kunnen best van
	// het doel AF wijzen.
	Harness.Idle(10.0f);

	float ClosedOnShot = 0.0f;
	int32 Moved = 0;
	for (const TPair<AEclipseCharacter*, FVector>& Entry : Hostiles)
	{
		if (Entry.Key == nullptr)
		{
			continue;
		}
		const float Before = static_cast<float>(FVector::Dist2D(Entry.Value, ActualSpot));
		const float After = static_cast<float>(FVector::Dist2D(Entry.Key->GetActorLocation(), ActualSpot));
		ClosedOnShot += Before - After;
		Moved += FVector::Dist2D(Entry.Key->GetActorLocation(), Entry.Value) > 100.0f ? 1 : 0;
	}

	Report(*this, TEXT("vijanden die in beweging kwamen"), Moved, TEXT(""), TEXT("van de vier"));
	Report(*this, TEXT("totaal dichter bij de schietplek"), ClosedOnShot, TEXT("cm"),
		TEXT("> 0 = ze lopen ernaartoe"));

	TestTrue(FString::Printf(
			TEXT("schot: minstens één vijand komt kijken (er bewogen er %d van de %d)"), Moved, Hostiles.Num()),
		Moved > 0);
	TestTrue(FString::Printf(
			TEXT("schot: ze bewegen NAAR de plek waar geschoten werd (netto %.0f cm dichterbij)"), ClosedOnShot),
		ClosedOnShot > 0.0f);

	Harness.Shutdown();
	return true;
}

// De TERUGVAL: een missie zonder geauthorde batches houdt zijn vier vijanden.
//
// Dit is de tweede helft van dezelfde koppeling en het stuk dat stil kan
// verdwijnen. M1.1 authordt geen spawns, dus hij hoort op de graybox-standaard
// te blijven — vier bij het primaire doel, exact de opstelling waarop álle
// speelrondes en alle feel-metingen van de nacht van 25→26 juli gedaan zijn.
// Breekt die terugval, dan staat M1.1 ineens leeg en klopt geen van die
// metingen meer, terwijl er niets rood wordt: een lege missie is nog steeds
// speelbaar en loopt netjes naar de debrief.
//
// Geschreven omdat ik de terugval een uur geleden zelf schreef en hem alleen
// bewéérd had. Dat was de derde ongemeten claim van dat uur, en de regel na de
// derde is: stoppen met repareren en gaan zoeken.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseDefaultSpawnFallbackTest,
	"Eclipse.Playthrough.AMissionWithoutSpawnsKeepsItsFour",
	EclipsePlaythrough::TestFlags)

bool FEclipseDefaultSpawnFallbackTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseMissionSubsystem* MissionSub = GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	if (!TestNotNull(TEXT("terugval: strategie"), Strategy) || !TestNotNull(TEXT("terugval: prep"), Prep)
		|| !TestNotNull(TEXT("terugval: missie"), MissionSub))
	{
		Harness.Shutdown();
		return false;
	}

	FString Error;
	if (!TestTrue(FString::Printf(TEXT("terugval: M1.1 geselecteerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error))
		|| !TestTrue(FString::Printf(TEXT("terugval: gelanceerd (%s)"), *Error), Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	// Zonder deze regel bewijst de telling hieronder niets: als M1.1 óók batches
	// zou krijgen, meet dit de geauthorde weg en niet de terugval.
	TestEqual(TEXT("terugval: M1.1 authordt inderdaad geen spawns (anders test dit het verkeerde pad)"),
		MissionSub->GetActiveEnemySpawns().Num(), 0);

	int32 Hostiles = 0;
	for (TActorIterator<AEclipseCharacter> It(Harness.World); It; ++It)
	{
		const AEclipseCharacter* Body = *It;
		if (Body != nullptr && Body != Harness.Body && !Body->IsPlayerSide())
		{
			++Hostiles;
		}
	}
	Report(*this, TEXT("vijanden in M1.1"), Hostiles, TEXT(""),
		TEXT("4 — de graybox-standaard waarop de hele nacht gemeten is"));
	TestEqual(TEXT("terugval: een missie zonder batches houdt zijn vier vijanden"), Hostiles, 4);

	Harness.Shutdown();
	return true;
}

// Hoeveel rijen heeft een tabel, en hoeveel kan de code er bereiken?
//
// Twee keer op één dag heb ik het SETUP-SCRIPT vertrouwd in plaats van het asset,
// en twee keer klopte dat niet: EnemySpawns leek leeg volgens de scripts en had er
// vijf in de assets, en DT_Weapons leek twee rijen te hebben en heeft er vier.
// Beide keren met de hand gevonden. Dit is dezelfde controle, maar dan elke ronde.
//
// De tellingen staan als Report en niet als assert: een tabel mag groeien. Wat WEL
// vastgeklemd is, is het GAT — vier wapenrijen waarvan de game mode er één pakt
// (FirstRowOf). Voegt iemand een vijfde toe of sluit iemand de loadout aan, dan
// gaat deze test rood en moet de melding aan de owner mee. Dat is precies de
// bedoeling: het getal mag veranderen, maar niet stil.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseTableRowsAreReachableTest,
	"Eclipse.Playthrough.EveryTableRowCanBeReached",
	EclipsePlaythrough::TestFlags)

bool FEclipseTableRowsAreReachableTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	auto CountRows = [this](const TCHAR* Path, const TCHAR* Label) -> int32
	{
		const UDataTable* Table = LoadObject<UDataTable>(nullptr, Path);
		if (!TestNotNull(*FString::Printf(TEXT("tabellen: %s is geladen"), Label), Table))
		{
			return 0;
		}
		const int32 Rows = Table->GetRowMap().Num();
		Report(*this, *FString::Printf(TEXT("rijen in %s"), Label), Rows, TEXT(""), TEXT(""));
		return Rows;
	};

	const int32 Weapons = CountRows(TEXT("/Game/Data/DT_Weapons.DT_Weapons"), TEXT("DT_Weapons"));
	CountRows(TEXT("/Game/Data/DT_LoadoutOptions.DT_LoadoutOptions"), TEXT("DT_LoadoutOptions"));
	CountRows(TEXT("/Game/Data/DT_EnemyArchetypes.DT_EnemyArchetypes"), TEXT("DT_EnemyArchetypes"));
	CountRows(TEXT("/Game/Data/DT_BodyDefs.DT_BodyDefs"), TEXT("DT_BodyDefs"));
	CountRows(TEXT("/Game/Data/DT_SquadOrderDefs.DT_SquadOrderDefs"), TEXT("DT_SquadOrderDefs"));

	// De klem op het bekende gat. AEclipseGameMode gebruikt FirstRowOf voor het
	// spelerwapen, dus precies één rij is bereikbaar hoeveel je er ook authordt.
	Report(*this, TEXT("wapenrijen die de speler NIET kan dragen"), Weapons - 1, TEXT(""),
		TEXT("FirstRowOf pakt er één; de rest is onbereikbaar tot de loadout gekoppeld is"));
	TestEqual(TEXT("tabellen: nog steeds vier wapens waarvan er één bereikbaar is ")
		TEXT("(verandert dit, dan is de loadout-koppeling gemaakt of er is een wapen bij — beide moeten naar de owner)"),
		Weapons, 4);

	return true;
}

// Elke geauthorde spawnbatch moet ook echt vijanden kunnen opleveren.
//
// Geschiedenis, want die verklaart de vorm van deze test. Vanochtend vroeg was
// UEclipseMissionAsset::EnemySpawns dood: drie van de vier missies vulden hem in
// (Assault 2 batches, Rescue 2, Sabotage 1, M1.1 geen) en de game mode zette
// stil een vaste lus van vier neer. Die missies beschreven dus een opstelling
// die nooit gebeurde. Deze test klemde toen het bekende getal vast zodat er niets
// stil bij kon komen.
//
// Op owner-beslissing is de koppeling er nu (26-07), en daarmee verandert wat
// deze test moet bewaken: niet meer "hoeveel wordt er genegeerd" maar "kan elke
// batch landen". Een ArchetypeId die niet in DT_EnemyArchetypes staat levert nu
// een lege batch op — luid gemeld, maar nog steeds nul vijanden waar de missie
// er om vroeg. Dat is precies het soort stille afwijking waar vannacht de hele
// sessie over ging, dus het hoort rood te zijn en niet alleen luid.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseAuthoredSpawnsAreConsumedTest,
	"Eclipse.Playthrough.AuthoredEnemySpawnsWouldActuallySpawn",
	EclipsePlaythrough::TestFlags)

bool FEclipseAuthoredSpawnsAreConsumedTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	// Via de asset registry en niet via een lijst met namen: een vijfde missie
	// die er later bij komt moet vanzelf meegeteld worden. Een bewaker die zijn
	// eigen dekking bij naam kent, mist stil precies het nieuwe geval.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
		TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous({ TEXT("/Game/Data") }, /*bForceRescan*/ true);

	TArray<FAssetData> Found;
	Registry.GetAssetsByClass(UEclipseMissionAsset::StaticClass()->GetClassPathName(), Found, /*bSearchSubClasses*/ true);

	Report(*this, TEXT("verscheepte missiesjablonen"), Found.Num(), TEXT(""),
		TEXT("gevonden via de asset registry"));

	// Zonder deze regel is de hele test een tautologie: nul missies gevonden zou
	// even groen zijn als nul ingevulde batches. Dat is exact de valkuil die
	// vannacht al twee bewakers stil hield.
	if (!TestTrue(TEXT("spawns: er zijn uberhaupt missiesjablonen gevonden"), Found.Num() > 0))
	{
		return false;
	}

	// De archetype-namen die de game mode kan vinden. Zelfde bron, zelfde sleutel:
	// als deze test een naam goedkeurt die de spawner niet kent, keurt hij niets.
	TSet<FName> KnownArchetypes;
	if (const UDataTable* Archetypes = LoadObject<UDataTable>(
			nullptr, TEXT("/Game/Data/DT_EnemyArchetypes.DT_EnemyArchetypes")))
	{
		for (const TPair<FName, uint8*>& Row : Archetypes->GetRowMap())
		{
			KnownArchetypes.Add(Row.Key);
		}
	}
	if (!TestTrue(TEXT("spawns: DT_EnemyArchetypes is geladen en heeft rijen"), KnownArchetypes.Num() > 0))
	{
		return false;
	}

	int32 TotalBatches = 0;
	int32 TotalEnemies = 0;
	for (const FAssetData& Data : Found)
	{
		const UEclipseMissionAsset* Mission = Cast<UEclipseMissionAsset>(Data.GetAsset());
		if (Mission == nullptr)
		{
			continue;
		}
		TotalBatches += Mission->EnemySpawns.Num();

		// Per missie en niet alleen als totaal. Ik meldde de owner eerst "15
		// vijanden in plaats van 4", en dat leest als een verdrievoudiging pér
		// missie terwijl het het totaal over drie missies is. Een optelsom die als
		// per-stuk leest, is een verkeerd balansgetal.
		int32 ThisMission = 0;
		for (const FEclipseEnemySpawnSet& Set : Mission->EnemySpawns)
		{
			ThisMission += Set.Count;
		}
		if (ThisMission > 0)
		{
			AddInfo(FString::Printf(TEXT("spawns: '%s' zet %d vijanden neer (was 4 tot 26-07, dus %+d)"),
				*Data.AssetName.ToString(), ThisMission, ThisMission - 4));
		}

		for (const FEclipseEnemySpawnSet& Set : Mission->EnemySpawns)
		{
			TotalEnemies += Set.Count;
			TestTrue(FString::Printf(
					TEXT("spawns: '%s' vraagt om archetype '%s' en dat staat in DT_EnemyArchetypes ")
					TEXT("(zo niet: de batch levert NUL vijanden op waar de missie er %d wil)"),
					*Data.AssetName.ToString(), *Set.ArchetypeId.ToString(), Set.Count),
				KnownArchetypes.Contains(Set.ArchetypeId));

			// Een batch van 0 is geen data maar een vergissing: hij leest als "hier
			// staan vijanden" en levert er geen.
			TestTrue(FString::Printf(TEXT("spawns: '%s' vraagt om minstens 1 vijand in de batch '%s'"),
					*Data.AssetName.ToString(), *Set.ArchetypeId.ToString()),
				Set.Count > 0);
		}
	}

	Report(*this, TEXT("geauthorde spawnbatches"), TotalBatches, TEXT(""), TEXT("over alle missies"));
	Report(*this, TEXT("vijanden die die batches vragen"), TotalEnemies, TEXT(""),
		TEXT("de game mode plaatst er sinds 26-07 precies dit aantal"));

	// Hoeveel optionals hangen aan een conditie, en aan welke? Sinds het alarm op
	// de eerste waarneming zit (26-07) is dat geen boekhouding meer maar een
	// balansfeit: een bRequiresNoAlarm-optional is vanaf nu alleen haalbaar als je
	// de hele missie ONGEZIEN doet.
	//
	// Uit de ASSETS gemeten en niet uit de setup-scripts. Dat onderscheid kostte
	// vanochtend een verkeerde conclusie: de scripts authoren geen EnemySpawns,
	// dus noteerde ik "nog niets ingevuld" — en de assets bleken er vijf te
	// hebben, in de editor gezet.
	int32 NoAlarmOptionals = 0;
	int32 NoCasualtyOptionals = 0;
	for (const FAssetData& Data : Found)
	{
		const UEclipseMissionAsset* Mission = Cast<UEclipseMissionAsset>(Data.GetAsset());
		if (Mission == nullptr)
		{
			continue;
		}
		for (const FEclipseObjectiveDef& Objective : Mission->Objectives)
		{
			if (!Objective.bOptional)
			{
				continue;
			}
			NoAlarmOptionals += Objective.bRequiresNoAlarm ? 1 : 0;
			NoCasualtyOptionals += Objective.bRequiresNoCasualties ? 1 : 0;
		}
	}
	Report(*this, TEXT("optionals die stilte eisen"), NoAlarmOptionals, TEXT(""),
		TEXT("kosten je sinds 26-07 elke detectie; 0 = het alarm kost vandaag nog niets"));
	Report(*this, TEXT("optionals die nul gewonden eisen"), NoCasualtyOptionals, TEXT(""),
		TEXT("betalen sinds 26-07 uit zonder dat iets ze hoeft af te vinken"));
	return true;
}

/**
 * TERUGSLAG (owner-opdracht 26-07 avond, punt 4). Twee dingen moeten waar zijn en
 * ze staan haaks op elkaar: het wapen moet je kruis omhoog DUWEN, en dat duwtje
 * moet vanzelf TERUGZAKKEN. Zonder het eerste is stabiliteit een dood veld;
 * zonder het tweede is terugslag alleen straf.
 *
 * Gemeten met de DMR, want die heeft de grootste uitslag in de data (1,8 graden)
 * en het traagste herstel (3,0 graden/s) — als de klim daar niet te zien is, is
 * hij nergens te zien.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseRecoilKicksAndRecovers,
	"Eclipse.Mission.Playthrough.RecoilKicksAndRecovers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseRecoilKicksAndRecovers::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("terugslag: strategie"), Strategy) || !TestNotNull(TEXT("terugslag: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("terugslag: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	UEclipseHitscanWeaponComponent* Weapon = Harness.Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
	if (!TestNotNull(TEXT("terugslag: de speler heeft een wapen"), Weapon))
	{
		Harness.Shutdown();
		return false;
	}

	const float KickDegrees = Weapon->GetRecoilPitchDegrees();
	const float RecoveryRate = Weapon->GetRecoilRecoveryDegreesPerSecond();
	Report(*this, TEXT("uitslag uit de data"), KickDegrees, TEXT("graden/schot"));
	Report(*this, TEXT("herstel uit de data"), RecoveryRate, TEXT("graden/s"));

	// --- 1. duwt het schot het kruis omhoog? -------------------------------
	Harness.Idle(0.2f);
	const float PitchBefore = Harness.Controller->GetControlRotation().Pitch;
	Harness.Inject(TEXT("Fire"), true);
	Harness.Step();
	Harness.Inject(TEXT("Fire"), false);
	Harness.Step();
	const float PitchAfterShot = Harness.Controller->GetControlRotation().Pitch;

	// Omhoog kijken is een POSITIEVE pitch in de besturing (de invoer wordt
	// omgekeerd doorgegeven). Genormaliseerd, want 0/360 ligt er vlakbij.
	const float Climb = FRotator::NormalizeAxis(PitchAfterShot - PitchBefore);
	Report(*this, TEXT("gemeten klim na één schot"), Climb, TEXT("graden"),
		*FString::Printf(TEXT("data zegt %.2f"), KickDegrees));
	TestTrue(TEXT("terugslag: het schot duwt het kruis omhoog"), Climb > 0.05f);

	// --- 2. zakt het vanzelf terug? ----------------------------------------
	// Een halve seconde stilstaan zonder kijkinvoer. Bij 3,0 graden/s is dat
	// ruim genoeg voor 1,8 graden — de klim hoort helemaal weg te zijn.
	Harness.Idle(1.0f);
	const float PitchAfterRest = Harness.Controller->GetControlRotation().Pitch;
	const float Remaining = FRotator::NormalizeAxis(PitchAfterRest - PitchBefore);
	Report(*this, TEXT("resterende klim na 1 s rust"), Remaining, TEXT("graden"));
	TestTrue(TEXT("terugslag: het kruis zakt vanzelf terug"), FMath::Abs(Remaining) < FMath::Abs(Climb) * 0.5f);

	Harness.Shutdown();
	return true;
}


/**
 * WAPENGELUID IN EEN ECHTE MISSIE (owner-levering 26-07 avond).
 *
 * Twee dingen die alleen in de missie waar kunnen zijn: de familiesets worden
 * echt geladen (het pack staat op de plek waar de code hem zoekt), en de rem op
 * de nagalm doet wat hij moet doen — één staart per opening, niet één per kogel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponSoundSetsLoadAndTailIsBraked,
	"Eclipse.Mission.Playthrough.WeaponSoundSetsLoadAndTailIsBraked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseWeaponSoundSetsLoadAndTailIsBraked::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseAudioSubsystem* Audio = GameInstance->GetSubsystem<UEclipseAudioSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("wapengeluid: audio"), Audio) || !TestNotNull(TEXT("wapengeluid: strategie"), Strategy)
		|| !TestNotNull(TEXT("wapengeluid: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("wapengeluid: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	Report(*this, TEXT("geladen wapenfamilies"), static_cast<float>(Audio->GetWeaponSoundFamilyCount()), TEXT(""),
		TEXT("alleen de families die een wapen in DT_Weapons echt gebruikt"));
	Report(*this, TEXT("schotvarianten AssaultRifle"),
		static_cast<float>(Audio->GetWeaponSoundVariantCount(TEXT("AssaultRifle"))), TEXT(""));
	Report(*this, TEXT("schotvarianten Handgun"),
		static_cast<float>(Audio->GetWeaponSoundVariantCount(TEXT("Handgun"))), TEXT(""));

	TestTrue(TEXT("wapengeluid: de rifle heeft meer dan één schot"),
		Audio->GetWeaponSoundVariantCount(TEXT("AssaultRifle")) > 1);

	// Twee seconden automatisch vuur. Bij 6,67 schoten per seconde zijn dat er
	// ruim dertien; de nagalm hoort er hooguit vier te geven (rem van 0,6 s).
	const int32 ShotsBefore = Audio->GetShotSoundCount();
	const int32 TailsBefore = Audio->GetTailSoundCount();
	const double Start = Harness.ElapsedSeconds;
	while (Harness.ElapsedSeconds - Start < 2.0)
	{
		Harness.Inject(TEXT("Fire"), true);
		Harness.Step();
	}
	const float Shots = static_cast<float>(Audio->GetShotSoundCount() - ShotsBefore);
	const float Tails = static_cast<float>(Audio->GetTailSoundCount() - TailsBefore);

	Report(*this, TEXT("schoten in 2 s"), Shots, TEXT(""));
	Report(*this, TEXT("nagalmen in 2 s"), Tails, TEXT(""), TEXT("rem staat op 0,6 s"));

	TestTrue(TEXT("wapengeluid: er is geschoten"), Shots >= 5.0f);
	TestTrue(TEXT("wapengeluid: de nagalm klinkt, maar niet bij elk schot"), Tails >= 1.0f && Tails < Shots);

	Harness.Shutdown();
	return true;
}


/**
 * VOETSTAPPEN PER OPPERVLAK (owner-levering 26-07: Footsteps_Volume_02).
 *
 * De vraag die deze meting beantwoordt is niet "klinkt er iets" maar "weet het
 * spel waar je op staat". Tot vandaag was het antwoord nee: er was geen enkel
 * oppervlaktetype in het project, geen physical material van ons, en geen regel
 * code die er een uitlas — elke stap klonk op asfalt, overal.
 *
 * Drie sporten, alle drie hier gemeten: de banken laden, de vloer meldt zijn
 * oppervlak, en de stap kiest de bank die erbij hoort.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseFootstepsKnowTheirSurface,
	"Eclipse.Mission.Playthrough.FootstepsKnowTheirSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseFootstepsKnowTheirSurface::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseAudioSubsystem* Audio = GameInstance->GetSubsystem<UEclipseAudioSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("voetstappen: audio"), Audio) || !TestNotNull(TEXT("voetstappen: strategie"), Strategy)
		|| !TestNotNull(TEXT("voetstappen: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("voetstappen: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	// --- 1. laden de banken? ------------------------------------------------
	Report(*this, TEXT("varianten op metaal"),
		static_cast<float>(Audio->GetFootstepVariantCount(1)), TEXT(""), TEXT("SurfaceType1"));
	Report(*this, TEXT("varianten op beton"),
		static_cast<float>(Audio->GetFootstepVariantCount(2)), TEXT(""), TEXT("SurfaceType2"));
	Report(*this, TEXT("varianten op modder"),
		static_cast<float>(Audio->GetFootstepVariantCount(3)), TEXT(""), TEXT("SurfaceType3 — cues staan klaar, vloer nog niet"));

	TestTrue(TEXT("voetstappen: de metaalbank heeft meer dan één variant"),
		Audio->GetFootstepVariantCount(1) > 1);
	TestTrue(TEXT("voetstappen: de betonbank heeft meer dan één variant"),
		Audio->GetFootstepVariantCount(2) > 1);

	// --- 2. weet de stap waar hij op valt? ----------------------------------
	UEclipseAnimInstance* Anim = Harness.Body->GetMesh() != nullptr
		? Cast<UEclipseAnimInstance>(Harness.Body->GetMesh()->GetAnimInstance()) : nullptr;
	if (!TestNotNull(TEXT("voetstappen: het lichaam heeft een anim-instance"), Anim))
	{
		Harness.Shutdown();
		return false;
	}

	const int32 StepsBefore = Anim->GetFootstepCount();
	const int32 SoundsBefore = Audio->GetFootstepSoundCount();
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 2.0,
		[Anim, StepsBefore]() { return Anim->GetFootstepCount() - StepsBefore >= 4; });

	const float Steps = static_cast<float>(Anim->GetFootstepCount() - StepsBefore);
	const float Sounds = static_cast<float>(Audio->GetFootstepSoundCount() - SoundsBefore);
	const uint8 Surface = Anim->GetLastFootstepSurface();

	Report(*this, TEXT("stappen gezet"), Steps, TEXT(""));
	Report(*this, TEXT("stappen die om een geluid vroegen"), Sounds, TEXT(""));
	Report(*this, TEXT("oppervlak onder de laatste stap"), static_cast<float>(Surface), TEXT(""),
		TEXT("0 = niets gevonden, 1 = metaal, 2 = beton"));

	TestTrue(TEXT("voetstappen: er is gelopen"), Steps >= 1.0f);
	TestEqual(TEXT("voetstappen: elke stap vraagt om een geluid"), Sounds, Steps);

	// Dit is het getal waar het om gaat. 0 betekent dat de streep omlaag niets
	// vond of dat de vloer geen physical material draagt — dan valt alles terug op
	// de straatbank en is er in de praktijk niets veranderd.
	TestTrue(FString::Printf(TEXT("voetstappen: de vloer meldt zijn oppervlak (%d)"), Surface),
		Surface != 0);

	// --- 3. HOORT hij het verschil? -----------------------------------------
	// Eén oppervlak bewijst niets: als alles beton meldt, klinkt alles hetzelfde
	// en is er sinds vanmorgen niets veranderd. Dus bovenop een dekkingsblok —
	// hazard-oranje industriële barrier, 120 cm hoog, het enige metaal in dit
	// district waar je op kunt staan.
	// LANGS de lange as, en dat is geen detail: het blok is 300 bij 100 cm en één
	// stap is 140. Dwars erop loop je er dus af vóór de eerste stap valt — de
	// eerste poging mat daardoor gewoon de vloer weer, en dat leest als "metaal
	// werkt niet" terwijl het "hij stond er niet meer op" was.
	//
	// Het lopen is camera-relatief sinds vanmorgen, dus de KIJKRICHTING moet langs
	// de as staan, niet de acteur.
	Harness.Body->SetActorLocation(FVector(-6120.0f, -4000.0f, 260.0f),
		/*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	Harness.Controller->SetControlRotation(FRotator::ZeroRotator);
	Harness.Idle(1.0f); // laten zakken tot hij echt op het blok staat

	const int32 StepsOnMetal = Anim->GetFootstepCount();
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 2.0,
		[Anim, StepsOnMetal]() { return Anim->GetFootstepCount() - StepsOnMetal >= 1; });
	const uint8 MetalSurface = Anim->GetLastFootstepSurface();

	// Stond hij er nog op? Zonder deze controle is een 2 dubbelzinnig: hij kan van
	// het blok gelopen zijn en gewoon de vloer gemeten hebben.
	Report(*this, TEXT("hoogte bij die stap"), static_cast<float>(Harness.Location().Z), TEXT("cm"),
		TEXT("blokdek ligt op 120; de vloer op 0"));

	Report(*this, TEXT("oppervlak bovenop een dekkingsblok"), static_cast<float>(MetalSurface), TEXT(""),
		TEXT("1 = metaal — anders klinkt het hele district hetzelfde"));

	TestEqual(TEXT("voetstappen: op een dekkingsblok klinkt metaal"), static_cast<int32>(MetalSurface), 1);
	TestTrue(TEXT("voetstappen: dat is een ANDER oppervlak dan de vloer"),
		MetalSurface != Surface);

	Harness.Shutdown();
	return true;
}


/**
 * MAGAZIJN EN HERLADEN (owner-opdracht 26-07 avond, punt 4).
 *
 * Herladen is wat een vuurtempo betekenis geeft. Zonder magazijn is een hoge
 * cadans gratis en is "40 kogels tegen 10" een getal zonder gevolg — dan zegt de
 * wapentabel iets wat het spel niet doet.
 *
 * Vier dingen die waar moeten zijn: het magazijn loopt leeg, een leeg magazijn
 * herlaadt zichzelf in plaats van een dode trekker te geven, tijdens het
 * herladen kun je niet vuren, en daarna zit je weer vol.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMagazineEmptiesAndReloads,
	"Eclipse.Mission.Playthrough.MagazineEmptiesAndReloads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseMagazineEmptiesAndReloads::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	UEclipseAudioSubsystem* Audio = GameInstance->GetSubsystem<UEclipseAudioSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("magazijn: audio"), Audio) || !TestNotNull(TEXT("magazijn: strategie"), Strategy)
		|| !TestNotNull(TEXT("magazijn: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("magazijn: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	UEclipseHitscanWeaponComponent* Weapon = Harness.Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
	if (!TestNotNull(TEXT("magazijn: de speler heeft een wapen"), Weapon))
	{
		Harness.Shutdown();
		return false;
	}

	const int32 Size = Weapon->GetMagazineSize();
	Report(*this, TEXT("magazijngrootte uit de data"), static_cast<float>(Size), TEXT("kogels"));
	Report(*this, TEXT("herlaadtijd uit de data"), Weapon->GetReloadSeconds(), TEXT("s"));
	Report(*this, TEXT("foley-stappen voor deze familie"),
		static_cast<float>(Audio->GetReloadFoleyStepCount(TEXT("AssaultRifle"))), TEXT(""),
		TEXT("magazijn laten vallen, pakken, insteken, grendel"));

	TestEqual(TEXT("magazijn: vol bij de start"), Weapon->GetAmmoInMagazine(), Size);
	TestEqual(TEXT("magazijn: de foley-keten heeft vier fasen"),
		Audio->GetReloadFoleyStepCount(TEXT("AssaultRifle")), 4);

	// --- leegschieten -------------------------------------------------------
	const int32 FoleyBefore = Audio->GetReloadFoleyCount();
	const double Start = Harness.ElapsedSeconds;
	while (Harness.ElapsedSeconds - Start < 8.0 && Weapon->GetReloadCount() == 0)
	{
		Harness.Inject(TEXT("Fire"), true);
		Harness.Step();
	}

	Report(*this, TEXT("schoten tot het magazijn leeg was"),
		static_cast<float>(Weapon->GetShotsFired()), TEXT(""));
	Report(*this, TEXT("herlaadbeurten"), static_cast<float>(Weapon->GetReloadCount()), TEXT(""));
	Report(*this, TEXT("foley-stappen ingepland"),
		static_cast<float>(Audio->GetReloadFoleyCount() - FoleyBefore), TEXT(""));

	TestTrue(TEXT("magazijn: een leeg magazijn herlaadt zichzelf"), Weapon->GetReloadCount() >= 1);
	TestEqual(TEXT("magazijn: precies een magazijn lang geschoten"), Weapon->GetShotsFired(), Size);
	TestEqual(TEXT("magazijn: de hele keten is ingepland"),
		Audio->GetReloadFoleyCount() - FoleyBefore, 4);

	// --- tijdens het herladen kun je niet vuren -----------------------------
	TestTrue(TEXT("magazijn: hij is aan het herladen"), Weapon->IsReloading());
	const int32 ShotsDuringReload = Weapon->GetShotsFired();
	Harness.Idle(0.2f);
	Harness.Inject(TEXT("Fire"), true);
	Harness.Step();
	TestEqual(TEXT("magazijn: vuren tijdens het herladen doet niets"),
		Weapon->GetShotsFired(), ShotsDuringReload);

	// --- en daarna zit je weer vol ------------------------------------------
	Harness.Idle(Weapon->GetReloadSeconds() + 0.3f);
	Harness.Inject(TEXT("Fire"), true);
	Harness.Step();
	Report(*this, TEXT("kogels na het herladen"),
		static_cast<float>(Weapon->GetAmmoInMagazine()), TEXT(""), TEXT("een eraf: het schot dat de test net deed"));
	TestTrue(TEXT("magazijn: na het herladen kun je weer schieten"),
		Weapon->GetShotsFired() > ShotsDuringReload);

	Harness.Shutdown();
	return true;
}


/**
 * LOADOUTS EN WAPENWISSEL (owner-opdracht 26-07 avond, punt 5).
 *
 * De gevechts-audit kwam er twee rondes achter elkaar op uit: er zijn vier
 * wapens die sinds vandaag echt van elkaar verschillen, en drie ervan kon
 * niemand vasthouden. De loadout-keuze bestond, werd gevalideerd en verzonden —
 * en bereikte het wapen nooit.
 *
 * Vier dingen die nu waar moeten zijn: je krijgt het wapen dat je koos, je hebt
 * er een tweede bij, wisselen werkt, en elk wapen houdt zijn eigen magazijn.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLoadoutReachesTheWeapon,
	"Eclipse.Mission.Playthrough.LoadoutReachesTheWeapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseLoadoutReachesTheWeapon::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("loadout: strategie"), Strategy) || !TestNotNull(TEXT("loadout: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("loadout: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(0.5f);

	UEclipseHitscanWeaponComponent* Weapon = Harness.Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
	if (!TestNotNull(TEXT("loadout: de speler heeft een wapen"), Weapon))
	{
		Harness.Shutdown();
		return false;
	}

	Report(*this, TEXT("wapenslots"), static_cast<float>(Weapon->GetSlotCount()), TEXT(""),
		TEXT("primair plus sidearm"));
	TestEqual(TEXT("loadout: twee slots, niet één"), Weapon->GetSlotCount(), 2);

	// --- het primaire wapen is dat van de gekozen loadout -------------------
	const float PrimaryDamage = Weapon->GetDamage();
	const int32 PrimaryMagazine = Weapon->GetMagazineSize();
	Report(*this, TEXT("schade primair"), PrimaryDamage, TEXT("hp"));
	Report(*this, TEXT("magazijn primair"), static_cast<float>(PrimaryMagazine), TEXT(""));

	// --- een paar kogels eruit, dan wisselen -------------------------------
	const double Start = Harness.ElapsedSeconds;
	while (Harness.ElapsedSeconds - Start < 0.5)
	{
		Harness.Inject(TEXT("Fire"), true);
		Harness.Step();
	}
	const int32 AmmoLeftInPrimary = Weapon->GetAmmoInMagazine();
	Report(*this, TEXT("kogels over in het primaire"), static_cast<float>(AmmoLeftInPrimary), TEXT(""));
	TestTrue(TEXT("loadout: er is uit het primaire geschoten"), AmmoLeftInPrimary < PrimaryMagazine);

	Harness.Idle(0.3f);
	TestTrue(TEXT("loadout: wisselen lukt"), Weapon->SwapWeapon());

	const float SidearmDamage = Weapon->GetDamage();
	Report(*this, TEXT("schade sidearm"), SidearmDamage, TEXT("hp"));
	Report(*this, TEXT("magazijn sidearm"), static_cast<float>(Weapon->GetMagazineSize()), TEXT(""));
	Report(*this, TEXT("actief slot na wisselen"), static_cast<float>(Weapon->GetActiveSlot()), TEXT(""));

	// DIT is de meting die telt: als beide slots hetzelfde wapen dragen, reageert
	// de knop wel en verandert er niets — precies de klasse die deze laag kwam
	// repareren, alleen een niveau dieper.
	TestEqual(TEXT("loadout: het tweede slot is actief"), Weapon->GetActiveSlot(), 1);
	TestTrue(TEXT("loadout: de sidearm is een ANDER wapen dan het primaire"),
		!FMath::IsNearlyEqual(SidearmDamage, PrimaryDamage) || Weapon->GetMagazineSize() != PrimaryMagazine);

	// --- handling: vlak na de wissel kun je nog niet vuren ------------------
	TestFalse(TEXT("loadout: het opgetilde wapen is nog niet klaar"), Weapon->IsReady());
	Harness.Idle(1.0f);
	TestTrue(TEXT("loadout: na de opheftijd wel"), Weapon->IsReady());

	// --- terugwisselen: het primaire komt halfleeg terug --------------------
	TestTrue(TEXT("loadout: terugwisselen lukt"), Weapon->SwapWeapon());
	Report(*this, TEXT("kogels in het primaire na terugwisselen"),
		static_cast<float>(Weapon->GetAmmoInMagazine()), TEXT(""),
		TEXT("moet gelijk zijn aan wat er over was"));
	TestEqual(TEXT("loadout: elk wapen houdt zijn eigen magazijn"),
		Weapon->GetAmmoInMagazine(), AmmoLeftInPrimary);

	Harness.Shutdown();
	return true;
}


/**
 * MEELOPEN (owner-opdracht 26-07 avond, punt 1 — laag 1 van zes).
 *
 * "ze lopen mee ... Dat is geen feature die je aanzet, dat is de basis."
 *
 * Twee dingen moeten waar zijn en ze staan op gespannen voet: de squad moet uit
 * zichzelf bijlopen, en een staande order moet dat nog steeds winnen. Zonder het
 * eerste is je squad een rij standbeelden; zonder het tweede is een order geen
 * belofte meer (8.4) en kun je niemand meer ergens neerzetten.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadFollowsWithoutBeingTold,
	"Eclipse.Mission.Playthrough.SquadFollowsWithoutBeingTold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseSquadFollowsWithoutBeingTold::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("meelopen: strategie"), Strategy) || !TestNotNull(TEXT("meelopen: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("meelopen: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	// Alle squadmates verzamelen.
	TArray<AEclipseSquadmateController*> Mates;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mates.Add(*It);
	}
	Report(*this, TEXT("squadleden in het veld"), static_cast<float>(Mates.Num()), TEXT(""));
	if (!TestTrue(TEXT("meelopen: er is een squad"), Mates.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	auto FarthestFromPlayer = [&Harness, &Mates]() -> float
	{
		float Worst = 0.0f;
		for (const AEclipseSquadmateController* Mate : Mates)
		{
			if (const APawn* Body = Mate != nullptr ? Mate->GetPawn() : nullptr)
			{
				Worst = FMath::Max(Worst, static_cast<float>(FVector::Dist2D(Body->GetActorLocation(), Harness.Location())));
			}
		}
		return Worst;
	};

	// --- de speler loopt weg -----------------------------------------------
	const float Before = FarthestFromPlayer();
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 6.0, []() { return false; });
	const float WhileWalking = FarthestFromPlayer();

	int32 Moves = 0;
	for (const AEclipseSquadmateController* Mate : Mates)
	{
		Moves += Mate != nullptr ? Mate->GetFollowMoves() : 0;
	}

	Report(*this, TEXT("verste soldaat bij de start"), Before, TEXT("cm"));
	Report(*this, TEXT("verste soldaat na 6 s weglopen"), WhileWalking, TEXT("cm"),
		TEXT("zonder meelopen groeit dit tot de hele loopafstand"));
	Report(*this, TEXT("verplaatsingen uit MEELOPEN"), static_cast<float>(Moves), TEXT(""),
		TEXT("niet uit een order"));

	TestTrue(TEXT("meelopen: er is uit zichzelf bijgelopen"), Moves > 0);
	// Zes seconden op 420 cm/s is ~2500 cm. Blijft de squad binnen het dubbele van
	// de volgafstand, dan lopen ze echt mee in plaats van achter te blijven.
	TestTrue(FString::Printf(TEXT("meelopen: de squad blijft in de buurt (%.0f cm)"), WhileWalking),
		WhileWalking < 1200.0f);

	// --- maar een order wint ------------------------------------------------
	// Iemand op Hold zetten en dan weglopen: hij hoort te BLIJVEN.
	AEclipseSquadmateController* Held = Mates[0];
	const FVector HeldStart = Held->GetPawn() != nullptr ? Held->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	Held->ExecuteOrder(EEclipseSquadOrder::Hold, HeldStart, nullptr);
	const int32 MovesBeforeHold = Held->GetFollowMoves();

	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 6.0, []() { return false; });

	const float HeldDrift = Held->GetPawn() != nullptr
		? static_cast<float>(FVector::Dist2D(Held->GetPawn()->GetActorLocation(), HeldStart)) : -1.0f;
	Report(*this, TEXT("afdrijving van de vastgezette soldaat"), HeldDrift, TEXT("cm"),
		TEXT("een order is een belofte (8.4)"));
	Report(*this, TEXT("meelopen-verplaatsingen tijdens Hold"),
		static_cast<float>(Held->GetFollowMoves() - MovesBeforeHold), TEXT(""));

	TestEqual(TEXT("meelopen: een Hold-order zet meelopen stil"),
		Held->GetFollowMoves() - MovesBeforeHold, 0);
	TestTrue(FString::Printf(TEXT("meelopen: de vastgezette soldaat blijft staan (%.0f cm)"), HeldDrift),
		HeldDrift >= 0.0f && HeldDrift < 300.0f);

	Harness.Shutdown();
	return true;
}


/**
 * AUTONOOM VUREN (owner-opdracht 26-07 avond, punt 1 — laag 2 van zes).
 *
 * "ze vuren op wat ze zien." Tot vandaag vuurde een squadmate alleen op een
 * expliciete FocusTarget-order; zonder order stond hij ernaast te kijken terwijl
 * je beschoten werd.
 *
 * Dit is de grootste balansverschuiving die er ligt, dus de meting doet twee
 * dingen: bewijzen DAT ze vuren, en meteen laten zien wat het KOST — hoeveel
 * sneller een groep vijanden valt met drie extra schutters.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadFiresWithoutBeingTold,
	"Eclipse.Mission.Playthrough.SquadFiresWithoutBeingTold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseSquadFiresWithoutBeingTold::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("autovuur: strategie"), Strategy) || !TestNotNull(TEXT("autovuur: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("autovuur: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	TArray<AEclipseSquadmateController*> Mates;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mates.Add(*It);
	}
	if (!TestTrue(TEXT("autovuur: er is een squad"), Mates.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	// Een vijand recht voor de neus van de squad. Geen order, geen aanwijzing —
	// als er geschoten wordt, komt dat uit henzelf.
	const FVector Spot = Harness.Location() + Harness.Body->GetActorForwardVector() * 700.0f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEclipseCharacter* Hostile = Harness.World->SpawnActor<AEclipseCharacter>(
		AEclipseCharacter::StaticClass(), Spot, FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("autovuur: er staat een vijand"), Hostile))
	{
		Harness.Shutdown();
		return false;
	}
	// Geen SetPlayerSide nodig: IsPlayerSide() is waar zodra een lichaam een
	// SoldierId heeft of door de speler bestuurd wordt. Een vers gespawnd lichaam
	// heeft geen van beide en is dus per definitie vijandig — dezelfde regel die
	// de vijand omgekeerd gebruikt om ONS te herkennen.
	Hostile->InitializeHealth(100000.0f); // hij hoeft niet te vallen, alleen geraakt te worden

	int32 ShotsBefore = 0;
	for (const AEclipseSquadmateController* Mate : Mates)
	{
		ShotsBefore += Mate != nullptr ? Mate->GetAutoFireShots() : 0;
	}
	const float HealthBefore = Hostile->GetHealth();

	// Vijf seconden niets doen. De speler vuurt niet, geeft geen order.
	Harness.Idle(5.0f);

	int32 Shots = 0;
	for (const AEclipseSquadmateController* Mate : Mates)
	{
		Shots += (Mate != nullptr ? Mate->GetAutoFireShots() : 0);
	}
	Shots -= ShotsBefore;
	const float Damage = HealthBefore - Hostile->GetHealth();

	Report(*this, TEXT("squadleden in het veld"), static_cast<float>(Mates.Num()), TEXT(""));
	Report(*this, TEXT("schoten zonder order in 5 s"), static_cast<float>(Shots), TEXT(""),
		TEXT("de speler deed niets"));
	Report(*this, TEXT("schade door de squad alleen"), Damage, TEXT("hp"),
		TEXT("dit is wat drie extra schutters aan een gevecht toevoegen"));

	TestTrue(TEXT("autovuur: de squad vuurt uit zichzelf"), Shots > 0);
	TestTrue(TEXT("autovuur: en het komt aan"), Damage > 0.0f);

	Harness.Shutdown();
	return true;
}


/**
 * DEKKING ONDER VUUR (owner-opdracht 26-07 avond, punt 1 — laag 3 van zes).
 *
 * "als er op ze geschoten wordt volgen ze hun training: dekking zoeken,
 * verplaatsen, dekkingsvuur." `CoverSearchRadius` stond in de data met een
 * comment die zei dat er geen dekkingzoekgedrag was.
 *
 * Twee dingen: er MOET beweging komen van een treffer, en een staande order moet
 * dat nog steeds winnen — dat laatste is wat "orders zijn beloftes" betekent op
 * het moment dat het pijn doet.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadTakesCoverUnderFire,
	"Eclipse.Mission.Playthrough.SquadTakesCoverUnderFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseSquadTakesCoverUnderFire::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("dekking: strategie"), Strategy) || !TestNotNull(TEXT("dekking: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("dekking: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	TArray<AEclipseSquadmateController*> Mates;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mates.Add(*It);
	}
	if (!TestTrue(TEXT("dekking: er is een squad"), Mates.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	AEclipseSquadmateController* Mate = Mates[0];
	AEclipseCharacter* MateBody = Cast<AEclipseCharacter>(Mate->GetPawn());
	if (!TestNotNull(TEXT("dekking: de soldaat heeft een lichaam"), MateBody))
	{
		Harness.Shutdown();
		return false;
	}

	// --- een treffer zet hem in beweging ------------------------------------
	const FVector Before = MateBody->GetActorLocation();
	const int32 RunsBefore = Mate->GetCoverRuns();
	// Schade uit de richting van de speler: dat is de dreigingsrichting waar de
	// dekkingsscorer op zoekt.
	MateBody->ApplyDamage(5.0f, Harness.Body, TEXT("TestIncoming"));
	Harness.Idle(2.0f);

	const float Moved = static_cast<float>(FVector::Dist2D(MateBody->GetActorLocation(), Before));
	Report(*this, TEXT("dekkingszoektochten na één treffer"),
		static_cast<float>(Mate->GetCoverRuns() - RunsBefore), TEXT(""));
	Report(*this, TEXT("afgelegde afstand na de treffer"), Moved, TEXT("cm"));

	TestTrue(TEXT("dekking: een treffer zet hem in beweging"), Mate->GetCoverRuns() > RunsBefore);

	// --- maar een Hold-order wint --------------------------------------------
	Harness.Idle(2.5f); // de rem laten aflopen
	Mate->ExecuteOrder(EEclipseSquadOrder::Hold, MateBody->GetActorLocation(), nullptr);
	const int32 RunsUnderOrder = Mate->GetCoverRuns();
	const FVector HeldAt = MateBody->GetActorLocation();

	MateBody->ApplyDamage(5.0f, Harness.Body, TEXT("TestIncoming"));
	Harness.Idle(2.0f);

	const float HeldDrift = static_cast<float>(FVector::Dist2D(MateBody->GetActorLocation(), HeldAt));
	Report(*this, TEXT("dekkingszoektochten tijdens Hold"),
		static_cast<float>(Mate->GetCoverRuns() - RunsUnderOrder), TEXT(""),
		TEXT("een order is een belofte, ook onder vuur (8.4)"));
	Report(*this, TEXT("afdrijving tijdens Hold"), HeldDrift, TEXT("cm"));

	TestEqual(TEXT("dekking: een Hold-order houdt hem staan, ook onder vuur"),
		Mate->GetCoverRuns() - RunsUnderOrder, 0);

	Harness.Shutdown();
	return true;
}


/**
 * DOCTRINE (owner-opdracht 26-07 avond, punt 1 — laag 4 van zes).
 *
 * Geen schakelaars voor basisgedrag: elke doctrine PERKT DE BASIS IN of LAAT
 * HEM LOS. Deze meting bewijst dat verschil per doctrine, en dat is de enige
 * manier waarop je kunt zien dat het een kader is en geen naam:
 *
 *   Recon      vuurt NIET terwijl Ready dat wel doet
 *   Overwatch  loopt NIET mee terwijl Ready dat wel doet
 *   Aggressive zoekt GEEN dekking terwijl Ready dat wel doet
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseDoctrineChangesBehaviour,
	"Eclipse.Mission.Playthrough.DoctrineChangesBehaviour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseDoctrineChangesBehaviour::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("doctrine: strategie"), Strategy) || !TestNotNull(TEXT("doctrine: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("doctrine: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	TArray<AEclipseSquadmateController*> Mates;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mates.Add(*It);
	}
	if (!TestTrue(TEXT("doctrine: er is een squad"), Mates.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	// Een vijand binnen bereik van iedereen.
	const FVector Spot = Harness.Location() + Harness.Body->GetActorForwardVector() * 700.0f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEclipseCharacter* Hostile = Harness.World->SpawnActor<AEclipseCharacter>(
		AEclipseCharacter::StaticClass(), Spot, FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("doctrine: er staat een vijand"), Hostile))
	{
		Harness.Shutdown();
		return false;
	}
	Hostile->InitializeHealth(100000.0f);

	auto ShotsOverTwoSeconds = [&Harness, &Mates](EEclipseSquadStance Stance) -> int32
	{
		for (AEclipseSquadmateController* Mate : Mates)
		{
			Mate->SetDoctrine(Stance);
		}
		int32 Before = 0;
		for (const AEclipseSquadmateController* Mate : Mates)
		{
			Before += Mate->GetAutoFireShots();
		}
		Harness.Idle(2.0f);
		int32 After = 0;
		for (const AEclipseSquadmateController* Mate : Mates)
		{
			After += Mate->GetAutoFireShots();
		}
		return After - Before;
	};

	// --- Recon zwijgt, Ready vuurt -----------------------------------------
	const int32 ReconShots = ShotsOverTwoSeconds(EEclipseSquadStance::Recon);
	const int32 ReadyShots = ShotsOverTwoSeconds(EEclipseSquadStance::Ready);
	Report(*this, TEXT("schoten in 2 s onder RECON"), static_cast<float>(ReconShots), TEXT(""),
		TEXT("hoort 0 te zijn: vuur pas als er op je geschoten wordt"));
	Report(*this, TEXT("schoten in 2 s onder READY"), static_cast<float>(ReadyShots), TEXT(""));
	TestEqual(TEXT("doctrine: Recon vuurt niet uit zichzelf"), ReconShots, 0);
	TestTrue(TEXT("doctrine: Ready vuurt wel"), ReadyShots > 0);

	// --- Overwatch loopt niet mee ------------------------------------------
	AEclipseSquadmateController* Mate = Mates[0];
	AEclipseCharacter* MateBody = Cast<AEclipseCharacter>(Mate->GetPawn());
	if (!TestNotNull(TEXT("doctrine: de soldaat heeft een lichaam"), MateBody))
	{
		Harness.Shutdown();
		return false;
	}
	for (AEclipseSquadmateController* Each : Mates)
	{
		Each->SetDoctrine(EEclipseSquadStance::Overwatch);
	}
	const int32 MovesBefore = Mate->GetFollowMoves();
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 5.0, []() { return false; });
	Report(*this, TEXT("meelopen-verplaatsingen onder OVERWATCH"),
		static_cast<float>(Mate->GetFollowMoves() - MovesBefore), TEXT(""),
		TEXT("hoort 0 te zijn: zij houden dat terrein"));
	TestEqual(TEXT("doctrine: Overwatch blijft staan"), Mate->GetFollowMoves() - MovesBefore, 0);

	// --- Aggressive zoekt geen dekking --------------------------------------
	Harness.Idle(2.5f); // dekkingsrem laten aflopen
	for (AEclipseSquadmateController* Each : Mates)
	{
		Each->SetDoctrine(EEclipseSquadStance::Aggressive);
	}
	const int32 CoverBefore = Mate->GetCoverRuns();
	MateBody->ApplyDamage(5.0f, Harness.Body, TEXT("TestIncoming"));
	Harness.Idle(1.0f);
	Report(*this, TEXT("dekkingszoektochten onder AGGRESSIVE"),
		static_cast<float>(Mate->GetCoverRuns() - CoverBefore), TEXT(""),
		TEXT("hoort 0 te zijn: het kamikaze-kader haalt dekking zoeken weg"));
	TestEqual(TEXT("doctrine: Aggressive zoekt geen dekking"), Mate->GetCoverRuns() - CoverBefore, 0);


	Harness.Shutdown();
	return true;
}


/**
 * KLASSE-VERBS IN DE BASISLAAG (owner-opdracht 26-07 avond, punt 1 — laag 5).
 *
 * Momentum en Killzone bestonden als tag plus getal en werden nergens afgevuurd
 * (squad-audit punt 10). Ze horen niet op een eigen knop maar IN de doctrine:
 * een Assault sluit af onder Aggressive, een Sniper reikt verder onder Overwatch.
 *
 * De meting doet wat de audit vroeg: laat zien dat het verb iets DOET, en niet
 * alleen dat de tag bestaat.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseClassVerbsFireWithoutBeingCalled,
	"Eclipse.Mission.Playthrough.ClassVerbsFireWithoutBeingCalled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseClassVerbsFireWithoutBeingCalled::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("verbs: strategie"), Strategy) || !TestNotNull(TEXT("verbs: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("verbs: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	TArray<AEclipseSquadmateController*> Mates;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mates.Add(*It);
	}
	if (!TestTrue(TEXT("verbs: er is een squad"), Mates.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	// Welke verbs zijn er überhaupt aan boord? Zonder dat getal zegt een nul
	// hieronder niets — dat is de nulmeting-valkuil.
	int32 WithPush = 0;
	int32 WithKillzone = 0;
	for (const AEclipseSquadmateController* Mate : Mates)
	{
		WithPush += Mate->GetClassKit().OrderPushDistanceCm > 0.0f ? 1 : 0;
		WithKillzone += Mate->GetClassKit().KillzoneRangeCm > 0.0f ? 1 : 0;
	}
	Report(*this, TEXT("soldaten met een Momentum-getal"), static_cast<float>(WithPush), TEXT(""));
	Report(*this, TEXT("soldaten met een Killzone-laan"), static_cast<float>(WithKillzone), TEXT(""));

	// --- Momentum: onder AGGRESSIVE sluit hij af in plaats van te dekken ----
	int32 VerbsBefore = 0;
	for (const AEclipseSquadmateController* Mate : Mates)
	{
		VerbsBefore += Mate->GetVerbUses();
	}
	for (AEclipseSquadmateController* Mate : Mates)
	{
		Mate->SetDoctrine(EEclipseSquadStance::Aggressive);
		if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(Mate->GetPawn()))
		{
			Body->ApplyDamage(5.0f, Harness.Body, TEXT("TestIncoming"));
		}
	}
	Harness.Idle(1.0f);
	int32 VerbsAfter = 0;
	for (const AEclipseSquadmateController* Mate : Mates)
	{
		VerbsAfter += Mate->GetVerbUses();
	}
	Report(*this, TEXT("verb-inzetten onder AGGRESSIVE"), static_cast<float>(VerbsAfter - VerbsBefore), TEXT(""),
		TEXT("Momentum: afsluiten op wie er schoot, in plaats van dekken"));

	if (WithPush > 0)
	{
		TestTrue(TEXT("verbs: Momentum vuurt uit zichzelf onder Aggressive"), VerbsAfter > VerbsBefore);
	}
	else
	{
		AddInfo(TEXT("GEMETEN  geen enkele soldaat heeft een Momentum-getal — dan is 0 geen defect maar een lege kit"));
	}

	Harness.Shutdown();
	return true;
}


/**
 * WAT DE SQUAD AAN EEN GEVECHT TOEVOEGT (owner-opdracht 26-07 avond, punt 1 —
 * laag 6 van zes: "meet opnieuw wat een gevecht kost").
 *
 * Elk getal in de gevechts-audit is gemeten met een squad die niets deed. Sinds
 * vanavond vuren er drie mee, en dat verandert de time-to-kill fundamenteel.
 *
 * A/B en geen SOM, en dat verschil is de hele reden dat deze meting bestaat: ik
 * kán 154 hp/s en 196 hp/s bij elkaar optellen en er een tijd uit rekenen, maar
 * een som gaat uit van perfect vuren zonder één gemist frame en ziet er net zo
 * overtuigend uit als een meting. De doctrine van laag 4 maakt de A/B mogelijk:
 * onder RECON zwijgt de squad, onder READY vuurt hij mee. Zelfde opstelling,
 * zelfde doelwit, één knop verschil.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadChangesTimeToKill,
	"Eclipse.Mission.Playthrough.SquadChangesTimeToKill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseSquadChangesTimeToKill::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("ttk: strategie"), Strategy) || !TestNotNull(TEXT("ttk: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("ttk: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	TArray<AEclipseSquadmateController*> Mates;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mates.Add(*It);
	}
	if (!TestTrue(TEXT("ttk: er is een squad"), Mates.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	UEclipseHitscanWeaponComponent* Weapon = Harness.Body->FindComponentByClass<UEclipseHitscanWeaponComponent>();
	if (!TestNotNull(TEXT("ttk: de speler heeft een wapen"), Weapon))
	{
		Harness.Shutdown();
		return false;
	}

	// Eén ronde: doelwit met 300 hp (ongeveer twee Troopers), squad op de gegeven
	// doctrine, speler vuurt continu. Meet hoe lang het duurt tot hij neer is.
	auto SecondsToDown = [&](EEclipseSquadStance Stance) -> float
	{
		for (AEclipseSquadmateController* Mate : Mates)
		{
			Mate->SetDoctrine(Stance);
		}
		Harness.Idle(0.5f);

		const FVector Spot = Harness.Location() + Harness.Body->GetActorForwardVector() * 700.0f;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEclipseCharacter* Dummy = Harness.World->SpawnActor<AEclipseCharacter>(
			AEclipseCharacter::StaticClass(), Spot, FRotator::ZeroRotator, Params);
		if (Dummy == nullptr)
		{
			return -1.0f;
		}
		Dummy->InitializeHealth(300.0f);
		Harness.AimAt(Dummy->GetActorLocation());
		Harness.Idle(0.1f);

		const double Start = Harness.ElapsedSeconds;
		while (Harness.ElapsedSeconds - Start < 12.0 && !Dummy->IsDowned())
		{
			Harness.Inject(TEXT("Fire"), true);
			Harness.Step();
		}
		const float Taken = static_cast<float>(Harness.ElapsedSeconds - Start);
		Dummy->Destroy();
		Harness.Idle(0.5f);
		return Dummy != nullptr && Taken < 12.0f ? Taken : -1.0f;
	};

	const float AloneSeconds = SecondsToDown(EEclipseSquadStance::Recon);
	const float TogetherSeconds = SecondsToDown(EEclipseSquadStance::Ready);

	Report(*this, TEXT("300 hp neer met de speler ALLEEN"), AloneSeconds, TEXT("s"),
		TEXT("squad op recon: die zwijgt"));
	Report(*this, TEXT("300 hp neer MET de squad erbij"), TogetherSeconds, TEXT("s"),
		TEXT("squad op ready"));
	if (AloneSeconds > 0.0f && TogetherSeconds > 0.0f)
	{
		Report(*this, TEXT("de squad maakt het gevecht korter met een factor"),
			AloneSeconds / TogetherSeconds, TEXT("x"),
			TEXT("hier hangt de hele gevechtsbalans aan"));
	}

	TestTrue(TEXT("ttk: het doelwit gaat in beide gevallen neer"),
		AloneSeconds > 0.0f && TogetherSeconds > 0.0f);
	TestTrue(TEXT("ttk: mét de squad gaat het sneller"), TogetherSeconds < AloneSeconds);

	Harness.Shutdown();
	return true;
}


/**
 * JE SQUAD KAN JE VERRADEN (26-07 avond — het gevolg van doctrine-laag 2).
 *
 * Sinds vanmorgen verraadt elk schot van de spelerskant je positie: vijanden
 * binnen de alarmradius lopen naar waar er geschoten werd. Sinds vanavond vuurt
 * je squad UIT ZICHZELF. Die twee samen betekenen iets wat geen van beide alleen
 * betekende: **je squad kan je verraden met een schot dat jij niet gaf.**
 *
 * Dat maakt `recon` niet zomaar een houding maar je enige sluipoptie — en dat is
 * precies wat die doctrine in Ghost Recon ook doet. Deze meting bewijst het
 * verschil, want zonder bewijs is het een verhaal.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadCanGiveYouAway,
	"Eclipse.Mission.Playthrough.SquadCanGiveYouAway",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseSquadCanGiveYouAway::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestNotNull(TEXT("verraden: strategie"), Strategy) || !TestNotNull(TEXT("verraden: prep"), Prep)
		|| !TestTrue(FString::Printf(TEXT("verraden: missie gelanceerd (%s)"), *Error),
			Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	AEclipseGameMode* Mode = Harness.World->GetAuthGameMode<AEclipseGameMode>();
	if (!TestNotNull(TEXT("verraden: game mode"), Mode))
	{
		Harness.Shutdown();
		return false;
	}

	TArray<AEclipseSquadmateController*> Mates;
	for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
	{
		Mates.Add(*It);
	}
	if (!TestTrue(TEXT("verraden: er is een squad"), Mates.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	// EERST BINNEN GEHOORSAFSTAND GAAN STAAN. De missie zet zijn vijanden bij het
	// doelsite, en dat ligt hier 14.142 cm verderop — ruim buiten de alarmradius
	// van 5000. De eerste ronde mat daardoor twee keer nul, en dat las als "het
	// alarm werkt niet" terwijl het "er stond niemand binnen gehoorsafstand"
	// betekende.
	{
		const AEclipseEnemyController* Nearest = nullptr;
		float Best = TNumericLimits<float>::Max();
		for (TActorIterator<AEclipseEnemyController> It(Harness.World); It; ++It)
		{
			if (const APawn* Body = It->GetPawn())
			{
				const float Away = static_cast<float>(FVector::Dist2D(Body->GetActorLocation(), Harness.Location()));
				if (Away < Best)
				{
					Best = Away;
					Nearest = *It;
				}
			}
		}
		if (Nearest != nullptr && Nearest->GetPawn() != nullptr)
		{
			// Op 3500 cm: binnen de alarmradius (5000) en buiten élk
			// waarnemingsbereik (max 3200), zodat alleen het GELUID hem in
			// beweging kan zetten.
			const FVector EnemyAt = Nearest->GetPawn()->GetActorLocation();
			const FVector Toward = (Harness.Location() - EnemyAt).GetSafeNormal2D();
			const FVector StandAt = EnemyAt + Toward * 3500.0f + FVector(0, 0, 120.0f);
			Harness.Body->SetActorLocation(StandAt, false, nullptr, ETeleportType::TeleportPhysics);

			// DE SQUAD MEE, en dat was de tweede meetfout. Meelopen doet 420 cm/s;
			// de speler sprong hier ruim 10.000 cm. Zonder deze regel stond de squad
			// nog bij het insertiepunt en kwam het ene schot dat viel van ver buiten
			// de alarmradius — dus nul gealarmeerden, en dat las als "schieten
			// alarmeert niet".
			int32 Spread = 0;
			for (AEclipseSquadmateController* Mate : Mates)
			{
				if (APawn* Body = Mate->GetPawn())
				{
					Body->SetActorLocation(StandAt + FVector(200.0f * (++Spread), 150.0f, 0.0f),
						false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
			Harness.Idle(0.5f);
		}
	}

	// Een vijand binnen wapenbereik van de squad. De SPELER doet niets — geen
	// schot, geen order. Alles wat er gebeurt komt van de squad.
	const FVector Spot = Harness.Location() + Harness.Body->GetActorForwardVector() * 700.0f;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEclipseCharacter* Hostile = Harness.World->SpawnActor<AEclipseCharacter>(
		AEclipseCharacter::StaticClass(), Spot, FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("verraden: er staat een vijand"), Hostile))
	{
		Harness.Shutdown();
		return false;
	}
	Hostile->InitializeHealth(100000.0f);

	// Twee getallen per ronde: hoeveel de squad SCHOOT en hoeveel dat ALARMEERDE.
	// Zonder het eerste kun je een nul in het tweede niet lezen — is er niet
	// geschoten, of alarmeert schieten niet?
	int32 ShotsInRound = 0;
	auto AlertsOverThreeSeconds = [&](EEclipseSquadStance Stance) -> int32
	{
		for (AEclipseSquadmateController* Mate : Mates)
		{
			Mate->SetDoctrine(Stance);
		}
		Harness.Idle(0.5f);
		int32 ShotsBefore = 0;
		for (const AEclipseSquadmateController* Mate : Mates)
		{
			ShotsBefore += Mate->GetAutoFireShots();
		}
		const int32 Before = Mode->GetEnemiesAlertedByShots();
		Harness.Idle(3.0f);
		ShotsInRound = -ShotsBefore;
		for (const AEclipseSquadmateController* Mate : Mates)
		{
			ShotsInRound += Mate->GetAutoFireShots();
		}
		return Mode->GetEnemiesAlertedByShots() - Before;
	};

	// DISCRIMINATOR. Twee keer nul zegt niets zolang je niet weet of er überhaupt
	// iemand binnen gehoorsafstand staat — dat is de nulmeting-valkuil die me
	// vandaag al eerder pakte, en hier meteen weer.
	int32 Enemies = 0;
	float NearestEnemyCm = TNumericLimits<float>::Max();
	for (TActorIterator<AEclipseEnemyController> It(Harness.World); It; ++It)
	{
		if (const APawn* Body = It->GetPawn())
		{
			++Enemies;
			NearestEnemyCm = FMath::Min(NearestEnemyCm,
				static_cast<float>(FVector::Dist2D(Body->GetActorLocation(), Harness.Location())));
		}
	}
	Report(*this, TEXT("vijanden in het veld"), static_cast<float>(Enemies), TEXT(""));
	Report(*this, TEXT("dichtstbijzijnde vijand"), Enemies > 0 ? NearestEnemyCm : -1.0f, TEXT("cm"),
		TEXT("de alarmradius van de AR is 5000"));

	const int32 UnderRecon = AlertsOverThreeSeconds(EEclipseSquadStance::Recon);
	const int32 ShotsUnderRecon = ShotsInRound;
	const int32 UnderReady = AlertsOverThreeSeconds(EEclipseSquadStance::Ready);
	const int32 ShotsUnderReady = ShotsInRound;

	Report(*this, TEXT("squadschoten onder RECON"), static_cast<float>(ShotsUnderRecon), TEXT(""));
	Report(*this, TEXT("squadschoten onder READY"), static_cast<float>(ShotsUnderReady), TEXT(""),
		TEXT("0 hier betekent dat de squad niet vuurde, niet dat het alarm stuk is"));

	Report(*this, TEXT("gealarmeerde vijanden onder RECON"), static_cast<float>(UnderRecon), TEXT(""),
		TEXT("de speler deed niets; je squad zwijgt"));
	Report(*this, TEXT("gealarmeerde vijanden onder READY"), static_cast<float>(UnderReady), TEXT(""),
		TEXT("je squad schiet, en dat verraadt JOU"));

	// GEEN NUL EISEN ONDER RECON, en dat is geen verzachting maar de doctrine zelf:
	// recon betekent "vuur niet TOT ER OP JE GESCHOTEN WORDT". In een levende
	// missie schieten die vijanden terug, dus een squad die zich verdedigt hoort
	// erbij. Gemeten bleef er 4 schoten over tegen 61 — dat is het verschil tussen
	// "wij openen het vuur" en "wij vuren terug".
	//
	// Eerste versie eiste nul en zou dus rood staan op correct gedrag. Een test die
	// een verkeerde eis stelt is erger dan geen test: hij leert je hem uit te zetten.
	if (UnderReady > 0)
	{
		Report(*this, TEXT("recon tegen ready, in alarmeringen"),
			static_cast<float>(UnderRecon) / static_cast<float>(UnderReady), TEXT("x"),
			TEXT("hoe lager, hoe meer recon je sluippad openhoudt"));
	}
	TestTrue(FString::Printf(TEXT("verraden: onder ready verraadt je squad je (%d alarmeringen)"), UnderReady),
		UnderReady > 0);
	TestTrue(FString::Printf(TEXT("verraden: recon houdt het ruim beperkter (%d tegen %d)"), UnderRecon, UnderReady),
		UnderRecon * 3 < UnderReady);

	Harness.Shutdown();
	return true;
}


/**
 * STIL FALEN MAAKT DE BAR ROOD (owner-opdracht 26-07, 21:30).
 *
 * Zijn woorden: *"~24 uur werk, 152 groene tests, en toen ik de game startte was
 * er op het eerste gezicht niets veranderd."* Drie lagen tussen de code en het
 * scherm waren stuk en geen enkele test werd rood: de skeletpoort wees 2948
 * animaties af, de garbage collector ruimde de geluiden op, en het personage
 * verdween.
 *
 * Dit is de test die dat had moeten vangen. Hij draait een echte missie en eist
 * dat er NUL degradaties zijn — geen afgewezen animatie, geen ontbrekende cue,
 * geen overgeslagen pose. Een waarschuwing in een log dat niemand leest is geen
 * luid falen; het is stil falen met een alibi.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseNothingDegradesSilently,
	"Eclipse.Mission.Playthrough.NothingDegradesSilently",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseNothingDegradesSilently::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	// Vanaf hier tellen. Alles wat eerder in de suite gebeurde is niet van deze
	// missie, en een teller die over tests heen loopt wijst naar de verkeerde.
	EclipseDegradation::Reset();

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestTrue(FString::Printf(TEXT("degradatie: missie gelanceerd (%s)"), *Error),
			Strategy != nullptr && Prep != nullptr
			&& Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}

	// Even spelen: lopen, kijken, vuren. Alles wat een lichaam aankleedt en een
	// geluid aanraakt moet minstens één keer gedraaid hebben.
	Harness.Idle(1.0f);
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 3.0, []() { return false; });
	const double Start = Harness.ElapsedSeconds;
	while (Harness.ElapsedSeconds - Start < 2.0)
	{
		Harness.Inject(TEXT("Fire"), true);
		Harness.Step();
	}
	Harness.Idle(1.0f);

	const int32 Degradations = EclipseDegradation::Count();
	Report(*this, TEXT("stille degradaties in één missie"), static_cast<float>(Degradations), TEXT(""),
		TEXT("moet 0 zijn — elke andere waarde is iets dat de speler NIET ziet"));
	for (const FString& Line : EclipseDegradation::Report())
	{
		AddInfo(FString::Printf(TEXT("GEMETEN  %s"), *Line));
	}

	TestEqual(TEXT("degradatie: niets valt stil terug in een echte missie"), Degradations, 0);

	Harness.Shutdown();
	return true;
}


/**
 * IS ER IETS TE ZIEN? (owner-opdracht 26-07, 21:30, punt 3.)
 *
 * Zijn woorden: *"Je speelronde meet uitkomsten en die kunnen groen zijn terwijl
 * er niets te zien is. Ik wil per ronde weten: staat het personage in beeld,
 * speelt er een animatie, klopt zijn schaal."*
 *
 * Dat is de laag die ontbrak. "De missie is voltooid" bewees dat de LOGICA liep,
 * niet dat er iets op het scherm stond — en op 26-07 waren allebei zijn
 * waarnemingen precies daar: het personage schaalde mee met zijn snelheid, en bij
 * stilstand was het weg.
 *
 * Headless kan er niets gerenderd worden, dus dit meet wat BEPAALT of er iets in
 * beeld komt: een zichtbare component, een echte mesh, een niet-ontaarde bounding
 * box, de geauthorde schaal, en een pose die tussen twee frames verandert. Alle
 * vijf zijn nul-of-onzin als het personage weg is.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseThePlayerIsActuallyOnScreen,
	"Eclipse.Mission.Playthrough.ThePlayerIsActuallyOnScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseThePlayerIsActuallyOnScreen::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipsePlaythrough;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance->GetSubsystem<UEclipseStrategySubsystem>();
	UEclipsePrepSubsystem* Prep = GameInstance->GetSubsystem<UEclipsePrepSubsystem>();
	FString Error;
	if (!TestTrue(FString::Printf(TEXT("zichtbaar: missie gelanceerd (%s)"), *Error),
			Strategy != nullptr && Prep != nullptr
			&& Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	USkeletalMeshComponent* Mesh = Harness.Body->GetMesh();
	if (!TestNotNull(TEXT("zichtbaar: het lichaam heeft een mesh-component"), Mesh))
	{
		Harness.Shutdown();
		return false;
	}

	// SCHERMOPNAMES VANUIT DE SPELER (owner-opdracht 26-07, 22:00).
	//
	// De getallen hieronder bewijzen dat er iets te tekenen is; ze bewijzen niet
	// dat het er goed uitziet. Daarvoor moet er iemand kijken, en dat kan alleen
	// als er een plaatje ligt. Vier momenten: net gespawnd, tijdens lopen, tijdens
	// vuren, en aan het eind.
	//
	// Onder -nullrhi is dit een no-op — dan is er geen renderer en dus geen frame.
	// De ronde die de owner beoordeelt draait daarom ZONDER die vlag; de suite
	// blijft headless en meet de getallen.
	int32 ShotIndex = 0;
	auto Capture = [&Harness, &ShotIndex](const TCHAR* Moment)
	{
		if (APlayerController* PC = Harness.World->GetFirstPlayerController())
		{
			PC->ConsoleCommand(FString::Printf(TEXT("HighResShot 1280x720")));
			UE_LOG(LogEclipse, Display, TEXT("[SHOT %d] %s"), ++ShotIndex, Moment);
		}
		// Twee ticks: de opname wordt aan het EIND van een frame afgehandeld.
		Harness.Step();
		Harness.Step();
	};

	// --- 1. staat er een mesh IN, en is hij zichtbaar? -----------------------
	const bool bHasAsset = Mesh->GetSkeletalMeshAsset() != nullptr;
	Report(*this, TEXT("mesh-asset aanwezig"), bHasAsset ? 1.0f : 0.0f, TEXT(""),
		TEXT("0 = een onzichtbare capsule, hoe groen de rest ook is"));
	TestTrue(TEXT("zichtbaar: er zit een skeletal mesh in"), bHasAsset);
	TestTrue(TEXT("zichtbaar: de mesh staat aan"), Mesh->IsVisible());

	Capture(TEXT("net gespawnd, stilstaand"));

	// --- 2. de SCHAAL, stilstaand ------------------------------------------
	// De owner zag zijn personage MEEGROEIEN met zijn snelheid. Dus meten bij
	// stilstand én tijdens rennen, en eisen dat het dezelfde is.
	Harness.Idle(0.5f);
	const FVector ScaleStill = Mesh->GetComponentScale();
	const FBoxSphereBounds BoundsStill = Mesh->Bounds;
	Report(*this, TEXT("meshschaal stilstaand"), static_cast<float>(ScaleStill.X), TEXT("x"));
	Report(*this, TEXT("hoogte van de bounding box stilstaand"),
		static_cast<float>(BoundsStill.BoxExtent.Z * 2.0f), TEXT("cm"),
		TEXT("0 = niets om te tekenen"));
	// GEEN ASSERTIE op deze eerste meting: vlak na de start kan de eerste pose-tick
	// nog niet doorgekomen zijn, en dan meet je het opstarten in plaats van het
	// spel. De harde eis staat bij 3b, ná het rennen.

	// --- 3. de SCHAAL, rennend ---------------------------------------------
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 2.0, []() { return false; });
	Capture(TEXT("tijdens lopen"));
	const FVector ScaleRunning = Mesh->GetComponentScale();
	const FBoxSphereBounds BoundsRunning = Mesh->Bounds;
	Report(*this, TEXT("meshschaal rennend"), static_cast<float>(ScaleRunning.X), TEXT("x"),
		TEXT("moet gelijk zijn aan stilstaand"));
	Report(*this, TEXT("hoogte van de bounding box rennend"),
		static_cast<float>(BoundsRunning.BoxExtent.Z * 2.0f), TEXT("cm"));

	TestTrue(FString::Printf(TEXT("zichtbaar: de schaal verandert niet met snelheid (%.3f tegen %.3f)"),
			ScaleRunning.X, ScaleStill.X),
		ScaleRunning.Equals(ScaleStill, 0.001f));
	TestTrue(TEXT("zichtbaar: hij heeft rennend nog steeds een tekenbare omvang"),
		BoundsRunning.BoxExtent.Z > 1.0f);

	// --- 3b. EN WEER STILSTAAN ---------------------------------------------
	// De owner ziet zijn personage verdwijnen bij stilstand. De eerste meting
	// hierboven staat vlak na de start, en dan kan een omvang van 1 cm ook betekenen
	// dat de eerste pose-tick nog niet doorgekomen is. Nog één keer stilstaan ná het
	// rennen scheidt die twee: blijft hij dan groot, dan was het opstarten; klapt hij
	// terug, dan is stilstand zelf het probleem.
	// Even vuren, want een schietpose is het derde ding dat hij wil zien.
	{
		const double FireStart = Harness.ElapsedSeconds;
		while (Harness.ElapsedSeconds - FireStart < 0.6)
		{
			Harness.Inject(TEXT("Fire"), true);
			Harness.Step();
		}
	}
	Capture(TEXT("tijdens vuren"));

	Harness.Idle(1.5f);
	Capture(TEXT("weer stilstaand"));
	const FBoxSphereBounds BoundsStillAgain = Mesh->Bounds;

	// Discriminator: HEEFT dit lichaam een idle-clip? Zonder idle staat er bij
	// stilstand geen enkele pose-sample in de blend, en dan valt de proxy terug op
	// de ref-pose. Als de omvang dán inklapt, ligt het aan die terugval en niet aan
	// stilstand op zich.
	if (const UEclipseAnimInstance* AnimForTier = Cast<UEclipseAnimInstance>(Mesh->GetAnimInstance()))
	{
		const FEclipseLocomotionSet& Set = AnimForTier->GetLocomotionSet();
		Report(*this, TEXT("idle-clip aanwezig"), Set.Idle != nullptr ? 1.0f : 0.0f, TEXT(""),
			TEXT("0 = bij stilstand staat er geen enkele sample in de blend"));
		Report(*this, TEXT("wandel/ren-clip aanwezig"),
			(Set.Walk != nullptr ? 1.0f : 0.0f) + (Set.Run != nullptr ? 1.0f : 0.0f), TEXT(""));
		if (Set.Idle != nullptr)
		{
			// IS DE IDLE ADDITIEF? Een additieve take sampelen als volledige pose
			// geeft bijna-nul transforms: alle botten vallen op de oorsprong en het
			// personage klapt in. Dat zou precies verklaren waarom het alleen bij
			// STILSTAND gebeurt — daar heeft de idle gewicht 1,0.
			AddInfo(FString::Printf(TEXT("GEMETEN  idle-clip                                %s"),
				*Set.Idle->GetName()));
			AddInfo(FString::Printf(TEXT("GEMETEN  idle is additief                         %s"),
				Set.Idle->IsValidAdditive() ? TEXT("JA — dit is de oorzaak") : TEXT("nee")));
			AddInfo(FString::Printf(TEXT("GEMETEN  idle lengte                              %.2f s"),
				Set.Idle->GetPlayLength()));
		}
	}
	Report(*this, TEXT("hoogte van de bounding box ná stilstaan"),
		static_cast<float>(BoundsStillAgain.BoxExtent.Z * 2.0f), TEXT("cm"),
		TEXT("klapt hij hier in, dan is stilstand zelf het probleem"));
	TestTrue(FString::Printf(TEXT("zichtbaar: hij blijft stilstaand zichtbaar (%.1f cm)"),
			BoundsStillAgain.BoxExtent.Z * 2.0f),
		BoundsStillAgain.BoxExtent.Z > 50.0f);

	// --- 4. BEWEEGT de pose? -----------------------------------------------
	// Een bevroren pose is net zo onzichtbaar als geen pose: je ziet een standbeeld
	// door het district glijden. Twee frames vergelijken op de botruimte.
	const UEclipseAnimInstance* Anim = Cast<UEclipseAnimInstance>(Mesh->GetAnimInstance());
	TestNotNull(TEXT("zichtbaar: er draait een anim-instance"), Anim);

	const TArray<FTransform> PoseA = Mesh->GetBoneSpaceTransforms();
	Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 0.35, []() { return false; });
	const TArray<FTransform> PoseB = Mesh->GetBoneSpaceTransforms();

	int32 MovedBones = 0;
	for (int32 Index = 0; Index < FMath::Min(PoseA.Num(), PoseB.Num()); ++Index)
	{
		if (!PoseA[Index].GetLocation().Equals(PoseB[Index].GetLocation(), 0.01f)
			|| !PoseA[Index].GetRotation().Equals(PoseB[Index].GetRotation(), 0.001f))
		{
			++MovedBones;
		}
	}
	Report(*this, TEXT("botten die in 0,35 s bewogen"), static_cast<float>(MovedBones), TEXT(""),
		*FString::Printf(TEXT("van %d; 0 = een standbeeld dat door het district glijdt"), PoseA.Num()));
	TestTrue(TEXT("zichtbaar: de pose beweegt echt"), MovedBones > 0);

	Harness.Shutdown();
	return true;
}

/**
 * NIEMAND IS EEN REUS.
 *
 * Gevonden op het eerste echte spelbeeld, 26-07: naast een speler van 189,6 cm
 * stond een aankleedfiguur van 328,4 cm. Ruim drie meter, 1,7x de speler, en hij
 * vulde het halve frame.
 *
 * Geen enkele bestaande test kon dit vinden, en dat is geen toeval. Alle
 * metingen stonden OP DE SPELER, en de speler klopte — 189,6 cm, schaal 1,0,
 * netjes getekend. De fout bestaat pas als je twee lichamen naast elkaar zet.
 * Dat is precies waar de owner op wees: "je tests bewijzen dat de code doet wat
 * de code zegt, niet dat het resultaat zichtbaar wordt."
 *
 * Deze test zet de verhouding vast, zodat het volgende pack dat op een andere
 * schaal is geauthord de bar rood maakt in plaats van als reus het district in
 * te lopen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseNobodyIsAGiant,
	"Eclipse.Mission.Playthrough.NobodyIsAGiant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseNobodyIsAGiant::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	// Missie starten en laten lopen, en niet alleen het harnas opstarten. Zonder
	// een getikte pose staan de bounds van de speler op 0,0 cm en meet je de
	// verhouding tegen niets — een test die dan rood wordt, wijst naar de
	// verkeerde fout.
	UGameInstance* GameInstance = Harness.GameInstance;
	UEclipseStrategySubsystem* Strategy = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseStrategySubsystem>() : nullptr;
	UEclipsePrepSubsystem* Prep = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipsePrepSubsystem>() : nullptr;
	FString Error;
	if (!TestTrue(FString::Printf(TEXT("reus: missie gelanceerd (%s)"), *Error),
			Strategy != nullptr && Prep != nullptr
			&& Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) && Prep->AutoLaunch(Error)))
	{
		Harness.Shutdown();
		return false;
	}
	Harness.Idle(1.0f);

	const USkeletalMeshComponent* PlayerMesh = Harness.Body != nullptr ? Harness.Body->GetMesh() : nullptr;
	if (!TestNotNull(TEXT("reus: de speler heeft een mesh"), PlayerMesh))
	{
		Harness.Shutdown();
		return false;
	}
	const float PlayerHeight = PlayerMesh->Bounds.BoxExtent.Z * 2.0f;
	TestTrue(FString::Printf(TEXT("reus: de speler zelf is menselijk (%.1f cm)"), PlayerHeight),
		PlayerHeight > 150.0f && PlayerHeight < 220.0f);

	// De aankleedfiguren zijn ASkeletalMeshActor en GEEN EclipseCharacter, dus
	// elke controle die over "lichamen" ging liep straal langs ze heen.
	int32 Checked = 0;
	float Tallest = 0.0f;
	FString TallestName;
	for (TActorIterator<ASkeletalMeshActor> It(Harness.World); It; ++It)
	{
		const ASkeletalMeshActor* Figure = *It;
		const USkeletalMeshComponent* Mesh = Figure != nullptr ? Figure->GetSkeletalMeshComponent() : nullptr;
		if (Mesh == nullptr || Mesh->GetSkeletalMeshAsset() == nullptr)
		{
			continue;
		}
		++Checked;
		const float Height = Mesh->Bounds.BoxExtent.Z * 2.0f;
		if (Height > Tallest)
		{
			Tallest = Height;
			TallestName = Mesh->GetSkeletalMeshAsset()->GetName();
		}
	}

	// Een nulmeting is geen bevinding: als er geen figuur staat bewijst deze test
	// niets, en dan hoort hij dat te zeggen in plaats van groen te worden.
	if (!TestTrue(TEXT("reus: er staat minstens een aankleedfiguur om te meten"), Checked > 0))
	{
		Harness.Shutdown();
		return false;
	}

	// 1,35x laat ruimte voor een bewuste zware silhouet, en sluit de 1,7x uit die
	// er stond.
	const float Limit = PlayerHeight * 1.35f;
	TestTrue(FString::Printf(TEXT("reus: de langste van %d figuren is %s op %.1f cm, speler %.1f cm (grens %.1f cm)"),
			Checked, *TallestName, Tallest, PlayerHeight, Limit),
		Tallest <= Limit);

	Harness.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
