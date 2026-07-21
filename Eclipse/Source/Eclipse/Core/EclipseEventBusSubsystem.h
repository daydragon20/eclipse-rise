#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseEventBusSubsystem.generated.h"

class IConsoleObject;

/**
 * One delivery = the fact tag + its typed payload. Native handlers bind to this;
 * the tag parameter lets a hierarchical subscriber (e.g. Event.Mission) know which
 * concrete fact arrived.
 */
DECLARE_DELEGATE_TwoParams(FEclipseEventNativeDelegate, FGameplayTag /*EventTag*/, const FInstancedStruct& /*Payload*/);

/** Blueprint-facing handler for existing event families (SPEC-P1-01: BP may subscribe, not define new families). */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FEclipseEventDynamicDelegate, FGameplayTag, EventTag, FInstancedStruct, Payload);

/**
 * Opaque subscription handle. Value-copyable so subscribers can store it in a
 * member and unsubscribe in their teardown without touching bus internals.
 */
USTRUCT(BlueprintType)
struct FEclipseEventSubscriptionHandle
{
	GENERATED_BODY()

	bool IsValid() const { return Id != 0; }

	UPROPERTY()
	uint64 Id = 0;
};

/**
 * The central gameplay event bus (SPEC-P1-01; GDD 12.2 rule 2: "event bus, not
 * references"). Subsystems communicate exclusively through Broadcast/Subscribe so
 * that Part 4's interconnection arrows stay literal subscriptions instead of
 * hard dependencies — the reason a small team can keep 12 systems buildable.
 *
 * Delivery is synchronous and in subscription order (SPEC-P1-01 out-of-scope:
 * queued/async delivery). Hierarchical matching: subscribing to "Event.Mission"
 * receives "Event.Mission.Completed" and every other child fact.
 *
 * State-changing facts are emitted only by the CampaignState commit (GDD 14.3.3);
 * the bus itself enforces no policy — it transports and audits.
 */
UCLASS()
class ECLIPSE_API UEclipseEventBusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Publish a fact. ExpectedPayloadType-checked subscribers whose type doesn't
	 * match the payload are skipped with a logged error, never a crash (GDD 14.3.5
	 * spirit: bad input degrades gracefully).
	 */
	void Broadcast(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/**
	 * Subscribe a native handler. Pass ExpectedPayloadType to opt into payload
	 * type checking (recommended: catches producer/consumer drift at the seam
	 * where it happens instead of deep in consumer code).
	 */
	FEclipseEventSubscriptionHandle Subscribe(FGameplayTag EventTag, FEclipseEventNativeDelegate Handler, const UScriptStruct* ExpectedPayloadType = nullptr);

	/** Remove a subscription; safe to call during a broadcast of the same tag. */
	void Unsubscribe(FEclipseEventSubscriptionHandle& Handle);

	/** Blueprint publish of an existing event family (debug UI / mission scripts glue). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Events", DisplayName = "Broadcast Event")
	void BroadcastEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Blueprint subscribe; the returned handle unsubscribes via Unsubscribe Event. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Events", DisplayName = "Subscribe Event")
	FEclipseEventSubscriptionHandle SubscribeEvent(FGameplayTag EventTag, FEclipseEventDynamicDelegate Handler);

	UFUNCTION(BlueprintCallable, Category = "Eclipse|Events", DisplayName = "Unsubscribe Event")
	void UnsubscribeEvent(UPARAM(ref) FEclipseEventSubscriptionHandle& Handle);

	/** Number of live subscriptions (test/diagnostic surface). */
	int32 GetSubscriptionCount() const { return SubscriptionsById.Num(); }

	/** Dump the recent-event ring buffer to the log (console: Eclipse.Events.Dump). */
	void DumpRecentEvents() const;

private:
	/** A single subscriber entry; exactly one of Native/Dynamic is bound. */
	struct FSubscription
	{
		uint64 Id = 0;
		FGameplayTag Tag;
		FEclipseEventNativeDelegate Native;
		FEclipseEventDynamicDelegate Dynamic;
		TWeakObjectPtr<const UScriptStruct> ExpectedPayloadType;
	};

	/** One line of the diagnostic ring buffer (Eclipse.Events.Dump). */
	struct FEventRecord
	{
		FGameplayTag Tag;
		FName PayloadTypeName;
		double TimeSeconds = 0.0;
	};

	FEclipseEventSubscriptionHandle AddSubscription(FSubscription&& Subscription);
	void DeliverToTag(FGameplayTag MatchTag, FGameplayTag BroadcastTag, const FInstancedStruct& Payload);
	void RecordEvent(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Subscription ids per exact tag key; broadcast walks the tag's parent chain. */
	TMap<FGameplayTag, TArray<uint64>> SubscriptionIdsByTag;
	TMap<uint64, FSubscription> SubscriptionsById;
	uint64 NextSubscriptionId = 1;

	/** Diagnostic ring buffer; size from CVar Eclipse.Events.HistorySize. */
	TArray<FEventRecord> RecentEvents;
	int32 RecentEventsHead = 0;

	IConsoleObject* DumpCommand = nullptr;
};
