#pragma once

#include "CoreMinimal.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/ObjectKey.h"
#include "EclipseStrategySubsystem.generated.h"

class IConsoleObject;
class UEclipseCampaignSubsystem;

/** One selectable offer as the map screen renders it (SPEC-P1-04). */
USTRUCT(BlueprintType)
struct FEclipseMissionOfferView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName RegionId;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Strategy")
	FName TemplateId;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Strategy")
	FText ContextLine;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Strategy")
	int32 RewardCredits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Strategy")
	int32 RewardMaterials = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Strategy")
	int32 RewardIntel = 0;
};

/**
 * Strategy-board service (SPEC-P1-04): resolves the region graph + offer table
 * against campaign state and turns a player's pick into
 * Event.Strategy.MissionSelected. The board itself never mutates state —
 * region flips arrive exclusively through CampaignState commits (GDD 14.3.3).
 */
UCLASS()
class ECLIPSE_API UEclipseStrategySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Offers for all currently legal targets (adjacency rule applied), definition order. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Strategy")
	TArray<FEclipseMissionOfferView> GetAvailableOffers() const;

	/** Player picked an offer; broadcasts MissionSelected on success. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Strategy")
	bool SelectMission(FName RegionId, FString& OutError);

	/** Resolve the offer (incl. rewards) for a region regardless of current legality — the mission runtime's lookup at launch time. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Strategy")
	bool TryGetOffer(FName RegionId, FEclipseMissionOfferView& OutOffer) const;

private:
	const UEclipseRegionGraphAsset* ResolveGraph() const;
	const FEclipseMissionOfferRow* FindOfferForType(const UEclipseRegionGraphAsset& Graph, EEclipseRegionType RegionType) const;

	/**
	 * The one offer-resolution path (SPEC-P2-04 locked decision 4): a story
	 * mission pinned to the region (unlocked, not done — EclipseStoryLogic
	 * decides over StoryFlags) takes precedence over the region-type offer.
	 * All three query sites route through here so precedence can never drift.
	 */
	bool ResolveOfferForRegion(const UEclipseRegionGraphAsset& Graph, FName RegionId, FEclipseMissionOfferView& OutOffer) const;

	/**
	 * Returns the campaign's StoryMissions table when usable, running the loud
	 * one-time-per-table validation the 14.3.5 ladder and the row contract
	 * (EclipseStoryTypes.h) promise: wrong row struct disables pins; an empty
	 * CompletionBeatTag (never-retiring pin), an unknown PinnedRegionId (story
	 * that can never surface) and same-unlock double pins each warn per row.
	 */
	const UDataTable* GetValidatedStoryTable(const UEclipseCampaignSubsystem& Campaign, const UEclipseRegionGraphAsset& Graph) const;

	/** Story tables already taken through the loud validation pass — keyed per table object, so a fresh table re-validates (not once per process). */
	mutable TSet<FObjectKey> ValidatedStoryTables;

	IConsoleObject* FlipRegionCommand = nullptr;
};
