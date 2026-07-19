#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EclipseSaveDataProvider.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UEclipseSaveDataProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * Per-subsystem serialization contract (GDD 12.3: "versioned SaveGame with
 * per-subsystem serialization contracts"). Each provider owns its payload's
 * byte layout; the save subsystem owns the container, versioning and migration.
 */
class ECLIPSESAVESYSTEM_API IEclipseSaveDataProvider
{
	GENERATED_BODY()

public:
	/** Stable block id inside the save container — never rename without a migration. */
	virtual FName GetSaveBlockId() const = 0;

	/** Serialize current state into the archive (writing side of the contract). */
	virtual void WriteSaveData(FArchive& Archive) = 0;

	/**
	 * Restore state from an archive already migrated to CurrentSchemaVersion.
	 * Return false to fail the whole load (a half-loaded campaign is worse than
	 * a failed one — save divergence is the project-killer bug, GDD 12.2 rule 4).
	 */
	virtual bool ReadSaveData(FArchive& Archive) = 0;
};
