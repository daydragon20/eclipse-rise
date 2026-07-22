#include "Audio/EclipseElevenLabsClient.h"

#include "Eclipse.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Serialization/JsonWriter.h"

namespace EclipseElevenLabs
{
	FString GetApiKeyFromEnvironment()
	{
		FString Key = FPlatformMisc::GetEnvironmentVariable(TEXT("ELEVENLABS_API_KEY"));
		if (!Key.IsEmpty())
		{
			return Key;
		}

		// Gitignored fallback for convenience: Config/UserSecrets.ini
		//   [ElevenLabs]
		//   ApiKey=sk_...
		const FString SecretsPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("UserSecrets.ini"));
		if (FPaths::FileExists(SecretsPath) && GConfig != nullptr)
		{
			GConfig->GetString(TEXT("ElevenLabs"), TEXT("ApiKey"), Key, SecretsPath);
		}
		return Key;
	}

	static FString BuildRequestBody(const FString& ModelId, const FString& Text, const FEclipseVoiceSettings& Settings)
	{
		FString Body;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("text"), Text);
		Writer->WriteValue(TEXT("model_id"), ModelId);
		Writer->WriteObjectStart(TEXT("voice_settings"));
		Writer->WriteValue(TEXT("stability"), Settings.Stability);
		Writer->WriteValue(TEXT("similarity_boost"), Settings.SimilarityBoost);
		Writer->WriteValue(TEXT("style"), Settings.Style);
		Writer->WriteValue(TEXT("use_speaker_boost"), Settings.bUseSpeakerBoost);
		Writer->WriteObjectEnd();
		Writer->WriteObjectEnd();
		Writer->Close();
		return Body;
	}

	void RequestTextToSpeech(const FString& ApiKey, const FString& VoiceId, const FString& ModelId, const FString& Text, const FEclipseVoiceSettings& Settings, FOnTtsComplete OnComplete)
	{
		if (ApiKey.IsEmpty() || VoiceId.IsEmpty() || Text.IsEmpty())
		{
			OnComplete.ExecuteIfBound(false, TArray<uint8>(), TEXT("Missing API key, voice id, or text"));
			return;
		}

		// output_format=pcm_<rate>: raw PCM16 instead of mp3, so callers only ever
		// need the EclipseWavUtil wrap — no audio decoder in the pipeline (16.12).
		const FString Url = FString::Printf(TEXT("https://api.elevenlabs.io/v1/text-to-speech/%s?output_format=pcm_%d"), *VoiceId, TtsPcmSampleRate);
		const FString Body = BuildRequestBody(ModelId.IsEmpty() ? TEXT("eleven_multilingual_v2") : ModelId, Text, Settings);

		const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(Url);
		Request->SetVerb(TEXT("POST"));
		Request->SetHeader(TEXT("xi-api-key"), ApiKey);
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetHeader(TEXT("Accept"), TEXT("audio/pcm"));
		Request->SetContentAsString(Body);
		Request->OnProcessRequestComplete().BindLambda(
			[OnComplete](FHttpRequestPtr /*Req*/, FHttpResponsePtr Response, bool bConnectedOk)
			{
				// Success payload is raw PCM16 (see header constants), not a container format.
				if (!bConnectedOk || !Response.IsValid())
				{
					OnComplete.ExecuteIfBound(false, TArray<uint8>(), TEXT("Request failed or no response"));
					return;
				}
				const int32 Code = Response->GetResponseCode();
				if (Code < 200 || Code >= 300)
				{
					OnComplete.ExecuteIfBound(false, TArray<uint8>(),
						FString::Printf(TEXT("HTTP %d: %s"), Code, *Response->GetContentAsString()));
					return;
				}
				OnComplete.ExecuteIfBound(true, Response->GetContent(), FString());
			});

		if (!Request->ProcessRequest())
		{
			OnComplete.ExecuteIfBound(false, TArray<uint8>(), TEXT("Failed to start HTTP request"));
		}
	}
}
