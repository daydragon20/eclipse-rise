#include "EclipseDataValidators.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Economy/EclipseEconomyDataAsset.h"
#include "Engine/DataTable.h"
#include "Quests/EclipseStoryTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseLiberationTypes.h"
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

int32 ValidateClassDefTables(TArray<FString>& OutErrors, int32& OutAssetsChecked)
{
	const int32 InitialErrors = OutErrors.Num();
	OutAssetsChecked = 0;

	// Intra-row sanity on every ClassDefs-shaped table, wired into a setup or
	// not. ClampMin only guards hand-edits in the editor UI; script-filled
	// tables (setup_class_data.py) bypass it.
	for (const FAssetData& AssetData : FindAssetsOfClass(UDataTable::StaticClass()))
	{
		const UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset());
		if (Table == nullptr || Table->GetRowStruct() != FEclipseClassDefRow::StaticStruct())
		{
			continue;
		}
		++OutAssetsChecked;

		Table->ForeachRow<FEclipseClassDefRow>(TEXT("ValidateClassDefs"),
			[&OutErrors, &AssetData](const FName& RowName, const FEclipseClassDefRow& Row)
			{
				if (Row.DisplayName.IsEmpty())
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class row '%s' has no display name (muster shows it — SPEC-P2-01)"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
				if (!Row.SignatureVerb.IsValid())
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class row '%s' has no signature verb (GDD 4.2.3)"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
				else if (!Row.SignatureVerb.ToString().StartsWith(TEXT("Class.Verb.")))
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class row '%s' signature verb '%s' is outside the Class.Verb family"), *AssetData.AssetName.ToString(), *RowName.ToString(), *Row.SignatureVerb.ToString()));
				}
				// Name-compare: the native tag symbols live unexported in the
				// game module; the editor module only needs identity, not the tag.
				if (Row.SignatureVerb.GetTagName() == FName(TEXT("Class.Verb.Stabilize")) && Row.StabilizeWindowSeconds <= 0.0f)
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class row '%s' carries Stabilize but has no stabilize window (GDD 4.2.5)"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
				if (Row.SignatureVerb.GetTagName() == FName(TEXT("Class.Verb.Killzone")) && Row.KillzoneRangeCm <= 0.0f)
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class row '%s' carries Killzone but has no lane range"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
				if (Row.StabilizeWindowSeconds < 0.0f || Row.KillzoneRangeCm < 0.0f || Row.OrderPushDistanceCm < 0.0f || Row.CoverLaneBias < 0.0f)
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class row '%s' has a negative tunable"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
			});
	}

	// Cross-refs resolve against the setup the table is wired into: a class kit
	// pointing at a missing weapon/body row deploys the fallback kit at runtime
	// (GDD 14.3.5) — legal in play, a defect in data.
	for (const FAssetData& AssetData : FindAssetsOfClass(UEclipseCampaignSetupAsset::StaticClass()))
	{
		const UEclipseCampaignSetupAsset* Setup = Cast<UEclipseCampaignSetupAsset>(AssetData.GetAsset());
		if (Setup == nullptr)
		{
			continue;
		}

		const UDataTable* ClassDefs = Setup->ClassDefs.LoadSynchronous();
		if (ClassDefs == nullptr)
		{
			// Missing table = classless campaign, a legal pre-content state (GDD 14.3.5).
			continue;
		}
		++OutAssetsChecked;
		if (ClassDefs->GetRowStruct() != FEclipseClassDefRow::StaticStruct())
		{
			OutErrors.Add(FString::Printf(TEXT("%s: ClassDefs table '%s' has the wrong row struct"), *AssetData.AssetName.ToString(), *ClassDefs->GetName()));
			continue;
		}

		// A wrong-shaped Weapons/BodyDefs table would pass a bare name lookup
		// while runtime rightly rejects the row — report it and treat the table
		// as absent so every ref against it errors too.
		const UDataTable* Weapons = Setup->Weapons.LoadSynchronous();
		if (Weapons != nullptr && Weapons->GetRowStruct() != FEclipseWeaponRow::StaticStruct())
		{
			OutErrors.Add(FString::Printf(TEXT("%s: Weapons table '%s' has the wrong row struct"), *AssetData.AssetName.ToString(), *Weapons->GetName()));
			Weapons = nullptr;
		}
		const UDataTable* BodyDefs = Setup->BodyDefs.LoadSynchronous();
		if (BodyDefs != nullptr && BodyDefs->GetRowStruct() != FEclipseBodyDefRow::StaticStruct())
		{
			OutErrors.Add(FString::Printf(TEXT("%s: BodyDefs table '%s' has the wrong row struct"), *AssetData.AssetName.ToString(), *BodyDefs->GetName()));
			BodyDefs = nullptr;
		}
		ClassDefs->ForeachRow<FEclipseClassDefRow>(TEXT("ValidateClassDefRefs"),
			[&OutErrors, &AssetData, Weapons, BodyDefs](const FName& RowName, const FEclipseClassDefRow& Row)
			{
				if (!Row.WeaponRow.IsNone() && (Weapons == nullptr || Weapons->FindRowUnchecked(Row.WeaponRow) == nullptr))
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class '%s' weapon row '%s' is not in the setup's Weapons table"), *AssetData.AssetName.ToString(), *RowName.ToString(), *Row.WeaponRow.ToString()));
				}
				if (!Row.BodyDefOverride.IsNone() && (BodyDefs == nullptr || BodyDefs->FindRowUnchecked(Row.BodyDefOverride) == nullptr))
				{
					OutErrors.Add(FString::Printf(TEXT("%s: class '%s' body row '%s' is not in the setup's BodyDefs table"), *AssetData.AssetName.ToString(), *RowName.ToString(), *Row.BodyDefOverride.ToString()));
				}
			});
	}

	return OutErrors.Num() - InitialErrors;
}

