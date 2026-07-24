#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Quests/EclipseStoryTypes.h"

/**
 * Pure story-sequencing core (SPEC-P2-04, 14.5 step 2). Decides which story
 * mission is pinned where and whether a beat may commit — over plain inputs,
 * so the same functions serve the subsystem wiring, the tests and (later) the
 * soak script. StoryFlags storage lands with the v4->v5 schema break in the
 * wiring commit; until then callers pass any flag set. No engine actor
 * headers (GDD 14.3.2).
 */
namespace EclipseStoryLogic
{
	/**
	 * The story mission pinned to a region, or nullptr: a row pins when its
	 * region matches, its unlock beat is set (or empty = from campaign start)
	 * and its completion beat is NOT set (done missions never re-pin — the
	 * idempotence half of SPEC-P2-04's "exactly once" seam). First matching
	 * row in table order wins; authoring keeps one story mission per region
	 * per beat window, and the deterministic pick makes a violation visible
	 * instead of random.
	 */
	ECLIPSE_API const FEclipseStoryMissionRow* ResolvePinnedMission(
		const TArray<const FEclipseStoryMissionRow*>& Rows,
		const FGameplayTagContainer& StoryFlags,
		const FName& RegionId);

	/** True when the beat is unset — the guard that makes double-completion commit nothing twice (Brick can never join twice). */
	ECLIPSE_API bool ShouldCommitBeat(const FGameplayTagContainer& StoryFlags, const FGameplayTag& BeatTag);
}
