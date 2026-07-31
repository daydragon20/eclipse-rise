// DE METING DIE HET INSLAGSPOOR-DOSSIER MIST (DEBUG_DISCIPLINE.md §4.3).
//
// §4.3 concludeert "transform-bug, geen rendering-bug". Die conclusie leunt op één
// controleproef: een magenta blok dat WEL verscheen, VASTGEMAAKT AAN HET PERSONAGE
// en neergezet bij BeginPlay. Een object dat per constructie bij het personage
// staat, kan niets zeggen over waar een GESPAWND spoor terechtkomt — het is
// letterlijk de enige plek waar het kán staan. De proef kan de conclusie dus niet
// dragen, en het bewijs in hetzelfde bestand wijst de andere kant op: de echte
// sporen stonden gemeten op 8,4-8,5 m VÓÓR de camera, met een positieve
// dot-product, niet bij de schutter.
//
// En het gat waar dat allemaal op rust: de logregel in SpawnImpactMark logde tot
// 31-07 alleen `Spot` — de plek waar het spoor NAARTOE werd gestuurd. Waar het
// daarna staat, `Mark->GetActorLocation()`, is in dit dossier nog nooit gemeten.
//
// Deze test doet die meting, headless, twintig schoten lang, en houdt twee eisen
// TEGELIJK aan per schot:
//
//   1. het spoor staat <= 1 cm van de inslagplek uit de trace, EN
//   2. het spoor staat >= 100 cm van de schutter.
//
// Alleen samen scheiden ze de twee overgebleven verklaringen. Eis 1 alleen laat
// "hij landt bij de schutter EN toevallig ook op de inslag" bestaan bij een schot
// van dichtbij; eis 2 alleen laat "hij landt ergens anders, maar in elk geval niet
// op de schutter" bestaan. Een transform-bug (lokale ruimte, niet-losgemaakte
// attach, vergeten offset) valt op eis 1 om en meestal ook op eis 2.
//
// ROOD  -> de transform-bug uit §4.3 is BEVESTIGD, en de logregel wijst met
//          `bedoeld` versus `ECHT` meteen aan in welke helft van de keten hij zit.
// GROEN -> §4.3 is WEERLEGD. De sporen landen dan waar de trace ze wil hebben, en
//          het dossier hoort terug naar de renderkant. Dat is winst, geen
//          mislukking: een weerlegde diagnose scheelt een dertiende hypothese.
//
// GEEN FIX IN DEZE ITERATIE — diagnose eerst, met een meting. Precies daarom zijn
// er in dit project al twee reparaties teruggedraaid.

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

