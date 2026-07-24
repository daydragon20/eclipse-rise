// Headless tests for the runtime audio consumer (GDD 16.12/16.7; 14.4 unit
// layer): the subsystem's bus contract — bind, consume, unbind — proven without
// an audio device. Playback itself degrades in world-less contexts by design
// (14.3.5), which is exactly what makes the consumer testable headless: the
// sting-request counter observes delivery, never the speaker.

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/EclipseAudioSubsystem.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "StructUtils/InstancedStruct.h"

namespace EclipseAudioRuntimeTest
{
	FInstancedStruct MakeMissionPayload(FName MissionId, bool bSuccess)
	{
		FEclipseMissionEventPayload Payload;
		Payload.MissionId = MissionId;
		Payload.bSuccess = bSuccess;
		return FInstancedStruct::Make(Payload);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseAudioSubsystemBusContractTest,
	"Eclipse.Audio.Subsystem.BusContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseAudioSubsystemBusContractTest::RunTest(const FString& Parameters)
{
	// Same construction style as the bus tests: a throwaway GameInstance outer
	// satisfies the subsystems' outer contract without full collection init —
	// BindToBus is the seam that replaces Initialize here.
	UGameInstance* OuterGameInstance = NewObject<UGameInstance>(GEngine);
	UEclipseEventBusSubsystem* Bus = NewObject<UEclipseEventBusSubsystem>(OuterGameInstance);
	UEclipseAudioSubsystem* Audio = NewObject<UEclipseAudioSubsystem>(OuterGameInstance);

	TestFalse(TEXT("Unbound at construction"), Audio->IsBoundToBus());

	Audio->BindToBus(*Bus);
	TestTrue(TEXT("Bound after BindToBus"), Audio->IsBoundToBus());
	TestEqual(TEXT("Exactly one live subscription"), Bus->GetSubscriptionCount(), 1);

	// Re-bind must swap, not stack — a leaked second subscription would double
	// every sting.
	Audio->BindToBus(*Bus);
	TestEqual(TEXT("Re-bind does not leak a subscription"), Bus->GetSubscriptionCount(), 1);

	// Completed is consumed; the world-less context skips playback but still
	// counts the request (the 14.3.5 degradation path this test rides on).
	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseAudioRuntimeTest::MakeMissionPayload(TEXT("M_Audio"), true));
	TestEqual(TEXT("Completed fact consumed as a sting request"), Audio->GetStingRequestCount(), 1);

	// Sibling mission facts are not sting triggers (16.12: the completion sting
	// answers Completed only; a failure sting is future, separately-cued work).
	Bus->Broadcast(EclipseTags::Event_Mission_Failed, EclipseAudioRuntimeTest::MakeMissionPayload(TEXT("M_Audio"), false));
	Bus->Broadcast(EclipseTags::Event_Mission_Started, EclipseAudioRuntimeTest::MakeMissionPayload(TEXT("M_Audio"), false));
	TestEqual(TEXT("Failed/Started do not request the sting"), Audio->GetStingRequestCount(), 1);

	Audio->UnbindFromBus();
	TestFalse(TEXT("Unbound after UnbindFromBus"), Audio->IsBoundToBus());
	TestEqual(TEXT("No live subscriptions remain"), Bus->GetSubscriptionCount(), 0);

	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseAudioRuntimeTest::MakeMissionPayload(TEXT("M_After"), true));
	TestEqual(TEXT("No delivery after unbind"), Audio->GetStingRequestCount(), 1);

	// Double-unbind is a no-op, not a crash — Deinitialize may race a bus that
	// already tore down.
	Audio->UnbindFromBus();
	TestEqual(TEXT("Double unbind stays clean"), Bus->GetSubscriptionCount(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
