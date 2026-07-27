// Headless tests for the runtime audio consumer (GDD 16.12/16.7; 14.4 unit
// layer): the subsystem's bus contract — bind, consume, unbind — proven without
// an audio device. Playback itself degrades in world-less contexts by design
// (14.3.5), which is exactly what makes the consumer testable headless: the
// sting-request counter observes delivery, never the speaker.

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/EclipseAudioSubsystem.h"
#include "Audio/EclipseCharacterVoiceData.h"
#include "Audio/EclipseDialogueVoiceSubsystem.h"
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
	// Vijf: sting, de twee order-antwoorden, het schot en de treffer (26-07).
	TestEqual(TEXT("Negen live subscriptions: sting + ack + refused + ZELF-ACTIE + schot + treffer + MISSER + herladen + wapenwissel"), Bus->GetSubscriptionCount(), 9);

	// Re-bind must swap, not stack — a leaked second subscription would double
	// every sting.
	Audio->BindToBus(*Bus);
	TestEqual(TEXT("Re-bind does not leak a subscription"), Bus->GetSubscriptionCount(), 9);

	// Completed is consumed; the world-less context skips playback but still
	// counts the request (the 14.3.5 degradation path this test rides on).
	Bus->Broadcast(EclipseTags::Event_Mission_Completed, EclipseAudioRuntimeTest::MakeMissionPayload(TEXT("M_Audio"), true));
	TestEqual(TEXT("Completed fact consumed as a sting request"), Audio->GetStingRequestCount(), 1);

	// Sibling mission facts are not sting triggers (16.12: the completion sting
	// answers Completed only; a failure sting is future, separately-cued work).
	Bus->Broadcast(EclipseTags::Event_Mission_Failed, EclipseAudioRuntimeTest::MakeMissionPayload(TEXT("M_Audio"), false));
	Bus->Broadcast(EclipseTags::Event_Mission_Started, EclipseAudioRuntimeTest::MakeMissionPayload(TEXT("M_Audio"), false));
	TestEqual(TEXT("Failed/Started do not request the sting"), Audio->GetStingRequestCount(), 1);

	// Het schot-event komt aan als GELUIDSVERZOEK, ook zonder audio-apparaat: de
	// teller telt de verzoeken en niet de speaker, net als de sting hierboven.
	// Zonder deze regel zou "het wapen klinkt nu" een bewering zijn.
	{
		FEclipseCombatEventPayload Shot;
		Shot.Origin = FVector(100.0f, 0.0f, 0.0f);
		Shot.AlertRadiusCm = 5000.0f;
		Shot.bPlayerSide = true;
		Bus->Broadcast(EclipseTags::Event_Combat_ShotFired, FInstancedStruct::Make(Shot));
		TestEqual(TEXT("Een schot vraagt om geluid"), Audio->GetShotSoundCount(), 1);

		// En een TREFFER is een eigen feit met een eigen geluid. Zonder dit
		// onderscheid zou een misser even luid klinken als een raak schot, en dan
		// weet je nog steeds niet of je raakt.
		FEclipseCombatEventPayload Landed;
		Landed.Origin = FVector(120.0f, 0.0f, 0.0f);
		Landed.bHeadshot = true;
		Landed.Damage = 55.0f;
		Bus->Broadcast(EclipseTags::Event_Combat_HitLanded, FInstancedStruct::Make(Landed));
		TestEqual(TEXT("Een treffer vraagt om een eigen geluid"), Audio->GetHitSoundCount(), 1);
		TestEqual(TEXT("...en telt niet mee als schot"), Audio->GetShotSoundCount(), 1);
	}

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

