#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "EclipseRegionGraphAsset.generated.h"

/** District node flavors (SPEC-P1-04); each maps to one mission-offer row. */
UENUM(BlueprintType)
enum class EEclipseRegionType : uint8
{
	Industrial,
	Residential,
	Checkpoint
};

/**
 * One mission offer per region type (DT_MissionOffers, SPEC-P1-04). Offers are
 * static data in Phase 1 — the Mission Generator (GDD 11.3) replaces this table
 * in Phase 3; the context line is its hand-written stub (11.3's causal rule).
 */
USTRUCT(BlueprintType)
struct FEclipseMissionOfferRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	EEclipseRegionType RegionType = EEclipseRegionType::Industrial;

	/** Mission template this offer launches (SPEC-P1-05 asset id). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName TemplateId;

	/** One-line causal context ("generated missions must explain their existence" — GDD 11.3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FText ContextLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 RewardCredits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 RewardMaterials = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 RewardIntel = 0;
};

/**
 * What a lane IS, before campaign state is consulted (GDD 3.1 rule 2). Exactly
 * three, and each one resolves differently in EclipseStrategyLogic — a fourth
 * name without a fourth behaviour would be a boolean wearing a hat.
 *
 *   Open         — military and smuggler traffic both pass, at the lane's own cost.
 *   SpireGated   — CONDITIONAL. A Gate Spire node (GateRegionId) sits on the lane;
 *                  military transit is blocked exactly as long as that node is
 *                  hostile. Flip the Spire and the lane behaves like Open again.
 *                  While it is shut, smugglers still run it "at cost and risk".
 *   SmugglerOnly — UNCONDITIONAL. No node gates it and nothing opens it: the lane
 *                  is a service crawl, a flooded conduit, a hull seam. Military
 *                  columns never fit through, in any campaign state.
 *
 * The pair SpireGated/SmugglerOnly is not two spellings of "shut": one is a
 * lane the player can OPEN by taking ground, the other is a lane that is
 * permanently Kaya's. Routing tells them apart, and so does the test suite.
 */
UENUM(BlueprintType)
enum class EEclipseLaneStatus : uint8
{
	Open,
	SpireGated,
	SmugglerOnly
};

/**
 * One end of one undirected lane, with what it COSTS (GDD 3.1 rule 4: distance
 * = time = risk). Stored mirrored — region A lists its lane to B, region B lists
 * the same lane back to A — and EclipseStrategyLogic::ValidateGraph enforces that
 * both halves agree on every field, not just on existence. An edge that is open
 * one way and gated the other is a data bug, not a one-way street.
 *
 * The implicit constructors are deliberate: a bare region id authors an OPEN
 * lane at unit cost, so fixtures and legacy content stay one line
 * (`Lanes = { TEXT("Depot"), TEXT("Housing") }`) and only lanes that actually
 * cost something have to say so.
 */
USTRUCT(BlueprintType)
struct FEclipseLaneDefinition
{
	GENERATED_BODY()

	FEclipseLaneDefinition() = default;
	FEclipseLaneDefinition(FName InNeighborRegionId)
		: NeighborRegionId(InNeighborRegionId) {}
	FEclipseLaneDefinition(const TCHAR* InNeighborRegionId)
		: NeighborRegionId(InNeighborRegionId) {}

	/** The region on the other end. The reverse lane must exist on that region. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName NeighborRegionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	EEclipseLaneStatus Status = EEclipseLaneStatus::Open;

	/**
	 * SpireGated only: the node whose owner decides whether military transit
	 * passes. Required on SpireGated, must be empty otherwise — the validator
	 * says so, because a gate id on an Open lane reads as "this is gated" to
	 * every human who opens the asset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName GateRegionId;

	/** Strategic days to cross. Never 0: a lane you cross for free is not a lane (GDD 3.1 rule 4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 1))
	int32 TravelDays = 1;

	/** Baseline danger of crossing, 0-100. Read by routing: it breaks ties and it can put a route out of tolerance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0, ClampMax = 100))
	int32 Risk = 0;

	/** Extra days a smuggler run costs over this lane (GDD 3.1 rule 2: "at cost"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 SmugglerDelayDays = 1;

	/** Extra risk a smuggler run carries over this lane (GDD 3.1 rule 2: "and risk"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0, ClampMax = 100))
	int32 SmugglerRiskPenalty = 10;
};

/**
 * Board-wide numbers routing needs that are NOT properties of any single lane
 * (GDD 14.2: numbers are data). Lives on the graph asset; the defaults are the
 * spec values so headless tests and asset-less fallbacks route identically to
 * the shipped board (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseLaneTuning
{
	GENERATED_BODY()

	/**
	 * Risk added to EVERY leg per Dominion Response Tier (GDD 9.4). This is the
	 * whole reason the tier is state and not a label: at tier 0 the empire is
	 * not looking, at tier 5 the same road is a different road.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 RiskPerResponseTier = 4;

	/**
	 * Gate owned by nobody in particular: military transit passes, but through a
	 * firefight. The GDD 6.3.2 trichotomy applied to movement — Player-held gate
	 * is free, Contested costs, Dominion-held shuts it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 ContestedGateRiskPenalty = 15;

	/** A region further than this (in military travel days) from friendly ground is cut off (GDD 3.1 rule 4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 1))
	int32 MaxSupplyDays = 3;
};

/**
 * Static region definitions for the 6-node district (SPEC-P1-04 data schema;
 * referenced by the campaign setup asset per SPEC-P1-02).
 */
USTRUCT(BlueprintType)
struct FEclipseRegionDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	EEclipseRegionType RegionType = EEclipseRegionType::Industrial;

