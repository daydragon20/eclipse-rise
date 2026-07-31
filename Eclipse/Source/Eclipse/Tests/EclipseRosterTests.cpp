// Unit tests for SPEC-P1-07 (GDD 14.4): generation determinism, the death
// resolution permutations, and wounded availability math — all pure logic.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Squad/EclipseRosterLogic.h"

namespace EclipseRosterTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	EclipseRosterLogic::FEclipseNameGenerationParams MakeParams()
	{
		EclipseRosterLogic::FEclipseNameGenerationParams Params;
		Params.FirstNames = { TEXT("Vara"), TEXT("Oscar"), TEXT("Ilya"), TEXT("Mirin") };
		Params.LastNames = { TEXT("Chen"), TEXT("Line"), TEXT("Kessler") };
		Params.TraitIds = { TEXT("Trait_SteadyHands"), TEXT("Trait_Claustrophobic") };
		return Params;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseRosterGenerationTest,
	"Eclipse.Squad.Roster.DeterministicGeneration",
	EclipseRosterTest::TestFlags)

bool FEclipseRosterGenerationTest::RunTest(const FString& Parameters)
{
	const EclipseRosterLogic::FEclipseNameGenerationParams Params = EclipseRosterTest::MakeParams();

	const FEclipseSoldierRecord A1 = EclipseRosterLogic::GenerateSoldier(TEXT("Kessara"), Params, 7);
	const FEclipseSoldierRecord A2 = EclipseRosterLogic::GenerateSoldier(TEXT("Kessara"), Params, 7);
	const FEclipseSoldierRecord B = EclipseRosterLogic::GenerateSoldier(TEXT("Kessara"), Params, 8);

	TestEqual(TEXT("Same seed, same person (id)"), A1.SoldierId, A2.SoldierId);
	TestEqual(TEXT("Same seed, same person (name)"), A1.Name, A2.Name);
	TestEqual(TEXT("Same seed, same person (trait)"), A1.TraitId, A2.TraitId);
	TestNotEqual(TEXT("Different seed, different id"), A1.SoldierId, B.SoldierId);
	TestTrue(TEXT("Name drawn from pools"), !A1.Name.IsEmpty() && A1.Name.Contains(TEXT(" ")));
	TestTrue(TEXT("Trait assigned"), !A1.TraitId.IsNone());

	// Empty pools: still a named human (Pillar 3 tolerates no blank names).
	const FEclipseSoldierRecord Fallback = EclipseRosterLogic::GenerateSoldier(TEXT("Kessara"), {}, 3);
	TestTrue(TEXT("Fallback name generated"), !Fallback.Name.IsEmpty());
	return true;
}

