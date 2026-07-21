#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Economy/EclipseEconomyLogic.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseEconomySubsystem.generated.h"

class IConsoleObject;

/**
 * Economy subsystem wrapper (SPEC-P1-03). Owns no balances — the wallet lives
 * in FEclipseCampaignState (single source of truth, GDD 12.2 rule 4). This
 * subsystem: reacts to Event.Campaign.DayAdvanced with a tick transaction,
 * exposes the workshop purchase path, and keeps the auditable ledger log
 * (GDD 7.6 transparency applied to money) by listening to Event.Economy.*.
 */
UCLASS()
class ECLIPSE_API UEclipseEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Buy one production item (spend + queue, atomic). The Workshop tab's only verb (SPEC-P1-08). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Economy")
	bool TryQueueProduction(FName ItemId, FString& OutError);

	/** Resolved tick params from the active campaign's data assets; empty params if data is missing (logged, graceful — GDD 14.3.5). */
	FEclipseEconomyTickParams ResolveTickParams() const;

	/** Ledger lines, oldest first (test/diagnostic surface; console: Eclipse.Economy.Report). */
	const TArray<FString>& GetLedgerLog() const { return LedgerLog; }

private:
	void OnDayAdvanced(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnEconomyEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void AppendLedgerLine(const FString& Line);

	FEclipseEventSubscriptionHandle DayAdvancedHandle;
	FEclipseEventSubscriptionHandle EconomyEventsHandle;

	/** Bounded audit trail; size is diagnostic tooling (CVar), not gameplay data. */
	TArray<FString> LedgerLog;

	IConsoleObject* ReportCommand = nullptr;
};
