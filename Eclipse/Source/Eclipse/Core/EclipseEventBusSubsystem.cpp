#include "Core/EclipseEventBusSubsystem.h"

#include "Eclipse.h"
#include "HAL/IConsoleManager.h"

namespace
{
	// Ring-buffer size is diagnostic tooling, not a gameplay tunable, so a CVar (not a
	// DataAsset) is the right home per GDD 14.2's intent: DataAssets hold *balance* data.
	TAutoConsoleVariable<int32> CVarEclipseEventsHistorySize(
		TEXT("Eclipse.Events.HistorySize"),
		32,
		TEXT("Number of recent event-bus broadcasts kept for Eclipse.Events.Dump."),
		ECVF_Default);
}

void UEclipseEventBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if !UE_BUILD_SHIPPING
	// Guarded: with several GameInstances alive (PIE clients, test fixtures) only
	// the first registers; the command binds to that instance's bus, which is
	// acceptable for a diagnostic.
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Events.Dump")) == nullptr)
	{
		DumpCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Events.Dump"),
			TEXT("Log the most recent event-bus broadcasts with payload type and age."),
			FConsoleCommandDelegate::CreateUObject(this, &UEclipseEventBusSubsystem::DumpRecentEvents),
			ECVF_Default);
	}
#endif
}

void UEclipseEventBusSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	if (DumpCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DumpCommand);
		DumpCommand = nullptr;
	}
#endif

	SubscriptionIdsByTag.Empty();
	SubscriptionsById.Empty();

	Super::Deinitialize();
}

void UEclipseEventBusSubsystem::Broadcast(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (!EventTag.IsValid())
	{
		UE_LOG(LogEclipse, Error, TEXT("EventBus: Broadcast called with an invalid tag — dropped."));
		return;
	}

	RecordEvent(EventTag, Payload);

	// Walk the tag's parent chain so a subscriber of Event.Mission receives
	// Event.Mission.Completed (SPEC-P1-01 hierarchical matching).
	FGameplayTag MatchTag = EventTag;
	while (MatchTag.IsValid())
	{
		DeliverToTag(MatchTag, EventTag, Payload);
		MatchTag = MatchTag.RequestDirectParent();
	}
}

FEclipseEventSubscriptionHandle UEclipseEventBusSubsystem::Subscribe(FGameplayTag EventTag, FEclipseEventNativeDelegate Handler, const UScriptStruct* ExpectedPayloadType)
{
	if (!EventTag.IsValid() || !Handler.IsBound())
	{
		UE_LOG(LogEclipse, Error, TEXT("EventBus: Subscribe rejected (invalid tag or unbound handler)."));
		return FEclipseEventSubscriptionHandle();
	}

	FSubscription Subscription;
	Subscription.Tag = EventTag;
	Subscription.Native = MoveTemp(Handler);
	Subscription.ExpectedPayloadType = ExpectedPayloadType;
	return AddSubscription(MoveTemp(Subscription));
}

void UEclipseEventBusSubsystem::Unsubscribe(FEclipseEventSubscriptionHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

	FSubscription Removed;
	if (SubscriptionsById.RemoveAndCopyValue(Handle.Id, Removed))
	{
		if (TArray<uint64>* Ids = SubscriptionIdsByTag.Find(Removed.Tag))
		{
			Ids->Remove(Handle.Id);
			if (Ids->IsEmpty())
			{
				SubscriptionIdsByTag.Remove(Removed.Tag);
			}
		}
	}

	Handle.Id = 0;
}

void UEclipseEventBusSubsystem::BroadcastEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	Broadcast(EventTag, Payload);
}

FEclipseEventSubscriptionHandle UEclipseEventBusSubsystem::SubscribeEvent(FGameplayTag EventTag, FEclipseEventDynamicDelegate Handler)
{
	if (!EventTag.IsValid() || !Handler.IsBound())
	{
		UE_LOG(LogEclipse, Error, TEXT("EventBus: SubscribeEvent rejected (invalid tag or unbound handler)."));
		return FEclipseEventSubscriptionHandle();
	}

	FSubscription Subscription;
	Subscription.Tag = EventTag;
	Subscription.Dynamic = Handler;
	return AddSubscription(MoveTemp(Subscription));
}