/**
 * PIJLER 3 ALS TOETS: een ploeg van N heeft N onderscheidbare mensen.
 *
 * ROOD = het defect van 31-07 staat er weer: twee soldaten met dezelfde voornaam,
 *        en dan is één op de drie in een bark of een order niet uit een ander te
 *        houden.
 *
 * DE CONTROLEPROEF GAAT VOOROP, en die is hier het halve werk. Een test die alleen
 * "0 botsingen" meet, is groen zolang de teller ook bij een échte botsing 0 zegt —
 * precies de vorm van meting die dit project al eerder heeft laten stranden. Dus
 * eerst: bouw met de hand een ploeg die WEL botst, en eis dat de teller hem ziet.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseRosterDistinctFirstNamesTest,
	"Eclipse.Squad.Roster.EverySoldierHasHisOwnFirstName",
	EclipseRosterTest::TestFlags)

bool FEclipseRosterDistinctFirstNamesTest::RunTest(const FString& Parameters)
{
	using namespace EclipseRosterLogic;

	// ---- CONTROLEPROEF: kan de teller een botsing überhaupt zien? --------------
	TArray<FEclipseSoldierRecord> Botsend;
	Botsend.AddDefaulted_GetRef().Name = TEXT("Sef Voss");
	Botsend.AddDefaulted_GetRef().Name = TEXT("Sef Chen");
	Botsend.AddDefaulted_GetRef().Name = TEXT("Anke Stahl");
	const int32 GezienInDeControleproef = CountFirstNameCollisions(Botsend);
	AddInfo(FString::Printf(TEXT("GEMETEN controleproef (de squad van het scherm van 31-07): %d botsende namen van 3"),
		GezienInDeControleproef));
	if (!TestEqual(TEXT("CONTROLEPROEF: de teller ziet de twee 'Sef'-en van het echte scherm"), GezienInDeControleproef, 2))
	{
		// Zonder dit kan de rest van deze test niets bewijzen.
		return false;
	}
	TestEqual(TEXT("CONTROLEPROEF: een ploeg zonder botsing telt 0"), CountFirstNameCollisions({}), 0);

	// ---- DE EIGENLIJKE METING --------------------------------------------------
	//
	// De pool van de verscheepte DT_NamePools-rij "Kessara", letterlijk overgenomen
	// (16 voornamen, 12 achternamen). Een test op een verzonnen pool zou de kans op
	// een botsing zelf kunnen wegnemen en dan meet hij zijn eigen fixture.
	FEclipseNameGenerationParams Kessara;
	Kessara.FirstNames = { TEXT("Vara"), TEXT("Oscar"), TEXT("Ilya"), TEXT("Mirin"), TEXT("Sef"),
		TEXT("Anke"), TEXT("Dario"), TEXT("Lupe"), TEXT("Tessa"), TEXT("Jorun"), TEXT("Calla"),
		TEXT("Bren"), TEXT("Nadia"), TEXT("Piotr"), TEXT("Yara"), TEXT("Emeric") };
	Kessara.LastNames = { TEXT("Chen"), TEXT("Line"), TEXT("Kessler"), TEXT("Voss"), TEXT("Ashdown"),
		TEXT("Ferro"), TEXT("Okonkwo"), TEXT("Reyes"), TEXT("Stahl"), TEXT("Marek"), TEXT("Odum"),
		TEXT("Calder") };

	// ELKE PLOEGGROOTTE TOT DE POOL VOL IS, en niet alleen die van vandaag. Drie is
	// de maat waarop het defect gezien is; met alleen die maat zou een fix die
	// toevallig bij 3 werkt en bij 6 omvalt hier groen blijven staan.
	for (int32 Size = 2; Size <= Kessara.FirstNames.Num(); ++Size)
	{
		bool bExhausted = true;
		const TArray<FEclipseSoldierRecord> Roster = GenerateRoster(TEXT("Kessara"), Kessara, Size, /*FirstSeed*/ 1, &bExhausted);
		const int32 Collisions = CountFirstNameCollisions(Roster);

		TestEqual(FString::Printf(TEXT("ploeg van %d levert %d mensen"), Size, Size), Roster.Num(), Size);
		TestFalse(FString::Printf(TEXT("ploeg van %d past in de pool van %d voornamen"), Size, Kessara.FirstNames.Num()), bExhausted);
		if (Collisions != 0)
		{
			AddInfo(FString::Printf(TEXT("GEMETEN ploeg van %d: %d botsende voornamen"), Size, Collisions));
			for (const FEclipseSoldierRecord& Soldier : Roster)
			{
				AddInfo(FString::Printf(TEXT("GEMETEN   %s"), *Soldier.Name));
			}
		}
		TestEqual(FString::Printf(TEXT("ploeg van %d: geen twee soldaten delen een voornaam"), Size), Collisions, 0);

		// En de volledige namen ook uniek — een ploeg met twee keer dezelfde persoon
		// zou op de voornaam-teller onzichtbaar blijven als de voornamen wél verschilden.
		TSet<FString> Volledige;
		for (const FEclipseSoldierRecord& Soldier : Roster)
		{
			Volledige.Add(Soldier.Name);
		}
		TestEqual(FString::Printf(TEXT("ploeg van %d: geen twee identieke volledige namen"), Size), Volledige.Num(), Size);
	}

	// ---- DE POOL TE KLEIN: luid degraderen, niet stil ---------------------------
	FEclipseNameGenerationParams Krap;
	Krap.FirstNames = { TEXT("Sef"), TEXT("Anke") };
	Krap.LastNames = { TEXT("Voss"), TEXT("Chen"), TEXT("Stahl") };
	bool bKrapExhausted = false;
	const TArray<FEclipseSoldierRecord> KrappePloeg = GenerateRoster(TEXT("Kessara"), Krap, 4, 1, &bKrapExhausted);
	TestTrue(TEXT("een pool die te klein is, meldt zichzelf (14.3.5)"), bKrapExhausted);
	TSet<FString> KrappeNamen;
	for (const FEclipseSoldierRecord& Soldier : KrappePloeg)
	{
		KrappeNamen.Add(Soldier.Name);
		AddInfo(FString::Printf(TEXT("GEMETEN krappe pool: %s"), *Soldier.Name));
	}
	TestEqual(TEXT("zelfs met een te krappe pool zijn de VOLLEDIGE namen uniek"), KrappeNamen.Num(), KrappePloeg.Num());

	// ---- DETERMINISME BLIJFT ---------------------------------------------------
	const TArray<FEclipseSoldierRecord> A = GenerateRoster(TEXT("Kessara"), Kessara, 6);
	const TArray<FEclipseSoldierRecord> B = GenerateRoster(TEXT("Kessara"), Kessara, 6);
	bool bIdentical = A.Num() == B.Num();
	for (int32 Index = 0; bIdentical && Index < A.Num(); ++Index)
	{
		bIdentical = A[Index].Name == B[Index].Name && A[Index].SoldierId == B[Index].SoldierId;
	}
	TestTrue(TEXT("dezelfde pools en hetzelfde zaad geven dezelfde ploeg"), bIdentical);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseRosterCasualtyResolutionTest,
	"Eclipse.Squad.Roster.DeathResolutionPermutations",
	EclipseRosterTest::TestFlags)

