#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategyLogic.h"
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
 *
 * SPEC-P2-05: this subsystem is also the liberation trigger — the single
 * writer of region flips from authored missions (locked decision 2). It
 * consumes Event.Mission.Completed, resolves the DT_LiberationInstances row
 * through the pure core (EclipseLiberationLogic), and proposes exactly one
 * transaction of existing SetRegionOwner mutations; the campaign commit emits
 * the RegionControlChanged facts. Facts, not commands: the listener never
 * mutates on the bus (GDD 14.3.4).
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

	/**
	 * Route across the live board: campaign state for ownership, the graph
	 * asset for lanes and tuning (GDD 3.1 rules 2 and 4). No graph loaded means
	 * an invalid route with a reason, never a crash (GDD 14.3.5).
	 *
	 * Not a UFUNCTION: FEclipseRoute is a pure C++ struct on purpose — routing
	 * belongs to the headless core, and wrapping it for Blueprint before a
	 * screen needs it would put a view shape in the logic layer (GDD 14.3.2).
	 */
	EclipseStrategyLogic::FEclipseRoute FindRoute(FName FromRegionId, FName ToRegionId,
		EclipseStrategyLogic::EEclipseTransitMode Mode = EclipseStrategyLogic::EEclipseTransitMode::Military,
		int32 MaxAcceptableRisk = MAX_int32) const;

	/** Is this region within supply reach of friendly ground (GDD 3.1 rule 4)? */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Strategy")
	bool IsRegionSupplied(FName RegionId) const;

private:
	/**
	 * Re-derive the Dominion Response Tier from campaign facts and commit the
	 * escalation when it is warranted (GDD 9.4). No-op when nothing changed —
	 * the ladder never descends, so "nothing to do" is the common case.
	 */
	void EscalateResponseTierIfWarranted(UEclipseCampaignSubsystem& Campaign, FName Reason);
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

	/**
	 * The liberation trigger (SPEC-P2-05 build step 3): on a mission-completion
	 * fact, resolve every triggered DT_LiberationInstances row through the pure
	 * core and commit the one resulting transaction. Obligations from the
	 * EclipseLiberationLogic.h header-doc: missing table/no-triggered-row = no
	 * flip + one logged warning, never a throw; an empty resolution is never
	 * committed (CommitTransaction rejects empty transactions); a non-empty
	 * dropped-id report warns once per resolution.
	 */
	void OnMissionCompleted(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/**
	 * The campaign's LiberationInstances table when usable (GetValidatedStoryTable
	 * pattern): wrong row struct warns once per table and disables liberations;
	 * missing table warns once per subsystem lifetime. Row-content health
	 * (duplicate triggers, unknown region ids, empty sets) is the ValidateData
	 * commandlet's job — runtime only guards what it must to iterate safely.
	 */
	const UDataTable* GetValidatedLiberationTable(const UEclipseCampaignSubsystem& Campaign) const;

	/** Liberation tables already struct-checked, keyed per table object (see ValidatedStoryTables). */
	mutable TSet<FObjectKey> ValidatedLiberationTables;

	/** One-shot 14.3.5 warnings: missing table once per lifetime; no-row/gate-closed once per mission id. */
	mutable bool bWarnedMissingLiberationTable = false;

#if !UE_BUILD_SHIPPING
	/** Eclipse.Liberation.Report (SPEC-P2-05 stap 4); alleen in dev-builds. */
	IConsoleObject* LiberationReportCommand = nullptr;
#endif
	TSet<FName> WarnedUntriggeredLiberationMissionIds;

	FEclipseEventSubscriptionHandle MissionCompletedHandle;

	IConsoleObject* FlipRegionCommand = nullptr;

	/** Debug surface for SelectMission (14.5): the fast test route's missing link. */
	IConsoleObject* SelectMissionCommand = nullptr;

	/**
	 * The lane layer's debug surface (14.5 step 4, the step before a widget):
	 * Route prints one route with its legs, Board prints every lane with its
	 * live status plus the response tier. These are what READ risk and supply
	 * today — a field nothing reads is not a field (21_quality_mandate.md).
	 */
	IConsoleObject* RouteCommand = nullptr;
	IConsoleObject* BoardCommand = nullptr;
};
