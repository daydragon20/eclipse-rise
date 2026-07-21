#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseSaveDataProvider.h"
#include "EclipseSaveSubsystem.generated.h"

/**
 * Container-level save/load (SPEC-P1-02 save v0). File layout:
 *   [Magic][SchemaVersion][BlockCount] then per block: [BlockId][Size][bytes]
 *
 * Loading migrates the block map version-by-version through the registry
 * before any provider reads — providers only ever see the current schema.
 * Save-migration tests are a per-merge CI requirement (GDD 14.4).
 */
UCLASS()
class ECLIPSESAVESYSTEM_API UEclipseSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** A migration rewrites the raw block map from FromVersion to FromVersion+1. */
	using FEclipseSaveMigration = TDelegate<bool(TMap<FName, TArray<uint8>>& /*Blocks*/)>;

	/** Providers register once (subsystem init); order of registration is not a file-format contract. */
	void RegisterProvider(const TScriptInterface<IEclipseSaveDataProvider>& Provider);
	void UnregisterProvider(const TScriptInterface<IEclipseSaveDataProvider>& Provider);

	/**
	 * Register the step that upgrades FromVersion -> FromVersion+1. The chain
	 * from file version to current must be complete or the load refuses —
	 * a silently skipped migration is corruption with extra steps.
	 */
	void RegisterMigration(int32 FromVersion, FEclipseSaveMigration Migration);

	UFUNCTION(BlueprintCallable, Category = "Eclipse|Save")
	bool SaveToSlot(const FString& SlotName, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "Eclipse|Save")
	bool LoadFromSlot(const FString& SlotName, FString& OutError);

	UFUNCTION(BlueprintCallable, Category = "Eclipse|Save")
	bool DoesSlotExist(const FString& SlotName) const;

	/** Number of migration steps executed by the last LoadFromSlot (test/diagnostic surface). */
	int32 GetLastLoadMigrationStepCount() const { return LastLoadMigrationStepCount; }

	static FString GetSlotFilePath(const FString& SlotName);

private:
	bool MigrateBlocks(int32 FileVersion, TMap<FName, TArray<uint8>>& Blocks, FString& OutError);

	TArray<TScriptInterface<IEclipseSaveDataProvider>> Providers;
	TMap<int32, FEclipseSaveMigration> Migrations;
	int32 LastLoadMigrationStepCount = 0;
};
