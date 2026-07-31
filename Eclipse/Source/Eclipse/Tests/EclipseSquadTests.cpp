// Squad-AI scenario suite, headless tier (GDD 14.4: runs per merge; a silent
// order failure is a release blocker BY TEST). The decision table is exercised
// exhaustively — no input combination may map to silence; the in-world variant
// of these scenarios attaches to the graybox map in the PIE pass.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Squad/EclipseRosterLogic.h"
#include "Squad/EclipseSquadOrderLogic.h"

namespace EclipseSquadTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadOrderDecisionTest,
	"Eclipse.Squad.Orders.DecisionTableNeverSilent",
	EclipseSquadTest::TestFlags)

bool FEclipseSquadOrderDecisionTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	// Scenario: move order with a route -> acknowledged.
	{
		FEclipseOrderWorldFacts Facts;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::MoveTo, Facts);
		TestTrue(TEXT("Move with route accepted"), Decision.bAccepted);
	}

	// Scenario: blocked route -> refusal with NoRoute, never timeout-silence (SPEC-P1-06).
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bHasPathToTarget = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::MoveTo, Facts);
		TestFalse(TEXT("Blocked move refused"), Decision.bAccepted);
		TestTrue(TEXT("Refusal carries NoRoute"), Decision.Reason == EEclipseOrderRefusalReason::NoRoute);
	}

	// Scenario: focus on dead/invalid target -> refusal (SPEC-P1-06 scenario list).
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bTargetValid = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::FocusTarget, Facts);
		TestFalse(TEXT("Invalid target refused"), Decision.bAccepted);
		TestTrue(TEXT("Refusal carries InvalidTarget"), Decision.Reason == EEclipseOrderRefusalReason::InvalidTarget);
	}

	// Scenario: focus without line of sight -> "Can't see it".
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bTargetVisible = false;
		const FEclipseOrderDecision Decision = DecideOrder(EEclipseSquadOrder::FocusTarget, Facts);
		TestFalse(TEXT("Unseen target refused"), Decision.bAccepted);
		TestTrue(TEXT("Refusal carries NoLineOfSight"), Decision.Reason == EEclipseOrderRefusalReason::NoLineOfSight);
	}

	// Scenario: downed soldier refuses everything with the same reason.
	{
		FEclipseOrderWorldFacts Facts;
		Facts.bSoldierConscious = false;
		for (const EEclipseSquadOrder Order : { EEclipseSquadOrder::MoveTo, EEclipseSquadOrder::FocusTarget, EEclipseSquadOrder::Hold, EEclipseSquadOrder::Regroup })
		{
			const FEclipseOrderDecision Decision = DecideOrder(Order, Facts);
			TestFalse(TEXT("Downed soldier refuses"), Decision.bAccepted);
			TestTrue(TEXT("Refusal carries Downed"), Decision.Reason == EEclipseOrderRefusalReason::Downed);
		}
	}

	// Exhaustive zero-silence sweep: every order x every fact combination yields
	// either acceptance or a named reason (the GDD 8.4 release-blocker contract).
	//
	// OP EclipseSquad::OrderCount EN NIET OP 4. Hier stond een 4, en dat was waar
	// zolang de tabel vier orders had; SPEC-P2-02 Stage B maakte er negen van en
	// deze lus zou de vijf nieuwe stilzwijgend overslaan — groen blijven terwijl
	// hij niets meer dekt is de gevaarlijkste vorm van een test. De VOLLEDIGE
	// Stage B-sweep (mét de nieuwe feiten) staat in EclipseCommandStageBTests.cpp;
	// deze lus houdt de oude belofte over de hele tabel.
	for (int32 OrderIndex = 0; OrderIndex < EclipseSquad::OrderCount; ++OrderIndex)
	{
		for (int32 Bits = 0; Bits < 16; ++Bits)
		{
			FEclipseOrderWorldFacts Facts;
			Facts.bSoldierConscious = (Bits & 1) != 0;
			Facts.bHasPathToTarget = (Bits & 2) != 0;
			Facts.bTargetValid = (Bits & 4) != 0;
			Facts.bTargetVisible = (Bits & 8) != 0;

			const FEclipseOrderDecision Decision = DecideOrder(static_cast<EEclipseSquadOrder>(OrderIndex), Facts);
			TestTrue(TEXT("Accepted or named reason — never silence"),
				Decision.bAccepted || Decision.Reason != EEclipseOrderRefusalReason::None);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadBarkDeterminismTest,
	"Eclipse.Squad.Orders.BarkSelection",
	EclipseSquadTest::TestFlags)

bool FEclipseSquadBarkDeterminismTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	const TArray<FString> Pool = { TEXT("On it."), TEXT("Moving."), TEXT("Copy that.") };
	const FGuid SoldierA(1, 2, 3, 4);
	const FGuid SoldierB(4, 3, 2, 1);

	TestEqual(TEXT("Same soldier, same salt, same voice"), PickBarkLine(Pool, SoldierA, 7), PickBarkLine(Pool, SoldierA, 7));
	TestTrue(TEXT("Line comes from the pool"), Pool.Contains(PickBarkLine(Pool, SoldierB, 7)));
	TestEqual(TEXT("Empty pool still answers (silence forbidden)"), PickBarkLine({}, SoldierA, 7), FString(TEXT("Copy.")));
	return true;
}

