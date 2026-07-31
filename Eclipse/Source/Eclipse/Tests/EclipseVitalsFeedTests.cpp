// De datalaag ONDER de schermlaag: Event.Player.VitalsChanged (bouwvolgorde GDD
// 14.5 — schema, pure logica + tests, bedrading + events; pas daarna iets
// zichtbaars).
//
// Deze suite is opgezet om de feed te FALSIFIEREN, niet om hem te bevestigen.
// De vier vragen die hij moet overleven:
//
//   1. Zwijgt hij als er niets verandert?      (anders is hij pollen met extra stappen)
//   2. Zendt hij bij een echte verandering PRECIES één feit, met de juiste velden?
//   3. Houdt hij een reeks houdingen op volgorde uit elkaar?
//   4. Komt het feit ook werkelijk AAN — op de echte bus, uit het echte lichaam?
//
// Vraag 4 staat er omdat 1 tot en met 3 alle drie groen kunnen zijn terwijl er in
// het spel niets gebeurt: pure logica die niemand aanroept is perfecte logica.
// Daarom draait de laatste test op het feel-harnas — echte wereld, echte
// AEclipseCharacter, echte AEclipsePlayerController, echte
// UEclipseEventBusSubsystem — en telt hij wat er op de bus landt.
//
// En bij elke "0" hoort de vraag: kapot of nooit geprobeerd? Elke nul-meting
// hieronder staat daarom naast een CONTROLEPROEF die bewijst dat er op diezelfde
// opstelling wél iets kán aankomen.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseVitalsFeed.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Tests/EclipseFeelHarness.h"

namespace EclipseVitalsFeedTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** Een gezond, staand lichaam op 100/100 — het vertrekpunt van elke meting. */
	EclipseVitalsFeed::FEclipseVitalsSnapshot Healthy()
	{
		EclipseVitalsFeed::FEclipseVitalsSnapshot Snapshot;
		Snapshot.Health = 100.0f;
		Snapshot.MaxHealth = 100.0f;
		return Snapshot;
	}

	const TCHAR* StanceName(EEclipseStance Stance)
	{
		switch (Stance)
		{
		case EEclipseStance::Crouched:  return TEXT("Crouched");
		case EEclipseStance::Sprinting: return TEXT("Sprinting");
		case EEclipseStance::InCover:   return TEXT("InCover");
		default:                        return TEXT("Standing");
		}
	}
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 1 — 60 tikken zonder verandering = 0 uitzendingen.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVitalsFeedSilentWhenUnchangedTest,
	"Eclipse.Characters.VitalsFeed.SilentWhenUnchanged",
	EclipseVitalsFeedTest::TestFlags)

