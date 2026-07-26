#include "EclipseDataValidators.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Base/EclipsePrepTypes.h"
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

int32 ValidateBodyDefTables(TArray<FString>& OutErrors, int32& OutAssetsChecked)
{
	const int32 InitialErrors = OutErrors.Num();
	OutAssetsChecked = 0;

	for (const FAssetData& AssetData : FindAssetsOfClass(UDataTable::StaticClass()))
	{
		const UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset());
		if (Table == nullptr || Table->GetRowStruct() != FEclipseBodyDefRow::StaticStruct())
		{
			continue;
		}
		++OutAssetsChecked;

		Table->ForeachRow<FEclipseBodyDefRow>(TEXT("ValidateBodyDefs"),
			[&OutErrors, &AssetData](const FName& RowName, const FEclipseBodyDefRow& Row)
			{
				const FString Where = FString::Printf(TEXT("%s: lichaam '%s'"),
					*AssetData.AssetName.ToString(), *RowName.ToString());

				// ZIJWAARTSE CYCLI (locomotie-audit punt 3, gerepareerd 26-07 avond).
				//
				// Vijf van de negen lichamen hadden er geen en schoven zijwaarts met
				// een vooruit-pas onder hun voeten. Sinds de skeletten als compatibel
				// geregistreerd zijn (Tools/link_compatible_skeletons.py) kan een
				// anim-arm pack ze lenen van een donor.
				//
				// Als EIS en niet als telling: een lichaam zonder zijcyclus is een
				// zichtbaar defect, en dat hoort rood te staan in plaats van in een
				// audit-regel die iemand moet lezen.
				// PER RICHTING en niet per tempo, want de code vult het tempo zelf
				// aan: ApplyBodyDef laat Walk* terugvallen op Run* en andersom
				// (FillFrom, EclipseCharacter.cpp). Eisen dat BEIDE er staan zou
				// data eisen die de runtime met opzet invult — en dan is de
				// validator fout, niet de tabel.
				//
				// Eerste versie deed dat wel, en zette de SPELER rood: Belica levert
				// alleen jog-takes en helemaal geen wandelcyclus. Precies de fout die
				// een validator hoort te vermijden, want hij leert je een defect te
				// negeren dat er niet is.
				auto MissingBoth = [](const TSoftObjectPtr<UAnimSequence>& Walk,
					const TSoftObjectPtr<UAnimSequence>& Run)
				{ return Walk.IsNull() && Run.IsNull(); };

				if (MissingBoth(Row.WalkLeftAnim, Row.RunLeftAnim)
					|| MissingBoth(Row.WalkRightAnim, Row.RunRightAnim))
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s heeft in GEEN van beide tempo's een zijwaartse cyclus — dat lichaam schuift zijwaarts met een vooruit-pas"),
						*Where));
				}
				if (MissingBoth(Row.WalkBackAnim, Row.RunBackAnim))
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s heeft geen ACHTERUIT-cyclus — sinds het camera-relatieve model loop je echt achteruit"),
						*Where));
				}
			});
	}

	return OutErrors.Num() - InitialErrors;
}

int32 ValidateLoadoutTables(TArray<FString>& OutErrors, int32& OutAssetsChecked)
{
	const int32 InitialErrors = OutErrors.Num();
	OutAssetsChecked = 0;

	// Alle wapenrijnamen die er zijn, uit ELKE wapentabel. Op naam en niet op één
	// tabel: welke tabel er verscheept wordt is een setup-detail, en een loadout
	// die naar een bestaand wapen wijst hoort te kloppen ongeacht waar dat wapen
	// staat.
	TSet<FName> KnownWeapons;
	for (const FAssetData& AssetData : FindAssetsOfClass(UDataTable::StaticClass()))
	{
		const UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset());
		if (Table != nullptr && Table->GetRowStruct() == FEclipseWeaponRow::StaticStruct())
		{
			for (const FName& RowName : Table->GetRowNames())
			{
				KnownWeapons.Add(RowName);
			}
		}
	}

	for (const FAssetData& AssetData : FindAssetsOfClass(UDataTable::StaticClass()))
	{
		const UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset());
		if (Table == nullptr || Table->GetRowStruct() != FEclipseLoadoutOptionRow::StaticStruct())
		{
			continue;
		}
		++OutAssetsChecked;

		Table->ForeachRow<FEclipseLoadoutOptionRow>(TEXT("ValidateLoadouts"),
			[&OutErrors, &AssetData, &KnownWeapons](const FName& RowName, const FEclipseLoadoutOptionRow& Row)
			{
				const FString Where = FString::Printf(TEXT("%s: loadout '%s'"),
					*AssetData.AssetName.ToString(), *RowName.ToString());

				auto CheckWeapon = [&OutErrors, &Where, &KnownWeapons](FName Weapon, const TCHAR* Slot)
				{
					if (Weapon.IsNone())
					{
						OutErrors.Add(FString::Printf(
							TEXT("%s noemt geen %s — dan val je terug op de eerste rij van DT_Weapons en doet je keuze niets"),
							*Where, Slot));
						return;
					}
					if (!KnownWeapons.Contains(Weapon))
					{
						OutErrors.Add(FString::Printf(
							TEXT("%s noemt %s '%s' en dat wapen bestaat in geen enkele wapentabel"),
							*Where, Slot, *Weapon.ToString()));
					}
				};
				CheckWeapon(Row.PrimaryWeapon, TEXT("primair wapen"));
				CheckWeapon(Row.SidearmWeapon, TEXT("sidearm"));

				// Twee identieke slots betekent dat wisselen niets doet, en dat is
				// erger dan geen tweede wapen: de knop reageert wel en er verandert
				// niets.
				if (!Row.PrimaryWeapon.IsNone() && Row.PrimaryWeapon == Row.SidearmWeapon)
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s heeft twee keer '%s' — dan reageert de wisselknop wel en verandert er niets"),
						*Where, *Row.PrimaryWeapon.ToString()));
				}
			});
	}

	return OutErrors.Num() - InitialErrors;
}

