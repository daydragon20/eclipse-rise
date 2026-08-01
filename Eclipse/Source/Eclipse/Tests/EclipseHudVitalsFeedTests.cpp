// DE SCHERMLAAG VAN DE GEZONDHEIDSHOEK: luistert hij, of tekent hij per frame?
//
// Stap 5 van de bouwvolgorde (GDD 14.5), en met opzet de tweelingtest van
// EclipseHudAmmoFeedTests. De vier stappen eronder zijn elders gemeten:
// EclipseVitalsFeedTests (de feed en de bus, `4747010`) en EclipseHudReadoutTests
// (de pure opmaakbeslissingen). Wat daar niet in kan zitten is de laatste meter:
// neemt het WIDGET het feit aan, en verandert er dan werkelijk iets aan wat de
// speler leest?
//
// Vandaar een ECHTE wereld met een echte AEclipsePlayerController, een echt
// AEclipseCharacter, de echte UEclipseEventBusSubsystem en het verscheepte
// UEclipseMissionHudWidget. Een test die de widget nabouwt, meet zijn eigen kopie.
//
// DE VOLGORDE IS DE OPDRACHT:
//
//   CONTROLEPROEF  de uitlezing KAN bewegen — 100 -> 60 hp verandert het scherm.
//   DE NUL         zestig getickte frames zonder feit = nul keer tekenen.
//   TWEE TOESTANDEN neergaan en gestabiliseerd worden staan er ANDERS op.
//   DE ZWAARSTE    het scherm is nooit stiller dan de bus.
//
// Zonder de controleproef vooraan bewijst de nul niets: een HUD die helemaal niet
// tekent haalt hem ook, en "kapot" en "nooit geprobeerd" zijn dan niet te scheiden.
// Bij de munitiehoek bleek diezelfde proef beslissend: met het pollen tijdelijk
// terug telden pollen en luisteren OP in plaats van elkaar te vervangen (20 -> 200
// tekenbeurten op 180 frames). Een half geslaagde ombouw ziet er zonder die proef
// uit als een geslaagde.

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipsePlayerController.h"
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

namespace EclipseHudVitalsFeedTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseHudVitalsFollowsTheBusTest,
	"Eclipse.UI.MissionHud.VitalsFollowTheBus",
	EclipseHudVitalsFeedTest::TestFlags)

