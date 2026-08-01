// N-c: HET BASISSCHERM TOONT WAT DE BASIS AAN HET DOEN IS, EN VERZINT NIETS.
//
// `phase0/REFERENTIE_BASE_MAP.md` §2.3 vraagt zeven dingen van dit scherm.
// Zes ervan zijn hier een getal dat een test kan vastpinnen zonder viewport:
// het slotraster, de bouw-ETA als VOORTGANG (een fractie, geen dagenteller),
// de rushprijs, de bemanning, de energiebalans en de schade.
//
// TWEE ONAFHANKELIJKE POORTEN, en dat is wat dit bestand vooral meet:
//   1. de DATApoort slaat het raster blank bij een tegenspraak tussen layout en
//      toestand — een faciliteit die nergens staat is erger dan geen raster;
//   2. de INHOUDSpoorten (energie, bemanning) sluiten alleen hun eigen band en
//      laten het raster staan. Energie tot een validatiefout maken zou het
//      raster blank slaan om een ontbrekende kolom in een datatabel.
//
// ELKE POORT KRIJGT ZIJN CONTROLEPROEF. Een test die aantoont dat de datapoort
// dichtslaat, bewijst niets zolang niet vaststaat dat DEZELFDE basis zonder die
// mutatie wél door de poort komt; anders meet je alleen dat er iets niet mag.
//
// EN DE SCHERPSTE: `EnergyUnauthoredIsNotBalanced`. De sliceverzameling authort
// geen energie, dus alle upkeep staat op 0. Een scherm dat daar "POWER 0 / 0,
// prima" van maakt, ziet er precies zo uit als een scherm dat de balans echt
// kent. Die test pint vast dat het verschil zichtbaar blijft.

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipseBaseLogic.h"
#include "Core/EclipseGameplayTags.h"
#include "Misc/AutomationTest.h"
#include "UI/EclipseBaseViewLogic.h"

