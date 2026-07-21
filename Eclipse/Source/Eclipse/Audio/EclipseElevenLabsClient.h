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
	/** API key from ELEVENLABS_API_KEY env var, else [ElevenLabs] ApiKey in Config/UserSecrets.ini; empty if unset. */
	ECLIPSE_API FString GetApiKeyFromEnvironment();

	/** bSuccess, the returned MP3 bytes (empty on failure), and an error string. Fires on the game thread. */
	DECLARE_DELEGATE_ThreeParams(FOnTtsComplete, bool /*bSuccess*/, const TArray<uint8>& /*AudioMp3*/, const FString& /*Error*/);

	/** POST /v1/text-to-speech/{VoiceId}. No-ops with an error if key/voice is missing. */
	ECLIPSE_API void RequestTextToSpeech(
		const FString& ApiKey,
		const FString& VoiceId,
		const FString& ModelId,
		const FString& Text,
		const FEclipseVoiceSettings& Settings,
		FOnTtsComplete OnComplete);
}
