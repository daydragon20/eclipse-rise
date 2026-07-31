// SPEC-P2-02 Stage B — de headless laag onder de vijf nieuwe 8.4-verbs.
//
// Wat hier bewezen wordt, en waarom juist dit:
//
//   1. GEEN ENKELE combinatie van wereldfeiten levert stilte op. Dat is de
//      bestaande 8.4-lat, nu over negen orders in plaats van vier.
//   2. Elke NIEUWE reden gaat ook echt af, en alleen waar hij hoort. Een reden
//      die geen enkele combinatie oplevert is een dood label; een reden die
//      overal opduikt legt niets uit. Beide vallen hier om.
//   3. Het venster van de flank keurt niet goed als het dicht is. Dat is de enige
//      regel die van "een venster" iets anders maakt dan decoratie.
//   4. De verdeling van een sync strike DEKT elke markering precies één keer.
//   5. Stealth is aantoonbaar iets ANDERS dan Recon. Twee doctrines die hetzelfde
//      doen zijn één doctrine met twee namen.
//
// Alles puur: geen wereld, geen actoren, geen tijdvertraging. De in-wereld-kant
// (schoten tellen, weigeringen horen, één emitter) staat in
// EclipseCommandStageBWorldTests.cpp — die kan pas iets bewijzen als dit klopt.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCommandModeComponent.h"
#include "Misc/AutomationTest.h"
#include "Squad/EclipseCommandModeTuning.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Squad/EclipseSquadTypes.h"
#include "UObject/UnrealType.h"

namespace EclipseCommandStageBTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** De vijf verbs die Stage B toevoegt — de vier uit Phase 1 hebben hun eigen suite. */
	const TArray<EEclipseSquadOrder>& NewVerbs()
	{
		static const TArray<EEclipseSquadOrder> Verbs = {
			EEclipseSquadOrder::Suppress,
			EEclipseSquadOrder::Flank,
			EEclipseSquadOrder::Breach,
			EEclipseSquadOrder::UseAbility,
			EEclipseSquadOrder::SyncStrike
		};
		return Verbs;
	}

	FString OrderName(EEclipseSquadOrder Order)
	{
		return UEnum::GetValueAsString(Order).RightChop(FString(TEXT("EEclipseSquadOrder::")).Len());
	}

	FString ReasonName(EEclipseOrderRefusalReason Reason)
	{
		return UEnum::GetValueAsString(Reason).RightChop(FString(TEXT("EEclipseOrderRefusalReason::")).Len());
	}

	/**
	 * Loop ALLE feitencombinaties af. Acht vlaggen plus drie standen van de
	 * markeringenteller (nul, één, vol) — 768 werelden per order.
	 *
	 * De teller doet met drie waarden mee en niet met twee, omdat "vol" en "één"
	 * voor de beslissing hetzelfde horen te zijn: als dat ooit uiteen gaat lopen,
	 * is dat een cap die op de verkeerde plek is gaan meebeslissen.
	 */
	void ForEachFactCombination(TFunctionRef<void(const EclipseSquadOrderLogic::FEclipseOrderWorldFacts&)> Visit)
	{
		for (int32 Bits = 0; Bits < 256; ++Bits)
		{
			for (const int32 Marks : { 0, 1, 4 })
			{
				EclipseSquadOrderLogic::FEclipseOrderWorldFacts Facts;
				Facts.bSoldierConscious        = (Bits & 1) != 0;
				Facts.bHasPathToTarget         = (Bits & 2) != 0;
				Facts.bTargetValid             = (Bits & 4) != 0;
				Facts.bTargetVisible           = (Bits & 8) != 0;
				Facts.bHasLineToArea           = (Bits & 16) != 0;
				Facts.bHasBreachPointInRange   = (Bits & 32) != 0;
				Facts.bAllAssignedConcealed    = (Bits & 64) != 0;
				Facts.bAbilityContextValid     = (Bits & 128) != 0;
				Facts.MarkedTargetCount        = Marks;
				Visit(Facts);
			}
		}
	}
}