int32 ValidateWeaponTables(TArray<FString>& OutErrors, int32& OutAssetsChecked)
{
	const int32 InitialErrors = OutErrors.Num();
	OutAssetsChecked = 0;

	for (const FAssetData& AssetData : FindAssetsOfClass(UDataTable::StaticClass()))
	{
		const UDataTable* Table = Cast<UDataTable>(AssetData.GetAsset());
		if (Table == nullptr || Table->GetRowStruct() != FEclipseWeaponRow::StaticStruct())
		{
			continue;
		}
		++OutAssetsChecked;

		TArray<TPair<FName, const FEclipseWeaponRow*>> Rows;
		Table->ForeachRow<FEclipseWeaponRow>(TEXT("ValidateWeapons"),
			[&OutErrors, &AssetData, &Rows](const FName& RowName, const FEclipseWeaponRow& Row)
			{
				Rows.Emplace(RowName, &Row);
				const FString Where = FString::Printf(TEXT("%s: wapen '%s'"), *AssetData.AssetName.ToString(), *RowName.ToString());

				// Een wapen zonder rolomschrijving is een wapen zonder bedoeling.
				if (Row.RoleSummary.IsEmpty())
				{
					OutErrors.Add(FString::Printf(TEXT("%s heeft geen RoleSummary — waar is dit wapen voor?"), *Where));
				}
				// Mikken moet nauwkeuriger zijn dan de heup, anders is mikken straf
				// zonder beloning: het kost sinds 26-07 al snelheid.
				if (Row.AimSpreadDegrees >= Row.HipSpreadDegrees)
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s: mikken (%.2f gr) is niet nauwkeuriger dan de heup (%.2f gr) — mikken kost al snelheid, dus dit is straf zonder beloning"),
						*Where, Row.AimSpreadDegrees, Row.HipSpreadDegrees));
				}
				// De geluidsfamilie moet bestaan. Een tikfout hier maakt een wapen
				// niet luidruchtig-verkeerd maar STIL, en dat merk je pas in het spel.
				static const TSet<FName> KnownFamilies = {
					TEXT("AssaultRifle"), TEXT("Handgun"), TEXT("Shotgun"), TEXT("GrenadeLauncher") };
				if (!KnownFamilies.Contains(Row.SoundFamily))
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s: geluidsfamilie '%s' bestaat niet in FreeWeaponSounds — dat wapen valt terug op de losse cue"),
						*Where, *Row.SoundFamily.ToString()));
				}

				// EEN DEMPER MOET IETS BETEKENEN. Als een gedempt schot verder draagt
				// dan een vijand kan kijken (2500 cm, DT_EnemyArchetypes), verraadt het
				// je aan iemand die je niet eens kon zien — en dan is de demper alleen
				// een ander timbre. Dit is de enige eis die het genre echt deelt.
				constexpr float EnemyPerceptionCm = 2500.0f;
				if (Row.bSuppressed && Row.GunshotAlertRadiusCm >= EnemyPerceptionCm)
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s is gedempt maar alarmeert tot %.0f cm — dat is verder dan een vijand kan kijken (%.0f cm), dus de demper doet niets"),
						*Where, Row.GunshotAlertRadiusCm, EnemyPerceptionCm));
				}

				// Afval moet binnen het bereik beginnen, anders bestaat hij niet.
				if (Row.FalloffStartCm >= Row.RangeCm)
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s: schade-afval begint op %.0f cm en het bereik is %.0f cm — het afval gebeurt dus nooit"),
						*Where, Row.FalloffStartCm, Row.RangeCm));
				}
				if (Row.MagazineSize > 0 && Row.ReloadSeconds <= 0.0f)
				{
					OutErrors.Add(FString::Printf(TEXT("%s heeft een magazijn maar herlaadt in nul seconden"), *Where));
				}
			});

		// DE BELANGRIJKSTE CONTROLE: zijn het rollen of varianten?
		//
		// De owner-opdracht is expliciet: "mogen niet als varianten van hetzelfde
		// voelen". Runtime merkt daar niets van — twee identieke wapens werken
		// prima — dus dit is de enige plek waar het kan opvallen.
		//
		// Vier assen die samen de ROL bepalen. Wijken twee wapens op geen enkele
		// daarvan meer dan 25% af, dan zijn het varianten.
		for (int32 A = 0; A < Rows.Num(); ++A)
		{
			for (int32 B = A + 1; B < Rows.Num(); ++B)
			{
				const FEclipseWeaponRow& L = *Rows[A].Value;
				const FEclipseWeaponRow& R = *Rows[B].Value;
				auto FarApart = [](float X, float Y)
				{
					const float Larger = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
					return Larger <= KINDA_SMALL_NUMBER || FMath::Abs(X - Y) / Larger > 0.25f;
				};
				const bool bDistinct = FarApart(L.Damage, R.Damage)
					|| FarApart(L.FireInterval, R.FireInterval)
					|| FarApart(L.RangeCm, R.RangeCm)
					|| FarApart(L.AimSpreadDegrees, R.AimSpreadDegrees);
				if (!bDistinct)
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s: '%s' en '%s' verschillen op geen enkele as meer dan 25%% (schade, cadans, bereik, mikspreiding) — dat zijn varianten en geen rollen"),
						*AssetData.AssetName.ToString(), *Rows[A].Key.ToString(), *Rows[B].Key.ToString()));
				}
			}
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
