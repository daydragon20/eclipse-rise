// DE METING BIJ DEFECT 2: "HERLADEN blijft staan tot je opnieuw de trekker overhaalt."
//
// De melding komt van de schermlaag, maar de vraag is een GEDRAGSVRAAG en hoort dus
// op het draaiende systeem gemeten te worden en niet uit de code afgeleid. Wat deze
// test doet is precies één ding: een herlaadbeurt starten, de wereld dóór laten
// lopen, en op vier momenten opschrijven wat het wapen zegt dat het is.
//
// WAAROM DIT EEN ECHTE, TIKKENDE WERELD NODIG HEEFT. Het component tikt zelf niet
// (PrimaryComponentTick.bCanEverTick = false) en de herlaadbeurt loopt af "op de
// klok". In een wereld die nooit tikt staat World->GetTimeSeconds() eeuwig op 0, en
// dan meet je dat er niets gebeurt terwijl er niets kán gebeuren — een meting die
// de twee verklaringen niet scheidt. Vandaar een standalone GameInstance-wereld die
// echt getickt wordt, net als in EclipseFeelHarness.
//
// DE CONTROLEPROEF STAAT ERBIJ EN GAAT VOOROP (owner-regel "bewijs eerst dat het
// daar ooit kan gebeuren"): dezelfde meetlus moet de GEZONDE toestand ook kunnen
// zien. Daarom meet de eerste helft van de reeks binnen de herlaadtijd, waar
// IsReloading() true HOORT te zijn. Zegt het instrument daar al "niet aan het
// herladen", dan meet het niets en zegt een groene tweede helft ook niets.
//
// GEMETEN op 2026-07-31 tegen de code van dat moment, herlaadtijd 2,2 s:
//   t = 0.50 s -> herladen=1  munitie=29   (binnen de beurt; hoort zo)
//   t = 2.20 s -> herladen=1  munitie=29   (precies op de eindtijd)
//   t = 3.00 s -> herladen=1  munitie=29   (0,8 s ERNA — hoort al klaar te zijn)
//   t = 6.00 s -> herladen=1  munitie=29   (3,8 s erna — nog steeds bezig)
// De beurt eindigt dus NOOIT uit zichzelf. Oorzaak, benoemd en niet gegokt:
// FinishReload() had precies één aanroeper, de herlaadtak bovenin Fire(). Zonder
// trekker komt die tak nooit langs, dus blijft bReloading true en blijft het
// magazijn op zijn oude stand staan.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCharacterTypes.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

namespace EclipseWeaponReloadTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::ProductFilter;

	/** 1/60 s: fijn genoeg om een eindtijd van 2,2 s op ~17 ms te lokaliseren. */
	constexpr float StepSeconds = 1.0f / 60.0f;

	/**
	 * Een wereld die ECHT TIKT. Dezelfde route als EclipseFeelHarness::Start, tot op
	 * het ophogen van GFrameCounter: FTickFunction::QueueTickFunction onthoudt per
	 * tick-functie in welk frame hij al bezocht is en slaat hem daarna over, dus
	 * zonder die regel tikt na de eerste ronde niets meer en staat de wereldklok
	 * stil zonder dat iets dat meldt.
	 */
	struct FTickingWorld
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;

		bool Start()
		{
			GameInstance = NewObject<UGameInstance>(GEngine);
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			if (World == nullptr)
			{
				return false;
			}
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			if (AWorldSettings* Settings = World->GetWorldSettings())
			{
				Settings->NotifyBeginPlay();
			}
			return true;
		}

		void Advance(double Seconds)
		{
			const int32 Steps = FMath::Max(1, FMath::RoundToInt(Seconds / StepSeconds));
			for (int32 Index = 0; Index < Steps; ++Index)
			{
				++GFrameCounter;
				World->Tick(LEVELTICK_All, StepSeconds);
			}
		}

		void Shutdown()
		{
			if (GameInstance != nullptr)
			{
				GameInstance->Shutdown();
				GameInstance = nullptr;
			}
			World = nullptr;
		}
	};

	/** Het profiel waarop gemeten wordt; de getallen komen uit de AR-rij van DT_Weapons. */
	FEclipseWeaponRow MakeRow()
	{
		FEclipseWeaponRow Row;
		Row.MagazineSize = 30;
		Row.ReloadSeconds = 2.2f;
		Row.FireInterval = 0.15f;
		Row.ReadySeconds = 0.0f;
		Row.RangeCm = 5000.0f;
		return Row;
	}
}

