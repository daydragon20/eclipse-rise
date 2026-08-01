#pragma once

#include "CoreMinimal.h"
#include "Strategy/EclipseStrategyLogic.h"

/**
 * DE KOPPELING DIE ONTBRAK (N-b, `phase0/REFERENTIE_BASE_MAP.md` §4.1).
 *
 * Het gat op de kaart was nooit een ontbrekend schema. De kanten bestaan al —
 * `FEclipseRegionDefinition::Lanes`, mét status, prijs en een validator die
 * symmetrie afdwingt. Het scherm las alleen de verkeerde helft: de widget liep
 * `FEclipseCampaignState::Regions` af, en dat is de MUTABELE toestand met vier
 * velden en géén kanten. De topologie lag een header verderop en werd nooit
 * opgehaald.
 *
 * Dit bestand is die koppeling, en niets meer: het legt de twee structuren naast
 * elkaar en zet er de regels op die het scherm hoort te tonen. Puur (GDD 14.3.2)
 * — geen widget, geen wereld, geen subsystem. Wat er op de kaart STAAT is hier
 * toetsbaar zonder één pixel; de widget bezit alleen nog kleur en plaatsing.
 *
 * DE ENE HARDE REGEL, en hij is de falsificatie van N-b:
 *   een graaf die zichzelf tegenspreekt HAALT HET SCHERM NIET.
 * `ComposeMapView` draait `ValidateGraph` als eerste, en bij een fout blijft
 * `Regions` LEEG. Een asymmetrische kant (open heen, gepoort terug) tekenen zou
 * een leugen tekenen die zich als geografie voordoet — en de speler zou de fout
 * pas vinden op het moment dat de route het niet doet. Zie
 * `Tests/EclipseStrategyMapViewTests.cpp`: de controleproef verzet één kant en
 * bewijst het rood vóórdat er iets te tekenen valt.
 *
 * BRONTAAL IS ENGELS (`13_roadmap.md` r45: NL is een DOELtaal). Alles wat de
 * speler leest gaat hier door NSLOCTEXT/FText.
 */
namespace EclipseStrategyMap
{
	/**
	 * In welke van de drie toestanden de kaartdata verkeert. Drie en geen twee,
	 * want "geen graaf" en "kapotte graaf" zijn verschillende reparaties en een
	 * scherm dat ze samenvat tot een leeg bord vertelt geen van beide. Dezelfde
	 * les als in `Eclipse.Liberation.Report`: noem eerst de ONTBREKENDE SCHAKEL.
	 */
	enum class EEclipseMapDataState : uint8
	{
		/** Graaf geladen en structureel geldig; `Regions` is het bord. */
		Valid,
		/** Er is helemaal geen graaf. Het bord is niet leeg, het is er niet. */
		Absent,
		/** Een graaf die zichzelf tegenspreekt. `Regions` blijft met opzet leeg. */
		Invalid
	};

	/** Eén lane vanuit één regio, met wat hij NU kost (GDD 3.1 regels 2 en 4). */
	struct FEclipseMapLaneView
	{
		FName NeighborRegionId;

		/** Leesbare naam van de buur; valt terug op de id als de data er geen heeft. */
		FText NeighborName;

		EEclipseLaneStatus Status = EEclipseLaneStatus::Open;

		/** Alleen bij SpireGated gevuld — de knoop die de lane dichthoudt. */
		FName GateRegionId;

		/** Wie die knoop bezit. Dominion = dicht voor colonnes; dit is het hele verschil met een muur. */
		EEclipseRegionOwner GateOwner = EEclipseRegionOwner::Dominion;

		bool bMilitaryPassable = false;
		int32 MilitaryTravelDays = 0;
		int32 MilitaryRisk = 0;

		/**
		 * Waaróm niet, in spelertaal. Bewust NIET de string uit
		 * `ResolveLaneTransit`: die draagt een GDD-verwijzing en is niet
		 * vertaalbaar. De logica houdt haar diagnose, het scherm krijgt zijn zin.
		 */
		FText MilitaryBlockedText;