namespace EclipseImpactMarkTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** Zoals EclipseBaseTests: een wegwerpwereld, actors alleen, nooit getickt. */
	struct FScopedTestWorld
	{
		FScopedTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		}

		~FScopedTestWorld()
		{
			if (World != nullptr)
			{
				World->DestroyWorld(/*bBroadcastWorldDestroyedEvent*/ false);
			}
		}

		UWorld* World = nullptr;
	};

	/** Eén schot: waar de schutter staat, waar de trace raakte, en hoe het oppervlak ligt. */
	struct FShot
	{
		FVector Shooter = FVector::ZeroVector;
		FVector Impact = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		double DistanceCm = 0.0;
		FString Wat;
	};

	/**
	 * TWINTIG VERSCHILLENDE SCHOTEN, en dat is geen decoratie: twintig keer hetzelfde
	 * schot is één meting. Gevarieerd over
	 *
	 *   - AFSTAND      250 cm tot 3955 cm (kort bereik tot bijna de DMR-grens),
	 *   - HOEK         de volle 360 graden yaw in stappen van 18, pitch -35..+35,
	 *   - OPPERVLAK    afwisselend een vloernormaal (+Z), een muurnormaal die naar de
	 *                  schutter wijst, en een scheve normaal — want de spawnrotatie
	 *                  komt uit MakeFromZ(ImpactNormal) en een normaal die met de
	 *                  schotrichting meebeweegt zou een fout kunnen maskeren,
	 *   - SCHUTTERPLEK de schutter staat NOOIT op de oorsprong en verschuift per
	 *                  schot. Dat is de belangrijkste variatie van de vier: staat de
	 *                  schutter op (0,0,0), dan zijn wereldruimte en lokale ruimte
	 *                  identiek en is precies de bug die §4.3 vermoedt onzichtbaar.
	 *                  De basis ligt op X=-7900 omdat dat de orde van grootte is waar
	 *                  de wijk werkelijk staat.
	 */
	TArray<FShot> MakeShots()
	{
		TArray<FShot> Shots;
		Shots.Reserve(20);
		for (int32 Index = 0; Index < 20; ++Index)
		{
			FShot& Shot = Shots.AddDefaulted_GetRef();
			Shot.Shooter = FVector(-7900.0 + Index * 137.0, 2300.0 - Index * 211.0, 92.0 + (Index % 3) * 40.0);

			const double Yaw = Index * 18.0;
			const double Pitch = -35.0 + (Index % 5) * 17.5;
			const FVector Direction = FRotator(Pitch, Yaw, 0.0).Vector().GetSafeNormal();

			Shot.DistanceCm = 250.0 + Index * 195.0;
			Shot.Impact = Shot.Shooter + Direction * Shot.DistanceCm;

			switch (Index % 3)
			{
			case 0:
				Shot.Normal = FVector::UpVector; // de grond
				Shot.Wat = TEXT("vloer");
				break;
			case 1:
				Shot.Normal = -Direction; // een muur die de schutter aankijkt
				Shot.Wat = TEXT("muur");
				break;
			default:
				Shot.Normal = (-Direction + FVector(0.0, 0.0, 0.6)).GetSafeNormal(); // scheve dekking
				Shot.Wat = TEXT("schuin");
				break;
			}
		}
		return Shots;
	}

	/** Een actor met een root, zodat GetActorLocation iets betekent. */
	AStaticMeshActor* SpawnBody(UWorld& World, const FVector& Where, FName Tag)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Body = World.SpawnActor<AStaticMeshActor>(Where, FRotator::ZeroRotator, Params);
		if (Body != nullptr)
		{
			Body->Tags.Add(Tag);
		}
		return Body;
	}

	/**
	 * Het verse spoor zoeken op zijn tag — niet "de laatste actor in de lijst", want
	 * die aanname is precies zo hard als de aanname dat een blok bij het personage
	 * iets over spawnen zegt.
	 *
	 * Eén functie voor de twintig schoten EN voor de controleproef eronder: een
	 * controleproef die langs een ánder pad meet dan de meting zelf, bewijst niets
	 * over die meting.
	 */
	AStaticMeshActor* FindFreshMark(UWorld& World, TSet<TObjectKey<AActor>>& Bekend, int32& OutFound)
	{
		AStaticMeshActor* Mark = nullptr;
		OutFound = 0;
		for (TActorIterator<AStaticMeshActor> It(&World); It; ++It)
		{
			AStaticMeshActor* Kandidaat = *It;
			if (Kandidaat == nullptr || !Kandidaat->Tags.Contains(TEXT("Eclipse_ImpactMark")) || Bekend.Contains(Kandidaat))
			{
				continue;
			}
			Bekend.Add(Kandidaat);
			++OutFound;
			Mark = Kandidaat;
		}
		return Mark;
	}

	/** De twee eisen als één plek, zodat de controleproef ze niet kan navertellen. */
	bool LandtOpDeInslag(double NaarInslagCm)
	{
		// 1,0 cm is de bewuste lift langs de normaal in SpawnImpactMark (tegen
		// z-fighting); de 0,01 cm eronder is drijvende-kommaruis en geen speelruimte
		// voor een bug. Een transform in de verkeerde ruimte scheelt meters.
		return NaarInslagCm <= 1.0 + 0.01;
	}

	bool StaatVanDeSchutter(double NaarSchutterCm)
	{
		return NaarSchutterCm >= 100.0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseImpactMarkLandsOnTheHitTest,
	"Eclipse.Combat.ImpactMarkLandsOnTheHitAndNotOnTheShooter",
	EclipseImpactMarkTest::TestFlags)

