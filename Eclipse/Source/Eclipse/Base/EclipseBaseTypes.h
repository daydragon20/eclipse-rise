#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipseBaseTypes.generated.h"

/**
 * Hollow Point base data (SPEC-P2-03, GDD 5.1-5.4 / 12.3). Slot-based inside an
 * authored shell, never freeform (5.1): the layout asset pins which facility can
 * occupy which slot, and every cost/timer/yield lives in DT_Facilities or
 * DA_BaseTuning (GDD 14.2 - a hardcoded gameplay constant is a defect).
 */

/**
 * One facility level's costs, timer and output (element of FEclipseFacilityRow.Levels;
 * index 0 = L1). The slice ships x0.8 Act-1 values (SPEC-P2-03 locked decision 3);
 * the GDD 5.3.1 numbers return with Phase 3 rebalancing - in this data, not in code.
 */
USTRUCT(BlueprintType)
struct FEclipseFacilityLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 CostMaterials = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 CostCredits = 0;

	/** Strategic-clock days uncrewed; a crew shaves DA_BaseTuning.CrewDayReduction off, never below 1 (5.3.2 staff dilemma). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 BuildDays = 1;

	/** Daily output while operational at this level, keyed by Resource.* tag (IC: +2 Resource.Intel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TMap<FGameplayTag, int32> YieldPerDay;

	/**
	 * Capability die vrijkomt zolang de faciliteit draait (bv. Workshop L2's
	 * fabricagetier); leeg = geen. NIET GELEZEN door C++ — setup_base_data.py
	 * vult de tags, maar niets vraagt er ooit naar, dus een draaiende faciliteit
	 * ontsluit vandaag niets.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FGameplayTag UnlockTag;

	/**
	 * Dagelijks energieverbruik op dit niveau — de kolom "Energy upkeep" uit
	 * GDD 5.3.1 (CC 2/4/6, Barracks 1/2/4, Workshop 2/5/9, IC 2/5/8, ...).
	 *
	 * NUL IS HIER GEEN NUL, EN DAT IS MET OPZET. De sliceverzameling in
	 * `Tools/setup_base_data.py` authort vandaag GEEN energie — geen upkeep en
	 * geen Power Plant — dus elke rij staat op 0. Het scherm mag daar niet
	 * "balans in orde" van maken: 0 verbruik tegen 0 opwekking is niet een
	 * gezonde basis, het is een basis waarvan niemand het verbruik heeft
	 * ingevuld. `EclipseBaseView::ComposeBaseView` scheidt die twee expliciet
	 * (`EEclipseEnergyDataState::Unauthored`), want een verzonnen groene balk
	 * ziet er precies zo uit als een gemeten groene balk.
	 *
	 * De GETALLEN staan in GDD 5.3.1 en horen in de data, niet hier: zodra
	 * iemand ze in DT_Facilities zet, gaat de band vanzelf leven zonder één
	 * regel code (GDD 14.2).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 EnergyUpkeep = 0;

	/**
	 * Dagelijkse energie-OPWEKKING op dit niveau (GDD 5.3.1: de Power Plant is
	 * de enige rij met "+10/+25/+50 Energy" als voordeel in plaats van upkeep).
	 *
	 * Een eigen veld en geen negatieve upkeep: opwekking en verbruik zijn twee
	 * getallen die de speler apart moet kunnen lezen ("ik verbruik 14 van 25"),
	 * en één veld met een teken zou die twee tot één samenvatten precies waar
	 * de schaarste zit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 EnergyOutput = 0;
};

/**
 * One facility (DT_Facilities row; row name = facility id, matching the
 * DT_ClassDefs convention - SPEC-P2-03 data schema). A slot whose facility id
 * has no row here rejects gracefully at validation, never a crash (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseFacilityRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FText DisplayName;

	/** Per-level data; index 0 = L1. Array length is the max level - Workshop ships 2 entries, everything else 1 (locked decision: no L2/L3 for anything but the Workshop). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FEclipseFacilityLevelData> Levels;
};

/**
 * One authored facility slot in the vault shell (SPEC-P2-03 slot-graph). The
 * streaming ids name the level instance / Data Layer swapped in per construction
 * state (GDD 12.3, 5.4 visible growth); a missing layer is a logged warning +
 * placeholder blockout, never a crash (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseBaseSlotDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FName SlotId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FText DisplayName;

	/** Which DT_Facilities rows may occupy this slot - placement is authored, build order is free (locked decision 2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FName> AllowedFacilityRows;

	/** Slot-graph edges (every slot connects to the Spine; C adds the generator room, B the memorial alcove). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FName> AdjacentSlotIds;

	/** Streaming id for the empty state (Slot B's empty state is the 5.2 bunk camp, not raw rock). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FName EmptyStateStreamingId;

	/** Streaming id while under construction (scaffold blockout, sparks, drill loop). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FName ConstructionStreamingId;

	/** Streaming id per built level; index 0 = L1 (Workshop adds an L2 entry). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FName> LevelStreamingIds;

	/**
	 * Vanaf welke uitbreidingstrap dit slot bestaat (GDD 5.2: 4 -> 8 -> 12 -> 16
	 * over de campagne). 1 = er vanaf dag één.
	 *
	 * WAAROM DIT VELD ER IS TERWIJL ER NOG GEEN TRAP 2 BESTAAT. Het slotraster
	 * moet drie toestanden tonen — bezet, vrij, nog VERGRENDELD
	 * (`phase0/REFERENTIE_BASE_MAP.md` §2.3) — en zonder dit veld bestaat de
	 * derde niet in de data. Dan is "vergrendeld" iets dat de tekenlaag moet
	 * verzinnen, en dat is precies de fout die dit document verbiedt.
	 *
	 * WAT HIER NIET STAAT, en dat is een echt gat: GDD 5.2 geeft de LADDER
	 * (4/8/12/16) maar nergens de TRIGGER — wat de trap doet oplopen (verhaal,
	 * Command Center-niveau, aankoop, gegraven uitbreiding) is niet beslist.
	 * `ComposeBaseView` neemt de bereikte trap daarom als PARAMETER aan in
	 * plaats van hem af te leiden; zodra de trigger beslist is, vult de
	 * aanroeper hem en verandert er niets aan de logica of het scherm.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 1))
	int32 UnlockTier = 1;
};

/**
 * The slot-graph per base level (DA_BaseLayout_HollowPoint - SPEC-P2-03,
 * GDD 12.3 building system). Phase 2 ships the 4-slot Act 1 vault; the sealed
 * excavation faces (slots 5-8) are level dressing, not slots, until 5.2's
 * expansion lands in Phase 3.
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseBaseLayoutAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	TArray<FEclipseBaseSlotDef> Slots;
};

/**
 * Base construction tunables (DA_BaseTuning - SPEC-P2-03 data schema). Rush is
 * available, never comfortable (5.4: money vs. time) - retuning happens here,
 * not in code (GDD 14.2).
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseBaseTuningAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Rush = this x remaining days, paid in credits; the rush commit completes construction instantly (clock rules). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 RushCostCreditsPerDay = 60;

	/** Build days a construction crew shaves off, floor 1 total (locked decision 6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 CrewDayReduction = 1;

	/** Extra daily yield per staffed analyst at an operational facility that produces AnalystBonusResource (IC: +1 Intel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 AnalystIntelBonusPerDay = 1;

	/** Effective staff cap per site - both slice roles are 1 soldier (locked decision 6); Phase 3 splits this if roles diverge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base", meta = (ClampMin = 0))
	int32 MaxCrewPerSite = 1;

	/** The resource the analyst bonus feeds (Resource.Intel in DA_BaseTuning); empty = no analyst bonus, logged once by the wrapper (GDD 14.3.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Base")
	FGameplayTag AnalystBonusResource;
};
