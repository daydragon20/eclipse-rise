#pragma once

#include "Base/EclipseBaseLogic.h"
#include "Base/EclipseBaseTypes.h"
#include "CoreMinimal.h"
#include "Strategy/EclipseCampaignTypes.h"

/**
 * WAT DE BASIS AAN HET DOEN IS, in plaats van wat hij heeft
 * (`phase0/REFERENTIE_BASE_MAP.md` §2.3).
 *
 * Het gat in §2.3 is niet "er missen velden". De hub toont vandaag drie
 * saldo's in een printf-kop en een productielijst; wat er ONTBREEKT is elke
 * vorm van SPANNING — hoeveel slots je nog hebt, hoe ver die bouw is, wat het
 * kost om hem af te kopen, wie er vastzit in een basisbaan, en of je jezelf in
 * het donker aan het bouwen bent. Dit bestand rekent die spanning uit; het
 * tekent niets.
 *
 * PUUR (GDD 14.3.2), en dat is hier meer waard dan gewoonlijk: elke rij uit
 * §2.3 wordt hiermee een vraag die een test kan stellen zonder viewport. "Hoe
 * ver is die bouw" en "wat kost rushen nu" zijn getallen, en getallen die je
 * pas in een screenshot kunt controleren zijn getallen die stilletjes kunnen
 * afwijken van de logica eronder.
 *
 * === TWEE ONAFHANKELIJKE POORTEN, en dat is de scherpste les van de kaartronde ===
 *
 *   1. DE DATAPOORT (`DataState`) slaat het RASTER blank. Een basistoestand die
 *      zichzelf tegenspreekt — twee faciliteiten op één slot, of een faciliteit
 *      op een slot dat de layout niet kent — mag geen raster opleveren. Zo'n
 *      faciliteit zou NERGENS staan: de speler betaalt hem, bemant hem en ziet
 *      hem niet. Dat is dezelfde klasse leugen als een asymmetrische lane op de
 *      kaart, en dus dezelfde behandeling.
 *
 *   2. DE INHOUDSPOORTEN (`EnergyState`, `bHasCrewBand`) sluiten ALLEEN hun
 *      eigen band. Een basis waar niemand energieverbruik heeft geauthord is
 *      niet kapot — hij is onvolledig, en dan hoort het slotraster gewoon
 *      leesbaar te blijven. Energie tot een validatiefout maken zou het raster
 *      blank slaan om een ontbrekende kolom in een datatabel.
 *
 * === WAT DIT BESTAND MET OPZET NIET WEET ===
 *
 * De energie-economie is NIET geauthord (zie `FEclipseFacilityLevelData::EnergyUpkeep`)
 * en de trigger die de slottrap ophoogt is NIET beslist (zie
 * `FEclipseBaseSlotDef::UnlockTier`). Beide zijn hier zichtbaar als toestand in
 * plaats van als aanname: `EEclipseEnergyState::Unauthored` en de
 * `AvailableSlotTier`-parameter. Een scherm dat 0 verbruik tegen 0 opwekking
 * als "balans in orde" toont, ziet er precies zo uit als een scherm dat de
 * balans echt kent.
 *
 * BRONTAAL IS ENGELS (`13_roadmap.md` r45: NL is een DOELtaal) — alles wat de
 * speler leest gaat door NSLOCTEXT/FText.
 */
namespace EclipseBaseView
{
	/**
	 * In welke toestand de basisDATA verkeert. Drie en geen twee, om dezelfde
	 * reden als op de kaart: "er is geen layout" en "de layout spreekt de
	 * toestand tegen" zijn verschillende reparaties, en een scherm dat ze
	 * samenvat tot een leeg raster vertelt geen van beide.
	 */
	enum class EEclipseBaseDataState : uint8
	{
		/** Layout geladen en consistent met de toestand; `Slots` is het raster. */
		Valid,
		/** Er is helemaal geen layout. Het raster is niet leeg, het is er niet. */
		Absent,
		/** Layout en toestand spreken elkaar tegen. `Slots` blijft met opzet leeg. */
		Invalid
	};

