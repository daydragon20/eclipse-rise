#include "EclipseGenerateVoicesCommandlet.h"

#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Audio/EclipseCharacterVoiceData.h"
#include "Audio/EclipseDialogueSeed.h"
#include "Audio/EclipseDialogueVoiceSubsystem.h"
#include "Audio/EclipseElevenLabsClient.h"
#include "Audio/EclipseVoiceCache.h"
#include "Audio/EclipseWavUtil.h"
#include "EclipseEditor.h"
#include "HAL/PlatformProcess.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	/** Imported USoundWave assets land next to the loose wavs (16.12: everything voice-generated lives under Content/Audio/Generated). */
	const TCHAR* GGeneratedAssetPath = TEXT("/Game/Audio/Generated");

	/** Default per-line wait; override with -Timeout=<seconds> (a parameter, not a hardcoded behavior — 14.2). */
	constexpr double GDefaultTimeoutSeconds = 60.0;

	/** Pump cadence while blocked. Only trades idle latency vs. CPU during the wait — not a gameplay tunable, hence a named constant. */
	constexpr float GHttpPumpIntervalSeconds = 0.05f;

	/** Stop calling the API after this many consecutive failures — why: a dead network or exhausted quota must not turn the run into Lines x Timeout. */
	constexpr int32 GMaxConsecutiveGenerationFailures = 3;

	/** Completion state shared with the HTTP lambda (see RequestTtsBlocking for why it must be heap-shared). */
	struct FEclipseTtsBlockingResult
	{
		bool bDone = false;
		bool bSuccess = false;
		TArray<uint8> Pcm;
		FString Error;
	};

	/**
	 * Synchronous TTS wait. Why the manual pump: commandlets have no game loop, so
	 * nothing ticks FHttpModule's manager and completion delegates would never
	 * fire; we tick it ourselves and sleep between pumps to stay off a hot spin
	 * (the transfer runs on the HTTP thread — Tick only dispatches finished
	 * requests back to this thread). The result struct is shared-ptr owned — why:
	 * on a timeout we return while the request is still in flight, and a later
	 * pump (next line) may complete it; the lambda must then write into memory
	 * that is still alive, not into this call's dead stack frame.
	 */
	bool RequestTtsBlocking(const FString& ApiKey, const UEclipseCharacterVoiceData& Voice, const FEclipseDialogueLine& Line, double TimeoutSeconds, TArray<uint8>& OutPcm, FString& OutError)
	{
		TSharedRef<FEclipseTtsBlockingResult, ESPMode::ThreadSafe> Result = MakeShared<FEclipseTtsBlockingResult, ESPMode::ThreadSafe>();
		EclipseElevenLabs::RequestTextToSpeech(ApiKey, Voice.ElevenLabsVoiceId, Voice.ModelId, Line.Text, Voice.ResolveSettings(Line.Emotion),
			EclipseElevenLabs::FOnTtsComplete::CreateLambda(
				[Result](bool bSuccess, const TArray<uint8>& Pcm, const FString& Error)
				{
					Result->bSuccess = bSuccess;
					Result->Pcm = Pcm;
					Result->Error = Error;
					Result->bDone = true;
				}));

		const double StartSeconds = FPlatformTime::Seconds();
		while (!Result->bDone)
		{
			if (FPlatformTime::Seconds() - StartSeconds > TimeoutSeconds)
			{
				OutError = FString::Printf(TEXT("Timed out after %.0f s"), TimeoutSeconds);
				return false;
			}
			FHttpModule::Get().GetHttpManager().Tick(GHttpPumpIntervalSeconds);
			FPlatformProcess::Sleep(GHttpPumpIntervalSeconds);
		}

		OutPcm = Result->Pcm;
		OutError = Result->Error;
		return Result->bSuccess && Result->Pcm.Num() > 0;
	}

	/** Import one cached wav as a USoundWave asset. bAutomated — why: a commandlet can never answer an import dialog. */
	USoundWave* ImportWavAsset(const FString& WavAbsolutePath, const FString& AssetName)
	{
		UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
		ImportTask->bAutomated = true;
		ImportTask->bReplaceExisting = true; // a re-generated line reuses its asset name instead of piling up suffixed duplicates
		ImportTask->bSave = true;
		ImportTask->Filename = WavAbsolutePath;
		ImportTask->DestinationPath = GGeneratedAssetPath;
		ImportTask->DestinationName = ObjectTools::SanitizeObjectName(AssetName);

		FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetTools.Get().ImportAssetTasks({ ImportTask });

		for (UObject* Imported : ImportTask->GetObjects())
		{
			if (USoundWave* Wave = Cast<USoundWave>(Imported))
			{
				return Wave;
			}
		}
		return nullptr;
	}

	/** Persist a modified voice DataAsset — the GeneratedAudio assignment is useless if it only lives in memory. */
	bool SaveVoiceAsset(UEclipseCharacterVoiceData& Voice)
	{
		UPackage* Package = Voice.GetOutermost();
		Package->MarkPackageDirty();
		const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Package, &Voice, *PackageFileName, SaveArgs);
	}

	/** Where seeded voice identity assets are created — /Game/Audio, not .../Generated: identity data is authored input, Generated holds pipeline output only (16.14 layout). */
	const TCHAR* GVoiceIdentityAssetPath = TEXT("/Game/Audio");

	/**
	 * Materialise seed entries into UEclipseCharacterVoiceData assets. Create-only
	 * by contract: an existing asset is the live, designer-owned copy (it may carry
	 * GeneratedAudio assignments and tuned settings), so the seed never touches it —
	 * re-running the commandlet is always safe.
	 */
	int32 SeedVoiceAssets(const FString& SeedFilePath)
	{
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *SeedFilePath))
		{
			// No seed file is a valid state (assets may be authored directly in-editor).
			return 0;
		}

		TArray<FEclipseVoiceSeedEntry> Entries;
		TArray<FString> Problems;
		const bool bParsed = EclipseDialogueSeed::Parse(JsonText, Entries, Problems);
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogEclipseEditor, Warning, TEXT("GenerateVoices: seed: %s"), *Problem);
		}
		if (!bParsed)
		{
			return 0;
		}

		int32 NumCreated = 0;
		for (const FEclipseVoiceSeedEntry& Entry : Entries)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), GVoiceIdentityAssetPath, *ObjectTools::SanitizeObjectName(Entry.AssetName));
			if (FPackageName::DoesPackageExist(PackageName))
			{
				continue; // live asset wins — the seed only bootstraps
			}

			UPackage* Package = CreatePackage(*PackageName);
			UEclipseCharacterVoiceData* Voice = NewObject<UEclipseCharacterVoiceData>(Package, FName(*ObjectTools::SanitizeObjectName(Entry.AssetName)), RF_Public | RF_Standalone);
			Voice->CharacterId = Entry.CharacterId;
			Voice->DisplayName = Entry.DisplayName;
			Voice->ElevenLabsVoiceId = Entry.ElevenLabsVoiceId;
			if (!Entry.ModelId.IsEmpty())
			{
				Voice->ModelId = Entry.ModelId;
			}
			for (const FEclipseVoiceSeedLine& SeedLine : Entry.Lines)
			{
				FEclipseDialogueLine Line;
				Line.LineId = SeedLine.LineId;
				Line.CharacterId = Entry.CharacterId;
				Line.Text = SeedLine.Text;
				Line.Emotion = SeedLine.Emotion;
				Voice->Lines.Add(MoveTemp(Line));
			}

			FAssetRegistryModule::AssetCreated(Voice);
			if (SaveVoiceAsset(*Voice))
			{
				++NumCreated;
				UE_LOG(LogEclipseEditor, Display, TEXT("GenerateVoices: seeded %s (%d lines)."), *PackageName, Voice->Lines.Num());
			}
			else
			{
				UE_LOG(LogEclipseEditor, Warning, TEXT("GenerateVoices: could not save seeded asset %s"), *PackageName);
			}
		}
		return NumCreated;
	}
}

int32 UEclipseGenerateVoicesCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamValues;
	ParseCommandLine(*Params, Tokens, Switches, ParamValues);

	double TimeoutSeconds = GDefaultTimeoutSeconds;
	if (const FString* TimeoutValue = ParamValues.Find(TEXT("Timeout")))
	{
		TimeoutSeconds = FCString::Atod(**TimeoutValue);
	}

	// Key from env/UserSecrets only (16.12). Never logged, never persisted here.
	const FString ApiKey = EclipseElevenLabs::GetApiKeyFromEnvironment();
	if (ApiKey.IsEmpty())
	{
		// Plain log + cache-only mode, not an error — CI without secrets stays green.
		UE_LOG(LogEclipseEditor, Display, TEXT("GenerateVoices: no ELEVENLABS_API_KEY / UserSecrets.ini key — cache-only mode (import + assign existing wavs, no generation)."));
	}

	// Step 0 — bootstrap the dialogue database from the seed file (16.12): a fresh
	// clone gets voice assets without a human clicking through the editor. -Seed=
	// overrides the canonical location.
	FString SeedPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Audio/DialogueSeed.json"));
	if (const FString* SeedOverride = ParamValues.Find(TEXT("Seed")))
	{
		SeedPath = *SeedOverride;
	}
	const int32 NumSeeded = SeedVoiceAssets(SeedPath);
	if (NumSeeded > 0)
	{
		UE_LOG(LogEclipseEditor, Display, TEXT("GenerateVoices: seeded %d new voice asset(s) from %s."), NumSeeded, *SeedPath);
	}

	FEclipseVoiceCache Cache(FEclipseVoiceCache::DefaultDirectory());
	Cache.Load();

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistry.Get().SearchAllAssets(/*bSynchronousSearch*/ true);
	TArray<FAssetData> VoiceAssets;
	AssetRegistry.Get().GetAssetsByClass(UEclipseCharacterVoiceData::StaticClass()->GetClassPathName(), VoiceAssets, /*bSearchSubClasses*/ true);

	int32 NumLines = 0;
	int32 NumCached = 0;
	int32 NumGenerated = 0;
	int32 NumImported = 0;
	int32 NumSkipped = 0;
	int32 NumFailed = 0;
	int32 ConsecutiveFailures = 0;

	for (const FAssetData& AssetData : VoiceAssets)
	{
		UEclipseCharacterVoiceData* Voice = Cast<UEclipseCharacterVoiceData>(AssetData.GetAsset());
		if (Voice == nullptr)
		{
			UE_LOG(LogEclipseEditor, Warning, TEXT("GenerateVoices: failed to load %s"), *AssetData.GetObjectPathString());
			continue;
		}
		if (Voice->ElevenLabsVoiceId.IsEmpty())
		{
			// Not wired to a provider voice yet — a data gap to report, not an error (14.3.5).
			UE_LOG(LogEclipseEditor, Display, TEXT("GenerateVoices: '%s' has no ElevenLabs voice id — skipped (%d lines)."), *AssetData.AssetName.ToString(), Voice->Lines.Num());
			continue;
		}

		bool bAssetModified = false;
		for (FEclipseDialogueLine& Line : Voice->Lines)
		{
			if (Line.Text.IsEmpty())
			{
				continue;
			}
			++NumLines;

			const FString Key = UEclipseDialogueVoiceSubsystem::MakeCacheKey(Voice->ElevenLabsVoiceId, Voice->ModelId, Line.Emotion, Line.Text);

			// Step 1 — fill the cache. A hit is free forever (16.15 rule 7).
			if (Cache.Contains(Key))
			{
				++NumCached;
			}
			else if (ApiKey.IsEmpty() || ConsecutiveFailures >= GMaxConsecutiveGenerationFailures)
			{
				++NumSkipped;
				continue;
			}
			else
			{
				TArray<uint8> Pcm;
				FString Error;
				if (!RequestTtsBlocking(ApiKey, *Voice, Line, TimeoutSeconds, Pcm, Error))
				{
					++NumFailed;
					++ConsecutiveFailures;
					UE_LOG(LogEclipseEditor, Warning, TEXT("GenerateVoices: '%s' line '%s' generation failed: %s"), *AssetData.AssetName.ToString(), *Line.LineId.ToString(), *Error);
					continue;
				}
				const FString FileName = FEclipseVoiceCache::MakeWavFileName(Key);
				const TArray<uint8> WavBytes = EclipseWavUtil::WrapPcm16ToWav(Pcm, EclipseElevenLabs::TtsPcmSampleRate, EclipseElevenLabs::TtsPcmNumChannels);
				if (WavBytes.IsEmpty() || !FFileHelper::SaveArrayToFile(WavBytes, *FPaths::Combine(Cache.GetDirectory(), FileName)))
				{
					++NumFailed;
					++ConsecutiveFailures;
					UE_LOG(LogEclipseEditor, Warning, TEXT("GenerateVoices: could not write wav for line '%s'"), *Line.LineId.ToString());
					continue;
				}
				Cache.Add(Key, FileName);
				ConsecutiveFailures = 0;
				++NumGenerated;
			}

			// Step 2 — import + assign so runtime playback uses a real asset (16.12 step 3).
			if (!Line.GeneratedAudio.IsNull() && Line.GeneratedAudio.LoadSynchronous() != nullptr)
			{
				continue; // already imported + assigned by an earlier run
			}
			const FString WavPath = Cache.PathForKey(Key);
			if (!WavPath.EndsWith(TEXT(".wav")))
			{
				// Legacy .mp3 cache entry: still a valid cache hit (never re-spend),
				// but only wavs are imported — delete the file to regenerate as wav.
				UE_LOG(LogEclipseEditor, Display, TEXT("GenerateVoices: line '%s' cached as legacy mp3 — not imported; delete %s to regenerate as wav."), *Line.LineId.ToString(), *WavPath);
				continue;
			}

			// Asset name: LineId is documented as "cache manifest key + generated
			// asset name" (EclipseDialogueTypes.h); the content hash is the
			// deterministic fallback when a line has no id yet.
			const FString AssetName = Line.LineId.IsNone() ? Key : Line.LineId.ToString();
			USoundWave* ImportedWave = ImportWavAsset(FPaths::ConvertRelativePathToFull(WavPath), AssetName);
			if (ImportedWave == nullptr)
			{
				++NumFailed;
				UE_LOG(LogEclipseEditor, Warning, TEXT("GenerateVoices: import failed for %s"), *WavPath);
				continue;
			}
			Line.GeneratedAudio = ImportedWave;
			bAssetModified = true;
			++NumImported;
		}

		if (bAssetModified && !SaveVoiceAsset(*Voice))
		{
			UE_LOG(LogEclipseEditor, Warning, TEXT("GenerateVoices: could not save %s"), *AssetData.GetObjectPathString());
		}
	}

	// The end report is the tool's contract (16.12): cached should trend to 100%
	// as the dialogue database stabilises — generation cost goes to zero.
	UE_LOG(LogEclipseEditor, Display, TEXT("GenerateVoices: %d voice assets, %d lines — %d cached, %d generated, %d imported/assigned, %d skipped (no key/backoff), %d failed."),
		VoiceAssets.Num(), NumLines, NumCached, NumGenerated, NumImported, NumSkipped, NumFailed);

	// Always 0 — why: generation is best-effort tooling; a machine without secrets
	// or a provider hiccup must never fail CI (task contract). Failures are
	// visible in the summary + warnings above.
	return 0;
}
