#pragma once

#include "CoreMinimal.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "Strategy/EclipseRegionGraphAsset.h"

/**
 * Strategy-board rules as pure functions (GDD 14.3.2). GDD 3.1 scaled down to
 * one district: control is per-region, edges matter, and you can only strike
 * from territory you hold (SPEC-P1-04 adjacency rule).
 *
 * Lanes (this file's second half) add what an edge COSTS and whether it is OPEN
 * — GDD 3.1 rules 2 and 4. Nothing here touches the campaign; routing reads
 * state and definitions and answers questions. The board never mutates.
 */
namespace EclipseStrategyLogic
{
	/** Who is asking to cross. The whole point of GDD 3.1 rule 2 is that these two get different answers. */
	enum class EEclipseTransitMode : uint8
	{
		/** Columns, armour, garrison rotations. Blocked by a hostile Gate Spire. */
		Military,
		/** Kaya's business. Passes where the military cannot, at cost and at risk. */
		Smuggler
	};

	/** What crossing ONE lane costs right now, given who owns what. */
	struct FEclipseLaneTransit
	{
		bool bPassable = false;

		/** Crossing time in strategic days, including any smuggler surcharge. */
		int32 TravelDays = 0;

		/** Danger of this leg: lane risk + smuggler penalty + contested-gate penalty + response-tier surcharge. */
		int32 Risk = 0;

		/** True when this crossing is only possible because smugglers were asked. */
		bool bSmugglerLeg = false;

		/** Why not, when bPassable is false — the map screen owes the player a reason (GDD 9.5 applied to geography). */
		FString BlockedReason;
	};

	/** One leg of a route, in travel order. */
	struct FEclipseRouteStep
	{
		FName FromRegionId;
		FName ToRegionId;
		int32 TravelDays = 0;
		int32 Risk = 0;
		bool bSmugglerLeg = false;
	};

	/**
	 * The answer to "how do I get from here to there, and what does it cost".
	 * Three distinguishable outcomes, which is the whole design:
	 *   bValid && !bUsesSmugglerLeg — a military route exists.
	 *   bValid &&  bUsesSmugglerLeg — only smugglers get through (Mode::Smuggler).
	 *   !bValid                     — no route at all; FailureReason says which wall.
	 */
	struct FEclipseRoute
	{
		bool bValid = false;

		/** Start..goal inclusive; empty when invalid. */
		TArray<FName> RegionPath;

		TArray<FEclipseRouteStep> Steps;

		int32 TotalTravelDays = 0;
		int32 TotalRisk = 0;

		/** True when at least one leg was crossed as a smuggler run. */
		bool bUsesSmugglerLeg = false;

		FString FailureReason;
	};

	struct FEclipseRouteQuery
	{
		FName StartRegionId;
		FName GoalRegionId;

		EEclipseTransitMode Mode = EEclipseTransitMode::Military;

		/**
		 * A route whose total risk exceeds this is refused as if it did not
		 * exist — the commander who will not send people through that. Default
		 * accepts anything, so a caller that does not care never pays for it.
		 */
		int32 MaxAcceptableRisk = MAX_int32;

		/** Board-wide numbers; defaults are the spec values (GDD 14.3.5). */
		FEclipseLaneTuning Tuning;
	};

	/**
	 * A region is a legal mission target iff it exists, is not already
	 * player-held, and borders at least one player-held region over a lane that
	 * is passable in SOME mode. A lane nobody can cross is not a border.
	 */
	ECLIPSE_API bool IsMissionTargetLegal(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName TargetRegionId, FString& OutReason);

	/** All currently legal target region ids, in definition order (deterministic for UI and tests). */
	ECLIPSE_API TArray<FName> GetLegalMissionTargets(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions);

	/**
	 * How you would have to reach a legal target from friendly ground.
	 * Returns false for an illegal target; otherwise OutMode is Military when at
	 * least one bordering player-held region has a militarily passable lane, and
	 * Smuggler when every such border is shut to columns. The strategy screen
	 * prices these differently; today the console board prints them differently.
	 */
	ECLIPSE_API bool ClassifyMissionApproach(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName TargetRegionId, const FEclipseLaneTuning& Tuning, EEclipseTransitMode& OutMode);

