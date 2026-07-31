// DE SCHERMLAAG VAN DE MUNITIEHOEK: luistert hij, of polt hij nog?
//
// Dit is stap 5 van de bouwvolgorde (GDD 14.5: schema -> pure logica -> bedrading +
// events -> debug-UI -> ECHTE UI). De vier stappen eronder zijn gemeten in
// EclipseWeaponStatusFeedTests (de feed en de bus) en EclipseHudReadoutTests (de
// pure opmaakbeslissingen). Wat daar NIET in kan zitten is de laatste meter: neemt
// het WIDGET het feit aan, en verandert er dan werkelijk iets aan wat de speler
// leest?
//
// Vandaar dat deze test op een ECHTE wereld draait, met een echte
// AEclipsePlayerController, een echt UEclipseHitscanWeaponComponent, de echte
// UEclipseEventBusSubsystem en het verscheepte UEclipseMissionHudWidget. Een test
// die de widget-klasse nabouwt, meet zijn eigen kopie.
//
// DE VOLGORDE IS DE OPDRACHT, en hij is niet vrijblijvend:
//
//   CONTROLEPROEF  de teller KAN bewegen — één schot verlaagt het getal op het scherm.
//   DE NUL         zestig getickte frames zonder feit = nul keer tekenen.
//   DE WISSEL      een ander wapen verandert de naam op het scherm, meteen.
//   DE ZWAARSTE    het scherm is nooit stiller dan de bus.
//
// Zonder de controleproef vooraan bewijst de nul niets: een HUD die helemaal niet
// tekent haalt hem ook, en "kapot" en "nooit geprobeerd" zijn dan niet te scheiden.
// Dat is in dit project al vaker de verklaring geweest dan een echte bug.
//
// EN DEZE TEST IS AANTOONBAAR ROOD GEWEEST, want een test die alleen groen heeft
// gestaan is niet te onderscheiden van `return true`. GEMETEN op 2026-08-01: met
// RefreshAmmoReadout tijdelijk terug in NativeTick (de toestand van vóór deze stap)
// viel hij op precies de drie plekken om waar hij over gaat, en op geen andere:
//
//   tekenbeurten in 60 stille frames      0  ->   60
//   tekenbeurten in een burst van 10      10 ->   20
//   tekenbeurten in één herlaadbeurt      20 ->  200   (op 180 frames)
//
// Die derde regel is de scherpste van de drie: 200 tekenbeurten op 180 frames laat
// zien dat het pollen en het luisteren ELKAAR NIET VERVINGEN maar OPTELDEN — precies
// de toestand waarin twee bronnen naar hetzelfde getal wijzen.

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Characters/EclipsePlayerController.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "Combat/EclipseWeaponStatusFeed.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Layout/Geometry.h"
#include "Misc/AutomationTest.h"
#include "Tests/EclipseFeelHarness.h"
#include "UI/EclipseMissionHudWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace EclipseHudAmmoFeedTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/**
	 * Het profiel waarop gemeten wordt. De getallen komen uit
	 * `Tools/create_phase1_content.py` (AR: magazijn 30, herladen 2,2 s, interval
	 * 0,15 s) en zijn niet verzonnen: "30" op het scherm is alleen betekenisvol als
	 * het het magazijn is van een wapen dat werkelijk in het spel zit.
	 *
	 * ReadySeconds 0: de handling-tijd na een wissel is echt gedrag, maar hij zou hier
	 * betekenen dat het eerste schot na een wissel geweigerd wordt en dat de test dan
	 * de handling-poort meet in plaats van de schermlaag.
	 */
	FEclipseWeaponRow TestRow(int32 MagazineSize, const TCHAR* DisplayName)
	{
		FEclipseWeaponRow Row;
		Row.MagazineSize = MagazineSize;
		Row.ReloadSeconds = 2.2f;
		Row.FireInterval = 0.15f;
		Row.ReadySeconds = 0.0f;
		Row.RangeCm = 5000.0f;
		if (DisplayName != nullptr)
		{
			Row.DisplayName = FText::FromString(DisplayName);
		}
		return Row;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudAmmoFollowsTheBusTest,
	"Eclipse.UI.MissionHud.AmmoFollowsTheBus",
	EclipseHudAmmoFeedTest::TestFlags)

