#include "Quests/EclipseStoryLogic.h"

namespace EclipseStoryLogic
{

const FEclipseStoryMissionRow* ResolvePinnedMission(
	const TArray<const FEclipseStoryMissionRow*>& Rows,
	const FGameplayTagContainer& StoryFlags,
	const FName& RegionId)
{
	for (const FEclipseStoryMissionRow* Row : Rows)
	{
		if (Row == nullptr || Row->PinnedRegionId != RegionId)
		{
			continue; // null rows are data damage, not a crash (GDD 14.3.5)
		}
		const bool bUnlocked = !Row->UnlockBeatTag.IsValid() || StoryFlags.HasTagExact(Row->UnlockBeatTag);
		const bool bDone = Row->CompletionBeatTag.IsValid() && StoryFlags.HasTagExact(Row->CompletionBeatTag);
		if (bUnlocked && !bDone)
		{
			return Row;
		}
	}
	return nullptr;
}

bool ShouldCommitBeat(const FGameplayTagContainer& StoryFlags, const FGameplayTag& BeatTag)
{
	return BeatTag.IsValid() && !StoryFlags.HasTagExact(BeatTag);
}

} // namespace EclipseStoryLogic