int32 ValidateLiberationTables(TArray<FString>& OutErrors, int32& OutAssetsChecked)
{
	const int32 InitialErrors = OutErrors.Num();
	OutAssetsChecked = 0;

	// Intra-row sanity on every liberation-shaped table, wired into a setup or
	// not (the ValidateClassDefTables pattern): runtime degrades this damage
	// silently-but-warned (GDD 14.3.5) — CI is where it fails the build.
	// NOTE: NewOwner is a plain enum with a Player default — an "empty owner"
	// cannot exist by construction; a wrong-shaped row is caught by the
	// row-struct checks below.
	for (const FAssetData& AssetData : FindAssetsOfClass(UDataTable::StaticClass()))
	{
		const UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset());
		if (Table == nullptr || Table->GetRowStruct() != FEclipseLiberationRow::StaticStruct())
		{
			continue;
		}
		++OutAssetsChecked;

		TSet<FName> SeenTriggers;
		Table->ForeachRow<FEclipseLiberationRow>(TEXT("ValidateLiberation"),
			[&OutErrors, &AssetData, &SeenTriggers](const FName& RowName, const FEclipseLiberationRow& Row)
			{
				if (Row.TriggerMissionId.IsNone())
				{
					OutErrors.Add(FString::Printf(TEXT("%s: liberation row '%s' has no TriggerMissionId — it can never fire (SPEC-P2-05 decision 2)"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
				else
				{
					bool bAlreadySeen = false;
					SeenTriggers.Add(Row.TriggerMissionId, &bAlreadySeen);
					if (bAlreadySeen)
					{
						OutErrors.Add(FString::Printf(TEXT("%s: liberation row '%s' duplicates TriggerMissionId '%s' — one writer per mission family (SPEC-P2-05 decision 2)"), *AssetData.AssetName.ToString(), *RowName.ToString(), *Row.TriggerMissionId.ToString()));
					}
				}

				if (Row.RegionIds.IsEmpty())
				{
					OutErrors.Add(FString::Printf(TEXT("%s: liberation row '%s' has an empty region set — a liberation that frees nothing"), *AssetData.AssetName.ToString(), *RowName.ToString()));
				}
				TSet<FName> SeenRegions;
				for (const FName& RegionId : Row.RegionIds)
				{
					bool bRegionSeen = false;
					SeenRegions.Add(RegionId, &bRegionSeen);
					if (bRegionSeen)
					{
						OutErrors.Add(FString::Printf(TEXT("%s: liberation row '%s' repeats region id '%s' — resolution drops the repeat at runtime (GDD 14.3.5)"), *AssetData.AssetName.ToString(), *RowName.ToString(), *RegionId.ToString()));
					}
				}
			});
	}

	// Cross-refs resolve against the setup the table is wired into: a row region
	// id outside the region graph is dropped at runtime with a warning
	// (GDD 14.3.5) — legal in play, a defect in data.
	for (const FAssetData& AssetData : FindAssetsOfClass(UEclipseCampaignSetupAsset::StaticClass()))
	{
		const UEclipseCampaignSetupAsset* Setup = Cast<UEclipseCampaignSetupAsset>(AssetData.GetAsset());
		if (Setup == nullptr)
		{
			continue;
		}

		const UDataTable* Liberations = Setup->LiberationInstances.LoadSynchronous();
		if (Liberations == nullptr)
		{
			// Missing table = no liberations, a legal pre-content state (GDD 14.3.5).
			continue;
		}
		++OutAssetsChecked;
		if (Liberations->GetRowStruct() != FEclipseLiberationRow::StaticStruct())
		{
			OutErrors.Add(FString::Printf(TEXT("%s: LiberationInstances table '%s' has the wrong row struct"), *AssetData.AssetName.ToString(), *Liberations->GetName()));
			continue;
		}

		// The two data couplings a liberation silently depends on (review
		// finding): the row can only ever fire if some story row (a) names the
		// same mission id and (b) commits the very beat this row gates on. Both
		// hold today only because one author wrote both tables in one sitting;
		// nothing enforced it, so a rename on either side produced a green suite,
		// a green validator and a dead feature.
		const UDataTable* StoryMissions = Setup->StoryMissions.LoadSynchronous();
		const bool bStoryTableUsable = StoryMissions != nullptr && StoryMissions->GetRowStruct() == FEclipseStoryMissionRow::StaticStruct();
		TSet<FName> StoryMissionIds;
		TSet<FGameplayTag> StoryCompletionBeats;
		if (bStoryTableUsable)
		{
			StoryMissions->ForeachRow<FEclipseStoryMissionRow>(TEXT("CollectStoryCouplings"),
				[&StoryMissionIds, &StoryCompletionBeats](const FName&, const FEclipseStoryMissionRow& Row)
				{
					StoryMissionIds.Add(Row.MissionId);
					if (Row.CompletionBeatTag.IsValid())
					{
						StoryCompletionBeats.Add(Row.CompletionBeatTag);
					}
				});
		}

		const UEclipseRegionGraphAsset* Graph = Setup->RegionGraph.LoadSynchronous();
		Liberations->ForeachRow<FEclipseLiberationRow>(TEXT("ValidateLiberationRefs"),
			[&OutErrors, &AssetData, Graph, bStoryTableUsable, &StoryMissionIds, &StoryCompletionBeats](const FName& RowName, const FEclipseLiberationRow& Row)
			{
				for (const FName& RegionId : Row.RegionIds)
				{
					const bool bKnown = Graph != nullptr && Graph->Regions.ContainsByPredicate(
						[&RegionId](const FEclipseRegionDefinition& D) { return D.RegionId == RegionId; });
					if (!bKnown)
					{
						OutErrors.Add(FString::Printf(TEXT("%s: liberation row '%s' region id '%s' is not in the setup's region graph"), *AssetData.AssetName.ToString(), *RowName.ToString(), *RegionId.ToString()));
					}
				}

				// Only meaningful once a story table exists: before that, an
				// unauthored trigger mission is the normal pre-content state.
				if (!bStoryTableUsable)
				{
					return;
				}
				if (!Row.TriggerMissionId.IsNone() && !StoryMissionIds.Contains(Row.TriggerMissionId))
				{
					OutErrors.Add(FString::Printf(TEXT("%s: liberation row '%s' triggers on mission '%s', which no DT_StoryMissions row authors — the row can never fire"),
						*AssetData.AssetName.ToString(), *RowName.ToString(), *Row.TriggerMissionId.ToString()));
				}
				if (Row.RequiredBeatTag.IsValid() && !StoryCompletionBeats.Contains(Row.RequiredBeatTag))
				{
					OutErrors.Add(FString::Printf(TEXT("%s: liberation row '%s' gates on beat '%s', which no story row commits as its CompletionBeatTag — the gate can never open"),
						*AssetData.AssetName.ToString(), *RowName.ToString(), *Row.RequiredBeatTag.ToString()));
				}
			});
	}

	return OutErrors.Num() - InitialErrors;
}

} // namespace EclipseDataValidators
