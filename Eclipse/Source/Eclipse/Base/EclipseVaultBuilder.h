#pragma once

#include "CoreMinimal.h"
#include "Base/EclipseBaseTypes.h"
#include "Containers/ArrayView.h"
#include "Strategy/EclipseCampaignTypes.h"

class UWorld;

/**
 * Runtime walkable-vault builder (SPEC-P2-03 step 4-5: the Hollow Point Act 1
 * geothermal vault). Same regime as the district graybox builder
 * (Core/EclipseGrayboxBuilder): generated from code so the layout is
 * reproducible and reviewable in diffs, dressed through the authored toon
 * master, replaced by an authored .umap at the art pass. The difference is the
 * input: the district is static level content, the vault renders the CAMPAIGN
 * STATE - which facility occupies which authored slot at which level (GDD 5.4:
 * "the base is the progress bar of the whole game"). It re-renders via the
 * UEclipseBaseSubsystem Event.Base.* consumer, never on a tick (GDD 12.4).
 *
 * Truth discipline: PlanSlots() is the single placement decision - a pure,
 * headless mapping from (layout slots, FEclipseBaseState) to per-slot plans.
 * BuildVault() only executes a plan. The parity Gauntlet in EclipseBaseTests
 * holds this seam: what the vault shows == what the state says == what the
 * menu hub renders, and two builds on the same state are identical.
 *
 * GDD 14.3.5: a missing layout asset is a loud log + an empty vault (anchor
 * only); missing materials degrade to the engine shape material; never a crash.
 */
namespace EclipseVault
{
	/** Non-slot node ids the slot-graph's AdjacentSlotIds may reference (SPEC-P2-03 slot-graph). */
	inline const FName SpineNodeId(TEXT("Spine"));
	inline const FName MemorialNodeId(TEXT("MemorialAlcove"));
	inline const FName GeneratorNodeId(TEXT("GeneratorRoom"));

	/**
	 * The per-slot visual read (SPEC-P2-03 visible-growth table). One value by
	 * priority: DaysRemaining > 0 reads Constructing even during an upgrade -
	 * the plan's Level field still carries the operational blockout underneath.
	 */
	enum class EEclipseVaultSlotVisual : uint8
	{
		Empty,
		Constructing,
		Built,
	};

	/** Stable string form of the visual read (marker tags + test assertions). */
	ECLIPSE_API const TCHAR* VisualToString(EEclipseVaultSlotVisual Visual);

	/**
	 * One slot's deterministic placement decision - pure data derived from the
	 * authored slot def and the slot's FEclipseFacilityState. This struct IS the
	 * parity contract: every field the builder physically expresses.
	 */
	struct FEclipseVaultSlotPlan
	{
		FName SlotId;

		/** DT_Facilities row occupying the slot; NAME_None when empty. */
		FName FacilityId;

		/** Highest completed level from state (0 = nothing built). */
		int32 Level = 0;

		/** Construction days left from state (drives the Constructing read). */
		int32 DaysRemaining = 0;

		EEclipseVaultSlotVisual Visual = EEclipseVaultSlotVisual::Empty;

		/** Assigned staff from state - rendered as idler figures (crew/analyst by position). */
		int32 StaffCount = 0;

		/**
		 * The per-state streaming id the layout declares for this read (GDD 12.3
		 * state swaps). NAME_None when the layout declares none; the graybox
		 * blockout stands in for the layer either way until the art pass.
		 */
		FName StreamingId;

		/** Slot declares an authored empty state (Slot B's 5.2 bunk camp) instead of raw rock. */
		bool bAuthoredEmptyState = false;

		/** Slot-graph annexes off this chamber (SPEC-P2-03: B -> memorial alcove, C -> generator room). */
		bool bHasMemorialAnnex = false;
		bool bHasGeneratorAnnex = false;

		/** State row without a layout slot (stale save / trimmed layout): still shown, tagged, never hidden. */
		bool bOrphan = false;

		/** Deterministic chamber placement (world space): index-alternating off the Spine. */
		FVector ChamberCenter = FVector::ZeroVector;
		bool bNorthSide = false;
	};

	/**
	 * What one physically placed slot marker reports back from a world (parsed
	 * from the marker's tags). The Gauntlet compares this against the state.
	 */
	struct FEclipseVaultSlotView
	{
		FName SlotId;
		FName FacilityId;
		int32 Level = 0;
		EEclipseVaultSlotVisual Visual = EEclipseVaultSlotVisual::Empty;
		int32 StaffCount = 0;
		bool bOrphan = false;
	};

	/**
	 * The pure placement decision (headless, deterministic - GDD 14.3.2 spirit):
	 * layout slots in authored order, then any orphaned state rows appended so
	 * state is never hidden (the Eclipse.Base.Report rule). Chamber positions
	 * derive from the index alone.
	 */
	ECLIPSE_API TArray<FEclipseVaultSlotPlan> PlanSlots(TConstArrayView<FEclipseBaseSlotDef> Slots, const FEclipseBaseState& BaseState);

	/**
	 * Content hash over the plan array (every parity field, in order - chamber
	 * positions are a pure function of index + count, so order covers geometry).
	 * This is the re-render coalescer of the UEclipseBaseSubsystem consumer: one
	 * campaign commit emits up to four Event.Base.* facts against the SAME
	 * post-commit state, so equal hash = identical vault = skip the rebuild.
	 * Process-local (FName hashing) and never persisted - a dirty check, not a
	 * save key; ComputeVaultSignature is the durable one.
	 */
	ECLIPSE_API uint32 ComputePlanHash(TConstArrayView<FEclipseVaultSlotPlan> Plans);

	/** True if the world already carries a vault (anchor present). */
	ECLIPSE_API bool IsVaultPresent(UWorld& World);

	/**
	 * Spawn the vault from the plan: shell (Spine, airlock, sealed excavation
	 * faces), one chamber per plan entry with its state read, interaction-point
	 * markers, global-growth lighting. Null layout = loud log + empty vault
	 * (GDD 14.3.5). Instanced meshes throughout (GDD 12.4); zero tick work.
	 */
	ECLIPSE_API void BuildVault(UWorld& World, const UEclipseBaseLayoutAsset* Layout, const FEclipseBaseState& BaseState);

	/** Tear down every vault actor and build fresh - the Event.Base.* re-render path. */
	ECLIPSE_API void RebuildVault(UWorld& World, const UEclipseBaseLayoutAsset* Layout, const FEclipseBaseState& BaseState);

	/** Read back the placed slot markers (sorted by slot id) - the Gauntlet's parity probe. */
	ECLIPSE_API TArray<FEclipseVaultSlotView> ReadSlotMarkers(UWorld& World);

	/**
	 * Deterministic content signature of the placed vault (sorted marker tags +
	 * locations and every mesh instance transform; actor names excluded - they
	 * are engine counters, not content). Two builds on the same state must
	 * produce identical signatures (the determinism Gauntlet).
	 */
	ECLIPSE_API TArray<FString> ComputeVaultSignature(UWorld& World);
}
