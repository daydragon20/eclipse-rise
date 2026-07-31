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

	/**
	 * Alleen de sfeerlaag: sloopt zon/skylight/atmosphere/mist en zet ze terug.
	 * BuildDistrict roept dit aan; het staat apart omdat het GPU-crashdossier
	 * (phase0/DEBUG_DISCIPLINE.md 4.5) een page fault aanwijst in de
	 * SkyAtmosphere-LUT compute-pass, en dit de enige code in het project is die
	 * een ASkyAtmosphere vernietigt en meteen opnieuw spawnt. Een reproductie
	 * moet EXACT dit pad kunnen aanroepen; iets wat er op lijkt bewijst niets.
	 */
	ECLIPSE_API void RebuildDistrictSky(UWorld& World);
}