namespace EclipseBaseViewTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	using EclipseBaseView::ComposeBaseView;
	using EclipseBaseView::EEclipseBaseDataState;
	using EclipseBaseView::EEclipseEnergyState;
	using EclipseBaseView::EEclipseSlotStatus;
	using EclipseBaseView::FEclipseBaseSlotView;
	using EclipseBaseView::FEclipseBaseView;

	/**
	 * Vier slots, waarvan één op uitbreidingstrap 2 — zo gekozen dat één
	 * verandering (de trap, de schadevlag, de energiekolom) telkens een ANDER
	 * antwoord oplevert. Een opstelling waarin een mutatie niets verandert,
	 * meet niets.
	 *
	 *   Slot_A  Command Center   trap 1   (voorgebouwd L1)
	 *   Slot_B  Barracks         trap 1
	 *   Slot_C  Workshop         trap 1   (2 niveaus -> kan upgraden)
	 *   Slot_D  Deep Gallery     trap 2   (vergrendeld tot de trap oploopt)
	 */
	TArray<FEclipseBaseSlotDef> MakeLayout()
	{
		TArray<FEclipseBaseSlotDef> Slots;

		auto Add = [&Slots](const TCHAR* Id, const TCHAR* Name, std::initializer_list<const TCHAR*> Allowed, int32 Tier)
		{
			FEclipseBaseSlotDef& Slot = Slots.AddDefaulted_GetRef();
			Slot.SlotId = FName(Id);
			Slot.DisplayName = FText::FromString(Name);
			Slot.UnlockTier = Tier;
			for (const TCHAR* Row : Allowed)
			{
				Slot.AllowedFacilityRows.Add(FName(Row));
			}
		};

		Add(TEXT("Slot_A"), TEXT("Command"), { TEXT("CommandCenter") }, 1);
		Add(TEXT("Slot_B"), TEXT("Bunks"), { TEXT("Barracks") }, 1);
		Add(TEXT("Slot_C"), TEXT("Machine Floor"), { TEXT("Workshop") }, 1);
		Add(TEXT("Slot_D"), TEXT("Deep Gallery"), { TEXT("Workshop") }, 2);
		return Slots;
	}

	/** DT_Facilities als plain data. Energie staat op 0 — precies zoals de slice hem verscheept. */
	TMap<FName, FEclipseFacilityRow> MakeFacilities()
	{
		TMap<FName, FEclipseFacilityRow> Rows;

		FEclipseFacilityRow& Command = Rows.Add(TEXT("CommandCenter"));
		Command.DisplayName = FText::FromString(TEXT("Command Center"));
		Command.Levels.AddDefaulted_GetRef().BuildDays = 0;

		FEclipseFacilityRow& Barracks = Rows.Add(TEXT("Barracks"));
		Barracks.DisplayName = FText::FromString(TEXT("Barracks"));
		Barracks.Levels.AddDefaulted_GetRef().BuildDays = 2;

		FEclipseFacilityRow& Workshop = Rows.Add(TEXT("Workshop"));
		Workshop.DisplayName = FText::FromString(TEXT("Workshop"));
		Workshop.Levels.AddDefaulted_GetRef().BuildDays = 3;
		Workshop.Levels.AddDefaulted_GetRef().BuildDays = 4;

		return Rows;
	}

	/** Een campagne met de voorgebouwde Command Center op Slot_A en een klein rooster. */
	FEclipseCampaignState MakeState()
	{
		FEclipseCampaignState State;
		State.Day = 3;
		State.BaseState.Facilities.Reset();

		FEclipseFacilityState& Command = State.BaseState.Facilities.AddDefaulted_GetRef();
		Command.SlotId = TEXT("Slot_A");
		Command.FacilityId = TEXT("CommandCenter");
		Command.Level = 1;

		for (const TCHAR* Name : { TEXT("Vasquez"), TEXT("Otoye"), TEXT("Brandt") })
		{
			FEclipseSoldierRecord& Soldier = State.Roster.AddDefaulted_GetRef();
			Soldier.SoldierId = FGuid::NewGuid();
			Soldier.Name = Name;
			Soldier.Status = EEclipseSoldierStatus::Available;
		}
		return State;
	}

	FEclipseFacilityState& AddFacility(FEclipseCampaignState& State, const TCHAR* SlotId, const TCHAR* FacilityId, int32 Level, int32 DaysRemaining)
	{
		FEclipseFacilityState& Facility = State.BaseState.Facilities.AddDefaulted_GetRef();
		Facility.SlotId = FName(SlotId);
		Facility.FacilityId = FName(FacilityId);
		Facility.Level = Level;
		Facility.DaysRemaining = DaysRemaining;
		return Facility;
	}

	const FEclipseBaseSlotView* FindSlot(const FEclipseBaseView& View, const TCHAR* SlotId)
	{
		return View.Slots.FindByPredicate([Id = FName(SlotId)](const FEclipseBaseSlotView& S) { return S.SlotId == Id; });
	}

	/** Eén aanroep met alle resolvers eraan. TFunctionRef wil lvalues die de aanroep overleven. */
	FEclipseBaseView Compose(
		const FEclipseCampaignState& State,
		const TArray<FEclipseBaseSlotDef>& Slots,
		const TMap<FName, FEclipseFacilityRow>& Rows,
		const EclipseBaseLogic::FEclipseBaseTuningParams& Tuning,
		int32 Tier = 1)
	{
		auto RowResolver = [&Rows](FName Id) -> const FEclipseFacilityRow* { return Rows.Find(Id); };
		auto SoldierResolver = [&State](const FGuid& Id) -> const FEclipseSoldierRecord* { return State.FindSoldier(Id); };
		return ComposeBaseView(State, Slots, Tuning, RowResolver, SoldierResolver, Tier);
	}

	EclipseBaseLogic::FEclipseBaseTuningParams MakeTuning()
	{
		EclipseBaseLogic::FEclipseBaseTuningParams Tuning;
		Tuning.RushCostCreditsPerDay = 60;
		Tuning.MaxCrewPerSite = 1;
		return Tuning;
	}
}

