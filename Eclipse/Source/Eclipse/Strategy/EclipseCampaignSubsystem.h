#pragma once

#include "CoreMinimal.h"
#include "EclipseSaveDataProvider.h"
#include "Strategy/EclipseCampaignTransaction.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseCampaignSubsystem.generated.h"

class IConsoleObject;
class UEclipseCampaignSetupAsset;

namespace EclipseSaveReport
{
	/**
	 * De eerste twee regels van Eclipse.Save.Report, als PURE functie.
	 *
	 * Staat hier los omdat de belangrijkste regel van dat rapport degene is die
	 * een gezonde start nooit haalt: "setup ONTBREEKT". Zolang die tak alleen in
	 * een console-lambda leeft, is hij niet te falsifieren — en precies die tak
	 * moet op een avond werken waarop verder niets meer werkt.
	 *
	 * Slots op schijf horen er bewust NIET bij: dat is IO, en die vraag komt pas
	 * nadat deze twee regels beantwoord zijn.
	 */
	ECLIPSE_API void BuildStateLines(
		const UEclipseCampaignSetupAsset* Setup,
		const FEclipseCampaignState& State,
		TArray<FString>& OutLines);
}

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
	 * Stamp DA_BaseTuning's numbers onto the mutations that consume them
	 * (AdvanceDay's construction tick, AssignStaff's cap validation) so every
	 * caller — hub, debrief, console — runs the same data (GDD 14.2); without
	 * a tuning asset the mutation defaults — the SPEC-P2-03 spec values —
	 * stand (GDD 14.3.5). Loads the asset only when the transaction actually
	 * contains such a mutation (step-3 review finding 5): resource spends,
	 * roster changes and story beats must not pay a synchronous asset load.
	 */
	void StampBaseTuning(FEclipseCampaignTransaction& Transaction) const;

	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

	FEclipseCampaignState State;

	UPROPERTY()
	TObjectPtr<const UEclipseCampaignSetupAsset> ActiveSetup;

	TArray<IConsoleObject*> ConsoleCommands;
};
