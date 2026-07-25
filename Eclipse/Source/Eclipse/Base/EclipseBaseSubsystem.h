#pragma once

#include "CoreMinimal.h"
#include "Base/EclipseBaseLogic.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseBaseSubsystem.generated.h"

class IConsoleObject;
class UDataTable;
class UEclipseCampaignSetupAsset;
class UWorld;

/**
 * Hollow Point base wrapper (SPEC-P2-03 step 3). Owns no state — facilities
 * live in FEclipseCampaignState.BaseState (GDD 12.2 rule 4). This subsystem
 * resolves DA_BaseLayout / DT_Facilities / DA_BaseTuning, validates orders
 * through the pure core, and commits them as campaign transactions
 * (GDD 14.3.3: UI issues build/staff orders through this API, never the bus).
 * The Event.Base.* facts are emitted by the CampaignState commit itself.
 *
 * Same shape as the Economy/Prep wrappers: data in, one transaction out —
 * a new GameInstance subsystem rather than more surface on the campaign
 * ledger, so the ledger stays domain-neutral.
 *
 * Step 4-5 adds the walkable-vault presentation (EclipseVaultBuilder) as a PURE
 * bus consumer: the standing vault re-renders on the existing Event.Base.*
 * facts — no new tags, no polling, no tick (GDD 12.4). Orders still flow the
 * other way through the API above, never through the bus (GDD 14.3.3).
 */
UCLASS()
class ECLIPSE_API UEclipseBaseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Validated build/upgrade order at the first layout slot that accepts the
	 * facility (build order is free, placement is authored — locked decision 2;
	 * an operational same-facility slot makes this an upgrade order). Spend +
	 * StartConstruction commit atomically; the ledger is the hard funds gate.
	 */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Base")
	bool TryStartConstruction(FName FacilityId, FString& OutError);

	/** Same order pinned to one slot (survey-post flow: the player stands at the slot). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Base")
	bool TryStartConstructionAtSlot(FName SlotId, FName FacilityId, FString& OutError);

	/** Rush the slot's construction: 60 C x remaining days (DA_BaseTuning), completes in that same commit (SPEC-P2-03 clock rules). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Base")
	bool TryRushConstruction(FName SlotId, FString& OutError);

	/** Assign an Available soldier to the slot (positional role: building site = crew, operational = analyst). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Base")
	bool TryAssignStaff(FName SlotId, const FGuid& SoldierId, FString& OutError);

	/** Release a soldier from the slot (the muster board reads base state — this is how staff become deployable again). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Base")
	bool TryUnassignStaff(FName SlotId, const FGuid& SoldierId, FString& OutError);

	/**
	 * Render the Hollow Point vault into the game instance's current world and
	 * pin it to today's campaign state (SPEC-P2-03 step 4-5). Idempotent: an
	 * existing vault is torn down and rebuilt, so this doubles as the manual
	 * re-render (console: Eclipse.Base.Vault). A missing DA_BaseLayout is a loud
	 * warning + an empty vault, never a crash (GDD 14.3.5).
	 *
	 * This is the level/entry seam the P1-08 menu-hub retirement hooks; from here
	 * on the vault keeps itself current off the Event.Base.* facts alone.
	 */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Base")
	void EnsureVaultPresent();

	/** DA_BaseTuning as plain params (asset values when linked, spec defaults otherwise — GDD 14.3.5). */
	EclipseBaseLogic::FEclipseBaseTuningParams ResolveTuningParams() const;

	/** Today's facility output for the economy day tick (SPEC-P2-03: one deterministic tick — the economy folds this in). */
	EclipseBaseLogic::FEclipseFacilityYieldParams ComputeTodaysFacilityYields() const;

private:
	const UEclipseCampaignSetupAsset* ResolveSetup() const;
	const UEclipseBaseLayoutAsset* ResolveLayout() const;
	const UDataTable* ResolveFacilitiesTable() const;
	const FEclipseFacilityRow* FindFacilityRow(FName FacilityId) const;
	class UEclipseCampaignSubsystem* GetCampaign() const;

	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();
	void LogBaseReport() const;

	/**
	 * The Event.Base.* consumer (all four facts through one family subscription,
	 * same shape as the strategy map widget on Event.Story.BeatReached).
	 */
	void OnBaseFact(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/**
	 * Re-render the standing vault from campaign state. Without bForceRebuild
	 * this is a strict no-op unless a vault already stands in this world (a
	 * mission world must never get one uninvited) and the plan actually changed -
	 * the four facts of one commit therefore cost exactly one rebuild.
	 */
	void RefreshVault(bool bForceRebuild);

	/** The world the vault lives in: the game instance's current world (null outside a world). */
	UWorld* GetVaultWorld() const;

	/** One-shot warning flags (GDD 14.3.5: degrade loudly, once). */
	mutable bool bWarnedMissingLayout = false;
	mutable bool bWarnedMissingFacilities = false;
	mutable bool bWarnedNoAnalystResource = false;

	FEclipseEventSubscriptionHandle BaseEventsHandle;

	/** Plan hash of the vault standing right now (0 = none); the re-render coalescer's dirty check. */
	uint32 RenderedVaultPlanHash = 0;

	TArray<IConsoleObject*> ConsoleCommands;
};
