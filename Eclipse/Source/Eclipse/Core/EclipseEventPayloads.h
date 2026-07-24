#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EclipseEventPayloads.generated.h"

/**
 * Typed payload structs per event family (SPEC-P1-01), carried over the bus as
 * FInstancedStruct. One struct per family (not per tag) keeps the catalog table
 * readable and lets subscribers type-check a whole family at once. Fields are the
 * Phase 1 minimum named in Docs/EventCatalog.md; emitting specs (P1-02..08) extend
 * them in their own commits — payloads are transient, never serialized, so adding
 * fields is not a save-schema change (GDD 14.3.6 does not apply).
 */

/** Event.Campaign.* — campaign clock facts (SPEC-P1-02). */
USTRUCT(BlueprintType)
struct FEclipseCampaignEventPayload
{
	GENERATED_BODY()

	/** Campaign day after the mutation that emitted this event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Day = 0;
};

/** Event.Economy.* — wallet and production facts (SPEC-P1-02/03). */
USTRUCT(BlueprintType)
struct FEclipseEconomyEventPayload
{
	GENERATED_BODY()

	/** Which resource changed (Resource.* tag family lands with SPEC-P1-03 data). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag ResourceType;

	/** Signed change applied to the balance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Delta = 0;

	/** Balance after the change — UI renders this, never recomputes (single source of truth). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 NewBalance = 0;

	/** Ledger reason line (GDD 7.6 transparency applied to money: every number has an origin). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Reason;

	/** Production item this event refers to (ProductionQueued/Completed only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName ItemId;

	/** Days until completion (ProductionQueued only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 EtaDays = 0;

	/** Loadout unlock granted by a completed item (ProductionCompleted only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag LoadoutTag;
};

/** Event.Strategy.* — region-graph facts (SPEC-P1-02/04). */
USTRUCT(BlueprintType)
struct FEclipseStrategyEventPayload
{
	GENERATED_BODY()

	/** Region node id from the region graph asset (SPEC-P1-04). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName RegionId;

	// PLACEHOLDER(GDD 12.3 strategy map): owners become EEclipseRegionOwner when
	// SPEC-P1-02 defines FCampaignState; FName keeps the bus decoupled until then.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName OldOwner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName NewOwner;

	/** Mission template offered/selected at this region (MissionSelected only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName TemplateId;
};

/** Event.Prep.* — the full launch request composed by preparation (SPEC-P1-08). */
USTRUCT(BlueprintType)
struct FEclipsePrepEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName MissionId;

	/** Roster ids of the picked squad (MaxDeployed - 1 squadmates; player + 3 per SPEC-P2-01). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	TArray<FGuid> SquadSoldierIds;

	/** Chosen loadout (options gated by produced items — SPEC-P1-03/08). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag LoadoutTag;

	/** Chosen insertion point (one of the mission's entries — SPEC-P1-05). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName InsertionId;

	/** Intel spent on the briefing reveal (0 = none — SPEC-P1-08). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 IntelLevel = 0;
};

/** Event.Mission.* — mission lifecycle facts (SPEC-P1-05). */
USTRUCT(BlueprintType)
struct FEclipseMissionEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName MissionId;

	/** Completed objective (ObjectiveCompleted only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName ObjectiveId;

	/** Overall result (Completed/Failed). Detailed results struct lands with SPEC-P1-05. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	bool bSuccess = false;
};

/** Event.Squad.* — order/soldier facts in-mission (SPEC-P1-06). */
USTRUCT(BlueprintType)
struct FEclipseSquadEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid SoldierId;

	/** Order id from DT_SquadOrderDefs (SPEC-P1-06). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Order;

	/** Order target: region location id / enemy id — resolved by the order layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName TargetId;

	/** Acknowledge bark shown on screen (GDD 9.5 verbal transparency). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FString BarkLine;

	/** Refusal reason ("NoRoute", "NoLineOfSight", ...) — never empty on OrderRefused (GDD 8.4). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Reason;

	/** Cause of a SoldierDowned fact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Cause;

	/** Who performed the save (SoldierStabilized) or fired the verb (ClassAbilityUsed) — SPEC-P2-01. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid StabilizerId;

	/** Signature verb that fired (ClassAbilityUsed only; Class.Verb.* family). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag AbilityVerb;
};

/** Event.Roster.* / Event.Memorial.* — persistent-people facts (SPEC-P1-07, Pillar 3). */
USTRUCT(BlueprintType)
struct FEclipseRosterEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid SoldierId;

	/** Cause of death/wound; also the memorial "how" line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName Cause;

	/** Campaign day of the fact (memorial records carry the day forever). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Day = 0;

	/** Days unavailable (SoldierWounded only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 DaysOut = 0;
};

/** Event.Base.* — Hollow Point construction/staffing facts (SPEC-P2-03), emitted only by the CampaignState commit (GDD 14.3.3). */
USTRUCT(BlueprintType)
struct FEclipseBaseEventPayload
{
	GENERATED_BODY()

	/** The authored facility slot (UEclipseBaseLayoutAsset slot-graph). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName SlotId;

	/** DT_Facilities row occupying/entering the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FName FacilityId;

	/** ConstructionStarted: target level. FacilityBuilt: 1. FacilityUpgraded: the new level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Level = 0;

	/** ConstructionStarted: campaign day the build completes uncrewed (a crew or rush beats it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 EtaDay = 0;

	/** StaffAssigned: the (un)assigned soldier (same id type as FEclipseRosterEventPayload). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGuid SoldierId;

	/** StaffAssigned: Base.Staff.Crew / Base.Staff.Analyst (positional role at commit), empty = unassigned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag RoleTag;
};

/** Event.Story.* — committed story beats (SPEC-P2-04); emitted only by the CampaignState commit (GDD 14.3.3). */
USTRUCT(BlueprintType)
struct FEclipseStoryEventPayload
{
	GENERATED_BODY()

	/** The beat that just committed (Story.Beat.* once the content tags land; any set-only flag works). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	FGameplayTag BeatTag;

	/** Campaign day the beat committed on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 Day = 0;
};

/** Event.Command.* — Command Mode lifecycle facts (SPEC-P2-02). */
USTRUCT(BlueprintType)
struct FEclipseCommandEventPayload
{
	GENERATED_BODY()

	/** World rate applied while held (ModeEntered; from DA_CommandModeTuning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float DilationFactor = 1.0f;

	/** How long the mode was held, in wall-clock seconds (ModeExited; locked decision 2 telemetry). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	float HeldSeconds = 0.0f;

	/** Orders issued during the hold (ModeExited; the R3 usage-pull metric). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Events")
	int32 OrdersIssuedWhileHeld = 0;
};