	/**
	 * Structural graph validation (SPEC-P1-04 test contract + ValidateData rule):
	 * non-empty unique ids, lanes reference existing regions, no orphan nodes,
	 * lanes symmetric — including their COST and STATUS, because a lane that is
	 * open one way and gated the other is not a one-way street, it is a typo.
	 * Also: a SpireGated lane must name an existing gate region, and no other
	 * status may name one. Returns true when OutErrors stays empty.
	 */
	ECLIPSE_API bool ValidateGraph(const TArray<FEclipseRegionDefinition>& Definitions, TArray<FString>& OutErrors);

	/**
	 * What one lane costs right now. The three statuses resolve here, and this
	 * is the only place they do:
	 *   Open          — passable both modes at the lane's own cost.
	 *   SpireGated    — gate owner Player: as Open. Contested: passable, plus
	 *                   ContestedGateRiskPenalty. Dominion: military refused,
	 *                   smuggler passes at the lane's smuggler surcharge.
	 *   SmugglerOnly  — military always refused; smuggler passes at surcharge.
	 * Response tier adds RiskPerResponseTier to every passable leg (GDD 9.4).
	 */
	ECLIPSE_API FEclipseLaneTransit ResolveLaneTransit(const FEclipseCampaignState& State, const FEclipseLaneDefinition& Lane, EEclipseTransitMode Mode, const FEclipseLaneTuning& Tuning);

	/**
	 * Can this mode get there at all, ignoring cost? Breadth-first, so it stays
	 * cheap enough to answer "why did the route fail" — which is its job: a
	 * military refusal reads differently when smugglers CAN reach the goal
	 * (take a Spire, or hire Kaya) than when the graph is simply cut (neither
	 * will help).
	 */
	ECLIPSE_API bool IsReachable(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName StartRegionId, FName GoalRegionId, EEclipseTransitMode Mode, const FEclipseLaneTuning& Tuning);

	/**
	 * Cheapest route by travel days, ties broken by total risk and then by
	 * region id, so the same board always yields the same path (tests and UI
	 * both depend on that). Dijkstra over the lane costs; a leg that
	 * ResolveLaneTransit refuses simply is not an edge.
	 */
	ECLIPSE_API FEclipseRoute FindRoute(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, const FEclipseRouteQuery& Query);

	/**
	 * GDD 3.1 rule 4, made falsifiable: a region is supplied when a MILITARY
	 * route reaches it from any player-held region within Tuning.MaxSupplyDays.
	 * A player-held region supplies itself. Cut the lane and this goes false —
	 * that is what "a cut lane strangles an economy" has to mean before it can
	 * mean anything on a balance sheet.
	 */
	ECLIPSE_API bool IsRegionSupplied(const FEclipseCampaignState& State, const TArray<FEclipseRegionDefinition>& Definitions, FName RegionId, const FEclipseLaneTuning& Tuning);

	/**
	 * The LOWEST tier the campaign's own facts justify (GDD 9.4 triggers,
	 * scaled to one district):
	 *   0 Indifference — nothing has happened yet.
	 *   1 Nuisance     — at least one story beat is committed (the story began).
	 *   2 Insurgency   — at least one region is Contested (you attacked).
	 *   3 Rebellion    — at least two regions are player-held (you took ground).
	 * 4 and 5 are NOT derived: their triggers are "multi-planet" and "Act 4",
	 * which a one-district campaign cannot observe. They are reachable, but only
	 * through an authored SetResponseTier — a story decision, not an inference.
	 *
	 * Never returns lower than State.ResponseTier: this answers "should it go
	 * up", and the ladder does not descend.
	 */
	ECLIPSE_API EEclipseDominionResponseTier DeriveResponseTier(const FEclipseCampaignState& State);
}