// ---------------------------------------------------------------------------
// STAAT ER EEN MENS OP HET SCHERM, OF EEN GETAL? (pijler 3, 31-07)
// ---------------------------------------------------------------------------
//
// Gemeten op een frame van de speelronde: de squadlijst toonde drie regels
// `45434C53  ->  Hold`. Drie verschillende soldaten, drie keer hetzelfde getal.
// De oorzaak is niet een ontbrekende naam maar een verkeerd stuk id: de HUD
// toonde `SoldierId.ToString().Left(8)`, en de eerste 32 bits van elke
// soldaat-GUID zijn per constructie de vaste kop 0x45434C53 ("ECLS", zie
// EclipseRosterLogic::GenerateSoldier).
//
// DEZE TEST MOET ROOD WORDEN OP DE HALVE FIX. De hele GUID tonen geeft wél drie
// verschillende regels en zet nog steeds geen mens op het scherm; een test die
// alleen op onderlinge verschillen let, zou dat goedkeuren. Vandaar twee asserts
// naast elkaar: de regel begint met de NAAM zoals die in de data staat, en er
// staat nergens een hexadecimale kop in.
//
// De ids komen uit GenerateSoldier zelf en niet uit een verzonnen FGuid: zo kan
// de test niet uit de pas lopen met hoe de roster ze echt bouwt.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadOrderLineNamesTest,
	"Eclipse.Squad.Orders.OrderLinesNameThePerson",
	EclipseSquadTest::TestFlags)

