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

/**
 * The empire's temperature (GDD 9.4, verbatim ladder). Campaign state, not a
 * display value: EclipseStrategyLogic adds LaneTuning.RiskPerResponseTier to
 * every routing leg per tier, so the same road is a different road at tier 4.
 *
 * Monotone by rule — the transaction API refuses a decrease. The Dominion does
 * not forget that you exist; de-escalation would need its own authored fact
 * (a peace, a decapitation), not an accounting slip.
 *
 * Tiers 4 and 5 are reachable only by an authored SetResponseTier: their GDD
 * triggers are "multi-planet" and "Act 4", and a one-district slice cannot
 * observe either. DeriveResponseTier says so out loud rather than pretending.
 */
UENUM(BlueprintType)
enum class EEclipseDominionResponseTier : uint8
{
	/** Pre-story. Police only. */
	Indifference = 0,
	/** Act 1. Veil investigations, checkpoints. */
	Nuisance = 1,
	/** Regional attacks. Garrison reinforcement, patrol density, bounties. */
	Insurgency = 2,
	/** First liberation. Military occupation, economic blockades, named counter-operations. */
	Rebellion = 3,
	/** Multi-planet. Kaine offensives under the player's own supply rules. */
	War = 4,
	/** Act 4. Total mobilization, AEGIS protocol, scorched earth. */
	Existential = 5
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

	/**
	 * DT_ClassDefs row id (SPEC-P2-01, GDD 4.2.3). NAME_None = classless
	 * Recruit — pre-v3 saves migrate to that, and the Academy assignment flow
	 * is Phase 3. Muster shows it read-only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName ClassId;

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
 * Shared base-identity ids (SPEC-P2-03 locked decision 2). These are the data
 * contract between the campaign state's seeded default below, the save
 * migration, and the authored DA_BaseLayout/DT_Facilities assets - the assets
 * must use these ids for the pre-built Command Center or migrated saves and
 * asset-less fallbacks (GDD 14.3.5) would disagree about slot A.
 */
namespace EclipseBaseDefaults
{
	inline const FName CommandSlotId(TEXT("Slot_A"));
	inline const FName CommandCenterFacilityId(TEXT("CommandCenter"));
}

/**
 * One facility slot's mutable state (SPEC-P2-03). Level = highest COMPLETED
 * level (0 = nothing built yet), so "operational at level X" stays queryable
 * during upgrades; DaysRemaining > 0 = construction toward Level + 1 is in
 * progress. Static definition (allowed rows, adjacency, streaming ids) lives in
 * UEclipseBaseLayoutAsset; costs/timers/yields in DT_Facilities (GDD 14.2).
 */
USTRUCT(BlueprintType)
struct FEclipseFacilityState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName SlotId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FName FacilityId;

	/** Highest completed level; 0 = under first construction or (transiently) empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 Level = 0;

	/** 0 = operational; > 0 = days of construction left toward Level + 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	int32 DaysRemaining = 0;

	/**
	 * Staff assignment lives in base state; the roster record is untouched and
	 * muster validation reads this (SPEC-P2-03 staffing v1). Role is positional:
	 * staff on a site under construction is the crew, staff on an operational
	 * site is the analyst - no role enum until Phase 3 needs one.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FGuid> AssignedSoldierIds;
};

/**
 * The walkable base (SPEC-P2-03). Default-constructed state already contains
 * the pre-built L1 Command Center (5.3.1 mandatory core, locked decision 2): an
 * empty base is not a legal campaign, so the spec start state is the TYPE's
 * default - new campaigns, migrated pre-v4 saves and asset-less fallbacks all
 * land on it deterministically (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseBaseState
{
	GENERATED_BODY()

	FEclipseBaseState()
	{
		FEclipseFacilityState& CommandCenter = Facilities.AddDefaulted_GetRef();
		CommandCenter.SlotId = EclipseBaseDefaults::CommandSlotId;
		CommandCenter.FacilityId = EclipseBaseDefaults::CommandCenterFacilityId;
		CommandCenter.Level = 1;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FEclipseFacilityState> Facilities;

	const FEclipseFacilityState* FindBySlot(FName SlotId) const
	{
		return Facilities.FindByPredicate([SlotId](const FEclipseFacilityState& F) { return F.SlotId == SlotId; });
	}

	FEclipseFacilityState* FindBySlot(FName SlotId)
	{
		return Facilities.FindByPredicate([SlotId](const FEclipseFacilityState& F) { return F.SlotId == SlotId; });
	}
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

	/** Bumped on breaking layout change; the save system routes migrations off it. v2: +UnlockedLoadoutTags (SPEC-P1-03). v3: +Roster ClassIds tail (SPEC-P2-01). v4: +BaseState tail (SPEC-P2-03). v5: +StoryFlags tail (SPEC-P2-04). v6: +ResponseTier tail (GDD 9.4). */
	UPROPERTY(VisibleAnywhere, Category = "Eclipse|Campaign")
	int32 SchemaVersion = 6;

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

	/** The walkable base (SPEC-P2-03); starts with the pre-built Command Center via FEclipseBaseState's seeded default. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	FEclipseBaseState BaseState;

	/**
	 * Committed story beats (SPEC-P2-04): set-only in Phase 2, written solely
	 * by the SetStoryFlag mutation. Pre-v5 saves migrate to empty — a campaign
	 * that predates the story layer simply hasn't reached a beat yet.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	TArray<FGameplayTag> StoryFlags;

	/**
	 * The Dominion Response Tier (GDD 9.4). Written only by the SetResponseTier
	 * mutation, which refuses to lower it. Pre-v6 saves come home at
	 * Indifference — a campaign that predates the strategic opponent has not
	 * been noticed yet, which is exactly tier 0.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eclipse|Campaign")
	EEclipseDominionResponseTier ResponseTier = EEclipseDominionResponseTier::Indifference;

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