bool FEclipseVitalsFeedSilentWhenUnchangedTest::RunTest(const FString& Parameters)
{
	EclipseVitalsFeed::FEclipseVitalsTracker Tracker;
	const EclipseVitalsFeed::FEclipseVitalsSnapshot Snapshot = EclipseVitalsFeedTest::Healthy();

	// CONTROLEPROEF EERST. Zonder deze regel zou "0 uitzendingen" ook waar zijn
	// voor een tracker die helemaal niets doet, en die twee verklaringen kan een
	// teller van 0 niet scheiden. Het eerste monster MOET vertrekken: de HUD heeft
	// een beginwaarde nodig, anders staat de balk bij spawn op nul.
	const EclipseVitalsFeed::FEclipseVitalsDecision First = Tracker.Submit(Snapshot);
	TestTrue(TEXT("Controleproef: het EERSTE monster gaat wel degelijk de deur uit"), First.bShouldBroadcast);
	TestTrue(TEXT("Het eerste feit is gemarkeerd als beginwaarde, niet als verandering"), First.Payload.bInitial);
	TestFalse(TEXT("Een beginwaarde is geen schade — bHealthChanged staat uit"), First.Payload.bHealthChanged);
	TestFalse(TEXT("Een beginwaarde is geen houdingswissel"), First.Payload.bStanceChanged);
	TestEqual(TEXT("Na de beginwaarde: 1 uitzending"), Tracker.GetBroadcastCount(), 1);

	// En dan zestig tikken lang exact hetzelfde lichaam aanbieden.
	for (int32 Tick = 0; Tick < 60; ++Tick)
	{
		const EclipseVitalsFeed::FEclipseVitalsDecision Decision = Tracker.Submit(Snapshot);
		if (Decision.bShouldBroadcast)
		{
			AddError(FString::Printf(TEXT("Tik %d zond een feit uit terwijl er niets veranderde — de feed spamt de bus."), Tick));
			break;
		}
	}

	TestEqual(TEXT("60 tikken zonder verandering voegen NIETS toe aan de teller"), Tracker.GetBroadcastCount(), 1);

	// Tweede controleproef, ná de stilte: de tracker is niet dichtgeslagen maar
	// gewoon stil. Een feed die na 60 tikken permanent zwijgt zou test 1 ook halen.
	EclipseVitalsFeed::FEclipseVitalsSnapshot Hurt = Snapshot;
	Hurt.Health = 90.0f;
	TestTrue(TEXT("Controleproef: na de stilte komt een echte verandering er nog steeds door"), Tracker.Submit(Hurt).bShouldBroadcast);
	TestEqual(TEXT("En dan pas staat de teller op 2"), Tracker.GetBroadcastCount(), 2);

	return true;
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 2 — 100 -> 60 hp = precies één uitzending, met de juiste velden.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVitalsFeedOneFactPerDamageTest,
	"Eclipse.Characters.VitalsFeed.OneFactPerDamage",
	EclipseVitalsFeedTest::TestFlags)

bool FEclipseVitalsFeedOneFactPerDamageTest::RunTest(const FString& Parameters)
{
	EclipseVitalsFeed::FEclipseVitalsTracker Tracker;
	Tracker.Submit(EclipseVitalsFeedTest::Healthy()); // beginwaarde, telt niet mee als schade

	EclipseVitalsFeed::FEclipseVitalsSnapshot Hurt = EclipseVitalsFeedTest::Healthy();
	Hurt.Health = 60.0f;

	const EclipseVitalsFeed::FEclipseVitalsDecision Decision = Tracker.Submit(Hurt);
	TestTrue(TEXT("40 hp schade levert een feit op"), Decision.bShouldBroadcast);
	TestEqual(TEXT("Precies één uitzending erbij"), Tracker.GetBroadcastCount(), 2);

	// De VELDEN, want een feit dat aankomt maar niets zegt is geen datalaag.
	TestEqual(TEXT("Health = de waarde NA de klap"), Decision.Payload.Health, 60.0f);
	TestEqual(TEXT("MaxHealth reist mee — een balk heeft een lengte nodig"), Decision.Payload.MaxHealth, 100.0f);
	TestEqual(TEXT("PreviousHealth = de waarde ERVOOR; het teken van dat verschil is schade-of-genezing"), Decision.Payload.PreviousHealth, 100.0f);
	TestTrue(TEXT("bHealthChanged staat aan"), Decision.Payload.bHealthChanged);
	TestFalse(TEXT("bStanceChanged staat UIT — anders flitst de HUD een houdingswissel bij elke treffer"), Decision.Payload.bStanceChanged);
	TestFalse(TEXT("bDownedChanged staat uit: 60 hp is geraakt, niet neer"), Decision.Payload.bDownedChanged);
	TestFalse(TEXT("bInitial staat uit: dit is een verandering, geen beginwaarde"), Decision.Payload.bInitial);
	TestFalse(TEXT("bDowned staat uit"), Decision.Payload.bDowned);
	TestEqual(TEXT("De houding is ongemoeid gebleven"), static_cast<int32>(Decision.Payload.Stance), static_cast<int32>(EEclipseStance::Standing));

	// Datzelfde lichaam nog eens aanbieden voegt niets toe: de klap is één feit,
	// geen toestand die zichzelf blijft herhalen.
	TestFalse(TEXT("Hetzelfde gewonde lichaam nogmaals aanbieden zendt niets"), Tracker.Submit(Hurt).bShouldBroadcast);
	TestEqual(TEXT("De teller staat nog steeds op 2"), Tracker.GetBroadcastCount(), 2);

	// Neergaan is een APART feit, ook als de gezondheid in dezelfde stap naar 0 gaat.
	EclipseVitalsFeed::FEclipseVitalsSnapshot Down = Hurt;
	Down.Health = 0.0f;
	Down.bDowned = true;
	const EclipseVitalsFeed::FEclipseVitalsDecision DownDecision = Tracker.Submit(Down);
	TestTrue(TEXT("Neergaan levert een feit op"), DownDecision.bShouldBroadcast);
	TestTrue(TEXT("bDowned staat aan"), DownDecision.Payload.bDowned);
	TestTrue(TEXT("bDownedChanged zegt dat JUIST DAT veranderde"), DownDecision.Payload.bDownedChanged);
	TestTrue(TEXT("De laatste gezondheid reist mee in hetzelfde feit"), DownDecision.Payload.bHealthChanged);
	TestEqual(TEXT("PreviousHealth = 60, de waarde waar hij vandaan viel"), DownDecision.Payload.PreviousHealth, 60.0f);

	return true;
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 3 — hurken -> staan -> sprint = 3 uitzendingen, in die volgorde.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVitalsFeedStanceSequenceTest,
	"Eclipse.Characters.VitalsFeed.StanceSequence",
	EclipseVitalsFeedTest::TestFlags)

