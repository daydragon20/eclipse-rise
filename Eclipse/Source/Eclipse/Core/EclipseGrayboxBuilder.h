#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Runtime graybox district builder (SPEC-P1-05: ~200x200 m, cover pieces, two
 * buildings, objective site, extraction zone, three entries). Generated from
 * code so the layout is reproducible and reviewable in diffs; an authored
 * .umap replaces this at the art pass. Geometry coordinates are level content,
 * not gameplay tunables — GDD 14.2 applies to balance data, which stays in
 * DataAssets.
 */
namespace EclipseGraybox
{
	/** True if the world already carries district sites (authored map or prior build). */
	ECLIPSE_API bool IsDistrictPresent(UWorld& World);

	/** Spawn the full graybox layout: geometry, sites (tagged), triggers, entries, lights. */
	ECLIPSE_API void BuildDistrict(UWorld& World);
}