		bool bSmugglerPassable = false;
		int32 SmugglerTravelDays = 0;
		int32 SmugglerRisk = 0;

		/** True als deze oversteek alleen lukt omdat er gesmokkeld wordt (Kaya's helft van regel 2). */
		bool bSmugglerLeg = false;

		/** De hele regel zoals hij op het bord komt. */
		FText Text;
	};

	/**
	 * ÉÉN LANE ALS LIJN — één keer per ONGERICHTE kant, met beide eindpunten.
	 *
	 * `FEclipseMapRegionView::Lanes` draagt elke kant twee keer (één keer per
	 * regio), want een regel op het bord hoort bij een regio. Een LIJN hoort bij
	 * een kant, en die twee keer tekenen zou elke lane dubbel zetten: dikker,
	 * donkerder, en bij een halfdoorzichtige inkt zichtbaar anders dan een lane
	 * die maar één kant heeft. De ontdubbeling gebeurt hier, in de pure laag,
	 * zodat een test hem kan vastpinnen zonder viewport.
	 */
	struct FEclipseMapEdgeView
	{
		/** A is de regio die in `Definitions` het eerst voorkomt; B de andere. Vast, dus toetsbaar. */
		FName RegionIdA;
		FName RegionIdB;

		/** Genormaliseerde bordpositie van beide uiteinden (X rechts, Y omlaag, 0..1). */
		FVector2D A = FVector2D::ZeroVector;
		FVector2D B = FVector2D::ZeroVector;

		EEclipseLaneStatus Status = EEclipseLaneStatus::Open;
		FName GateRegionId;
		EEclipseRegionOwner GateOwner = EEclipseRegionOwner::Dominion;

		bool bMilitaryPassable = false;
		int32 MilitaryTravelDays = 0;
		int32 MilitaryRisk = 0;
		bool bSmugglerPassable = false;
		int32 SmugglerTravelDays = 0;
		int32 SmugglerRisk = 0;

		/** "2d · r18" — kort, want dit staat óp de lijn en niet op een regel. */
		FText CostText;
	};

	/** Eén regio: de mutabele toestand en de statische topologie, eindelijk in één rij. */
	struct FEclipseMapRegionView
	{
		FName RegionId;
		FText DisplayName;

		EEclipseRegionOwner Owner = EEclipseRegionOwner::Dominion;
		int32 Unrest = 0;
		int32 GarrisonStrength = 0;

		/** GDD 3.1 regel 4 op het scherm: bereikbaar vanaf eigen grond binnen MaxSupplyDays. */
		bool bSupplied = false;

		/** False = de graaf kent deze regio, de campagne niet (of andersom, zie hieronder). */
		bool bHasState = true;

		/**
		 * False = deze rij komt uit de campagnetoestand en heeft géén definitie.
		 * Zo'n regio ligt aan geen enkele lane en kan dus nooit bereikt worden;
		 * hem stil weglaten zou de drift onzichtbaar maken die hem veroorzaakte.
		 */
		bool bHasDefinition = true;

		TArray<FEclipseMapLaneView> Lanes;

		/**
		 * Waar deze knoop op het bord staat (genormaliseerd, X rechts, Y omlaag).
		 * Geauthord op de regiodefinitie; zie `FEclipseRegionDefinition::BoardPosition`
		 * voor waarom dit geen afgeleide layout is.
		 */
		FVector2D BoardPosition = FVector2D::ZeroVector;

		/** False = niemand heeft deze regio op het bord gezet. Dan wordt er niets getekend. */
		bool bHasBoardPosition = false;

		/** "Underworks — PLAYER · garrison 3 · unrest 12 · SUPPLIED" */
		FText HeaderText;

		/**
		 * "borders: Checkpoint, Housing" — letterlijk de eis uit N-b: de kaartlaag
		 * NOEMT per regio zijn buren. Losstaand veld en niet alleen als onderdeel
		 * van de lane-regels, want dit is wat een test kan vastpinnen.
		 */
		FText NeighborsText;
	};

