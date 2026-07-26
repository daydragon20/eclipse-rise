#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "EclipseStoryTypes.generated.h"

class UEclipseMissionAsset;

/**
 * One story-pinned mission (DT_StoryMissions row; row name = mission id —
 * SPEC-P2-04 data schema). Story missions surface as pinned offers on the
 * existing map/offer flow (locked decision 4): a pinned row takes precedence
 * over the region-type offer; sequence M1.1 -> M1.4 is expressed purely by
 * unlock tags. Missing row = the mission is never offered and the campaign
 * runs on region offers alone (GDD 14.3.5).
 */
USTRUCT(BlueprintType)
struct FEclipseStoryMissionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Mission template id (matches the authored UEclipseMissionAsset + offer TemplateId). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story")
	FName MissionId;

	/**
	 * NIET GELEZEN. De koppeling loopt volledig via MissionId hierboven: die wordt
	 * de TemplateId van het aanbod en daarmee zoekt de missie zijn spec op. Deze
	 * pointer is een tweede manier om hetzelfde te zeggen, en de stille: wijs hem
	 * naar een andere missie dan MissionId en er verandert niets.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story")
	TSoftObjectPtr<UEclipseMissionAsset> MissionAsset;

	/** Region this mission is pinned to on the strategic map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story")
	FName PinnedRegionId;

	/** Story flag that unlocks this mission; empty = available from campaign start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story")
	FGameplayTag UnlockBeatTag;

	/**
	 * Story flag the debrief commits on success — also the "done, never
	 * re-pin" marker. Leaving this empty means the mission never retires from
	 * the pin (a repeatable story offer); the wiring layer warns on load when
	 * that is unintentional (14.3.5).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story")
	FGameplayTag CompletionBeatTag;

	/** Authored reward band (SPEC-P2-04 decision 10; overrides the generic offer rewards). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story", meta = (ClampMin = 0))
	int32 RewardCredits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story", meta = (ClampMin = 0))
	int32 RewardMaterials = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story", meta = (ClampMin = 0))
	int32 RewardIntel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story")
	FText BriefingText;

	/**
	 * NIET GELEZEN. BriefingText hierboven komt wél in het spel terecht (als
	 * ContextLine op het missieaanbod), maar zonder naam ervoor — de briefing
	 * leest dus als tekst van niemand.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Story")
	FName BriefingSpeaker;
};
