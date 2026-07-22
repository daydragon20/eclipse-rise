// Unit tests for the voice pipeline's pure pieces (GDD 16.12; 14.4 unit layer):
// WAV wrap/parse round-trip and cache-key determinism. Headless by design — the
// helpers carry no engine-actor or audio-device dependency (14.3.2), so the
// pipeline's correctness is provable without ever spending an API credit.

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/EclipseDialogueSeed.h"
#include "Audio/EclipseDialogueVoiceSubsystem.h"
#include "Audio/EclipseWavUtil.h"
#include "Misc/AutomationTest.h"

namespace EclipseAudioTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter;

	/** Deterministic non-trivial PCM16 pattern — all-zero data would let a broken data offset still pass the byte comparison. */
	TArray<uint8> MakePcmPattern(int32 NumSamples)
	{
		TArray<uint8> Pcm;
		Pcm.Reserve(NumSamples * 2);
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			const uint16 Sample = static_cast<uint16>((static_cast<uint32>(Index) * 2654435761u) & 0xFFFF);
			Pcm.Add(static_cast<uint8>(Sample & 0xFF));
			Pcm.Add(static_cast<uint8>((Sample >> 8) & 0xFF));
		}
		return Pcm;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseAudioWavRoundTripTest,
	"Eclipse.Audio.Wav.RoundTrip",
	EclipseAudioTest::TestFlags)

