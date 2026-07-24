#pragma once

#include "CoreMinimal.h"
#include "EclipseSaveDataProvider.h"
#include "Strategy/EclipseCampaignTransaction.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseCampaignSubsystem.generated.h"

class IConsoleObject;
class UEclipseCampaignSetupAsset;

/**
 * Owner of the campaign state (GDD 12.2 rule 4). Read access is const-only;
 * the single write path is CommitTransaction, which validates atomically,
 * applies, and then broadcasts the resulting facts on the event bus —
 * state-changing events exist only downstream of a commit (GDD 14.3.3).
 */
UCLASS()
class ECLIPSE_API UEclipseCampaignSubsystem : public UGameInstanceSubsystem, public IEclipseSaveDataProvider
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Reset to the setup asset's starting conditions. Null/missing asset logs and yields an empty campaign (GDD 14.3.5). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Campaign")
	void StartNewCampaign(const UEclipseCampaignSetupAsset* Setup);

	const FEclipseCampaignState& GetState() const { return State; }

	/**
	 * Setup asset of the running campaign (region/economy definitions). Null
	 * before StartNewCampaign or after a bare load — consumers must degrade
	 * gracefully (GDD 14.3.5). PLACEHOLDER(GDD 12.3): a loaded save re-resolves
	 * its setup via project settings once campaign-slot metadata exists.
	 */
	const UEclipseCampaignSetupAsset* GetActiveSetup() const { return ActiveSetup; }

	/** The only writer. Emits one event per applied mutation after the whole transaction lands. */
	bool CommitTransaction(const FEclipseCampaignTransaction& Transaction, FString& OutError);

	/** Blueprint commit for debug UI / mission scripts; logs the error on failure. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Campaign", DisplayName = "Commit Campaign Transaction")
	bool CommitTransactionBP(const FEclipseCampaignTransaction& Transaction);

	/** Human-readable state dump (GDD 12.2 rule 4: debug export is a design requirement). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Campaign")
	FString ExportStateAsJson() const;

	// IEclipseSaveDataProvider
	virtual FName GetSaveBlockId() const override { return TEXT("Campaign"); }
	virtual void WriteSaveData(FArchive& Archive) override;
	virtual bool ReadSaveData(FArchive& Archive) override;

private:
	void EmitEventsForApplied(const TArray<FEclipseAppliedMutation>& Applied);

	/**
	 * Stamp DA_BaseTuning's construction-tick numbers onto AdvanceDay mutations
	 * so every day-advance path (hub, debrief, console) ticks with the same
	 * data (GDD 14.2); without a tuning asset the mutation defaults — the
	 * SPEC-P2-03 spec values — stand (GDD 14.3.5).
	 */
	void StampAdvanceDayTuning(FEclipseCampaignTransaction& Transaction) const;

	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

	FEclipseCampaignState State;

	UPROPERTY()
	TObjectPtr<const UEclipseCampaignSetupAsset> ActiveSetup;

	TArray<IConsoleObject*> ConsoleCommands;
};
