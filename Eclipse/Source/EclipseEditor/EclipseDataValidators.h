#pragma once

#include "CoreMinimal.h"

/**
 * Content-data validators for the CI commandlet (GDD 12.2 rule 3: DataAssets
 * validated by a ValidateData commandlet). Each validator scans the asset
 * registry for its schema and returns human-actionable errors; the commandlet
 * fails the build on any.
 */
namespace EclipseDataValidators
{
	/** SPEC-P1-04: structural region-graph validation (orphans, asymmetric edges, ids) + offer-table sanity. */
	ECLIPSEEDITOR_API int32 ValidateRegionGraphAssets(TArray<FString>& OutErrors, int32& OutAssetsChecked);

	/** SPEC-P1-03: production tables must contain valid rows (positive time, some cost). */
	ECLIPSEEDITOR_API int32 ValidateProductionItemTables(TArray<FString>& OutErrors, int32& OutAssetsChecked);

	/** SPEC-P2-01: class-def rows must be internally sane (verb family, verb-matching tunables) and their weapon/body refs must resolve in the campaign setup they're wired into — runtime degrades such rows silently (GDD 14.3.5), so CI is where they get loud. */
	ECLIPSEEDITOR_API int32 ValidateClassDefTables(TArray<FString>& OutErrors, int32& OutAssetsChecked);

	/** SPEC-P2-05: liberation rows must be internally sane (trigger id, non-empty unique region set, unique TriggerMissionId per table) and their region ids must exist in the region graph of the campaign setup they're wired into — runtime drops such damage with a warning (GDD 14.3.5), so CI is where it gets loud. */
	ECLIPSEEDITOR_API int32 ValidateLiberationTables(TArray<FString>& OutErrors, int32& OutAssetsChecked);
}
