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
