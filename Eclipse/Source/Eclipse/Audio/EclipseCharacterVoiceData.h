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

	/**
	 * Basisinstellingen wanneer een emotie geen override heeft. NIET GELEZEN —
	 * noch BaseSettings noch EmotionSettings hieronder komt ergens in de code
	 * terecht; de stemgeneratie draait op wat er in het genereerscript staat.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	FEclipseVoiceSettings BaseSettings;

	/** Optional per-emotion setting overrides (GDD 16.4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	TMap<EEclipseVoiceEmotion, FEclipseVoiceSettings> EmotionSettings;

	/**
	 * This character's dialogue lines — the Phase 1 dialogue database is
	 * per-character (16.3/16.5), so the voice identity and its lines live on one
	 * asset and the bulk generator + runtime playback share a single source of
	 * truth instead of a parallel table that could drift.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	TArray<FEclipseDialogueLine> Lines;

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