void UEclipseEventBusSubsystem::UnsubscribeEvent(FEclipseEventSubscriptionHandle& Handle)
{
	Unsubscribe(Handle);
}

FEclipseEventSubscriptionHandle UEclipseEventBusSubsystem::AddSubscription(FSubscription&& Subscription)
{
	Subscription.Id = NextSubscriptionId++;

	FEclipseEventSubscriptionHandle Handle;
	Handle.Id = Subscription.Id;

	SubscriptionIdsByTag.FindOrAdd(Subscription.Tag).Add(Subscription.Id);
	SubscriptionsById.Add(Subscription.Id, MoveTemp(Subscription));
	return Handle;
}

void UEclipseEventBusSubsystem::DeliverToTag(FGameplayTag MatchTag, FGameplayTag BroadcastTag, const FInstancedStruct& Payload)
{
	const TArray<uint64>* Ids = SubscriptionIdsByTag.Find(MatchTag);
	if (Ids == nullptr)
	{
		return;
	}

	// Copy: handlers may subscribe/unsubscribe re-entrantly; a stale id after an
	// in-flight unsubscribe simply misses the map lookup below and is skipped.
	const TArray<uint64> IdsCopy = *Ids;
	for (const uint64 Id : IdsCopy)
	{
		const FSubscription* Subscription = SubscriptionsById.Find(Id);
		if (Subscription == nullptr)
		{
			continue;
		}

		if (const UScriptStruct* Expected = Subscription->ExpectedPayloadType.Get())
		{
			const UScriptStruct* Actual = Payload.GetScriptStruct();
			if (Actual == nullptr || !Actual->IsChildOf(Expected))
			{
				UE_LOG(LogEclipse, Error,
					TEXT("EventBus: payload type mismatch on %s — subscriber expects %s, got %s. Handler skipped."),
					*BroadcastTag.ToString(),
					*Expected->GetName(),
					Actual != nullptr ? *Actual->GetName() : TEXT("<empty>"));
				continue;
			}
		}

		if (Subscription->Native.IsBound())
		{
			Subscription->Native.Execute(BroadcastTag, Payload);
		}
		else
		{
			// Dynamic delegates lose their binding when the target object dies;
			// ExecuteIfBound makes that a silent no-op, which is correct here —
			// the subscription is garbage awaiting its owner's Unsubscribe.
			Subscription->Dynamic.ExecuteIfBound(BroadcastTag, Payload);
		}
	}
}

void UEclipseEventBusSubsystem::RecordEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const int32 HistorySize = FMath::Max(1, CVarEclipseEventsHistorySize.GetValueOnGameThread());
	if (RecentEvents.Num() != HistorySize)
	{
		RecentEvents.SetNum(HistorySize);
		RecentEventsHead = 0;
	}

	FEventRecord& Record = RecentEvents[RecentEventsHead];
	Record.Tag = EventTag;
	Record.PayloadTypeName = Payload.GetScriptStruct() != nullptr ? Payload.GetScriptStruct()->GetFName() : NAME_None;
	Record.TimeSeconds = FPlatformTime::Seconds();
	RecentEventsHead = (RecentEventsHead + 1) % HistorySize;
}

void UEclipseEventBusSubsystem::DumpRecentEvents() const
{
	const double Now = FPlatformTime::Seconds();
	int32 Logged = 0;

	UE_LOG(LogEclipse, Display, TEXT("EventBus: recent broadcasts (newest last), %d live subscriptions:"), SubscriptionsById.Num());
	for (int32 Offset = 0; Offset < RecentEvents.Num(); ++Offset)
	{
		// Oldest-first: start one past the head (the next overwrite target).
		const int32 Index = (RecentEventsHead + Offset) % RecentEvents.Num();
		const FEventRecord& Record = RecentEvents[Index];
		if (!Record.Tag.IsValid())
		{
			continue;
		}

		UE_LOG(LogEclipse, Display, TEXT("  [%6.1fs ago] %s (%s)"),
			Now - Record.TimeSeconds,
			*Record.Tag.ToString(),
			*Record.PayloadTypeName.ToString());
		++Logged;
	}

	if (Logged == 0)
	{
		UE_LOG(LogEclipse, Display, TEXT("  (no broadcasts recorded)"));
	}
}