bool FEclipseAudioWavRoundTripTest::RunTest(const FString& Parameters)
{
	// Mono at the TTS pipeline rate — the exact shape GenerateLine writes to the cache.
	{
		const TArray<uint8> Pcm = EclipseAudioTest::MakePcmPattern(2048);
		const TArray<uint8> Wav = EclipseWavUtil::WrapPcm16ToWav(Pcm, 22050, 1);
		TestEqual(TEXT("Canonical wav = 44-byte header + payload"), Wav.Num(), EclipseWavUtil::WavHeaderNumBytes + Pcm.Num());

		FEclipseWavInfo Info;
		TestTrue(TEXT("Wrapped buffer parses"), EclipseWavUtil::ParseWav(Wav, Info));
		TestEqual(TEXT("Sample rate round-trips"), Info.SampleRate, 22050);
		TestEqual(TEXT("Channel count round-trips"), Info.NumChannels, 1);
		TestEqual(TEXT("Bits per sample is PCM16"), Info.BitsPerSample, EclipseWavUtil::PcmBitsPerSample);

		const TArrayView<const uint8> View = EclipseWavUtil::GetPcmView(Wav, Info);
		TestEqual(TEXT("Payload size round-trips"), View.Num(), Pcm.Num());
		TestTrue(TEXT("PCM bytes are byte-identical after the round-trip"),
			View.Num() == Pcm.Num() && FMemory::Memcmp(View.GetData(), Pcm.GetData(), Pcm.Num()) == 0);
	}

	// A second configuration proves the helper is parameterised, not hardwired to the TTS default (14.2).
	{
		const TArray<uint8> Pcm = EclipseAudioTest::MakePcmPattern(500);
		const TArray<uint8> Wav = EclipseWavUtil::WrapPcm16ToWav(Pcm, 44100, 2);

		FEclipseWavInfo Info;
		TestTrue(TEXT("Stereo 44.1k parses"), EclipseWavUtil::ParseWav(Wav, Info));
		TestEqual(TEXT("Stereo sample rate round-trips"), Info.SampleRate, 44100);
		TestEqual(TEXT("Stereo channel count round-trips"), Info.NumChannels, 2);

		const TArrayView<const uint8> View = EclipseWavUtil::GetPcmView(Wav, Info);
		TestTrue(TEXT("Stereo PCM bytes are byte-identical after the round-trip"),
			View.Num() == Pcm.Num() && FMemory::Memcmp(View.GetData(), Pcm.GetData(), Pcm.Num()) == 0);
	}

	// Bad input must parse to false, never crash (14.3.5) — runtime playback feeds
	// this parser whatever happens to be on disk, including legacy .mp3 entries.
	{
		FEclipseWavInfo Info;
		TArray<uint8> Garbage;
		Garbage.Init(0xAB, 64);
		TestFalse(TEXT("Garbage does not parse"), EclipseWavUtil::ParseWav(Garbage, Info));

		const TArray<uint8> Wav = EclipseWavUtil::WrapPcm16ToWav(EclipseAudioTest::MakePcmPattern(64), 22050, 1);
		const TArray<uint8> Truncated(Wav.GetData(), 20);
		TestFalse(TEXT("Truncated header does not parse"), EclipseWavUtil::ParseWav(Truncated, Info));

		TestEqual(TEXT("Invalid sample rate wraps to an empty buffer"), EclipseWavUtil::WrapPcm16ToWav(EclipseAudioTest::MakePcmPattern(4), 0, 1).Num(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseAudioCacheKeyDeterminismTest,
	"Eclipse.Audio.Cache.KeyDeterminism",
	EclipseAudioTest::TestFlags)

bool FEclipseAudioCacheKeyDeterminismTest::RunTest(const FString& Parameters)
{
	const FString VoiceId = TEXT("voice_kaine");
	const FString ModelId = TEXT("eleven_multilingual_v2");
	const FString Text = TEXT("Hold the line. The Dominion breaks today.");

	const FString KeyA = UEclipseDialogueVoiceSubsystem::MakeCacheKey(VoiceId, ModelId, EEclipseVoiceEmotion::Confident, Text);
	const FString KeyB = UEclipseDialogueVoiceSubsystem::MakeCacheKey(VoiceId, ModelId, EEclipseVoiceEmotion::Confident, Text);

	// Same inputs => same key on every run and every machine — this is what makes
	// the committed cache enforce "never generate the same line twice" (16.15
	// rule 7) structurally instead of by discipline.
	TestEqual(TEXT("Identical inputs produce the identical key"), KeyA, KeyB);
	TestEqual(TEXT("Key is an MD5 hex file stem (32 chars)"), KeyA.Len(), 32);

	// Any changed input is a different line and must be paid for exactly once more.
	TestNotEqual(TEXT("Different emotion changes the key"), KeyA,
		UEclipseDialogueVoiceSubsystem::MakeCacheKey(VoiceId, ModelId, EEclipseVoiceEmotion::Angry, Text));
	TestNotEqual(TEXT("Different text changes the key"), KeyA,
		UEclipseDialogueVoiceSubsystem::MakeCacheKey(VoiceId, ModelId, EEclipseVoiceEmotion::Confident, TEXT("Hold the line.")));
	TestNotEqual(TEXT("Different voice changes the key"), KeyA,
		UEclipseDialogueVoiceSubsystem::MakeCacheKey(TEXT("voice_mara"), ModelId, EEclipseVoiceEmotion::Confident, Text));
	TestNotEqual(TEXT("Different model changes the key"), KeyA,
		UEclipseDialogueVoiceSubsystem::MakeCacheKey(VoiceId, TEXT("eleven_turbo_v2"), EEclipseVoiceEmotion::Confident, Text));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseAudioSeedParseRulesTest,
	"Eclipse.Audio.Seed.ParseRules",
	EclipseAudioTest::TestFlags)

bool FEclipseAudioSeedParseRulesTest::RunTest(const FString& Parameters)
{
	// Happy path — fields land, emotion string maps to the enum.
	{
		const FString Json = TEXT(R"({"voices":[{"assetName":"DA_Voice_Test","characterId":"Test.Voice","displayName":"Test Voice","elevenLabsVoiceId":"voice123","modelId":"eleven_turbo_v2","lines":[{"lineId":"L1","emotion":"Urgent","text":"Go go go!"},{"lineId":"L2","text":"Copy."}]}]})");
		TArray<FEclipseVoiceSeedEntry> Entries;
		TArray<FString> Problems;
		TestTrue(TEXT("Valid seed parses"), EclipseDialogueSeed::Parse(Json, Entries, Problems));
		TestEqual(TEXT("No problems on a clean seed"), Problems.Num(), 0);
		TestEqual(TEXT("One entry parsed"), Entries.Num(), 1);
		if (Entries.Num() == 1)
		{
			TestEqual(TEXT("AssetName lands"), Entries[0].AssetName, FString(TEXT("DA_Voice_Test")));
			TestEqual(TEXT("CharacterId lands"), Entries[0].CharacterId, FName(TEXT("Test.Voice")));
			TestEqual(TEXT("VoiceId lands"), Entries[0].ElevenLabsVoiceId, FString(TEXT("voice123")));
			TestEqual(TEXT("ModelId lands"), Entries[0].ModelId, FString(TEXT("eleven_turbo_v2")));
			TestEqual(TEXT("Both lines land"), Entries[0].Lines.Num(), 2);
			if (Entries[0].Lines.Num() == 2)
			{
				TestEqual(TEXT("Emotion string maps to enum"), Entries[0].Lines[0].Emotion, EEclipseVoiceEmotion::Urgent);
				TestEqual(TEXT("Missing emotion defaults to Neutral"), Entries[0].Lines[1].Emotion, EEclipseVoiceEmotion::Neutral);
			}
		}
	}

	// Recoverable problems are reported, never silently dropped (14.3.5): unknown
	// emotion defaults to Neutral, rows missing identity fields are skipped,
	// duplicate line ids are skipped (they would collide as asset names).
	{
		const FString Json = TEXT(R"({"voices":[
			{"assetName":"DA_A","characterId":"A","elevenLabsVoiceId":"v1","lines":[
				{"lineId":"L1","emotion":"Heroic","text":"Charge!"},
				{"lineId":"L1","text":"Duplicate id"},
				{"lineId":"","text":"No id"}]},
			{"assetName":"","characterId":"B","elevenLabsVoiceId":"v2"},
			{"assetName":"DA_A","characterId":"C","elevenLabsVoiceId":"v3"}]})");
		TArray<FEclipseVoiceSeedEntry> Entries;
		TArray<FString> Problems;
		TestTrue(TEXT("Seed with recoverable problems still parses"), EclipseDialogueSeed::Parse(Json, Entries, Problems));
		TestEqual(TEXT("Only the addressable entry survives"), Entries.Num(), 1);
		if (Entries.Num() == 1)
		{
			TestEqual(TEXT("Only the first 'L1' line survives"), Entries[0].Lines.Num(), 1);
			TestEqual(TEXT("Unknown emotion falls back to Neutral"), Entries[0].Lines[0].Emotion, EEclipseVoiceEmotion::Neutral);
		}
		// 5 = unknown emotion + duplicate lineId + empty lineId + empty assetName + duplicate assetName.
		TestEqual(TEXT("Every dropped/defaulted row is reported"), Problems.Num(), 5);
	}

	// Structurally unusable input returns false — the commandlet must know the
	// difference between "nothing to seed" and "seed file is broken".
	{
		TArray<FEclipseVoiceSeedEntry> Entries;
		TArray<FString> Problems;
		TestFalse(TEXT("Garbage is rejected"), EclipseDialogueSeed::Parse(TEXT("not json at all"), Entries, Problems));
		TestFalse(TEXT("JSON without a voices array is rejected"), EclipseDialogueSeed::Parse(TEXT("{\"foo\":1}"), Entries, Problems));
		TestTrue(TEXT("Rejection reasons are reported"), Problems.Num() > 0);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
