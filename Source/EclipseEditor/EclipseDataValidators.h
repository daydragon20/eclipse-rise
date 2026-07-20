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
}