/**
 * DE HERLAADBEURT MOET UIT ZICHZELF AFLOPEN — zonder dat er iets geschoten wordt.
 *
 * ROOD  = defect 2 bestaat: de speler ziet "HERLADEN" staan tot hij de trekker
 *         opnieuw overhaalt, en zijn magazijn is intussen niet bijgevuld.
 * GROEN = de beurt sluit zichzelf. De schermlaag volgt dan vanzelf, want die leest
 *         alleen af.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponReloadFinishesOnItsOwnTest,
	"Eclipse.Combat.ReloadFinishesWithoutPullingTheTriggerAgain",
	EclipseWeaponReloadTest::TestFlags)

bool FEclipseWeaponReloadFinishesOnItsOwnTest::RunTest(const FString&)
{
	using namespace EclipseWeaponReloadTest;

	FTickingWorld Scene;
	if (!TestTrue(TEXT("meetopstelling: een wereld die tikt"), Scene.Start()))
	{
		return false;
	}
	ON_SCOPE_EXIT { Scene.Shutdown(); };

	AActor* Holder = Scene.World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("meetopstelling: een drager voor het wapen"), Holder))
	{
		return false;
	}

	UEclipseHitscanWeaponComponent* Weapon = NewObject<UEclipseHitscanWeaponComponent>(Holder);
	Weapon->RegisterComponent();

	const FEclipseWeaponRow Row = MakeRow();
	Weapon->ApplyWeaponRow(Row);

	// EERST DE KLOK LATEN LOPEN. StartReload zet zijn eindtijd op
	// World->GetTimeSeconds() + ReloadSeconds; staat die klok nog op 0, dan valt de
	// eindtijd samen met het begin van de meting en meet de test zijn eigen
	// opstelling. Een halve seconde aanloop haalt dat uit elkaar.
	Scene.Advance(0.5);

	// EEN KOGEL ERUIT, anders weigert StartReload ("vol is vol") en meet de test
	// een beurt die nooit begon. Rechtstreeks via Fire, want dat is het pad dat een
	// speler ook loopt.
	//
	// EN GETELD OP GetShotsFired(), NIET op wat Fire() teruggeeft — dat is een fout
	// die deze test in zijn eerste versie zelf maakte en die precies de vorm heeft
	// waar hij tegen bedoeld is. `Fire()` geeft true op een TREFFER OP EEN PERSONAGE
	// (zo staat het ook in de header), en in deze meetwereld staat geen personage.
	// De assert "het schot ging af" viel dus om terwijl het schot gewoon afging: hij
	// mat de trefkans en niet de trekker. GetShotsFired() telt wat er door de
	// cadanspoort kwam, raak of mis, en dat is precies de vraag hier.
	const int32 ShotsBefore = Weapon->GetShotsFired();
	Weapon->Fire(Holder->GetActorLocation(), FVector::ForwardVector, TEXT("Meting"));
	const int32 ShotsFired = Weapon->GetShotsFired() - ShotsBefore;
	TestEqual(TEXT("meetopstelling: het schot ging af (geteld op de trekker, niet op de treffer)"), ShotsFired, 1);
	TestEqual(TEXT("meetopstelling: er is één kogel uit het magazijn"), Weapon->GetAmmoInMagazine(), Row.MagazineSize - 1);

	const double StartSeconds = Scene.World->GetTimeSeconds();
	if (!TestTrue(TEXT("meetopstelling: de herlaadbeurt start"), Weapon->StartReload(TEXT("Meting"))))
	{
		return false;
	}

	// DE CONTROLEPROEF, EN HIJ GAAT VOOROP. Kan dit instrument "aan het herladen"
	// überhaupt zien? Zegt het hier al nee, dan bewijst een latere "nee" niets.
	Scene.Advance(0.5);
	const bool bMidway = Weapon->IsReloading();
	AddInfo(FString::Printf(TEXT("GEMETEN t=%.2f s (binnen de beurt van %.2f s): herladen=%d munitie=%d"),
		Scene.World->GetTimeSeconds() - StartSeconds, Row.ReloadSeconds, bMidway ? 1 : 0, Weapon->GetAmmoInMagazine()));
	if (!TestTrue(TEXT("CONTROLEPROEF: halverwege de beurt staat het wapen op 'herladen' — het instrument kan het verschil zien"), bMidway))
	{
		return false;
	}

	// En dan de eigenlijke vraag, op drie momenten voorbij de eindtijd. Drie en niet
	// één, want "net te vroeg" en "allang voorbij" zijn twee verschillende uitspraken
	// en een enkel meetpunt kan ze niet uit elkaar houden.
	struct FSample { double AtSeconds; bool bReloading; int32 Ammo; };
	TArray<FSample> Samples;
	for (const double Target : { 2.4, 3.0, 6.0 })
	{
		Scene.Advance(Target - (Scene.World->GetTimeSeconds() - StartSeconds));
		Samples.Add({ Scene.World->GetTimeSeconds() - StartSeconds, Weapon->IsReloading(), Weapon->GetAmmoInMagazine() });
	}

	bool bAllSettled = true;
	for (const FSample& Sample : Samples)
	{
		AddInfo(FString::Printf(TEXT("GEMETEN t=%.2f s (%.2f s NA het einde van de beurt): herladen=%d munitie=%d/%d"),
			Sample.AtSeconds, Sample.AtSeconds - Row.ReloadSeconds,
			Sample.bReloading ? 1 : 0, Sample.Ammo, Row.MagazineSize));
		bAllSettled &= !Sample.bReloading && Sample.Ammo == Row.MagazineSize;
	}

	TestTrue(TEXT("de herlaadbeurt loopt uit zichzelf af, zonder tweede trekkerbeweging, en het magazijn is vol"), bAllSettled);
	return true;
}

/**
 * DE TWEEDE HELFT VAN HETZELFDE GEDRAG: een beurt die afloopt mag niet ALVAST
 * afgelopen zijn. Zonder deze test zou "IsReloading() geeft altijd false" een groene
 * suite opleveren — dezelfde vorm als een teller die altijd het antwoord zegt dat je
 * wilt horen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponReloadBlocksFireUntilItIsDoneTest,
	"Eclipse.Combat.ReloadBlocksFiringUntilItIsDone",
	EclipseWeaponReloadTest::TestFlags)

bool FEclipseWeaponReloadBlocksFireUntilItIsDoneTest::RunTest(const FString&)
{
	using namespace EclipseWeaponReloadTest;

	FTickingWorld Scene;
	if (!TestTrue(TEXT("meetopstelling: een wereld die tikt"), Scene.Start()))
	{
		return false;
	}
	ON_SCOPE_EXIT { Scene.Shutdown(); };

	AActor* Holder = Scene.World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("meetopstelling: een drager voor het wapen"), Holder))
	{
		return false;
	}

	UEclipseHitscanWeaponComponent* Weapon = NewObject<UEclipseHitscanWeaponComponent>(Holder);
	Weapon->RegisterComponent();
	const FEclipseWeaponRow Row = MakeRow();
	Weapon->ApplyWeaponRow(Row);

	Scene.Advance(0.5);
	Weapon->Fire(Holder->GetActorLocation(), FVector::ForwardVector, TEXT("Meting"));
	Scene.Advance(Row.FireInterval * 2.0);
	if (!TestTrue(TEXT("meetopstelling: de herlaadbeurt start"), Weapon->StartReload(TEXT("Meting"))))
	{
		return false;
	}

	// CONTROLEPROEF OP DE TELLER ZELF: staat de teller van "schoten door de poort"
	// al op 1 na dat ene schot hierboven? Zonder dat is "hij ging niet omhoog tijdens
	// het herladen" geen uitspraak — een teller die nooit oploopt haalt die test ook.
	if (!TestEqual(TEXT("CONTROLEPROEF: de schotenteller telt het schot van vóór de beurt"), Weapon->GetShotsFired(), 1))
	{
		return false;
	}

	// Halverwege: de trekker hoort niets te doen en het magazijn hoort NIET vol te
	// zijn. Dat tweede is de scherpe helft — een fix die het magazijn meteen vult en
	// alleen de vlag laat aflopen, zou hier omvallen.
	Scene.Advance(Row.ReloadSeconds * 0.5);
	const int32 ShotsBefore = Weapon->GetShotsFired();
	Weapon->Fire(Holder->GetActorLocation(), FVector::ForwardVector, TEXT("Meting"));
	const int32 ShotsDuringReload = Weapon->GetShotsFired() - ShotsBefore;
	AddInfo(FString::Printf(TEXT("GEMETEN halverwege: schoten door de poort=%d, munitie=%d/%d, herladen=%d"),
		ShotsDuringReload, Weapon->GetAmmoInMagazine(), Row.MagazineSize, Weapon->IsReloading() ? 1 : 0));
	TestEqual(TEXT("halverwege de beurt komt er geen schot door de poort"), ShotsDuringReload, 0);
	TestTrue(TEXT("halverwege de beurt is het magazijn nog NIET bijgevuld"), Weapon->GetAmmoInMagazine() < Row.MagazineSize);
	TestTrue(TEXT("halverwege de beurt staat het wapen op 'herladen'"), Weapon->IsReloading());

	// En na afloop is hij klaar. Zelfde meting, andere kant van de grens.
	Scene.Advance(Row.ReloadSeconds);
	AddInfo(FString::Printf(TEXT("GEMETEN na afloop: munitie=%d/%d, herladen=%d"),
		Weapon->GetAmmoInMagazine(), Row.MagazineSize, Weapon->IsReloading() ? 1 : 0));
	TestFalse(TEXT("na afloop staat het wapen niet meer op 'herladen'"), Weapon->IsReloading());
	TestEqual(TEXT("na afloop is het magazijn vol"), Weapon->GetAmmoInMagazine(), Row.MagazineSize);

	// EN DE TREKKER DOET HET WEER. Dit is de andere helft van de controleproef: zou
	// de poort ná de beurt dicht blijven, dan zou "0 schoten tijdens het herladen"
	// hierboven ook waar zijn geweest omdat er helemaal niet meer geschoten kán
	// worden — en dan meet die assert niets over het herladen.
	const int32 ShotsBeforeAfter = Weapon->GetShotsFired();
	Weapon->Fire(Holder->GetActorLocation(), FVector::ForwardVector, TEXT("Meting"));
	TestEqual(TEXT("CONTROLEPROEF: na de beurt komt er wél weer een schot door de poort"),
		Weapon->GetShotsFired() - ShotsBeforeAfter, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