// ---------------------------------------------------------------------------
// 1. DE DATAPOORT — een faciliteit die nergens staat, blankt het raster
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewDataGateTest,
	"Eclipse.UI.BaseViewDataGateBlanksTheGrid",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewDataGateTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	const TArray<FEclipseBaseSlotDef> Layout = MakeLayout();
	const TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = MakeTuning();

	// CONTROLEPROEF EERST. Zonder deze helft bewijst de rode uitkomst hieronder
	// alleen dat de poort iets niet mag, niet dat hij het JUISTE niet mag.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestTrue(TEXT("Control: a consistent base composes"), View.IsRenderable());
		TestEqual(TEXT("Control: all four slots reach the grid"), View.Slots.Num(), 4);
		TestTrue(TEXT("Control: no errors"), View.Errors.IsEmpty());
	}

	// DE MUTATIE: dezelfde basis, maar de Workshop staat op een slot dat de
	// layout niet kent. Hij bestaat, hij kostte materiaal, en hij zou op geen
	// enkele tegel verschijnen.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_Z"), TEXT("Workshop"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestFalse(TEXT("A facility on an unknown slot is not renderable"), View.IsRenderable());
		TestTrue(TEXT("DataState is Invalid, not Absent — the layout exists"),
			View.DataState == EEclipseBaseDataState::Invalid);
		TestEqual(TEXT("The grid stays EMPTY rather than silently dropping the facility"), View.Slots.Num(), 0);
		TestTrue(TEXT("The problem is named"), View.Errors.Num() >= 1);
		TestFalse(TEXT("And the screen says so"), View.StatusText.IsEmpty());
	}

	// TWEEDE MUTATIE: twee faciliteiten op één slot. Het raster heeft één tegel
	// per slot, dus één van de twee zou onzichtbaar zijn.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		AddFacility(State, TEXT("Slot_C"), TEXT("Barracks"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestFalse(TEXT("Two facilities on one slot is a contradiction"), View.IsRenderable());
		TestEqual(TEXT("Grid withheld"), View.Slots.Num(), 0);
	}

	// GEEN LAYOUT is iets anders dan een KAPOTTE layout — verschillende reparaties.
	{
		const FEclipseCampaignState State = MakeState();
		const FEclipseBaseView View = Compose(State, TArray<FEclipseBaseSlotDef>(), Rows, Tuning);
		TestTrue(TEXT("No layout reads as Absent, not Invalid"),
			View.DataState == EEclipseBaseDataState::Absent);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 2. DE BOUW-ETA IS EEN VOORTGANG, GEEN GETAL (§2.3 rij 2)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewEtaIsProgressTest,
	"Eclipse.UI.BaseViewEtaIsProgressNotANumber",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewEtaIsProgressTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	const TArray<FEclipseBaseSlotDef> Layout = MakeLayout();
	const TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = MakeTuning();

	// Workshop L1 kost 3 dagen. Net begonnen: 3 resterend, dus 0 gedaan.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 0, 3);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing from the grid"));
			return false;
		}
		TestTrue(TEXT("Status is UnderConstruction"), Slot->Status == EEclipseSlotStatus::UnderConstruction);
		TestTrue(TEXT("There is a bar to draw"), Slot->bHasProgress);
		TestEqual(TEXT("Freshly started = 0 progress"), Slot->Progress01, 0.0f);
		TestEqual(TEXT("Total days come from the TARGET level's data"), Slot->TotalDays, 3);
		TestEqual(TEXT("Building toward L1"), Slot->TargetLevel, 1);
	}

	// Eén dag verder: 2 resterend van 3 = 1/3.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 0, 2);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestEqual(TEXT("One of three days done = 1/3"), Slot->Progress01, 1.0f / 3.0f, 0.0001f);
		TestTrue(TEXT("The text names both halves of the fraction"),
			Slot->ProgressText.ToString().Contains(TEXT("day 1 of 3")));
	}

	// EEN UPGRADE meet tegen het NIEUWE niveau, niet het oude. Workshop L2 = 4
	// dagen; tegen L1's 3 dagen zou 2 resterend "1 of 3" heten in plaats van
	// "2 of 4" — een balk die te ver staat.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 2);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestEqual(TEXT("Upgrade measures against L2's build days"), Slot->TotalDays, 4);
		TestEqual(TEXT("Two of four days done"), Slot->Progress01, 0.5f, 0.0001f);
		TestEqual(TEXT("Target level is 2"), Slot->TargetLevel, 2);
	}

	// DE POORT ONDER DE BALK: geen rij in DT_Facilities = geen noemer = geen
	// balk. De dagen blijven staan, en het RASTER blijft heel — dat is het
	// verschil tussen een ontbrekend detail en een datafout.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("GhostFacility"), 0, 2);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestTrue(TEXT("An unknown facility row does NOT blank the grid"), View.IsRenderable());
		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestFalse(TEXT("No denominator means no bar — never a guessed one"), Slot->bHasProgress);
		TestEqual(TEXT("Days remaining are still known and shown"), Slot->DaysRemaining, 2);
		TestTrue(TEXT("The text falls back to days"), Slot->ProgressText.ToString().Contains(TEXT("2 days left")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 3. DE RUSHPRIJS (§2.3 rij 3) — en hij noemt het tekort
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewRushPriceTest,
	"Eclipse.UI.BaseViewRushPriceTracksRemainingDays",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewRushPriceTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	const TArray<FEclipseBaseSlotDef> Layout = MakeLayout();
	const TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = MakeTuning();

	// 3 dagen x 60 C = 180 C, en de beurs heeft 500.
	{
		FEclipseCampaignState State = MakeState();
		State.Wallet.Add(EclipseTags::Resource_Credits.GetTag(), 500);
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 0, 3);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestEqual(TEXT("Rush = 60 C per remaining day"), Slot->RushCostCredits, 180);
		TestTrue(TEXT("Rush is offered"), Slot->bCanRush);
		TestTrue(TEXT("And affordable at 500 C"), Slot->bRushAffordable);
	}

	// DE PRIJS DAALT MET DE DAGEN. Een rushprijs die niet meebeweegt, is geen
	// beslissing maar een tarief.
	{
		FEclipseCampaignState State = MakeState();
		State.Wallet.Add(EclipseTags::Resource_Credits.GetTag(), 500);
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 0, 1);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		TestEqual(TEXT("One day left costs one day"), Slot != nullptr ? Slot->RushCostCredits : -1, 60);
	}

	// ONBETAALBAAR IS NIET AFWEZIG: de knop hoort te bestaan en te weigeren,
	// mét het bedrag dat ontbreekt.
	{
		FEclipseCampaignState State = MakeState();
		State.Wallet.Add(EclipseTags::Resource_Credits.GetTag(), 120);
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 0, 3);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestTrue(TEXT("Rush is still offered"), Slot->bCanRush);
		TestFalse(TEXT("But not affordable"), Slot->bRushAffordable);
		TestTrue(TEXT("The shortfall is named, not left to the player to compute"),
			Slot->RushText.ToString().Contains(TEXT("60 short")));
	}

	// Een DRAAIENDE faciliteit heeft geen rush.
	{
		FEclipseCampaignState State = MakeState();
		State.Wallet.Add(EclipseTags::Resource_Credits.GetTag(), 500);
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		TestFalse(TEXT("Nothing to rush on an operational site"), Slot != nullptr && Slot->bCanRush);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 4. HET SLOTRASTER: bezet, vrij, vergrendeld (§2.3 rij 1)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewSlotGridTest,
	"Eclipse.UI.BaseViewSlotGridCountsScarcity",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewSlotGridTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	const TArray<FEclipseBaseSlotDef> Layout = MakeLayout();
	const TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = MakeTuning();

	// Trap 1: A bezet, B en C vrij, D vergrendeld.
	{
		const FEclipseCampaignState State = MakeState();
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning, /*Tier*/ 1);

		TestEqual(TEXT("One built"), View.SlotsOccupied, 1);
		TestEqual(TEXT("Two free"), View.SlotsFree, 2);
		TestEqual(TEXT("One sealed"), View.SlotsLocked, 1);

		const FEclipseBaseSlotView* Deep = FindSlot(View, TEXT("Slot_D"));
		TestTrue(TEXT("The tier-2 slot reads as Locked"), Deep != nullptr && Deep->Status == EEclipseSlotStatus::Locked);

		// Een vrij slot dat niet zegt wat het TOELAAT, is een gat en geen keuze.
		const FEclipseBaseSlotView* Bunks = FindSlot(View, TEXT("Slot_B"));
		TestTrue(TEXT("A free slot names what it accepts"),
			Bunks != nullptr && Bunks->StatusText.ToString().Contains(TEXT("Barracks")));
	}

	// DE TRAP LOOPT OP -> het vergrendelde slot wordt vrij. Zonder deze helft
	// zou "Locked" ook kunnen betekenen "altijd dicht".
	{
		const FEclipseCampaignState State = MakeState();
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning, /*Tier*/ 2);

		TestEqual(TEXT("At tier 2 nothing is sealed"), View.SlotsLocked, 0);
		TestEqual(TEXT("And there are three free slots"), View.SlotsFree, 3);
	}

	// VERGRENDELD MAG NOOIT EEN FACILITEIT VERBERGEN. Stond er toch iets op een
	// slot boven de trap, dan is dat zichtbaar — anders verdwijnt bezit achter
	// een presentatieregel.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_D"), TEXT("Workshop"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning, /*Tier*/ 1);

		const FEclipseBaseSlotView* Deep = FindSlot(View, TEXT("Slot_D"));
		if (Deep == nullptr)
		{
			AddError(TEXT("Slot_D missing"));
			return false;
		}
		TestFalse(TEXT("An occupied slot above the tier is NOT hidden as sealed"),
			Deep->Status == EEclipseSlotStatus::Locked);
		TestTrue(TEXT("It shows what it holds"), Deep->Status == EEclipseSlotStatus::Operational);
	}

	// De rasterplaatsing is vast, dus toetsbaar zonder viewport.
	{
		const FEclipseCampaignState State = MakeState();
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);
		if (View.Slots.Num() == 4)
		{
			TestEqual(TEXT("Slot 0 sits at column 0"), View.Slots[0].Column, 0);
			TestEqual(TEXT("Slot 3 sits at column 3"), View.Slots[3].Column, 3);
			TestEqual(TEXT("All four fit on one row at 4 columns"), View.Slots[3].Row, 0);
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// 5. DE SCHERPSTE: 0 / 0 IS GEEN GEZONDE BALANS
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewEnergyHonestyTest,
	"Eclipse.UI.BaseViewEnergyUnauthoredIsNotBalanced",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewEnergyHonestyTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	const TArray<FEclipseBaseSlotDef> Layout = MakeLayout();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = MakeTuning();

	// ZOALS DE SLICE HEM VANDAAG VERSCHEEPT: geen enkele energiekolom ingevuld.
	{
		const TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestTrue(TEXT("Unauthored energy reads as Unauthored, never as Surplus"),
			View.EnergyState == EEclipseEnergyState::Unauthored);
		TestFalse(TEXT("So there is no band to draw"), View.bHasEnergyBand);
		TestTrue(TEXT("And the screen says WHY, not '0 / 0'"),
			View.EnergyText.ToString().Contains(TEXT("not authored")));

		// DE ONAFHANKELIJKE POORT: het raster staat er gewoon.
		TestTrue(TEXT("Missing energy data does NOT blank the slot grid"), View.IsRenderable());
		TestEqual(TEXT("All four tiles still readable"), View.Slots.Num(), 4);
		TestFalse(TEXT("And the slot counts still read"), View.SlotCountText.IsEmpty());
	}

	// CONTROLEPROEF: zodra iemand de GDD 5.3.1-getallen wél authort, gaat de
	// band leven — zonder één regel codewijziging. Zonder deze helft bewijst
	// "Unauthored" alleen dat de band nooit werkt.
	{
		TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
		Rows[TEXT("CommandCenter")].Levels[0].EnergyUpkeep = 2;
		Rows[TEXT("Workshop")].Levels[0].EnergyUpkeep = 2;

		// Een Power Plant als leverancier.
		FEclipseFacilityRow& Plant = Rows.Add(TEXT("PowerPlant"));
		Plant.DisplayName = FText::FromString(TEXT("Power Plant"));
		Plant.Levels.AddDefaulted_GetRef().EnergyOutput = 10;

		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		AddFacility(State, TEXT("Slot_B"), TEXT("PowerPlant"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestTrue(TEXT("Authored energy produces a band"), View.bHasEnergyBand);
		TestEqual(TEXT("Draw is 2 + 2"), View.EnergyDraw, 4);
		TestEqual(TEXT("Supply is 10"), View.EnergySupply, 10);
		TestEqual(TEXT("Headroom is 6"), View.EnergyHeadroom, 6);
		TestTrue(TEXT("6 of 10 spare is Surplus"), View.EnergyState == EEclipseEnergyState::Surplus);
	}

	// TEKORT: je kunt jezelf in het donker bouwen, en dat hoort te blijken.
	{
		TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
		Rows[TEXT("CommandCenter")].Levels[0].EnergyUpkeep = 6;
		Rows[TEXT("Workshop")].Levels[0].EnergyUpkeep = 9;
		FEclipseFacilityRow& Plant = Rows.Add(TEXT("PowerPlant"));
		Plant.Levels.AddDefaulted_GetRef().EnergyOutput = 10;

		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		AddFacility(State, TEXT("Slot_B"), TEXT("PowerPlant"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestTrue(TEXT("15 draw against 10 supply is a Deficit"), View.EnergyState == EEclipseEnergyState::Deficit);
		TestEqual(TEXT("Five over"), View.EnergyHeadroom, -5);
		TestTrue(TEXT("The overrun is named"), View.EnergyText.ToString().Contains(TEXT("OVER CAPACITY")));
	}

	// EEN BOUWPLAATS TREKT NOG GEEN STROOM. Anders zou een basis in het donker
	// staan om iets dat nog niet bestaat.
	{
		TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
		Rows[TEXT("CommandCenter")].Levels[0].EnergyUpkeep = 2;
		Rows[TEXT("Workshop")].Levels[0].EnergyUpkeep = 9;

		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 0, 3);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestEqual(TEXT("Only the operational Command Center draws"), View.EnergyDraw, 2);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 6. SCHADE (§2.3 rij 7) — zichtbaar, en het kost echt iets
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewDamageTest,
	"Eclipse.UI.BaseViewDamageReadsWithoutClicking",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewDamageTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	const TArray<FEclipseBaseSlotDef> Layout = MakeLayout();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = MakeTuning();

	TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
	Rows[TEXT("CommandCenter")].Levels[0].EnergyUpkeep = 2;
	Rows[TEXT("Workshop")].Levels[0].EnergyUpkeep = 9;

	// CONTROLEPROEF: heel.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		TestEqual(TEXT("Control: nothing damaged"), View.DamagedCount, 0);
		TestTrue(TEXT("Control: no damage banner"), View.DamageText.IsEmpty());
		TestEqual(TEXT("Control: the workshop draws its 9"), View.EnergyDraw, 11);
	}

	// DEZELFDE basis met één vlag om.
	{
		FEclipseCampaignState State = MakeState();
		AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0).bDamaged = true;
		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestTrue(TEXT("The tile reads as Damaged, not Operational"), Slot->Status == EEclipseSlotStatus::Damaged);
		TestEqual(TEXT("Counted on the base banner"), View.DamagedCount, 1);
		TestTrue(TEXT("Visible without clicking anything"), View.DamageText.ToString().Contains(TEXT("OFFLINE")));
		TestTrue(TEXT("The tile says why"), Slot->StatusText.ToString().Contains(TEXT("until repaired")));

		// EN HET KOST ECHT IETS: een stilstaande faciliteit trekt geen stroom.
		// Zou hij dat wel doen, dan zou REPAREREN de balans verslechteren.
		TestEqual(TEXT("A dead facility draws no power"), View.EnergyDraw, 2);
	}

	// De vlag raakt ook de OPBRENGST, niet alleen het plaatje (GDD 5.4).
	{
		FEclipseCampaignState State = MakeState();
		FEclipseFacilityRow& Intel = Rows.Add(TEXT("IntelCenter"));
		Intel.Levels.AddDefaulted_GetRef().YieldPerDay.Add(EclipseTags::Resource_Intel.GetTag(), 2);

		FEclipseFacilityState& Facility = AddFacility(State, TEXT("Slot_C"), TEXT("IntelCenter"), 1, 0);
		auto RowResolver = [&Rows](FName Id) -> const FEclipseFacilityRow* { return Rows.Find(Id); };

		const int32 Healthy = EclipseBaseLogic::ComputeFacilityYields(State.BaseState, Tuning, RowResolver)
			.YieldPerDay.FindRef(EclipseTags::Resource_Intel.GetTag());
		TestEqual(TEXT("Control: an intact IC yields 2 intel"), Healthy, 2);

		Facility.bDamaged = true;
		const int32 Broken = EclipseBaseLogic::ComputeFacilityYields(State.BaseState, Tuning, RowResolver)
			.YieldPerDay.FindRef(EclipseTags::Resource_Intel.GetTag());
		TestEqual(TEXT("A damaged IC yields nothing — the flag costs the player a day"), Broken, 0);
	}

	return true;
}

// ---------------------------------------------------------------------------
// 7. BEMANNING ALS SCHAARSTE (§2.3 rij 4)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewCrewTest,
	"Eclipse.UI.BaseViewCrewIsScarcityNotAStatistic",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewCrewTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	const TArray<FEclipseBaseSlotDef> Layout = MakeLayout();
	const TMap<FName, FEclipseFacilityRow> Rows = MakeFacilities();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = MakeTuning();

	{
		FEclipseCampaignState State = MakeState();
		FEclipseFacilityState& Workshop = AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 0, 3);
		Workshop.AssignedSoldierIds.Add(State.Roster[0].SoldierId);

		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);

		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestEqual(TEXT("One crew on the site"), Slot->Crew.Num(), 1);
		TestTrue(TEXT("By NAME, not by guid"), Slot->CrewText.ToString().Contains(TEXT("Vasquez")));
		TestTrue(TEXT("A building site takes a CREW, not staff (positional role)"), Slot->bCrewIsConstruction);
		TestTrue(TEXT("And the tile says CREW"), Slot->CrewText.ToString().Contains(TEXT("CREW")));

		// DE SCHAARSTE: de noemer telt de toegewezene mee. Zou hij dat niet
		// doen, dan daalt de pool zodra je iemand toewijst en LIJKT de druk af
		// te nemen precies wanneer hij toeneemt.
		TestEqual(TEXT("One assigned"), View.CrewAssigned, 1);
		TestEqual(TEXT("Pool stays 3 — assigning does not shrink the roster"), View.CrewPool, 3);
		TestTrue(TEXT("The band reads as pressure"), View.CrewText.ToString().Contains(TEXT("1 of 3 assigned")));
	}

	// EEN DRAAIENDE post neemt STAFF, geen bouwploeg — twee verschillende
	// beslissingen, dus twee verschillende woorden.
	{
		FEclipseCampaignState State = MakeState();
		FEclipseFacilityState& Workshop = AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		Workshop.AssignedSoldierIds.Add(State.Roster[1].SoldierId);

		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);
		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		TestFalse(TEXT("An operational site is not a construction crew"), Slot != nullptr && Slot->bCrewIsConstruction);
		TestTrue(TEXT("It reads STAFF"), Slot != nullptr && Slot->CrewText.ToString().Contains(TEXT("STAFF")));
	}

	// DRIFT WORDT GETOOND, NIET VERZWEGEN: een toewijzing naar iemand die het
	// rooster niet kent, verdwijnt niet stil van de tegel.
	{
		FEclipseCampaignState State = MakeState();
		FEclipseFacilityState& Workshop = AddFacility(State, TEXT("Slot_C"), TEXT("Workshop"), 1, 0);
		Workshop.AssignedSoldierIds.Add(FGuid::NewGuid());

		const FEclipseBaseView View = Compose(State, Layout, Rows, Tuning);
		const FEclipseBaseSlotView* Slot = FindSlot(View, TEXT("Slot_C"));
		if (Slot == nullptr)
		{
			AddError(TEXT("Slot_C missing"));
			return false;
		}
		TestEqual(TEXT("The post still shows as staffed"), Slot->Crew.Num(), 1);
		TestFalse(TEXT("But the soldier is flagged as unknown"), Slot->Crew[0].bInRoster);
		TestTrue(TEXT("And the tile says so"), Slot->CrewText.ToString().Contains(TEXT("unknown")));
	}

	return true;
}