/**
 * FALSIFICATIE — de 8.4-lat over de VOLLE tabel.
 *
 * De bestaande sweep in EclipseSquadTests liep tot vier; met negen orders zou hij
 * de vijf nieuwe stilzwijgend overslaan en groen blijven. Dit is dezelfde belofte,
 * geteld op EclipseSquad::OrderCount zodat de zesde verb die iemand toevoegt er
 * automatisch in valt in plaats van er stilletjes buiten.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBNeverSilentTest,
	"Eclipse.Command.StageB.EveryOrderAnsweredNeverSilent",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBNeverSilentTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;
	using namespace EclipseCommandStageBTest;

	int32 Combinations = 0;
	int32 Silent = 0;
	for (int32 Index = 0; Index < EclipseSquad::OrderCount; ++Index)
	{
		const EEclipseSquadOrder Order = static_cast<EEclipseSquadOrder>(Index);
		ForEachFactCombination([this, Order, &Combinations, &Silent](const FEclipseOrderWorldFacts& Facts)
		{
			++Combinations;
			const FEclipseOrderDecision Decision = DecideOrder(Order, Facts);
			if (!Decision.bAccepted && Decision.Reason == EEclipseOrderRefusalReason::None)
			{
				++Silent;
			}
		});
	}

	AddInfo(FString::Printf(TEXT("GEMETEN  %d order-x-feitencombinaties over %d orders, %d stil"),
		Combinations, EclipseSquad::OrderCount, Silent));
	TestEqual(TEXT("Geen enkele combinatie eindigt in stilte (GDD 8.4)"), Silent, 0);

	// En de sweep moet ook echt de nieuwe verbs geraakt hebben: een test die
	// negen keer over vier orders loopt zou hierboven ook nul stiltes vinden.
	TestEqual(TEXT("De tabel telt negen orders"), EclipseSquad::OrderCount, 9);
	return true;
}

/**
 * FALSIFICATIE 1 (niet-onderhandelbaar) — elke nieuwe verb wordt geweigerd
 * wanneer hij hoort te worden geweigerd, met de JUISTE reden.
 *
 * Drie manieren waarop dit rood gaat, en alle drie zijn echte fouten die je
 * anders pas in een gevecht merkt:
 *   - een nieuwe reden die geen enkele wereld oplevert (dood label);
 *   - een nieuwe reden die bij meerdere verbs opduikt (generieke reden die alles
 *     afdekt en dus niets uitlegt);
 *   - een verb die ALTIJD accepteert (dan is zijn weigering nooit getest) of
 *     ALTIJD weigert (dan is hij niet gebouwd).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBReasonsAreReachableTest,
	"Eclipse.Command.StageB.NewReasonsAreReachableAndSpecific",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBReasonsAreReachableTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;
	using namespace EclipseCommandStageBTest;

	// Welke redenen produceert elke verb, gemeten in plaats van aangenomen.
	TMap<EEclipseSquadOrder, TSet<EEclipseOrderRefusalReason>> ReasonsPerOrder;
	TMap<EEclipseSquadOrder, int32> AcceptsPerOrder;

	for (int32 Index = 0; Index < EclipseSquad::OrderCount; ++Index)
	{
		const EEclipseSquadOrder Order = static_cast<EEclipseSquadOrder>(Index);
		ReasonsPerOrder.Add(Order);
		AcceptsPerOrder.Add(Order, 0);
		ForEachFactCombination([Order, &ReasonsPerOrder, &AcceptsPerOrder](const FEclipseOrderWorldFacts& Facts)
		{
			// Alleen BEWUSTE soldaten: 'Downed' dekt alles af zodra iemand ligt,
			// en dan zou elke verb "een reden hebben" zonder dat zijn eigen
			// weigering ooit bereikt werd. Precies het gat dat deze test moet zien.
			if (!Facts.bSoldierConscious)
			{
				return;
			}
			const FEclipseOrderDecision Decision = DecideOrder(Order, Facts);
			if (Decision.bAccepted)
			{
				++AcceptsPerOrder[Order];
			}
			else
			{
				ReasonsPerOrder[Order].Add(Decision.Reason);
			}
		});
	}

	// --- Elke nieuwe verb kan beide: accepteren én weigeren -------------------
	for (const EEclipseSquadOrder Verb : NewVerbs())
	{
		const int32 Accepts = AcceptsPerOrder[Verb];
		const TSet<EEclipseOrderRefusalReason>& Reasons = ReasonsPerOrder[Verb];
		AddInfo(FString::Printf(TEXT("GEMETEN  %s: %d accepterende werelden, %d verschillende weigerredenen"),
			*OrderName(Verb), Accepts, Reasons.Num()));

		TestTrue(FString::Printf(TEXT("%s kan geaccepteerd worden (anders is het verb onbruikbaar)"), *OrderName(Verb)),
			Accepts > 0);
		TestTrue(FString::Printf(TEXT("%s kan geweigerd worden (anders is zijn belofte nooit getoetst)"), *OrderName(Verb)),
			Reasons.Num() > 0);
	}

	// --- De drie nieuwe redenen gaan af, elk bij precies één verb -------------
	struct FReasonExpectation
	{
		EEclipseOrderRefusalReason Reason;
		EEclipseSquadOrder Owner;
	};
	const TArray<FReasonExpectation> NewReasons = {
		{ EEclipseOrderRefusalReason::NoBreachPoint,   EEclipseSquadOrder::Breach },
		{ EEclipseOrderRefusalReason::NoTargetsMarked, EEclipseSquadOrder::SyncStrike },
		{ EEclipseOrderRefusalReason::NotConcealed,    EEclipseSquadOrder::SyncStrike },
	};

	for (const FReasonExpectation& Expectation : NewReasons)
	{
		TArray<FString> ProducedBy;
		for (const TPair<EEclipseSquadOrder, TSet<EEclipseOrderRefusalReason>>& Pair : ReasonsPerOrder)
		{
			if (Pair.Value.Contains(Expectation.Reason))
			{
				ProducedBy.Add(OrderName(Pair.Key));
			}
		}
		AddInfo(FString::Printf(TEXT("GEMETEN  reden %s komt uit: %s"),
			*ReasonName(Expectation.Reason), ProducedBy.IsEmpty() ? TEXT("(NIETS)") : *FString::Join(ProducedBy, TEXT(", "))));

		// Bereikbaar: hij gaat ergens af.
		TestTrue(FString::Printf(TEXT("Reden %s is bereikbaar (een reden die nooit afgaat is geen reden)"),
			*ReasonName(Expectation.Reason)), ProducedBy.Num() > 0);

		// Specifiek: en nergens anders. Zodra hij bij twee verbs opduikt, legt hij
		// niets meer uit — dan is het een generieke reden met een specifieke naam.
		TestEqual(FString::Printf(TEXT("Reden %s hoort bij precies één verb"), *ReasonName(Expectation.Reason)),
			ProducedBy.Num(), 1);
		if (ProducedBy.Num() == 1)
		{
			TestEqual(FString::Printf(TEXT("Reden %s hoort bij %s"),
				*ReasonName(Expectation.Reason), *OrderName(Expectation.Owner)),
				ProducedBy[0], OrderName(Expectation.Owner));
		}
	}

	// --- Geen verb leent een reden die niets met hem te maken heeft ----------
	// Suppress kan geen NoRoute geven (hij loopt nergens heen), Flank geen
	// NoLineOfSight (hij kijkt nergens naar), UseAbility geen NoRoute.
	TestFalse(TEXT("Suppress weigert nooit met NoRoute — onderdrukken doe je vanaf waar je staat"),
		ReasonsPerOrder[EEclipseSquadOrder::Suppress].Contains(EEclipseOrderRefusalReason::NoRoute));
	TestFalse(TEXT("Flank weigert nooit met NoLineOfSight — flankeren is juist wat je doet zonder zicht"),
		ReasonsPerOrder[EEclipseSquadOrder::Flank].Contains(EEclipseOrderRefusalReason::NoLineOfSight));
	TestFalse(TEXT("SyncStrike weigert nooit met NoRoute"),
		ReasonsPerOrder[EEclipseSquadOrder::SyncStrike].Contains(EEclipseOrderRefusalReason::NoRoute));

	// --- En de volgorde binnen een verb legt het juiste probleem uit ---------
	// Een breach zonder breekpunt EN zonder route hoort "er is hier niets om open
	// te breken" te zeggen; "ik kom er niet" zou de speler naar een routeprobleem
	// sturen dat niet het echte is (dezelfde val als de Downed-bug van 26-07).
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bHasBreachPointInRange = false;
		Facts.bHasPathToTarget = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::Breach, Facts);
		TestTrue(TEXT("Breach zonder punt EN zonder route noemt het ontbrekende punt"),
			Decision.Reason == EEclipseOrderRefusalReason::NoBreachPoint);
	}
	{
		// Idem: niets gemarkeerd én gezien worden. "Er staat niets gemarkeerd" is
		// het probleem dat je kunt oplossen; "ze zien me" zou je laten wachten op
		// dekking die niets verandert.
		FEclipseOrderWorldFacts Facts;
		Facts.MarkedTargetCount = 0;
		Facts.bAllAssignedConcealed = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::SyncStrike, Facts);
		TestTrue(TEXT("Sync strike zonder markeringen noemt de markeringen"),
			Decision.Reason == EEclipseOrderRefusalReason::NoTargetsMarked);
	}
	{
		FEclipseOrderWorldFacts Facts;
		Facts.MarkedTargetCount = 3;
		Facts.bAllAssignedConcealed = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::SyncStrike, Facts);
		TestTrue(TEXT("Sync strike met markeringen maar gezien: NotConcealed"),
			Decision.Reason == EEclipseOrderRefusalReason::NotConcealed);
	}

	return true;
}

/**
 * De weigerzin hoort bij de REDEN, niet altijd bij het ordertype.
 *
 * Dit is de generalisatie van de Downed-bug (26-07): een neergeschoten soldaat
 * zei "No route, boss." op een MoveTo. De drie nieuwe redenen lopen in dezelfde
 * val, dus krijgen ze hun eigen pool — en de drie oude blijven met opzet de
 * orderzin delen, want dáár zegt de orderzin al wat er mis is.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBRefusalPoolTest,
	"Eclipse.Command.StageB.RefusalPoolsSeparateTheConfusingReasons",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBRefusalPoolTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	TestEqual(TEXT("Downed spreekt uit zijn eigen pool"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::Downed), FName(TEXT("Downed")));
	TestEqual(TEXT("NoBreachPoint spreekt uit zijn eigen pool"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::NoBreachPoint), FName(TEXT("NoBreachPoint")));
	TestEqual(TEXT("NoTargetsMarked spreekt uit zijn eigen pool"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::NoTargetsMarked), FName(TEXT("NoTargetsMarked")));
	TestEqual(TEXT("NotConcealed spreekt uit zijn eigen pool"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::NotConcealed), FName(TEXT("NotConcealed")));

	// En de drie die de orderzin DELEN blijven dat doen: NoRoute onder een MoveTo
	// zegt precies wat er aan de hand is, en een eigen pool zou daar alleen maar
	// een tweede plek maken waar dezelfde zin onderhouden moet worden.
	TestTrue(TEXT("NoRoute deelt de zin van het ordertype"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::NoRoute).IsNone());
	TestTrue(TEXT("NoLineOfSight deelt de zin van het ordertype"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::NoLineOfSight).IsNone());
	TestTrue(TEXT("InvalidTarget deelt de zin van het ordertype"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::InvalidTarget).IsNone());
	TestTrue(TEXT("None heeft geen pool"),
		RefusalPoolRowName(EEclipseOrderRefusalReason::None).IsNone());
	return true;
}

/** De markeringenset: cap uit data, schakelen, snoeien. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBMarkSetTest,
	"Eclipse.Command.StageB.SyncStrikeMarkSetCapsTogglesAndPrunes",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBMarkSetTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	const FGuid A(1, 0, 0, 1);
	const FGuid B(2, 0, 0, 2);
	const FGuid C(3, 0, 0, 3);
	const FGuid D(4, 0, 0, 4);
	const FGuid E(5, 0, 0, 5);

	FEclipseSyncStrikeMarkSet Marks;
	TestTrue(TEXT("Eerste markering landt"), Marks.AddMark(A, 4));
	TestFalse(TEXT("Dezelfde markering telt niet twee keer"), Marks.AddMark(A, 4));
	TestFalse(TEXT("Een ongeldige id markeert niets"), Marks.AddMark(FGuid(), 4));

	Marks.AddMark(B, 4);
	Marks.AddMark(C, 4);
	Marks.AddMark(D, 4);
	TestEqual(TEXT("Vier markeringen staan"), Marks.Num(), 4);

	// DE CAP KOMT UIT DATA (8.4: "up to 4 marked"). De vijfde hoort te weigeren,
	// en dat is het enige moment waarop een druk op de knop niets doet.
	TestFalse(TEXT("De vijfde markering weigert op de cap"), Marks.AddMark(E, 4));
	TestEqual(TEXT("En er staan er nog steeds vier"), Marks.Num(), 4);

	// Een cap van nul is een datafout en degradeert naar één (14.3.5) — de knop
	// mag niet stilletjes dood gaan omdat iemand een 0 typte.
	{
		FEclipseSyncStrikeMarkSet Degraded;
		TestTrue(TEXT("Cap 0 degradeert naar één bruikbare markering"), Degraded.AddMark(A, 0));
		TestFalse(TEXT("Maar niet naar twee"), Degraded.AddMark(B, 0));
	}

	// Schakelen: nog eens aanwijzen haalt hem weg.
	bool bMarked = true;
	TestTrue(TEXT("Schakelen op een gemarkeerd doel doet iets"), Marks.ToggleMark(A, 4, bMarked));
	TestFalse(TEXT("En laat hem los"), bMarked);
	TestEqual(TEXT("Drie over"), Marks.Num(), 3);
	TestFalse(TEXT("A staat er niet meer"), Marks.IsMarked(A));

	// Snoeien: alles wat niet meer geldig is valt weg, en de teller zegt hoeveel.
	const int32 Pruned = Marks.PruneMarks({ B });
	TestEqual(TEXT("Twee markeringen gesnoeid"), Pruned, 2);
	TestEqual(TEXT("Alleen B blijft staan"), Marks.Num(), 1);
	TestTrue(TEXT("En dat is B"), Marks.IsMarked(B));

	TestEqual(TEXT("Snoeien zonder verlies meldt nul"), Marks.PruneMarks({ B }), 0);
	Marks.Reset();
	TestEqual(TEXT("Reset laat niets staan"), Marks.Num(), 0);
	return true;
}

/**
 * Het goedkeuringsvenster van de flank.
 *
 * De dragende regel: GOEDKEUREN NA DE TIJD KEURT NIET GOED. Zonder die regel is
 * het venster decoratie — je drukt een tel te laat en de soldaat vertrekt alsnog
 * naar een route die jij allang had losgelaten.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBFlankWindowTest,
	"Eclipse.Command.StageB.FlankApprovesOnlyInsideTheWindow",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBFlankWindowTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;
	constexpr double Timeout = 6.0;

	// Voorstel -> goedkeuring binnen de tijd.
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 100.0, Timeout);
		TestTrue(TEXT("Voorgesteld"), State.State == EEclipseFlankState::Proposed);
		TestTrue(TEXT("Het venster staat open"), IsFlankWindowOpen(State, 103.0, Timeout));

		State = ApplyFlankSignal(State, EEclipseFlankSignal::Approve, 103.0, Timeout);
		TestTrue(TEXT("Binnen de tijd keurt goed"), State.State == EEclipseFlankState::Approved);
	}

	// Voorstel -> te laat. DIT is de regel waar alles om draait.
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 100.0, Timeout);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Approve, 100.0 + Timeout + 0.01, Timeout);
		TestTrue(TEXT("Te laat keurt NIET goed"), State.State != EEclipseFlankState::Approved);
		TestTrue(TEXT("Te laat verloopt"), State.State == EEclipseFlankState::Expired);
	}

	// Precies op de rand telt nog mee: het venster is inclusief, net als het
	// stabilize-venster (de dramatische redding op de laatste tel is ontwerp).
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 0.0, Timeout);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Approve, Timeout, Timeout);
		TestTrue(TEXT("Op de rand keurt nog goed"), State.State == EEclipseFlankState::Approved);
	}

	// De klok alleen laat hem verlopen, ook als de speler niets doet.
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 0.0, Timeout);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Tick, 1.0, Timeout);
		TestTrue(TEXT("Een tik binnen de tijd verandert niets"), State.State == EEclipseFlankState::Proposed);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Tick, Timeout + 1.0, Timeout);
		TestTrue(TEXT("Een tik na de tijd laat hem verlopen"), State.State == EEclipseFlankState::Expired);

		// En verlopen blijft verlopen: een late tik maakt er geen goedkeuring van.
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Approve, Timeout + 2.0, Timeout);
		TestTrue(TEXT("Goedkeuren na verval doet niets"), State.State == EEclipseFlankState::Expired);
	}

	// Annuleren is iets ANDERS dan verlopen: jouw nieuwe besluit tegenover jouw
	// besluiteloosheid. De debug-UI hoort het verschil te kunnen tonen.
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 0.0, Timeout);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Cancel, 1.0, Timeout);
		TestTrue(TEXT("Geannuleerd"), State.State == EEclipseFlankState::Cancelled);
		TestFalse(TEXT("Een geannuleerd venster staat niet open"), IsFlankWindowOpen(State, 1.0, Timeout));
	}

	// Goedkeuren zonder voorstel is een no-op en geen fout.
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Approve, 5.0, Timeout);
		TestTrue(TEXT("Goedkeuren zonder voorstel blijft Idle"), State.State == EEclipseFlankState::Idle);
	}

	// Een nieuw voorstel wint van een oud, ook van een goedgekeurd.
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 0.0, Timeout);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Approve, 1.0, Timeout);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 50.0, Timeout);
		TestTrue(TEXT("Opnieuw flankeren stelt opnieuw voor"), State.State == EEclipseFlankState::Proposed);
		TestEqual(TEXT("Met een verse stempel"), State.ProposedWallSeconds, 50.0);
	}

	// Geen venster geconfigureerd = geen verval (14.3.5: degradeer bruikbaar).
	{
		FEclipseFlankApproval State;
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Propose, 0.0, /*Timeout*/ 0.0);
		State = ApplyFlankSignal(State, EEclipseFlankSignal::Tick, 10000.0, /*Timeout*/ 0.0);
		TestTrue(TEXT("Zonder venster verloopt er niets"), State.State == EEclipseFlankState::Proposed);
	}
	return true;
}

