#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "EclipseGenerateVoicesCommandlet.generated.h"

/**
 * Bulk voice generation + import (GDD 16.12, the "UEclipseVoiceGenerator" role):
 * scans every UEclipseCharacterVoiceData asset via the AssetRegistry and, per
 * dialogue line: (1) fills the repo cache (Content/Audio/Generated, WAV) through
 * ElevenLabs when the content-hash key is a miss, (2) imports the cached wav as
 * a USoundWave under /Game/Audio/Generated, (3) assigns it to the line's
 * GeneratedAudio and saves the DataAsset — so runtime playback needs no loose
 * files and no key.
 *
 * Run: UnrealEditor-Cmd <project> -run=EclipseGenerateVoices [-Timeout=<seconds>]
 *
 * No API key => cached import/assign still runs, generation is skipped with a
 * plain log — never an error, so CI machines without secrets stay green
 * (task contract + 14.3.5 spirit). The key itself is never logged (16.12).
 */
UCLASS()
class UEclipseGenerateVoicesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