// Kan de squad ECHT praten, of heb ik alleen een koppeling gebouwd?
//
// De rem-test hierboven bewijst de knijper en met opzet niet het geluid. Daarmee
// bleef de belangrijkste claim onbewezen: ik meldde de owner "de stemmen zijn
// aangesloten" terwijl geen enkele test aanraakte of er een clip bestaat voor de
// zinnen die de handler opvraagt. Dat is exact het patroon van deze hele sessie —
// een correct gezet doel is geen bewijs van effect.
//
// De handler speelt ALLEEN wat al gegenereerd is (laten genereren kost een
// betaalde API-aanroep, en dat is geld van de owner). Dus als tekst óf emotie óf
// stem niet exact matcht met wat er in de cache staat, blijft het stil — zonder
// dat er iets kapot is. Deze test controleert die drie tegelijk, via dezelfde
// IsLineCached die de handler gebruikt.
//
// Asserteerbaar en niet alleen rapporteerbaar omdat de 17 clips en het manifest
// in de repo staan: op elke kloon van dit project geldt dezelfde uitkomst.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSquadBarksHaveAudioTest,
	"Eclipse.Audio.Subsystem.EveryWiredBarkHasAClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseSquadBarksHaveAudioTest::RunTest(const FString& Parameters)
{
	UGameInstance* OuterGameInstance = NewObject<UGameInstance>(GEngine);
	UEclipseDialogueVoiceSubsystem* Voices = NewObject<UEclipseDialogueVoiceSubsystem>(OuterGameInstance);
	Voices->Initialize(*(new FSubsystemCollection<UGameInstanceSubsystem>()));

	const UEclipseCharacterVoiceData* VoiceA = LoadObject<UEclipseCharacterVoiceData>(
		nullptr, TEXT("/Game/Audio/DA_Voice_SquadA.DA_Voice_SquadA"));
	const UEclipseCharacterVoiceData* VoiceB = LoadObject<UEclipseCharacterVoiceData>(
		nullptr, TEXT("/Game/Audio/DA_Voice_SquadB.DA_Voice_SquadB"));
	if (!TestNotNull(TEXT("stem: DA_Voice_SquadA bestaat"), VoiceA)
		|| !TestNotNull(TEXT("stem: DA_Voice_SquadB bestaat"), VoiceB))
	{
		return false;
	}

	// Exact de zes regels die EclipseAudioSubsystem::OnOrderAnswered opvraagt.
	// Wijkt deze lijst af van die handler, dan meet deze test de verkeerde dingen —
	// daarom staat de handler-naam erbij en niet alleen de zin.
	struct FWired { const TCHAR* When; EEclipseVoiceEmotion Emotion; const TCHAR* Text; };
	const FWired Wired[] = {
		{ TEXT("ack MoveTo"),        EEclipseVoiceEmotion::Confident, TEXT("Copy. Moving up.") },
		{ TEXT("ack Hold"),          EEclipseVoiceEmotion::Calm,      TEXT("Holding position.") },
		{ TEXT("ack Regroup"),       EEclipseVoiceEmotion::Calm,      TEXT("Falling back to you.") },
		{ TEXT("weiger NoRoute"),    EEclipseVoiceEmotion::Urgent,    TEXT("Negative, I can't find a route.") },
		{ TEXT("weiger NoShot"),     EEclipseVoiceEmotion::Urgent,    TEXT("No shot from here.") },
		{ TEXT("weiger Downed"),     EEclipseVoiceEmotion::Sad,       TEXT("I'm hit. Someone patch me up.") },
		// De twee die 26-07 geschreven zijn omdat ze ontbraken (owner-opdracht).
		{ TEXT("ack FocusTarget"),   EEclipseVoiceEmotion::Confident, TEXT("Eyes on. Engaging.") },
		{ TEXT("weiger InvalidTarget"), EEclipseVoiceEmotion::Urgent, TEXT("That's not a target.") },
	};

	int32 Playable = 0;
	for (const FWired& Line : Wired)
	{
		const bool bA = Voices->IsLineCached(VoiceA, Line.Emotion, Line.Text);
		const bool bB = Voices->IsLineCached(VoiceB, Line.Emotion, Line.Text);
		Playable += (bA || bB) ? 1 : 0;
		TestTrue(FString::Printf(
				TEXT("bark-audio: '%s' klinkt echt — er is een clip voor \"%s\" (stem A=%d, B=%d). ")
				TEXT("Nul betekent stilte in het spel zonder dat er iets kapot is: tekst, emotie of stem wijkt af van de cache"),
				Line.When, Line.Text, bA ? 1 : 0, bB ? 1 : 0),
			bA || bB);
	}

	AddInfo(FString::Printf(TEXT("bark-audio: %d van de %d aangesloten zinnen hebben een clip"),
		Playable, UE_ARRAY_COUNT(Wired)));
	return true;
}


