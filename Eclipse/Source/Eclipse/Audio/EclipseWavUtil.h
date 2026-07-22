#pragma once

#include "CoreMinimal.h"

/**
 * Pure PCM16 <-> WAV (RIFF) helpers for the voice pipeline (GDD 16.12/16.14).
 * Deliberately engine-light (no actor/audio-device headers) so the generation
 * path and its tests stay headless (14.3.2): the ElevenLabs client returns raw
 * PCM, the cache stores WAV, the editor commandlet imports it, and the runtime
 * queues it into a procedural wave — all through these functions.
 */

/** Parsed facts about a WAV buffer; offsets index into the buffer that was parsed. */
struct FEclipseWavInfo
{
	int32 SampleRate = 0;
	int32 NumChannels = 0;
	int32 BitsPerSample = 0;

	/** Byte offset of the PCM payload inside the source buffer. */
	int32 DataOffset = 0;

	/** PCM payload size in bytes (clamped to what the source buffer really holds). */
	int32 DataNumBytes = 0;
};

namespace EclipseWavUtil
{
	/** Canonical RIFF/fmt/data header size we write; parsing tolerates extra chunks. */
	inline constexpr int32 WavHeaderNumBytes = 44;

	/** The pipeline speaks 16-bit PCM only — matches ElevenLabs' pcm_* output formats. */
	inline constexpr int32 PcmBitsPerSample = 16;

	/**
	 * Prepend a 44-byte RIFF/fmt/data header (linear PCM16, little-endian) to raw
	 * PCM. Why: stored-as-WAV makes the repo cache directly importable as a
	 * USoundWave asset and directly parseable for runtime playback — no decoder
	 * dependency anywhere. Invalid rate/channel counts return an empty array
	 * (14.3.5: graceful, never a crash); a dangling odd byte (half a sample) is
	 * dropped rather than writing a header that lies about the sample count.
	 */
	ECLIPSE_API TArray<uint8> WrapPcm16ToWav(const TArray<uint8>& Pcm, int32 SampleRate, int32 Channels);

	/**
	 * Read SampleRate/NumChannels/PCM location out of a WAV buffer. Chunk-walking
	 * rather than assuming a fixed 44-byte layout — why: runtime playback must
	 * survive hand-replaced or externally produced cache files (extra LIST/fact
	 * chunks) without crashing (14.3.5). Returns false for anything that is not
	 * linear PCM16 RIFF (e.g. a legacy .mp3 cache entry).
	 */
	ECLIPSE_API bool ParseWav(const TArray<uint8>& Wav, FEclipseWavInfo& OutInfo);

	/** View of the PCM payload inside a parsed buffer; empty view if info/bounds do not match. */
	ECLIPSE_API TArrayView<const uint8> GetPcmView(const TArray<uint8>& Wav, const FEclipseWavInfo& Info);
}
