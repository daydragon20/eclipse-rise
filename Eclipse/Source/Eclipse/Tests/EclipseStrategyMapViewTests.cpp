// N-b: DE KAARTLAAG NOEMT ZIJN BUREN, EN EEN KAPOTTE GRAAF HAALT HET SCHERM NIET.
//
// Twee helften van één falsificatie (`phase0/EXECUTION_PLAN.md` §2b):
//   1. de kaartlaag noemt per regio zijn buren — met status en prijs, want die
//      zijn er sinds de vorige ijking bij gekomen;
//   2. een ASYMMETRISCHE kant laat de validator roodlopen VOORDAT hij het scherm
//      haalt.
//
// De controleproef staat eerst en draait op elk geval: hetzelfde bord zonder
// mutatie is groen, en dezelfde mutatie op BEIDE einden is weer groen. Zonder
// die twee bewijst een rode balk alleen dat de validator iets niet mag.
//
// De symmetriecontrole zelf is `EclipseStrategyTests.cpp`' onderwerp. Dit bestand
// meet iets anders en nieuwers: dat de SCHERMLAAG hem raadpleegt vóór ze tekent —
// een validator die rood staat terwijl de kaart vrolijk lanes tekent, is geen
// poort maar een logregel.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/EclipseStrategyMapLogic.h"

namespace EclipseMapViewTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	using EclipseStrategyMap::ComposeMapView;
	using EclipseStrategyMap::EEclipseMapDataState;
	using EclipseStrategyMap::FEclipseMapView;

	/**
	 * Eén district met alle drie de lane-statussen, symmetrisch geauthord.
	 *
	 *   Home(P) ——open 1d/r5—— Relay ——SPIRE-GATED by SpireBeta, 1d—— Target
	 *      |                     |
	 *      |open 1d          open 1d
	 *      +——————— SpireBeta ———+
	 *      |
	 *      +~~SMUGGLER ONLY 1d~~ Vault
	 *
	 * Zo gekozen dat ÉÉN eigendomsflip op SpireBeta drie antwoorden verandert:
	 * of Relay->Target militair open is, wat die oversteek kost, en of Target
	 * bevoorraad is. Een bord waarop een flip niets verandert, meet niets.
	 */
	TArray<FEclipseRegionDefinition> MakeBoard()
	{
		TArray<FEclipseRegionDefinition> Board;
		for (const TCHAR* Id : { TEXT("Home"), TEXT("Relay"), TEXT("SpireBeta"), TEXT("Target"), TEXT("Vault") })
		{
			Board.AddDefaulted_GetRef().RegionId = FName(Id);
		}

		auto Find = [&Board](FName Id) -> FEclipseRegionDefinition&
		{
			FEclipseRegionDefinition* Found = Board.FindByPredicate(
				[Id](const FEclipseRegionDefinition& D) { return D.RegionId == Id; });
			check(Found != nullptr);
			return *Found;
		};

		// Elke lane wordt ÉÉN keer geschreven en hier gespiegeld: het fixture is
		// symmetrisch van constructie, zodat elke asymmetrie hieronder met de
		// hand is aangebracht en dus benoembaar is.
		auto Link = [&Find](FName A, FName B, int32 Days, int32 Risk,
			EEclipseLaneStatus Status = EEclipseLaneStatus::Open, FName Gate = NAME_None)
		{
			auto Half = [&](FName From, FName To)
			{
				FEclipseLaneDefinition& Lane = Find(From).Lanes.AddDefaulted_GetRef();
				Lane.NeighborRegionId = To;
				Lane.TravelDays = Days;
				Lane.Risk = Risk;
				Lane.Status = Status;
				Lane.GateRegionId = Gate;
			};
			Half(A, B);
			Half(B, A);
		};

		// GEAUTHORDE POSITIES, want dat is de beslissing (`EXECUTION_PLAN.md`
		// §2a-quinquies): geen afgeleide layout. Ze staan hier zo dat de vorm het
		// plaatje in de kop hierboven volgt — Home links, Vault eronder, Target
		// helemaal rechts achter de poort.
		auto Place = [&Find](FName Id, double X, double Y) { Find(Id).BoardPosition = FVector2D(X, Y); };
		Place(TEXT("Home"), 0.10, 0.55);
		Place(TEXT("Relay"), 0.50, 0.30);
		Place(TEXT("SpireBeta"), 0.45, 0.78);
		Place(TEXT("Target"), 0.88, 0.22);
		Place(TEXT("Vault"), 0.16, 0.94);

		Link(TEXT("Home"), TEXT("Relay"), 1, 5);
		Link(TEXT("Home"), TEXT("SpireBeta"), 1, 0);
		Link(TEXT("Home"), TEXT("Vault"), 1, 0, EEclipseLaneStatus::SmugglerOnly);
		Link(TEXT("Relay"), TEXT("SpireBeta"), 1, 0);
		Link(TEXT("Relay"), TEXT("Target"), 1, 0, EEclipseLaneStatus::SpireGated, TEXT("SpireBeta"));

		return Board;
	}

	/** Dag 3, tier Insurgency (2) — dus +8 risico op elke leg bij de standaardtuning. */
	FEclipseCampaignState MakeState()
	{
		FEclipseCampaignState State;
		State.Day = 3;
		State.ResponseTier = EEclipseDominionResponseTier::Insurgency;

		auto Add = [&State](FName Id, EEclipseRegionOwner Owner, int32 Garrison, int32 Unrest)
		{
			FEclipseRegionState& Region = State.Regions.AddDefaulted_GetRef();
			Region.RegionId = Id;
			Region.Owner = Owner;
			Region.GarrisonStrength = Garrison;
			Region.Unrest = Unrest;
		};
		Add(TEXT("Home"), EEclipseRegionOwner::Player, 3, 12);
		Add(TEXT("Relay"), EEclipseRegionOwner::Dominion, 7, 40);
		Add(TEXT("SpireBeta"), EEclipseRegionOwner::Dominion, 5, 20);
		Add(TEXT("Target"), EEclipseRegionOwner::Dominion, 9, 55);
		Add(TEXT("Vault"), EEclipseRegionOwner::Dominion, 1, 5);
		return State;
	}

	FEclipseLaneDefinition& LaneOf(TArray<FEclipseRegionDefinition>& Board, FName From, FName To)
	{
		FEclipseRegionDefinition* Region = Board.FindByPredicate(
			[From](const FEclipseRegionDefinition& D) { return D.RegionId == From; });
		check(Region != nullptr);
		FEclipseLaneDefinition* Lane = Region->Lanes.FindByPredicate(
			[To](const FEclipseLaneDefinition& L) { return L.NeighborRegionId == To; });
		check(Lane != nullptr);
		return *Lane;
	}

	const EclipseStrategyMap::FEclipseMapRegionView* RegionOf(const FEclipseMapView& View, FName Id)
	{
		return View.Regions.FindByPredicate(
			[Id](const EclipseStrategyMap::FEclipseMapRegionView& R) { return R.RegionId == Id; });
	}

	const EclipseStrategyMap::FEclipseMapLaneView* LaneViewOf(const FEclipseMapView& View, FName From, FName To)
	{
		const EclipseStrategyMap::FEclipseMapRegionView* Region = RegionOf(View, From);
		if (Region == nullptr)
		{
			return nullptr;
		}
		return Region->Lanes.FindByPredicate(
			[To](const EclipseStrategyMap::FEclipseMapLaneView& L) { return L.NeighborRegionId == To; });
	}

	void SetOwner(FEclipseCampaignState& State, FName Id, EEclipseRegionOwner Owner)
	{
		for (FEclipseRegionState& Region : State.Regions)
		{
			if (Region.RegionId == Id)
			{
				Region.Owner = Owner;
			}
		}
	}
}