/**
 * De verdeling van een sync strike DEKT elke markering precies één keer.
 *
 * Waarom dit een eigenschap is en geen voorbeeld: een halve sync strike is de
 * ergste uitkomst die er is. Vier man gaan tegelijk, drie keeltjes gaan om, en de
 * vierde vijand staat te kijken — dan heb je het alarm dat het verb moest
 * voorkomen, en je hebt je markeringen ervoor opgebruikt.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBAssignmentTest,
	"Eclipse.Command.StageB.SyncStrikeAssignmentCoversEveryMarkExactlyOnce",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBAssignmentTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	for (int32 Soldiers = 1; Soldiers <= 4; ++Soldiers)
	{
		for (int32 Marks = 1; Marks <= 4; ++Marks)
		{
			TArray<int32> Covered;
			for (int32 Soldier = 0; Soldier < Soldiers; ++Soldier)
			{
				for (const int32 MarkIndex : AssignSyncStrikeMarkIndices(Soldier, Soldiers, Marks))
				{
					TestTrue(FString::Printf(TEXT("%d soldaten / %d marks: index %d ligt binnen bereik"),
						Soldiers, Marks, MarkIndex), MarkIndex >= 0 && MarkIndex < Marks);
					TestFalse(FString::Printf(TEXT("%d soldaten / %d marks: mark %d krijgt geen twee uitvoerders"),
						Soldiers, Marks, MarkIndex), Covered.Contains(MarkIndex));
					Covered.Add(MarkIndex);
				}
			}
			TestEqual(FString::Printf(TEXT("%d soldaten / %d marks: elke markering heeft een uitvoerder"),
				Soldiers, Marks), Covered.Num(), Marks);
		}
	}

	// Randgevallen leveren niets op in plaats van een crash of een index -1.
	TestEqual(TEXT("Geen markeringen = geen toewijzing"), AssignSyncStrikeMarkIndices(0, 3, 0).Num(), 0);
	TestEqual(TEXT("Geen soldaten = geen toewijzing"), AssignSyncStrikeMarkIndices(0, 0, 3).Num(), 0);
	TestEqual(TEXT("Een soldaat buiten de squad krijgt niets"), AssignSyncStrikeMarkIndices(5, 3, 3).Num(), 0);
	TestEqual(TEXT("Een negatieve index krijgt niets"), AssignSyncStrikeMarkIndices(-1, 3, 3).Num(), 0);
	return true;
}

/**
 * FALSIFICATIE 3 (pure helft) — Stealth is aantoonbaar iets ANDERS dan Recon.
 *
 * De in-wereld-helft telt echte schoten (zie EclipseCommandStageBWorldTests). Dit
 * pint de regel zelf vast, en vooral de plek waar de twee doctrines uit elkaar
 * lopen: de vijand die ons OPMERKT zonder te schieten. Onder Recon blijft de
 * squad daar stil terwijl er vier man op hem af komen; onder Stealth vuurt hij.
 * Zonder deze kolom zijn het twee namen voor hetzelfde gedrag.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBStealthDisciplineTest,
	"Eclipse.Command.StageB.StealthHoldsFireUntilOrderedOrCompromised",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBStealthDisciplineTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	const FEclipseFireDisciplineFacts Quiet;                                  // niets aan de hand
	FEclipseFireDisciplineFacts Ordered;   Ordered.bOrderedToFire = true;     // jij zei: vuur
	FEclipseFireDisciplineFacts UnderFire; UnderFire.bTakenFire = true;       // er wordt op hem geschoten
	FEclipseFireDisciplineFacts Spotted;   Spotted.bEnemyAware = true;        // ze hebben ons gezien

	// --- Stealth: dicht tot een van de twee poorten opengaat ------------------
	TestFalse(TEXT("Stealth zwijgt als er niets aan de hand is"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Stealth, Quiet));
	TestTrue(TEXT("Stealth vuurt als je het beveelt"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Stealth, Ordered));
	TestTrue(TEXT("Stealth vuurt als er op hem geschoten wordt"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Stealth, UnderFire));
	TestTrue(TEXT("Stealth vuurt zodra de vijand ons doorheeft"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Stealth, Spotted));

	// --- Recon: alleen als er op hem geschoten is ----------------------------
	TestFalse(TEXT("Recon zwijgt als er niets aan de hand is"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Recon, Quiet));
	TestTrue(TEXT("Recon vuurt onder vuur"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Recon, UnderFire));

	// --- HET VERSCHIL, en het is er maar één ---------------------------------
	// Zodra deze twee asserts allebei omvallen, zijn Recon en Stealth hetzelfde
	// geworden en heeft een van de twee geen bestaansrecht meer.
	TestFalse(TEXT("VERSCHIL: Recon blijft stil terwijl de vijand ons al ziet"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Recon, Spotted));
	TestTrue(TEXT("VERSCHIL: Stealth vuurt dan wel"),
		StanceAllowsAutonomousFire(EEclipseSquadStance::Stealth, Spotted));

	// --- De drie basisdoctrines blijven de basis: altijd vuren ---------------
	for (const EEclipseSquadStance Stance : { EEclipseSquadStance::Ready, EEclipseSquadStance::Overwatch, EEclipseSquadStance::Aggressive })
	{
		TestTrue(FString::Printf(TEXT("%s vuurt uit zichzelf (dat is de basis, geen feature)"),
			EclipseSquad::StanceLabel(Stance)),
			StanceAllowsAutonomousFire(Stance, Quiet));
	}

	// De doctrine is te KIEZEN, ook op naam — anders bestaat stealth wel in de
	// enum en nergens waar iemand hem kan zetten.
	EEclipseSquadStance Parsed = EEclipseSquadStance::Ready;
	TestTrue(TEXT("'stealth' is als doctrine te kiezen"), EclipseSquad::ParseStance(TEXT("stealth"), Parsed));
	TestTrue(TEXT("En levert de stealth-doctrine op"), Parsed == EEclipseSquadStance::Stealth);
	TestFalse(TEXT("Onzin levert niets op"), EclipseSquad::ParseStance(TEXT("kamikaze"), Parsed));
	TestEqual(TEXT("Er zijn vijf doctrines"), EclipseSquad::StanceCount, 5);
	return true;
}

/**
 * De Stage B-getallen staan in DATA en worden GELEZEN.
 *
 * Beide helften tellen. Een getal dat niet in data staat is een 14.2-overtreding;
 * een getal dat in data staat en niemand leest is erger, want dan verdraait de
 * owner het en gebeurt er niets (de dode-veldensweep van 26-07 vond er 23).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBTuningTest,
	"Eclipse.Command.StageB.TuningDefaultsMatchTheSpec",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBTuningTest::RunTest(const FString& Parameters)
{
	const UEclipseCommandModeTuningAsset* Defaults = GetDefault<UEclipseCommandModeTuningAsset>();

	TestEqual(TEXT("Sync-strike cap 4 (8.4: up to 4 marked)"), Defaults->MaxSyncStrikeMarks, 4);
	TestTrue(TEXT("Het flankvenster is een echt venster (> 0)"), Defaults->FlankApprovalTimeoutSeconds > 0.0f);
	TestTrue(TEXT("De onderdrukkingsstraal is een echt gebied"), Defaults->SuppressRadiusCm > 0.0f);
	TestTrue(TEXT("Onderdrukken heeft een einde"), Defaults->SuppressBurstSeconds > 0.0f);
	TestTrue(TEXT("Een flank zwenkt echt uit"), Defaults->FlankOffsetCm > 0.0f);
	TestTrue(TEXT("Een breekpunt heeft een bereik"), Defaults->BreachPointRangeCm > 0.0f);
	return true;
}

/**
 * EEN ASSET, TWEE LEZERS — en die moeten hetzelfde asset lezen.
 *
 * De mode-component leest DA_CommandModeTuning voor de tijdvertraging; de
 * squad-subsystem leest hem voor de Stage B-getallen. Ze staan in verschillende
 * mappen en horen bij verschillende bouwers, en dat is precies de situatie waarin
 * twee letterlijke paden één keer uit elkaar lopen: de owner stelt de dilatatie
 * af op asset A terwijl zijn sync-strike-cap uit asset B komt, en niets meldt dat.
 *
 * Via reflectie, want het veld is privé — en dat is hier geen truc maar de kern:
 * de test moet lezen wat de component ECHT geconfigureerd heeft, niet wat een
 * comment erover zegt.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseStageBTuningOneSourceTest,
	"Eclipse.Command.StageB.TuningHasOneSource",
	EclipseCommandStageBTest::TestFlags)

bool FEclipseStageBTuningOneSourceTest::RunTest(const FString& Parameters)
{
	const FSoftObjectProperty* TuningProperty = CastField<FSoftObjectProperty>(
		UEclipseCommandModeComponent::StaticClass()->FindPropertyByName(TEXT("Tuning")));
	if (!TestNotNull(TEXT("De component heeft een Tuning-veld"), TuningProperty))
	{
		return false;
	}

	const UEclipseCommandModeComponent* Defaults = GetDefault<UEclipseCommandModeComponent>();
	const void* ValuePtr = TuningProperty->ContainerPtrToValuePtr<void>(Defaults);
	const FString ComponentPath = TuningProperty->GetPropertyValue(ValuePtr).ToSoftObjectPath().ToString();
	const FString SharedPath = FSoftObjectPath(EclipseCommandMode::DefaultTuningPath).ToString();

	AddInfo(FString::Printf(TEXT("GEMETEN  component leest '%s'"), *ComponentPath));
	AddInfo(FString::Printf(TEXT("GEMETEN  subsystem leest '%s'"), *SharedPath));
	TestEqual(TEXT("Component en subsystem lezen HETZELFDE DA_CommandModeTuning"), ComponentPath, SharedPath);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