// ---------------------------------------------------------------------------
// 8. DE OPNAMETOESTAND DRAAGT ALLE VIJF DE VORMEN
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseBaseViewReviewStateTest,
	"Eclipse.UI.BaseViewReviewStateCarriesEveryShape",
	EclipseBaseViewTest::TestFlags)

bool FEclipseBaseViewReviewStateTest::RunTest(const FString& Parameters)
{
	using namespace EclipseBaseViewTest;

	// De opnameronde fotografeert deze toestand omdat een campagne op dag 1 maar
	// TWEE van de vijf tegelvormen kan tonen. Die belofte hoort vastgepind: zakt
	// hij stil weg, dan blijft de opnameronde draaien en fotografeert hij drie
	// vormen minder zonder dat iemand het merkt.
	const FEclipseBaseView View = EclipseBaseView::MakeReviewView();

	TestTrue(TEXT("The review state composes"), View.IsRenderable());
	TestEqual(TEXT("Five tiles"), View.Slots.Num(), 5);

	TSet<EEclipseSlotStatus> Seen;
	for (const FEclipseBaseSlotView& Slot : View.Slots)
	{
		Seen.Add(Slot.Status);
	}
	for (const EEclipseSlotStatus Status : {
		EEclipseSlotStatus::Locked, EEclipseSlotStatus::Empty,
		EEclipseSlotStatus::UnderConstruction, EEclipseSlotStatus::Operational,
		EEclipseSlotStatus::Damaged })
	{
		TestTrue(*FString::Printf(TEXT("Shape present: %s"), *EclipseBaseView::SlotStatusText(Status).ToString()),
			Seen.Contains(Status));
	}

	// En de drie banden die op dag 1 leeg of onbekend zijn, staan er ook op.
	TestTrue(TEXT("The energy band is authored here, so it draws"), View.bHasEnergyBand);
	TestTrue(TEXT("A building power plant supplies nothing yet — the base is in deficit"),
		View.EnergyState == EEclipseEnergyState::Deficit);
	TestEqual(TEXT("Only the operational Command Center draws"), View.EnergyDraw, 2);
	TestEqual(TEXT("...and the DAMAGED workshop draws nothing (repairing must not worsen the balance)"),
		View.EnergySupply, 0);
	TestEqual(TEXT("One facility offline"), View.DamagedCount, 1);
	TestTrue(TEXT("A rush price is on screen"),
		View.Slots.ContainsByPredicate([](const FEclipseBaseSlotView& S) { return S.bCanRush && S.bRushAffordable; }));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
