#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EclipseCampaignTypes.generated.h"

/**
 * The entire strategic state as one serializable struct tree (GDD 12.2 rule 4,
 * SPEC-P1-02). This file is pure data — no engine actor headers (GDD 14.3.2) —
 * so campaign logic stays headless-testable forever. The only legal writer is
 * the transaction API (EclipseCampaignTransaction.h); everything else reads.
 */

/** Who holds a region. Phase 1 keeps the GDD 6.3.2 trichotomy (yield: 100%/40%/0%). */
UENUM(BlueprintType)
enum class EEclipseRegionOwner : uint8
{
	Dominion,
	Contested,
	Player
};

UENUM(BlueprintType)
enum class EEclipseSoldierStatus : uint8
{
	Available,
	Deployed,
	Wounded,
	Dead
};

/** One region node's mutable state. Static definition (edges, type, yields) lives in the region graph asset (SPEC-P1-04). */
USTRUCT(BlueprintType)
struct FEclipseRegionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	EEclipseRegionOwner Owner = EEclipseRegionOwner::Dominion;

	/** 0–100; recruitment/mission-offer input in later specs (GDD 9.2 district mood, scaled down). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 Unrest = 0;

	/** Abstract defense strength; consumed by mission consequences (SPEC-P1-05). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 GarrisonStrength = 0;
};

/** One human being (Pillar 3: People, Not Units). Names are data, never hardcoded. */
USTRUCT(BlueprintType)
struct FEclipseSoldierRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FGuid SoldierId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FString Name;

	/** Origin pool tag; drives name/trait generation and future accent sets (GDD 4.2.2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName OriginId;

	/** One visible trait line in Phase 1 (SPEC-P1-07 stub of GDD 4.2.2's three). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName TraitId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 MissionsServed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	EEclipseSoldierStatus Status = EEclipseSoldierStatus::Available;

	/** Campaign day the soldier returns to Available (Wounded only — SPEC-P1-07 injury stub). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 WoundedUntilDay = 0;
};

/** A memorial record. The wall never resets (SPEC-P1-07); entries snapshot the name so they outlive the roster row. */
USTRUCT(BlueprintType)
struct FEclipseMemorialEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FGuid SoldierId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 MissionsServed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName Cause;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 Day = 0;
};

/** The single Phase 1 production slot (SPEC-P1-03's one real choice). */
USTRUCT(BlueprintType)
struct FEclipseProductionOrder
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 CompletesOnDay = 0;
};

/**
 * The campaign. Serialized member-by-member (explicit operator<< in the save
 * provider) so byte layout is deliberate; SchemaVersion changes require a
 * migration entry + test in the same commit (GDD 14.3.6).
 */
USTRUCT(BlueprintType)
struct FEclipseCampaignState
{
	GENERATED_BODY()

	/** Bumped on breaking layout change; the save system routes migrations off it. v2: +UnlockedLoadoutTags (SPEC-P1-03). */
	UPROPERTY(VisibleAnywhere, Category = "Eclipse|Campaign")
	int32 SchemaVersion = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 Day = 0;

	/** Resource balances keyed by Resource.* tag (types are data — GDD 14.2). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TMap<FGameplayTag, int32> Wallet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FEclipseRegionState> Regions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FEclipseSoldierRecord> Roster;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FEclipseMemorialEntry> Memorial;

	/** 0 or 1 entries in Phase 1; slot count becomes data when facilities land (GDD 5.3). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FEclipseProductionOrder> ProductionQueue;

	/** Loadout options earned by completed production (SPEC-P1-03: the choice must matter next mission). Append-only, insertion-ordered. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FGameplayTag> UnlockedLoadoutTags;

	int32 GetBalance(const FGameplayTag& ResourceType) const
	{
		const int32* Found = Wallet.Find(ResourceType);
		return Found != nullptr ? *Found : 0;
	}

	const FEclipseRegionState* FindRegion(FName RegionId) const
	{
		return Regions.FindByPredicate([RegionId](const FEclipseRegionState& R) { return R.RegionId == RegionId; });
	}

	const FEclipseSoldierRecord* FindSoldier(const FGuid& SoldierId) const
	{
		return Roster.FindByPredicate([&SoldierId](const FEclipseSoldierRecord& S) { return S.SoldierId == SoldierId; });
	}

	/**
	 * Deterministic content hash (SPEC-P1-02 test + save-integrity contract).
	 * Explicit field-order hashing — reflection iteration order is not a
	 * stability contract; this function is.
	 */
	ECLIPSE_API uint32 ComputeStateHash() const;
};
