// DE WAPENWISSEL MOET IETS ZICHTBAARS DOEN — owner-punt 5, O-5 "volledig", 31-07.
//
// ---------------------------------------------------------------- WAT DIT IS
// De wapenlaag was als DATA al compleet: twee slots, RB wisselt, elk slot houdt
// zijn eigen magazijn (`component=1 magazijn=30 munitie=30`). En toch deed de
// wissel visueel NIETS. De oorzaak stond in REFERENTIE_TPS.md hoofdstuk 4: het
// wapen was geen los object maar zat in de karaktermesh, dus er viel niets te
// wisselen.
//
// ------------------------------------------- WAT DEZE TEST WEL EN NIET BEWIJST
// Dit is met opzet een KLEINE test, en het is belangrijk dat hij zijn eigen
// grenzen kent — een groene suite is in dit dossier expliciet géén bewijs:
//
//   "Een groene testsuite is hier geen bewijs — de hele reden dat dit dossier
//    bestaat is dat een codeconclusie het van een frame verloor."
//
// Wat hij WEL bewijst, headless en zonder wereld:
//   1. de vier wapenrijen wijzen elk naar een ANDER mesh-asset, en die assets
//      bestaan en laden;
//   2. hun afmetingen liggen in de orde van het wapen dat ze voorstellen, en —
//      belangrijker — ze zijn ONDERLING verschillend genoeg om op een frame uit
//      elkaar te houden;
//   3. de asconventie klopt: de langste as is X, want daar hangt de hele
//      greeprotatie van de C++-kant aan.
//
// Wat hij NIET bewijst: dat er op het scherm iets verandert. Dat kan alleen een
// frame, en dat frame komt uit de wapenproef in de opnameronde
// (`wapen_D_los_aan_hand` naast `wapen_E_na_wissel`, plus `wapen_G_BEIDE` als
// tegenproef). Deze test is de goedkope voorwacht: valt hij om, dan is er geen
// enkele reden om een ronde te draaien.
//
// ------------------------------------------------------- WAAROM PUNT 2 ERTOE DOET
// "Elk slot heeft een mesh" is te halen met vier keer hetzelfde mesh, en dan doet
// de wissel nog steeds visueel niets terwijl alles groen staat. Precies dát is de
// bug die hier gerepareerd wordt, dus de assertie moet op het VERSCHIL zitten en
// niet op de aanwezigheid. Een test die de twee verklaringen niet scheidt, is
// geen meting.

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectGlobals.h"

namespace EclipseWeaponVisualTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	struct FExpected
	{
		const TCHAR* Row;
		float MinLengthCm;
		float MaxLengthCm;
	};

	/**
	 * De grenzen komen uit wat het wapen VOORSTELT, niet uit wat er toevallig
	 * geexporteerd werd. Een aanvalsgeweer is 60-130 cm, een pistool 18-50. Zou
	 * iemand de Blender-bron aanpassen tot een geweer van 3 meter, dan hoort dit
	 * rood te worden — de maat bepaalt hoe het ding in de handen van een lichaam
	 * van 1,80 m staat, en dat is geen vrije parameter.
	 */
	const FExpected Expected[] = {
		{ TEXT("AR_Foundry"),      60.0f, 130.0f },
		{ TEXT("SMG_Patch"),       40.0f, 100.0f },
		{ TEXT("DMR_Longsight"),   80.0f, 160.0f },
		{ TEXT("Sidearm_Scrap"),   18.0f,  50.0f },
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponMeshesExistTest,
	"Eclipse.Combat.WapenMeshPerFamilie", EclipseWeaponVisualTest::TestFlags)