	/**
	 * De vijf toestanden waarin een slot kan staan. §2.3 vraagt er drie (bezet,
	 * vrij, vergrendeld); de andere twee zijn precies waar het document om
	 * draait — een slot dat BOUWT en een slot dat STUK is, zijn allebei "bezet"
	 * en betekenen iets volstrekt anders voor wat je nu moet doen.
	 *
	 * Volgorde is de ernstvolgorde die de tekenlaag gebruikt om vorm te kiezen.
	 */
	enum class EEclipseSlotStatus : uint8
	{
		/** Bestaat pas op een hogere uitbreidingstrap (GDD 5.2: 4 -> 8 -> 12 -> 16). */
		Locked,
		/** Vrij, en dus de plek waar de volgende beslissing valt. */
		Empty,
		/** Bouwt of upgradet: heeft een voortgang, een ETA en een rushprijs. */
		UnderConstruction,
		/** Draait en levert. */
		Operational,
		/** Gebouwd maar OFFLINE tot reparatie (GDD 5.4). Levert vandaag niets. */
		Damaged
	};

	/**
	 * De energieband. `Unauthored` is geen foutwaarde maar het eerlijke
	 * antwoord op "hoeveel energie kost dit" wanneer niemand dat ooit heeft
	 * ingevuld — en het is met opzet de eerste waarde, zodat een
	 * default-geconstrueerde view nooit per ongeluk "prima" beweert.
	 */
	enum class EEclipseEnergyState : uint8
	{
		/** Geen enkele faciliteit authort upkeep of opwekking. Er is geen band om te tonen. */
		Unauthored,
		/** Opwekking dekt het verbruik ruim. */
		Surplus,
		/** Gedekt, maar de volgende faciliteit past er niet meer bij (zie TightHeadroomPercent). */
		Tight,
		/** Verbruik overschrijdt de opwekking: je hebt jezelf in het donker gebouwd. */
		Deficit
	};

	/** Eén bemande post. Naam en niet id, want een guid is geen bemanning. */
	struct FEclipseBaseCrewView
	{
		FGuid SoldierId;

		/** Naam uit het rooster; valt terug op een korte id-vorm als het rooster hem niet kent. */
		FText Name;

		/**
		 * False = deze toewijzing verwijst naar iemand die niet (meer) in het
		 * rooster staat. Stil weglaten zou de drift onzichtbaar maken die hem
		 * veroorzaakte — dezelfde afweging als `bHasDefinition` op de kaart.
		 */
		bool bInRoster = true;
	};

	/** Eén slot in het raster: wat erin staat, wat het doet, en wat het nu kost. */
	struct FEclipseBaseSlotView
	{
		FName SlotId;
		FText SlotName;

		EEclipseSlotStatus Status = EEclipseSlotStatus::Empty;

		FName FacilityId;

		/** Leesbare naam uit DT_Facilities; valt terug op de id als de rij ontbreekt. */
		FText FacilityName;

		/** Hoogst voltooide niveau (0 = nog niets af). */
		int32 Level = 0;

		/** Hoeveel niveaus deze faciliteit kent (Levels.Num()); 0 = rij onbekend. */
		int32 MaxLevel = 0;

		/** Waar deze bouw naartoe werkt (Level + 1), 0 als er niet gebouwd wordt. */
		int32 TargetLevel = 0;

		/** Vanaf welke uitbreidingstrap dit slot bestaat. */
		int32 UnlockTier = 1;

		// --- Bouw-ETA ALS VOORTGANG (§2.3 rij 2) -----------------------------

		int32 DaysRemaining = 0;

		/**
		 * De volle bouwduur van het lopende niveau, uit DT_Facilities.
		 * 0 = onbekend, en dan is er GEEN balk (zie bHasProgress).
		 */
		int32 TotalDays = 0;

		/**
		 * Voortgang 0..1. Alleen betekenisvol als `bHasProgress` waar is.
		 *
		 * De eis van §2.3 is letterlijk "ETA als voortgang, niet als getal":
		 * een balk die vult zegt iets anders dan "3 uur". Vandaar dat dit een
		 * fractie is en geen dagenteller — de tekenlaag hoeft niets te delen.
		 */
		float Progress01 = 0.0f;

		/**
		 * False = we weten wel hoeveel dagen er nog te gaan zijn, maar niet
		 * hoeveel het er waren. Dan toont het scherm de dagen zonder balk.
		 *
		 * DIT IS EEN POORT, GEEN DETAIL. Een balk tekenen op een geraden
		 * totaal zou een voortgang tonen die niemand heeft gemeten, en dat is
		 * de ene fout die een voortgangsbalk niet mag maken.
		 */
		bool bHasProgress = false;