bool FEclipseVitalsFeedStanceSequenceTest::RunTest(const FString& Parameters)
{
	EclipseVitalsFeed::FEclipseVitalsTracker Tracker;
	Tracker.Submit(EclipseVitalsFeedTest::Healthy()); // staand; de beginwaarde
	const int32 CountAfterSeed = Tracker.GetBroadcastCount();

	TArray<EEclipseStance> Seen;
	TArray<EEclipseStance> SeenPrevious;

	auto Apply = [&Tracker, &Seen, &SeenPrevious](bool bCrouched, bool bSprinting)
	{
		EclipseVitalsFeed::FEclipseVitalsSnapshot Snapshot = EclipseVitalsFeedTest::Healthy();
		Snapshot.bCrouched = bCrouched;
		Snapshot.bSprinting = bSprinting;
		const EclipseVitalsFeed::FEclipseVitalsDecision Decision = Tracker.Submit(Snapshot);
		if (Decision.bShouldBroadcast)
		{
			Seen.Add(Decision.Payload.Stance);
			SeenPrevious.Add(Decision.Payload.PreviousStance);
		}
		return Decision;
	};

	const EclipseVitalsFeed::FEclipseVitalsDecision Crouch = Apply(/*bCrouched*/ true, /*bSprinting*/ false);
	const EclipseVitalsFeed::FEclipseVitalsDecision Stand = Apply(false, false);
	const EclipseVitalsFeed::FEclipseVitalsDecision Sprint = Apply(false, true);

	TestEqual(TEXT("Drie houdingswissels leveren precies drie uitzendingen op"), Tracker.GetBroadcastCount() - CountAfterSeed, 3);

	if (Seen.Num() != 3)
	{
		AddError(FString::Printf(TEXT("Verwacht 3 houdingsfeiten, gekregen %d."), Seen.Num()));
		return false;
	}

	// De gemeten reeks als tekst in het log, niet alleen pass/fail: bij rood wil je
	// zien WELKE volgorde eruit kwam, anders begint het zoeken pas na de test.
	FString Sequence;
	for (int32 Index = 0; Index < Seen.Num(); ++Index)
	{
		Sequence += FString::Printf(TEXT("%s%s->%s"), Index > 0 ? TEXT(", ") : TEXT(""),
			EclipseVitalsFeedTest::StanceName(SeenPrevious[Index]), EclipseVitalsFeedTest::StanceName(Seen[Index]));
	}
	AddInfo(FString::Printf(TEXT("Gemeten houdingsreeks: %s"), *Sequence));

	// DE VOLGORDE, niet alleen het aantal: drie feiten in de verkeerde volgorde
	// zouden dezelfde teller opleveren en een HUD laten zien die achterloopt.
	TestEqual(TEXT("Feit 1 = HURKEN"), static_cast<int32>(Seen[0]), static_cast<int32>(EEclipseStance::Crouched));
	TestEqual(TEXT("Feit 2 = STAAND"), static_cast<int32>(Seen[1]), static_cast<int32>(EEclipseStance::Standing));
	TestEqual(TEXT("Feit 3 = SPRINT"), static_cast<int32>(Seen[2]), static_cast<int32>(EEclipseStance::Sprinting));

	// De ketting van vorige-houdingen moet aansluiten, anders is een overgang niet
	// leesbaar en kan de HUD geen "van X naar Y" tonen.
	TestEqual(TEXT("Feit 1 komt uit STAAND"), static_cast<int32>(SeenPrevious[0]), static_cast<int32>(EEclipseStance::Standing));
	TestEqual(TEXT("Feit 2 komt uit HURKEN"), static_cast<int32>(SeenPrevious[1]), static_cast<int32>(EEclipseStance::Crouched));
	TestEqual(TEXT("Feit 3 komt uit STAAND"), static_cast<int32>(SeenPrevious[2]), static_cast<int32>(EEclipseStance::Standing));

	for (const EclipseVitalsFeed::FEclipseVitalsDecision& Decision : { Crouch, Stand, Sprint })
	{
		TestTrue(TEXT("Elk houdingsfeit is als houdingswissel gemarkeerd"), Decision.Payload.bStanceChanged);
		TestFalse(TEXT("Een houdingswissel is GEEN gezondheidsverandering"), Decision.Payload.bHealthChanged);
	}

	// De voorrangsregel, expliciet: gehurkt met een vastgezette sprint-latch is
	// geen sprint — je kruipt, want gehurkt geldt MaxWalkSpeedCrouched. Dat als
	// "SPRINT" tonen zou het scherm laten liegen over hoe snel je bent.
	EclipseVitalsFeed::FEclipseVitalsSnapshot Both = EclipseVitalsFeedTest::Healthy();
	Both.bCrouched = true;
	Both.bSprinting = true;
	TestEqual(TEXT("Hurken + sprint tegelijk leest als HURKEN"),
		static_cast<int32>(EclipseVitalsFeed::ResolveStance(Both)), static_cast<int32>(EEclipseStance::Crouched));

	// Dekking wint van allebei: het tactische feit gaat voor het gevolg voor de knieën.
	Both.bInCover = true;
	TestEqual(TEXT("Dekking wint van hurken en sprint"),
		static_cast<int32>(EclipseVitalsFeed::ResolveStance(Both)), static_cast<int32>(EEclipseStance::InCover));

	return true;
}