bool FEclipseWeaponMeshesExistTest::RunTest(const FString& Parameters)
{
	using namespace EclipseWeaponVisualTest;

	TArray<FString> Names;
	TArray<float> Lengths;

	for (const FExpected& Want : Expected)
	{
		// HETZELFDE PAD DAT DE SPELCODE LOOPT. RefreshWeaponVisual valt terug op
		// deze naamconventie als DT_Weapons geen mesh draagt, dus als deze test een
		// eigen pad zou gebruiken, zou hij groen kunnen staan terwijl de game niets
		// vindt.
		const FString Path = FString::Printf(
			TEXT("/Game/Art/Weapons/SM_Weapon_%s.SM_Weapon_%s"), Want.Row, Want.Row);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
		if (!TestNotNull(*FString::Printf(TEXT("wapenmesh voor '%s' laadt (%s)"), Want.Row, *Path), Mesh))
		{
			continue;
		}

		const FVector Size = Mesh->GetBoundingBox().GetSize();
		const float Longest = static_cast<float>(Size.GetMax());
		AddInfo(FString::Printf(
			TEXT("GEMETEN %s: %.1f x %.1f x %.1f cm, langste as %.1f cm"),
			Want.Row, Size.X, Size.Y, Size.Z, Longest));

		TestTrue(*FString::Printf(TEXT("'%s' is %.0f-%.0f cm lang (gemeten %.1f)"),
			Want.Row, Want.MinLengthCm, Want.MaxLengthCm, Longest),
			Longest >= Want.MinLengthCm && Longest <= Want.MaxLengthCm);

		// DE ASCONVENTIE. De C++-aanhechting rekent erop dat +X de loop is; klopt
		// dat niet, dan hangt het wapen dwars in de hand en dat is op een frame wel
		// te zien maar niet te begrijpen. Hier is het één regel.
		TestTrue(*FString::Printf(TEXT("'%s': de langste as is X (de loop) — X=%.1f Y=%.1f Z=%.1f"),
			Want.Row, Size.X, Size.Y, Size.Z),
			Size.X > Size.Y && Size.X > Size.Z);

		// DE GREEP OP DE OORSPRONG. De bbox hoort de oorsprong te OMSLUITEN; ligt
		// hij er volledig naast, dan is de pivot verschoven en hangt het wapen naast
		// de hand in plaats van erin.
		const FBox Box = Mesh->GetBoundingBox();
		TestTrue(*FString::Printf(TEXT("'%s': de oorsprong ligt IN de greep (bbox X %.1f..%.1f, Z %.1f..%.1f)"),
			Want.Row, Box.Min.X, Box.Max.X, Box.Min.Z, Box.Max.Z),
			Box.Min.X <= 0.0 && Box.Max.X >= 0.0 && Box.Min.Z <= 0.0 && Box.Max.Z >= 0.0);

		Names.Add(Mesh->GetName());
		Lengths.Add(Longest);
	}

	// DE ASSERTIE DIE ERTOE DOET: vier VERSCHILLENDE meshes.
	//
	// Vier keer hetzelfde asset haalt elke test hierboven en laat de wapenwissel
	// nog steeds visueel niets doen. Dat is exact de bug die dit repareert, dus de
	// eis zit op het verschil.
	TArray<FString> Unique = Names;
	Unique.Sort();
	int32 Duplicates = 0;
	for (int32 Index = 1; Index < Unique.Num(); ++Index)
	{
		Duplicates += Unique[Index] == Unique[Index - 1] ? 1 : 0;
	}
	TestEqual(TEXT("elke wapenfamilie heeft zijn EIGEN mesh (geen dubbele)"), Duplicates, 0);
	TestEqual(TEXT("alle vier de wapenfamilies leverden een mesh"),
		Names.Num(), static_cast<int32>(UE_ARRAY_COUNT(Expected)));

	// EN ZE MOETEN OP EEN FRAME UIT ELKAAR TE HOUDEN ZIJN.
	//
	// Twee verschillende assets die allebei 94 cm zijn en op elkaar lijken, maken
	// de wissel nog steeds onleesbaar. De sidearm is de scherpste eis: die hoort
	// duidelijk korter te zijn dan het primaire wapen, want dat is wat je op het
	// scherm ziet gebeuren als je wisselt.
	if (Lengths.Num() == static_cast<int32>(UE_ARRAY_COUNT(Expected)))
	{
		const float Ar = Lengths[0];
		const float Sidearm = Lengths[3];
		AddInfo(FString::Printf(TEXT("GEMETEN wisselcontrast AR->sidearm: %.1f cm -> %.1f cm (factor %.2f)"),
			Ar, Sidearm, Sidearm > 0.0f ? Ar / Sidearm : 0.0f));
		TestTrue(*FString::Printf(
			TEXT("de sidearm is minstens 1,5x korter dan de AR — anders is de wissel op een frame niet te zien (%.1f vs %.1f)"),
			Ar, Sidearm), Ar >= Sidearm * 1.5f);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