		/** "BUILDING L2 — day 2 of 3" of "2 days left" als het totaal onbekend is. */
		FText ProgressText;

		// --- Rush (§2.3 rij 3) ----------------------------------------------

		/** Prijs op DIT moment: tuning x resterende dagen. 0 als er niets te rushen valt. */
		int32 RushCostCredits = 0;

		bool bCanRush = false;

		/** False terwijl bCanRush waar is = de knop hoort zichtbaar maar geweigerd te zijn. */
		bool bRushAffordable = false;

		/** "RUSH 180 C" — of "RUSH 180 C (need 60 more)" als de beurs het niet dekt. */
		FText RushText;

		// --- Bemanning (§2.3 rij 4) -----------------------------------------

		TArray<FEclipseBaseCrewView> Crew;

		/** Effectief plafond op deze post (DA_BaseTuning MaxCrewPerSite). */
		int32 CrewCap = 0;

		/**
		 * True zolang de post BOUWT: de rol is positioneel (bouwploeg op een
		 * bouwplaats, analist op een draaiende faciliteit — SPEC-P2-03), en het
		 * scherm moet dat verschil noemen omdat het twee verschillende
		 * beslissingen zijn.
		 */
		bool bCrewIsConstruction = false;

		/** "CREW 1/1 — Vasquez" / "ANALYST 0/1 — unstaffed". */
		FText CrewText;

		// --- Energie (§2.3 rij 5), per slot ---------------------------------

		int32 EnergyUpkeep = 0;
		int32 EnergyOutput = 0;

		// --- Plaats in het raster -------------------------------------------

		/** Kolom en rij, afgeleid van de layoutvolgorde en `ColumnsPerRow`. Vast, dus toetsbaar. */
		int32 Column = 0;
		int32 Row = 0;

		/** "Workshop L1" / "empty" / "sealed rock" — de regel in de tegel. */
		FText HeaderText;

		/** De statusregel eronder: voortgang, schade, of wat het slot toelaat. */
		FText StatusText;
	};

	/** Het hele basisscherm zoals de tekenlaag het nodig heeft. */
	struct FEclipseBaseView
	{
		EEclipseBaseDataState DataState = EEclipseBaseDataState::Absent;

		/** Ruwe consistentiefouten; leeg tenzij DataState == Invalid. Diagnose, geen spelertekst. */
		TArray<FString> Errors;

		/** Wat er in plaats van het raster staat als er geen raster is. Leeg bij Valid. */
		FText StatusText;

		/** De strategische klok (§2.3 laatste rij): zonder dag heeft geen ETA betekenis. */
		int32 Day = 0;

		/** "HOLLOW POINT — day 3" */
		FText HeaderText;

		/** Leeg zodra DataState != Valid. Dat is de falsificatie, geen bijwerking. */
		TArray<FEclipseBaseSlotView> Slots;

		// --- Slotschaarste als getal (§2.3 rij 1) ---------------------------

		int32 SlotsOccupied = 0;
		int32 SlotsFree = 0;
		int32 SlotsLocked = 0;

		/** "SLOTS 2 built · 2 free · 4 sealed" */
		FText SlotCountText;

		// --- Energieband (§2.3 rij 5) ---------------------------------------

		EEclipseEnergyState EnergyState = EEclipseEnergyState::Unauthored;

		/** Opgeteld verbruik van alles wat DRAAIT (beschadigd telt niet mee; het staat stil). */
		int32 EnergyDraw = 0;

		int32 EnergySupply = 0;

		/** Supply - Draw. Negatief = tekort. */
		int32 EnergyHeadroom = 0;

		/** "POWER 14 / 25" — of de reden waarom er geen band is. */
		FText EnergyText;

		/**
		 * False = de band hoort niet getekend te worden (Unauthored). Aparte
		 * poort van `DataState`: het raster blijft staan.
		 */
		bool bHasEnergyBand = false;

		// --- Bemanning als schaarste (§2.3 rij 4) ---------------------------

		/** Soldaten die vastzitten in een basisbaan — en dus NIET inzetbaar zijn. */
		int32 CrewAssigned = 0;

		/** Hoeveel er in totaal beschikbaar waren (rooster met status Available + de toegewezenen). */
		int32 CrewPool = 0;

