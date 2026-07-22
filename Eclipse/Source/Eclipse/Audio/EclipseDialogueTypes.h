#pragma once

#include "CoreMinimal.h"
#include "EclipseDialogueTypes.generated.h"

class USoundWave;

/**
 * Dialogue/voice data types (GDD 16.3/16.12). Kept engine-light so the pure
 * generation + cache logic stays testable and the runtime carries no editor
 * dependency. The ElevenLabs API key is NEVER a field here — it comes from the
 * environment at request time (16.12 security rule).
 */

/** Emotional colour for a line; maps to ElevenLabs voice settings (GDD 16.4/17.7). */
UENUM(BlueprintType)
enum class EEclipseVoiceEmotion : uint8
{
	Neutral,
	Calm,
	Confident,
	Angry,
	Afraid,
	Sad,
	Urgent
};

/** ElevenLabs voice settings (0..1). Tunable per character and per emotion. */
USTRUCT(BlueprintType)
struct FEclipseVoiceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio", meta = (ClampMin = 0, ClampMax = 1))
	float Stability = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio", meta = (ClampMin = 0, ClampMax = 1))
	float SimilarityBoost = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio", meta = (ClampMin = 0, ClampMax = 1))
	float Style = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio")
	bool bUseSpeakerBoost = true;
};

/** One line to synthesize (a dialogue-database row; GDD 12.x/16.5). */
USTRUCT(BlueprintType)
struct FEclipseDialogueLine
{
	GENERATED_BODY()

	/** Stable id (cache manifest key + generated asset name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio")
	FName LineId;

	/** Which character speaks it (CharacterId on UEclipseCharacterVoiceData). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio")
	FName CharacterId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio", meta = (MultiLine = true))
	FString Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eclipse|Audio")
	EEclipseVoiceEmotion Emotion = EEclipseVoiceEmotion::Neutral;

	/**
	 * Imported VO asset for this exact line. Filled by the bulk tool
	 * (EclipseGenerateVoices commandlet) after ElevenLabs generation + import;
	 * when set, runtime playback uses it directly (cooked/compressed) instead of
	 * the loose wav cache. Soft ref — why: dialogue data must load without
	 * dragging every voice clip into memory (14.3.5 graceful-content rule).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Eclipse|Audio")
	TSoftObjectPtr<USoundWave> GeneratedAudio;
};