	/**
	 * Undirected edges WITH their price; the validator enforces symmetry of the
	 * whole record (GDD 3.1 rule 1: it is a graph, edges matter — rule 4: and
	 * they cost). Replaces the bare ConnectedRegionIds id list; pre-lane assets
	 * are folded forward once by EclipseRegionGraph::UpgradeLegacyLanes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TArray<FEclipseLaneDefinition> Lanes;

	/**
	 * Pre-lane authoring (load-only). UHT serializes this under its old name
	 * "ConnectedRegionIds", so an asset saved before lanes existed still reads
	 * its topology; PostLoad folds it into Lanes as open unit-cost lanes and
	 * empties it. Never written by code, never read after PostLoad.
	 */
	UPROPERTY()
	TArray<FName> ConnectedRegionIds_DEPRECATED;

	/**
	 * WHERE this region sits on the board, in normalised board space: X to the
	 * right, Y DOWN, both in 0..1. The drawing layer maps it to pixels; nothing
	 * in routing reads it.
	 *
	 * AUTHORED, NOT DERIVED, and that decision is the whole reason this field
	 * exists instead of a layout function.
	 *
	 * An automatic layout (force-directed, circular, layered) is the short road:
	 * it needs no content and it always produces something. On six nodes it
	 * produces an arbitrary scatter that says nothing about the district — and
	 * the standing quality mandate (`21_quality_mandate.md`) is "never the
	 * shortest way to the goal, always the best way". Authored coordinates put
	 * the Underworks UNDER, the Gate Spire above the roads it watches, and the
	 * Comms Relay at the far end of the gated lane. Then the SHAPE carries the
	 * fiction, which is exactly what GDD 3.1's four map rules ask a map to do.
	 *
	 * Reversible in one field: delete the authoring and the board falls back to
	 * the list, loudly (see `EclipseStrategyMap::ComposeMapView`), never to a
	 * scatter that pretends to be geography.
	 *
	 * DEFAULT IS DELIBERATELY OFF-BOARD (-1,-1). (0,0) is a legal corner, so a
	 * zero default would be indistinguishable from "authored top-left" and every
	 * unauthored region would silently pile up in one corner.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FVector2D BoardPosition = FVector2D(-1.0, -1.0);

	/** True once someone put this region somewhere on the board (see BoardPosition). */
	bool HasBoardPosition() const
	{
		return BoardPosition.X >= 0.0 && BoardPosition.X <= 1.0
			&& BoardPosition.Y >= 0.0 && BoardPosition.Y <= 1.0;
	}

	/** The lane to a given neighbour, or null when no lane exists (GDD 3.1 rule 1: no lane, no movement). */
	const FEclipseLaneDefinition* FindLane(FName NeighborRegionId) const
	{
		return Lanes.FindByPredicate(
			[NeighborRegionId](const FEclipseLaneDefinition& Lane) { return Lane.NeighborRegionId == NeighborRegionId; });
	}

	bool HasLaneTo(FName NeighborRegionId) const { return FindLane(NeighborRegionId) != nullptr; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	EEclipseRegionOwner StartingOwner = EEclipseRegionOwner::Dominion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0, ClampMax = 100))
	int32 StartingUnrest = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy", meta = (ClampMin = 0))
	int32 StartingGarrison = 0;

	/** Per-day yield by Resource.* tag at 100% control (SPEC-P1-03; GDD 6.3.2 factors apply per owner). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TMap<FGameplayTag, int32> BaseYieldPerDay;
};

UCLASS(BlueprintType)
class ECLIPSE_API UEclipseRegionGraphAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TArray<FEclipseRegionDefinition> Regions;

	/** Board-wide routing numbers (GDD 14.2); defaults are the spec values. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	FEclipseLaneTuning LaneTuning;

	/** Rows: FEclipseMissionOfferRow — one offer per region type (SPEC-P1-04). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Strategy")
	TSoftObjectPtr<UDataTable> MissionOffers;

	/** Folds any pre-lane authoring forward exactly once (see UpgradeLegacyLanes). */
	virtual void PostLoad() override;
};

namespace EclipseRegionGraph
{
	/**
	 * One-way migration of pre-lane content: every id still sitting in the
	 * deprecated ConnectedRegionIds becomes an OPEN lane at the FEclipseLaneDefinition
	 * defaults, and the legacy array is emptied so there is never a second source
	 * of truth. Ids that already have a lane are left alone — authored costs win
	 * over a default. Returns how many lanes it created (0 = nothing to do).
	 *
	 * A pure function on the array rather than a method on the asset, so the
	 * migration is testable headless without loading a UObject (GDD 14.3.2) —
	 * the same reason the rest of this layer is free functions.
	 */
	ECLIPSE_API int32 UpgradeLegacyLanes(TArray<FEclipseRegionDefinition>& Regions);

	/**
	 * Bare ids -> open unit-cost lanes. The one place that spelling lives, so
	 * the migration above and every fixture that only cares about topology
	 * agree on what "a plain edge" means.
	 */
	ECLIPSE_API TArray<FEclipseLaneDefinition> OpenLanes(const TArray<FName>& NeighborIds);
}
