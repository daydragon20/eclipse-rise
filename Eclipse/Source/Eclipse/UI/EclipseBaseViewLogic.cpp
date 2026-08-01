#include "UI/EclipseBaseViewLogic.h"

#include "Core/EclipseGameplayTags.h"
#include "Internationalization/Text.h"

#define LOCTEXT_NAMESPACE "EclipseBaseView"

namespace EclipseBaseView
{
namespace
{
	/** Naam uit de data, of de id als niemand er een naam bij heeft gezet. */
	FText NameOrId(const FText& Authored, FName Id)
	{
		return Authored.IsEmpty() ? FText::FromName(Id) : Authored;
	}

	/**
	 * Een guid die het rooster niet kent, kort genoeg om op een tegel te passen.
	 * Bewust NIET weggelaten: een lege bemanningsregel op een bemande post zou
	 * de drift verbergen in plaats van hem te melden.
	 */
	FText UnknownSoldierText(const FGuid& SoldierId)
	{
		return FText::Format(
			LOCTEXT("UnknownSoldier", "<unknown {0}>"),
			FText::FromString(SoldierId.ToString(EGuidFormats::Digits).Left(6)));
	}
}

FText SlotStatusText(EEclipseSlotStatus Status)
{
	switch (Status)
	{
	case EEclipseSlotStatus::Locked:            return LOCTEXT("SlotSealed", "SEALED");
	case EEclipseSlotStatus::Empty:             return LOCTEXT("SlotEmpty", "EMPTY");
	case EEclipseSlotStatus::UnderConstruction: return LOCTEXT("SlotBuilding", "BUILDING");
	case EEclipseSlotStatus::Damaged:           return LOCTEXT("SlotOffline", "OFFLINE");
	default:                                    return LOCTEXT("SlotOnline", "ONLINE");
	}
}

FEclipseBaseView ComposeBaseView(
	const FEclipseCampaignState& State,
	TConstArrayView<FEclipseBaseSlotDef> Slots,
	const EclipseBaseLogic::FEclipseBaseTuningParams& Tuning,
	EclipseBaseLogic::FEclipseFacilityRowResolver FindFacilityRow,
	FEclipseSoldierResolver FindSoldier,
	int32 AvailableSlotTier,
	int32 ColumnsPerRow)
{
	FEclipseBaseView View;
	View.Day = State.Day;
	View.HeaderText = FText::Format(
		LOCTEXT("BaseHeader", "HOLLOW POINT · day {0}"),
		FText::AsNumber(State.Day));

	// ---- DE DATAPOORT --------------------------------------------------
	//
	// Staat vooraan en niet halverwege, precies zoals `ComposeMapView` zijn
	// validator vooraan zet: een raster dat half gevuld raakt voordat de
	// tegenspraak opvalt, heeft de speler al iets getoond.

	if (Slots.IsEmpty())
	{
		View.DataState = EEclipseBaseDataState::Absent;
		View.StatusText = LOCTEXT("BaseNoLayout", "No base layout loaded — DA_BaseLayout is missing from the campaign setup.");
		return View;
	}

	TSet<FName> SeenSlotIds;
	for (const FEclipseBaseSlotDef& Slot : Slots)
	{
		if (Slot.SlotId.IsNone())
		{
			View.Errors.Add(TEXT("Layout carries a slot with no id."));
		}
		else if (SeenSlotIds.Contains(Slot.SlotId))
		{
			View.Errors.Add(FString::Printf(TEXT("Layout declares slot '%s' twice."), *Slot.SlotId.ToString()));
		}
		SeenSlotIds.Add(Slot.SlotId);
	}

	// Een faciliteit op een slot dat de layout niet kent, is de fout waar deze
	// poort voor bestaat: hij bestaat, hij kost, hij is bemand — en hij komt
	// NERGENS op het raster. Een raster dat zoiets stil weglaat, liegt over wat
	// je bezit.
	TSet<FName> SeenFacilitySlots;
	for (const FEclipseFacilityState& Facility : State.BaseState.Facilities)
	{
		if (!SeenSlotIds.Contains(Facility.SlotId))
		{
			View.Errors.Add(FString::Printf(
				TEXT("Facility '%s' sits at slot '%s', which the layout does not declare."),
				*Facility.FacilityId.ToString(), *Facility.SlotId.ToString()));
		}
		if (SeenFacilitySlots.Contains(Facility.SlotId))
		{
			View.Errors.Add(FString::Printf(TEXT("Slot '%s' holds more than one facility."), *Facility.SlotId.ToString()));
		}
		SeenFacilitySlots.Add(Facility.SlotId);
	}

	if (!View.Errors.IsEmpty())
	{
		View.DataState = EEclipseBaseDataState::Invalid;
		View.StatusText = FText::Format(
			LOCTEXT("BaseInvalid", "Base data contradicts itself ({0} problems) — the slot grid is withheld rather than drawn wrong."),
			FText::AsNumber(View.Errors.Num()));
		return View;
	}

	View.DataState = EEclipseBaseDataState::Valid;

	// ---- HET RASTER ----------------------------------------------------

	const int32 SafeColumns = FMath::Max(1, ColumnsPerRow);
	int32 Index = 0;
	bool bAnyEnergyAuthored = false;
	bool bAnyCrewablePost = false;

	/** Toewijzingen die het rooster niet als Available kent — zie de noemer onderaan. */
	int32 AssignedOutsideRoster = 0;

	for (const FEclipseBaseSlotDef& SlotDef : Slots)
	{
		FEclipseBaseSlotView Slot;
		Slot.SlotId = SlotDef.SlotId;
		Slot.SlotName = NameOrId(SlotDef.DisplayName, SlotDef.SlotId);
		Slot.UnlockTier = FMath::Max(1, SlotDef.UnlockTier);
		Slot.Column = Index % SafeColumns;
		Slot.Row = Index / SafeColumns;
		++Index;

		const FEclipseFacilityState* Facility = State.BaseState.FindBySlot(SlotDef.SlotId);

		// VERGRENDELD WINT VAN ALLES. Een slot boven de bereikte trap hoort niet
		// te bestaan; stond er toch iets, dan is dat een datafout die de
		// datapoort niet ziet (de layout kent het slot immers wel). Hem hier als
		// vergrendeld tonen zou de faciliteit verbergen, dus dat gebeurt alleen
		// als hij ECHT leeg is.
		if (Slot.UnlockTier > AvailableSlotTier && Facility == nullptr)
		{
			Slot.Status = EEclipseSlotStatus::Locked;
			Slot.HeaderText = LOCTEXT("SlotSealedHeader", "SEALED ROCK");
			Slot.StatusText = FText::Format(
				LOCTEXT("SlotSealedStatus", "excavation tier {0}"),
				FText::AsNumber(Slot.UnlockTier));
			++View.SlotsLocked;
			View.Slots.Add(MoveTemp(Slot));
			continue;
		}

		if (Facility == nullptr || Facility->FacilityId.IsNone())
		{
			Slot.Status = EEclipseSlotStatus::Empty;
			Slot.HeaderText = LOCTEXT("SlotEmptyHeader", "EMPTY");

			// Wat het slot TOELAAT is de reden dat een vrij slot een keuze is en
			// geen gat. Zonder die regel is "empty" alleen een mededeling.
			if (SlotDef.AllowedFacilityRows.IsEmpty())
			{
				Slot.StatusText = LOCTEXT("SlotAcceptsNothing", "accepts nothing — no facility is authored for this slot");
			}
			else
			{
				TArray<FText> Allowed;
				for (const FName& RowId : SlotDef.AllowedFacilityRows)
				{
					const FEclipseFacilityRow* Row = FindFacilityRow(RowId);
					Allowed.Add(Row != nullptr ? NameOrId(Row->DisplayName, RowId) : FText::FromName(RowId));
				}
				Slot.StatusText = FText::Format(
					LOCTEXT("SlotAccepts", "accepts {0}"),
					FText::Join(FText::FromString(TEXT(", ")), Allowed));
			}
			++View.SlotsFree;
			View.Slots.Add(MoveTemp(Slot));
			continue;
		}

		// ---- BEZET: faciliteit, niveau, en wat hij nu doet ----------------

		Slot.FacilityId = Facility->FacilityId;
		Slot.Level = Facility->Level;

		const FEclipseFacilityRow* Row = FindFacilityRow(Facility->FacilityId);
		Slot.FacilityName = Row != nullptr ? NameOrId(Row->DisplayName, Facility->FacilityId) : FText::FromName(Facility->FacilityId);
		Slot.MaxLevel = Row != nullptr ? Row->Levels.Num() : 0;

		const bool bBuilding = Facility->DaysRemaining > 0;
		if (bBuilding)
		{
			Slot.Status = EEclipseSlotStatus::UnderConstruction;
		}
		else if (Facility->bDamaged)
		{
			Slot.Status = EEclipseSlotStatus::Damaged;
			++View.DamagedCount;
		}
		else
		{
			Slot.Status = EEclipseSlotStatus::Operational;
		}
		++View.SlotsOccupied;

		Slot.HeaderText = Facility->Level > 0
			? FText::Format(LOCTEXT("SlotFacilityLevel", "{0} L{1}"), Slot.FacilityName, FText::AsNumber(Facility->Level))
			: Slot.FacilityName;

		// ---- BOUW-ETA ALS VOORTGANG ---------------------------------------

		if (bBuilding)
		{
			Slot.DaysRemaining = Facility->DaysRemaining;
			Slot.TargetLevel = Facility->Level + 1;

			// Het totaal komt uit de data van het niveau dat GEBOUWD wordt, niet
			// van het huidige. StartConstruction zet DaysRemaining op
			// max(1, BuildDays), dus dat is hier de eerlijke noemer.
			const FEclipseFacilityLevelData* TargetLevelData = EclipseBaseLogic::GetLevelData(Row, Slot.TargetLevel);
			if (TargetLevelData != nullptr)
			{
				Slot.TotalDays = FMath::Max(1, TargetLevelData->BuildDays);
			}

			// DE POORT ONDER DE BALK. Zonder totaal is er geen breuk, en een
			// balk op een geraden noemer is precies de leugen die een
			// voortgangsbalk niet mag vertellen. Dan blijven de dagen staan en
			// verdwijnt alleen de balk.
			//
			// Ook geen balk als de bouw al langer duurt dan de data zegt: dat
			// KAN (een bemande site die met de klok mee schuift, of hergebalanceerde
			// data onder een lopende save), en een negatieve voortgang is een
			// getal dat niemand kan lezen.
			if (Slot.TotalDays > 0 && Slot.DaysRemaining <= Slot.TotalDays)
			{
				Slot.bHasProgress = true;
				const int32 DaysDone = Slot.TotalDays - Slot.DaysRemaining;
				Slot.Progress01 = FMath::Clamp(static_cast<float>(DaysDone) / static_cast<float>(Slot.TotalDays), 0.0f, 1.0f);
				Slot.ProgressText = FText::Format(
					LOCTEXT("SlotBuildProgress", "BUILDING L{0} — day {1} of {2}"),
					FText::AsNumber(Slot.TargetLevel),
					FText::AsNumber(DaysDone),
					FText::AsNumber(Slot.TotalDays));
			}
			else
			{
				// Meervoud met de hand en niet via |plural(): dat pluralformaat
				// wil een numeriek argument, en FText::AsNumber levert al Text —
				// dan valt de keuze stil terug op "other" en staat er ooit
				// "1 days". Twee regels zijn goedkoper dan die klasse fout.
				Slot.ProgressText = Slot.DaysRemaining == 1
					? FText::Format(LOCTEXT("SlotBuildOneDayLeft", "BUILDING L{0} — 1 day left"),
						FText::AsNumber(Slot.TargetLevel))
					: FText::Format(LOCTEXT("SlotBuildDaysLeft", "BUILDING L{0} — {1} days left"),
						FText::AsNumber(Slot.TargetLevel),
						FText::AsNumber(Slot.DaysRemaining));
			}

			// ---- RUSH: de beslissing die op dit scherm valt -----------------

			Slot.RushCostCredits = EclipseBaseLogic::ComputeRushCost(Facility, Tuning);
			Slot.bCanRush = Slot.RushCostCredits > 0;

			const int32 Credits = State.GetBalance(EclipseTags::Resource_Credits.GetTag());
			Slot.bRushAffordable = Slot.bCanRush && Credits >= Slot.RushCostCredits;

			if (Slot.bCanRush)
			{
				// De onbetaalbare variant NOEMT HET TEKORT. "Te duur" laat de
				// speler rekenen; "60 short" is de beslissing zelf.
				Slot.RushText = Slot.bRushAffordable
					? FText::Format(LOCTEXT("SlotRush", "RUSH {0} C"), FText::AsNumber(Slot.RushCostCredits))
					: FText::Format(LOCTEXT("SlotRushShort", "RUSH {0} C — {1} short"),
						FText::AsNumber(Slot.RushCostCredits),
						FText::AsNumber(Slot.RushCostCredits - Credits));
			}
		}
		else if (Slot.Status == EEclipseSlotStatus::Damaged)
		{
			Slot.ProgressText = LOCTEXT("SlotDamaged", "DAMAGED — offline until repaired");
		}

		// ---- BEMANNING ----------------------------------------------------

		Slot.bCrewIsConstruction = bBuilding;
		Slot.CrewCap = FMath::Max(0, Tuning.MaxCrewPerSite);
		bAnyCrewablePost = bAnyCrewablePost || Slot.CrewCap > 0;

		for (const FGuid& SoldierId : Facility->AssignedSoldierIds)
		{
			FEclipseBaseCrewView Crew;
			Crew.SoldierId = SoldierId;
			const FEclipseSoldierRecord* Record = FindSoldier(SoldierId);
			Crew.bInRoster = Record != nullptr;
			Crew.Name = Record != nullptr && !Record->Name.IsEmpty()
				? FText::FromString(Record->Name)
				: UnknownSoldierText(SoldierId);

			// Zie de noemer hieronder: alleen toewijzingen die NIET als
			// Available in het rooster staan, moeten er nog bij geteld worden.
			if (Record == nullptr || Record->Status != EEclipseSoldierStatus::Available)
			{
				++AssignedOutsideRoster;
			}

			Slot.Crew.Add(MoveTemp(Crew));
		}

		View.CrewAssigned += Slot.Crew.Num();
		View.CrewPostsOpen += FMath::Max(0, Slot.CrewCap - Slot.Crew.Num());

		{
			const FText RoleText = Slot.bCrewIsConstruction
				? LOCTEXT("RoleCrew", "CREW")
				: LOCTEXT("RoleAnalyst", "STAFF");
			if (Slot.Crew.IsEmpty())
			{
				Slot.CrewText = FText::Format(
					LOCTEXT("SlotUnstaffed", "{0} 0/{1} — unstaffed"),
					RoleText, FText::AsNumber(Slot.CrewCap));
			}
			else
			{
				TArray<FText> Names;
				for (const FEclipseBaseCrewView& Crew : Slot.Crew)
				{
					Names.Add(Crew.Name);
				}
				Slot.CrewText = FText::Format(
					LOCTEXT("SlotStaffed", "{0} {1}/{2} — {3}"),
					RoleText,
					FText::AsNumber(Slot.Crew.Num()),
					FText::AsNumber(Slot.CrewCap),
					FText::Join(FText::FromString(TEXT(", ")), Names));
			}
		}

		// ---- ENERGIE, per slot --------------------------------------------

		const FEclipseFacilityLevelData* CurrentLevelData = EclipseBaseLogic::GetLevelData(Row, Facility->Level);
		if (CurrentLevelData != nullptr)
		{
			Slot.EnergyUpkeep = FMath::Max(0, CurrentLevelData->EnergyUpkeep);
			Slot.EnergyOutput = FMath::Max(0, CurrentLevelData->EnergyOutput);
			bAnyEnergyAuthored = bAnyEnergyAuthored || Slot.EnergyUpkeep > 0 || Slot.EnergyOutput > 0;

			// Alleen wat DRAAIT telt mee. Een bouwplaats verbruikt nog niets en
			// een beschadigde faciliteit staat stil (GDD 5.4) — die zou anders
			// stroom trekken die hij niet gebruikt, en dan zou repareren de
			// balans verslechteren in plaats van verbeteren.
			if (Slot.Status == EEclipseSlotStatus::Operational)
			{
				View.EnergyDraw += Slot.EnergyUpkeep;
				View.EnergySupply += Slot.EnergyOutput;
			}
		}

		if (Slot.StatusText.IsEmpty())
		{
			Slot.StatusText = Slot.ProgressText.IsEmpty() ? SlotStatusText(Slot.Status) : Slot.ProgressText;
		}

		View.Slots.Add(MoveTemp(Slot));
	}

	View.SlotCountText = FText::Format(
		LOCTEXT("SlotCounts", "SLOTS {0} built · {1} free · {2} sealed"),
		FText::AsNumber(View.SlotsOccupied),
		FText::AsNumber(View.SlotsFree),
		FText::AsNumber(View.SlotsLocked));

	// ---- DE ENERGIEPOORT (eigen poort, raakt het raster niet) ------------

	View.EnergyHeadroom = View.EnergySupply - View.EnergyDraw;
	if (!bAnyEnergyAuthored)
	{
		// HET EERLIJKE ANTWOORD OP EEN NIET-GESTELDE VRAAG. GDD 5.3.1 heeft de
		// kolom "Energy upkeep" met getallen per faciliteit per niveau, maar de
		// sliceverzameling authort ze niet en er is geen Power Plant. "0 / 0" als
		// gezonde balans tonen zou een economie verzinnen die niemand heeft
		// ontworpen — en dat ziet er precies zo uit als een economie die klopt.
		View.EnergyState = EEclipseEnergyState::Unauthored;
		View.bHasEnergyBand = false;
		View.EnergyText = LOCTEXT("EnergyUnauthored", "POWER — not authored (GDD 5.3.1 upkeep is not in the facility data)");
	}
	else
	{
		View.bHasEnergyBand = true;
		if (View.EnergyHeadroom < 0)
		{
			View.EnergyState = EEclipseEnergyState::Deficit;
		}
		else if (View.EnergySupply > 0 && View.EnergyHeadroom * 100 < View.EnergySupply * TightHeadroomPercent)
		{
			View.EnergyState = EEclipseEnergyState::Tight;
		}
		else
		{
			View.EnergyState = EEclipseEnergyState::Surplus;
		}

		View.EnergyText = View.EnergyState == EEclipseEnergyState::Deficit
			? FText::Format(LOCTEXT("EnergyDeficit", "POWER {0} / {1} — {2} OVER CAPACITY"),
				FText::AsNumber(View.EnergyDraw), FText::AsNumber(View.EnergySupply), FText::AsNumber(-View.EnergyHeadroom))
			: FText::Format(LOCTEXT("EnergyBand", "POWER {0} / {1}"),
				FText::AsNumber(View.EnergyDraw), FText::AsNumber(View.EnergySupply));
	}

	// ---- DE BEMANNINGSPOORT (eigen poort, raakt het raster niet) ---------

	int32 AvailableSoldiers = 0;
	for (const FEclipseSoldierRecord& Soldier : State.Roster)
	{
		if (Soldier.Status == EEclipseSoldierStatus::Available)
		{
			++AvailableSoldiers;
		}
	}
	// DE NOEMER, en hier zat een dubbeltelling die de test ving.
	//
	// Ik schreef eerst `AvailableSoldiers + CrewAssigned`, in de veronderstelling
	// dat een toegewezen soldaat uit de beschikbare pool valt. Dat is NIET zo:
	// toewijzing leeft in de basistoestand en laat het roosterrecord ongemoeid
	// (SPEC-P2-03 staffing v1 — muster leest de basistoestand, niet de status).
	// Een toegewezen soldaat staat dus nog steeds op Available en werd twee keer
	// geteld: 3 soldaten met 1 toewijzing lazen als "1 of 4".
	//
	// Dat is precies de verkeerde kant op liegen. De bedoeling van deze band is
	// dat de noemer STIL BLIJFT STAAN als je iemand toewijst — de druk zit in de
	// teller die oploopt. Een noemer die meegroeit, verbergt de schaarste die hij
	// hoort te tonen.
	//
	// Alleen toewijzingen die uit Available zijn gevallen (drift: gewond, dood,
	// of helemaal niet in het rooster) komen er nog bij, zodat de teller nooit
	// boven de noemer uit kan komen.
	View.CrewPool = AvailableSoldiers + AssignedOutsideRoster;
	View.bHasCrewBand = bAnyCrewablePost;
	// Meervoud met de hand, om dezelfde reden als bij de bouwdagen: |plural() wil
	// een numeriek argument en FText::AsNumber levert al Text. GEMETEN op
	// `HUD_hub_faciliteiten.png` (01-08) stond er letterlijk "1 posts open".
	View.CrewText = View.bHasCrewBand
		? FText::Format(
			View.CrewPostsOpen == 1
				? LOCTEXT("CrewBandOnePost", "CREW {0} of {1} assigned · {2} post open")
				: LOCTEXT("CrewBandPosts", "CREW {0} of {1} assigned · {2} posts open"),
			FText::AsNumber(View.CrewAssigned), FText::AsNumber(View.CrewPool), FText::AsNumber(View.CrewPostsOpen))
		: FText::GetEmpty();

	// ---- SCHADE ----------------------------------------------------------

	if (View.DamagedCount > 0)
	{
		View.DamageText = View.DamagedCount == 1
			? LOCTEXT("DamageBandOne", "1 FACILITY OFFLINE — damaged")
			: FText::Format(LOCTEXT("DamageBandMany", "{0} FACILITIES OFFLINE — damaged"),
				FText::AsNumber(View.DamagedCount));
	}

	return View;
}

FEclipseBaseView MakeReviewView()
{
	// Vijf slots, één per toestand, mét energie geauthord zodat OOK de band te
	// zien is die de verscheepte data vandaag niet kan vullen. Deze getallen
	// zijn de GDD 5.3.1-waarden; ze staan hier omdat dit een demonstratie is en
	// niet de economie — de echte plek is DT_Facilities (GDD 14.2).
	static TMap<FName, FEclipseFacilityRow> Rows;
	Rows.Reset();

	auto AddRow = [](const TCHAR* Id, const TCHAR* Name, std::initializer_list<TTuple<int32, int32, int32>> Levels)
	{
		FEclipseFacilityRow& Row = Rows.Add(FName(Id));
		Row.DisplayName = FText::FromString(Name);
		for (const TTuple<int32, int32, int32>& Level : Levels)
		{
			FEclipseFacilityLevelData& Data = Row.Levels.AddDefaulted_GetRef();
			Data.BuildDays = Level.Get<0>();
			Data.EnergyUpkeep = Level.Get<1>();
			Data.EnergyOutput = Level.Get<2>();
		}
	};
	AddRow(TEXT("CommandCenter"), TEXT("Command Center"), { { 0, 2, 0 }, { 3, 4, 0 } });
	AddRow(TEXT("PowerPlant"), TEXT("Power Plant"), { { 2, 0, 10 } });
	AddRow(TEXT("Workshop"), TEXT("Workshop"), { { 3, 2, 0 }, { 4, 5, 0 } });
	AddRow(TEXT("Medbay"), TEXT("Medbay"), { { 3, 2, 0 } });

	TArray<FEclipseBaseSlotDef> Slots;
	auto AddSlot = [&Slots](const TCHAR* Id, const TCHAR* Name, const TCHAR* Allowed, int32 Tier)
	{
		FEclipseBaseSlotDef& Slot = Slots.AddDefaulted_GetRef();
		Slot.SlotId = FName(Id);
		Slot.DisplayName = FText::FromString(Name);
		Slot.UnlockTier = Tier;
		Slot.AllowedFacilityRows.Add(FName(Allowed));
	};
	AddSlot(TEXT("Slot_A"), TEXT("Command"), TEXT("CommandCenter"), 1);
	AddSlot(TEXT("Slot_B"), TEXT("Generator Room"), TEXT("PowerPlant"), 1);
	AddSlot(TEXT("Slot_C"), TEXT("Machine Floor"), TEXT("Workshop"), 1);
	AddSlot(TEXT("Slot_D"), TEXT("Infirmary"), TEXT("Medbay"), 1);
	AddSlot(TEXT("Slot_E"), TEXT("Deep Gallery"), TEXT("Workshop"), 2);

	FEclipseCampaignState State;
	State.Day = 12;
	State.Wallet.Add(EclipseTags::Resource_Credits.GetTag(), 140);
	State.BaseState.Facilities.Reset();

	auto AddFacility = [&State](const TCHAR* SlotId, const TCHAR* FacilityId, int32 Level, int32 DaysLeft, bool bDamaged)
		-> FEclipseFacilityState&
	{
		FEclipseFacilityState& Facility = State.BaseState.Facilities.AddDefaulted_GetRef();
		Facility.SlotId = FName(SlotId);
		Facility.FacilityId = FName(FacilityId);
		Facility.Level = Level;
		Facility.DaysRemaining = DaysLeft;
		Facility.bDamaged = bDamaged;
		return Facility;
	};

	// ONLINE, BUILDING (met rushprijs die de beurs NET niet dekt), DAMAGED,
	// EMPTY en SEALED — in die volgorde op het raster.
	AddFacility(TEXT("Slot_A"), TEXT("CommandCenter"), 1, 0, false);
	AddFacility(TEXT("Slot_B"), TEXT("PowerPlant"), 0, 2, false);
	AddFacility(TEXT("Slot_C"), TEXT("Workshop"), 1, 0, true);

	for (const TCHAR* Name : { TEXT("Vasquez"), TEXT("Otoye"), TEXT("Brandt"), TEXT("Sela") })
	{
		FEclipseSoldierRecord& Soldier = State.Roster.AddDefaulted_GetRef();
		Soldier.SoldierId = FGuid::NewGuid();
		Soldier.Name = Name;
		Soldier.Status = EEclipseSoldierStatus::Available;
	}
	// Eén bemande bouwplaats, zodat de bemanningsregel ook op een tegel staat.
	State.BaseState.Facilities[1].AssignedSoldierIds.Add(State.Roster[0].SoldierId);

	EclipseBaseLogic::FEclipseBaseTuningParams Tuning;
	Tuning.RushCostCreditsPerDay = 60;
	Tuning.MaxCrewPerSite = 1;

	auto RowResolver = [](FName Id) -> const FEclipseFacilityRow* { return Rows.Find(Id); };
	auto SoldierResolver = [&State](const FGuid& Id) -> const FEclipseSoldierRecord* { return State.FindSoldier(Id); };
	return ComposeBaseView(State, Slots, Tuning, RowResolver, SoldierResolver, /*AvailableSlotTier*/ 1, /*ColumnsPerRow*/ 5);
}

} // namespace EclipseBaseView

#undef LOCTEXT_NAMESPACE