bool FEclipseRosterCasualtyResolutionTest::RunTest(const FString& Parameters)
{
	FEclipseCampaignState State;
	FEclipseSoldierRecord& Soldier = State.Roster.AddDefaulted_GetRef();
	Soldier.SoldierId = FGuid(1, 1, 1, 1);
	Soldier.Name = TEXT("Mirin Kessler");

	TMap<FGuid, FName> Downed;
	Downed.Add(Soldier.SoldierId, TEXT("Gunfire"));

	// Downed + win -> wounded, out N days (N from data).
	TArray<FEclipseResolvedCasualty> WinResolution = EclipseRosterLogic::ResolveCasualties(Downed, State, /*bMissionSuccess*/ true, /*WoundedDaysOut*/ 5);
	TestEqual(TEXT("One casualty resolved"), WinResolution.Num(), 1);
	TestFalse(TEXT("Downed + win -> wounded, not dead"), WinResolution[0].bDead);
	TestEqual(TEXT("Wounded duration from data"), WinResolution[0].DaysOut, 5);
	TestEqual(TEXT("Name carried for the record"), WinResolution[0].SoldierName, FString(TEXT("Mirin Kessler")));

	// Downed + fail -> dead (extraction without the body, SPEC-P1-07 stub).
	TArray<FEclipseResolvedCasualty> LossResolution = EclipseRosterLogic::ResolveCasualties(Downed, State, /*bMissionSuccess*/ false, 5);
	TestTrue(TEXT("Downed + fail -> dead"), LossResolution[0].bDead);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseRosterAvailabilityTest,
	"Eclipse.Squad.Roster.WoundedAvailability",
	EclipseRosterTest::TestFlags)

bool FEclipseRosterAvailabilityTest::RunTest(const FString& Parameters)
{
	FEclipseSoldierRecord Soldier;
	Soldier.Status = EEclipseSoldierStatus::Wounded;
	Soldier.WoundedUntilDay = 10;

	TestFalse(TEXT("Wounded and out on day 9"), EclipseRosterLogic::IsSoldierAvailableOnDay(Soldier, 9));
	TestTrue(TEXT("Recovered on day 10"), EclipseRosterLogic::IsSoldierAvailableOnDay(Soldier, 10));

	Soldier.Status = EEclipseSoldierStatus::Dead;
	TestFalse(TEXT("Dead is forever (GDD 4.2.7)"), EclipseRosterLogic::IsSoldierAvailableOnDay(Soldier, 999));

	Soldier.Status = EEclipseSoldierStatus::Deployed;
	TestFalse(TEXT("Deployed is unavailable for a second mission"), EclipseRosterLogic::IsSoldierAvailableOnDay(Soldier, 10));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
