#include "Core/EclipseGrayboxBuilder.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Eclipse.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInterface.h"
#include "Quests/EclipseObjectiveTrigger.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	struct FBlockDef { const TCHAR* Label; float X; float Y; float Z; float SX; float SY; float SZ; };
	struct FPointDef { const TCHAR* Id; float X; float Y; };

	// ~200x200 m district: perimeter, two buildings (control-post compound with a
	// west entrance gap; warehouse with an east gap), scattered cover field.
	constexpr FBlockDef Blocks[] = {
		{ TEXT("Floor"), 0, 0, -50, 200, 200, 1 },
		{ TEXT("Wall_N"), 0, 10000, 200, 200, 1, 5 },
		{ TEXT("Wall_S"), 0, -10000, 200, 200, 1, 5 },
		{ TEXT("Wall_E"), 10000, 0, 200, 1, 200, 5 },
		{ TEXT("Wall_W"), -10000, 0, 200, 1, 200, 5 },
		{ TEXT("BldgA_N"), 5000, -1200, 200, 16, 1, 4 },
		{ TEXT("BldgA_S"), 5000, -2800, 200, 16, 1, 4 },
		{ TEXT("BldgA_E"), 5800, -2000, 200, 1, 16, 4 },
		{ TEXT("BldgA_W"), 4200, -2400, 200, 1, 8, 4 },
		{ TEXT("BldgB_N"), -4000, 3800, 200, 12, 1, 4 },
		{ TEXT("BldgB_S"), -4000, 2200, 200, 12, 1, 4 },
		{ TEXT("BldgB_E"), -3200, 3400, 200, 1, 6, 4 },
		{ TEXT("BldgB_W"), -4800, 3000, 200, 1, 12, 4 },
	};

	constexpr FPointDef CoverPoints[] = {
		{ TEXT("Cover"), -6000, -4000 }, { TEXT("Cover"), -4500, -5500 }, { TEXT("Cover"), -2500, -3000 },
		{ TEXT("Cover"), -1000, -5000 }, { TEXT("Cover"), 500, -2500 },   { TEXT("Cover"), 2000, -4000 },
		{ TEXT("Cover"), 3500, -1500 },  { TEXT("Cover"), 1500, 500 },    { TEXT("Cover"), -500, 2000 },
		{ TEXT("Cover"), -2000, 500 },   { TEXT("Cover"), -3500, -500 },  { TEXT("Cover"), 2500, 2500 },
		{ TEXT("Cover"), 4000, 1000 },   { TEXT("Cover"), 5500, 3000 },   { TEXT("Cover"), -6500, 1500 },
		{ TEXT("Cover"), -5000, 4500 },  { TEXT("Cover"), 6500, -4500 },  { TEXT("Cover"), 7000, 500 },
		{ TEXT("Cover"), -7500, -2000 }, { TEXT("Cover"), 0, 6500 },
	};

	constexpr FPointDef Sites[] = {
		{ TEXT("Site_ControlPost"), 5000, -2000 },
		{ TEXT("Site_AlarmRelay"), 5000, 1500 },
		{ TEXT("Site_Crane"), -4000, 3000 },
		{ TEXT("Site_Pens"), -4000, 2600 },
		{ TEXT("Site_Extraction"), -8500, -8500 },
		{ TEXT("Spawn_Checkpoint"), 4600, -2000 },
		{ TEXT("Spawn_Reserve"), 6500, -3500 },
		{ TEXT("Spawn_Yard"), -3600, 3000 },
		{ TEXT("Spawn_Pens"), -4400, 2600 },
		{ TEXT("Spawn_Patrol"), 0, 0 },
	};

	/** Sites that double as objective triggers (SPEC-P1-05 objective primitives). */
	constexpr FPointDef TriggerSites[] = {
		{ TEXT("Site_ControlPost"), 5000, -2000 },
		{ TEXT("Site_AlarmRelay"), 5000, 1500 },
		{ TEXT("Site_Crane"), -4000, 3000 },
		{ TEXT("Site_Pens"), -4000, 2600 },
		{ TEXT("Site_Extraction"), -8500, -8500 },
	};

	constexpr FPointDef EntryPoints[] = {
		{ TEXT("Entry_Main"), -9000, 0 },
		{ TEXT("Entry_Sewer"), 0, -9000 },
		{ TEXT("Entry_Roof"), 8500, 8500 },
	};

	/**
	 * PLACEHOLDER(Part 15.5): early stylized dressing, owner-authorized ahead of
	 * the Phase 2 art pass. Bold cel-banded block colors (Borderlands-leaning:
	 * punchy, readable at command distance) as dynamic instances of the authored
	 * toon master (/Game/Art/M_EclipseToon); the real district replaces all of it
	 * with authored kits. Shade tones are hue-shifted cool, never just darker —
	 * that hue shift is what makes flat cel shading read as painted, not dimmed.
	 */
	// TexPath (optional): CC0 world-aligned albedo multiplied into the cel bands
	// (Content/Art/Textures/SOURCES.md — owner-authorized asset pass). Null =
	// flat cel, bit-identical to the pre-texture look.
	// TexGain normalizes each texture's average luminance back to ~1.0 so the
	// district's histogram — and with it the banked dusk auto-exposure — is
	// unchanged by texturing (first textured round: the darker floor re-metered
	// the whole scene into daylight). TexMix < 1 blends toward the flat cel
	// color where a texture's value range fights the palette.
	struct FPaletteDef { const TCHAR* Prefix; FLinearColor Lit; FLinearColor Shade; const TCHAR* TexPath; float TexScale; float TexGain; float TexMix; };
	const FPaletteDef Palette[] = {
		// TexGain = 1/measured-linear-average (System.Drawing sample pass,
		// 2026-07-22): asphalt .059, concrete .049, metal .031, corrugated .095.
		{ TEXT("Floor"),  FLinearColor(0.165f, 0.150f, 0.160f), FLinearColor(0.055f, 0.052f, 0.080f), TEXT("/Game/Art/Textures/T_asphalt_03_diff.T_asphalt_03_diff"), 700.0f, 16.8f, 0.5f },  // asphalt — dark but never crushed
		{ TEXT("Wall_"),  FLinearColor(0.230f, 0.250f, 0.290f), FLinearColor(0.075f, 0.082f, 0.130f), TEXT("/Game/Art/Textures/T_concrete_block_wall_diff.T_concrete_block_wall_diff"), 500.0f, 20.5f, 0.5f },  // perimeter concrete, cold
		{ TEXT("BldgA"),  FLinearColor(0.560f, 0.160f, 0.085f), FLinearColor(0.200f, 0.045f, 0.085f), TEXT("/Game/Art/Textures/T_metal_plate_diff.T_metal_plate_diff"), 350.0f, 32.5f, 0.5f },  // Dominion post: oxide red, shade to maroon-purple (variation is luminance-only, hue stays palette)
		{ TEXT("BldgB"),  FLinearColor(0.060f, 0.300f, 0.310f), FLinearColor(0.020f, 0.100f, 0.150f), TEXT("/Game/Art/Textures/T_corrugated_iron_02_diff.T_corrugated_iron_02_diff"), 300.0f, 10.5f, 0.45f },  // warehouse: worker teal, shade to deep sea
		{ TEXT("Cover"),  FLinearColor(0.850f, 0.360f, 0.050f), FLinearColor(0.360f, 0.110f, 0.060f), nullptr, 0.0f, 1.0f, 0.0f },  // hazard orange, reads as cover
		{ TEXT("Skyline"), FLinearColor(0.048f, 0.044f, 0.058f), FLinearColor(0.018f, 0.017f, 0.028f), nullptr, 0.0f, 1.0f, 0.0f }, // graphite massing silhouetted in the smog (03.3); the haze adds the aerial fade
		{ TEXT("Glow"),   FLinearColor(2.200f, 1.000f, 0.300f), FLinearColor(2.200f, 1.000f, 0.300f), nullptr, 0.0f, 1.0f, 0.0f },  // sodium-orange window strips — bright enough to survive 15 km of smog
		{ TEXT("Outland"), FLinearColor(0.045f, 0.042f, 0.055f), FLinearColor(0.020f, 0.020f, 0.030f), nullptr, 0.0f, 1.0f, 0.0f }, // industrial plain under the skyline, darker than the district floor
		{ TEXT("DecoLine"), FLinearColor(0.700f, 0.660f, 0.520f), FLinearColor(0.300f, 0.280f, 0.240f), nullptr, 0.0f, 1.0f, 0.0f }, // worn lane paint on the plaza asphalt
		{ TEXT("DecoStain"), FLinearColor(0.070f, 0.062f, 0.075f), FLinearColor(0.028f, 0.026f, 0.038f), nullptr, 0.0f, 1.0f, 0.0f }, // oil/rust staining, darker than the floor mid tone
	};
	const FPaletteDef DefaultPalette = { TEXT(""), FLinearColor(0.35f, 0.35f, 0.38f), FLinearColor(0.12f, 0.12f, 0.16f), nullptr, 0.0f, 1.0f, 0.0f };

	const FPaletteDef& PaletteForLabel(const TCHAR* Label)
	{
		for (const FPaletteDef& Entry : Palette)
		{
			if (FCString::Strnicmp(Label, Entry.Prefix, FCString::Strlen(Entry.Prefix)) == 0)
			{
				return Entry;
			}
		}
		return DefaultPalette;
	}

	/**
	 * One sun definition shared by the light actor AND the toon material's LightDir
	 * parameter — if these ever diverge, material banding and pawn lighting tell
	 * two different stories about where the sun is.
	 */
	// Low dusk sun: vertical faces split hard into lit/shade (the cel read), the
	// floor stays in the mid band (BandLo < sin 25 deg < BandHi in M_EclipseToon).
	const FRotator SunRotation(-25.0f, 55.0f, 0.0f);

	/**
	 * Cel luminance calibration for the dev box: the unlit toon emissive is scaled
	 * so the district occupies the same luminance range a lit surface had under
	 * the banked pass-19 sun (legacy intensity 8 + fill 2.5) — auto-exposure then
	 * lands on the same proven dusk grade, and the palette survives as ratios.
	 * Pinning exposure + physical lux was tried (passes 20-22 forensics) and
	 * fought three unit systems at once on SM5; the RTX pass redoes this in real
	 * physical units with Lumen.
	 */
	const float ToonEmissiveScale = 10.0f;
}