// ---------------------------------------------------------------------------
// De drempel mag geen verandering OPETEN — alleen ruis dempen.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVitalsFeedThresholdAccumulatesTest,
	"Eclipse.Characters.VitalsFeed.ThresholdAccumulates",
	EclipseVitalsFeedTest::TestFlags)

bool FEclipseVitalsFeedThresholdAccumulatesTest::RunTest(const FString& Parameters)
{
	// Een drempel is een geldig middel tegen float-ruis en een uitstekend middel om
	// echte schade te laten verdwijnen. Het verschil zit in WAARTEGEN je vergelijkt:
	// tegen het laatst UITGEZONDEN moment stapelt een drup op tot hij zichtbaar is,
	// tegen het laatst GEZIENE moment verdwijnt hij voorgoed. Deze test pint dat vast.
	EclipseVitalsFeed::FEclipseVitalsTracker Tracker;
	EclipseVitalsFeed::FEclipseVitalsSnapshot Snapshot = EclipseVitalsFeedTest::Healthy();
	Tracker.Submit(Snapshot);

	// Vijftig druppels van 0,004 hp: elk ver onder de drempel, samen 0,2 hp.
	constexpr int32 DripCount = 50;
	constexpr float DripSize = 0.004f;
	int32 Broadcasts = 0;
	for (int32 Drip = 0; Drip < DripCount; ++Drip)
	{
		Snapshot.Health -= DripSize;
		if (Tracker.Submit(Snapshot).bShouldBroadcast)
		{
			++Broadcasts;
		}
	}

	// De bovengrens is AFGELEID, niet gegokt: de totale verandering gedeeld door de
	// drempel is het maximale aantal feiten dat er ooit uit kan komen. Een grens uit
	// de duim (de eerste versie zei "< 10") meet niets — die zegt alleen iets over
	// mijn rekenwerk. Verwacht: 0,2 hp / 0,01 = ten hoogste 20 feiten uit 50 monsters.
	const int32 TheoreticalMax = FMath::CeilToInt((DripCount * DripSize) / EclipseVitalsFeed::HealthEpsilon);
	EclipseFeelHarness::Report(*this, TEXT("uitzendingen uit 50 sub-drempel-druppels"), Broadcasts, TEXT("feiten"));
	TestTrue(TEXT("Opgestapelde druppels bereiken het scherm alsnog (niet nul uitzendingen)"), Broadcasts > 0);
	TestTrue(FString::Printf(TEXT("...en nooit meer dan de totale verandering gedeeld door de drempel (%d)"), TheoreticalMax),
		Broadcasts <= TheoreticalMax);
	TestTrue(TEXT("...en fors minder dan één per monster: de ruis is wel degelijk gedempt"), Broadcasts * 2 < DripCount);
	TestTrue(TEXT("En het laatst uitgezonden getal loopt niet meer dan één drempel achter"),
		FMath::Abs(Tracker.GetLastBroadcast().Health - Snapshot.Health) <= EclipseVitalsFeed::HealthEpsilon);

	// Een verschuivend MAXIMUM is ook nieuws: de balk verandert van lengte terwijl
	// het getal blijft staan (ApplyTuning doet precies dat).
	EclipseVitalsFeed::FEclipseVitalsTracker MaxTracker;
	EclipseVitalsFeed::FEclipseVitalsSnapshot Base = EclipseVitalsFeedTest::Healthy();
	MaxTracker.Submit(Base);
	Base.MaxHealth = 140.0f;
	const EclipseVitalsFeed::FEclipseVitalsDecision MaxDecision = MaxTracker.Submit(Base);
	TestTrue(TEXT("Een gewijzigd MAXIMUM is een gezondheidsfeit"), MaxDecision.bShouldBroadcast);
	TestTrue(TEXT("...en is als zodanig gemarkeerd"), MaxDecision.Payload.bHealthChanged);
	TestEqual(TEXT("Het nieuwe maximum reist mee"), MaxDecision.Payload.MaxHealth, 140.0f);

	return true;
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 4 (de belangrijkste) — komt het feit ook AAN, op de echte bus,
// uit het echte lichaam? Pure logica die niemand aanroept is perfecte logica.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVitalsFeedWiredToBusTest,
	"Eclipse.Characters.VitalsFeed.WiredToBus",
	EclipseVitalsFeedTest::TestFlags)

