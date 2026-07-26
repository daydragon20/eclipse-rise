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

	/**
	 * Owner-opdracht 26-07 avond, punt 4: elk wapen moet een eigen ROL hebben en
	 * die rol moet in de data staan, niet in een hoofd.
	 *
	 * Twee soorten fout worden hier luid. Een wapen zonder rolomschrijving of met
	 * een onmogelijk profiel (mikken slordiger dan de heup, afval voorbij het
	 * bereik) is een authoring-fout. Maar de belangrijkste controle is de tweede:
	 * twee wapens die op ELKE as vrijwel gelijk zijn, zijn varianten en geen
	 * rollen — en dat is precies wat de owner niet wil. Runtime merkt daar niets
	 * van, dus CI is de enige plek waar het kan opvallen.
	 */
	ECLIPSEEDITOR_API int32 ValidateWeaponTables(TArray<FString>& OutErrors, int32& OutAssetsChecked);

	/**
	 * Loadouts (26-07 avond, punt 5): elke loadout moet wapens noemen die BESTAAN,
	 * en de twee moeten van elkaar verschillen. Een tikfout hier geeft je stilletjes
	 * de terugval, en dan lijkt het alsof je keuze niets doet — precies de bug die
	 * deze laag kwam repareren.
	 */
	ECLIPSEEDITOR_API int32 ValidateLoadoutTables(TArray<FString>& OutErrors, int32& OutAssetsChecked);
}
