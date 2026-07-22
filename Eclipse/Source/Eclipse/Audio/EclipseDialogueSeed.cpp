#include "Audio/EclipseDialogueSeed.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Class.h"

namespace
{
	/** Emotion string -> enum; accepts both "Urgent" and "EEclipseVoiceEmotion::Urgent". INDEX_NONE = unknown. */
	int64 ParseEmotion(const FString& EmotionString)
	{
		const UEnum* Enum = StaticEnum<EEclipseVoiceEmotion>();
		return Enum->GetValueByNameString(EmotionString);
	}
}

bool EclipseDialogueSeed::Parse(const FString& JsonText, TArray<FEclipseVoiceSeedEntry>& OutEntries, TArray<FString>& OutProblems)
{
	OutEntries.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutProblems.Add(TEXT("Seed is not valid JSON."));
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Voices = nullptr;
	if (!Root->TryGetArrayField(TEXT("voices"), Voices))
	{
		OutProblems.Add(TEXT("Seed has no \"voices\" array."));
		return false;
	}

	TSet<FString> SeenAssetNames;
	for (int32 VoiceIndex = 0; VoiceIndex < Voices->Num(); ++VoiceIndex)
	{
		const TSharedPtr<FJsonObject>* VoiceObject = nullptr;
		if (!(*Voices)[VoiceIndex]->TryGetObject(VoiceObject))
		{
			OutProblems.Add(FString::Printf(TEXT("voices[%d] is not an object — skipped."), VoiceIndex));
			continue;
		}

		FEclipseVoiceSeedEntry Entry;
		(*VoiceObject)->TryGetStringField(TEXT("assetName"), Entry.AssetName);
		FString CharacterIdString;
		(*VoiceObject)->TryGetStringField(TEXT("characterId"), CharacterIdString);
		Entry.CharacterId = FName(*CharacterIdString);
		(*VoiceObject)->TryGetStringField(TEXT("displayName"), Entry.DisplayName);
		(*VoiceObject)->TryGetStringField(TEXT("elevenLabsVoiceId"), Entry.ElevenLabsVoiceId);
		(*VoiceObject)->TryGetStringField(TEXT("modelId"), Entry.ModelId);

		// Required identity fields — an entry we cannot address or voice is unusable.
		if (Entry.AssetName.IsEmpty() || CharacterIdString.IsEmpty() || Entry.ElevenLabsVoiceId.IsEmpty())
		{
			OutProblems.Add(FString::Printf(TEXT("voices[%d] ('%s') misses assetName/characterId/elevenLabsVoiceId — skipped."), VoiceIndex, *Entry.AssetName));
			continue;
		}
		bool bAlreadySeen = false;
		SeenAssetNames.Add(Entry.AssetName, &bAlreadySeen);
		if (bAlreadySeen)
		{
			OutProblems.Add(FString::Printf(TEXT("Duplicate assetName '%s' — later entry skipped."), *Entry.AssetName));
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* Lines = nullptr;
		if ((*VoiceObject)->TryGetArrayField(TEXT("lines"), Lines))
		{
			TSet<FName> SeenLineIds;
			for (int32 LineIndex = 0; LineIndex < Lines->Num(); ++LineIndex)
			{
				const TSharedPtr<FJsonObject>* LineObject = nullptr;
				if (!(*Lines)[LineIndex]->TryGetObject(LineObject))
				{
					OutProblems.Add(FString::Printf(TEXT("'%s' lines[%d] is not an object — skipped."), *Entry.AssetName, LineIndex));
					continue;
				}

				FEclipseVoiceSeedLine Line;
				FString LineIdString;
				(*LineObject)->TryGetStringField(TEXT("lineId"), LineIdString);
				Line.LineId = FName(*LineIdString);
				(*LineObject)->TryGetStringField(TEXT("text"), Line.Text);

				// LineId doubles as the generated asset name, Text as the payload — both required.
				if (LineIdString.IsEmpty() || Line.Text.IsEmpty())
				{
					OutProblems.Add(FString::Printf(TEXT("'%s' lines[%d] misses lineId/text — skipped."), *Entry.AssetName, LineIndex));
					continue;
				}
				bool bLineSeen = false;
				SeenLineIds.Add(Line.LineId, &bLineSeen);
				if (bLineSeen)
				{
					OutProblems.Add(FString::Printf(TEXT("'%s' duplicate lineId '%s' — later line skipped."), *Entry.AssetName, *LineIdString));
					continue;
				}

				FString EmotionString;
				if ((*LineObject)->TryGetStringField(TEXT("emotion"), EmotionString))
				{
					const int64 EmotionValue = ParseEmotion(EmotionString);
					if (EmotionValue == INDEX_NONE)
					{
						OutProblems.Add(FString::Printf(TEXT("'%s' line '%s': unknown emotion '%s' — using Neutral."), *Entry.AssetName, *LineIdString, *EmotionString));
					}
					else
					{
						Line.Emotion = static_cast<EEclipseVoiceEmotion>(EmotionValue);
					}
				}

				Entry.Lines.Add(MoveTemp(Line));
			}
		}

		OutEntries.Add(MoveTemp(Entry));
	}

	return true;
}
