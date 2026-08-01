#include "UI/EclipseStrategyMapLogic.h"

#define LOCTEXT_NAMESPACE "EclipseStrategyMap"

namespace EclipseStrategyMap
{

namespace
{
	/**
	 * Een getal zonder duizendtalscheiding. `FText::AsNumber` zet er in sommige
	 * locales een punt of spatie in, en "1.200 risk" op een lane leest als iets
	 * anders dan het is. Zelfde reden als in EclipseHudReadoutLogic.
	 */
	FText Num(int32 Value)
	{
		FNumberFormattingOptions Options;
		Options.SetUseGrouping(false);
		return FText::AsNumber(Value, &Options);
	}

	/** De leesbare naam uit de data, of de id als de data er geen draagt (14.3.5: degradeer leesbaar). */
	FText NameOf(const TArray<FEclipseRegionDefinition>& Definitions, FName RegionId)
	{
		const FEclipseRegionDefinition* Definition = Definitions.FindByPredicate(
			[RegionId](const FEclipseRegionDefinition& D) { return D.RegionId == RegionId; });
		if (Definition != nullptr && !Definition->DisplayName.IsEmpty())
		{
			return Definition->DisplayName;
		}
		return FText::FromName(RegionId);
	}

	/**
	 * De statusaanduiding zoals hij op de lane-regel komt. SpireGated draagt de
	 * poort en de eigenaar van die poort mee, want dat is precies het feit dat
	 * de lane vandaag dicht of open houdt — "SPIRE-GATED" alleen zegt niet of
	 * je er nu doorheen kunt.
	 */
	FText StatusClause(const FEclipseMapLaneView& Lane)
	{
		if (Lane.Status != EEclipseLaneStatus::SpireGated)
		{
			return LaneStatusText(Lane.Status);
		}
		return FText::Format(
			LOCTEXT("LaneStatusGated", "SPIRE-GATED by {0} ({1})"),
			FText::FromName(Lane.GateRegionId),
			OwnerText(Lane.GateOwner));
	}

	/**
	 * Twee knopen op dezelfde plek zijn één knoop op het scherm. Onder deze
	 * afstand (in genormaliseerde bordruimte, dus ~2,5 % van de bordbreedte)
	 * overlappen de knooplabels en is de lijn ertussen korter dan de knopen
	 * zelf — dan tekent het bord een leugen over wie waar ligt.
	 */
	constexpr double MinNodeSeparation = 0.025;

