#pragma once

#include "CoreMinimal.h"
#include "Audio/EclipseDialogueTypes.h"
#include "Audio/EclipseVoiceCache.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseDialogueVoiceSubsystem.generated.h"

class UEclipseCharacterVoiceData;

/**
 * Dialogue voice runtime + generation cache (GDD 16.12). Generates missing lines
 * through ElevenLabs (raw PCM16, stored as WAV in Content/Audio/Generated) keyed
 * by a content hash so a line is NEVER generated twice (16.15 rule 7) — re-runs
 * and other machines pay zero. The API key comes from the environment (16.12),
 * never from code or a commit.
 *
 * Generation is a dev/editor-time operation; the EclipseGenerateVoices commandlet
 * bulk-fills the cache and imports USoundWave assets. PlayLine() is the runtime
 * seam: imported asset first, raw cached wav second, generate-then-play last.
 */
UCLASS()
class ECLIPSE_API UEclipseDialogueVoiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Content hash of voice+model+emotion+text — the dedup key and the clip's file
	 * stem. Must stay byte-stable forever: every committed cache entry was paid
	 * for under this exact formula (16.13).
	 */
	static FString MakeCacheKey(const FString& VoiceId, const FString& ModelId, EEclipseVoiceEmotion Emotion, const FString& Text);

	/** Absolute path where a clip for this key lives (Content/Audio/Generated/<key>.wav; legacy entries may be .mp3). */
	FString CachePathForKey(const FString& Key) const;

	/** True if this exact line (voice+emotion+text) is already generated on disk. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Audio")
	bool IsLineCached(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text) const;

	/**
	 * Generate one line if not cached; saves the WAV + a manifest entry. OnDone
	 * gets (bSuccess, MessageOrPath). Cached lines return success immediately with
	 * zero API cost.
	 */
	void GenerateLine(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text, TFunction<void(bool, const FString&)> OnDone);

	/** Bulk fill for a character's lines (editor/dev tool). Skips cached + in-flight; returns how many API calls were queued. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Audio")
	int32 GenerateLinesForVoice(const UEclipseCharacterVoiceData* Voice, const TArray<FEclipseDialogueLine>& Lines);

	/**
	 * Play one line in-game, no editor involved (GDD 16.12 runtime shape).
	 * Resolution order and why: (1) the line's imported GeneratedAudio asset —
	 * the shipping path, cooked and compressed, works in packaged builds where
	 * loose files do not exist; (2) the cached wav parsed into a procedural wave —
	 * dev machines that generated but never ran the import commandlet still hear
	 * VO; (3) generate now and play on arrival — a brand-new line costs exactly
	 * one request, ever. Missing voice/key/asset logs and degrades, never crashes
	 * (14.3.5). Returns true when audio started or generation was queued.
	 */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Audio", meta = (WorldContext = "WorldContextObject"))
	bool PlayLine(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text, UObject* WorldContextObject, float VolumeMultiplier = 1.0f);

	/** How many lines are cached so far (manifest size). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Audio")
	int32 GetCachedLineCount() const { return Cache.Num(); }

private:
	/** Load a cached wav and play it as a one-shot procedural wave; false when the file is absent or not PCM16 WAV (e.g. a legacy .mp3 entry). */
	bool TryPlayCachedWav(const FString& WavPath, UObject* WorldContextObject, float VolumeMultiplier);

	/** Shared manifest/key->file logic — the same class the editor commandlet uses, so both writers agree on the format (16.12). */
	FEclipseVoiceCache Cache{ FEclipseVoiceCache::DefaultDirectory() };

	/** Keys currently being requested this session (avoids duplicate concurrent calls). */
	TSet<FString> InFlight;
};
