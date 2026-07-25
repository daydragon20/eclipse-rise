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

#include "Characters/EclipseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/EclipsePlayerController.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
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
#include "Squad/EclipseSquadSubsystem.h"
#include "Squad/EclipseSquadTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
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
	FEclipseEventSubscriptionHandle RewardHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&CreditsRewarded, &MaterialsRewarded](FGameplayTag, const FInstancedStruct& Payload)
		{
			const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>();
			if (Economy == nullptr || Economy->Reason != TEXT("MissionReward"))
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

	Report(*this, TEXT("afgelegde weg naar het controlepost"), FVector::Dist2D(Harness.Location(), StartLocation), TEXT("cm"));
	Report(*this, TEXT("resterende afstand tot het site"), FVector::Dist2D(Harness.Location(), SiteControlPost), TEXT("cm"));
	Report(*this, TEXT("ticks waarin gevuurd is"), FiringTicks, TEXT(""), TEXT("> 0 — er MOET geschoten worden"));
	TestFalse(TEXT("speelronde: de speler overleefde het vuurgevecht"), Harness.Body->IsDowned());
	TestTrue(TEXT("speelronde: er is daadwerkelijk gevuurd"), FiringTicks > 0);

	Report(*this, TEXT("vijanden neergehaald"), HostilesDowned, TEXT(""), TEXT("minstens het doelwit"));
	TestTrue(TEXT("speelronde: er ging minstens één vijand neer"), HostilesDowned > 0);

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
	for (const FEclipseObjectiveDef& Objective : Mission->GetActiveObjectives())
	{
		if (Objective.Type == EEclipseObjectiveType::ReachLocation || Objective.Type == EEclipseObjectiveType::CollectItem)
		{
			Mission->NotifySiteEntered(Objective.TargetId);
			TestTrue(FString::Printf(TEXT("pin: aanwezigheid vervult '%s' wél"), *Objective.ObjectiveId.ToString()),
				Mission->GetCompletedObjectiveIds().Contains(Objective.ObjectiveId));
		}
	}

	GameInstance->Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