	/** Het hele bord zoals het scherm het nodig heeft. */
	struct FEclipseMapView
	{
		EEclipseMapDataState DataState = EEclipseMapDataState::Absent;

		/** Ruwe validatorfouten; leeg tenzij DataState == Invalid. Diagnose, geen spelertekst. */
		TArray<FString> GraphErrors;

		/** Wat er in plaats van het bord staat als er geen bord is. Leeg bij Valid. */
		FText StatusText;

		int32 Day = 0;
		EEclipseDominionResponseTier ResponseTier = EEclipseDominionResponseTier::Indifference;

		/** Wat de temperatuur van het rijk op ELKE lane-leg oplegt (GDD 9.4). */
		int32 RiskPerLegFromTier = 0;

		/** "DISTRICT BOARD — Day 3 · Dominion response: INSURGENCY (+8 risk on every leg)" */
		FText HeaderText;

		/** Leeg zodra DataState != Valid. Dat is de falsificatie, geen bijwerking. */
		TArray<FEclipseMapRegionView> Regions;

		/**
		 * De lanes als LIJNEN, ontdubbeld. Leeg zodra `bHasLayout` false is.
		 *
		 * Volgorde: de kanten van de eerste regio eerst, dan die van de tweede, en
		 * binnen een regio de volgorde van `Lanes`. Vast, dus een test kan er iets
		 * over zeggen zonder te sorteren.
		 */
		TArray<FEclipseMapEdgeView> Edges;

		/**
		 * TWEE ONAFHANKELIJKE POORTEN, en dat is met opzet.
		 *
		 * `IsRenderable()` (de asymmetriepoort) zegt of het BORD mag bestaan: een
		 * graaf die zichzelf tegenspreekt tekent niets, ook geen lijst. `bHasLayout`
		 * zegt alleen of de GRAAF getekend mag worden. Een bord zonder geauthorde
		 * posities is niet kapot — het is niet ingedeeld, en dan hoort de lijst
		 * gewoon te blijven staan (REFERENTIE_BASE_MAP.md §1.5: de lijst mag naast
		 * de graaf bestaan, nooit ervoor in de plaats).
		 *
		 * Layout tot een validatiefout maken zou het bord blank slaan om een
		 * TEKENkwestie, en dat is precies de klasse fout die de asymmetriepoort
		 * moest voorkomen: niet tekenen wat je niet kunt garanderen, maar ook niet
		 * weggooien wat je wél weet.
		 */
		bool bHasLayout = false;

		/** Waaróm er geen graaf getekend wordt. Leeg zodra bHasLayout waar is. */
		FText LayoutStatusText;

		bool IsRenderable() const { return DataState == EEclipseMapDataState::Valid; }
	};

	/**
	 * Toestand + definities -> het bord.
	 *
	 * Volgorde is die van `Definitions` (dezelfde als `GetLegalMissionTargets`),
	 * zodat bord, aanbod en tests het over dezelfde volgorde hebben. Rijen die
	 * alleen in de campagnetoestand bestaan komen er achteraan met
	 * `bHasDefinition = false`.
	 *
	 * `Definitions` leeg -> `Absent`. Validator rood -> `Invalid` mét de fouten,
	 * en `Regions` blijft leeg.
	 */
	ECLIPSE_API FEclipseMapView ComposeMapView(
		const FEclipseCampaignState& State,
		const TArray<FEclipseRegionDefinition>& Definitions,
		const FEclipseLaneTuning& Tuning);

	/** "PLAYER" / "CONTESTED" / "DOMINION" — spelertekst, niet de enum-naam. */
	ECLIPSE_API FText OwnerText(EEclipseRegionOwner Owner);

	/** De GDD 9.4-ladder in spelertaal ("Insurgency"), niet `EEclipse...::Insurgency`. */
	ECLIPSE_API FText ResponseTierText(EEclipseDominionResponseTier Tier);

	/** "OPEN" / "SPIRE-GATED" / "SMUGGLER LANE". */
	ECLIPSE_API FText LaneStatusText(EEclipseLaneStatus Status);
}