		/** Hoeveel posten er open staan op alles wat bemand KAN worden. */
		int32 CrewPostsOpen = 0;

		/** "CREW 2 of 6 assigned · 3 posts open" */
		FText CrewText;

		/** False = er is niets te bemannen. De band collapst; het raster blijft. */
		bool bHasCrewBand = false;

		// --- Schade (§2.3 rij 7) --------------------------------------------

		int32 DamagedCount = 0;

		/** "2 FACILITIES OFFLINE — damaged" — leeg als er niets stuk is. */
		FText DamageText;

		bool IsRenderable() const { return DataState == EEclipseBaseDataState::Valid; }
	};

	/**
	 * Hoeveel procent speling nog "Tight" heet in plaats van "Surplus".
	 *
	 * EEN GEKOZEN DREMPEL EN GEEN GEVONDEN GETAL. Het ontwerp legt geen grens
	 * vast tussen "ruim" en "krap"; dit is een presentatiedrempel die alleen
	 * bepaalt wanneer de band waarschuwt, en hij verandert nooit een
	 * spelregel — `Deficit` (verbruik > opwekking) is het enige harde feit en
	 * dat is een vergelijking, geen drempel.
	 */
	constexpr int32 TightHeadroomPercent = 20;

	/** Rooster-opzoeking; mag null teruggeven voor onbekende soldaten (drift wordt getoond, niet verzwegen). */
	using FEclipseSoldierResolver = TFunctionRef<const FEclipseSoldierRecord*(const FGuid&)>;

	/**
	 * Toestand + layout + faciliteitentabel -> het basisscherm.
	 *
	 * Volgorde is die van `Slots` (de layoutvolgorde), zodat raster, tests en
	 * de tekenlaag het over dezelfde volgorde hebben.
	 *
	 * `Slots` leeg -> `Absent`. Een tegenspraak tussen layout en toestand ->
	 * `Invalid` mét de fouten, en `Slots` blijft leeg.
	 *
	 * @param AvailableSlotTier  De bereikte uitbreidingstrap (GDD 5.2). PARAMETER
	 *                           en geen afleiding, want wat de trap ophoogt is
	 *                           niet beslist — zie `FEclipseBaseSlotDef::UnlockTier`.
	 * @param ColumnsPerRow      Rasterbreedte; alleen plaatsing, nooit inhoud.
	 */
	ECLIPSE_API FEclipseBaseView ComposeBaseView(
		const FEclipseCampaignState& State,
		TConstArrayView<FEclipseBaseSlotDef> Slots,
		const EclipseBaseLogic::FEclipseBaseTuningParams& Tuning,
		EclipseBaseLogic::FEclipseFacilityRowResolver FindFacilityRow,
		FEclipseSoldierResolver FindSoldier,
		int32 AvailableSlotTier = 1,
		int32 ColumnsPerRow = 4);

	/** "EMPTY" / "BUILDING" / "ONLINE" / "OFFLINE" / "SEALED" — spelertekst, niet de enum-naam. */
	ECLIPSE_API FText SlotStatusText(EEclipseSlotStatus Status);

	/**
	 * ALLE VIJF DE SLOTTOESTANDEN TEGELIJK, als PRESENTATIETOESTAND.
	 *
	 * WAAROM DIT BESTAAT. Een campagne op dag 1 heeft één gebouwde faciliteit en
	 * drie lege slots — er bouwt niets, er is niets stuk en er is niets
	 * vergrendeld. Op een opname van dag 1 zijn dus maar TWEE van de vijf
	 * tegelvormen te zien, en de andere drie zouden alleen door tests gedekt
	 * zijn. Dat is precies de blinde hoek waar de hele basislaag in zat: dingen
	 * die kloppen volgens de code en die niemand ooit heeft bekeken.
	 *
	 * Zelfde patroon en zelfde belofte als `EclipseVault::MakeReviewState`: dit
	 * is een presentatietoestand die NOOIT wordt gecommit, opgeslagen of
	 * uitgezonden (GDD 12.2 regel 4). Hij loopt bovendien door de ECHTE
	 * `ComposeBaseView`, zodat het frame toont wat de pijplijn maakt en niet wat
	 * een tekenroutine met de hand heeft neergezet.
	 */
	ECLIPSE_API FEclipseBaseView MakeReviewView();
}