bool FEclipseImpactMarkLandsOnTheHitTest::RunTest(const FString& Parameters)
{
	using namespace EclipseImpactMarkTest;

	FScopedTestWorld Scoped;
	if (!TestNotNull(TEXT("er is een wereld om in te spawnen"), Scoped.World))
	{
		return false;
	}
	UWorld& World = *Scoped.World;

	// SpawnImpactMark leest de eigenaar niet, maar de nieuwe logregel wel — en een
	// eigenaar op een echte plek is precies wat een lokale-ruimte-fout zichtbaar
	// zou maken. Per schot een verse schutter en niet één die meeloopt: een
	// AStaticMeshActor heeft statische mobiliteit en weigert ná registratie te
	// bewegen, dus een verplaatste schutter zou stilletjes blijven staan waar hij
	// stond. Dat is precies de klasse fout die dit dossier al twee keer heeft
	// gemaakt: meten aan iets dat niet is wat je denkt.
	const TArray<FShot> Shots = MakeShots();
	TSet<TObjectKey<AActor>> Bekend;
	TArray<AStaticMeshActor*> Marks;
	Marks.SetNumZeroed(20);

	double SlechtsteAfwijkingCm = 0.0;
	double KleinsteSchutterAfstandCm = TNumericLimits<double>::Max();
	int32 GeenSpoor = 0;
	int32 OpDeInslag = 0;
	int32 VanDeSchutter = 0;

	for (int32 Index = 0; Index < Shots.Num(); ++Index)
	{
		const FShot& Shot = Shots[Index];

		AStaticMeshActor* ShooterBody = SpawnBody(World, Shot.Shooter, TEXT("Eclipse_TestSchutter"));
		AStaticMeshActor* Wall = SpawnBody(World, Shot.Impact, TEXT("Eclipse_TestOppervlak"));
		if (!TestNotNull(TEXT("de schutter staat er"), ShooterBody) || !TestNotNull(TEXT("het oppervlak staat er"), Wall))
		{
			return false;
		}
		Bekend.Add(ShooterBody);
		Bekend.Add(Wall);

		UEclipseHitscanWeaponComponent* Weapon = NewObject<UEclipseHitscanWeaponComponent>(ShooterBody);
		Weapon->RegisterComponent();

		// DE OPSTELLING MOET DE TWEE EISEN KUNNEN LATEN BOTSEN. Ligt de inslagplek
		// zelf binnen 100 cm van de schutter, dan is er geen schot dat beide eisen
		// tegelijk haalt en meet de test zichzelf in plaats van de code.
		const double OpstellingCm = FVector::Dist(Shot.Impact, Shot.Shooter);
		if (!TestTrue(FString::Printf(TEXT("schot %d: de opstelling zet de inslag ver genoeg van de schutter (%.0f cm)"),
				Index + 1, OpstellingCm), OpstellingCm >= 200.0))
		{
			return false;
		}

		FHitResult Hit;
		Hit.bBlockingHit = true;
		Hit.Location = Shot.Impact;
		Hit.ImpactPoint = Shot.Impact;
		Hit.Normal = Shot.Normal;
		Hit.ImpactNormal = Shot.Normal;
		Hit.TraceStart = Shot.Shooter;
		Hit.TraceEnd = Shot.Impact;
		Hit.Distance = static_cast<float>(OpstellingCm);
		Hit.HitObjectHandle = FActorInstanceHandle(Wall);

		Weapon->SpawnImpactMark(World, Hit);

		int32 NieuweSporen = 0;
		AStaticMeshActor* Mark = FindFreshMark(World, Bekend, NieuweSporen);

		if (Mark == nullptr)
		{
			++GeenSpoor;
			AddError(FString::Printf(
				TEXT("GEMETEN  schot %2d (%s, %.0f cm): ER ONTSTOND GEEN SPOOR — SpawnImpactMark viel voortijdig terug."),
				Index + 1, *Shot.Wat, Shot.DistanceCm));
			continue;
		}
		TestEqual(FString::Printf(TEXT("schot %d: precies één nieuw spoor"), Index + 1), NieuweSporen, 1);
		Marks[Index] = Mark;

		const FVector Landed = Mark->GetActorLocation();
		const double NaarInslagCm = FVector::Dist(Landed, Shot.Impact);
		const double NaarSchutterCm = FVector::Dist(Landed, Shot.Shooter);
		// De bedoelde plek is de inslag plus één centimeter langs de normaal (die
		// lift staat in SpawnImpactMark om z-fighting te vermijden). Dit getal
		// scheidt "de transform klopt" van "de lift klopt": een spawnfout meet in
		// meters, een lift-fout in centimeters.
		const double NaarBedoeldCm = FVector::Dist(Landed, Shot.Impact + Shot.Normal * 1.0);

		SlechtsteAfwijkingCm = FMath::Max(SlechtsteAfwijkingCm, NaarInslagCm);
		KleinsteSchutterAfstandCm = FMath::Min(KleinsteSchutterAfstandCm, NaarSchutterCm);

		AddInfo(FString::Printf(
			TEXT("GEMETEN  schot %2d %-7s %6.0f cm ver  ->  spoor %.3f cm van de inslag, %.3f cm van de bedoelde plek, %8.1f cm van de schutter"),
			Index + 1, *Shot.Wat, Shot.DistanceCm, NaarInslagCm, NaarBedoeldCm, NaarSchutterCm));

		// EIS 1 — op de inslagplek uit de trace.
		const bool bOpDeInslag = LandtOpDeInslag(NaarInslagCm);
		// EIS 2 — niet bij de schutter. Dit is de eis die de controleproef van §4.3
		// nooit heeft kunnen stellen, want dat blok zat aan het personage vast.
		const bool bVanDeSchutter = StaatVanDeSchutter(NaarSchutterCm);
		OpDeInslag += bOpDeInslag ? 1 : 0;
		VanDeSchutter += bVanDeSchutter ? 1 : 0;

		TestTrue(FString::Printf(
			TEXT("schot %d (%s, %.0f cm): het spoor staat <= 1 cm van de inslagplek (gemeten %.3f cm)"),
			Index + 1, *Shot.Wat, Shot.DistanceCm, NaarInslagCm), bOpDeInslag);
		TestTrue(FString::Printf(
			TEXT("schot %d (%s, %.0f cm): het spoor staat >= 100 cm van de schutter (gemeten %.1f cm)"),
			Index + 1, *Shot.Wat, Shot.DistanceCm, NaarSchutterCm), bVanDeSchutter);
	}

	// ===== STAAN ZE ER NOG, NA ALLE TWINTIG? =======================================
	//
	// Hierboven wordt gemeten op het moment dat SpawnImpactMark terugkeert. "Er is
	// niets dat de actor daarna verplaatst" is beredeneerd (geen attach, geen tick,
	// collision uit) — en beredeneerd is in dit dossier precies wat drie keer is
	// teruggenomen. Dus wordt het ook gemeten: na negentien verdere spawns worden
	// alle sporen opnieuw nagelopen. Dit is geen frame-tijd (deze wereld tickt
	// nooit) en dus geen bewijs over de volgende frame; het sluit alleen uit dat een
	// LATERE spawn een eerder spoor meesleept.
	double GrootsteVerloopCm = 0.0;
	int32 NogAanwezig = 0;
	for (int32 Index = 0; Index < Marks.Num(); ++Index)
	{
		if (Marks[Index] == nullptr)
		{
			continue;
		}
		++NogAanwezig;
		const double NuCm = FVector::Dist(Marks[Index]->GetActorLocation(), Shots[Index].Impact);
		GrootsteVerloopCm = FMath::Max(GrootsteVerloopCm, NuCm);
		TestTrue(FString::Printf(
			TEXT("schot %d: het spoor staat ná alle twintig schoten nog steeds <= 1 cm van zijn inslagplek (gemeten %.3f cm)"),
			Index + 1, NuCm), LandtOpDeInslag(NuCm));
	}
	AddInfo(FString::Printf(TEXT("GEMETEN  sporen nog aanwezig na 20 schoten        %d/%d"), NogAanwezig, Shots.Num()));
	AddInfo(FString::Printf(TEXT("GEMETEN  slechtste afwijking ná alle 20           %.3f cm"), GrootsteVerloopCm));

	// ================= DE CONTROLEPROEF: KAN DEZE METING ÜBERHAUPT ROOD WORDEN =====
	//
	// Twintig groene schoten zeggen niets zolang niet vaststaat dat de meting een
	// verkeerd geplaatst spoor zóú afkeuren. Dat is de fout die dit dossier al twee
	// keer heeft gemaakt: een proef die per constructie niet kan mislukken werd als
	// bewijs geteld (het magenta blok dat aan het personage vastzat), en een filter
	// dat alleen op Error keek gooide 122 meetregels weg.
	//
	// Dus: hier wordt met de hand precies het spoor neergezet dat §4.3 vermoedt — op
	// de schutter in plaats van op de inslag — en langs HETZELFDE zoek- en meetpad
	// gehaald. De eisen horen het af te keuren. Doen ze dat niet, dan meet de test
	// hierboven niets en is elke groene uitslag waardeloos.
	if (Shots.Num() > 0)
	{
		const FShot& Proef = Shots[0];
		AStaticMeshActor* Nep = SpawnBody(World, Proef.Shooter, TEXT("Eclipse_ImpactMark"));
		if (TestNotNull(TEXT("controleproef: het nepspoor staat er"), Nep))
		{
			int32 Gevonden = 0;
			AStaticMeshActor* Gepakt = FindFreshMark(World, Bekend, Gevonden);
			TestEqual(TEXT("controleproef: de zoeker vindt het nepspoor"), Gevonden, 1);
			TestTrue(TEXT("controleproef: de zoeker pakt hetzelfde nepspoor"), Gepakt == Nep);

			if (Gepakt != nullptr)
			{
				const FVector NepLanded = Gepakt->GetActorLocation();
				const double NepNaarInslagCm = FVector::Dist(NepLanded, Proef.Impact);
				const double NepNaarSchutterCm = FVector::Dist(NepLanded, Proef.Shooter);
				AddInfo(FString::Printf(
					TEXT("GEMETEN  controleproef (spoor OP de schutter)     %.1f cm van de inslag, %.1f cm van de schutter"),
					NepNaarInslagCm, NepNaarSchutterCm));

				TestFalse(TEXT("controleproef: eis 1 KEURT een spoor op de schutter AF"),
					LandtOpDeInslag(NepNaarInslagCm));
				TestFalse(TEXT("controleproef: eis 2 KEURT een spoor op de schutter AF"),
					StaatVanDeSchutter(NepNaarSchutterCm));
			}
		}
	}

	AddInfo(FString::Printf(TEXT("GEMETEN  schoten                                  %d"), Shots.Num()));
	AddInfo(FString::Printf(TEXT("GEMETEN  schoten zonder spoor                     %d"), GeenSpoor));
	AddInfo(FString::Printf(TEXT("GEMETEN  spoor <= 1 cm van de inslag              %d/%d"), OpDeInslag, Shots.Num()));
	AddInfo(FString::Printf(TEXT("GEMETEN  spoor >= 100 cm van de schutter          %d/%d"), VanDeSchutter, Shots.Num()));
	AddInfo(FString::Printf(TEXT("GEMETEN  slechtste afwijking van de inslag        %.3f cm"), SlechtsteAfwijkingCm));
	AddInfo(FString::Printf(TEXT("GEMETEN  kleinste afstand tot de schutter         %.1f cm"),
		KleinsteSchutterAfstandCm == TNumericLimits<double>::Max() ? -1.0 : KleinsteSchutterAfstandCm));

	// De samenvatting ook in het log, want deze meting hoort vindbaar te zijn zonder
	// het testrapport erbij te pakken — dat rapport is eerder weggegooid door een
	// filter dat alleen op Error keek.
	UE_LOG(LogTemp, Display,
		TEXT("[INSLAGMETING] %d schoten: %d/%d <= 1 cm van de inslag, %d/%d >= 100 cm van de schutter, slechtste afwijking %.3f cm, dichtste bij de schutter %.1f cm, %d zonder spoor."),
		Shots.Num(), OpDeInslag, Shots.Num(), VanDeSchutter, Shots.Num(), SlechtsteAfwijkingCm,
		KleinsteSchutterAfstandCm == TNumericLimits<double>::Max() ? -1.0 : KleinsteSchutterAfstandCm, GeenSpoor);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
