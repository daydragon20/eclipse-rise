#include "Audio/EclipseDialogueVoiceSubsystem.h"

#include "Audio/EclipseCharacterVoiceData.h"
#include "Audio/EclipseElevenLabsClient.h"
#include "Audio/EclipseWavUtil.h"
#include "Eclipse.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"

void UEclipseDialogueVoiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Cache lives in Content (not Saved) since the WAV pipeline: 16.12 wants the
	// generated audio + manifest committed so no machine re-pays for a line. A
	// pre-move Saved/Audio cache is simply abandoned; its lines regenerate once
	// into the shared location and are then covered forever.
	Cache.Load();
	UE_LOG(LogEclipse, Display, TEXT("DialogueVoice: %d cached lines at %s"), Cache.Num(), *Cache.GetDirectory());
}

FString UEclipseDialogueVoiceSubsystem::MakeCacheKey(const FString& VoiceId, const FString& ModelId, EEclipseVoiceEmotion Emotion, const FString& Text)
{
	const FString Combined = FString::Printf(TEXT("%s|%s|%d|%s"), *VoiceId, *ModelId, static_cast<int32>(Emotion), *Text);
	return FMD5::HashAnsiString(*Combined);
}

FString UEclipseDialogueVoiceSubsystem::CachePathForKey(const FString& Key) const
{
	return Cache.PathForKey(Key);
}

bool UEclipseDialogueVoiceSubsystem::IsLineCached(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text) const
{
	if (Voice == nullptr)
	{
		return false;
	}
	return Cache.Contains(MakeCacheKey(Voice->ElevenLabsVoiceId, Voice->ModelId, Emotion, Text));
}

void UEclipseDialogueVoiceSubsystem::GenerateLine(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text, TFunction<void(bool, const FString&)> OnDone)
{
	if (Voice == nullptr || Text.IsEmpty())
	{
		if (OnDone) OnDone(false, TEXT("No voice asset or empty text"));
		return;
	}

	const FString Key = MakeCacheKey(Voice->ElevenLabsVoiceId, Voice->ModelId, Emotion, Text);

	// Never generate the same line twice (GDD 16.15 rule 7): cache hit is free.
	// Legacy .mp3 manifest entries count as hits while their file exists; once
	// the file is gone this very miss path regenerates the line as .wav and the
	// manifest entry upgrades with it (pre-WAV caches stay valid, never re-paid).
	if (Cache.Contains(Key))
	{
		if (OnDone) OnDone(true, Cache.PathForKey(Key));
		return;
	}
	if (InFlight.Contains(Key))
	{
		if (OnDone) OnDone(true, TEXT("Already in flight this session"));
		return;
	}

	const FString ApiKey = EclipseElevenLabs::GetApiKeyFromEnvironment();
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogEclipse, Warning, TEXT("DialogueVoice: no ELEVENLABS_API_KEY set — cannot generate (GDD 16.12)."));
		if (OnDone) OnDone(false, TEXT("ELEVENLABS_API_KEY not set"));
		return;
	}

	InFlight.Add(Key);
	// Regenerations always write the canonical <key>.wav name, never a stale
	// legacy manifest name — the content being written IS wav.
	const FString FileName = FEclipseVoiceCache::MakeWavFileName(Key);
	const FString OutPath = FPaths::Combine(Cache.GetDirectory(), FileName);
	const FEclipseVoiceSettings Settings = Voice->ResolveSettings(Emotion);

	// TWeakObjectPtr: the request outlives no strong ref here; the subsystem owns
	// the manifest and lives for the game instance, so a weak self is correct.
	TWeakObjectPtr<UEclipseDialogueVoiceSubsystem> WeakThis(this);

	EclipseElevenLabs::RequestTextToSpeech(ApiKey, Voice->ElevenLabsVoiceId, Voice->ModelId, Text, Settings,
		EclipseElevenLabs::FOnTtsComplete::CreateLambda(
			[WeakThis, Key, FileName, OutPath, OnDone](bool bSuccess, const TArray<uint8>& AudioPcm, const FString& Error)
			{
				UEclipseDialogueVoiceSubsystem* Self = WeakThis.Get();
				if (Self != nullptr)
				{
					Self->InFlight.Remove(Key);
				}
				if (!bSuccess || AudioPcm.Num() == 0)
				{
					UE_LOG(LogEclipse, Warning, TEXT("DialogueVoice: generation failed: %s"), *Error);
					if (OnDone) OnDone(false, Error);
					return;
				}
				// The client delivers raw PCM16 (16.12); wrap it in a RIFF header so
				// the cache file is a plain WAV — importable as USoundWave and
				// runtime-parseable with no decoder dependency.
				const TArray<uint8> WavBytes = EclipseWavUtil::WrapPcm16ToWav(AudioPcm, EclipseElevenLabs::TtsPcmSampleRate, EclipseElevenLabs::TtsPcmNumChannels);
				if (WavBytes.IsEmpty() || !FFileHelper::SaveArrayToFile(WavBytes, *OutPath))
				{
					if (OnDone) OnDone(false, TEXT("Failed to write audio file"));
					return;
				}
				if (Self != nullptr)
				{
					Self->Cache.Add(Key, FileName);
				}
				UE_LOG(LogEclipse, Display, TEXT("DialogueVoice: generated %s (%d PCM bytes)"), *FileName, AudioPcm.Num());
				if (OnDone) OnDone(true, OutPath);
			}));
}

