#pragma once

#include "CoreMinimal.h"
#include "Audio/EclipseDialogueTypes.h"

/**
 * Thin async client for the ElevenLabs Text-to-Speech REST API (GDD 16.12).
 * The API key is read from the environment (ELEVENLABS_API_KEY) or a gitignored
 * Config/UserSecrets.ini — never from source or a committed asset.
 */
namespace EclipseElevenLabs
{
	/**
	 * PCM output contract we request from ElevenLabs (query output_format=pcm_<rate>):
	 * raw 16-bit little-endian mono PCM. Why PCM instead of mp3: one WAV wrap away
	 * from both editor import (USoundWave) and runtime procedural playback, with no
	 * decoder dependency anywhere (16.12/16.14). Why 22050 mono: ample for VO and
	 * half the bytes of 44.1k. Named constants, not magic numbers (14.2) — this is
	 * a provider wire contract, not a gameplay tunable, so it is not a DataAsset.
	 */
	inline constexpr int32 TtsPcmSampleRate = 22050;
	inline constexpr int32 TtsPcmNumChannels = 1;

	/** API key from ELEVENLABS_API_KEY env var, else [ElevenLabs] ApiKey in Config/UserSecrets.ini; empty if unset. */
	ECLIPSE_API FString GetApiKeyFromEnvironment();

	/** bSuccess, the returned raw PCM16 bytes at TtsPcmSampleRate/TtsPcmNumChannels (empty on failure), and an error string. Fires on the game thread. */
	DECLARE_DELEGATE_ThreeParams(FOnTtsComplete, bool /*bSuccess*/, const TArray<uint8>& /*AudioPcm*/, const FString& /*Error*/);

	/** POST /v1/text-to-speech/{VoiceId}?output_format=pcm_<TtsPcmSampleRate>. No-ops with an error if key/voice is missing. */
	ECLIPSE_API void RequestTextToSpeech(
		const FString& ApiKey,
		const FString& VoiceId,
		const FString& ModelId,
		const FString& Text,
		const FEclipseVoiceSettings& Settings,
		FOnTtsComplete OnComplete);
}