	/**
	 * DE LIJST NOG EEN KEER LEZEN, NU ALS VORM.
	 *
	 * Twee dingen gebeuren hier en verder nergens:
	 *   1. de kanten worden ONTDUBBELD (elke ongerichte lane staat in beide
	 *      regio's; een lijn hoort er één keer te zijn);
	 *   2. de tekenpoort gaat open of dicht, en als hij dicht gaat staat er WAAROM.
	 *
	 * De poort is opzettelijk streng: één regio zonder positie betekent geen
	 * graaf, geen half bord. Een halve graaf is erger dan geen graaf, want een
	 * ontbrekende knoop leest als "die regio grenst nergens aan" — precies de
	 * verkeerde conclusie, en niet van een fout te onderscheiden.
	 */
	void BuildLayout(FEclipseMapView& View)
	{
		TArray<FName> Unplaced;
		for (const FEclipseMapRegionView& Region : View.Regions)
		{
			if (!Region.bHasBoardPosition)
			{
				Unplaced.Add(Region.RegionId);
			}
		}

		if (!Unplaced.IsEmpty())
		{
			TArray<FText> Names;
			for (FName Id : Unplaced)
			{
				Names.Add(FText::FromName(Id));
			}
			View.LayoutStatusText = FText::Format(
				LOCTEXT("LayoutUnplaced",
					"NO MAP LAYOUT — {0} of {1} region(s) have no authored board position ({2}). The list below is complete; the graph is not drawn, because half a graph reads as \"this region borders nothing\"."),
				Num(Unplaced.Num()), Num(View.Regions.Num()),
				FText::Join(FText::FromString(TEXT(", ")), Names));
			return;
		}

		// Twee knopen op dezelfde plek. Apart van "geen positie", want het is een
		// andere fout met een andere reparatie: hier heeft iemand wél geauthord.
		for (int32 First = 0; First < View.Regions.Num(); ++First)
		{
			for (int32 Second = First + 1; Second < View.Regions.Num(); ++Second)
			{
				const double Distance = FVector2D::Distance(
					View.Regions[First].BoardPosition, View.Regions[Second].BoardPosition);
				if (Distance < MinNodeSeparation)
				{
					View.LayoutStatusText = FText::Format(
						LOCTEXT("LayoutCoincident",
							"MAP LAYOUT REJECTED — {0} and {1} are authored on the same spot. Two nodes in one place is one node on screen."),
						FText::FromName(View.Regions[First].RegionId),
						FText::FromName(View.Regions[Second].RegionId));
					return;
				}
			}
		}

		auto PositionOf = [&View](FName RegionId, FVector2D& Out) -> bool
		{
			const FEclipseMapRegionView* Found = View.Regions.FindByPredicate(
				[RegionId](const FEclipseMapRegionView& R) { return R.RegionId == RegionId; });
			if (Found == nullptr)
			{
				return false;
			}
			Out = Found->BoardPosition;
			return true;
		};

		TSet<TPair<FName, FName>> Seen;
		for (const FEclipseMapRegionView& Region : View.Regions)
		{
			for (const FEclipseMapLaneView& Lane : Region.Lanes)
			{
				// De sleutel is de kant en niet de richting: het paar wordt
				// gesorteerd, dus A->B en B->A landen op dezelfde sleutel. De
				// validator garandeert al dat beide helften hetzelfde zeggen,
				// dus de eerste helft die langskomt is de waarheid.
				const bool bFirstFirst = Region.RegionId.Compare(Lane.NeighborRegionId) < 0;
				const TPair<FName, FName> Key = bFirstFirst
					? TPair<FName, FName>(Region.RegionId, Lane.NeighborRegionId)
					: TPair<FName, FName>(Lane.NeighborRegionId, Region.RegionId);
				if (Seen.Contains(Key))
				{
					continue;
				}

				FVector2D Other;
				if (!PositionOf(Lane.NeighborRegionId, Other))
				{
					// Een lane naar een regio die niet op het bord staat. De
					// validator vangt dat al af vóór deze functie draait, dus
					// hier stil overslaan is geen verlies — en tekenen zou een
					// lijn naar het niets zetten.
					continue;
				}
				Seen.Add(Key);

				FEclipseMapEdgeView& Edge = View.Edges.AddDefaulted_GetRef();
				Edge.RegionIdA = Region.RegionId;
				Edge.RegionIdB = Lane.NeighborRegionId;
				Edge.A = Region.BoardPosition;
				Edge.B = Other;
				Edge.Status = Lane.Status;
				Edge.GateRegionId = Lane.GateRegionId;
				Edge.GateOwner = Lane.GateOwner;
				Edge.bMilitaryPassable = Lane.bMilitaryPassable;
				Edge.MilitaryTravelDays = Lane.MilitaryTravelDays;
				Edge.MilitaryRisk = Lane.MilitaryRisk;
				Edge.bSmugglerPassable = Lane.bSmugglerPassable;
				Edge.SmugglerTravelDays = Lane.SmugglerTravelDays;
				Edge.SmugglerRisk = Lane.SmugglerRisk;

				// Wat de oversteek KOST staat op de lijn (GDD 3.1 regel 4). Kan
				// er geen colonne door, dan is de smokkelprijs de enige prijs
				// die er is — en dan hoort díé er te staan, met een teken ervoor
				// zodat je niet denkt dat er een colonne doorheen kan.
				//
				// Een TEKEN en niet "(smug)": op het frame van 01-08 liep dat
				// woord dwars door twee knooplabels. Op een lijn van 90 px telt
				// elke letter, en de gestreepte lijn zegt het al een keer.
				Edge.CostText = Lane.bMilitaryPassable
					? FText::Format(LOCTEXT("EdgeCost", "{0}d · r{1}"),
						Num(Lane.MilitaryTravelDays), Num(Lane.MilitaryRisk))
					: FText::Format(LOCTEXT("EdgeCostSmuggler", "~{0}d · r{1}"),
						Num(Lane.SmugglerTravelDays), Num(Lane.SmugglerRisk));
			}
		}

		View.bHasLayout = true;
	}
}

FText OwnerText(EEclipseRegionOwner Owner)
{
	switch (Owner)
	{
	case EEclipseRegionOwner::Player:    return LOCTEXT("OwnerPlayer", "PLAYER");
	case EEclipseRegionOwner::Contested: return LOCTEXT("OwnerContested", "CONTESTED");
	default:                             return LOCTEXT("OwnerDominion", "DOMINION");
	}
}

FText ResponseTierText(EEclipseDominionResponseTier Tier)
{
	// De ladder van GDD 9.4, woordelijk. `UEnum::GetValueAsString` zou hier
	// "EEclipseDominionResponseTier::Insurgency" op het scherm zetten — een
	// interne sleutel, en onvertaalbaar.
	switch (Tier)
	{
	case EEclipseDominionResponseTier::Indifference: return LOCTEXT("TierIndifference", "INDIFFERENCE");
	case EEclipseDominionResponseTier::Nuisance:     return LOCTEXT("TierNuisance", "NUISANCE");
	case EEclipseDominionResponseTier::Insurgency:   return LOCTEXT("TierInsurgency", "INSURGENCY");
	case EEclipseDominionResponseTier::Rebellion:    return LOCTEXT("TierRebellion", "REBELLION");
	case EEclipseDominionResponseTier::War:          return LOCTEXT("TierWar", "WAR");
	default:                                         return LOCTEXT("TierExistential", "EXISTENTIAL");
	}
}

FText LaneStatusText(EEclipseLaneStatus Status)
{
	switch (Status)
	{
	case EEclipseLaneStatus::SpireGated:   return LOCTEXT("LaneStatusSpire", "SPIRE-GATED");
	case EEclipseLaneStatus::SmugglerOnly: return LOCTEXT("LaneStatusSmuggler", "SMUGGLER LANE");
	default:                               return LOCTEXT("LaneStatusOpen", "OPEN");
	}
}

FEclipseMapView ComposeMapView(
	const FEclipseCampaignState& State,
	const TArray<FEclipseRegionDefinition>& Definitions,
	const FEclipseLaneTuning& Tuning)
{
	using namespace EclipseStrategyLogic;

	FEclipseMapView View;
	View.Day = State.Day;
	View.ResponseTier = State.ResponseTier;
	View.RiskPerLegFromTier = static_cast<int32>(State.ResponseTier) * Tuning.RiskPerResponseTier;

	// De kop staat er in alle drie de toestanden: dag en temperatuur zijn feiten
	// over de campagne, niet over de graaf, en ze blijven waar als de kaart valt.
	View.HeaderText = View.RiskPerLegFromTier > 0
		? FText::Format(
			LOCTEXT("BoardHeaderWithTier", "DISTRICT BOARD — Day {0} · Dominion response: {1} (+{2} risk on every leg)"),
			Num(View.Day), ResponseTierText(View.ResponseTier), Num(View.RiskPerLegFromTier))
		: FText::Format(
			LOCTEXT("BoardHeader", "DISTRICT BOARD — Day {0} · Dominion response: {1}"),
			Num(View.Day), ResponseTierText(View.ResponseTier));

	// ------------------------------------------------------------------ absent
	if (Definitions.IsEmpty())
	{
		View.DataState = EEclipseMapDataState::Absent;
		View.StatusText = LOCTEXT("MapAbsent",
			"NO REGION GRAPH LOADED — this board is not empty, it is missing its map.");
		return View;
	}

	// ----------------------------------------------------------------- invalid
	//
	// DIT IS DE FALSIFICATIE VAN N-b, en hij staat met opzet vóór alle opmaak.
	// Een graaf waarin één kant open is en zijn tegenkant gepoort, is niet
	// "grotendeels bruikbaar": elke route eroverheen liegt, en de speler ontdekt
	// dat pas als een colonne niet aankomt. Tekenen wat je niet kunt garanderen
	// is erger dan niets tekenen, dus `Regions` blijft leeg.
	if (!ValidateGraph(Definitions, View.GraphErrors))
	{
		View.DataState = EEclipseMapDataState::Invalid;
		View.StatusText = FText::Format(
			LOCTEXT("MapInvalid",
				"MAP DATA REJECTED — {0} problem(s) in the region graph. No lanes are drawn: a contradictory graph would draw a lie."),
			Num(View.GraphErrors.Num()));
		return View;
	}

	// ------------------------------------------------------------------- valid
	View.DataState = EEclipseMapDataState::Valid;

	for (const FEclipseRegionDefinition& Definition : Definitions)
	{
		FEclipseMapRegionView& Region = View.Regions.AddDefaulted_GetRef();
		Region.RegionId = Definition.RegionId;
		Region.DisplayName = NameOf(Definitions, Definition.RegionId);

		const FEclipseRegionState* RegionState = State.FindRegion(Definition.RegionId);
		Region.bHasState = RegionState != nullptr;
		if (RegionState != nullptr)
		{
			Region.Owner = RegionState->Owner;
			Region.Unrest = RegionState->Unrest;
			Region.GarrisonStrength = RegionState->GarrisonStrength;
		}

		Region.bSupplied = IsRegionSupplied(State, Definitions, Definition.RegionId, Tuning);

		// Waar de knoop staat. Geauthord op de definitie; de logica leest hem
		// alleen door en beslist er niets over.
		Region.bHasBoardPosition = Definition.HasBoardPosition();
		Region.BoardPosition = Region.bHasBoardPosition ? Definition.BoardPosition : FVector2D::ZeroVector;

		// --- de kanten, eindelijk aan de toestand gekoppeld ---
		TArray<FText> NeighborNames;
		NeighborNames.Reserve(Definition.Lanes.Num());

		for (const FEclipseLaneDefinition& LaneDefinition : Definition.Lanes)
		{
			FEclipseMapLaneView& Lane = Region.Lanes.AddDefaulted_GetRef();
			Lane.NeighborRegionId = LaneDefinition.NeighborRegionId;
			Lane.NeighborName = NameOf(Definitions, LaneDefinition.NeighborRegionId);
			Lane.Status = LaneDefinition.Status;
			Lane.GateRegionId = LaneDefinition.GateRegionId;

			// Een poort zonder campagnerij telt als vijandig — exact wat
			// ResolveLaneTransit doet, en om dezelfde reden: een onbekende poort
			// is geen open poort.
			if (!LaneDefinition.GateRegionId.IsNone())
			{
				const FEclipseRegionState* Gate = State.FindRegion(LaneDefinition.GateRegionId);
				Lane.GateOwner = Gate != nullptr ? Gate->Owner : EEclipseRegionOwner::Dominion;
			}

			const FEclipseLaneTransit Military = ResolveLaneTransit(State, LaneDefinition, EEclipseTransitMode::Military, Tuning);
			const FEclipseLaneTransit Smuggler = ResolveLaneTransit(State, LaneDefinition, EEclipseTransitMode::Smuggler, Tuning);

			Lane.bMilitaryPassable = Military.bPassable;
			Lane.MilitaryTravelDays = Military.TravelDays;
			Lane.MilitaryRisk = Military.Risk;
			Lane.bSmugglerPassable = Smuggler.bPassable;
			Lane.SmugglerTravelDays = Smuggler.TravelDays;
			Lane.SmugglerRisk = Smuggler.Risk;
			Lane.bSmugglerLeg = Smuggler.bSmugglerLeg;

			if (!Military.bPassable)
			{
				Lane.MilitaryBlockedText = LaneDefinition.Status == EEclipseLaneStatus::SpireGated
					? FText::Format(
						LOCTEXT("BlockedByGate", "Gate Spire {0} is hostile"),
						FText::FromName(LaneDefinition.GateRegionId))
					: LOCTEXT("BlockedSmugglerOnly", "no column fits through, in any campaign state");
			}

			// De regel zelf. Militair doorlaatbaar = één prijs; dicht = de reden
			// plus wat smokkelaars ervoor rekenen. De smokkelclausule verschijnt
			// alleen als smokkelen ECHT iets anders is (bSmugglerLeg), zodat een
			// gewone open lane niet twee keer dezelfde prijs opschrijft.
			if (Military.bPassable && !Smuggler.bSmugglerLeg)
			{
				Lane.Text = FText::Format(
					LOCTEXT("LaneOpenLine", "  -> {0} · {1} · {2}d, risk {3}"),
					Lane.NeighborName, StatusClause(Lane), Num(Lane.MilitaryTravelDays), Num(Lane.MilitaryRisk));
			}
			else if (Military.bPassable)
			{
				Lane.Text = FText::Format(
					LOCTEXT("LaneOpenPlusSmugglerLine", "  -> {0} · {1} · {2}d, risk {3} · smugglers {4}d, risk {5}"),
					Lane.NeighborName, StatusClause(Lane), Num(Lane.MilitaryTravelDays), Num(Lane.MilitaryRisk),
					Num(Lane.SmugglerTravelDays), Num(Lane.SmugglerRisk));
			}
			else if (Smuggler.bPassable)
			{
				Lane.Text = FText::Format(
					LOCTEXT("LaneBlockedLine", "  -> {0} · {1} · military BLOCKED: {2} · smugglers {3}d, risk {4}"),
					Lane.NeighborName, StatusClause(Lane), Lane.MilitaryBlockedText,
					Num(Lane.SmugglerTravelDays), Num(Lane.SmugglerRisk));
			}
			else
			{
				Lane.Text = FText::Format(
					LOCTEXT("LaneShutLine", "  -> {0} · {1} · SHUT to everyone: {2}"),
					Lane.NeighborName, StatusClause(Lane), Lane.MilitaryBlockedText);
			}

			NeighborNames.Add(Lane.NeighborName);
		}

		Region.NeighborsText = NeighborNames.IsEmpty()
			? LOCTEXT("NoBorders", "borders: none")
			: FText::Format(LOCTEXT("Borders", "borders: {0}"), FText::Join(FText::FromString(TEXT(", ")), NeighborNames));

		Region.HeaderText = Region.bHasState
			? FText::Format(
				LOCTEXT("RegionLine", "{0} — {1} · garrison {2} · unrest {3} · {4}"),
				Region.DisplayName, OwnerText(Region.Owner),
				Num(Region.GarrisonStrength), Num(Region.Unrest),
				Region.bSupplied ? LOCTEXT("Supplied", "SUPPLIED") : LOCTEXT("CutOff", "CUT OFF"))
			: FText::Format(
				LOCTEXT("RegionNoState", "{0} — NO CAMPAIGN STATE (the graph knows this region, the campaign does not)"),
				Region.DisplayName);
	}

	// ------------------------------------------------------ knopen op posities
	//
	// DE GRAAF ALS VORM (REFERENTIE_BASE_MAP.md §1.4 rij 1). Vanaf hier wordt er
	// niets meer aan de LIJST toegevoegd; dit is de tweede lezing van dezelfde
	// data, als knopen en lijnen.
	BuildLayout(View);

	// Rijen die alleen in de campagnetoestand bestaan. Ze horen op geen enkele
	// lane en zijn dus onbereikbaar; stil weglaten zou precies de datadrift
	// verbergen die ze veroorzaakte.
	for (const FEclipseRegionState& Orphan : State.Regions)
	{
		const bool bKnown = Definitions.ContainsByPredicate(
			[&Orphan](const FEclipseRegionDefinition& D) { return D.RegionId == Orphan.RegionId; });
		if (bKnown)
		{
			continue;
		}

		FEclipseMapRegionView& Region = View.Regions.AddDefaulted_GetRef();
		Region.RegionId = Orphan.RegionId;
		Region.DisplayName = FText::FromName(Orphan.RegionId);
		Region.Owner = Orphan.Owner;
		Region.Unrest = Orphan.Unrest;
		Region.GarrisonStrength = Orphan.GarrisonStrength;
		Region.bHasDefinition = false;
		Region.bSupplied = false;
		Region.HeaderText = FText::Format(
			LOCTEXT("RegionOffMap", "{0} — {1} · OFF THE MAP (no lanes in the region graph)"),
			Region.DisplayName, OwnerText(Region.Owner));
		Region.NeighborsText = LOCTEXT("NoBorders", "borders: none");
	}

	return View;
}

}

#undef LOCTEXT_NAMESPACE
