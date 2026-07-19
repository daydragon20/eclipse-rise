#pragma once

#include "CoreMinimal.h"
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
	AdvanceDay
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

	/** KillSoldier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FGuid SoldierId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName Cause;

	/** AddMemorialEntry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FEclipseMemorialEntry MemorialEntry;

	/** QueueProduction / CompleteProduction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FName ProductionItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	int32 EtaDays = 0;

	/** CompleteProduction: loadout option the finished item unlocks (SPEC-P1-03). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Campaign")
	FGameplayTag LoadoutTag;
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
