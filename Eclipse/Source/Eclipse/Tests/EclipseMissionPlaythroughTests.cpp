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
	const bool bEngaged = DamageTaken > 0.0f || FurthestHostileMove > 200.0f;
	TestTrue(TEXT("contact: de vijand doet iets als je zijn bereik in loopt (nadert of vuurt terug)"), bEngaged);
	TestTrue(TEXT("contact: de speler is daadwerkelijk binnen hun waarnemingsbereik gekomen"),
		ClosestApproach < 3000.0f);

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

#endif // WITH_DEV_AUTOMATION_TESTS