int32 UEclipseDialogueVoiceSubsystem::GenerateLinesForVoice(const UEclipseCharacterVoiceData* Voice, const TArray<FEclipseDialogueLine>& Lines)
{
	if (Voice == nullptr)
	{
		return 0;
	}
	int32 Queued = 0;
	for (const FEclipseDialogueLine& Line : Lines)
	{
		if (IsLineCached(Voice, Line.Emotion, Line.Text))
		{
			continue;
		}
		GenerateLine(Voice, Line.Emotion, Line.Text, TFunction<void(bool, const FString&)>());
		++Queued;
	}
	UE_LOG(LogEclipse, Display, TEXT("DialogueVoice: queued %d/%d lines for '%s' (rest already cached)."),
		Queued, Lines.Num(), *Voice->DisplayName);
	return Queued;
}

bool UEclipseDialogueVoiceSubsystem::PlayLine(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text, UObject* WorldContextObject, float VolumeMultiplier)
{
	if (Voice == nullptr || Text.IsEmpty() || WorldContextObject == nullptr)
	{
		// Missing content/context is a data gap, not a crash (14.3.5).
		UE_LOG(LogEclipse, Warning, TEXT("DialogueVoice: PlayLine skipped — voice asset, text, or world context missing."));
		return false;
	}

	// (1) Imported asset assigned by the bulk tool: the shipping path — cooked and
	// compressed; packaged builds have no loose cache folder at all.
	for (const FEclipseDialogueLine& Line : Voice->Lines)
	{
		if (Line.Emotion == Emotion && Line.Text.Equals(Text) && !Line.GeneratedAudio.IsNull())
		{
			// Sync load — why: VO clips are small and this is a deliberate play
			// request; async streaming would add latency + plumbing for no gain.
			if (USoundWave* Asset = Line.GeneratedAudio.LoadSynchronous())
			{
				UGameplayStatics::PlaySound2D(WorldContextObject, Asset, VolumeMultiplier);
				return true;
			}
		}
	}

	const FString Key = MakeCacheKey(Voice->ElevenLabsVoiceId, Voice->ModelId, Emotion, Text);

	// (2) Loose cached wav (dev path: generated but not yet imported).
	if (Cache.Contains(Key))
	{
		if (TryPlayCachedWav(Cache.PathForKey(Key), WorldContextObject, VolumeMultiplier))
		{
			return true;
		}
		// A hit that fails to parse is a legacy .mp3 (or corrupt) file. Do NOT fall
		// through to generation: a cache hit must never re-spend credits (16.15
		// rule 7). Importing via the commandlet or deleting the file is the cure.
		UE_LOG(LogEclipse, Warning, TEXT("DialogueVoice: cached line %s is not playable PCM WAV (legacy mp3?) — run the EclipseGenerateVoices commandlet."), *Key);
		return false;
	}

	// (3) Miss: generate now, play on arrival (dev convenience; the line is cached
	// forever afterwards). Weak captures — the world or subsystem may be torn down
	// before the HTTP round-trip completes.
	TWeakObjectPtr<UEclipseDialogueVoiceSubsystem> WeakThis(this);
	TWeakObjectPtr<UObject> WeakContext(WorldContextObject);
	GenerateLine(Voice, Emotion, Text,
		[WeakThis, WeakContext, VolumeMultiplier](bool bSuccess, const FString& PathOrMessage)
		{
			UEclipseDialogueVoiceSubsystem* Self = WeakThis.Get();
			UObject* Context = WeakContext.Get();
			if (bSuccess && Self != nullptr && Context != nullptr && FPaths::FileExists(PathOrMessage))
			{
				Self->TryPlayCachedWav(PathOrMessage, Context, VolumeMultiplier);
			}
		});
	return true;
}

bool UEclipseDialogueVoiceSubsystem::TryPlayCachedWav(const FString& WavPath, UObject* WorldContextObject, float VolumeMultiplier)
{
	TArray<uint8> WavBytes;
	if (!FFileHelper::LoadFileToArray(WavBytes, *WavPath))
	{
		return false;
	}

	FEclipseWavInfo Info;
	if (!EclipseWavUtil::ParseWav(WavBytes, Info))
	{
		return false;
	}
	const TArrayView<const uint8> Pcm = EclipseWavUtil::GetPcmView(WavBytes, Info);
	if (Pcm.IsEmpty())
	{
		return false;
	}

	// Procedural wave instead of importing at runtime — why: USoundWave import is
	// an editor/cook operation; queueing the PCM straight to the mixer plays the
	// cache in a packaged-dev or PIE context with zero asset machinery.
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(GetTransientPackage());
	Wave->SetSampleRate(static_cast<uint32>(Info.SampleRate));
	Wave->NumChannels = Info.NumChannels;
	Wave->SoundGroup = SOUNDGROUP_Voice;
	Wave->bLooping = false;
	const int32 BytesPerFrame = Info.NumChannels * (EclipseWavUtil::PcmBitsPerSample / 8);
	// Exact duration — why: a procedural wave with no duration idles as an
	// endless stream; the one-shot must end when the PCM runs out.
	Wave->Duration = BytesPerFrame > 0 ? static_cast<float>(Pcm.Num()) / static_cast<float>(BytesPerFrame * Info.SampleRate) : 0.0f;
	Wave->QueueAudio(Pcm.GetData(), Pcm.Num());

	UGameplayStatics::PlaySound2D(WorldContextObject, Wave, VolumeMultiplier);
	return true;
}
