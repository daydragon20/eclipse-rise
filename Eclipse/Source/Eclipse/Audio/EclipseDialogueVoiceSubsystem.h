#pragma once

#include "CoreMinimal.h"
#include "Audio/EclipseDialogueTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseDialogueVoiceSubsystem.generated.h"

class UEclipseCharacterVoiceData;

/**
 * Dialogue voice runtime + generation cache (GDD 16.12). Generates missing lines
 * through ElevenLabs and stores them locally, keyed by a content hash so a line
 * is NEVER generated twice (16.15 rule 7) — re-runs and other machines pay zero.
 * The API key comes from the environment (16.12), never from code or a commit.
 *
 * Generation is a dev/editor-time operation; a thin editor tool (or Blueprint
 * utility) calls GenerateLinesForVoice() to bulk-fill the cache.
 */
UCLASS()
class ECLIPSE_API UEclipseDialogueVoiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Content hash of voice+model+emotion+text — the dedup key and the clip's file name. */
	static FString MakeCacheKey(const FString& VoiceId, const FString& ModelId, EEclipseVoiceEmotion Emotion, const FString& Text);

	/** Absolute path where a clip for this key lives (Saved/Audio/Generated/<key>.mp3). */
	FString CachePathForKey(const FString& Key) const;

	/** True if this exact line (voice+emotion+text) is already generated on disk. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Audio")
	bool IsLineCached(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text) const;

	/**
	 * Generate one line if not cached; saves the MP3 + a manifest entry. OnDone
	 * gets (bSuccess, MessageOrPath). Cached lines return success immediately with
	 * zero API cost.
	 */
	void GenerateLine(const UEclipseCharacterVoiceData* Voice, EEclipseVoiceEmotion Emotion, const FString& Text, TFunction<void(bool, const FString&)> OnDone);

	/** Bulk fill for a character's lines (editor/dev tool). Skips cached + in-flight; returns how many API calls were queued. */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Audio")
	int32 GenerateLinesForVoice(const UEclipseCharacterVoiceData* Voice, const TArray<FEclipseDialogueLine>& Lines);

	/** How many lines are cached so far (manifest size). */
	UFUNCTION(BlueprintCallable, Category = "Eclipse|Audio")
	int32 GetCachedLineCount() const { return Manifest.Num(); }

private:
	FString GeneratedDir() const;
	void LoadManifest();
	void SaveManifest() const;

	/** cacheKey -> relative mp3 file name. */
	TMap<FString, FString> Manifest;

	/** Keys currently being requested this session (avoids duplicate concurrent calls). */
	TSet<FString> InFlight;
};
