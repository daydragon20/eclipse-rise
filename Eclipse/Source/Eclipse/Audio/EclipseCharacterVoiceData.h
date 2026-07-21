#pragma once

#include "CoreMinimal.h"
#include "Audio/EclipseDialogueTypes.h"
#include "Engine/DataAsset.h"
#include "EclipseCharacterVoiceData.generated.h"

/**
 * One speaking character's voice identity (GDD 16.3): a fixed ElevenLabs voice
 * plus default and per-emotion settings, so a given character always sounds like
 * themselves (16.15 rule 4). All tunables in data (14.2) — no hardcoded voice ids.
 */
UCLASS(BlueprintType)
class ECLIPSE_API UEclipseCharacterVoiceData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Canonical character id (matches FEclipseDialogueLine::CharacterId and the 00_INDEX glossary). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	FString DisplayName;

	/** ElevenLabs voice id from the account's voice library (16.12). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	FString ElevenLabsVoiceId;

	/** ElevenLabs model id (default multilingual v2). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	FString ModelId = TEXT("eleven_multilingual_v2");

	/** Base settings used when an emotion has no override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	FEclipseVoiceSettings BaseSettings;

	/** Optional per-emotion setting overrides (GDD 16.4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	TMap<EEclipseVoiceEmotion, FEclipseVoiceSettings> EmotionSettings;

	/** Settings for a given emotion (override if present, else base). */
	FEclipseVoiceSettings ResolveSettings(EEclipseVoiceEmotion Emotion) const
	{
		if (const FEclipseVoiceSettings* Found = EmotionSettings.Find(Emotion))
		{
			return *Found;
		}
		return BaseSettings;
	}
};
