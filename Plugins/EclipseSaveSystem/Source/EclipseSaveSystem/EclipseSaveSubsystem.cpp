#include "EclipseSaveSubsystem.h"

#include "EclipseSaveSystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

void UEclipseSaveSubsystem::RegisterProvider(const TScriptInterface<IEclipseSaveDataProvider>& Provider)
{
	if (Provider.GetInterface() == nullptr)
	{
		UE_LOG(LogEclipseSave, Error, TEXT("RegisterProvider: null provider rejected."));
		return;
	}
	Providers.AddUnique(Provider);
}

void UEclipseSaveSubsystem::UnregisterProvider(const TScriptInterface<IEclipseSaveDataProvider>& Provider)
{
	Providers.Remove(Provider);
}

void UEclipseSaveSubsystem::RegisterMigration(int32 FromVersion, FEclipseSaveMigration Migration)
{
	if (Migrations.Contains(FromVersion))
	{
		UE_LOG(LogEclipseSave, Error, TEXT("RegisterMigration: duplicate migration for version %d rejected."), FromVersion);
		return;
	}
	Migrations.Add(FromVersion, MoveTemp(Migration));
}

FString UEclipseSaveSubsystem::GetSlotFilePath(const FString& SlotName)
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), SlotName + TEXT(".eclipsesav"));
}

bool UEclipseSaveSubsystem::DoesSlotExist(const FString& SlotName) const
{
	return IFileManager::Get().FileExists(*GetSlotFilePath(SlotName));
}

bool UEclipseSaveSubsystem::SaveToSlot(const FString& SlotName, FString& OutError)
{
	if (Providers.IsEmpty())
	{
		OutError = TEXT("No save data providers registered");
		return false;
	}

	TArray<uint8> FileBytes;
	FMemoryWriter FileWriter(FileBytes);

	uint32 Magic = EclipseSave::FileMagic;
	int32 Version = EclipseSave::CurrentSchemaVersion;
	int32 BlockCount = Providers.Num();
	FileWriter << Magic << Version << BlockCount;

	for (const TScriptInterface<IEclipseSaveDataProvider>& Provider : Providers)
	{
		TArray<uint8> BlockBytes;
		FMemoryWriter BlockWriter(BlockBytes);
		Provider.GetInterface()->WriteSaveData(BlockWriter);

		FString BlockId = Provider.GetInterface()->GetSaveBlockId().ToString();
		int64 BlockSize = BlockBytes.Num();
		FileWriter << BlockId << BlockSize;
		FileWriter.Serialize(BlockBytes.GetData(), BlockBytes.Num());
	}

	const FString Path = GetSlotFilePath(SlotName);
	if (!FFileHelper::SaveArrayToFile(FileBytes, *Path))
	{
		OutError = FString::Printf(TEXT("Could not write save file '%s'"), *Path);
		return false;
	}

	UE_LOG(LogEclipseSave, Display, TEXT("Saved slot '%s' (%d blocks, %d bytes, schema v%d)."), *SlotName, BlockCount, FileBytes.Num(), Version);
	return true;
}

bool UEclipseSaveSubsystem::LoadFromSlot(const FString& SlotName, FString& OutError)
{
	LastLoadMigrationStepCount = 0;

	const FString Path = GetSlotFilePath(SlotName);
	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *Path))
	{
		OutError = FString::Printf(TEXT("Save slot '%s' not found"), *SlotName);
		return false;
	}

	FMemoryReader FileReader(FileBytes);
	uint32 Magic = 0;
	int32 FileVersion = 0;
	int32 BlockCount = 0;
	FileReader << Magic << FileVersion << BlockCount;

	if (Magic != EclipseSave::FileMagic)
	{
		OutError = FString::Printf(TEXT("'%s' is not an Eclipse save file"), *Path);
		return false;
	}
	if (FileVersion > EclipseSave::CurrentSchemaVersion)
	{
		OutError = FString::Printf(TEXT("Save schema v%d is newer than this build (v%d)"), FileVersion, EclipseSave::CurrentSchemaVersion);
		return false;
	}
	if (BlockCount < 0 || FileReader.IsError())
	{
		OutError = TEXT("Corrupt save header");
		return false;
	}

	TMap<FName, TArray<uint8>> Blocks;
	for (int32 Index = 0; Index < BlockCount; ++Index)
	{
		FString BlockId;
		int64 BlockSize = 0;
		FileReader << BlockId << BlockSize;
		if (FileReader.IsError() || BlockSize < 0 || FileReader.Tell() + BlockSize > FileBytes.Num())
		{
			OutError = FString::Printf(TEXT("Corrupt save block %d"), Index);
			return false;
		}

		TArray<uint8>& BlockBytes = Blocks.Add(FName(*BlockId));
		BlockBytes.SetNumUninitialized(static_cast<int32>(BlockSize));
		FileReader.Serialize(BlockBytes.GetData(), BlockSize);
	}

	if (!MigrateBlocks(FileVersion, Blocks, OutError))
	{
		return false;
	}

	for (const TScriptInterface<IEclipseSaveDataProvider>& Provider : Providers)
	{
		const FName BlockId = Provider.GetInterface()->GetSaveBlockId();
		TArray<uint8>* BlockBytes = Blocks.Find(BlockId);
		if (BlockBytes == nullptr)
		{
			// Absent block = provider added after the save was written. The provider
			// keeps defaults — graceful, logged, not fatal (GDD 14.3.5).
			UE_LOG(LogEclipseSave, Warning, TEXT("Save slot '%s' has no block '%s'; provider keeps defaults."), *SlotName, *BlockId.ToString());
			continue;
		}

		FMemoryReader BlockReader(*BlockBytes);
		if (!Provider.GetInterface()->ReadSaveData(BlockReader))
		{
			OutError = FString::Printf(TEXT("Provider '%s' rejected its save block"), *BlockId.ToString());
			return false;
		}
	}

	UE_LOG(LogEclipseSave, Display, TEXT("Loaded slot '%s' (schema v%d -> v%d, %d migration steps)."), *SlotName, FileVersion, EclipseSave::CurrentSchemaVersion, LastLoadMigrationStepCount);
	return true;
}

bool UEclipseSaveSubsystem::MigrateBlocks(int32 FileVersion, TMap<FName, TArray<uint8>>& Blocks, FString& OutError)
{
	for (int32 Version = FileVersion; Version < EclipseSave::CurrentSchemaVersion; ++Version)
	{
		FEclipseSaveMigration* Migration = Migrations.Find(Version);
		if (Migration == nullptr || !Migration->IsBound())
		{
			OutError = FString::Printf(TEXT("No migration registered for save schema v%d -> v%d"), Version, Version + 1);
			return false;
		}
		if (!Migration->Execute(Blocks))
		{
			OutError = FString::Printf(TEXT("Migration v%d -> v%d failed"), Version, Version + 1);
			return false;
		}
		++LastLoadMigrationStepCount;
	}
	return true;
}
