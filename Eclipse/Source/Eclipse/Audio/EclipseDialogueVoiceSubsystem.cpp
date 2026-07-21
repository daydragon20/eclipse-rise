#include "Audio/EclipseDialogueVoiceSubsystem.h"

#include "Audio/EclipseCharacterVoiceData.h"
#include "Audio/EclipseElevenLabsClient.h"
#include "Dom/JsonObject.h"
#include "Eclipse.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UEclipseDialogueVoiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	IFileManager::Get().MakeDirectory(*GeneratedDir(), /*Tree*/ true);
	LoadManifest();
	UE_LOG(LogEclipse, Display, TEXT("DialogueVoice: %d cached lines at %s"), Manifest.Num(), *GeneratedDir());
}

FString UEclipseDialogueVoiceSubsystem::GeneratedDir() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Audio"), TEXT("Generated"));
}

FString UEclipseDialogueVoiceSubsystem::MakeCacheKey(const FString& VoiceId, const FString& ModelId, EEclipseVoiceEmotion Emotion, const FString& Text)
{
	const FString Combined = FString::Printf(TEXT("%s|%s|%d|%s"), *VoiceId, *ModelId, static_cast<int32>(Emotion), *Text);
	return FMD5::HashAnsiString(*Combined);
}

FString UEclipseDialogueVoiceSubsystem::CachePathForKey(const FString& Key) const
{
	return FPaths::Combine(GeneratedDir(), Key + TEXT(".mp3"));
}

bool UEclipseDialogueVoiceSubsystem::IsLineCached(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text) const
{
	if (Voice == nullptr)
	{
		return false;
	}
	const FString Key = MakeCacheKey(Voice->ElevenLabsVoiceId, Voice->ModelId, Emotion, Text);
	return Manifest.Contains(Key) && FPaths::FileExists(CachePathForKey(Key));
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
	if (Manifest.Contains(Key) && FPaths::FileExists(CachePathForKey(Key)))
	{
		if (OnDone) OnDone(true, CachePathForKey(Key));
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
	const FString OutPath = CachePathForKey(Key);
	const FEclipseVoiceSettings Settings = Voice->ResolveSettings(Emotion);

	// TWeakObjectPtr: the request outlives no strong ref here; the subsystem owns
	// the manifest and lives for the game instance, so a weak self is correct.
	TWeakObjectPtr<UEclipseDialogueVoiceSubsystem> WeakThis(this);

	EclipseElevenLabs::RequestTextToSpeech(ApiKey, Voice->ElevenLabsVoiceId, Voice->ModelId, Text, Settings,
		EclipseElevenLabs::FOnTtsComplete::CreateLambda(
			[WeakThis, Key, OutPath, OnDone](bool bSuccess, const TArray<uint8>& AudioMp3, const FString& Error)
			{
				UEclipseDialogueVoiceSubsystem* Self = WeakThis.Get();
				if (Self != nullptr)
				{
					Self->InFlight.Remove(Key);
				}
				if (!bSuccess || AudioMp3.Num() == 0)
				{
					UE_LOG(LogEclipse, Warning, TEXT("DialogueVoice: generation failed: %s"), *Error);
					if (OnDone) OnDone(false, Error);
					return;
				}
				if (!FFileHelper::SaveArrayToFile(AudioMp3, *OutPath))
				{
					if (OnDone) OnDone(false, TEXT("Failed to write audio file"));
					return;
				}
				if (Self != nullptr)
				{
					Self->Manifest.Add(Key, FPaths::GetCleanFilename(OutPath));
					Self->SaveManifest();
				}
				UE_LOG(LogEclipse, Display, TEXT("DialogueVoice: generated %s (%d bytes)"), *FPaths::GetCleanFilename(OutPath), AudioMp3.Num());
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

void UEclipseDialogueVoiceSubsystem::LoadManifest()
{
	Manifest.Reset();
	const FString ManifestPath = FPaths::Combine(GeneratedDir(), TEXT("VoiceCacheManifest.json"));
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *ManifestPath))
	{
		return;
	}
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
	if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
		{
			Manifest.Add(Pair.Key, Pair.Value->AsString());
		}
	}
}

void UEclipseDialogueVoiceSubsystem::SaveManifest() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : Manifest)
	{
		Root->SetStringField(Pair.Key, Pair.Value);
	}
	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	FFileHelper::SaveStringToFile(Out, *FPaths::Combine(GeneratedDir(), TEXT("VoiceCacheManifest.json")));
}