namespace EclipseGraybox
{

bool IsDistrictPresent(UWorld& World)
{
	for (TActorIterator<ATargetPoint> It(&World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Site_ControlPost")))
		{
			return true;
		}
	}
	return false;
}

void BuildDistrict(UWorld& World)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh == nullptr)
	{
		UE_LOG(LogEclipse, Error, TEXT("Graybox: engine cube mesh missing — district not built."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The SM6 target (strong PC) runs full-fidelity extras the SM5 laptop
	// fallback cannot; computed here because the material choice below also
	// depends on it (15.2C + the 15.5 fidelity revision).
	const bool bFullFidelity = World.GetFeatureLevel() >= ERHIFeatureLevel::SM6;

	// One dynamic instance per palette entry (not per block) keeps the dressing cheap.
	// Toon master first (cel bands computed in-material — deterministic on the SM5
	// fallback where scene lights never reach horizontals); engine shape material
	// as the flat-color fallback when the authored asset is absent (GDD 14.3.5).
	// The lit-toon migration experiment (15.5 revision) rides behind
	// -EclipseLitToon on SM6: same banded color as BaseColor so VSM + software
	// Lumen paint real light on top. Never the default until an A/B locks it.
	const bool bLitToon = bFullFidelity && FParse::Param(FCommandLine::Get(), TEXT("EclipseLitToon"));
	UMaterialInterface* ToonMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToon.M_EclipseToon"));
	UMaterialInterface* ToonLitMaterial = bLitToon ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToonLit.M_EclipseToonLit")) : nullptr;
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ToonMaterial == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: M_EclipseToon missing — falling back to flat engine-material dressing (GDD 14.3.5)."));
	}
	if (bLitToon && ToonLitMaterial == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: -EclipseLitToon set but M_EclipseToonLit missing — unlit toon fallback (run Tools/author_toon_material.py)."));
	}
	TMap<uint32, UMaterialInstanceDynamic*> MidByColor;
	auto MidForPalette = [ToonMaterial, ToonLitMaterial, BaseMaterial, &MidByColor, &World](const FPaletteDef& Entry) -> UMaterialInstanceDynamic*
	{
		// Glow strips stay unlit-emissive under every mode — they are light
		// sources, not lit surfaces.
		const bool bWantsLit = ToonLitMaterial != nullptr && FCString::Strnicmp(Entry.Prefix, TEXT("Glow"), 4) != 0;
		UMaterialInterface* Master = bWantsLit ? ToonLitMaterial : (ToonMaterial != nullptr ? ToonMaterial : BaseMaterial);
		if (Master == nullptr)
		{
			return nullptr; // both materials missing = plain blocks, never a crash (GDD 14.3.5)
		}
		UMaterialInstanceDynamic*& Mid = MidByColor.FindOrAdd(GetTypeHash(Entry.Lit.ToFColor(true)));
		if (Mid == nullptr)
		{
			Mid = UMaterialInstanceDynamic::Create(Master, &World);
			if (Master != BaseMaterial)
			{
				Mid->SetVectorParameterValue(TEXT("LitColor"), Entry.Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Entry.Shade);
				// The material's L = -LightDir, so pass the travel direction of the sun.
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
				if (!bWantsLit)
				{
					// Lit variant keeps EmissiveScale 1: BaseColor is albedo,
					// the real lights supply the energy.
					Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
				}
				// CC0 albedo pass: opt in per entry; a missing asset degrades to
				// the flat cel look, never a crash (GDD 14.3.5).
				if (Entry.TexPath != nullptr)
				{
					if (UTexture* Albedo = LoadObject<UTexture>(nullptr, Entry.TexPath))
					{
						Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Albedo);
						Mid->SetScalarParameterValue(TEXT("TexWorldScale"), Entry.TexScale);
						Mid->SetScalarParameterValue(TEXT("AlbedoGain"), Entry.TexGain);
						Mid->SetScalarParameterValue(TEXT("AlbedoMix"), Entry.TexMix);
					}
					else
					{
						UE_LOG(LogEclipse, Warning, TEXT("Graybox: albedo %s missing — flat cel fallback (run Tools/import_polyhaven_textures.py)."), Entry.TexPath);
					}
				}
			}
			else
			{
				Mid->SetVectorParameterValue(TEXT("Color"), Entry.Lit);
			}
		}
		return Mid;
	};

	auto SpawnBlock = [&World, CubeMesh, &Params, &MidForPalette](const TCHAR* Label, const FVector& Location, const FVector& Scale)
	{
		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
		if (Actor != nullptr)
		{
			Actor->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			Actor->SetActorScale3D(Scale);
			Actor->Tags.Add(Label);
			if (UMaterialInstanceDynamic* Mid = MidForPalette(PaletteForLabel(Label)))
			{
				Actor->GetStaticMeshComponent()->SetMaterial(0, Mid);
			}
			// The engine cube's distance field breaks at these non-uniform scales
			// (up to 200:1) and poisons DFAO/DF-shadows into blanket blackness at
			// high scalability. Graybox blocks light via CSM only; the authored
			// district gets proper per-kit distance fields in the art pass.
			Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
			if (FCString::Stricmp(Label, TEXT("Floor")) == 0 ||
				FCString::Strnicmp(Label, TEXT("Skyline"), 7) == 0 ||
				FCString::Strnicmp(Label, TEXT("Glow"), 4) == 0 ||
				FCString::Strnicmp(Label, TEXT("Outland"), 7) == 0 ||
				FCString::Strnicmp(Label, TEXT("Deco"), 4) == 0)
			{
				// Ground slabs have nothing under them; skyline dressing sits
				// kilometers out — VSM pages for backdrop shadows are pure waste.
				Actor->GetStaticMeshComponent()->SetCastShadow(false);
			}
			if (FCString::Strnicmp(Label, TEXT("Deco"), 4) == 0 ||
				FCString::Strnicmp(Label, TEXT("Glow"), 4) == 0)
			{
				// Dressing never collides: lane paint, stains, and light strips
				// must not perturb nav, cover queries, or hitscan (SPEC-P1-05).
				Actor->SetActorEnableCollision(false);
			}
		}
	};

	for (const FBlockDef& Block : Blocks)
	{
		SpawnBlock(Block.Label, FVector(Block.X, Block.Y, Block.Z), FVector(Block.SX, Block.SY, Block.SZ));
	}

	int32 CoverIndex = 0;
	for (const FPointDef& Cover : CoverPoints)
	{
		const bool bRotated = (CoverIndex++ % 2) == 0;
		SpawnBlock(TEXT("Cover"), FVector(Cover.X, Cover.Y, 60.0f),
			FVector(bRotated ? 3.0f : 1.0f, bRotated ? 1.0f : 3.0f, 1.2f));
	}

	// PLACEHOLDER(15.4): first real prop meshes — CC0 Poly Haven FBX restyled
	// through the toon pipeline (mesh-UV albedo, luminance-only; provenance in
	// Content/Art/Textures/SOURCES.md). Dressing tier for now: no collision —
	// a later pass promotes correctly-sized props to real cover WITH the squad
	// scenario suite re-run (SPEC-P1-06); missing assets degrade to nothing.
	{
		struct FPropDef { const TCHAR* Label; const TCHAR* MeshPath; const TCHAR* TexPath; float TexGain; FLinearColor Lit; FLinearColor Shade; };
		const FPropDef Props[] = {
			{ TEXT("Prop_Barrel"), TEXT("/Game/Art/Props/Barrel_01.Barrel_01"), TEXT("/Game/Art/Textures/T_Barrel_01_diff.T_Barrel_01_diff"), 17.6f, FLinearColor(0.160f, 0.100f, 0.070f), FLinearColor(0.055f, 0.042f, 0.062f) },
			{ TEXT("Prop_Barrier"), TEXT("/Game/Art/Props/concrete_road_barrier.concrete_road_barrier"), TEXT("/Game/Art/Textures/T_concrete_road_barrier_diff.T_concrete_road_barrier_diff"), 6.7f, FLinearColor(0.26f, 0.27f, 0.30f), FLinearColor(0.085f, 0.090f, 0.140f) },
			{ TEXT("Prop_Crate"), TEXT("/Game/Art/Props/plastic_crate_03.plastic_crate_03"), TEXT("/Game/Art/Textures/T_plastic_crate_03_diff.T_plastic_crate_03_diff"), 8.0f, FLinearColor(0.080f, 0.280f, 0.300f), FLinearColor(0.030f, 0.100f, 0.150f) },
		};

		auto SpawnProp = [&World, &Params](UStaticMesh* Mesh, UMaterialInstanceDynamic* Mid, const FVector& Location, float YawDeg, float Scale)
		{
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator(0.0f, YawDeg, 0.0f), Params);
			if (Actor != nullptr)
			{
				Actor->SetMobility(EComponentMobility::Movable);
				Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
				Actor->SetActorScale3D(FVector(Scale));
				if (Mid != nullptr)
				{
					// Every slot: imported FBX props carry multiple material
					// slots; any slot left on the default engine material
					// renders as pale gray and breaks the palette (first prop
					// round, camera 2 — the "white barrels").
					for (int32 SlotIndex = 0; SlotIndex < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++SlotIndex)
					{
						Actor->GetStaticMeshComponent()->SetMaterial(SlotIndex, Mid);
					}
				}
				Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
				Actor->SetActorEnableCollision(false);
			}
		};

		FRandomStream PropRng(211);
		for (int32 PropIndex = 0; PropIndex < static_cast<int32>(UE_ARRAY_COUNT(Props)); ++PropIndex)
		{
			const FPropDef& Prop = Props[PropIndex];
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Prop.MeshPath);
			if (Mesh == nullptr)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: prop %s missing — skipped (run Tools/import_polyhaven_props.py)."), Prop.MeshPath);
				continue;
			}
			UTexture* Tex = LoadObject<UTexture>(nullptr, Prop.TexPath);
			UMaterialInstanceDynamic* Mid = nullptr;
			if (ToonMaterial != nullptr)
			{
				Mid = UMaterialInstanceDynamic::Create(ToonMaterial, &World);
				Mid->SetVectorParameterValue(TEXT("LitColor"), Prop.Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Prop.Shade);
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
				Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
				Mid->SetScalarParameterValue(TEXT("AlbedoGain"), Prop.TexGain);
				if (Tex != nullptr)
				{
					Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Tex);
					Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.85f);
				}
			}
			UE_LOG(LogEclipse, Display, TEXT("Graybox: prop %s slots=%d tex=%s mid=%s"),
				Prop.Label, Mesh->GetStaticMaterials().Num(),
				Tex != nullptr ? TEXT("ok") : TEXT("MISSING"),
				Mid != nullptr ? TEXT("ok") : TEXT("null"));

			if (PropIndex == 0)
			{
				// Barrel clusters: warehouse yard, control-post rear, road side.
				const FVector Centers[] = { FVector(-3300, 3000, 0), FVector(6300, -2600, 0), FVector(2100, 750, 0), FVector(-8600, 8300, 0) };
				for (const FVector& Center : Centers)
				{
					const int32 Count = 3 + (PropRng.RandRange(0, 1));
					for (int32 Index = 0; Index < Count; ++Index)
					{
						SpawnProp(Mesh, Mid,
							Center + FVector(PropRng.FRandRange(-200.0f, 200.0f), PropRng.FRandRange(-200.0f, 200.0f), 0.0f),
							PropRng.FRandRange(0.0f, 360.0f), 1.4f);
					}
				}
			}
			else if (PropIndex == 1)
			{
				// Checkpoint barriers flanking the EW artery near the compound approach.
				for (int32 Index = 0; Index < 6; ++Index)
				{
					const float X = -5500.0f + Index * 1700.0f;
					const float Y = (Index % 2 == 0) ? 640.0f : -640.0f;
					SpawnProp(Mesh, Mid, FVector(X, Y, 0), (Index % 2 == 0) ? 8.0f : 172.0f, 1.8f);
				}
			}
			else
			{
				// Crate stacks in the warehouse yard: two on the ground, one on top.
				const FVector Stacks[] = { FVector(-4300, 3350, 0), FVector(-3700, 2500, 0) };
				for (const FVector& Base : Stacks)
				{
					SpawnProp(Mesh, Mid, Base, PropRng.FRandRange(0.0f, 360.0f), 2.2f);
					SpawnProp(Mesh, Mid, Base + FVector(150, 40, 0), PropRng.FRandRange(0.0f, 360.0f), 2.2f);
					SpawnProp(Mesh, Mid, Base + FVector(75, 20, 92), PropRng.FRandRange(0.0f, 360.0f), 2.2f);
				}
			}
		}
	}

	// PLACEHOLDER(15.5/03.3): Kessara skyline massing OUTSIDE the playable
	// perimeter — "silhouetted crane forests" in the amber smog. Pure backdrop:
	// no nav, no cover, no mission space touched; the art pass replaces it with
	// authored kits. Deterministic seed so every machine builds the same city
	// (reproducible-from-code graybox, SPEC-P1-05).
	{
		FRandomStream SkylineRng(503); // 503 AE — the present year (00_INDEX)

		// Industrial plain under the backdrop, one slab: without it the massing
		// floats over sky beyond the district floor's ±10 km edge.
		SpawnBlock(TEXT("Outland"), FVector(0, 0, -80.0f), FVector(620.0f, 620.0f, 1.0f));

		// Mega-blocks: factory hulks ringing the district at 13–26 km.
		for (int32 Index = 0; Index < 56; ++Index)
		{
			const float Angle = SkylineRng.FRandRange(0.0f, 2.0f * UE_PI);
			const float Radius = SkylineRng.FRandRange(13000.0f, 26000.0f);
			const float Height = SkylineRng.FRandRange(9.0f, 34.0f);
			SpawnBlock(TEXT("Skyline"),
				FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height * 50.0f - 80.0f),
				FVector(SkylineRng.FRandRange(9.0f, 26.0f), SkylineRng.FRandRange(9.0f, 26.0f), Height));

			// Sodium window strips on roughly a third of the hulks: worker light
			// against the graphite (03.3's sodium-orange vs. Dominion white-gold).
			if (SkylineRng.FRand() < 0.35f)
			{
				// 1600 units inward clears the widest hulk half-extent (26*50), so
				// strips sit proud of the facade instead of embedded in it.
				const float StripHeight = SkylineRng.FRandRange(150.0f, Height * 50.0f);
				SpawnBlock(TEXT("Glow"),
					FVector(FMath::Cos(Angle) * (Radius - 1600.0f), FMath::Sin(Angle) * (Radius - 1600.0f), StripHeight),
					FVector(0.4f, SkylineRng.FRandRange(3.0f, 7.0f), 0.45f));
			}
		}

		// Chimney stacks: the forge-world's vertical punctuation.
		for (int32 Index = 0; Index < 18; ++Index)
		{
			const float Angle = SkylineRng.FRandRange(0.0f, 2.0f * UE_PI);
			const float Radius = SkylineRng.FRandRange(12000.0f, 22000.0f);
			const float Height = SkylineRng.FRandRange(26.0f, 44.0f);
			SpawnBlock(TEXT("Skyline"),
				FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height * 50.0f - 80.0f),
				FVector(SkylineRng.FRandRange(1.2f, 2.0f), SkylineRng.FRandRange(1.2f, 2.0f), Height));
		}

		// Street dressing + checkpoint light strips INSIDE the district: a
		// readable occupation story on the empty plaza (15.5 "occupation &
		// story"), all no-collision deco — nav/cover/missions untouched.
		{
			// East-west artery: Entry_Main to the control-post compound.
			SpawnBlock(TEXT("DecoLine"), FVector(0, 460, 3), FVector(190.0f, 0.16f, 0.06f));
			SpawnBlock(TEXT("DecoLine"), FVector(0, -460, 3), FVector(190.0f, 0.16f, 0.06f));
			for (int32 Index = 0; Index < 24; ++Index)
			{
				SpawnBlock(TEXT("DecoLine"), FVector(-9200.0f + Index * 800.0f, 0, 3), FVector(1.6f, 0.14f, 0.06f));
			}
			// North-south cross street toward the warehouse yard.
			SpawnBlock(TEXT("DecoLine"), FVector(-4460, 0, 3), FVector(0.16f, 190.0f, 0.06f));
			SpawnBlock(TEXT("DecoLine"), FVector(-3540, 0, 3), FVector(0.16f, 190.0f, 0.06f));
			// Oil and rust staining, biased toward the driven crossing.
			FRandomStream DecoRng(77);
			for (int32 Index = 0; Index < 14; ++Index)
			{
				SpawnBlock(TEXT("DecoStain"),
					FVector(DecoRng.FRandRange(-8000.0f, 8000.0f), DecoRng.FRandRange(-7000.0f, 7000.0f), 2.0f),
					FVector(DecoRng.FRandRange(2.0f, 6.5f), DecoRng.FRandRange(2.0f, 6.5f), 0.04f));
			}
			// Sodium checkpoint strips on the inner wall faces (03.3: sodium
			// checkpoints vs. Dominion white-gold) — three per wall.
			for (int32 Index = -1; Index <= 1; ++Index)
			{
				const float Along = Index * 6000.0f;
				SpawnBlock(TEXT("Glow"), FVector(Along, 9935, 360), FVector(3.0f, 0.12f, 0.35f));
				SpawnBlock(TEXT("Glow"), FVector(Along, -9935, 360), FVector(3.0f, 0.12f, 0.35f));
				SpawnBlock(TEXT("Glow"), FVector(9935, Along, 360), FVector(0.12f, 3.0f, 0.35f));
				SpawnBlock(TEXT("Glow"), FVector(-9935, Along, 360), FVector(0.12f, 3.0f, 0.35f));
			}
		}

		// Crane gantries: two legs + a long beam, alternating axis — the layered
		// silhouette Kessara's identity hangs on. Axis-aligned is right for a
		// forge world's orthogonal yards.
		for (int32 Index = 0; Index < 12; ++Index)
		{
			const float Angle = SkylineRng.FRandRange(0.0f, 2.0f * UE_PI);
			const float Radius = SkylineRng.FRandRange(11500.0f, 17000.0f);
			const FVector Base(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
			const bool bAlongX = (Index % 2) == 0;
			const float LegHeight = SkylineRng.FRandRange(18.0f, 28.0f);
			const float Span = SkylineRng.FRandRange(1600.0f, 2600.0f);
			const FVector LegOffset = bAlongX ? FVector(Span * 0.5f, 0, 0) : FVector(0, Span * 0.5f, 0);
			SpawnBlock(TEXT("Skyline"), Base + LegOffset + FVector(0, 0, LegHeight * 50.0f - 80.0f), FVector(1.4f, 1.4f, LegHeight));
			SpawnBlock(TEXT("Skyline"), Base - LegOffset + FVector(0, 0, LegHeight * 50.0f - 80.0f), FVector(1.4f, 1.4f, LegHeight));
			SpawnBlock(TEXT("Skyline"), Base + FVector(0, 0, LegHeight * 100.0f - 140.0f),
				bAlongX ? FVector(Span / 100.0f + 6.0f, 1.6f, 1.6f) : FVector(1.6f, Span / 100.0f + 6.0f, 1.6f));
		}
	}

	for (const FPointDef& Site : Sites)
	{
		if (ATargetPoint* Point = World.SpawnActor<ATargetPoint>(FVector(Site.X, Site.Y, 120.0f), FRotator::ZeroRotator, Params))
		{
			Point->Tags.Add(Site.Id);
		}
	}

	for (const FPointDef& Site : TriggerSites)
	{
		if (AEclipseObjectiveTrigger* Trigger = World.SpawnActor<AEclipseObjectiveTrigger>(FVector(Site.X, Site.Y, 120.0f), FRotator::ZeroRotator, Params))
		{
			Trigger->SiteId = Site.Id;
		}
	}

	for (const FPointDef& Entry : EntryPoints)
	{
		if (APlayerStart* Start = World.SpawnActor<APlayerStart>(FVector(Entry.X, Entry.Y, 200.0f), FRotator::ZeroRotator, Params))
		{
			Start->Tags.Add(Entry.Id);
		}
	}

	// PLACEHOLDER(Part 15.3/15.5): the district's mood pass, owner-authorized ahead
	// of Phase 2 — a low warm sun through Kessara smog, punchy grade, ink outlines.
	// Everything below rides standard UE features behind scalability (15.10); the
	// authored art pass on target hardware replaces these numbers wholesale.

	// One authoritative mood: purge the host map's own sun/sky/fog first. Entry
	// ships a horizon-level sun that paints facades warm but leaves every
	// horizontal surface black — with two suns the district's look is luck.
	{
		TArray<AActor*> Stale;
		for (TActorIterator<ADirectionalLight> It(&World); It; ++It) { Stale.Add(*It); }
		for (TActorIterator<ASkyLight> It(&World); It; ++It) { Stale.Add(*It); }
		for (TActorIterator<ASkyAtmosphere> It(&World); It; ++It) { Stale.Add(*It); }
		for (TActorIterator<AExponentialHeightFog> It(&World); It; ++It) { Stale.Add(*It); }
		for (AActor* Actor : Stale)
		{
			Actor->Destroy();
		}
	}

	// Low industrial sun; drives the SkyAtmosphere so the horizon carries the mood.
	// Mid-afternoon sun: warm but high enough that shade sides stay readable —
	// the stylized look wants soft, lifted shadows, not noir silhouettes.
	if (ADirectionalLight* Sun = World.SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), SunRotation, Params))
	{
		if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			// ADirectionalLight ships static-mobility; a runtime spawn keeps the
			// default horizontal direction unless made movable and re-rotated —
			// which lit facades and left every floor black (pass-8 forensics).
			// SunRotation is shared with the toon material's LightDir (see above).
			SunComponent->SetMobility(EComponentMobility::Movable);
			Sun->SetActorRotation(SunRotation);
			// Legacy intensity 8 pairs with the toon emissive x10 range under auto
			// exposure. Atmosphere re-linked: the old decouple protected LIT ground
			// from transmittance loss (pass-15) — the district is unlit now, so the
			// sun may power a real dusk sky and only lights pawns.
			SunComponent->SetIntensity(8.0f);
			SunComponent->SetLightColor(FLinearColor(1.0f, 0.87f, 0.70f));
			SunComponent->SetAtmosphereSunLight(true);
			SunComponent->SetVolumetricScatteringIntensity(2.0f);
			// SM5 laptop: the CSM path blankets the 200x-scaled ground slab in
			// shadow no matter the caster set (passes 5-16 forensics) — sun ships
			// shadowless there. SM6: VSM shadows return; the unlit district cannot
			// darken (emissive ignores shadowing), but the volumetric smog CAN —
			// buildings now cut real light shafts through the haze (15.5 revision).
			SunComponent->SetCastShadows(bFullFidelity);
			UE_LOG(LogEclipse, Display, TEXT("Graybox: sun direction %s (movable)."), *SunComponent->GetDirection().ToString());
		}
	}

	World.SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Params);

	// SM6 only: the real captured skylight the fill light stands in for on SM5
	// (the laptop's realtime capture fed horizontal glare + a black zenith,
	// passes 3-14). Lights pawns/props; the unlit district ignores it.
	if (bFullFidelity)
	{
		if (ASkyLight* Sky = World.SpawnActor<ASkyLight>(FVector(0, 0, 400), FRotator::ZeroRotator, Params))
		{
			if (USkyLightComponent* SkyComponent = Sky->GetLightComponent())
			{
				SkyComponent->SetMobility(EComponentMobility::Movable);
				SkyComponent->SetRealTimeCaptureEnabled(true);
				SkyComponent->SetIntensity(1.0f);
			}
		}
	}

	// Fill light instead of a captured skylight: on this box's SM5 fallback the
	// realtime sky capture feeds horizontal glare and a black zenith (floors go
	// dark, facades blow out — passes 3-14 forensics). A soft cool shadowless
	// counter-sun gives the flat, readable stylized shade the direction wants;
	// the strong PC swaps this for a real captured skylight + Lumen bounce.
	if (ADirectionalLight* Fill = World.SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-35.0f, 235.0f, 0), Params))
	{
		if (UDirectionalLightComponent* FillComponent = Cast<UDirectionalLightComponent>(Fill->GetLightComponent()))
		{
			FillComponent->SetMobility(EComponentMobility::Movable);
			Fill->SetActorRotation(FRotator(-35.0f, 235.0f, 0));
			FillComponent->SetIntensity(2.5f); // pawn fill in the banked pass-19 units; the unlit district ignores it
			FillComponent->SetLightColor(FLinearColor(0.55f, 0.65f, 0.85f));
			FillComponent->SetCastShadows(false);
			FillComponent->SetAtmosphereSunLight(false);
		}
	}

	// Kessara smog: warm sodium haze with volumetrics so the sun shafts read.
	if (AExponentialHeightFog* Fog = World.SpawnActor<AExponentialHeightFog>(FVector(0, 0, -50), FRotator::ZeroRotator, Params))
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			// Haze, not soup: the district must read across its full 200 m at
			// command distance (15.5); the smog hugs the ground via the falloff.
			FogComponent->SetFogDensity(0.006f);
			FogComponent->SetFogHeightFalloff(0.2f);
			FogComponent->SetFogInscatteringColor(FLinearColor(0.42f, 0.32f, 0.24f));
			// SM5 laptop: volumetric fog receives no sun on the fallback path and
			// extinguishes the whole ground plane to black — plain exponential
			// haze there. SM6: real volumetric smog, so the shadowed sun draws
			// shafts through the crane-and-compound silhouettes (Kessara identity
			// 03.3: amber smog; 15.5 revision: more atmosphere within the style).
			FogComponent->SetVolumetricFog(bFullFidelity);
		}
	}

	// Graphic-novel grade: locked-ish exposure (no graybox swim), saturation punch,
	// gentle vignette, and the authored ink-outline blendable when it exists.
	if (APostProcessVolume* Post = World.SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
	{
		Post->bUnbound = true;
		FPostProcessSettings& Settings = Post->Settings;
		// Default histogram auto-exposure (the banked pass-19 regime — pinned
		// EV100 + physical lux fought three unit systems at once on this SM5 box,
		// passes 20-22 forensics). Local exposure stays neutralized so it cannot
		// re-contrast the flat cel bands.
		// Pull the histogram key down: without it the mostly-bright emissive
		// district washes to pastel (pass-27 forensics) — the dusk look wants
		// saturated mids, not chalk.
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = -1.0f;
		Settings.bOverride_LocalExposureHighlightContrastScale = true;
		Settings.LocalExposureHighlightContrastScale = 1.0f;
		Settings.bOverride_LocalExposureShadowContrastScale = true;
		Settings.LocalExposureShadowContrastScale = 1.0f;
		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = 0.45f; // 15.5 revision: punchier bloom within the graphic-novel look
		// Subtle film grain per the 15.5 fidelity revision — texture, not noise;
		// SSAO is deliberately absent: it is a no-op on the unlit emissive
		// district and returns with the lit-toon migration.
		Settings.bOverride_FilmGrainIntensity = true;
		Settings.FilmGrainIntensity = 0.07f;
		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(1.22f, 1.22f, 1.22f, 1.0f);
		Settings.bOverride_ColorContrast = true;
		Settings.ColorContrast = FVector4(1.04f, 1.04f, 1.04f, 1.0f);
		Settings.bOverride_VignetteIntensity = true;
		Settings.VignetteIntensity = 0.35f;

		// Ink lines: PP_EclipseInk is the Tools/author_outline_material.py build —
		// a verified scene passthrough (zero edges = untouched frame). The old
		// PP_EclipseOutline asset replaced the whole frame with its tint and masked
		// every material change from pass 20-26; it stays on disk unused. Until the
		// script has authored PP_EclipseInk, the district renders without lines.
		if (UMaterialInterface* Outline = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/PP_EclipseInk.PP_EclipseInk")))
		{
			Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, Outline));
		}
		else
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: PP_EclipseInk not authored yet — district renders without ink lines (GDD 14.3.5; run Tools/author_outline_material.py)."));
		}
	}

	UE_LOG(LogEclipse, Display, TEXT("Graybox: district built (%d blocks, %d cover, %d sites, %d entries)."),
		static_cast<int32>(UE_ARRAY_COUNT(Blocks)), static_cast<int32>(UE_ARRAY_COUNT(CoverPoints)),
		static_cast<int32>(UE_ARRAY_COUNT(Sites)), static_cast<int32>(UE_ARRAY_COUNT(EntryPoints)));
}

} // namespace EclipseGraybox
