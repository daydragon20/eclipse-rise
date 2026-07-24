#pragma once

#include "CoreMinimal.h"
#include "Base/EclipseBaseLogic.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseBaseSubsystem.generated.h"

class IConsoleObject;
class UDataTable;
class UEclipseCampaignSetupAsset;

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

	/** One-shot warning flags (GDD 14.3.5: degrade loudly, once). */
	mutable bool bWarnedMissingLayout = false;
	mutable bool bWarnedMissingFacilities = false;
	mutable bool bWarnedNoAnalystResource = false;

	TArray<IConsoleObject*> ConsoleCommands;
};
