#include "Audio/EclipseVoiceCache.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	// The file name is part of the 16.12 contract ("VoiceCacheManifest.json
	// travels with the repo") — renaming it would orphan every committed cache.
	const TCHAR* GVoiceCacheManifestFileName = TEXT("VoiceCacheManifest.json");
}

FString FEclipseVoiceCache::DefaultDirectory()
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Audio"), TEXT("Generated"));
}

FEclipseVoiceCache::FEclipseVoiceCache(const FString& InDirectory)
	: Directory(InDirectory)
{
}

void FEclipseVoiceCache::Load()
{
	// Tree-create so first use on a fresh clone works without setup steps.
	IFileManager::Get().MakeDirectory(*Directory, /*Tree*/ true);

	Manifest.Reset();
	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *ManifestPath()))
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

bool FEclipseVoiceCache::Contains(const FString& Key) const
{
	return Manifest.Contains(Key) && FPaths::FileExists(PathForKey(Key));
}

FString FEclipseVoiceCache::PathForKey(const FString& Key) const
{
	// Manifest-aware resolution keeps legacy .mp3 entries valid (header comment):
	// a hit is a hit whatever the extension; only new writes are canonical .wav.
	if (const FString* ExistingFileName = Manifest.Find(Key))
	{
		return FPaths::Combine(Directory, *ExistingFileName);
	}
	return FPaths::Combine(Directory, MakeWavFileName(Key));
}

FString FEclipseVoiceCache::MakeWavFileName(const FString& Key)
{
	return Key + TEXT(".wav");
}

void FEclipseVoiceCache::Add(const FString& Key, const FString& CleanFileName)
{
	Manifest.Add(Key, CleanFileName);
	Save();
}

FString FEclipseVoiceCache::ManifestPath() const
{
	return FPaths::Combine(Directory, GVoiceCacheManifestFileName);
}

void FEclipseVoiceCache::Save() const
{
	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : Manifest)
	{
		Root->SetStringField(Pair.Key, Pair.Value);
	}
	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	FFileHelper::SaveStringToFile(Out, *ManifestPath());
}
