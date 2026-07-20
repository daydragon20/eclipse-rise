#include "EclipseDataValidators.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Economy/EclipseEconomyDataAsset.h"
#include "Engine/DataTable.h"
#include "Strategy/EclipseRegionGraphAsset.h"
#include "Strategy/EclipseStrategyLogic.h"

namespace
{
	/** All loadable assets of a class, editor-only scan (commandlet context). */
	TArray<FAssetData> FindAssetsOfClass(const UClass* Class)
	{
		FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		AssetRegistry.Get().SearchAllAssets(/*bSynchronousSearch*/ true);

		TArray<FAssetData> Assets;
		AssetRegistry.Get().GetAssetsByClass(Class->GetClassPathName(), Assets, /*bSearchSubClasses*/ true);
		return Assets;
	}
}

namespace EclipseDataValidators
{

int32 ValidateRegionGraphAssets(TArray<FString>& OutErrors, int32& OutAssetsChecked)
{
	const int32 InitialErrors = OutErrors.Num();
	OutAssetsChecked = 0;

	for (const FAssetData& AssetData : FindAssetsOfClass(UEclipseRegionGraphAsset::StaticClass()))
	{
		const UEclipseRegionGraphAsset* Graph = Cast<UEclipseRegionGraphAsset>(AssetData.GetAsset());
		if (Graph == nullptr)
		{
			OutErrors.Add(FString::Printf(TEXT("%s: failed to load region graph"), *AssetData.GetObjectPathString()));
			continue;
		}
		++OutAssetsChecked;

		TArray<FString> GraphErrors;
		if (!EclipseStrategyLogic::ValidateGraph(Graph->Regions, GraphErrors))
		{
			for (const FString& GraphError : GraphErrors)
			{
				OutErrors.Add(FString::Printf(TEXT("%s: %s"), *AssetData.AssetName.ToString(), *GraphError));
			}
		}

		// Every region type present on the board needs an offer row, or the node
		// can never produce a mission (SPEC-P1-04: one offer per region).
		if (const UDataTable* Offers = Graph->MissionOffers.LoadSynchronous())
		{
			TSet<EEclipseRegionType> CoveredTypes;
			Offers->ForeachRow<FEclipseMissionOfferRow>(TEXT("ValidateOffers"),
				[&CoveredTypes, &OutErrors, &AssetData](const FName& RowName, const FEclipseMissionOfferRow& Row)
				{
					if (Row.TemplateId.IsNone())
					{
						OutErrors.Add(FString::Printf(TEXT("%s: offer row '%s' has no template id"), *AssetData.AssetName.ToString(), *RowName.ToString()));
					}
					CoveredTypes.Add(Row.RegionType);
				});

			for (const FEclipseRegionDefinition& Definition : Graph->Regions)
			{
				if (!CoveredTypes.Contains(Definition.RegionType))
				{
					OutErrors.Add(FString::Printf(TEXT("%s: region '%s' type has no mission-offer row"), *AssetData.AssetName.ToString(), *Definition.RegionId.ToString()));
				}
			}
		}
		else if (!Graph->Regions.IsEmpty())
		{
			OutErrors.Add(FString::Printf(TEXT("%s: region graph has no mission-offer table"), *AssetData.AssetName.ToString()));
		}
	}

	return OutErrors.Num() - InitialErrors;
}

int32 ValidateProductionItemTables(TArray<FString>& OutErrors, int32& OutAssetsChecked)
{
	const int32 InitialErrors = OutErrors.Num();
	OutAssetsChecked = 0;

	for (const FAssetData& AssetData : FindAssetsOfClass(UDataTable::StaticClass()))
	{
		const UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset());
		if (Table == nullptr || Table->GetRowStruct() != FEclipseProductionItemRow::StaticStruct())
		{
			continue;
		}
		++OutAssetsChecked;

		Table->ForeachRow<FEclipseProductionItemRow>(TEXT("ValidateProduction"),
			[&OutErrors, &AssetData](const FName& RowName, const FEclipseProductionItemRow& Row)
			{
				if (Row.TimeDays <= 0)
				{
					OutErrors.Add(FString::Printf(TEXT("%s: production row '%s' has non-positive TimeDays"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
				if (Row.CostMaterials <= 0 && Row.CostCredits <= 0)
				{
					OutErrors.Add(FString::Printf(TEXT("%s: production row '%s' is free — a choice with no cost is no choice (GDD 6.1)"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
			});
	}

	return OutErrors.Num() - InitialErrors;
}

} // namespace EclipseDataValidators