bool FEclipseHudAmmoFollowsTheBusTest::RunTest(const FString& Parameters)
{
	using namespace EclipseHudAmmoFeedTest;

	EclipseFeelHarness::FHarness Harness;
	EclipseFeelHarness::FHarness::FOptions Options;
	// 1/60 en niet 1/120: de nul-meting is geformuleerd in FRAMES ("zestig frames
	// zonder verandering"), en dan hoort een frame de framestap van het spel te zijn.
	Options.StepSeconds = 1.0f / 60.0f;
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UEclipseEventBusSubsystem* Bus = Harness.GameInstance != nullptr
		? Harness.GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("De echte event-bus staat in deze wereld"), Bus))
	{
		Harness.Shutdown();
		return false;
	}

	// EEN ONAFHANKELIJKE TELLER OP DEZELFDE BUS, en dat is de kern van de zwaarste
	// falsificatie. Zonder een tweede luisteraar is "de HUD kreeg drie feiten" niet te
	// onderscheiden van "er waren drie feiten": de HUD zou zijn eigen noemer leveren,
	// en dan meet de test niets. Deze abonnee telt wat er ECHT langskwam.
	int32 BusFacts = 0;
	int32 BusMagazine = -1;
	FEclipseEventSubscriptionHandle BusHandle = Bus->Subscribe(
		EclipseTags::Event_Player_WeaponStatusChanged,
		FEclipseEventNativeDelegate::CreateLambda([&BusFacts, &BusMagazine](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseWeaponStatusPayload* Status = Payload.GetPtr<FEclipseWeaponStatusPayload>())
			{
				++BusFacts;
				BusMagazine = Status->AmmoInMagazine;
			}
		}),
		FEclipseWeaponStatusPayload::StaticStruct());

	// Het wapen zoals de game mode het uitdeelt: twee slots. De sidearm krijgt met
	// OPZET geen DisplayName — dat is de degradatiemelding van :207-216, en die hoort
	// bewezen te worden op het SCHERM en niet alleen in een logregel.
	UEclipseHitscanWeaponComponent* Weapon = NewObject<UEclipseHitscanWeaponComponent>(Harness.Body);
	Harness.Body->AddOwnedComponent(Weapon);
	Weapon->RegisterComponent();
	Weapon->ApplyLoadout(
		TestRow(30, TEXT("Foundry AR")), FName(TEXT("AR_Foundry")),
		TestRow(12, nullptr), FName(TEXT("Sidearm_Scrap")));

	auto Cleanup = [&Bus, &BusHandle, &Harness]()
	{
		Bus->Unsubscribe(BusHandle);
		Harness.Shutdown();
	};

	// DE ECHTE WIDGET, gemaakt zoals AEclipsePlayerController::EnterMissionMode hem
	// maakt. TStrongObjectPtr en geen kale pointer: deze test tickt seconden aan
	// wereldtijd, en een widget die niet in een viewport hangt heeft verder niemand
	// die hem vasthoudt.
	TStrongObjectPtr<UEclipseMissionHudWidget> Hud(
		CreateWidget<UEclipseMissionHudWidget>(Harness.Controller, UEclipseMissionHudWidget::StaticClass()));
	if (!TestTrue(TEXT("De missie-HUD kan headless gemaakt worden"), Hud.IsValid()))
	{
		Cleanup();
		return false;
	}

	// AddToViewport kan hier niet (er is geen spelvenster), dus de montage-helft van
	// de levenscyclus wordt met de hand gedraaid. Dat is precies wat de engine doet
	// zodra de widget wél in beeld komt: NativeOnInitialized draaide al bij
	// CreateWidget, NativeConstruct is de montage.
	Hud->NativeConstruct();

	const FGeometry HudGeometry;
	auto TickFrame = [&Harness, &Hud, &HudGeometry, &Options]()
	{
		Harness.Step();
		Hud->NativeTick(HudGeometry, Options.StepSeconds);
	};

	// ---------------------------------------------------------------------------
	// CONTROLEPROEF 0 — staat er bij de montage überhaupt iets?
	// ---------------------------------------------------------------------------
	//
	// Dit is meteen de meting van de MONTAGEVOLGORDE. Het wapen zendt zijn eerste
	// feit uit bij de bezetting en bij het uitrusten; de HUD wordt later gemonteerd,
	// en de bus heeft geen geheugen. Zou RequestInitialWeaponStatus ontbreken, dan
	// staat hier een leeg vakje tot het eerste schot — en juist de eerste frame na de
	// montage is het frame dat een opnameronde vastlegt.
	EclipseFeelHarness::Report(*this, TEXT("feiten die de HUD bij de montage aannam"),
		Hud->GetWeaponStatusEventCount(), TEXT("feiten"));
	TestEqual(TEXT("CONTROLEPROEF: bij de montage staat het volle magazijn op het scherm"),
		Hud->GetMagazineTextOnScreen(), FString(TEXT("30")));
	if (Hud->GetMagazineTextOnScreen() != FString(TEXT("30")))
	{
		// Zonder een teller die kan bewegen bewijst geen enkele meting hieronder iets.
		AddError(TEXT("De munitiehoek toont bij de montage niets; elke nul hieronder zou 'nooit geprobeerd' betekenen in plaats van 'niet gepolt'."));
		Hud->NativeDestruct();
		Cleanup();
		return false;
	}
	TestEqual(TEXT("...met de LEESBARE wapennaam uit de data, niet de rijnaam"),
		Hud->GetWeaponTextOnScreen(), FString(TEXT("Foundry AR")));

	// ---------------------------------------------------------------------------
	// CONTROLEPROEF 1 — één schot verlaagt het getal op het scherm.
	// ---------------------------------------------------------------------------
	//
	// SCHUIN OMHOOG: een schot in de vloer spawnt een inslagspoor, en dat is werk dat
	// niets met deze meting te maken heeft.
	const FVector Muzzle = Harness.Body->GetActorLocation();
	const FVector SkyWard = FVector(1.0f, 0.0f, 0.35f).GetSafeNormal();

	const int32 DrawsBeforeShot = Hud->GetAmmoDrawCount();
	Harness.Idle(0.16f);
	Weapon->Fire(Muzzle, SkyWard, TEXT("HudMeting"));

	// GEEN TICK ERTUSSEN, en dat is met opzet de scherpste vorm van de vraag: het feit
	// gaat synchroon over de bus, dus het scherm hoort te zijn veranderd vóórdat er
	// ook maar één frame verstreken is. Bij het pollen kon dit per constructie niet:
	// daar wachtte het scherm altijd op de volgende NativeTick.
	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten door één schot"),
		Hud->GetAmmoDrawCount() - DrawsBeforeShot, TEXT("beurten"));
	TestEqual(TEXT("CONTROLEPROEF: één schot brengt de weergegeven munitie omlaag"),
		Hud->GetMagazineTextOnScreen(), FString(TEXT("29")));
	TestEqual(TEXT("...en dat kostte precies één tekenbeurt"),
		Hud->GetAmmoDrawCount() - DrawsBeforeShot, 1);

	// ---------------------------------------------------------------------------
	// DE NUL — zestig getickte frames zonder feit = nul keer tekenen.
	// ---------------------------------------------------------------------------
	//
	// Dit is de falsificatie van de omzetting zelf. Zolang RefreshAmmoReadout in
	// NativeTick stond, was dit getal per definitie 60. Er wordt hier ECHT getickt:
	// de wereld draait, de camera blendt, de spreiding vervalt en NativeTick van de
	// HUD loopt elke frame — alleen het wapen heeft niets te melden.
	const int32 DrawsBeforeIdle = Hud->GetAmmoDrawCount();
	const int32 FactsBeforeIdle = Hud->GetWeaponStatusEventCount();
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		TickFrame();
	}
	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten in 60 getickte frames zonder wapenfeit"),
		Hud->GetAmmoDrawCount() - DrawsBeforeIdle, TEXT("beurten"));
	TestEqual(TEXT("60 frames zonder verandering = NUL aanroepen van het tekenpad"),
		Hud->GetAmmoDrawCount() - DrawsBeforeIdle, 0);
	TestEqual(TEXT("...en nul feiten, dus de stilte komt niet van een verzwegen feit"),
		Hud->GetWeaponStatusEventCount() - FactsBeforeIdle, 0);
	// EN HET SCHERM IS NIET LEEGGELOPEN. Een HUD die stopt met tekenen mag niet
	// stiekem ook stoppen met TONEN — dat zou de nul halen en de speler alles kosten.
	TestEqual(TEXT("...en het getal staat er na die zestig frames nog steeds"),
		Hud->GetMagazineTextOnScreen(), FString(TEXT("29")));

	// ---------------------------------------------------------------------------
	// DE WISSEL — een ander wapen verandert de naam op het scherm, meteen.
	// ---------------------------------------------------------------------------
	//
	// En tegelijk de degradatiemelding: de sidearm-rij heeft geen DisplayName, dus de
	// speler hoort de OPGEPOETSTE rijnaam te zien ("Sidearm Scrap") en nooit de ruwe
	// sleutel met underscore. Dat `Sidearm_Scrap` stond op 31-07 twee dagen op het
	// scherm; deze regel is de reden dat dat niet stil kan terugkomen.
	const int32 DrawsBeforeSwap = Hud->GetAmmoDrawCount();
	TestTrue(TEXT("Er valt te wisselen"), Weapon->SwapWeapon());
	TestEqual(TEXT("De wapennaam op het scherm volgt de wissel binnen hetzelfde frame"),
		Hud->GetWeaponTextOnScreen(), FString(TEXT("Sidearm Scrap")));
	TestEqual(TEXT("...en het magazijn van het NIEUWE slot staat er"),
		Hud->GetMagazineTextOnScreen(), FString(TEXT("12")));
	TestEqual(TEXT("...voor precies één tekenbeurt"), Hud->GetAmmoDrawCount() - DrawsBeforeSwap, 1);

	// Terug naar het primaire wapen: een wissel die één kant op werkt is een halve
	// wissel, en het magazijn van slot 0 hoort nog steeds op 29 te staan.
	TestTrue(TEXT("En terug"), Weapon->SwapWeapon());
	TestEqual(TEXT("Het wegstoppen bewaart het magazijn: slot 0 staat nog op 29"),
		Hud->GetMagazineTextOnScreen(), FString(TEXT("29")));
	TestEqual(TEXT("...en de leesbare naam is terug"),
		Hud->GetWeaponTextOnScreen(), FString(TEXT("Foundry AR")));

	// ---------------------------------------------------------------------------
	// DE ZWAARSTE — het scherm mag nooit stiller zijn dan de bus.
	// ---------------------------------------------------------------------------
	//
	// Een teller die achterloopt ziet er op een frame volstrekt gezond uit: er staat
	// een getal, het is alleen het verkeerde. Dat is erger dan pollen, want pollen kon
	// deze fout per constructie niet maken. Hier wordt een burst van tien schoten
	// afgevuurd en daarna liggen drie getallen naast elkaar: wat de bus droeg, wat de
	// HUD aannam, en wat er op het scherm staat.
	BusFacts = 0;
	const int32 FactsBeforeBurst = Hud->GetWeaponStatusEventCount();
	const int32 DrawsBeforeBurst = Hud->GetAmmoDrawCount();
	for (int32 Shot = 0; Shot < 10; ++Shot)
	{
		// De cadanspoort staat op 0,15 s; wie sneller aanbiedt, meet de poort.
		Harness.Idle(0.16f);
		Weapon->Fire(Muzzle, SkyWard, TEXT("HudMeting"));
		Hud->NativeTick(HudGeometry, Options.StepSeconds);
	}

	EclipseFeelHarness::Report(*this, TEXT("feiten op de bus tijdens de burst"), BusFacts, TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("feiten aangenomen door de HUD"),
		Hud->GetWeaponStatusEventCount() - FactsBeforeBurst, TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten van de HUD"),
		Hud->GetAmmoDrawCount() - DrawsBeforeBurst, TEXT("beurten"));

	TestEqual(TEXT("Tien schoten gaven tien feiten op de bus"), BusFacts, 10);
	TestEqual(TEXT("De HUD nam ELK feit aan — niet stiller dan de bus"),
		Hud->GetWeaponStatusEventCount() - FactsBeforeBurst, BusFacts);
	TestEqual(TEXT("...en tekende voor elk feit — niet stiller dan zijn eigen postvak"),
		Hud->GetAmmoDrawCount() - DrawsBeforeBurst, BusFacts);
	TestEqual(TEXT("Het getal op het scherm is het getal in het wapen"),
		Hud->GetMagazineTextOnScreen(), FString::Printf(TEXT("%d"), Weapon->GetAmmoInMagazine()));
	TestEqual(TEXT("...en dat is ook het getal dat het laatste buspakket droeg"),
		Hud->GetMagazineTextOnScreen(), FString::Printf(TEXT("%d"), BusMagazine));

	// ---------------------------------------------------------------------------
	// DE HERLAADBEURT — het enige moment waarop de bus wél doorloopt.
	// ---------------------------------------------------------------------------
	//
	// Voortgang is geen gebeurtenis maar een klok, en de feed kwantiseert hem op 5 %.
	// Twee dingen horen hier waar te zijn en ze spreken elkaar bijna tegen: het scherm
	// moet MEEBEWEGEN (anders staat de balk stil) en het mag GEEN framestroom worden
	// (anders is het pollen met extra stappen).
	//
	// En de reparatie van defect 1 erbij: het magazijngetal mag tijdens de beurt niet
	// verdwijnen. Dat was de bug waarbij "RELOADING" het hele veld overschreef, dus
	// precies wanneer je wilt weten hoeveel er straks in zit, stond het er niet.
	const int32 DrawsBeforeReload = Hud->GetAmmoDrawCount();
	TestTrue(TEXT("De herlaadbeurt start"), Weapon->StartReload(TEXT("HudMeting")));

	int32 ReloadFrames = 0;
	FString MidReloadText;
	// 3,0 s tegen een beurt van 2,2 s: ruim voorbij het eind, zodat het EINDfeit ook
	// echt in het venster valt.
	const int32 Frames = FMath::RoundToInt(3.0f / Options.StepSeconds);
	for (int32 Index = 0; Index < Frames; ++Index)
	{
		TickFrame();
		++ReloadFrames;
		if (Index == Frames / 4)
		{
			MidReloadText = Hud->GetMagazineTextOnScreen();
		}
	}

	const int32 ReloadDraws = Hud->GetAmmoDrawCount() - DrawsBeforeReload;
	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten in één herlaadbeurt"), ReloadDraws, TEXT("beurten"));
	EclipseFeelHarness::Report(*this, TEXT("getickte frames in datzelfde venster"), ReloadFrames, TEXT("frames"));
	TestTrue(TEXT("De balk beweegt: een herlaadbeurt levert echte tekenbeurten op"), ReloadDraws > 1);
	TestTrue(TEXT("...maar het is geen framestroom: ruim minder beurten dan frames"),
		ReloadDraws < ReloadFrames / 2);
	TestFalse(TEXT("Defect 1 blijft weg: het magazijngetal verdwijnt niet tijdens de beurt"),
		MidReloadText.IsEmpty());
	TestEqual(TEXT("Na de beurt staat het volle magazijn op het scherm"),
		Hud->GetMagazineTextOnScreen(), FString(TEXT("30")));

	Hud->NativeDestruct();
	Cleanup();
	return true;
}

