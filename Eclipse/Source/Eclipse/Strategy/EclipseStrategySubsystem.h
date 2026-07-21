#pragma once

#include "CoreMinimal.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseStrategySubsystem.generated.h"

class IConsoleObject;

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

	IConsoleObject* FlipRegionCommand = nullptr;
};
