#pragma once

#include "CoreMinimal.h"

/**
 * Voice-line cache bookkeeping (GDD 16.12): the VoiceCacheManifest.json and
 * key->file resolution over the generated-audio folder. One class shared by the
 * runtime subsystem and the editor bulk commandlet — why: two independent
 * manifest writers would inevitably drift and break the "never generate the
 * same line twice" guarantee (16.15 rule 7). Engine-light on purpose (no actor
 * headers) so it works in commandlets and headless tests (14.3.2).
 *
 * Format note: entries were historically .mp3; the pipeline now stores .wav
 * (PCM16). Legacy .mp3 manifest entries stay valid — a hit never regenerates —
 * but once such a file is missing, the miss path simply regenerates the line as
 * .wav and the manifest entry upgrades with it.
 */
class ECLIPSE_API FEclipseVoiceCache
{
public:
	/**
	 * Content/Audio/Generated — deliberately Content, not Saved: 16.12 requires
	 * the cache (audio + manifest) to travel with the repo so no machine ever
	 * re-pays for an existing line. The loose .wav/.json files are not .uassets —
	 * that is fine: they are the source of truth, and the EclipseGenerateVoices
	 * commandlet creates USoundWave assets next to them in this same folder.
	 */
	static FString DefaultDirectory();

	explicit FEclipseVoiceCache(const FString& InDirectory);

	/**
	 * Ensure the folder exists and read the manifest. A missing manifest is an
	 * empty cache, not an error; an unwritable folder (packaged build) is fine
	 * too — generation is a dev/editor-time operation and shipped playback uses
	 * imported assets, not loose files (16.4).
	 */
	void Load();

	/** True only when the manifest knows the key AND the file is really on disk — a stale entry must count as a miss. */
	bool Contains(const FString& Key) const;

	/** Where a hit for this key lives: the manifest's file name if present (may be legacy .mp3), else the canonical .wav path. */
	FString PathForKey(const FString& Key) const;

	/** Canonical file name new generations write: <key>.wav (regenerations never reuse a stale legacy name — content is WAV now). */
	static FString MakeWavFileName(const FString& Key);

	/** Record a generated file and persist the manifest immediately — a crash later must not lose paid-for API work. */
	void Add(const FString& Key, const FString& CleanFileName);

	int32 Num() const { return Manifest.Num(); }
	const FString& GetDirectory() const { return Directory; }

private:
	void Save() const;
	FString ManifestPath() const;

	FString Directory;

	/** Cache key (MD5 of voice+model+emotion+text) -> clean file name inside Directory. */
	TMap<FString, FString> Manifest;
};
