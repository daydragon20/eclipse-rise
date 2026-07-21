// Unit tests for SPEC-P1-01 (GDD 14.4: unit layer, deterministic).
// The bus is constructed directly (no GameInstance) — its logic is deliberately
// self-contained so the unit layer stays headless-fast.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

namespace EclipseEventBusTest
{
	UEclipseEventBusSubsystem* MakeBus()
	{
		// The subsystem's ClassWithin is UGameInstance; a bare NewObject fires the
		// invalid-outer ensure. A throwaway GameInstance outer satisfies the contract
		// without paying for full subsystem-collection init (the bus needs none).
		UGameInstance* OuterGameInstance = NewObject<UGameInstance>(GEngine);
		return NewObject<UEclipseEventBusSubsystem>(OuterGameInstance);
	}

	FInstancedStruct MakeMissionPayload(FName MissionId, bool bSuccess)
	{
		FEclipseMissionEventPayload Payload;
		Payload.MissionId = MissionId;
		Payload.bSuccess = bSuccess;
		return FInstancedStruct::Make(Payload);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEventBusSubscribeBroadcastTest,
	"Eclipse.Core.EventBus.SubscribeBroadcastUnsubscribe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseEventBusSubscribeBroadcastTest::RunTest(const FString& Parameters)
{
	UEclipseEventBusSubsystem* Bus = EclipseEventBusTest::MakeBus();

	int32 ReceivedCount = 0;
	FName ReceivedMissionId = NAME_None;

	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		EclipseTags::Event_Mission_Completed,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			++ReceivedCount;
			if (const FEclipseMissionEventPayload* Mission = Payload.GetPtr<FEclipseMissionEventPayload>())
			{
				ReceivedMissionId = Mission->MissionId;
			}
		}));

	TestTrue(TEXT("Subscribe returns a valid handle"), Handle.IsValid());

	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseEventBusTest::MakeMissionPayload(TEXT("M_Test"), true));
	TestEqual(TEXT("Handler invoked exactly once"), ReceivedCount, 1);
	TestEqual(TEXT("Typed payload arrived intact"), ReceivedMissionId, FName(TEXT("M_Test")));

	// A different fact in another family must not reach this subscriber.
	Bus->Broadcast(EclipseTags::Event_Campaign_DayAdvanced, FInstancedStruct::Make(FEclipseCampaignEventPayload()));
	TestEqual(TEXT("Unrelated tag not delivered"), ReceivedCount, 1);

	Bus->Unsubscribe(Handle);
	TestFalse(TEXT("Handle invalidated by Unsubscribe"), Handle.IsValid());
	TestEqual(TEXT("No live subscriptions remain"), Bus->GetSubscriptionCount(), 0);

	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseEventBusTest::MakeMissionPayload(TEXT("M_Test2"), true));
	TestEqual(TEXT("No delivery after Unsubscribe"), ReceivedCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEventBusHierarchicalMatchingTest,
	"Eclipse.Core.EventBus.HierarchicalMatching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseEventBusHierarchicalMatchingTest::RunTest(const FString& Parameters)
{
	UEclipseEventBusSubsystem* Bus = EclipseEventBusTest::MakeBus();

	const FGameplayTag MissionFamily = EclipseTags::Event_Mission_Completed.GetTag().RequestDirectParent();
	TestTrue(TEXT("Family tag Event.Mission resolves"), MissionFamily.IsValid());

	int32 FamilyCount = 0;
	FGameplayTag LastConcreteTag;
	FEclipseEventSubscriptionHandle FamilyHandle = Bus->Subscribe(
		MissionFamily,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag Tag, const FInstancedStruct& Payload)
		{
			++FamilyCount;
			LastConcreteTag = Tag;
		}));

	int32 EconomyCount = 0;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Economy_ResourcesChanged,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct&)
		{
			++EconomyCount;
		}));

	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseEventBusTest::MakeMissionPayload(TEXT("M_A"), true));
	Bus->Broadcast(EclipseTags::Event_Mission_Failed, EclipseEventBusTest::MakeMissionPayload(TEXT("M_B"), false));

	TestEqual(TEXT("Family subscriber received both child facts"), FamilyCount, 2);
	TestEqual(TEXT("Handler sees the concrete child tag, not the family key"),
		LastConcreteTag, EclipseTags::Event_Mission_Failed.GetTag());
	TestEqual(TEXT("Sibling family untouched"), EconomyCount, 0);

	Bus->Unsubscribe(FamilyHandle);
	Bus->Unsubscribe(EconomyHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEventBusPayloadMismatchTest,
	"Eclipse.Core.EventBus.PayloadTypeMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseEventBusPayloadMismatchTest::RunTest(const FString& Parameters)
{
	UEclipseEventBusSubsystem* Bus = EclipseEventBusTest::MakeBus();

	int32 TypedCount = 0;
	FEclipseEventSubscriptionHandle Handle = Bus->Subscribe(
		EclipseTags::Event_Mission_Completed,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct&)
		{
			++TypedCount;
		}),
		FEclipseMissionEventPayload::StaticStruct());

	// Wrong family's payload: the bus must log an error, skip the handler, not crash (14.3.5 spirit).
	AddExpectedError(TEXT("payload type mismatch"), EAutomationExpectedErrorFlags::Contains, 1);
	Bus->Broadcast(EclipseTags::Event_Mission_Completed, FInstancedStruct::Make(FEclipseCampaignEventPayload()));
	TestEqual(TEXT("Mismatched payload skipped the typed handler"), TypedCount, 0);

	// Correct payload still delivers afterwards — the subscription survives the bad producer.
	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseEventBusTest::MakeMissionPayload(TEXT("M_OK"), true));
	TestEqual(TEXT("Typed handler received the valid payload"), TypedCount, 1);

	Bus->Unsubscribe(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEventBusReentrancyTest,
	"Eclipse.Core.EventBus.UnsubscribeDuringBroadcast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseEventBusReentrancyTest::RunTest(const FString& Parameters)
{
	UEclipseEventBusSubsystem* Bus = EclipseEventBusTest::MakeBus();

	// First handler unsubscribes itself mid-broadcast; the bus must neither crash
	// nor lose the second handler's delivery.
	FEclipseEventSubscriptionHandle SelfRemovingHandle;
	int32 SelfRemovingCount = 0;
	SelfRemovingHandle = Bus->Subscribe(
		EclipseTags::Event_Campaign_DayAdvanced,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct&)
		{
			++SelfRemovingCount;
			Bus->Unsubscribe(SelfRemovingHandle);
		}));

	int32 SecondCount = 0;
	FEclipseEventSubscriptionHandle SecondHandle = Bus->Subscribe(
		EclipseTags::Event_Campaign_DayAdvanced,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct&)
		{
			++SecondCount;
		}));

	const FInstancedStruct DayPayload = FInstancedStruct::Make(FEclipseCampaignEventPayload());
	Bus->Broadcast(EclipseTags::Event_Campaign_DayAdvanced, DayPayload);
	Bus->Broadcast(EclipseTags::Event_Campaign_DayAdvanced, DayPayload);

	TestEqual(TEXT("Self-removing handler ran exactly once"), SelfRemovingCount, 1);
	TestEqual(TEXT("Surviving handler received both broadcasts"), SecondCount, 2);

	Bus->Unsubscribe(SecondHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseEventBusDummySubsystemConversationTest,
	"Eclipse.Core.EventBus.DummySubsystemConversation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseEventBusDummySubsystemConversationTest::RunTest(const FString& Parameters)
{
	// SPEC-P1-01 Definition of Done: two dummy subsystems communicate exclusively
	// via the bus. "MissionStub" reports a completed mission; "EconomyStub" reacts
	// by broadcasting a resource change; "UIStub" observes the economy family.
	// Neither stub holds a reference to another — only to the bus.
	UEclipseEventBusSubsystem* Bus = EclipseEventBusTest::MakeBus();

	int32 UIObservedDelta = 0;
	FEclipseEventSubscriptionHandle EconomyHandle = Bus->Subscribe(
		EclipseTags::Event_Mission_Completed,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct& Payload)
		{
			const FEclipseMissionEventPayload* Mission = Payload.GetPtr<FEclipseMissionEventPayload>();
			if (Mission != nullptr && Mission->bSuccess)
			{
				FEclipseEconomyEventPayload Economy;
				Economy.Delta = 40;
				Economy.NewBalance = 140;
				Economy.Reason = TEXT("MissionReward");
				Bus->Broadcast(EclipseTags::Event_Economy_ResourcesChanged, FInstancedStruct::Make(Economy));
			}
		}),
		FEclipseMissionEventPayload::StaticStruct());

	const FGameplayTag EconomyFamily = EclipseTags::Event_Economy_ResourcesChanged.GetTag().RequestDirectParent();
	FEclipseEventSubscriptionHandle UIHandle = Bus->Subscribe(
		EconomyFamily,
		FEclipseEventNativeDelegate::CreateLambda([&](FGameplayTag, const FInstancedStruct& Payload)
		{
			if (const FEclipseEconomyEventPayload* Economy = Payload.GetPtr<FEclipseEconomyEventPayload>())
			{
				UIObservedDelta = Economy->Delta;
			}
		}));

	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseEventBusTest::MakeMissionPayload(TEXT("M_Loop"), true));

	TestEqual(TEXT("Chained bus-only conversation reached the observer"), UIObservedDelta, 40);

	Bus->Unsubscribe(EconomyHandle);
	Bus->Unsubscribe(UIHandle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
