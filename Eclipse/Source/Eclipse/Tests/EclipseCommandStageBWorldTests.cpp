// SPEC-P2-02 Stage B — de in-wereld helft.
//
// De pure suite (EclipseCommandStageBTests.cpp) bewijst dat de TABEL klopt. Dat
// is niet hetzelfde als bewijzen dat er iets gebeurt. Deze ronde meet het effect:
//
//   · elke nieuwe verb loopt over het BESTAANDE ordercontract — één Issued, één
//     antwoord, en de wandklokmeter van SPEC-P1-06 telt hem mee. Ontstaat er ooit
//     een tweede pad dat om IssueOrder heen gaat, dan loopt die telling uiteen;
//   · de drie nieuwe weigeringen gaan in een ECHTE wereld af, mét gesproken regel,
//     en ze zijn daarna ook weer op te lossen (een weigering die nooit overgaat is
//     een muur, geen antwoord);
//   · stealth verandert GEDRAG: schoten geteld, niet een vlag gelezen;
//   · flank beweegt pas ná goedkeuring, suppress legt echt rondes in het gebied,
//     en een breach gaat pas naar binnen als iedereen staat.
//
// Waarom de echte game mode: de squad, de navmesh en de vijanden komen daarvandaan.
// Een kale wereld zou de helft van deze beweringen tot een formule terugbrengen.

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/EclipseEnemyController.h"
#include "AI/EclipseSquadmateController.h"
#include "Base/EclipsePrepSubsystem.h"
#include "Characters/EclipseCharacter.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Squad/EclipseBreachPoint.h"
#include "Squad/EclipseCommandModeTuning.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Squad/EclipseSquadSubsystem.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "Tests/EclipseFeelHarness.h"

namespace EclipseStageBWorld
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Wat er van één order langskwam op de bus — de zero-silence-boekhouding. */
	struct FOrderWatch
	{
		int32 Issued = 0;
		int32 Acknowledged = 0;
		int32 Refused = 0;
		int32 Queued = 0;
		TArray<FName> AnsweredOrders;
		TArray<FName> Reasons;
		TArray<FString> Barks;
		TArray<FName> QueuedTransitions;

		int32 Answers() const { return Acknowledged + Refused; }
		void Reset() { *this = FOrderWatch(); }
	};

	/** Abonneer op de hele Event.Squad-familie en boek elk feit in. */
	FEclipseEventSubscriptionHandle Watch(UEclipseEventBusSubsystem& Bus, FOrderWatch& Into)
	{
		return Bus.Subscribe(
			FGameplayTag::RequestGameplayTag(TEXT("Event.Squad")),
			FEclipseEventNativeDelegate::CreateLambda([&Into](FGameplayTag Tag, const FInstancedStruct& Payload)
			{
				const FEclipseSquadEventPayload* Squad = Payload.GetPtr<FEclipseSquadEventPayload>();
				if (Squad == nullptr)
				{
					return;
				}
				if (Tag == EclipseTags::Event_Squad_OrderIssued)
				{
					++Into.Issued;
				}
				else if (Tag == EclipseTags::Event_Squad_OrderAcknowledged)
				{
					++Into.Acknowledged;
					Into.AnsweredOrders.Add(Squad->Order);
					Into.Barks.Add(Squad->BarkLine);
				}
				else if (Tag == EclipseTags::Event_Squad_OrderRefused)
				{
					++Into.Refused;
					Into.AnsweredOrders.Add(Squad->Order);
					Into.Reasons.Add(Squad->Reason);
					Into.Barks.Add(Squad->BarkLine);
				}
				else if (Tag == EclipseTags::Event_Squad_OrderQueued)
				{
					++Into.Queued;
					Into.QueuedTransitions.Add(Squad->Order);
				}
			}));
	}

	/** Start M1.1 via het ECHTE laadpad en lever de squad op; leeg = opzetten mislukt. */
	TArray<AEclipseSquadmateController*> LaunchAndCollectSquad(FAutomationTestBase& Test, EclipseFeelHarness::FHarness& Harness)
	{
		TArray<AEclipseSquadmateController*> Mates;
		UGameInstance* GameInstance = Harness.GameInstance;
		UEclipseStrategySubsystem* Strategy = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseStrategySubsystem>() : nullptr;
		UEclipsePrepSubsystem* Prep = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipsePrepSubsystem>() : nullptr;
		FString Error;
		if (Strategy == nullptr || Prep == nullptr
			|| !Strategy->SelectMission(TEXT("TransitCheckpoint"), Error) || !Prep->AutoLaunch(Error))
		{
			Test.AddError(FString::Printf(TEXT("Stage B: missie kon niet starten (%s)"), *Error));
			return Mates;
		}
		Harness.Idle(1.0f);
		for (TActorIterator<AEclipseSquadmateController> It(Harness.World); It; ++It)
		{
			Mates.Add(*It);
		}
		return Mates;
	}

	/** Een vijandig lichaam zonder controller: hij ziet niets en meldt niets — een stil doelwit. */
	AEclipseCharacter* SpawnUnawareHostile(UWorld& World, const FVector& Where, float Health = 100000.0f)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AEclipseCharacter* Hostile = World.SpawnActor<AEclipseCharacter>(
			AEclipseCharacter::StaticClass(), Where, FRotator::ZeroRotator, Params);
		if (Hostile != nullptr)
		{
			// Geen SetPlayerSide nodig: IsPlayerSide() is waar zodra een lichaam een
			// SoldierId heeft of bestuurd wordt. Vers gespawnd = per definitie vijandig.
			Hostile->InitializeHealth(Health);
		}
		return Hostile;
	}
}