/**
 * DE CONTROLEPROEF, en hij draait eerst.
 *
 * Zeven manieren waarop twee kanten van dezelfde lane het oneens kunnen zijn.
 * Elke mutatie moet: (a) de kaart op Invalid zetten, (b) NUL regio's opleveren —
 * er wordt niets getekend — en (c) de fout benoemen. En elke mutatie krijgt zijn
 * eigen bewijs dat het de ASYMMETRIE is en niet de waarde: dezelfde mutatie op
 * beide einden is weer groen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewAsymmetryStopsTheBoardTest,
	"Eclipse.UI.MapView.AsymmetryStopsTheBoardBeforeItIsDrawn",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewAsymmetryStopsTheBoardTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const FEclipseCampaignState State = MakeState();
	const FEclipseLaneTuning Tuning;

	// CONTROLE 1 — het onaangeraakte bord komt WEL op het scherm. Zonder deze
	// regel bewijst elk rood hieronder alleen dat er iets stuk is, niet wat.
	{
		const FEclipseMapView View = ComposeMapView(State, MakeBoard(), Tuning);
		TestTrue(TEXT("CONTROLE — het symmetrische bord is tekenbaar"), View.IsRenderable());
		TestEqual(TEXT("CONTROLE — en het draagt alle vijf de regio's"), View.Regions.Num(), 5);
		TestEqual(TEXT("CONTROLE — zonder validatorfouten"), View.GraphErrors.Num(), 0);
	}

	auto ExpectRejected = [this, &State, &Tuning](const TCHAR* What, const TCHAR* Fragment,
		TFunctionRef<void(TArray<FEclipseRegionDefinition>&)> MutateOneEnd,
		TFunctionRef<void(TArray<FEclipseRegionDefinition>&)> MutateBothEnds)
	{
		TArray<FEclipseRegionDefinition> Broken = MakeBoard();
		MutateOneEnd(Broken);
		const FEclipseMapView View = ComposeMapView(State, Broken, Tuning);

		TestEqual(FString::Printf(TEXT("%s: de kaart staat op Invalid"), What),
			static_cast<int32>(View.DataState), static_cast<int32>(EEclipseMapDataState::Invalid));
		TestFalse(FString::Printf(TEXT("%s: de kaart is NIET tekenbaar"), What), View.IsRenderable());

		// Dit is de zin uit de falsificatie: rood VOORDAT hij het scherm haalt.
		TestEqual(FString::Printf(TEXT("%s: er bereikt geen enkele regio het scherm"), What),
			View.Regions.Num(), 0);
		TestTrue(FString::Printf(TEXT("%s: en het scherm zegt waarom"), What),
			!View.StatusText.IsEmpty());
		TestTrue(FString::Printf(TEXT("%s: de fout is benoemd"), What),
			View.GraphErrors.ContainsByPredicate([Fragment](const FString& E) { return E.Contains(Fragment); }));

		// CONTROLE 2 — dezelfde waarde op beide einden is weer tekenbaar. Wat
		// geweigerd wordt is de tegenspraak, niet het getal.
		TArray<FEclipseRegionDefinition> Symmetric = MakeBoard();
		MutateBothEnds(Symmetric);
		TestTrue(FString::Printf(TEXT("%s: op BEIDE einden is het bord weer tekenbaar"), What),
			ComposeMapView(State, Symmetric, Tuning).IsRenderable());
	};

	// 1. De tegenkant bestaat niet meer.
	ExpectRejected(TEXT("ontbrekende tegenkant"), TEXT("Asymmetric edge"),
		[](TArray<FEclipseRegionDefinition>& B)
		{
			B[1].Lanes.RemoveAll([](const FEclipseLaneDefinition& L) { return L.NeighborRegionId == TEXT("Home"); });
		},
		[](TArray<FEclipseRegionDefinition>& B)
		{
			B[1].Lanes.RemoveAll([](const FEclipseLaneDefinition& L) { return L.NeighborRegionId == TEXT("Home"); });
			B[0].Lanes.RemoveAll([](const FEclipseLaneDefinition& L) { return L.NeighborRegionId == TEXT("Relay"); });
		});

	// 2. Status. Stiller kapot dan een ontbrekende kant: de lane bestaat, maar
	//    heen en terug zijn het oneens over wie erdoor mag.
	ExpectRejected(TEXT("status"), TEXT("Asymmetric lane status"),
		[](TArray<FEclipseRegionDefinition>& B) { LaneOf(B, TEXT("Home"), TEXT("Relay")).Status = EEclipseLaneStatus::SmugglerOnly; },
		[](TArray<FEclipseRegionDefinition>& B)
		{
			LaneOf(B, TEXT("Home"), TEXT("Relay")).Status = EEclipseLaneStatus::SmugglerOnly;
			LaneOf(B, TEXT("Relay"), TEXT("Home")).Status = EEclipseLaneStatus::SmugglerOnly;
		});

	// 3. Poort: zelfde status, andere Spire. Twee kanten die naar elkaar wijzen
	//    en een andere knoop aanwijzen als degene die ze dichthoudt.
	ExpectRejected(TEXT("poort"), TEXT("Asymmetric lane gate"),
		[](TArray<FEclipseRegionDefinition>& B) { LaneOf(B, TEXT("Relay"), TEXT("Target")).GateRegionId = TEXT("Home"); },
		[](TArray<FEclipseRegionDefinition>& B)
		{
			LaneOf(B, TEXT("Relay"), TEXT("Target")).GateRegionId = TEXT("Home");
			LaneOf(B, TEXT("Target"), TEXT("Relay")).GateRegionId = TEXT("Home");
		});

	// 4-7. De vier prijsvelden, elk apart. De laatste twee zijn de smokkelprijs,
	//      en die stond tot nu toe niet in de knoppenlijst van de logica-suite.
	auto ExpectCostRejected = [&ExpectRejected](const TCHAR* What, const TCHAR* FieldFragment,
		TFunctionRef<void(FEclipseLaneDefinition&)> Mutate)
	{
		ExpectRejected(What, FieldFragment,
			[&Mutate](TArray<FEclipseRegionDefinition>& B) { Mutate(LaneOf(B, TEXT("Home"), TEXT("Relay"))); },
			[&Mutate](TArray<FEclipseRegionDefinition>& B)
			{
				Mutate(LaneOf(B, TEXT("Home"), TEXT("Relay")));
				Mutate(LaneOf(B, TEXT("Relay"), TEXT("Home")));
			});
	};

	ExpectCostRejected(TEXT("reistijd"), TEXT("TravelDays"),
		[](FEclipseLaneDefinition& L) { L.TravelDays = 4; });
	ExpectCostRejected(TEXT("risico"), TEXT("Risk 25 vs 5"),
		[](FEclipseLaneDefinition& L) { L.Risk = 25; });
	ExpectCostRejected(TEXT("smokkelvertraging"), TEXT("SmugglerDelayDays"),
		[](FEclipseLaneDefinition& L) { L.SmugglerDelayDays = 3; });
	ExpectCostRejected(TEXT("smokkeltoeslag"), TEXT("SmugglerRiskPenalty"),
		[](FEclipseLaneDefinition& L) { L.SmugglerRiskPenalty = 45; });

	return true;
}

/**
 * FALSIFICATIE-HELFT 1 — de kaartlaag noemt per regio zijn buren.
 *
 * Deze test gaat rood op de code van vóór N-b, en dat is de hele reden dat hij
 * bestaat: de widget las `FEclipseCampaignState::Regions` en die structuur HEEFT
 * geen kanten. Geen enkele opmaakverandering kon hem groen krijgen.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewNamesNeighboursTest,
	"Eclipse.UI.MapView.NamesItsNeighboursPerRegion",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewNamesNeighboursTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();
	const FEclipseMapView View = ComposeMapView(MakeState(), Board, FEclipseLaneTuning());

	TestTrue(TEXT("Het bord is tekenbaar"), View.IsRenderable());

	// Elke regio draagt exact de kanten uit de GRAAF — niet uit de toestand.
	for (const FEclipseRegionDefinition& Definition : Board)
	{
		const EclipseStrategyMap::FEclipseMapRegionView* Region = RegionOf(View, Definition.RegionId);
		if (Region == nullptr)
		{
			AddError(FString::Printf(TEXT("Regio '%s' ontbreekt op het bord"), *Definition.RegionId.ToString()));
			continue;
		}

		TestEqual(FString::Printf(TEXT("'%s' draagt evenveel lanes als de graaf"), *Definition.RegionId.ToString()),
			Region->Lanes.Num(), Definition.Lanes.Num());
		TestTrue(FString::Printf(TEXT("'%s' heeft überhaupt buren"), *Definition.RegionId.ToString()),
			Region->Lanes.Num() > 0);

		for (int32 Index = 0; Index < Definition.Lanes.Num() && Index < Region->Lanes.Num(); ++Index)
		{
			TestEqual(FString::Printf(TEXT("'%s' lane %d wijst naar dezelfde buur"), *Definition.RegionId.ToString(), Index),
				Region->Lanes[Index].NeighborRegionId.ToString(), Definition.Lanes[Index].NeighborRegionId.ToString());

			// En de buur staat ook echt in de regel die op het scherm komt.
			TestTrue(FString::Printf(TEXT("'%s' lane %d noemt zijn buur in de regel"), *Definition.RegionId.ToString(), Index),
				Region->Lanes[Index].Text.ToString().Contains(Definition.Lanes[Index].NeighborRegionId.ToString()));
		}
	}

	// De letterlijke eis, in één regel te lezen.
	const EclipseStrategyMap::FEclipseMapRegionView* Home = RegionOf(View, TEXT("Home"));
	if (Home != nullptr)
	{
		TestEqual(TEXT("Home noemt zijn drie buren, in graafvolgorde"),
			Home->NeighborsText.ToString(), FString(TEXT("borders: Relay, SpireBeta, Vault")));
	}
	else
	{
		AddError(TEXT("Home ontbreekt op het bord"));
	}

	return true;
}

/**
 * FALSIFICATIE-HELFT 1, tweede stuk: het scherm heeft méér te tonen dan buren.
 * Drie statussen die er hetzelfde uitzien, leren niets (REFERENTIE_BASE_MAP §1.4).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewLanePriceTest,
	"Eclipse.UI.MapView.LaneStatusAndPriceReachTheScreen",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewLanePriceTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const FEclipseLaneTuning Tuning;
	const FEclipseMapView View = ComposeMapView(MakeState(), MakeBoard(), Tuning);
	TestTrue(TEXT("Het bord is tekenbaar"), View.IsRenderable());

	// Open lane: één prijs, militair doorlaatbaar. Risico 5 (lane) + 8 (tier 2).
	const EclipseStrategyMap::FEclipseMapLaneView* Open = LaneViewOf(View, TEXT("Home"), TEXT("Relay"));
	if (Open == nullptr)
	{
		AddError(TEXT("Home->Relay ontbreekt"));
		return false;
	}
	TestTrue(TEXT("Open lane laat colonnes door"), Open->bMilitaryPassable);
	TestEqual(TEXT("Open lane kost 1 dag"), Open->MilitaryTravelDays, 1);
	TestEqual(TEXT("Open lane draagt het tier-risico"), Open->MilitaryRisk, 13);
	TestTrue(TEXT("De regel noemt de status"), Open->Text.ToString().Contains(TEXT("OPEN")));
	TestTrue(TEXT("De regel noemt het risico"), Open->Text.ToString().Contains(TEXT("risk 13")));

	// Gepoorte lane met vijandige Spire: militair DICHT, met reden, smokkel wel.
	const EclipseStrategyMap::FEclipseMapLaneView* Gated = LaneViewOf(View, TEXT("Relay"), TEXT("Target"));
	if (Gated == nullptr)
	{
		AddError(TEXT("Relay->Target ontbreekt"));
		return false;
	}
	TestFalse(TEXT("Vijandige Spire houdt colonnes tegen"), Gated->bMilitaryPassable);
	TestTrue(TEXT("De reden noemt de Spire bij naam"),
		Gated->MilitaryBlockedText.ToString().Contains(TEXT("SpireBeta")));
	TestTrue(TEXT("Smokkelaars komen er wel door"), Gated->bSmugglerPassable);
	TestTrue(TEXT("En dat is een smokkelleg"), Gated->bSmugglerLeg);
	TestEqual(TEXT("Smokkel kost een dag extra"), Gated->SmugglerTravelDays, 2);
	TestEqual(TEXT("Smokkel kost toeslag + tier"), Gated->SmugglerRisk, 18);
	TestTrue(TEXT("De regel zegt dat het militair dicht is"),
		Gated->Text.ToString().Contains(TEXT("BLOCKED")));
	TestTrue(TEXT("De regel prijst de smokkelroute"),
		Gated->Text.ToString().Contains(TEXT("smugglers")));

	// Smokkellane: nooit militair, in geen enkele campagnestaat.
	const EclipseStrategyMap::FEclipseMapLaneView* Smuggler = LaneViewOf(View, TEXT("Home"), TEXT("Vault"));
	if (Smuggler == nullptr)
	{
		AddError(TEXT("Home->Vault ontbreekt"));
		return false;
	}
	TestFalse(TEXT("Smokkellane laat nooit colonnes door"), Smuggler->bMilitaryPassable);
	TestTrue(TEXT("De regel noemt hem een smokkellane"),
		Smuggler->Text.ToString().Contains(TEXT("SMUGGLER LANE")));

	// De drie regels moeten van elkaar te ONDERSCHEIDEN zijn. Een geblokkeerde
	// lane die er hetzelfde uitziet als een open lane leert je niets.
	TestNotEqual(TEXT("open != gepoort"), Open->Text.ToString(), Gated->Text.ToString());
	TestNotEqual(TEXT("gepoort != smokkel"), Gated->Text.ToString(), Smuggler->Text.ToString());
	TestNotEqual(TEXT("open != smokkel"), Open->Text.ToString(), Smuggler->Text.ToString());

	// GDD 3.1 regel 4 op het scherm: achter de dichte Spire is Target afgesneden.
	const EclipseStrategyMap::FEclipseMapRegionView* Target = RegionOf(View, TEXT("Target"));
	if (Target != nullptr)
	{
		TestFalse(TEXT("Target is afgesneden zolang de Spire vijandig is"), Target->bSupplied);
		TestTrue(TEXT("En dat staat in zijn regel"), Target->HeaderText.ToString().Contains(TEXT("CUT OFF")));
	}

	return true;
}

/**
 * "Dezelfde weg is een andere weg" (GDD 3.1 regel 2 + 9.4), en dat moet op het
 * SCHERM te zien zijn. Eén eigendomsflip verandert drie dingen tegelijk.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewSpireFlipTest,
	"Eclipse.UI.MapView.FlippingTheSpireChangesTheSameLane",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewSpireFlipTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();
	const FEclipseLaneTuning Tuning;

	// Speler neemt de Spire: dezelfde lane gaat open, en Target komt binnen bereik.
	FEclipseCampaignState Taken = MakeState();
	SetOwner(Taken, TEXT("SpireBeta"), EEclipseRegionOwner::Player);
	const FEclipseMapView Open = ComposeMapView(Taken, Board, Tuning);

	const EclipseStrategyMap::FEclipseMapLaneView* OpenLane = LaneViewOf(Open, TEXT("Relay"), TEXT("Target"));
	if (OpenLane == nullptr)
	{
		AddError(TEXT("Relay->Target ontbreekt na de flip"));
		return false;
	}
	TestTrue(TEXT("Eigen Spire: colonnes mogen erdoor"), OpenLane->bMilitaryPassable);
	TestEqual(TEXT("Eigen Spire: alleen het tier-risico blijft"), OpenLane->MilitaryRisk, 8);
	TestFalse(TEXT("Eigen Spire: geen smokkelleg meer nodig"), OpenLane->bSmugglerLeg);

	const EclipseStrategyMap::FEclipseMapRegionView* Target = RegionOf(Open, TEXT("Target"));
	if (Target != nullptr)
	{
		TestTrue(TEXT("Eigen Spire: Target is bevoorraad"), Target->bSupplied);
	}

	// Betwiste Spire: je komt erdoor, maar door een vuurgevecht (+15).
	FEclipseCampaignState Contested = MakeState();
	SetOwner(Contested, TEXT("SpireBeta"), EEclipseRegionOwner::Contested);
	const EclipseStrategyMap::FEclipseMapLaneView* ContestedLane =
		LaneViewOf(ComposeMapView(Contested, Board, Tuning), TEXT("Relay"), TEXT("Target"));
	if (ContestedLane != nullptr)
	{
		TestTrue(TEXT("Betwiste Spire: doorlaatbaar"), ContestedLane->bMilitaryPassable);
		TestEqual(TEXT("Betwiste Spire: +15 boven het tier-risico"), ContestedLane->MilitaryRisk, 23);
	}
	else
	{
		AddError(TEXT("Relay->Target ontbreekt bij een betwiste Spire"));
	}

	// En de statusregel zegt WIE de poort vasthoudt — anders is "SPIRE-GATED"
	// een label zonder gevolg.
	if (OpenLane != nullptr)
	{
		TestTrue(TEXT("De regel noemt de eigenaar van de poort"),
			OpenLane->Text.ToString().Contains(TEXT("PLAYER")));
	}

	return true;
}

/** De tegenspeler staat op het bord (GDD 9.4): de tier prijst elke leg. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewResponseTierTest,
	"Eclipse.UI.MapView.ResponseTierPricesEveryLegOnScreen",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewResponseTierTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();
	const FEclipseLaneTuning Tuning;

	FEclipseCampaignState Cold = MakeState();
	Cold.ResponseTier = EEclipseDominionResponseTier::Indifference;
	const FEclipseMapView ColdView = ComposeMapView(Cold, Board, Tuning);

	const EclipseStrategyMap::FEclipseMapLaneView* ColdLane = LaneViewOf(ColdView, TEXT("Home"), TEXT("Relay"));
	if (ColdLane == nullptr)
	{
		AddError(TEXT("Home->Relay ontbreekt"));
		return false;
	}
	TestEqual(TEXT("Tier 0: alleen de lane-prijs"), ColdLane->MilitaryRisk, 5);
	TestEqual(TEXT("Tier 0: geen opslag"), ColdView.RiskPerLegFromTier, 0);
	TestTrue(TEXT("Tier 0: de kop noemt de tier"), ColdView.HeaderText.ToString().Contains(TEXT("INDIFFERENCE")));
	TestFalse(TEXT("Tier 0: en schrijft geen +0 op"), ColdView.HeaderText.ToString().Contains(TEXT("+0")));

	const FEclipseMapView HotView = ComposeMapView(MakeState(), Board, Tuning);
	const EclipseStrategyMap::FEclipseMapLaneView* HotLane = LaneViewOf(HotView, TEXT("Home"), TEXT("Relay"));
	if (HotLane != nullptr)
	{
		TestEqual(TEXT("Tier 2: dezelfde lane kost meer"), HotLane->MilitaryRisk, 13);
	}
	TestEqual(TEXT("Tier 2: +8 per leg"), HotView.RiskPerLegFromTier, 8);
	TestTrue(TEXT("De kop noemt de tier bij naam"), HotView.HeaderText.ToString().Contains(TEXT("INSURGENCY")));
	TestTrue(TEXT("De kop noemt de opslag"), HotView.HeaderText.ToString().Contains(TEXT("+8")));
	TestTrue(TEXT("De kop noemt de dag"), HotView.HeaderText.ToString().Contains(TEXT("Day 3")));

	return true;
}

/**
 * Ontbrekend is niet leeg, en scheef is niet ontbrekend. Drie toestanden, drie
 * verschillende reparaties — een scherm dat ze samenvat tot een leeg bord
 * vertelt geen van drieën.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewMissingDataTest,
	"Eclipse.UI.MapView.AbsentGraphSaysAbsentInsteadOfEmpty",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewMissingDataTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const FEclipseMapView Absent = ComposeMapView(MakeState(), TArray<FEclipseRegionDefinition>(), FEclipseLaneTuning());
	TestEqual(TEXT("Geen graaf = Absent"), static_cast<int32>(Absent.DataState), static_cast<int32>(EEclipseMapDataState::Absent));
	TestFalse(TEXT("En dus niets te tekenen"), Absent.IsRenderable());
	TestTrue(TEXT("Maar het scherm zegt het hardop"), !Absent.StatusText.IsEmpty());
	TestEqual(TEXT("Absent is geen validatorfout"), Absent.GraphErrors.Num(), 0);
	TestTrue(TEXT("De kop blijft waar: de campagne bestaat wel"),
		Absent.HeaderText.ToString().Contains(TEXT("Day 3")));

	// Een graaf die zichzelf tegenspreekt is een ANDERE toestand dan geen graaf.
	TArray<FEclipseRegionDefinition> Broken = MakeBoard();
	Broken[1].Lanes.RemoveAll([](const FEclipseLaneDefinition& L) { return L.NeighborRegionId == TEXT("Home"); });
	const FEclipseMapView Invalid = ComposeMapView(MakeState(), Broken, FEclipseLaneTuning());
	TestEqual(TEXT("Kapotte graaf = Invalid"), static_cast<int32>(Invalid.DataState), static_cast<int32>(EEclipseMapDataState::Invalid));
	TestTrue(TEXT("Invalid draagt de fouten mee"), Invalid.GraphErrors.Num() > 0);
	TestNotEqual(TEXT("Absent en Invalid zeggen niet hetzelfde"),
		Absent.StatusText.ToString(), Invalid.StatusText.ToString());

	return true;
}

/**
 * De twee structuren kunnen uit de pas lopen, en dan hoort dat ZICHTBAAR te
 * zijn. Een regio die alleen de graaf kent, en een regio die alleen de campagne
 * kent, zijn allebei datadrift — en allebei stil als je ze weglaat.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewDriftTest,
	"Eclipse.UI.MapView.GraphAndStateDisagreementsStayVisible",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewDriftTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	// Graaf kent Vault, campagne niet.
	FEclipseCampaignState Thin = MakeState();
	Thin.Regions.RemoveAll([](const FEclipseRegionState& R) { return R.RegionId == TEXT("Vault"); });
	const FEclipseMapView ThinView = ComposeMapView(Thin, MakeBoard(), FEclipseLaneTuning());

	TestTrue(TEXT("Het bord blijft tekenbaar"), ThinView.IsRenderable());
	const EclipseStrategyMap::FEclipseMapRegionView* Vault = RegionOf(ThinView, TEXT("Vault"));
	if (Vault != nullptr)
	{
		TestFalse(TEXT("Vault heeft geen campagnestaat"), Vault->bHasState);
		TestTrue(TEXT("En dat staat er"), Vault->HeaderText.ToString().Contains(TEXT("NO CAMPAIGN STATE")));
		TestTrue(TEXT("Zijn kanten blijven wel staan"), Vault->Lanes.Num() > 0);
	}
	else
	{
		AddError(TEXT("Vault verdween van het bord"));
	}

	// Campagne kent Orbital, graaf niet: hij ligt aan geen enkele lane.
	FEclipseCampaignState Extra = MakeState();
	FEclipseRegionState& Orbital = Extra.Regions.AddDefaulted_GetRef();
	Orbital.RegionId = TEXT("Orbital");
	Orbital.Owner = EEclipseRegionOwner::Dominion;
	const FEclipseMapView ExtraView = ComposeMapView(Extra, MakeBoard(), FEclipseLaneTuning());

	const EclipseStrategyMap::FEclipseMapRegionView* OrbitalView = RegionOf(ExtraView, TEXT("Orbital"));
	if (OrbitalView != nullptr)
	{
		TestFalse(TEXT("Orbital heeft geen definitie"), OrbitalView->bHasDefinition);
		TestTrue(TEXT("En dat staat er"), OrbitalView->HeaderText.ToString().Contains(TEXT("OFF THE MAP")));
		TestEqual(TEXT("Onbereikbaar: geen kanten"), OrbitalView->Lanes.Num(), 0);
	}
	else
	{
		AddError(TEXT("Orbital werd stil weggelaten — precies de drift die zichtbaar moest blijven"));
	}

	return true;
}

// ---------------------------------------------------------------------------
// N-d: VAN LIJST MET TOPOLOGIE NAAR GRAAF.
//
// Het bord noemde zijn buren al, maar als TEKST. Deze drie tests meten de
// tweede lezing van dezelfde data: knopen op posities en lanes als lijnen.
//
// Wat ze bewaken is niet "er staat een plaatje" (dat kan geen enkele headless
// test zien) maar de twee dingen die stil fout kunnen gaan en die je op een
// screenshot pas ontdekt als je gaat tellen:
//   1. elke ongerichte lane hoort ÉÉN lijn te zijn en niet twee;
//   2. een bord zonder geauthorde indeling tekent GEEN graaf en behoudt WEL
//      de lijst — twee poorten, niet één.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewLayoutDrawsEachLaneOnceTest,
	"Eclipse.UI.MapView.LayoutDrawsEachLaneExactlyOnce",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewLayoutDrawsEachLaneOnceTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const TArray<FEclipseRegionDefinition> Board = MakeBoard();
	const FEclipseMapView View = ComposeMapView(MakeState(), Board, FEclipseLaneTuning());

	if (!TestTrue(TEXT("Een volledig geauthord bord heeft een indeling"), View.bHasLayout))
	{
		AddError(FString::Printf(TEXT("reden: %s"), *View.LayoutStatusText.ToString()));
		return false;
	}

	// DE HELE CLAIM IN ÉÉN VERGELIJKING. Het fixture draagt vijf ongerichte
	// lanes en dus TIEN lane-helften (elke regio noemt zijn eigen kant). Zou de
	// tekenlaag de lijst simpelweg aflopen, dan stonden er tien lijnen — elke
	// lane twee keer over elkaar, dikker en donkerder dan een lane die maar één
	// kant heeft. Vijf is het bewijs dat er ontdubbeld wordt.
	int32 LaneHalves = 0;
	for (const FEclipseRegionDefinition& Definition : Board)
	{
		LaneHalves += Definition.Lanes.Num();
	}
	TestEqual(TEXT("CONTROLE — het fixture draagt tien lane-helften"), LaneHalves, 10);
	TestEqual(TEXT("En de tekenlaag krijgt vijf lijnen"), View.Edges.Num(), 5);

	// Geen enkele kant twee keer, in welke richting dan ook.
	TSet<FString> Seen;
	for (const EclipseStrategyMap::FEclipseMapEdgeView& Edge : View.Edges)
	{
		const FString Forward = FString::Printf(TEXT("%s>%s"), *Edge.RegionIdA.ToString(), *Edge.RegionIdB.ToString());
		const FString Backward = FString::Printf(TEXT("%s>%s"), *Edge.RegionIdB.ToString(), *Edge.RegionIdA.ToString());
		TestFalse(FString::Printf(TEXT("%s staat er maar één keer"), *Forward),
			Seen.Contains(Forward) || Seen.Contains(Backward));
		Seen.Add(Forward);
	}

	// De posities komen ONGEWIJZIGD door. Een layout die onderweg iets schaalt of
	// verschuift zou de authoring stil overrulen, en dan is "geauthord" een leugen.
	const EclipseStrategyMap::FEclipseMapRegionView* Home = RegionOf(View, TEXT("Home"));
	if (TestNotNull(TEXT("Home staat op het bord"), Home))
	{
		TestTrue(TEXT("Home draagt zijn geauthorde positie"), Home->bHasBoardPosition);
		TestEqual(TEXT("Home X"), Home->BoardPosition.X, 0.10, 1e-6);
		TestEqual(TEXT("Home Y"), Home->BoardPosition.Y, 0.55, 1e-6);
	}

	// En één lijn helemaal nagelopen: de gepoorte lane draagt zijn twee
	// eindpunten, zijn status én de reden dat er geen colonne door kan.
	const EclipseStrategyMap::FEclipseMapEdgeView* Gated = View.Edges.FindByPredicate(
		[](const EclipseStrategyMap::FEclipseMapEdgeView& E) { return E.Status == EEclipseLaneStatus::SpireGated; });
	if (TestNotNull(TEXT("De gepoorte lane is een lijn"), Gated))
	{
		TestEqual(TEXT("Hij loopt tussen Relay en Target"),
			FString::Printf(TEXT("%s-%s"), *Gated->RegionIdA.ToString(), *Gated->RegionIdB.ToString()),
			FString(TEXT("Relay-Target")));
		TestEqual(TEXT("Zijn eerste eindpunt is de positie van Relay"), Gated->A.X, 0.50, 1e-6);
		TestEqual(TEXT("Zijn tweede eindpunt is de positie van Target"), Gated->B.X, 0.88, 1e-6);
		TestFalse(TEXT("Militair dicht zolang SpireBeta vijandig is"), Gated->bMilitaryPassable);
		TestTrue(TEXT("Smokkelaars komen er wel door"), Gated->bSmugglerPassable);
		TestFalse(TEXT("En wat hij kost staat op de lijn"), Gated->CostText.IsEmpty());
	}

	// De TWEEDE lezing moet dezelfde waarheid vertellen als de eerste: evenveel
	// lijnen als er unieke kanten in de tekstregels staan.
	int32 LaneRows = 0;
	for (const EclipseStrategyMap::FEclipseMapRegionView& Region : View.Regions)
	{
		LaneRows += Region.Lanes.Num();
	}
	TestEqual(TEXT("Lijst en graaf spreken over dezelfde kanten"), LaneRows, View.Edges.Num() * 2);

	AddInfo(FString::Printf(TEXT("GEMETEN  %d regio's · %d lane-helften in de lijst · %d lijnen in de graaf"),
		View.Regions.Num(), LaneRows, View.Edges.Num()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewLayoutGateKeepsTheListTest,
	"Eclipse.UI.MapView.NoLayoutHidesTheGraphButKeepsTheList",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewLayoutGateKeepsTheListTest::RunTest(const FString& Parameters)
{
	using namespace EclipseMapViewTest;

	const FEclipseCampaignState State = MakeState();
	const FEclipseLaneTuning Tuning;

	// CONTROLE EERST: bewijs dat dit bord met indeling WEL een graaf oplevert.
	// Zonder deze regel bewijst "geen graaf" hieronder alleen dat er iets stuk is.
	{
		const FEclipseMapView Good = ComposeMapView(State, MakeBoard(), Tuning);
		TestTrue(TEXT("CONTROLE — mét authoring is er een graaf"), Good.bHasLayout);
		TestEqual(TEXT("CONTROLE — met vijf lijnen"), Good.Edges.Num(), 5);
		TestTrue(TEXT("CONTROLE — en geen klacht"), Good.LayoutStatusText.IsEmpty());
	}

	// 1. Eén regio niet geplaatst. HALVE GRAAF IS ERGER DAN GEEN GRAAF: een
	//    ontbrekende knoop leest als "die regio grenst nergens aan".
	{
		TArray<FEclipseRegionDefinition> Board = MakeBoard();
		Board.FindByPredicate([](const FEclipseRegionDefinition& D) { return D.RegionId == TEXT("Vault"); })
			->BoardPosition = FVector2D(-1.0, -1.0);
		const FEclipseMapView View = ComposeMapView(State, Board, Tuning);

		TestFalse(TEXT("Eén ontbrekende positie sluit de tekenlaag"), View.bHasLayout);
		TestEqual(TEXT("Er wordt geen enkele lijn getekend"), View.Edges.Num(), 0);
		TestTrue(TEXT("Het scherm noemt de regio die ontbreekt"),
			View.LayoutStatusText.ToString().Contains(TEXT("Vault")));

		// EN DIT IS HET PUNT VAN TWEE POORTEN. De asymmetriepoort slaat het bord
		// blank; deze niet. Een bord zonder indeling is niet kapot — het is niet
		// ingedeeld, en de lijst is dan alles wat de speler heeft.
		TestTrue(TEXT("Het bord blijft tekenbaar"), View.IsRenderable());
		TestEqual(TEXT("En de lijst staat er nog voluit"), View.Regions.Num(), 5);
		TestTrue(TEXT("Inclusief de buren per regio"),
			RegionOf(View, TEXT("Home"))->NeighborsText.ToString().Contains(TEXT("Relay")));
	}

	// 2. Twee knopen op dezelfde plek. Andere fout, andere reparatie: hier heeft
	//    iemand wél geauthord, alleen twee keer hetzelfde.
	{
		TArray<FEclipseRegionDefinition> Board = MakeBoard();
		Board.FindByPredicate([](const FEclipseRegionDefinition& D) { return D.RegionId == TEXT("Vault"); })
			->BoardPosition = Board.FindByPredicate(
				[](const FEclipseRegionDefinition& D) { return D.RegionId == TEXT("Home"); })->BoardPosition;
		const FEclipseMapView View = ComposeMapView(State, Board, Tuning);

		TestFalse(TEXT("Twee knopen op één plek is geen indeling"), View.bHasLayout);
		TestEqual(TEXT("Er wordt niets getekend"), View.Edges.Num(), 0);
		TestTrue(TEXT("Het scherm noemt beide regio's"),
			View.LayoutStatusText.ToString().Contains(TEXT("Home"))
			&& View.LayoutStatusText.ToString().Contains(TEXT("Vault")));
		TestTrue(TEXT("En ook hier blijft de lijst staan"), View.IsRenderable() && View.Regions.Num() == 5);
	}

	// 3. Een positie BUITEN het bord telt niet als geauthord. Een knoop op
	//    (1,4 · 0,5) zou buiten het vlak vallen en dus onzichtbaar zijn — dat is
	//    hetzelfde als niet geplaatst, en het hoort net zo hard te melden.
	{
		TArray<FEclipseRegionDefinition> Board = MakeBoard();
		Board.FindByPredicate([](const FEclipseRegionDefinition& D) { return D.RegionId == TEXT("Target"); })
			->BoardPosition = FVector2D(1.4, 0.5);
		const FEclipseMapView View = ComposeMapView(State, Board, Tuning);

		TestFalse(TEXT("Buiten het bord is niet geplaatst"), View.bHasLayout);
		TestTrue(TEXT("En dat wordt bij naam gemeld"),
			View.LayoutStatusText.ToString().Contains(TEXT("Target")));
	}

	return true;
}

/**
 * AUTHORED IS NOT SHIPPED — dezelfde toets als bij de lanes
 * (`Eclipse.Strategy.Lanes.ShippedBoardActuallyUsesTheThreeStatuses`), en om
 * dezelfde reden: alle tests hierboven draaien op een C++-fixture en zouden
 * groen blijven op een verscheept bord waar niemand ooit een knoop heeft
 * neergezet. Dan bestaat de graaf in de engine en nergens in het spel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseMapViewShippedLayoutTest,
	"Eclipse.UI.MapView.ShippedBoardCarriesAnAuthoredLayout",
	EclipseMapViewTest::TestFlags)

bool FEclipseMapViewShippedLayoutTest::RunTest(const FString& Parameters)
{
	UEclipseRegionGraphAsset* Graph = LoadObject<UEclipseRegionGraphAsset>(
		nullptr, TEXT("/Game/Data/DA_KessaraDistrictGraph.DA_KessaraDistrictGraph"));
	if (!TestNotNull(TEXT("Het verscheepte districtsbord laadt"), Graph))
	{
		AddError(TEXT("Draai Tools/author_region_layout.py."));
		return false;
	}

	FEclipseCampaignState State;
	for (const FEclipseRegionDefinition& Definition : Graph->Regions)
	{
		FEclipseRegionState& Region = State.Regions.AddDefaulted_GetRef();
		Region.RegionId = Definition.RegionId;
		Region.Owner = Definition.StartingOwner;
	}

	const EclipseStrategyMap::FEclipseMapView View =
		EclipseStrategyMap::ComposeMapView(State, Graph->Regions, Graph->LaneTuning);
	if (!TestTrue(TEXT("Het verscheepte bord heeft een geauthorde indeling"), View.bHasLayout))
	{
		AddError(FString::Printf(TEXT("reden: %s"), *View.LayoutStatusText.ToString()));
		return false;
	}

	TestEqual(TEXT("Elke regio staat ergens"), View.Regions.Num(), Graph->Regions.Num());
	TestTrue(TEXT("En er lopen lijnen tussen"), View.Edges.Num() > 0);

	// DE VORM MOET DE FICTIE DRAGEN, en dat is precies waarom er geauthord wordt
	// in plaats van afgeleid. Twee eisen die een automatische layout NIET haalt
	// en die een mens per ongeluk kan omgooien:
	//   - de Underworks liggen ONDER (grootste Y van het bord);
	//   - de Comms Relay ligt aan het uiteinde van de gepoorte lane, dus ver
	//     naar rechts.
	const EclipseStrategyMap::FEclipseMapRegionView* Under = View.Regions.FindByPredicate(
		[](const EclipseStrategyMap::FEclipseMapRegionView& R) { return R.RegionId == TEXT("Underworks"); });
	const EclipseStrategyMap::FEclipseMapRegionView* Relay = View.Regions.FindByPredicate(
		[](const EclipseStrategyMap::FEclipseMapRegionView& R) { return R.RegionId == TEXT("CommsRelay"); });
	if (Under != nullptr)
	{
		double LowestY = 0.0;
		for (const EclipseStrategyMap::FEclipseMapRegionView& Region : View.Regions)
		{
			LowestY = FMath::Max(LowestY, Region.BoardPosition.Y);
		}
		TestEqual(TEXT("De Underworks liggen onderaan het bord"), Under->BoardPosition.Y, LowestY, 1e-6);
	}
	if (Relay != nullptr)
	{
		double RightmostX = 0.0;
		for (const EclipseStrategyMap::FEclipseMapRegionView& Region : View.Regions)
		{
			RightmostX = FMath::Max(RightmostX, Region.BoardPosition.X);
		}
		TestEqual(TEXT("De Comms Relay ligt aan het verre eind"), Relay->BoardPosition.X, RightmostX, 1e-6);
	}

	AddInfo(FString::Printf(TEXT("GEMETEN  verscheept bord: %d knopen op posities · %d lijnen"),
		View.Regions.Num(), View.Edges.Num()));
	return true;
}

#endif
