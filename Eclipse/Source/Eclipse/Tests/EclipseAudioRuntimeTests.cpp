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
	// Drie: de missie-sting plus de twee order-antwoorden (barks, 26-07).
	TestEqual(TEXT("Drie live subscriptions: sting + ack + refused"), Bus->GetSubscriptionCount(), 3);

	// Re-bind must swap, not stack — a leaked second subscription would double
	// every sting.
	Audio->BindToBus(*Bus);
	TestEqual(TEXT("Re-bind does not leak a subscription"), Bus->GetSubscriptionCount(), 3);

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

// De rem op de squad-barks (owner-beslissing 26-07: 2 s per soldaat).
//
// Waarom juist de rem en niet het geluid: of er echt iets klinkt hangt af van
// gegenereerde audio op schijf, en dat is per machine anders — een test die
// daarop asserteert gaat rood op de verkeerde machine. De rem is wél
// deterministisch, en hij is het enige stuk waar een fout stil doorwerkt: te
// hard remmen maakt de squad stom, niet remmen levert vier stemmen over elkaar.
//
// De onderdrukkingsteller telt VOOR al het laadwerk, dus deze test meet de rem
// zelf en niet de audio-pijplijn.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadBarkCooldownTest,
	"Eclipse.Audio.Subsystem.SquadBarksHaveABrake",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseSquadBarkCooldownTest::RunTest(const FString& Parameters)
{
	UGameInstance* OuterGameInstance = NewObject<UGameInstance>(GEngine);
	UEclipseEventBusSubsystem* Bus = NewObject<UEclipseEventBusSubsystem>(OuterGameInstance);
	UEclipseAudioSubsystem* Audio = NewObject<UEclipseAudioSubsystem>(OuterGameInstance);
	Audio->BindToBus(*Bus);

	auto Ack = [](const FGuid& Soldier)
	{
		FEclipseSquadEventPayload Payload;
		Payload.SoldierId = Soldier;
		Payload.Order = FName(TEXT("EEclipseSquadOrder::MoveTo"));
		Payload.BarkLine = TEXT("Copy. Moving up.");
		return FInstancedStruct::Make(Payload);
	};

	// Vaste ids: het geheugen zit per soldaat, dus twee verschillende mannen
	// mogen elkaar niet remmen.
	const FGuid SoldierOne(0x11111111, 0x2222, 0x3333, 0x44444444);
	const FGuid SoldierTwo(0x55555555, 0x6666, 0x7777, 0x88888888);

	Bus->Broadcast(EclipseTags::Event_Squad_OrderAcknowledged, Ack(SoldierOne));
	const int32 AfterFirst = Audio->GetBarkSuppressedCount();
	TestEqual(TEXT("bark: het eerste antwoord van een soldaat wordt niet geremd"), AfterFirst, 0);

	// Vier orders achter elkaar, exact het geval waar de rem voor is.
	Bus->Broadcast(EclipseTags::Event_Squad_OrderAcknowledged, Ack(SoldierOne));
	Bus->Broadcast(EclipseTags::Event_Squad_OrderAcknowledged, Ack(SoldierOne));
	Bus->Broadcast(EclipseTags::Event_Squad_OrderAcknowledged, Ack(SoldierOne));

	// Nul zou hier betekenen dat de rem niet bestaat; drie dat hij alles slikt na
	// de eerste. Alleen als de eerste door mocht kan dit getal 3 zijn.
	TestEqual(TEXT("bark: drie snelle vervolgorders van dezelfde soldaat worden geremd (2 s)"),
		Audio->GetBarkSuppressedCount(), 3);

	// De rem is PER soldaat: een tweede man die tegelijk antwoordt is juist het
	// geluid van een squad die meedoet, en mag niet weggeremd worden.
	Bus->Broadcast(EclipseTags::Event_Squad_OrderAcknowledged, Ack(SoldierTwo));
	TestEqual(TEXT("bark: een ANDERE soldaat wordt niet geremd door de eerste"),
		Audio->GetBarkSuppressedCount(), 3);

	// Losgekoppeld = stil, net als de sting.
	Audio->UnbindFromBus();
	const int32 Before = Audio->GetBarkSuppressedCount();
	Bus->Broadcast(EclipseTags::Event_Squad_OrderAcknowledged, Ack(SoldierOne));
	TestEqual(TEXT("bark: na loskoppelen komt er niets meer binnen"),
		Audio->GetBarkSuppressedCount(), Before);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