bool FEclipseVitalsFeedWiredToBusTest::RunTest(const FString& Parameters)
{
	EclipseFeelHarness::FHarness Harness;
	if (!Harness.Start(*this))
	{
		Harness.Shutdown();
		return false;
	}

	UEclipseEventBusSubsystem* Bus = Harness.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (!TestNotNull(TEXT("De echte event-bus staat in deze wereld"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	TArray<FEclipsePlayerVitalsPayload> Received;
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		EclipseTags::Event_Player_VitalsChanged,
		FEclipseEventNativeDelegate::CreateLambda([&Received](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			if (const FEclipsePlayerVitalsPayload* Vitals = Payload.GetPtr<FEclipsePlayerVitalsPayload>())
			{
				Received.Add(*Vitals);
			}
		}),
		FEclipsePlayerVitalsPayload::StaticStruct());

	// --- 60 tikken van een ECHT tickende wereld, en niemand raakt iets aan ------
	// Sterker dan de pure variant: hier draaien camera-blends, bewegingsupdates en
	// animatie mee. Als er ergens een per-frame-schrijver zat, ziet hij dat hier.
	Received.Reset();
	Harness.Idle(60.0f * EclipseFeelHarness::FixedStepSeconds);
	TestEqual(TEXT("60 getickte frames zonder gebeurtenis = 0 feiten op de bus"), Received.Num(), 0);

	// --- CONTROLEPROEF: kan er op DEZE opstelling überhaupt iets aankomen? ------
	// Eerst METEN, dan pas asserten. Een "0 feiten" heeft hier vier mogelijke
	// verklaringen — de klap kwam niet aan, de poort liet het lichaam niet door,
	// de tracker vond het geen verandering, of de bus leverde niet af — en een
	// kale teller scheidt die vier niet. Deze vier metingen wél: elke stap in de
	// keten krijgt zijn eigen getal.
	const float MaxHealth = Harness.Body->GetMaxHealth();
	const float HealthBefore = Harness.Body->GetHealth();
	const int32 TrackerBefore = Harness.Body->GetVitalsBroadcastCount();
	const AController* Driver = Harness.Body->GetController();

	Received.Reset();
	Harness.Body->ApplyDamage(MaxHealth * 0.4f, nullptr, FName(TEXT("Test")));

	EclipseFeelHarness::Report(*this, TEXT("MaxHealth op het lichaam"), MaxHealth, TEXT("hp"));
	EclipseFeelHarness::Report(*this, TEXT("gezondheid vóór de klap"), HealthBefore, TEXT("hp"));
	EclipseFeelHarness::Report(*this, TEXT("gezondheid ná de klap"), Harness.Body->GetHealth(), TEXT("hp"));
	AddInfo(FString::Printf(TEXT("GEMETEN  bestuurder: %s · IsPlayerController=%d · APawn::IsPlayerControlled=%d · PlayerState=%d"),
		Driver != nullptr ? *Driver->GetClass()->GetName() : TEXT("<geen>"),
		Driver != nullptr && Driver->IsPlayerController() ? 1 : 0,
		Harness.Body->IsPlayerControlled() ? 1 : 0,
		Driver != nullptr && Driver->PlayerState != nullptr ? 1 : 0));
	EclipseFeelHarness::Report(*this, TEXT("tracker-uitzendingen door de klap"),
		Harness.Body->GetVitalsBroadcastCount() - TrackerBefore, TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("op de BUS aangekomen"), Received.Num(), TEXT("feiten"));

	TestTrue(TEXT("Ketenstap 1: de klap kwam werkelijk aan op het lichaam"), Harness.Body->GetHealth() < HealthBefore);
	TestEqual(TEXT("Ketenstap 2: de feed telde precies één uitzending"),
		Harness.Body->GetVitalsBroadcastCount() - TrackerBefore, 1);
	if (!TestEqual(TEXT("Ketenstap 3: eén klap = precies één feit op de bus"), Received.Num(), 1))
	{
		Bus->Unsubscribe(Handle);
		Harness.Shutdown();
		return false;
	}
	TestTrue(TEXT("Het feit meldt een gezondheidsverandering"), Received[0].bHealthChanged);
	TestFalse(TEXT("...en geen houdingswissel"), Received[0].bStanceChanged);
	TestEqual(TEXT("PreviousHealth = het volle lichaam"), Received[0].PreviousHealth, MaxHealth);
	TestEqual(TEXT("MaxHealth komt uit DA_CharacterTuning, niet uit een testgetal"), Received[0].MaxHealth, MaxHealth);
	TestTrue(TEXT("Health is gedaald"), Received[0].Health < Received[0].PreviousHealth);
	TestFalse(TEXT("Een klap van 40% legt niemand neer"), Received[0].bDowned);

	// --- de echte hurk-route, via het bewegingscomponent ------------------------
	Received.Reset();
	Harness.Body->Crouch();
	Harness.Step();
	TestEqual(TEXT("Hurken = één feit"), Received.Num(), 1);
	if (Received.Num() == 1)
	{
		TestEqual(TEXT("...en de houding erin is HURKEN"), static_cast<int32>(Received[0].Stance), static_cast<int32>(EEclipseStance::Crouched));
		TestTrue(TEXT("...gemarkeerd als houdingswissel"), Received[0].bStanceChanged);
		TestFalse(TEXT("...en niet als schade"), Received[0].bHealthChanged);
	}

	Received.Reset();
	Harness.Body->UnCrouch();
	Harness.Step();
	TestEqual(TEXT("Opstaan = één feit"), Received.Num(), 1);
	if (Received.Num() == 1)
	{
		TestEqual(TEXT("...terug naar STAAND"), static_cast<int32>(Received[0].Stance), static_cast<int32>(EEclipseStance::Standing));
		TestEqual(TEXT("...en het feit weet waar hij vandaan kwam"), static_cast<int32>(Received[0].PreviousStance), static_cast<int32>(EEclipseStance::Crouched));
	}

	// --- sprint loopt via de controller, maar landt op het LICHAAM --------------
	Received.Reset();
	Harness.Body->SetSprinting(true);
	TestEqual(TEXT("Sprint = één feit"), Received.Num(), 1);
	if (Received.Num() == 1)
	{
		TestEqual(TEXT("...met houding SPRINT"), static_cast<int32>(Received[0].Stance), static_cast<int32>(EEclipseStance::Sprinting));
	}
	Received.Reset();
	Harness.Body->SetSprinting(true);
	TestEqual(TEXT("Dezelfde sprint nog eens zetten zendt NIETS"), Received.Num(), 0);
	Harness.Body->SetSprinting(false);

	// --- de poort: een lichaam ZONDER speler hoort hier niets te doen -----------
	// Zonder deze meting zou "de bus is rustig" ook kunnen betekenen "er staan
	// gewoon geen vijanden"; met een echte vijand die echt schade oploopt scheidt
	// hij de twee verklaringen.
	Received.Reset();
	AEclipseCharacter* Hostile = Harness.World->SpawnActor<AEclipseCharacter>(FVector(500.0f, 0.0f, 120.0f), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Vijandelijk lichaam gespawnd"), Hostile))
	{
		Hostile->InitializeHealth(100.0f);
		Hostile->ApplyDamage(50.0f, nullptr, FName(TEXT("Test")));
		TestEqual(TEXT("Een AI-lichaam zet zijn gezondheid NIET op de spelersbus"), Received.Num(), 0);
		TestEqual(TEXT("...en zijn eigen teller staat ook op nul: de poort zit vóór de tracker"), Hostile->GetVitalsBroadcastCount(), 0);
		TestTrue(TEXT("Controleproef: die schade is wél echt aangekomen bij dat lichaam"), Hostile->GetHealth() < 100.0f);
	}

	// --- neergaan is een feit, geen stilte -------------------------------------
	Received.Reset();
	Harness.Body->ApplyDamage(MaxHealth * 2.0f, nullptr, FName(TEXT("Test")));
	TestTrue(TEXT("Neergaan bereikt de HUD"), Received.Num() >= 1);
	if (Received.Num() >= 1)
	{
		const FEclipsePlayerVitalsPayload& Last = Received.Last();
		TestTrue(TEXT("Het laatste feit zegt: neer"), Last.bDowned);
		TestTrue(TEXT("...en markeert dat als de verandering"), Last.bDownedChanged);
		TestEqual(TEXT("De gezondheid staat op nul, niet negatief"), Last.Health, 0.0f);
	}

	Bus->Unsubscribe(Handle);
	Harness.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
