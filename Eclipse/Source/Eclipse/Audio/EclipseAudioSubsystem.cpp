#include "Audio/EclipseAudioSubsystem.h"

#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	/**
	 * Sting mix level (GDD 16.12): sits clearly above the Layer-1 ambient bed
	 * (0.35 in the graybox builder) with headroom left for dialogue. Debug-tier
	 * named constant by choice — the real mix (with 16.7 ducking) belongs in the
	 * coming audio-layer DataAsset; a one-value DA for a single sting now would
	 * be ceremony without tuning value (14.2 governs balance data, and a
	 * placeholder mix level is not balance yet).
	 */
	constexpr float MissionCompleteStingVolume = 0.9f;

	const TCHAR* MissionCompleteStingPath = TEXT("/Game/Audio/Music/MUS_Sting_MissionComplete_Resistance_01.MUS_Sting_MissionComplete_Resistance_01");
}

void UEclipseAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The bus must outlive-and-precede every consumer (same discipline as the
	// gameplay subsystems: Base/Economy/Mission all InitializeDependency it).
	if (UEclipseEventBusSubsystem* Bus = Collection.InitializeDependency<UEclipseEventBusSubsystem>())
	{
		BindToBus(*Bus);
	}
}

void UEclipseAudioSubsystem::Deinitialize()
{
	UnbindFromBus();
	Super::Deinitialize();
}

void UEclipseAudioSubsystem::BindToBus(UEclipseEventBusSubsystem& Bus)
{
	UnbindFromBus();
	BoundBus = &Bus;
	// Concrete tag, not the Event.Mission family: today's only reaction is the
	// completion sting, and the typed subscription lets the bus type-check the
	// payload at the seam (producer/consumer drift fails loud, GDD 14.3.5).
	MissionCompletedHandle = Bus.Subscribe(
		EclipseTags::Event_Mission_Completed,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnMissionCompleted),
		FEclipseMissionEventPayload::StaticStruct());
}

void UEclipseAudioSubsystem::UnbindFromBus()
{
	if (UEclipseEventBusSubsystem* Bus = BoundBus.Get())
	{
		Bus->Unsubscribe(MissionCompletedHandle);
	}
	MissionCompletedHandle = FEclipseEventSubscriptionHandle();
	BoundBus = nullptr;
}

void UEclipseAudioSubsystem::OnMissionCompleted(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	++StingRequestCount;

	if (!bTriedLoadSting)
	{
		bTriedLoadSting = true;
		MissionCompleteSting = LoadObject<USoundBase>(nullptr, MissionCompleteStingPath);
		if (MissionCompleteSting == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Audio: mission-complete sting %s missing — debrief stays silent (GDD 14.3.5; run Tools/generate_audio_assets.py + import_generated_audio.py)."),
				MissionCompleteStingPath);
		}
	}
	if (MissionCompleteSting == nullptr)
	{
		return;
	}

	// 2D by design (16.12): the sting is non-diegetic debrief punctuation, not a
	// world sound. World-less contexts (unit tests, commandlets) consume the
	// fact but skip playback — degradation, never a crash (GDD 14.3.5).
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return;
	}
	UGameplayStatics::PlaySound2D(World, MissionCompleteSting, MissionCompleteStingVolume);
}