/**
 * FALSIFICATIE 2 (niet-onderhandelbaar) — elke nieuwe verb loopt over het
 * BESTAANDE ordercontract.
 *
 * De meter die dit hard maakt is de wandklok-rondgangteller uit SPEC-P1-06. Die
 * telt binnen `UEclipseSquadSubsystem::IssueOrder` en nergens anders. Zou iemand
 * later een tweede pad bouwen — een `ExecuteFlankDirectly` naast het contract —
 * dan doet de soldaat wél iets terwijl deze telling níét meebeweegt, en dan loopt
 * hij hier uiteen met het aantal orders dat de test gaf.
 *
 * Daarnaast de zero-silence-boekhouding zelf: vijf orders, vijf Issued-feiten,
 * vijf antwoorden. Niet vier, niet zes.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBOneOrderContractTest,
	"Eclipse.Command.StageB.EveryNewVerbRunsOnTheOneOrderContract",
	EclipseStageBWorld::TestFlags)

bool FEclipseStageBOneOrderContractTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipseStageBWorld;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	const TArray<AEclipseSquadmateController*> Mates = LaunchAndCollectSquad(*this, Harness);
	UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>();
	UEclipseEventBusSubsystem* Bus = Harness.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestTrue(TEXT("contract: er is een squad"), Mates.Num() > 0)
		|| !TestNotNull(TEXT("contract: squad-subsystem"), Squad)
		|| !TestNotNull(TEXT("contract: event bus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	const TArray<FGuid> Alive = Squad->GetAliveSquadmateIds();
	if (!TestTrue(TEXT("contract: er staat iemand overeind"), Alive.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}

	// Een doelwit voor de verbs die er een nodig hebben.
	AEclipseCharacter* Hostile = SpawnUnawareHostile(*Harness.World,
		Harness.Location() + Harness.Body->GetActorForwardVector() * 900.0f);
	Harness.Idle(0.2f);

	FOrderWatch Watch;
	FEclipseEventSubscriptionHandle Handle = EclipseStageBWorld::Watch(*Bus, Watch);

	const TArray<EEclipseSquadOrder> NewVerbs = {
		EEclipseSquadOrder::Suppress,
		EEclipseSquadOrder::Flank,
		EEclipseSquadOrder::Breach,
		EEclipseSquadOrder::UseAbility,
		EEclipseSquadOrder::SyncStrike
	};

	const EclipseSquadOrderLogic::FEclipseOrderRoundTripStats Before = Squad->GetOrderRoundTripStats();
	const FVector AimPoint = Hostile != nullptr ? Hostile->GetActorLocation() : Harness.Location();

	int32 Dispatched = 0;
	for (const EEclipseSquadOrder Verb : NewVerbs)
	{
		// Per soldaat, want dat is het pad dat Command Mode gebruikt (locked
		// decision 4). De uitkomst maakt niet uit: geaccepteerd én geweigerd zijn
		// allebei antwoorden, en juist dat is de belofte die geteld wordt.
		if (Squad->IssueOrder(Alive[0], Verb, AimPoint, Hostile))
		{
			++Dispatched;
		}
	}
	Harness.Idle(0.2f);
	Bus->Unsubscribe(Handle);

	const EclipseSquadOrderLogic::FEclipseOrderRoundTripStats After = Squad->GetOrderRoundTripStats();
	const int32 NewSamples = After.SampleCount - Before.SampleCount;

	Report(*this, TEXT("orders uitgegeven"), static_cast<float>(Dispatched), TEXT(""), TEXT("vijf nieuwe verbs"));
	Report(*this, TEXT("Event.Squad.OrderIssued"), static_cast<float>(Watch.Issued), TEXT(""));
	Report(*this, TEXT("antwoorden (ack + refused)"), static_cast<float>(Watch.Answers()), TEXT(""));
	Report(*this, TEXT("rondgangmonsters erbij"), static_cast<float>(NewSamples), TEXT(""),
		TEXT("de SPEC-P1-06-meter telt alleen binnen IssueOrder"));

	TestEqual(TEXT("contract: alle vijf verbs zijn uitgegeven"), Dispatched, NewVerbs.Num());
	TestEqual(TEXT("contract: precies één OrderIssued per order"), Watch.Issued, Dispatched);
	TestEqual(TEXT("contract: precies één antwoord per order (nooit stilte, nooit dubbel)"),
		Watch.Answers(), Dispatched);

	// DE TWEEDE-PAD-DETECTOR. Elke nieuwe verb moet door dezelfde IssueOrder heen
	// zijn gegaan, en die telt zichzelf op de wandklok. Een verb die zijn eigen
	// route naar de squad zou vinden, laat dit getal achter.
	TestEqual(TEXT("contract: elke nieuwe verb is door de bestaande rondgangmeter gegaan"),
		NewSamples, Dispatched);

	// En het antwoord draagt de NAAM van het verb op het bestaande payload-veld —
	// geen eigen struct, geen eigen tag.
	for (const EEclipseSquadOrder Verb : NewVerbs)
	{
		const FName Expected(*UEnum::GetValueAsString(Verb));
		TestTrue(FString::Printf(TEXT("contract: %s reisde op FEclipseSquadEventPayload::Order"), *Expected.ToString()),
			Watch.AnsweredOrders.Contains(Expected));
	}

	// Elke weigering draagt een reden EN een gesproken regel. Een weigering zonder
	// zin is een stille weigering met extra stappen.
	for (const FName Reason : Watch.Reasons)
	{
		TestFalse(TEXT("contract: een weigering noemt altijd een reden"), Reason.IsNone());
	}
	for (const FString& Bark : Watch.Barks)
	{
		TestFalse(TEXT("contract: elk antwoord heeft een gesproken regel"), Bark.IsEmpty());
	}

	Harness.Shutdown();
	return true;
}

/**
 * FALSIFICATIE 1 (in-wereld helft) — de drie nieuwe redenen gaan in een ECHT
 * gevecht af, met een gesproken regel, en ze zijn daarna op te lossen.
 *
 * Dat laatste hoort erbij en wordt bijna altijd vergeten: een weigering die nooit
 * kan overgaan is geen antwoord maar een muur. Dus per reden twee metingen — hij
 * gaat af, en als je het probleem wegneemt accepteert dezelfde order alsnog.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBRefusalsInWorldTest,
	"Eclipse.Command.StageB.NewRefusalsSpeakAndCanBeResolved",
	EclipseStageBWorld::TestFlags)

bool FEclipseStageBRefusalsInWorldTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipseStageBWorld;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	const TArray<AEclipseSquadmateController*> Mates = LaunchAndCollectSquad(*this, Harness);
	UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>();
	UEclipseEventBusSubsystem* Bus = Harness.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestTrue(TEXT("weigeringen: er is een squad"), Mates.Num() > 0)
		|| !TestNotNull(TEXT("weigeringen: squad-subsystem"), Squad)
		|| !TestNotNull(TEXT("weigeringen: event bus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}
	const TArray<FGuid> Alive = Squad->GetAliveSquadmateIds();
	if (!TestTrue(TEXT("weigeringen: er staat iemand overeind"), Alive.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}
	AEclipseSquadmateController* Mate = Mates[0];
	AEclipseCharacter* MateBody = Cast<AEclipseCharacter>(Mate->GetPawn());
	const FGuid MateId = Alive[0];

	auto IssueAndCatch = [this, &Harness, Bus, Squad, MateId](EEclipseSquadOrder Order, AActor* Target, const FVector& At) -> FOrderWatch
	{
		FOrderWatch Watch;
		FEclipseEventSubscriptionHandle Handle = EclipseStageBWorld::Watch(*Bus, Watch);
		Squad->IssueOrder(MateId, Order, At, Target);
		Harness.Idle(0.1f);
		Bus->Unsubscribe(Handle);
		return Watch;
	};

	// ---- NoBreachPoint ----------------------------------------------------
	// De graybox heeft (nog) geen geauthorde deurframes, dus dit is precies de
	// 14.3.5-degradatie: content-afwezigheid weigert netjes en zegt waarom.
	{
		const FOrderWatch Watch = IssueAndCatch(EEclipseSquadOrder::Breach, nullptr, Harness.Location());
		TestEqual(TEXT("NoBreachPoint: precies één antwoord"), Watch.Answers(), 1);
		TestEqual(TEXT("NoBreachPoint: het is een weigering"), Watch.Refused, 1);
		if (Watch.Reasons.Num() == 1)
		{
			AddInfo(FString::Printf(TEXT("GEMETEN  breach zonder punt -> %s : \"%s\""),
				*Watch.Reasons[0].ToString(), Watch.Barks.Num() > 0 ? *Watch.Barks[0] : TEXT("")));
			TestTrue(TEXT("NoBreachPoint: en de reden is NoBreachPoint"),
				Watch.Reasons[0].ToString().Contains(TEXT("NoBreachPoint")));
		}
		TestTrue(TEXT("NoBreachPoint: hij zegt het hardop"), Watch.Barks.Num() == 1 && !Watch.Barks[0].IsEmpty());
	}

	// ... en hij is OP TE LOSSEN: zet er een breekpunt neer en dezelfde order gaat door.
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector DoorSpot = MateBody != nullptr
			? MateBody->GetActorLocation() + MateBody->GetActorForwardVector() * 400.0f
			: Harness.Location();
		AEclipseBreachPoint* Point = Harness.World->SpawnActor<AEclipseBreachPoint>(
			AEclipseBreachPoint::StaticClass(), DoorSpot, FRotator::ZeroRotator, Params);
		TestNotNull(TEXT("NoBreachPoint: er staat nu een breekpunt"), Point);

		const FOrderWatch Watch = IssueAndCatch(EEclipseSquadOrder::Breach, nullptr, DoorSpot);
		AddInfo(FString::Printf(TEXT("GEMETEN  breach MET punt -> %d ack / %d refused"),
			Watch.Acknowledged, Watch.Refused));
		TestEqual(TEXT("NoBreachPoint: met een punt erbij is er nog steeds precies één antwoord"),
			Watch.Answers(), 1);
		TestEqual(TEXT("NoBreachPoint: en die weigering is geen muur — met een punt gaat hij door"),
			Watch.Acknowledged, 1);
		if (Point != nullptr)
		{
			Point->Destroy();
		}
	}

	// ---- NoTargetsMarked --------------------------------------------------
	{
		Squad->ClearSyncStrikeMarks();
		const FOrderWatch Watch = IssueAndCatch(EEclipseSquadOrder::SyncStrike, nullptr, Harness.Location());
		TestEqual(TEXT("NoTargetsMarked: precies één antwoord"), Watch.Answers(), 1);
		TestEqual(TEXT("NoTargetsMarked: het is een weigering"), Watch.Refused, 1);
		if (Watch.Reasons.Num() == 1)
		{
			AddInfo(FString::Printf(TEXT("GEMETEN  sync strike zonder markering -> %s : \"%s\""),
				*Watch.Reasons[0].ToString(), Watch.Barks.Num() > 0 ? *Watch.Barks[0] : TEXT("")));
			TestTrue(TEXT("NoTargetsMarked: en de reden is NoTargetsMarked"),
				Watch.Reasons[0].ToString().Contains(TEXT("NoTargetsMarked")));
		}
		TestTrue(TEXT("NoTargetsMarked: hij zegt het hardop"), Watch.Barks.Num() == 1 && !Watch.Barks[0].IsEmpty());
	}

	// ---- NotConcealed -----------------------------------------------------
	// Een stil doelwit om te markeren, plus een WAKER die onze soldaat ziet. De
	// waker is een echte vijandcontroller met het standaardarchetype (2500 cm
	// waarneming), dus zijn denkbeurt zet HasSeenPlayerSide echt aan.
	{
		AEclipseCharacter* Quarry = SpawnUnawareHostile(*Harness.World,
			Harness.Location() + Harness.Body->GetActorForwardVector() * 800.0f);
		bool bMarked = false;
		const bool bToggled = Squad->ToggleSyncStrikeMark(Quarry, bMarked);
		TestTrue(TEXT("NotConcealed: het stille doelwit is te markeren"), bToggled && bMarked);

		// DE WAKER. Zelfde opzet als AEclipseGameMode zijn vijanden spawnt:
		// lichaam, dan een losse controller, dan Possess.
		AEclipseCharacter* WatcherBody = SpawnUnawareHostile(*Harness.World,
			(MateBody != nullptr ? MateBody->GetActorLocation() : Harness.Location()) + FVector(300.0f, 0.0f, 0.0f));
		AEclipseEnemyController* Watcher = Harness.World->SpawnActor<AEclipseEnemyController>();
		if (Watcher != nullptr && WatcherBody != nullptr)
		{
			Watcher->Possess(WatcherBody);

			// GEZONDHEID PAS NA HET BEZETTEN, en dat is geen detail.
			//
			// De eerste ronde zag deze waker ons nooit, terwijl de meting zei:
			// pion aanwezig, 244 cm, zichtlijn ja. De oorzaak stond in
			// AEclipseEnemyController::OnPossess — die roept
			// InitializeHealth(Archetype.Health) aan en zette mijn 100.000 hp
			// terug naar de 60 van het standaardarchetype. De squad staat op
			// Ready en vuurt uit zichzelf (33 schoten per 2 s, 22 schade), dus de
			// waker lag neer vóór zijn eerste denkbeurt op 0,8 s — en een
			// neergeschoten vijand wist zijn eigen denkklok.
			//
			// Hij hoeft niet onsterfelijk te zijn omdat dat mooier meet; hij moet
			// de twee seconden halen waarin hij ons kán opmerken. Dat is de
			// voorwaarde van de meting, niet de meting zelf.
			WatcherBody->InitializeHealth(100000.0f);
		}

		// Wachten TOT hij ons ziet in plaats van een vast aantal seconden: zijn
		// denkbeurt loopt op het vuurinterval van het archetype, en een test die
		// op een gok van twee seconden staat, meet vroeg of laat de klok in plaats
		// van het gedrag.
		const double SeeDeadline = FPlatformTime::Seconds() + 6.0;
		while (FPlatformTime::Seconds() < SeeDeadline && Watcher != nullptr && !Watcher->HasSeenPlayerSide())
		{
			Harness.Idle(0.25f);
		}

		const bool bSeen = MateBody != nullptr && !Squad->IsBodyConcealed(MateBody);

		// De TUSSENSTAPPEN erbij, want "hij ziet ons niet" heeft vier mogelijke
		// oorzaken en zonder deze regels is niet te zien welke het was. Dat kostte
		// de vorige ronde een hele iteratie.
		AddInfo(FString::Printf(TEXT("GEMETEN  waker: pion=%s, %.0f hp%s, afstand=%.0f cm, zichtlijn=%s, gezien=%s ; soldaat is %s"),
			Watcher != nullptr ? *GetNameSafe(Watcher->GetPawn()) : TEXT("(geen controller)"),
			WatcherBody != nullptr ? WatcherBody->GetHealth() : -1.0f,
			WatcherBody != nullptr && WatcherBody->IsDowned() ? TEXT(" (NEER)") : TEXT(""),
			(Watcher != nullptr && Watcher->GetPawn() != nullptr && MateBody != nullptr)
				? FVector::Dist(Watcher->GetPawn()->GetActorLocation(), MateBody->GetActorLocation()) : -1.0f,
			(Watcher != nullptr && MateBody != nullptr && Watcher->LineOfSightTo(MateBody)) ? TEXT("ja") : TEXT("nee"),
			Watcher != nullptr && Watcher->HasSeenPlayerSide() ? TEXT("ja") : TEXT("nee"),
			bSeen ? TEXT("GEZIEN") : TEXT("ongezien")));

		if (TestTrue(TEXT("NotConcealed: de waker ziet onze soldaat echt (anders meet de rest niets)"), bSeen))
		{
			// Het doelwit zelf blijft onwetend — anders zou de markering wegvallen
			// en zouden we NoTargetsMarked meten in plaats van NotConcealed.
			TestTrue(TEXT("NotConcealed: het doelwit staat er nog"), Squad->GetSyncStrikeMarkCount() > 0);

			const FOrderWatch Watch = IssueAndCatch(EEclipseSquadOrder::SyncStrike, nullptr, Harness.Location());
			TestEqual(TEXT("NotConcealed: precies één antwoord"), Watch.Answers(), 1);
			if (Watch.Reasons.Num() == 1)
			{
				AddInfo(FString::Printf(TEXT("GEMETEN  sync strike terwijl hij gezien wordt -> %s : \"%s\""),
					*Watch.Reasons[0].ToString(), Watch.Barks.Num() > 0 ? *Watch.Barks[0] : TEXT("")));
				TestTrue(TEXT("NotConcealed: en de reden is NotConcealed"),
					Watch.Reasons[0].ToString().Contains(TEXT("NotConcealed")));
				TestTrue(TEXT("NotConcealed: hij zegt het hardop"),
					Watch.Barks.Num() == 1 && !Watch.Barks[0].IsEmpty());
			}

			// OPLOSBAAR: haal de waker weg en dezelfde order slaat wél toe — en
			// het doelwit gaat er ook echt aan. Zonder deze helft zou "hij weigert
			// altijd" ook groen zijn.
			if (Watcher != nullptr)
			{
				Watcher->Destroy();
			}
			if (WatcherBody != nullptr)
			{
				WatcherBody->Destroy();
			}
			Harness.Idle(0.3f);

			const float HealthBefore = Quarry != nullptr ? Quarry->GetHealth() : 0.0f;
			const FOrderWatch Resolved = IssueAndCatch(EEclipseSquadOrder::SyncStrike, nullptr, Harness.Location());
			const float HealthAfter = Quarry != nullptr ? Quarry->GetHealth() : 0.0f;
			AddInfo(FString::Printf(TEXT("GEMETEN  na het wegvallen van de waker: %d ack / %d refused, doelwit %.0f -> %.0f hp"),
				Resolved.Acknowledged, Resolved.Refused, HealthBefore, HealthAfter));
			TestEqual(TEXT("NotConcealed: ongezien slaat de strike wel toe"), Resolved.Acknowledged, 1);
			TestTrue(TEXT("NotConcealed: en het doelwit gaat er echt aan (niet alleen een ack)"),
				Mate->GetSyncStrikeKills() > 0);
		}
	}

	Harness.Shutdown();
	return true;
}

/**
 * FALSIFICATIE 3 (niet-onderhandelbaar) — de stealth-stance verandert AANTOONBAAR
 * gedrag, en het effect wordt gemeten, niet de instelling.
 *
 * Vier metingen in dezelfde wereld, met dezelfde vijand, in dezelfde twee
 * seconden per arm:
 *   Ready              -> vuurt      (de basis)
 *   Stealth, stil      -> vuurt NIET (het kader knijpt de basis af)
 *   Stealth, alarm     -> vuurt      (de tweede poort gaat open)
 *   Recon,   alarm     -> vuurt NIET (het VERSCHIL met stealth, in schoten)
 *
 * Die laatste regel is de belangrijkste. Zonder hem zijn Recon en Stealth twee
 * namen voor hetzelfde gedrag en had de spec net zo goed niets kunnen vragen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBStealthBehaviourTest,
	"Eclipse.Command.StageB.StealthChangesBehaviourNotJustAFlag",
	EclipseStageBWorld::TestFlags)

bool FEclipseStageBStealthBehaviourTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipseStageBWorld;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	const TArray<AEclipseSquadmateController*> Mates = LaunchAndCollectSquad(*this, Harness);
	UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>();
	UEclipseMissionSubsystem* Mission = Harness.GameInstance->GetSubsystem<UEclipseMissionSubsystem>();
	if (!TestTrue(TEXT("stealth: er is een squad"), Mates.Num() > 0)
		|| !TestNotNull(TEXT("stealth: squad-subsystem"), Squad)
		|| !TestNotNull(TEXT("stealth: missie-subsystem"), Mission))
	{
		Harness.Shutdown();
		return false;
	}

	// DE NULMETING DIE ALLES DRAAGT: bij aanvang mag niemand ons gezien hebben.
	// Is dat wel zo, dan meet de "stille" arm hieronder niets — en dat is dan een
	// echte vondst over het district, geen testprobleem.
	const bool bQuietAtStart = !Squad->IsEnemyAwareOfSquad();
	AddInfo(FString::Printf(TEXT("GEMETEN  bij aanvang is de vijand %s van ons op de hoogte"),
		bQuietAtStart ? TEXT("NIET") : TEXT("WEL")));
	if (!TestTrue(TEXT("stealth: het district begint stil (anders is 'ongezien' niet te meten)"), bQuietAtStart))
	{
		Harness.Shutdown();
		return false;
	}

	AEclipseCharacter* Hostile = SpawnUnawareHostile(*Harness.World,
		Harness.Location() + Harness.Body->GetActorForwardVector() * 700.0f);
	if (!TestNotNull(TEXT("stealth: er staat een vijand"), Hostile))
	{
		Harness.Shutdown();
		return false;
	}

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

	// --- arm 1: STEALTH EERST, en dat is geen smaakkwestie -----------------
	//
	// De eerste versie van deze test mat Ready eerst en stealth daarna, en kreeg
	// 19 schoten waar 0 hoorde te staan. De code was niet stuk: het VUREN VAN DE
	// CONTROLEPROEF ZET HET ALARM AAN. `AEclipseGameMode` alarmeert op elk schot
	// dat gehoord wordt (Event.Combat.ShotFired -> NotifyAlarmRaised), dus 33
	// Ready-schoten maakten de wereld luid, en stealth vuurde daarna volkomen
	// terecht — precies wat zijn tweede poort belooft.
	//
	// Dat is de val van "meten ná de gebeurtenis in plaats van tijdens": mijn
	// eigen meetopstelling vernietigde de voorwaarde die de volgende meting nodig
	// had. Stille arm dus eerst, en meteen erna nog een keer vragen of het stil
	// gebleven is — een 0 in een luide wereld zou niets bewijzen.
	const int32 StealthQuietShots = ShotsOverTwoSeconds(EEclipseSquadStance::Stealth);
	const bool bStillQuiet = !Squad->IsEnemyAwareOfSquad();
	Report(*this, TEXT("schoten in 2 s onder STEALTH, ongezien"), static_cast<float>(StealthQuietShots), TEXT(""),
		TEXT("hoort 0 te zijn: hij wacht op je order of op ontdekking"));
	AddInfo(FString::Printf(TEXT("GEMETEN  na de stille arm is de wereld nog steeds %s"),
		bStillQuiet ? TEXT("stil") : TEXT("LUID — de meting telt niet")));

	TestTrue(TEXT("stealth: de wereld bleef stil tijdens de stille arm (anders meet die 0 niets)"), bStillQuiet);
	TestEqual(TEXT("stealth: ongezien vuurt hij niet uit zichzelf"), StealthQuietShots, 0);

	// --- arm 2: de controleproef ------------------------------------------
	// Zonder deze arm is de 0 hierboven waardeloos: een squad die helemaal niet
	// kán vuren geeft ook 0. Ready moet in dezelfde opstelling, op dezelfde
	// vijand, in dezelfde twee seconden wél schieten.
	const int32 ReadyShots = ShotsOverTwoSeconds(EEclipseSquadStance::Ready);
	Report(*this, TEXT("schoten in 2 s onder READY"), static_cast<float>(ReadyShots), TEXT(""),
		TEXT("autonoom vuren is de basis — de controleproef"));
	TestTrue(TEXT("stealth: de controleproef staat — onder Ready wordt er echt gevuurd"), ReadyShots > 0);

	// --- arm 3: de tweede poort. De wereld is nu luid ----------------------
	// En hij is luid geworden UIT DE FICTIE: de squad heeft zichzelf verraden met
	// het vuur van de controleproef. Dat is de eerlijkste versie van deze
	// voorwaarde die er is; de latch erna is alleen een vangnet als de vijanden
	// buiten gehoorsafstand stonden.
	const bool bAlarmFromOwnGunfire = Squad->IsEnemyAwareOfSquad();
	AddInfo(FString::Printf(TEXT("GEMETEN  het alarm kwam %s"),
		bAlarmFromOwnGunfire ? TEXT("uit het eigen vuur van de squad") : TEXT("er niet vanzelf — latch gezet door de test")));
	if (!bAlarmFromOwnGunfire)
	{
		Mission->NotifyAlarmRaised();
	}
	TestTrue(TEXT("stealth: de vijand is nu van ons op de hoogte"), Squad->IsEnemyAwareOfSquad());
	const int32 StealthLoudShots = ShotsOverTwoSeconds(EEclipseSquadStance::Stealth);
	Report(*this, TEXT("schoten in 2 s onder STEALTH, na het alarm"), static_cast<float>(StealthLoudShots), TEXT(""),
		TEXT("hoort > 0 te zijn: zwijgen koopt niets meer"));
	TestTrue(TEXT("stealth: zodra ze ons doorhebben, vuurt hij wel"), StealthLoudShots > 0);

	// --- arm 4: HET VERSCHIL met Recon, in schoten -------------------------
	const int32 ReconLoudShots = ShotsOverTwoSeconds(EEclipseSquadStance::Recon);
	Report(*this, TEXT("schoten in 2 s onder RECON, na het alarm"), static_cast<float>(ReconLoudShots), TEXT(""),
		TEXT("hoort 0 te blijven: recon wacht tot er op HEM geschoten wordt"));
	TestEqual(TEXT("stealth: recon blijft na het alarm nog steeds stil"), ReconLoudShots, 0);
	TestTrue(TEXT("stealth: en dat is een ECHT verschil met stealth, niet twee namen voor één gedrag"),
		StealthLoudShots > ReconLoudShots);

	Harness.Shutdown();
	return true;
}

/**
 * De drie verbs die een EFFECT beloven, gemeten op dat effect.
 *
 * Suppress: rondes in het gebied, met een einde.
 * Flank:    NIEMAND beweegt voor jij goedkeurt — dat is de hele belofte van
 *           "squad computes route; player approves" (8.4).
 * Breach:   pas naar binnen als iedereen staat.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBVerbsDoSomethingTest,
	"Eclipse.Command.StageB.SuppressFlankAndBreachHaveRealEffects",
	EclipseStageBWorld::TestFlags)

bool FEclipseStageBVerbsDoSomethingTest::RunTest(const FString& Parameters)
{
	using namespace EclipseFeelHarness;
	using namespace EclipseStageBWorld;

	FHarness::FOptions Options;
	Options.bRealGameMode = true;
	Options.StepSeconds = 1.0f / 60.0f;

	FHarness Harness;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	const TArray<AEclipseSquadmateController*> Mates = LaunchAndCollectSquad(*this, Harness);
	UEclipseSquadSubsystem* Squad = Harness.World->GetSubsystem<UEclipseSquadSubsystem>();
	UEclipseEventBusSubsystem* Bus = Harness.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestTrue(TEXT("verbs: er is een squad"), Mates.Num() > 0)
		|| !TestNotNull(TEXT("verbs: squad-subsystem"), Squad)
		|| !TestNotNull(TEXT("verbs: event bus"), Bus))
	{
		Harness.Shutdown();
		return false;
	}
	const TArray<FGuid> Alive = Squad->GetAliveSquadmateIds();
	if (!TestTrue(TEXT("verbs: er staat iemand overeind"), Alive.Num() > 0))
	{
		Harness.Shutdown();
		return false;
	}
	AEclipseSquadmateController* Mate = Mates[0];
	AEclipseCharacter* MateBody = Cast<AEclipseCharacter>(Mate->GetPawn());
	const FGuid MateId = Alive[0];

	// ---- SUPPRESS: rondes in het gebied, en daarna stil ------------------
	{
		// Recht vooruit, vlak boven de grond: zicht op het gebied is het feit dat
		// deze order accepteert, dus dat moet er zijn om iets te kunnen meten.
		const FVector Area = MateBody != nullptr
			? MateBody->GetActorLocation() + MateBody->GetActorForwardVector() * 600.0f
			: Harness.Location();
		const int32 Before = Mate->GetSuppressShots();
		Squad->IssueOrder(MateId, EEclipseSquadOrder::Suppress, Area, nullptr);
		Harness.Idle(1.5f);
		const int32 During = Mate->GetSuppressShots() - Before;

		Report(*this, TEXT("onderdrukkingsschoten in 1,5 s"), static_cast<float>(During), TEXT(""),
			TEXT("hoort > 0 te zijn: een order die niets doet is geen order"));
		TestTrue(TEXT("suppress: er gaan echt rondes het gebied in"), During > 0);

		// EN HIJ HOUDT OOK OP. Zonder einde is het geen onderdrukking maar een
		// soldaat die de rest van de missie op een muur staat te schieten.
		const UEclipseCommandModeTuningAsset* Tuning = Squad->ResolveCommandTuning();
		const float Burst = Tuning != nullptr ? Tuning->SuppressBurstSeconds : 5.0f;
		Harness.Idle(Burst + 0.5f);
		const int32 AtRest = Mate->GetSuppressShots();
		Harness.Idle(1.0f);
		Report(*this, TEXT("onderdrukkingsschoten ná het salvo"),
			static_cast<float>(Mate->GetSuppressShots() - AtRest), TEXT(""), TEXT("hoort 0 te zijn"));
		TestEqual(TEXT("suppress: het salvo houdt op"), Mate->GetSuppressShots() - AtRest, 0);
	}

	// ---- FLANK: niemand beweegt voor jij goedkeurt -----------------------
	{
		AEclipseCharacter* Hostile = SpawnUnawareHostile(*Harness.World,
			Harness.Location() + Harness.Body->GetActorForwardVector() * 1200.0f);
		Harness.Idle(0.2f);

		FOrderWatch Watch;
		FEclipseEventSubscriptionHandle Handle = EclipseStageBWorld::Watch(*Bus, Watch);

		const int32 MovesBefore = Mate->GetFlankMoves();
		Squad->IssueOrder(MateId, EEclipseSquadOrder::Flank, Hostile->GetActorLocation(), Hostile);
		Harness.Idle(0.2f);

		const bool bProposed = Mate->GetFlankState() == EclipseSquadOrderLogic::EEclipseFlankState::Proposed;
		AddInfo(FString::Printf(TEXT("GEMETEN  na het eerste flank-commando: staat = %s, verplaatsingen = %d"),
			Mate->GetFlankStateLabel(), Mate->GetFlankMoves() - MovesBefore));

		if (TestTrue(TEXT("flank: het eerste commando STELT VOOR"), bProposed))
		{
			// DE BELOFTE VAN 8.4: "squad computes route; player approves". Zolang
			// jij niet hebt goedgekeurd, beweegt er niemand.
			TestEqual(TEXT("flank: er beweegt niemand voor de goedkeuring"),
				Mate->GetFlankMoves() - MovesBefore, 0);
			TestTrue(TEXT("flank: en het voorstel is een FEIT op de bus"),
				Watch.QueuedTransitions.Contains(FName(TEXT("Flank.Proposed"))));

			// Tweede commando = goedkeuring, en dan pas beweegt hij.
			Squad->IssueOrder(MateId, EEclipseSquadOrder::Flank, Hostile->GetActorLocation(), Hostile);
			Harness.Idle(0.2f);
			AddInfo(FString::Printf(TEXT("GEMETEN  na de goedkeuring: staat = %s, verplaatsingen = %d"),
				Mate->GetFlankStateLabel(), Mate->GetFlankMoves() - MovesBefore));
			TestTrue(TEXT("flank: goedgekeurd"),
				Mate->GetFlankState() == EclipseSquadOrderLogic::EEclipseFlankState::Approved);
			TestEqual(TEXT("flank: en NU beweegt hij"), Mate->GetFlankMoves() - MovesBefore, 1);
			TestTrue(TEXT("flank: de goedkeuring is een feit op dezelfde stroom"),
				Watch.QueuedTransitions.Contains(FName(TEXT("Flank.Approved"))));
		}
		Bus->Unsubscribe(Handle);

		// En het venster verloopt ook als je NIETS doet — met een gesproken regel,
		// want nietsdoen is geen weigering maar wel een verandering (9.5).
		const UEclipseCommandModeTuningAsset* Tuning = Squad->ResolveCommandTuning();
		const float Window = Tuning != nullptr ? Tuning->FlankApprovalTimeoutSeconds : 6.0f;

		FOrderWatch ExpiryWatch;
		FEclipseEventSubscriptionHandle ExpiryHandle = EclipseStageBWorld::Watch(*Bus, ExpiryWatch);
		const int32 MovesBeforeExpiry = Mate->GetFlankMoves();
		Squad->IssueOrder(MateId, EEclipseSquadOrder::Flank, Hostile->GetActorLocation(), Hostile);
		// Het venster loopt op de WANDKLOK (locked decision 2), dus wachten doen we
		// ook echt in wandkloktijd — het harnas tikt de wereld ondertussen door.
		const double Deadline = FPlatformTime::Seconds() + Window + 1.0;
		while (FPlatformTime::Seconds() < Deadline
			&& Mate->GetFlankState() == EclipseSquadOrderLogic::EEclipseFlankState::Proposed)
		{
			Harness.Idle(0.25f);
		}
		Bus->Unsubscribe(ExpiryHandle);

		AddInfo(FString::Printf(TEXT("GEMETEN  na %.1f s niets doen: staat = %s"), Window, Mate->GetFlankStateLabel()));
		TestTrue(TEXT("flank: een venster waar je niets mee doet, verloopt"),
			Mate->GetFlankState() == EclipseSquadOrderLogic::EEclipseFlankState::Expired);
		TestEqual(TEXT("flank: en dan is er ook niemand vertrokken"),
			Mate->GetFlankMoves() - MovesBeforeExpiry, 0);
		TestTrue(TEXT("flank: het verlopen is hoorbaar (nietsdoen is geen weigering, wel een feit)"),
			ExpiryWatch.QueuedTransitions.Contains(FName(TEXT("Flank.Expired"))));
	}

	// ---- BREACH: pas naar binnen als iedereen staat -----------------------
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector DoorSpot = Harness.Location() + Harness.Body->GetActorForwardVector() * 500.0f;
		AEclipseBreachPoint* Point = Harness.World->SpawnActor<AEclipseBreachPoint>(
			AEclipseBreachPoint::StaticClass(), DoorSpot, Harness.Body->GetActorRotation(), Params);
		if (TestNotNull(TEXT("breach: er staat een breekpunt"), Point))
		{
			FOrderWatch Watch;
			FEclipseEventSubscriptionHandle Handle = EclipseStageBWorld::Watch(*Bus, Watch);

			Squad->IssueOrderToAll(EEclipseSquadOrder::Breach, DoorSpot, nullptr);
			Harness.Idle(6.0f);
			Bus->Unsubscribe(Handle);

			int32 Stacked = 0;
			int32 Entries = 0;
			int32 LastStackedIndex = INDEX_NONE;
			int32 FirstEntryIndex = INDEX_NONE;
			for (int32 Index = 0; Index < Watch.QueuedTransitions.Num(); ++Index)
			{
				if (Watch.QueuedTransitions[Index] == FName(TEXT("Breach.Stacked")))
				{
					++Stacked;
					LastStackedIndex = Index;
				}
				else if (Watch.QueuedTransitions[Index] == FName(TEXT("Breach.Entry")))
				{
					++Entries;
					if (FirstEntryIndex == INDEX_NONE)
					{
						FirstEntryIndex = Index;
					}
				}
			}

			int32 EntryMoves = 0;
			for (const AEclipseSquadmateController* Each : Mates)
			{
				EntryMoves += Each->GetBreachEntries();
			}
			Report(*this, TEXT("soldaten gestapeld"), static_cast<float>(Stacked), TEXT(""));
			Report(*this, TEXT("gezamenlijke entries"), static_cast<float>(Entries), TEXT(""));
			Report(*this, TEXT("entry-verplaatsingen"), static_cast<float>(EntryMoves), TEXT(""));

			TestTrue(TEXT("breach: er stapelt echt iemand op het punt"), Stacked > 0);
			TestTrue(TEXT("breach: en er gaat ook echt iemand naar binnen"), Entries > 0);

			// DE SYNCHRONISATIE, en dit is het enige wat "synchronized entry" op
			// sliceschaal kan betekenen: geen enkele entry vóór de laatste die
			// aankwam. Gaat er iemand alvast naar binnen, dan valt dit om.
			if (FirstEntryIndex != INDEX_NONE && LastStackedIndex != INDEX_NONE)
			{
				TestTrue(TEXT("breach: niemand gaat naar binnen voordat de laatste staat"),
					FirstEntryIndex > LastStackedIndex);
			}
		}
	}

	Harness.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