bool FEclipseSquadOrderLineNamesTest::RunTest(const FString& Parameters)
{
	using namespace EclipseSquadOrderLogic;

	// DRIE VERSCHILLENDE NAMEN, EXPLICIET GEZET EN NIET GEHOOPT.
	//
	// De eerste versie liet GenerateSoldier de namen trekken uit een pool van 3x3 —
	// en zaad 1 en 2 trokken allebei "Mara Vale". De test viel toen op zijn eigen
	// opzet in plaats van op het defect: de voorwaarde "drie verschillende namen"
	// was een aanname, geen feit. De ID's komen nog steeds uit GenerateSoldier
	// zelf, want dáár mag de test niet uit de pas lopen met de echte roster; alleen
	// de naam wordt gezet, want die is hier het onderwerp.
	EclipseRosterLogic::FEclipseNameGenerationParams Params;
	Params.FirstNames = { TEXT("Mara") };
	Params.LastNames = { TEXT("Vale") };

	const TArray<FString> Names = { TEXT("Mara Vale"), TEXT("Brick Okonkwo"), TEXT("Kaya Reyes") };
	TArray<FEclipseSoldierRecord> Soldiers;
	for (int32 Index = 0; Index < Names.Num(); ++Index)
	{
		FEclipseSoldierRecord Soldier = EclipseRosterLogic::GenerateSoldier(TEXT("Kessara"), Params, Index + 1);
		Soldier.Name = Names[Index];
		Soldiers.Add(Soldier);
	}

	// De controleproef eerst: staat de vaste kop er echt in élke id? Zonder deze
	// regel bewijst "de kop staat niet in de regel" niets — dan zou de test ook
	// groen zijn als de ids toevallig anders waren opgebouwd. En de ids moeten
	// onderling verschillen, anders toetst de laatste assert hieronder niets.
	for (int32 Index = 0; Index < Soldiers.Num(); ++Index)
	{
		TestTrue(TEXT("controleproef: elke soldaat-id draagt dezelfde vaste kop 45434C53"),
			Soldiers[Index].SoldierId.ToString(EGuidFormats::Digits).StartsWith(TEXT("45434C53")));
		if (Index > 0)
		{
			TestNotEqual(TEXT("controleproef: de ids zelf verschillen wel degelijk"),
				Soldiers[Index].SoldierId, Soldiers[Index - 1].SoldierId);
		}
	}

	TArray<FString> Lines;
	for (const FEclipseSoldierRecord& Soldier : Soldiers)
	{
		Lines.Add(ComposeOrderStateLine(Soldier.Name, Soldier.SoldierId, TEXT("Hold")));
	}

	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		AddInfo(FString::Printf(TEXT("GEMETEN regel %d: '%s'"), Index, *Lines[Index]));

		TestTrue(*FString::Printf(TEXT("regel %d begint met de naam uit de data ('%s')"), Index, *Names[Index]),
			Lines[Index].StartsWith(Names[Index]));
		TestTrue(*FString::Printf(TEXT("regel %d noemt de order"), Index),
			Lines[Index].Contains(TEXT("Hold")));
		// DE ANTI-TERUGVAL. Elke vorm van "toon de id" — Left(8), de hele GUID,
		// een streepjesformaat — sleept deze kop mee.
		TestFalse(*FString::Printf(TEXT("regel %d toont GEEN id-kop (45434C53)"), Index),
			Lines[Index].Contains(TEXT("45434C53")));
	}

	// En drie mensen zijn drie regels. Dit is de zwakste van de drie asserts en
	// staat er alleen omdat het oorspronkelijke defect zich zo liet zien.
	TestNotEqual(TEXT("soldaat 0 en 1 lezen verschillend"), Lines[0], Lines[1]);
	TestNotEqual(TEXT("soldaat 1 en 2 lezen verschillend"), Lines[1], Lines[2]);
	TestNotEqual(TEXT("soldaat 0 en 2 lezen verschillend"), Lines[0], Lines[2]);

	// Een soldaat die niet in de roster staat is een GAT, en een gat hoort er niet
	// uit te zien als een naam. Wat er dan wel staat mag geen vaste kop zijn — dan
	// zou het gat er voor iedereen identiek uitzien, precies de fout van hierboven.
	{
		const FEclipseSoldierRecord Ghost = EclipseRosterLogic::GenerateSoldier(TEXT("Kessara"), Params, 42);
		const FString Line = ComposeOrderStateLine(FString(), Ghost.SoldierId, TEXT("Hold"));
		AddInfo(FString::Printf(TEXT("GEMETEN gatregel: '%s'"), *Line));
		TestTrue(TEXT("een naamloze soldaat meldt zich als gat"), Line.Contains(TEXT("niet in de roster")));
		TestFalse(TEXT("de gatregel toont GEEN vaste id-kop"), Line.Contains(TEXT("45434C53")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