bool FEclipseHudVitalsFollowsTheBusTest::RunTest(const FString& Parameters)
{
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

	// EEN ONAFHANKELIJKE TELLER OP DEZELFDE BUS. Zonder tweede luisteraar is "de HUD
	// kreeg drie feiten" niet te onderscheiden van "er waren drie feiten": de HUD zou
	// zijn eigen noemer leveren, en dan meet de test niets.
	int32 BusFacts = 0;
	float BusHealth = -1.0f;
	FEclipseEventSubscriptionHandle BusHandle = Bus->Subscribe(
		EclipseTags::Event_Player_VitalsChanged,
		FEclipseEventNativeDelegate::CreateLambda([&BusFacts, &BusHealth](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipsePlayerVitalsPayload* Vitals = Payload.GetPtr<FEclipsePlayerVitalsPayload>())
			{
				++BusFacts;
				BusHealth = Vitals->Health;
			}
		}),
		FEclipsePlayerVitalsPayload::StaticStruct());

	auto Cleanup = [&Bus, &BusHandle, &Harness]()
	{
		Bus->Unsubscribe(BusHandle);
		Harness.Shutdown();
	};

	// Een bekend maximum, zodat "60" op het scherm een betekenis heeft die uit deze
	// test komt en niet uit de tuning van vandaag.
	Harness.Body->InitializeHealth(100.0f);

	// DE ECHTE WIDGET, gemaakt zoals AEclipsePlayerController::EnterMissionMode hem
	// maakt. TStrongObjectPtr en geen kale pointer: deze test tickt wereldtijd, en een
	// widget die niet in een viewport hangt heeft verder niemand die hem vasthoudt.
	TStrongObjectPtr<UEclipseMissionHudWidget> Hud(
		CreateWidget<UEclipseMissionHudWidget>(Harness.Controller, UEclipseMissionHudWidget::StaticClass()));
	if (!TestTrue(TEXT("De missie-HUD kan headless gemaakt worden"), Hud.IsValid()))
	{
		Cleanup();
		return false;
	}
	Hud->NativeConstruct();

	const FGeometry HudGeometry;
	auto TickFrame = [&Harness, &Hud, &HudGeometry, &Options]()
	{
		Harness.Step();
		Hud->NativeTick(HudGeometry, Options.StepSeconds);
	};

	// ---------------------------------------------------------------------------
	// CONTROLEPROEF 0 — de MONTAGEVOLGORDE. Staat er bij montage iets?
	// ---------------------------------------------------------------------------
	//
	// Het lichaam zendt zijn eerste foto bij de bezetting en bij InitializeHealth; de
	// HUD monteert later, en de bus bewaart niets. Zonder RequestInitialVitals staat
	// hier een leeg vakje tot de eerste klap — en juist de eerste frame na de montage
	// is het frame dat een opnameronde vastlegt.
	EclipseFeelHarness::Report(*this, TEXT("vitals-feiten die de HUD bij de montage aannam"),
		Hud->GetVitalsEventCount(), TEXT("feiten"));
	TestEqual(TEXT("CONTROLEPROEF: bij de montage staat de volle gezondheid op het scherm"),
		Hud->GetHealthTextOnScreen(), FString(TEXT("100")));
	if (Hud->GetHealthTextOnScreen() != FString(TEXT("100")))
	{
		// Zonder een uitlezing die kan bewegen bewijst geen enkele meting hieronder iets.
		AddError(TEXT("De gezondheidshoek toont bij de montage niets; elke nul hieronder zou 'nooit geprobeerd' betekenen in plaats van 'niet gepolt'."));
		Hud->NativeDestruct();
		Cleanup();
		return false;
	}
	TestEqual(TEXT("...met de houding erbij"), Hud->GetStanceTextOnScreen(), FString(TEXT("STANDING")));
	TestEqual(TEXT("...en zonder statusmelding, want er is niets aan de hand"),
		Hud->GetVitalsStatusOnScreen(), FString());

	// ---------------------------------------------------------------------------
	// CONTROLEPROEF 1 — 100 -> 60 hp geeft PRECIES ÉÉN tekenbeurt met de nieuwe waarde.
	// ---------------------------------------------------------------------------
	//
	// GEEN TICK ERTUSSEN, met opzet: het feit gaat synchroon over de bus, dus het
	// scherm hoort veranderd te zijn vóórdat er ook maar één frame verstreken is.
	const int32 DrawsBeforeHit = Hud->GetVitalsDrawCount();
	const int32 FactsBeforeHit = Hud->GetVitalsEventCount();
	Harness.Body->ApplyDamage(40.0f, nullptr, FName(TEXT("HudMeting")));

	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten door één klap van 40 hp"),
		Hud->GetVitalsDrawCount() - DrawsBeforeHit, TEXT("beurten"));
	TestEqual(TEXT("CONTROLEPROEF: één klap brengt de weergegeven gezondheid omlaag"),
		Hud->GetHealthTextOnScreen(), FString(TEXT("60")));
	TestEqual(TEXT("...en dat kostte precies één tekenbeurt"),
		Hud->GetVitalsDrawCount() - DrawsBeforeHit, 1);
	TestEqual(TEXT("...op precies één feit"), Hud->GetVitalsEventCount() - FactsBeforeHit, 1);

	// ---------------------------------------------------------------------------
	// DE NUL — zestig getickte frames zonder feit = nul keer tekenen.
	// ---------------------------------------------------------------------------
	//
	// Er wordt hier ECHT getickt: de wereld draait, de camera blendt, NativeTick van
	// de HUD loopt elke frame — alleen het lichaam heeft niets te melden.
	const int32 DrawsBeforeIdle = Hud->GetVitalsDrawCount();
	const int32 FactsBeforeIdle = Hud->GetVitalsEventCount();
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		TickFrame();
	}
	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten in 60 getickte frames zonder vitals-feit"),
		Hud->GetVitalsDrawCount() - DrawsBeforeIdle, TEXT("beurten"));
	TestEqual(TEXT("60 frames zonder verandering = NUL aanroepen van het tekenpad"),
		Hud->GetVitalsDrawCount() - DrawsBeforeIdle, 0);
	TestEqual(TEXT("...en nul feiten, dus de stilte komt niet van een verzwegen feit"),
		Hud->GetVitalsEventCount() - FactsBeforeIdle, 0);
	// EN HET SCHERM IS NIET LEEGGELOPEN. Een HUD die stopt met tekenen mag niet
	// stiekem ook stoppen met TONEN — dat zou de nul halen en de speler alles kosten.
	TestEqual(TEXT("...en het getal staat er na die zestig frames nog steeds"),
		Hud->GetHealthTextOnScreen(), FString(TEXT("60")));

	// ---------------------------------------------------------------------------
	// DE HOUDING — een feit dat GEEN gezondheid is, moet ook aankomen.
	// ---------------------------------------------------------------------------
	//
	// Zonder deze stap zou "de hoek volgt de bus" alleen bewezen zijn voor schade, en
	// een HUD die alleen op bHealthChanged tekent haalt alle bovenstaande metingen.
	const int32 DrawsBeforeCrouch = Hud->GetVitalsDrawCount();
	Harness.Body->Crouch();
	Harness.Step();
	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten door het hurken"),
		Hud->GetVitalsDrawCount() - DrawsBeforeCrouch, TEXT("beurten"));
	TestEqual(TEXT("Hurken staat op het scherm"), Hud->GetStanceTextOnScreen(), FString(TEXT("CROUCHED")));
	TestEqual(TEXT("...en de gezondheid is er niet door veranderd"),
		Hud->GetHealthTextOnScreen(), FString(TEXT("60")));
	Harness.Body->UnCrouch();
	Harness.Step();
	TestEqual(TEXT("En weer terug naar staand"), Hud->GetStanceTextOnScreen(), FString(TEXT("STANDING")));

	// ---------------------------------------------------------------------------
	// TWEE ONDERSCHEIDBARE TOESTANDEN — neergaan en gestabiliseerd worden.
	// ---------------------------------------------------------------------------
	//
	// De scherpste eis van deze stap. Beide gaan gepaard met een extreme
	// gezondheidswaarde; met een balk alleen zijn ze niet te scheiden, terwijl het
	// verschil tussen "ik lig" en "ik sta weer" het enige is waar je op dat moment
	// naar handelt.
	Harness.Body->ApplyDamage(500.0f, nullptr, FName(TEXT("HudMeting")));
	const FString OpHetScherm_Neer = Hud->GetVitalsStatusOnScreen();
	const FString Houding_Neer = Hud->GetStanceTextOnScreen();
	EclipseFeelHarness::Report(*this, TEXT("vitals-feiten na het neergaan"),
		Hud->GetVitalsEventCount(), TEXT("feiten"));
	AddInfo(FString::Printf(TEXT("GEMETEN op het scherm na het neergaan: gezondheid '%s', status '%s', houding '%s'"),
		*Hud->GetHealthTextOnScreen(), *OpHetScherm_Neer, *Houding_Neer));
	TestEqual(TEXT("Neergaan staat er als DOWN"), OpHetScherm_Neer, FString(TEXT("DOWN")));
	TestEqual(TEXT("...de gezondheid staat op nul"), Hud->GetHealthTextOnScreen(), FString(TEXT("0")));
	TestEqual(TEXT("...en er staat geen houding meer; het scherm spreekt zichzelf niet tegen"),
		Houding_Neer, FString());

	// En de andere kant op: hetzelfde pad als de game mode bij een missiestart.
	Harness.Body->ReviveForMission();
	const FString OpHetScherm_Terug = Hud->GetVitalsStatusOnScreen();
	AddInfo(FString::Printf(TEXT("GEMETEN op het scherm na het stabiliseren: gezondheid '%s', status '%s', houding '%s'"),
		*Hud->GetHealthTextOnScreen(), *OpHetScherm_Terug, *Hud->GetStanceTextOnScreen()));
	TestEqual(TEXT("Gestabiliseerd worden staat er als STABILIZED"), OpHetScherm_Terug, FString(TEXT("STABILIZED")));
	TestNotEqual(TEXT("DE EIS: de twee toestanden zijn op het scherm ONDERSCHEIDBAAR"),
		OpHetScherm_Neer, OpHetScherm_Terug);
	TestEqual(TEXT("...en de gezondheid is terug"), Hud->GetHealthTextOnScreen(), FString(TEXT("100")));
	TestEqual(TEXT("...met de houding erbij"), Hud->GetStanceTextOnScreen(), FString(TEXT("STANDING")));

	// De melding blijft niet eeuwig staan: het eerstvolgende feit dat er niets aan de
	// hand is, haalt hem weg. Zonder deze regel zou "STABILIZED" een permanente
	// schermtekst worden en dus betekenisloos.
	Harness.Body->Crouch();
	Harness.Step();
	TestEqual(TEXT("Een volgend feit ruimt de statusmelding op"),
		Hud->GetVitalsStatusOnScreen(), FString());
	Harness.Body->UnCrouch();
	Harness.Step();

	// ---------------------------------------------------------------------------
	// DE ZWAARSTE — het scherm mag nooit stiller zijn dan de bus.
	// ---------------------------------------------------------------------------
	//
	// Een uitlezing die achterloopt ziet er op een frame volstrekt gezond uit: er
	// staat een getal, het is alleen het verkeerde.
	BusFacts = 0;
	const int32 FactsBeforeBurst = Hud->GetVitalsEventCount();
	const int32 DrawsBeforeBurst = Hud->GetVitalsDrawCount();
	for (int32 Hit = 0; Hit < 8; ++Hit)
	{
		Harness.Body->ApplyDamage(5.0f, nullptr, FName(TEXT("HudMeting")));
		Hud->NativeTick(HudGeometry, Options.StepSeconds);
	}

	EclipseFeelHarness::Report(*this, TEXT("feiten op de bus tijdens de reeks klappen"), BusFacts, TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("feiten aangenomen door de HUD"),
		Hud->GetVitalsEventCount() - FactsBeforeBurst, TEXT("feiten"));
	EclipseFeelHarness::Report(*this, TEXT("tekenbeurten van de HUD"),
		Hud->GetVitalsDrawCount() - DrawsBeforeBurst, TEXT("beurten"));

	TestEqual(TEXT("Acht klappen gaven acht feiten op de bus"), BusFacts, 8);
	TestEqual(TEXT("De HUD nam ELK feit aan — niet stiller dan de bus"),
		Hud->GetVitalsEventCount() - FactsBeforeBurst, BusFacts);
	TestEqual(TEXT("...en tekende voor elk feit — niet stiller dan zijn eigen postvak"),
		Hud->GetVitalsDrawCount() - DrawsBeforeBurst, BusFacts);
	TestEqual(TEXT("Het getal op het scherm is de gezondheid van het lichaam"),
		Hud->GetHealthTextOnScreen(), FString::Printf(TEXT("%d"), FMath::CeilToInt(Harness.Body->GetHealth())));
	TestEqual(TEXT("...en dat is ook het getal dat het laatste buspakket droeg"),
		Hud->GetHealthTextOnScreen(), FString::Printf(TEXT("%d"), FMath::CeilToInt(BusHealth)));

	Hud->NativeDestruct();
	Cleanup();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