// ---------------------------------------------------------------------------
// EN DE VRAAG DIE ERONDER LIGT: kan een consument die TE LAAT aankomt de stand nog
// krijgen? De bus levert synchroon af en bewaart niets, dus dit is geen theorie —
// het is de volgorde waarin het spel werkelijk opstart.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponStatusResendTest,
	"Eclipse.Combat.WeaponStatus.ResendForLateConsumer",
	EclipseHudAmmoFeedTest::TestFlags)

bool FEclipseWeaponStatusResendTest::RunTest(const FString& Parameters)
{
	using namespace EclipseWeaponStatusFeed;

	FEclipseWeaponStatusTracker Tracker;
	FEclipseWeaponSnapshot Snapshot;
	Snapshot.AmmoInMagazine = 30;
	Snapshot.MagazineSize = 30;
	Snapshot.WeaponRowName = FName(TEXT("AR_Foundry"));

	TestTrue(TEXT("Het eerste monster gaat de deur uit"), Tracker.Submit(Snapshot).bShouldBroadcast);
	// CONTROLEPROEF: zonder vergeten zwijgt hij, en dat IS het probleem waar
	// ForgetLastBroadcast voor bestaat. Zonder deze regel zou de test hieronder
	// evengoed slagen op een tracker die altijd uitzendt.
	TestFalse(TEXT("CONTROLEPROEF: hetzelfde monster nogmaals levert niets op"),
		Tracker.Submit(Snapshot).bShouldBroadcast);

	Tracker.ForgetLastBroadcast();
	const FEclipseWeaponStatusDecision Resend = Tracker.Submit(Snapshot);
	TestTrue(TEXT("Na ForgetLastBroadcast gaat dezelfde stand opnieuw de deur uit"), Resend.bShouldBroadcast);
	TestTrue(TEXT("...als FOTO en niet als verandering, dus met bInitial"), Resend.Payload.bInitial);
	TestFalse(TEXT("...en dus zonder munitie-verandervlag"), Resend.Payload.bAmmoChanged);
	TestEqual(TEXT("...met de stand erin"), Resend.Payload.AmmoInMagazine, 30);

	// EN DE UITZENDTELLER OVERLEEFT HET. Dat is het enige verschil met Reset(), en het
	// is geen detail: die teller is het meetinstrument waarmee een stille HUD te
	// bisecteren valt (poort / feed / bus). Een montage die het bewijsmateriaal
	// uitgumt, breekt precies het onderzoek dat je daarna zou willen doen.
	// Twee en niet drie: het tweede monster was identiek en ging dus NIET de deur uit
	// (dat is de controleproef hierboven). Wat telt is dat het vergeten de teller niet
	// terugzet — na de resend staat hij op 2, niet op 1 en zeker niet op 0.
	TestEqual(TEXT("De uitzendteller telt door over het vergeten heen"), Tracker.GetBroadcastCount(), 2);
	Tracker.Reset();
	TestEqual(TEXT("Reset() wist hem WEL — dat is het verschil tussen de twee"),
		Tracker.GetBroadcastCount(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