/**
 * VARIANTEN (owner-levering 26-07 avond: FreeWeaponSounds).
 *
 * De regel is niet "kies willekeurig" maar "kies nooit dezelfde als net". Dat
 * verschil hoor je: bij een gewone loting uit drie valt een op de drie schoten
 * identiek aan de vorige, en dan klinkt automatisch vuur alsnog als een loop —
 * precies wat drie varianten moesten oplossen.
 *
 * Duizend trekkingen zonder geluidskaart: dit is de hele bewijsbare kant.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseWeaponSoundVariantsNeverRepeatTest,
	"Eclipse.Audio.Subsystem.WeaponSoundVariantsNeverRepeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseWeaponSoundVariantsNeverRepeatTest::RunTest(const FString& Parameters)
{
	// Drie varianten, zoals het pack ze per wapenfamilie levert.
	constexpr int32 Count = 3;
	constexpr int32 Draws = 1000;

	int32 Last = UEclipseAudioSubsystem::PickVariantIndex(Count, INDEX_NONE);
	int32 Repeats = 0;
	int32 Seen[Count] = { 0, 0, 0 };
	for (int32 Draw = 0; Draw < Draws; ++Draw)
	{
		const int32 Next = UEclipseAudioSubsystem::PickVariantIndex(Count, Last);
		if (Next == Last)
		{
			++Repeats;
		}
		if (Next >= 0 && Next < Count)
		{
			++Seen[Next];
		}
		Last = Next;
	}

	AddInfo(FString::Printf(TEXT("GEMETEN  herhalingen in %d trekkingen              %d"), Draws, Repeats));
	AddInfo(FString::Printf(TEXT("GEMETEN  verdeling over de drie varianten          %d / %d / %d"),
		Seen[0], Seen[1], Seen[2]));

	TestEqual(TEXT("varianten: nooit twee dezelfde achter elkaar"), Repeats, 0);

	// Alle drie moeten voorkomen. Een variant die nooit klinkt is een dood asset,
	// en dat is precies de klasse die deze levering kwam repareren.
	for (int32 Index = 0; Index < Count; ++Index)
	{
		TestTrue(FString::Printf(TEXT("varianten: variant %d wordt gebruikt"), Index), Seen[Index] > 0);
	}

	// Eén variant: dan is er niets te kiezen en moet hij hem gewoon teruggeven,
	// niet in een lus blijven zoeken naar iets anders.
	TestEqual(TEXT("varianten: één optie geeft die optie"), UEclipseAudioSubsystem::PickVariantIndex(1, 0), 0);
	TestEqual(TEXT("varianten: geen opties geeft geen keuze"), UEclipseAudioSubsystem::PickVariantIndex(0, 0), int32(INDEX_NONE));

	return true;
}


/**
 * DE GELUIDEN OVERLEVEN EEN GARBAGE COLLECT (owner-diagnose 26-07, 21:01).
 *
 * De crash: EXCEPTION_ACCESS_VIOLATION in PlayFootstep. De oorzaak was niet de
 * logica maar het GEHEUGEN — de banken lagen in gewone TMaps met gewone structs,
 * dus de garbage collector zag de TObjectPtr-referenties niet, ruimde de cues op,
 * en de eerste voetstap daarna speelde vrijgegeven geheugen af.
 *
 * De bug zat er vanaf het bouwen van de voetstappen. Hij werd pas zichtbaar toen
 * er MINDER geluid geladen bleef, want toen viel de race de verkeerde kant op.
 * Precies daarom moet dit een test zijn en geen zorgvuldigheid: het is niet te
 * zien aan de code, alleen aan het moment.
 *
 * De test forceert een volledige GC-pass tussen laden en afspelen. Zonder
 * USTRUCT/UPROPERTY op de banken haalt hij het niet.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseSoundBanksSurviveGarbageCollection,
	"Eclipse.Audio.Subsystem.SoundBanksSurviveGarbageCollection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)

bool FEclipseSoundBanksSurviveGarbageCollection::RunTest(const FString& Parameters)
{
	// STERKE REFERENTIES op de subsystemen zelf. In het spel houdt de
	// subsystem-collectie ze vast; hier houdt niemand ze vast, dus een volledige
	// purge ruimde het AUDIOSUBSYSTEEM op en crashte de test op zijn eigen
	// opstelling. Dat is geen productfout maar wel precies de valkuil waar deze
	// test over gaat: wat niemand vasthoudt, verdwijnt.
	TStrongObjectPtr<UGameInstance> OuterGameInstance(NewObject<UGameInstance>(GEngine));
	TStrongObjectPtr<UEclipseAudioSubsystem> AudioKeep(NewObject<UEclipseAudioSubsystem>(OuterGameInstance.Get()));
	TStrongObjectPtr<UEclipseEventBusSubsystem> BusKeep(NewObject<UEclipseEventBusSubsystem>(OuterGameInstance.Get()));
	UEclipseAudioSubsystem* Audio = AudioKeep.Get();
	UEclipseEventBusSubsystem* Bus = BusKeep.Get();
	if (!TestNotNull(TEXT("gc: audiosubsysteem"), Audio) || !TestNotNull(TEXT("gc: bus"), Bus))
	{
		return false;
	}
	// Binden laadt de banken (zelfde pad als in het spel).
	Audio->BindToBus(*Bus);

	// HET GETAL DAT HET BEWIJST: hoeveel cues nog een GELDIG object zijn. Een
	// telling van array-lengtes zou dit niet vangen — een TArray houdt zijn lengte
	// ook als de objecten eronder zijn opgeruimd, en dan is de crash pas bij het
	// afspelen te zien.
	const int32 LiveBefore = Audio->CountLiveCues();
	const int32 RifleBefore = Audio->GetWeaponSoundVariantCount(TEXT("AssaultRifle"));
	const int32 MetalBefore = Audio->GetFootstepVariantCount(1);
	AddInfo(FString::Printf(TEXT("GEMETEN  varianten vóór de GC-pass                rifle %d, metaal %d"),
		RifleBefore, MetalBefore));

	if (!TestTrue(TEXT("gc: er is überhaupt iets geladen om kwijt te raken"), RifleBefore > 0 && MetalBefore > 0))
	{
		return false;
	}

	// DE PASS ZELF. Purge=true dwingt het opruimen af in plaats van het te plannen;
	// zonder dat zou de test kunnen slagen omdat er toevallig nog niets geruimd is.
	CollectGarbage(RF_NoFlags, /*bFullPurge=*/true);

	const int32 RifleAfter = Audio->GetWeaponSoundVariantCount(TEXT("AssaultRifle"));
	const int32 MetalAfter = Audio->GetFootstepVariantCount(1);
	AddInfo(FString::Printf(TEXT("GEMETEN  varianten ná de GC-pass                  rifle %d, metaal %d"),
		RifleAfter, MetalAfter));

	const int32 LiveAfter = Audio->CountLiveCues();
	AddInfo(FString::Printf(TEXT("GEMETEN  geldige cues vóór/ná de GC-pass          %d / %d"), LiveBefore, LiveAfter));

	TestEqual(TEXT("gc: de wapenbank houdt zijn aantal"), RifleAfter, RifleBefore);
	TestEqual(TEXT("gc: de voetstapbank houdt zijn aantal"), MetalAfter, MetalBefore);
	// DIT is de assertie die zonder USTRUCT/UPROPERTY omvalt.
	TestEqual(TEXT("gc: geen enkele cue is opgeruimd"), LiveAfter, LiveBefore);
	TestTrue(TEXT("gc: er waren überhaupt cues om te beschermen"), LiveBefore > 0);

	// En daadwerkelijk AFSPELEN na de pass — dat is waar het crashte. Zonder wereld
	// keert PlayFootstep netjes terug; het gaat hier om het aanraken van de cues.
	Audio->PlayFootstep(FVector::ZeroVector, 1, 0.45f);
	AddInfo(TEXT("GEMETEN  een voetstap ná de GC-pass is zonder crash afgespeeld"));

	Audio->UnbindFromBus();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
