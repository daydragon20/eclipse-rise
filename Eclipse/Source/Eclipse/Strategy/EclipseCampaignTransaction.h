#pragma once

#include "CoreMinimal.h"
#include "Base/EclipseBaseLogic.h"
#include "GameplayTagContainer.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "EclipseCampaignTransaction.generated.h"

/**
 * The transaction API — the only writer of strategic state (GDD 14.3.3:
 * "Ground gameplay proposes; the transaction commits"). Pure logic, no engine
 * actor headers: validation and application are free functions over
 * FEclipseCampaignState so they compile and unit-test headless (GDD 14.3.2).
 *
 * Atomicity contract (SPEC-P1-02): a transaction validates completely against
 * a copy-in-progress before anything is visible; one invalid mutation rejects
 * the whole transaction.
 */

UENUM()
enum class EEclipseCampaignMutationType : uint8
{
	AdjustResource,
	SetRegionOwner,
	AddSoldier,
	KillSoldier,
	AddMemorialEntry,
	QueueProduction,
	CompleteProduction,
	WoundSoldier,
	AdvanceDay,
	MarkMissionServed,
	// SPEC-P2-03 base mutations (14.3.3: base state changes only through here).
	StartConstruction,
	RushConstruction,
	AssignStaff,
	// SPEC-P2-04: story beats are facts in campaign state, set-only in Phase 2.
	SetStoryFlag
};

/**
 * One typed mutation. Deliberately a fat variant struct: serializable, BP-friendly,
 * and adding a mutation type is one enum case + one validate/apply branch — the
 * cost we pay for not templating the save format into unreadability.
 */
USTRUCT(BlueprintType)
struct FEclipseCampaignMutation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	EEclipseCampaignMutationType Type = EEclipseCampaignMutationType::AdvanceDay;

	/** AdjustResource */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FGameplayTag ResourceType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	int32 Amount = 0;

	/** Ledger reason — every number must be explainable (GDD 7.6 transparency, 6.3 audit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName Reason;

	/** SetRegionOwner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	EEclipseRegionOwner NewOwner = EEclipseRegionOwner::Dominion;

	/** AddSoldier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FEclipseSoldierRecord SoldierRecord;

	/** KillSoldier / WoundSoldier / MarkMissionServed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FGuid SoldierId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName Cause;

	/** AddMemorialEntry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FEclipseMemorialEntry MemorialEntry;

	/** SetStoryFlag (SPEC-P2-04): the beat to commit; composers filter already-set beats via EclipseStoryLogic::ShouldCommitBeat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FGameplayTag StoryFlagTag;

	/** QueueProduction / CompleteProduction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName ProductionItemId;

	/** QueueProduction: days to completion. StartConstruction: build days from DT_Facilities (applied with the never-instant floor of 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	int32 EtaDays = 0;

	/** CompleteProduction: loadout option the finished item unlocks (SPEC-P1-03). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FGameplayTag LoadoutTag;

	/** StartConstruction / RushConstruction / AssignStaff: the base slot (SPEC-P2-03). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName SlotId;

	/** StartConstruction: DT_Facilities row to build/upgrade at the slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName FacilityId;

	/**
	 * AssignStaff: a valid tag assigns (SoldierId at SlotId), an empty tag
	 * unassigns (SPEC-P2-03: "none = unassign"). The stored role stays
	 * positional - staff on a building site is the crew, staff on an
	 * operational site is the analyst - so the tag is intent, not state; the
	 * emitted StaffAssigned fact reports the role the assignment actually took.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FGameplayTag StaffRoleTag;

	/**
	 * AdvanceDay (construction tick) / AssignStaff (cap validation):
	 * DA_BaseTuning values, stamped onto the mutation by the campaign subsystem
	 * at commit time so every caller - hub, debrief, console - runs the same
	 * data-driven numbers (GDD 14.2). Defaults are the SPEC-P2-03 spec values,
	 * so headless tests and setups without a tuning asset behave to spec
	 * (GDD 14.3.5). CrewDayReduction is consumed by AdvanceDay only;
	 * MaxStaffPerSite by both.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	int32 CrewDayReduction = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	int32 MaxStaffPerSite = 1;
};

USTRUCT(BlueprintType)
struct FEclipseCampaignTransaction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	TArray<FEclipseCampaignMutation> Mutations;

	/** Where this transaction came from (debrief, debug console, economy tick) — audit trail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName Source;
};

/**
 * Record of an applied mutation with the before-facts events need (old owner,
 * new balance). The subsystem translates these into bus events; the pure core
 * never touches the bus (GDD 14.3.1 seam kept clean).
 */
struct FEclipseAppliedMutation
{
	FEclipseCampaignMutation Mutation;
	EEclipseRegionOwner OldOwner = EEclipseRegionOwner::Dominion;
	int32 NewBalance = 0;
	int32 DayAfter = 0;
	int32 ProductionCompletesOnDay = 0;

	/** AdvanceDay (construction tick) / RushConstruction: build steps completed by this mutation (SPEC-P2-03). */
	TArray<EclipseBaseLogic::FEclipseFacilityCompletion> FacilityCompletions;

	/** StartConstruction: the level this order builds toward, and the campaign day it completes uncrewed. */
	int32 TargetLevel = 0;
	int32 EtaDay = 0;

	/** AssignStaff (assign only): true when the site was under construction at apply time - the positional crew role. */
	bool bAssignedAsCrew = false;

	/** AssignStaff: the facility occupying the slot at apply time (the mutation itself only names the slot). */
	FName StaffedFacilityId;

	/** KillSoldier / WoundSoldier: base posts the casualty vacated in this apply - the commit emits StaffAssigned(RoleTag none) per entry (SPEC-P2-03). */
	TArray<EclipseBaseLogic::FEclipseStaffRelease> StaffReleases;
};

namespace EclipseCampaignLogic
{
	/** True if the mutation is legal against the given state; OutError explains the first violation. */
	ECLIPSE_API bool ValidateMutation(const FEclipseCampaignState& State, const FEclipseCampaignMutation& Mutation, FString& OutError);

	/** Applies one mutation. Caller guarantees prior validation; invalid input here is a programming error (checked). */
	ECLIPSE_API FEclipseAppliedMutation ApplyMutation(FEclipseCampaignState& State, const FEclipseCampaignMutation& Mutation);

	/**
	 * Validate-all-then-apply-all (atomic). On success, State is advanced and
	 * OutApplied lists effect records in order; on failure, State is untouched
	 * and OutError names the offending mutation.
	 */
	ECLIPSE_API bool CommitTransaction(FEclipseCampaignState& State, const FEclipseCampaignTransaction& Transaction, TArray<FEclipseAppliedMutation>& OutApplied, FString& OutError);
}
